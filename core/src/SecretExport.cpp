// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonDocument>

#include "Aead.h"
#include "CryptoError.h"
#include "Random.h"
#include "SecretExport.h"
#include "SecretStore.h"

#include <cmath>

namespace SecretExportKeys {
inline constexpr auto Version = QLatin1String("v");
inline constexpr auto Kind = QLatin1String("kind");
inline constexpr auto Kdf = QLatin1String("kdf");
inline constexpr auto KdfParameters = QLatin1String("kdfParams");
inline constexpr auto Salt = QLatin1String("salt");
inline constexpr auto Nonce = QLatin1String("nonce");
inline constexpr auto Ciphertext = QLatin1String("ct");
inline constexpr auto Secrets = QLatin1String("secrets");
inline constexpr auto KindValue = QLatin1String("veyon-secrets");
inline constexpr auto Scrypt = QLatin1String("scrypt");
inline constexpr auto Argon2id = QLatin1String("argon2id");
inline constexpr auto ScryptCost = QLatin1String("N");
inline constexpr auto ScryptBlockSize = QLatin1String("r");
inline constexpr auto Parallelism = QLatin1String("p");
inline constexpr auto Argon2idIterations = QLatin1String("t");
inline constexpr auto Argon2idMemory = QLatin1String("m");
}

static constexpr auto Cipher = Veyon::Crypto::Aead::Algorithm::Aes256Gcm;


static std::optional<qint64> integerValue(const QJsonValue& value, qint64 minimum, qint64 maximum)
{
	if (value.isDouble() == false)
	{
		return {};
	}

	const auto number = value.toDouble();
	if (std::isfinite(number) == false || std::floor(number) != number ||
		number < double(minimum) || number > double(maximum))
	{
		return {};
	}

	return qint64(number);
}



static std::optional<QByteArray> base64Value(const QJsonValue& value, qsizetype minimumSize, qsizetype maximumSize)
{
	// base64 is 4/3 of the decoded size: refuse oversized input before decoding
	if (value.isString() == false || value.toString().size() > (maximumSize / 3 + 1) * 4)
	{
		return {};
	}

	const auto decodingResult = QByteArray::fromBase64Encoding(value.toString().toLatin1(),
															   QByteArray::AbortOnBase64DecodingErrors);
	if (decodingResult.decodingStatus != QByteArray::Base64DecodingStatus::Ok ||
		decodingResult.decoded.size() < minimumSize || decodingResult.decoded.size() > maximumSize)
	{
		return {};
	}

	return decodingResult.decoded;
}



bool SecretExport::isAcceptablePassphrase(Veyon::Crypto::ByteView passphrase)
{
	if (passphrase.size() > Veyon::Crypto::PasswordKdf::MaximumPasswordSize)
	{
		return false;
	}

	const auto text = QString::fromUtf8(passphrase.data(), passphrase.size());
	const auto isValidUtf8 = text.toUtf8() == QByteArray::fromRawData(passphrase.data(), passphrase.size());

	return isValidUtf8 && text.size() >= MinimumPassphraseLength;
}



