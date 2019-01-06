/*
 * AuthKeysManager.h - declaration of AuthKeysManager class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "CryptoCore.h"

class AuthKeysManager : public QObject
{
	Q_OBJECT
public:
	AuthKeysManager( QObject* parent = nullptr );
	~AuthKeysManager() = default;

	const QString& resultMessage() const
	{
		return m_resultMessage;
	}

	bool createKeyPair( const QString& name );
	bool deleteKey( const QString& name, const QString& type );
	bool exportKey( const QString& name, const QString& type, const QString& outputFile );
	bool importKey( const QString& name, const QString& type, const QString& inputFile );
	QStringList listKeys();
	bool extractPublicFromPrivateKey( const QString& name );

	bool writePrivateKeyFile( const CryptoCore::PrivateKey& privateKey, const QString& privateKeyFileName );
	bool writePublicKeyFile( const CryptoCore::PublicKey& publicKey, const QString& publicKeyFileName );

	QString detectKeyType( const QString& keyFile );

	bool setAccessGroup( const QString& key, const QString& group );
	QString accessGroup( const QString& key );

	QString keyPairId( const QString& key );

	static QString exportedKeyFileName( const QString& name, const QString& type );
	static QString keyNameFromExportedKeyFile( const QString& keyFile );

private:
	bool checkKey( const QString& name, const QString& type, bool checkIsReadable = true );

	QString keyFilePathFromType( const QString& name, const QString& type ) const;
	bool setKeyFilePermissions( const QString& name, const QString& type ) const;
	bool setPrivateKeyFilePermissions( const QString& fileName ) const;
	bool setPublicKeyFilePermissions( const QString& fileName ) const;

	const QString m_keyTypePrivate;
	const QString m_keyTypePublic;
	const QString m_checkPermissions;
	const QString m_invalidKeyName;
	const QString m_invalidKeyType;
	const QString m_keyDoesNotExist;
	const QString m_keysAlreadyExists;
	QString m_resultMessage;

};
