// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRandomGenerator>

#include "CryptoCore.h"

CryptoCore::CryptoCore() :
	m_qcaInitializer(),
	m_defaultPrivateKey()
{
	const auto features = QCA::supportedFeatures();

	vDebug() << "CryptoCore instance created - features supported by QCA" << qcaVersionStr() << features;

	if (features.contains(QStringLiteral("rsa")) == false)
	{
		qFatal("CryptoCore: RSA not supported! Please install a QCA plugin which provides RSA support "
			   "(e.g. packages such as libqca-qt5-2-plugins or qca-qt5-ossl).");
	}

	m_defaultPrivateKey = PrivateKey::fromPEMFile(QStringLiteral(":/core/default-pkey.pem"));
}



CryptoCore::~CryptoCore()
{
	vDebug();
}



QByteArray CryptoCore::generateChallenge()
{
	static_assert(ChallengeSize % sizeof(quint32) == 0);

	using ChallengeWords = std::array<quint32, ChallengeSize / sizeof(quint32)>;

	ChallengeWords challenge{};
	QRandomGenerator::system()->fillRange(challenge.data(), challenge.size());

	return QByteArray(reinterpret_cast<const char*>(challenge.data()), ChallengeSize);
}



QString CryptoCore::encryptPassword(const PlaintextPassword& password) const
{
	return QString::fromLatin1(m_defaultPrivateKey.toPublicKey().encrypt(password, DefaultEncryptionAlgorithm).toByteArray().toHex());
}



CryptoCore::PlaintextPassword CryptoCore::decryptPassword(const QString& encryptedPassword) const
{
	PlaintextPassword decryptedPassword;

	if (PrivateKey(m_defaultPrivateKey).decrypt(QByteArray::fromHex(encryptedPassword.toUtf8()), &decryptedPassword, DefaultEncryptionAlgorithm))
	{
		return decryptedPassword;
	}

	vCritical() << "failed to decrypt password!";

	return {};
}