std::optional<QJsonObject> SecretExport::encrypt(const Secrets& secrets, Veyon::Crypto::ByteView passphrase, Kdf kdf)
{
	if (isAcceptablePassphrase(passphrase) == false)
	{
		vWarning() << "passphrase too short";
		return {};
	}

	if (secrets.size() > MaximumSecretCount)
	{
		vWarning() << "too many secrets";
		return {};
	}

	QJsonObject encodedSecrets;
	for (const auto& [key, value] : secrets)
	{
		if (SecretStore::isValidKey(key) == false)
		{
			vWarning() << "invalid secret key";
			return {};
		}
		auto unprotectedValue = value.toUnprotectedByteArray();
		encodedSecrets[key] = QString::fromLatin1(unprotectedValue.toBase64());
		unprotectedValue.fill(0);
	}

	auto plaintextJson = QJsonDocument(QJsonObject{{SecretExportKeys::Secrets, encodedSecrets}}).toJson(QJsonDocument::Compact);
	const auto plaintext = Veyon::Crypto::SecureBuffer::fromByteArray(plaintextJson);
	plaintextJson.fill(0);
	if (plaintext.size() > MaximumPlaintextSize)
	{
		vWarning() << "secrets too large";
		return {};
	}

	EncryptedDocument document;
	document.kdf = kdf;
	const auto salt = Veyon::Crypto::Random::bytes(Veyon::Crypto::PasswordKdf::SaltSize);
	const auto nonce = Veyon::Crypto::Random::bytes(Veyon::Crypto::Aead::NonceSize);
	if (salt.has_value() == false || nonce.has_value() == false)
	{
		return {};
	}
	document.salt = salt->toUnprotectedByteArray();
	document.nonce = nonce->toUnprotectedByteArray();

	const auto key = deriveKey(document, passphrase);
	if (key.has_value() == false)
	{
		vCritical() << "key derivation failed:" << Veyon::Crypto::lastError();
		return {};
	}

	const auto ciphertext = Veyon::Crypto::Aead::seal(Cipher, *key, document.nonce, plaintext, associatedData(document));
	if (ciphertext.has_value() == false)
	{
		vCritical() << "encryption failed:" << Veyon::Crypto::lastError();
		return {};
	}

	QJsonObject kdfParameters;
	if (kdf == Kdf::Argon2id)
	{
		kdfParameters = {{SecretExportKeys::Argon2idIterations, qint64(document.argon2idParameters.iterations)},
						 {SecretExportKeys::Argon2idMemory, qint64(document.argon2idParameters.memoryKiB)},
						 {SecretExportKeys::Parallelism, qint64(document.argon2idParameters.parallelism)}};
	}
	else
	{
		kdfParameters = {{SecretExportKeys::ScryptCost, qint64(document.scryptParameters.cost)},
						 {SecretExportKeys::ScryptBlockSize, qint64(document.scryptParameters.blockSize)},
						 {SecretExportKeys::Parallelism, qint64(document.scryptParameters.parallelism)}};
	}

	return QJsonObject{
		{SecretExportKeys::Version, Version},
		{SecretExportKeys::Kind, SecretExportKeys::KindValue},
		{SecretExportKeys::Kdf, kdf == Kdf::Argon2id ? SecretExportKeys::Argon2id : SecretExportKeys::Scrypt},
		{SecretExportKeys::KdfParameters, kdfParameters},
		{SecretExportKeys::Salt, QString::fromLatin1(document.salt.toBase64())},
		{SecretExportKeys::Nonce, QString::fromLatin1(document.nonce.toBase64())},
		{SecretExportKeys::Ciphertext, QString::fromLatin1(ciphertext->toBase64())},
	};
}



std::optional<SecretExport::Secrets> SecretExport::decrypt(const QJsonObject& document, Veyon::Crypto::ByteView passphrase)
{
	const auto encryptedDocument = parseDocument(document);
	if (encryptedDocument.has_value() == false)
	{
		vWarning() << "invalid secrets document";
		return {};
	}

	if (passphrase.isEmpty() || passphrase.size() > Veyon::Crypto::PasswordKdf::MaximumPasswordSize)
	{
		return {};
	}

	const auto key = deriveKey(*encryptedDocument, passphrase);
	if (key.has_value() == false)
	{
		vWarning() << "key derivation failed:" << Veyon::Crypto::lastError();
		return {};
	}

	const auto plaintext = Veyon::Crypto::Aead::open(Cipher, *key, encryptedDocument->nonce, encryptedDocument->ciphertext,
													 associatedData(*encryptedDocument));
	if (plaintext.has_value() == false)
	{
		vWarning() << "wrong passphrase or modified secrets document";
		return {};
	}

	auto secrets = parsePlaintext(*plaintext);
	if (secrets.has_value() == false)
	{
		vWarning() << "invalid content of secrets document";
	}

	return secrets;
}



