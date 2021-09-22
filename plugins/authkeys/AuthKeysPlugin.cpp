/*
 * AuthKeysPlugin.cpp - implementation of AuthKeysPlugin class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcessEnvironment>

#include "AuthKeysConfigurationWidget.h"
#include "AuthKeysPlugin.h"
#include "AuthKeysManager.h"
#include "Filesystem.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"


AuthKeysPlugin::AuthKeysPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() ),
	m_manager( m_configuration ),
	m_commands( {
{ QStringLiteral("create"), tr( "Create new authentication key pair" ) },
{ QStringLiteral("delete"), tr( "Delete authentication key" ) },
{ QStringLiteral("list"), tr( "List authentication keys" ) },
{ QStringLiteral("import"), tr( "Import public or private key" ) },
{ QStringLiteral("export"), tr( "Export public or private key" ) },
{ QStringLiteral("extract"), tr( "Extract public key from existing private key" ) },
{ QStringLiteral("setaccessgroup"), tr( "Set user group allowed to access a key" ) },
				} )
{
}



void AuthKeysPlugin::upgrade(const QVersionNumber& oldVersion)
{
	if( oldVersion < QVersionNumber( 2, 0 ) )
	{
		m_configuration.setPublicKeyBaseDir( m_configuration.legacyPublicKeyBaseDir() );
		m_configuration.setPrivateKeyBaseDir( m_configuration.legacyPrivateKeyBaseDir() );
	}
}



QWidget* AuthKeysPlugin::createAuthenticationConfigurationWidget()
{
	return new AuthKeysConfigurationWidget( m_configuration, m_manager );
}



bool AuthKeysPlugin::initializeCredentials()
{
	m_privateKey = {};

	auto authKeyName = QProcessEnvironment::systemEnvironment().value( QStringLiteral("VEYON_AUTH_KEY_NAME") );

	if( authKeyName.isEmpty() == false )
	{
		if( AuthKeysManager::isKeyNameValid( authKeyName ) &&
			loadPrivateKey( m_manager.privateKeyPath( authKeyName ) ) )
		{
			m_authKeyName = authKeyName;
			return true;
		}
	}
	else
	{
		// try to auto-detect private key file by searching for readable file
		const auto privateKeyBaseDir = VeyonCore::filesystem().expandPath( m_configuration.privateKeyBaseDir() );
		const auto privateKeyDirs = QDir( privateKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

		for( const auto& privateKeyDir : privateKeyDirs )
		{
			if( loadPrivateKey( m_manager.privateKeyPath( privateKeyDir ) ) )
			{
				m_authKeyName = privateKeyDir;
				return true;
			}
		}
	}

	return false;
}



bool AuthKeysPlugin::hasCredentials() const
{
	return m_privateKey.isNull() == false;
}



bool AuthKeysPlugin::checkCredentials() const
{
	if( hasCredentials() == false )
	{
		vWarning() << "Authentication key files not set up properly!";

		QMessageBox::critical( QApplication::activeWindow(),
								  authenticationTestTitle(),
								  tr( "Authentication key files are not set up properly on this computer. "
									  "Please create new key files or switch to a different authentication method "
									  "using the Veyon Configurator." ) );

		return false;
	}

	return true;
}



VncServerClient::AuthState AuthKeysPlugin::performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		client->setChallenge( CryptoCore::generateChallenge() );
		if( VariantArrayMessage( message.ioDevice() ).write( client->challenge() ).send() == false )
		{
			vWarning() << "failed to send challenge";
			return VncServerClient::AuthState::Failed;
		}
		return VncServerClient::AuthState::Stage1;

	case VncServerClient::AuthState::Stage1:
	{
		// get authentication key name
		const auto authKeyName = message.read().toString(); // Flawfinder: ignore

		if( AuthKeysManager::isKeyNameValid( authKeyName ) == false )
		{
			vDebug() << "invalid auth key name!";
			return VncServerClient::AuthState::Failed;
		}

		// now try to verify received signed data using public key of the user
		// under which the client claims to run
		const auto signature = message.read().toByteArray(); // Flawfinder: ignore

		const auto publicKeyPath = m_manager.publicKeyPath( authKeyName );

		CryptoCore::PublicKey publicKey( publicKeyPath );
		if( publicKey.isNull() || publicKey.isPublic() == false )
		{
			vWarning() << "failed to load public key from" << publicKeyPath;
			return VncServerClient::AuthState::Failed;
		}

		vDebug() << "loaded public key from" << publicKeyPath;
		if( publicKey.verifyMessage( client->challenge(), signature, CryptoCore::DefaultSignatureAlgorithm ) == false )
		{
			vWarning() << "FAIL";
			return VncServerClient::AuthState::Failed;
		}

		vDebug() << "SUCCESS";
		return VncServerClient::AuthState::Successful;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}



bool AuthKeysPlugin::authenticate( QIODevice* socket ) const
{
	VariantArrayMessage challengeReceiveMessage( socket );
	challengeReceiveMessage.receive();
	const auto challenge = challengeReceiveMessage.read().toByteArray();

	if( challenge.size() != CryptoCore::ChallengeSize )
	{
		vCritical() << QThread::currentThreadId() << "challenge size mismatch!";
		return false;
	}

	// create local copy of private key so we can modify it within our own thread
	auto key = m_privateKey;

	if( key.isNull() || key.canSign() == false )
	{
		vCritical() << QThread::currentThreadId() << "invalid private key!";
		return false;
	}

	const auto signature = key.signMessage( challenge, CryptoCore::DefaultSignatureAlgorithm );

	VariantArrayMessage challengeResponseMessage( socket );
	challengeResponseMessage.write( m_authKeyName );
	challengeResponseMessage.write( signature );
	challengeResponseMessage.send();

	return true;
}



QStringList AuthKeysPlugin::commands() const
{
	return m_commands.keys();
}



QString AuthKeysPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );
	if( command.isEmpty() )
	{
		error( tr("Please specify the command to display help for.") );
		return NoResult;
	}

	const QMap<QString, QStringList> commands = {
		{ QStringLiteral("create"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("NAME") ),
						 tr( "This command creates a new authentication key pair with name <NAME> and saves private and "
						 "public key to the configured key directories. The parameter must be a name for the key, which "
						 "may only contain letters." ) } ) },
		{ QStringLiteral("delete"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("KEY") ),
						 tr( "This command deletes the authentication key <KEY> from the configured key directory. "
						 "Please note that a key can't be recovered once it has been deleted." ) } ) },
		{ QStringLiteral("export"),
		  QStringList( { QStringLiteral("<%1> [<%2>]").arg( tr("KEY"), tr("FILE") ),
						 tr( "This command exports the authentication key <KEY> to <FILE>. "
						 "If <FILE> is not specified a name will be constructed from name and type of <KEY>." ) } ) },
		{ QStringLiteral("extract"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("KEY") ),
						 tr( "This command extracts the public key part from the private key <KEY> and saves it as the "
						 "corresponding public key. When setting up another master computer, it is therefore sufficient "
						 "to transfer the private key only. The public key can then be extracted." ) } ) },
		{ QStringLiteral("import"),
		  QStringList( { QStringLiteral("<%1> [<%2>]").arg( tr( "KEY" ), tr( "FILE" ) ),
						 tr( "This command imports the authentication key <KEY> from <FILE>. "
						 "If <FILE> is not specified a name will be constructed from name and type of <KEY>." ) } ) },
		{ QStringLiteral("list"),
		  QStringList( { QStringLiteral("[details]"),
						 tr( "This command lists all available authentication keys in the configured key directory. "
						 "If the option \"%1\" is specified a table with key details will be displayed instead. "
						 "Some details might be missing if a key is not accessible e.g. due to the lack of read permissions." ).arg( QLatin1String("details") ) } ) },
		{ QStringLiteral("setaccessgroup"),
		  QStringList( { QStringLiteral("<%1> <%2>").arg( tr("KEY"), tr("ACCESS GROUP") ),
						 tr( "This command adjusts file access permissions to <KEY> such that only the "
						 "user group <ACCESS GROUP> has read access to it." ) } ) },
	};

	if( commands.contains( command ) )
	{
		const auto& helpString = commands[command];
		print( QStringLiteral("\n%1 %2\n\n%3\n\n").arg( command, helpString[0], helpString[1] ) );

		return NoResult;
	}

	error( tr("The specified command does not exist or no help is available for it.") );

	return NoResult;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_setaccessgroup( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto key = arguments[0];
	const auto accessGroup = arguments[1];

	if( m_manager.setAccessGroup( key, accessGroup ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_create( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	if( m_manager.createKeyPair( arguments.first() ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_delete( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments.first().split( QLatin1Char('/') );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	if( m_manager.deleteKey( name, type ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_export( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( QLatin1Char('/') );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	auto outputFile = arguments.value( 1 );

	if( outputFile.isEmpty() )
	{
		outputFile = AuthKeysManager::exportedKeyFileName( name, type );
	}

	if( m_manager.exportKey( name, type, outputFile, arguments.contains( QLatin1String("-f") ) ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_import( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	if( QFileInfo::exists( arguments[0] ) )
	{
		error( tr( "Please specify the key name (e.g. \"teacher/public\") as the first argument." ) );
		return InvalidArguments;
	}

	const auto nameAndType = arguments[0].split( QLatin1Char('/') );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	auto inputFile = arguments.value( 1 );

	if( inputFile.isEmpty() )
	{
		inputFile = AuthKeysManager::exportedKeyFileName( name, type );
	}

	if( m_manager.importKey( name, type, inputFile ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_list( const QStringList& arguments )
{
	if( arguments.value( 0 ) == QLatin1String("details") )
	{
		printAuthKeyTable();
	}
	else
	{
		printAuthKeyList();
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_extract( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	if( m_manager.extractPublicFromPrivateKey( arguments.first() ) == false )
	{
		error( m_manager.resultMessage() );

		return Failed;
	}

	info( m_manager.resultMessage() );

	return Successful;
}



bool AuthKeysPlugin::loadPrivateKey( const QString& privateKeyFile )
{
	vDebug() << privateKeyFile;

	if( privateKeyFile.isEmpty() )
	{
		return false;
	}

	m_privateKey = CryptoCore::PrivateKey( privateKeyFile );

	return m_privateKey.isNull() == false && m_privateKey.isPrivate();
}



void AuthKeysPlugin::printAuthKeyTable()
{
	AuthKeysTableModel tableModel( m_manager );
	tableModel.reload();

	TableHeader tableHeader( { tr("NAME"), tr("TYPE"), tr("PAIR ID"), tr("ACCESS GROUP") } );
	TableRows tableRows;

	tableRows.reserve( tableModel.rowCount() );

	for( int i = 0; i < tableModel.rowCount(); ++i )
	{
		tableRows.append( { authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyName ),
							authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyType ),
							authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyPairID ),
							authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnAccessGroup ) } );
	}

	printTable( Table( tableHeader, tableRows ) );
}



QString AuthKeysPlugin::authKeysTableData( const AuthKeysTableModel& tableModel, int row, int column )
{
	return tableModel.data( tableModel.index( row, column ), Qt::DisplayRole ).toString();
}



void AuthKeysPlugin::printAuthKeyList()
{
	const auto keys = m_manager.listKeys();

	for( const auto& key : keys )
	{
		print( key );
	}
}


IMPLEMENT_CONFIG_PROXY(AuthKeysConfiguration)
