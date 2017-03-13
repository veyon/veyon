/*
 * ConfiguratorCore.cpp - global instances for the iTALC Configurator
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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
#include <QMessageBox>

#include "Configuration/LocalStore.h"
#include "ConfiguratorCore.h"
#include "CryptoCore.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "MainWindow.h"
#include "SystemConfigurationModifier.h"


namespace ConfiguratorCore
{

// static data initialization
MainWindow *mainWindow = NULL;
bool silent = false;


static void configApplyError( const QString &msg )
{
	criticalMessage( MainWindow::tr( "%1 Configurator" ).arg( ItalcCore::applicationName() ), msg );
}


bool applyConfiguration( const ItalcConfiguration &c )
{
	// merge configuration
	ItalcCore::config() += c;

	// do necessary modifications of system configuration
	if( !SystemConfigurationModifier::setServiceAutostart(
									ItalcCore::config().autostartService() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not modify the autostart property "
										"for the %1 Service." ).arg( ItalcCore::applicationName() ) );
	}

	if( !SystemConfigurationModifier::setServiceArguments(
									ItalcCore::config().serviceArguments() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not modify the service arguments "
									"for the %1 Service." ).arg( ItalcCore::applicationName() ) );
	}
	if( !SystemConfigurationModifier::enableFirewallException(
							ItalcCore::config().isFirewallExceptionEnabled() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not change the firewall configuration "
									"for the %1 Service." ).arg( ItalcCore::applicationName() ) );
	}

	// write global configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	localStore.flush( &ItalcCore::config() );

	return true;
}




static void listConfiguration( const ItalcConfiguration::DataMap &map,
									const QString &parentKey )
{
	for( ItalcConfiguration::DataMap::ConstIterator it = map.begin();
												it != map.end(); ++it )
	{
		QString curParentKey = parentKey.isEmpty() ?
									it.key() : parentKey + "/" + it.key();
		if( it.value().type() == QVariant::Map )
		{
			listConfiguration( it.value().toMap(), curParentKey );
		}
		else if( it.value().type() == QVariant::String )
		{
			QTextStream( stdout ) << curParentKey << "="
									<< it.value().toString() << endl;
		}
		else
		{
			qWarning( "unknown value in configuration data map" );
		}
	}
}



void listConfiguration( const ItalcConfiguration &config )
{
	listConfiguration( config.data(), QString() );
}




bool createKeyPair( ItalcCore::UserRole role, const QString &destDir )
{
	QString privateKeyFile = LocalSystem::Path::privateKeyPath( role, destDir );
	QString publicKeyFile = LocalSystem::Path::publicKeyPath( role, destDir );

	LocalSystem::Path::ensurePathExists( QFileInfo( privateKeyFile ).path() );
	LocalSystem::Path::ensurePathExists( QFileInfo( publicKeyFile ).path() );

	LogStream() << "ConfiguratorCore: creating new key pair in" << privateKeyFile << "and" << publicKeyFile;

	CryptoCore::PrivateKey privateKey = CryptoCore::KeyGenerator().createRSA( CryptoCore::RsaKeySize );
	CryptoCore::PublicKey publicKey = privateKey.toPublicKey();

	if( privateKey.isNull() || publicKey.isNull() )
	{
		ilog_failed( "key generation" );
		return false;
	}

	if( privateKey.toPEMFile( privateKeyFile ) == false )
	{
		ilog_failed( "saving private key" );
		return false;
	}

	if( publicKey.toPEMFile( publicKeyFile ) == false )
	{
		ilog_failed( "saving public key" );
		return false;
	}

	QFile( privateKeyFile ).setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup );
	QFile( publicKeyFile ).setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );

	printf( "...done, saved key-pair in\n\n%s\n\nand\n\n%s",
						privateKeyFile.toUtf8().constData(),
						publicKeyFile.toUtf8().constData() );
	printf( "\n\n\nFor now the file is only readable by "
				"root and members of group root (if you\n"
				"didn't ran this command as non-root).\n"
				"I suggest changing the ownership of the "
				"private key so that the file is\nreadable "
				"by all members of a special group to which "
				"all users belong who are\nallowed to use "
				"iTALC.\n\n\n" );
	return true;
}




bool importPublicKey( ItalcCore::UserRole role,
							const QString& publicKeyFile, const QString &destDir )
{
	// look whether the public key file is valid
	CryptoCore::PublicKey publicKey( publicKeyFile );
	if( publicKey.isNull() )
	{
		qCritical() << "ConfiguratorCore::importPublicKey(): file" << publicKeyFile
					<< "is not a valid public key file";
		return false;
	}

	QString destinationPublicKeyPath = LocalSystem::Path::publicKeyPath( role, destDir );
	QFile destFile( destinationPublicKeyPath );
	if( destFile.exists() )
	{
		destFile.setPermissions( QFile::WriteOwner );
		if( !destFile.remove() )
		{
			qCritical() << "ConfiguratorCore::importPublicKey(): could not remove "
							"existing public key file" << destFile.fileName();
			return false;
		}
	}

	// now try to copy it
	return publicKey.toPEMFile( destinationPublicKeyPath );
}



void informationMessage( const QString &title, const QString &msg )
{
	LogStream( Logger::LogLevelInfo ) << title.toUtf8().constData()
								<< ":" << msg.toUtf8().constData();
	if( qobject_cast<QApplication *>( QCoreApplication::instance() ) && !silent )
	{
		QMessageBox::information( NULL, title, msg );
	}
}



void criticalMessage( const QString &title, const QString &msg )
{
	LogStream( Logger::LogLevelCritical ) << title.toUtf8().constData()
								<< ":" << msg.toUtf8().constData();
	if( qobject_cast<QApplication *>( QCoreApplication::instance() ) && !silent )
	{
		QMessageBox::critical( NULL, title, msg );
	}
}



int clearConfiguration()
{
	// clear global configuration
	Configuration::LocalStore( Configuration::LocalStore::System ).clear();

	informationMessage( MainWindow::tr( "Configuration cleared" ),
						MainWindow::tr( "The local configuration has been cleared successfully." ) );

	return 0;
}


}
