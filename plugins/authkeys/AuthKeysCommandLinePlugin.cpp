/*
 * AuthKeysCommandLinePlugin.cpp - implementation of AuthKeysCommandLinePlugin class
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QDir>
#include <QFileInfo>

#include "AuthKeysCommandLinePlugin.h"
#include "CryptoCore.h"
#include "Filesystem.h"
#include "VeyonConfiguration.h"


AuthKeysCommandLinePlugin::AuthKeysCommandLinePlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ "create", tr( "Create new authentication key pair" ) },
{ "delete", tr( "Delete authentication key pair" ) },
{ "list", tr( "List authentication key pairs" ) },
{ "import", tr( "Import public or private key" ) },
{ "export", tr( "Export public or private key" ) },
				} )
{
}



AuthKeysCommandLinePlugin::~AuthKeysCommandLinePlugin()
{
}



QStringList AuthKeysCommandLinePlugin::commands() const
{
	return m_commands.keys();
}



QString AuthKeysCommandLinePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_create( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	const auto name = arguments.first();
	if( QRegExp( "\\w+").exactMatch( name ) == false )
	{
		printf( "%s\n", qPrintable( tr( "Name contains invalid characters!" ) ) );

		return InvalidArguments;
	}

	QString privateKeyFileName = VeyonCore::filesystem().privateKeyPath( name );
	QString publicKeyFileName = VeyonCore::filesystem().publicKeyPath( name );

	VeyonCore::filesystem().ensurePathExists( QFileInfo( privateKeyFileName ).path() );
	VeyonCore::filesystem().ensurePathExists( QFileInfo( publicKeyFileName ).path() );

	printf( "%s\n", qPrintable( tr( "Creating new key pair in %1 and %2" ).
								arg( privateKeyFileName, publicKeyFileName ) ) );

	CryptoCore::PrivateKey privateKey = CryptoCore::KeyGenerator().createRSA( CryptoCore::RsaKeySize );
	CryptoCore::PublicKey publicKey = privateKey.toPublicKey();

	if( privateKey.isNull() || publicKey.isNull() )
	{
		printf( "%s\n", qPrintable( tr( "Failed to create public or private key!" ) ) );

		return Failed;
	}

	QFile privateKeyFile( privateKeyFileName );
	QFile publicKeyFile( publicKeyFileName );

	if( privateKeyFile.exists() || publicKeyFile.exists() )
	{
		if( arguments.contains( QStringLiteral( "-f" ) ) == false )
		{
			printf( "%s\n", qPrintable( tr( "Private and/or public key file already exist! "
											"Please pass \"-f\" to force overwriting them." ) ) );
			return Failed;
		}

		privateKeyFile.setPermissions( QFile::WriteOwner | QFile::WriteGroup | QFile::WriteOther );
		privateKeyFile.remove();

		publicKeyFile.setPermissions( QFile::WriteOwner | QFile::WriteGroup | QFile::WriteOther );
		publicKeyFile.remove();
	}

	if( privateKey.toPEMFile( privateKeyFileName ) == false )
	{
		printf( "%s\n", qPrintable( tr( "Failed to save private key!" ) ) );

		return Failed;
	}

	if( publicKey.toPEMFile( publicKeyFileName ) == false )
	{
		printf( "%s\n", qPrintable( tr( "Failed to save public key!" ) ) );

		return Failed;
	}

	privateKeyFile.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup );
	publicKeyFile.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );

	printf( "%s\n\n\n", qPrintable( tr( "Newly created key pair has been saved to \n\n%1\n\nand\n\n%2" ).
									arg( privateKeyFileName, publicKeyFileName ) ) );
	printf( "%s\n", qPrintable( tr( "For now the private key file is only readable by root and members of group root "
									"(except you ran this command as non-root)." ) ) );
	printf( "%s\n\n", qPrintable( tr( "It's recommended to change the ownership of the private key so that the file is\n"
									  "readable by all members of a special group to which all users belong who are\n"
									  "allowed to use Veyon." ) ) );

	return Successful;
}




CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_list( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	QMap<QString, QStringList> keyNames;

	const auto privateKeyBaseDir = VeyonCore::filesystem().expandPath( VeyonCore::config().privateKeyBaseDir() );
	const auto privateKeys = QDir( privateKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

	for( const auto& privateKey : privateKeys )
	{
		keyNames[privateKey] += tr( "private" );
	}

	const auto publicKeyBaseDir = VeyonCore::filesystem().expandPath( VeyonCore::config().publicKeyBaseDir() );
	const auto publicKeys = QDir( publicKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

	for( const auto& publicKey : publicKeys )
	{
		keyNames[publicKey] += tr( "public" );
	}

	for( auto it = keyNames.begin(), end = keyNames.end(); it != end; ++it )
	{
		printf( "%s: %s\n", qPrintable( it.key() ), qPrintable( it.value().join('+') ) );
	}

	return NoResult;
}
