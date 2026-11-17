// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtCrypto>

#include "VeyonCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT CryptoCore
{
public:
	using KeyGenerator = QCA::KeyGenerator;
	using PrivateKey = QCA::PrivateKey;
	using PublicKey = QCA::PublicKey;
	using SecureArray = QCA::SecureArray;
	using PlaintextPassword = SecureArray;

	enum {
		RsaKeySize = 4096,
		ChallengeSize = 128,
	};

	static constexpr QCA::EncryptionAlgorithm DefaultEncryptionAlgorithm = QCA::EME_PKCS1_OAEP;
	static constexpr QCA::SignatureAlgorithm DefaultSignatureAlgorithm = QCA::EMSA3_SHA512;

	CryptoCore();
	~CryptoCore();

	static QByteArray generateChallenge();

	// legacy obfuscation with the public default-pkey.pem: only for the UserLogin feature message
	// (replaced in 4.12, 05-tls.md §7) and LegacySecretMigration, never for new stored data
	QString encryptPassword(const PlaintextPassword& password) const;
	// ok (if given) tells a failure apart from an empty plaintext
	PlaintextPassword decryptPassword(const QString& encryptedPassword, bool* ok = nullptr) const;

private:
	QCA::Initializer m_qcaInitializer;
	PrivateKey m_defaultPrivateKey;
};
