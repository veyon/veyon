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

	QString encryptPassword(const PlaintextPassword& password) const;
	PlaintextPassword decryptPassword(const QString& encryptedPassword) const;

private:
	QCA::Initializer m_qcaInitializer;
	PrivateKey m_defaultPrivateKey;
};
