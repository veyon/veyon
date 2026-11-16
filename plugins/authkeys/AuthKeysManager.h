// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

class AuthKeysManager : public QObject
{
	Q_OBJECT
public:
	explicit AuthKeysManager(QObject* parent = nullptr);
	~AuthKeysManager() override = default;

	const QString& resultMessage() const
	{
		return m_resultMessage;
	}

	bool createKeyPair(const QString& name);
	bool deleteKey(const QString& name, const QString& type);
	bool exportKey(const QString& name, const QString& type, const QString& outputFile, bool overwriteExisting);
	bool importKey(const QString& name, const QString& type, const QString& inputFile);
	QStringList listKeys();
	bool extractPublicFromPrivateKey(const QString& name);

	bool writePrivateKeyFile(const CryptoCore::PrivateKey& privateKey, const QString& privateKeyFileName);
	bool writePublicKeyFile(const CryptoCore::PublicKey& publicKey, const QString& publicKeyFileName);

	QString detectKeyType(const QString& keyFile);

	bool setAccessGroup(const QString& key, const QString& group);
	QString accessGroup(const QString& key);

	QString keyPairId(const QString& key);

	static QString exportedKeyFileName(const QString& name, const QString& type);
	static QString keyNameFromExportedKeyFile(const QString& keyFile);

private:
	bool checkKey(const QString& name, const QString& type, bool checkIsReadable = true);

	QString keyFilePathFromType(const QString& name, const QString& type) const;
	bool setKeyFilePermissions(const QString& name, const QString& type) const;
	bool setPrivateKeyFilePermissions(const QString& fileName) const;
	bool setPublicKeyFilePermissions(const QString& fileName) const;

	const QString m_keyTypePrivate;
	const QString m_keyTypePublic;
	const QString m_checkPermissions;
	const QString m_invalidKeyName;
	const QString m_invalidKeyType;
	const QString m_keyDoesNotExist;
	const QString m_keysAlreadyExists;
	QString m_resultMessage;
};