std::optional<SecretExport::EncryptedDocument> SecretExport::parseDocument(const QJsonObject& document)
{
	if (integerValue(document.value(SecretExportKeys::Version), Version, Version).has_value() == false ||
		document.value(SecretExportKeys::Kind).toString() != SecretExportKeys::KindValue ||
		document.value(SecretExportKeys::KdfParameters).isObject() == false)
	{
		return {};
	}

	EncryptedDocument encryptedDocument;

	const auto kdf = document.value(SecretExportKeys::Kdf).toString();
	const auto kdfParameters = document.value(SecretExportKeys::KdfParameters).toObject();
	if (kdf == SecretExportKeys::Scrypt)
	{
		const auto cost = integerValue(kdfParameters.value(SecretExportKeys::ScryptCost), 2, MaximumScryptCost);
		const auto blockSize = integerValue(kdfParameters.value(SecretExportKeys::ScryptBlockSize), 1, MaximumScryptBlockSize);
		const auto parallelism = integerValue(kdfParameters.value(SecretExportKeys::Parallelism), 1, MaximumParallelism);
		if (kdfParameters.size() != 3 ||
			cost.has_value() == false || (*cost & (*cost - 1)) != 0 ||
			blockSize.has_value() == false || parallelism.has_value() == false ||
			quint64(128) * quint64(*cost) * quint64(*blockSize) > MaximumScryptMemory)
		{
			return {};
		}
		encryptedDocument.kdf = Kdf::Scrypt;
		encryptedDocument.scryptParameters.cost = quint64(*cost);
		encryptedDocument.scryptParameters.blockSize = quint32(*blockSize);
		encryptedDocument.scryptParameters.parallelism = quint32(*parallelism);
	}
	else if (kdf == SecretExportKeys::Argon2id)
	{
		const auto iterations = integerValue(kdfParameters.value(SecretExportKeys::Argon2idIterations), 1, MaximumArgon2idIterations);
		const auto memory = integerValue(kdfParameters.value(SecretExportKeys::Argon2idMemory), 8, MaximumArgon2idMemoryKiB);
		const auto parallelism = integerValue(kdfParameters.value(SecretExportKeys::Parallelism), 1, MaximumParallelism);
		if (kdfParameters.size() != 3 ||
			iterations.has_value() == false || memory.has_value() == false || parallelism.has_value() == false ||
			*memory < 8 * *parallelism)
		{
			return {};
		}
		encryptedDocument.kdf = Kdf::Argon2id;
		encryptedDocument.argon2idParameters.iterations = quint32(*iterations);
		encryptedDocument.argon2idParameters.memoryKiB = quint32(*memory);
		encryptedDocument.argon2idParameters.parallelism = quint32(*parallelism);
	}
	else
	{
		return {};
	}

	const auto salt = base64Value(document.value(SecretExportKeys::Salt), Veyon::Crypto::PasswordKdf::SaltSize,
								  Veyon::Crypto::PasswordKdf::SaltSize);
	const auto nonce = base64Value(document.value(SecretExportKeys::Nonce), Veyon::Crypto::Aead::NonceSize,
								   Veyon::Crypto::Aead::NonceSize);
	const auto ciphertext = base64Value(document.value(SecretExportKeys::Ciphertext), Veyon::Crypto::Aead::TagSize,
										MaximumPlaintextSize + Veyon::Crypto::Aead::TagSize);
	if (salt.has_value() == false || nonce.has_value() == false || ciphertext.has_value() == false)
	{
		return {};
	}

	encryptedDocument.salt = *salt;
	encryptedDocument.nonce = *nonce;
	encryptedDocument.ciphertext = *ciphertext;

	return encryptedDocument;
}



std::optional<SecretExport::Secrets> SecretExport::parsePlaintext(Veyon::Crypto::ByteView plaintext)
{
	if (plaintext.size() > MaximumPlaintextSize)
	{
		return {};
	}

	QJsonParseError parseError{};
	const auto jsonDocument = QJsonDocument::fromJson(QByteArray::fromRawData(plaintext.data(), plaintext.size()), &parseError);
	if (parseError.error != QJsonParseError::NoError || jsonDocument.isObject() == false)
	{
		return {};
	}

	const auto root = jsonDocument.object();
	const auto encodedSecrets = root.value(SecretExportKeys::Secrets);
	if (root.size() != 1 || encodedSecrets.isObject() == false ||
		encodedSecrets.toObject().size() > MaximumSecretCount)
	{
		return {};
	}

	Secrets secrets;
	const auto encodedSecretsObject = encodedSecrets.toObject();
	for (auto it = encodedSecretsObject.constBegin(); it != encodedSecretsObject.constEnd(); ++it)
	{
		auto value = base64Value(it.value(), 0, MaximumPlaintextSize);
		if (SecretStore::isValidKey(it.key()) == false || value.has_value() == false)
		{
			return {};
		}
		secrets.emplace(it.key(), Veyon::Crypto::SecureBuffer::fromByteArray(*value));
		value->fill(0);
	}

	return secrets;
}



QByteArray SecretExport::associatedData(const EncryptedDocument& document)
{
	if (document.kdf == Kdf::Argon2id)
	{
		return QStringLiteral("veyon-secrets v%1 argon2id t=%2 m=%3 p=%4")
			.arg(Version)
			.arg(document.argon2idParameters.iterations)
			.arg(document.argon2idParameters.memoryKiB)
			.arg(document.argon2idParameters.parallelism)
			.toLatin1();
	}

	return QStringLiteral("veyon-secrets v%1 scrypt N=%2 r=%3 p=%4")
		.arg(Version)
		.arg(document.scryptParameters.cost)
		.arg(document.scryptParameters.blockSize)
		.arg(document.scryptParameters.parallelism)
		.toLatin1();
}



std::optional<Veyon::Crypto::SecureBuffer> SecretExport::deriveKey(const EncryptedDocument& document,
																   Veyon::Crypto::ByteView passphrase)
{
	if (document.kdf == Kdf::Argon2id)
	{
		return Veyon::Crypto::PasswordKdf::argon2id(passphrase, document.salt, document.argon2idParameters,
													Veyon::Crypto::Aead::KeySize);
	}

	return Veyon::Crypto::PasswordKdf::scrypt(passphrase, document.salt, document.scryptParameters,
											  Veyon::Crypto::Aead::KeySize);
}
