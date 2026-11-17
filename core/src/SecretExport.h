// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>

#include "PasswordKdf.h"
#include "VeyonCore.h"

#include <map>
#include <optional>

// Passphrase-protected secrets in configuration exports (veyon-cli config export/import
// --include-secrets, 03-secretstore-config.md §4.6):
//   {"v":1,"kind":"veyon-secrets","kdf":"scrypt|argon2id","kdfParams":{...},"salt":b64,"nonce":b64,"ct":b64}
// The key is derived with scrypt (N=2^15, r=8, p=1, default, available with every supported
// OpenSSL version) or Argon2id (t=3, m=64 MiB, p=1, OpenSSL >= 3.2); ct is AES-256-GCM over
// {"secrets":{"<key>":"<base64 value>",...}} with the KDF name and parameters as associated data.
// Documents are untrusted input: the reader bounds the KDF parameters and all sizes.
// The associated data does not cover the surrounding configuration of an export: anyone can
// change the plain configuration part of an export file, which is unauthenticated anyway; only
// the secrets (and their keys) are authenticated by the passphrase (accepted, WP4 review L5).
class VEYON_CORE_EXPORT SecretExport
{
public:
	using Secrets = std::map<QString, Veyon::Crypto::SecureBuffer>;
	using Kdf = Veyon::Crypto::PasswordKdf::Algorithm;

	static constexpr int Version = 1;
	static constexpr int MinimumPassphraseLength = 12;
	static constexpr int MaximumSecretCount = 256;
	static constexpr qsizetype MaximumPlaintextSize = 1024 * 1024;

	// reader limits (well above the defaults, far below what Veyon::Crypto::PasswordKdf accepts)
	static constexpr quint64 MaximumScryptCost = 1 << 18;
	static constexpr quint32 MaximumScryptBlockSize = 16;
	static constexpr quint64 MaximumScryptMemory = 256 * 1024 * 1024;
	static constexpr quint32 MaximumArgon2idIterations = 8;
	static constexpr quint32 MaximumArgon2idMemoryKiB = 256 * 1024;
	static constexpr quint32 MaximumParallelism = 4;

	// member of a configuration export holding the document
	static QString configurationKey()
	{
		return QStringLiteral("VeyonSecrets");
	}

	struct EncryptedDocument
	{
		Kdf kdf = Kdf::Scrypt;
		Veyon::Crypto::PasswordKdf::ScryptParameters scryptParameters{};
		Veyon::Crypto::PasswordKdf::Argon2idParameters argon2idParameters{};
		QByteArray salt{};
		QByteArray nonce{};
		QByteArray ciphertext{};
	};

	// at least MinimumPassphraseLength characters of valid UTF-8
	[[nodiscard]] static bool isAcceptablePassphrase(Veyon::Crypto::ByteView passphrase);

	[[nodiscard]] static std::optional<QJsonObject> encrypt(const Secrets& secrets, Veyon::Crypto::ByteView passphrase,
															Kdf kdf = Kdf::Scrypt);
	// nullopt for malformed documents, a wrong passphrase and modified documents alike
	[[nodiscard]] static std::optional<Secrets> decrypt(const QJsonObject& document, Veyon::Crypto::ByteView passphrase);

	// the parsers of decrypt(), separately for tests and fuzzing
	[[nodiscard]] static std::optional<EncryptedDocument> parseDocument(const QJsonObject& document);
	[[nodiscard]] static std::optional<Secrets> parsePlaintext(Veyon::Crypto::ByteView plaintext);

private:
	static QByteArray associatedData(const EncryptedDocument& document);
	static std::optional<Veyon::Crypto::SecureBuffer> deriveKey(const EncryptedDocument& document,
																Veyon::Crypto::ByteView passphrase);
};
