/*
 * ImcCore.cpp - global instances for the iTALC Management Console
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

#include <italcconfig.h>

#include "Configuration/LocalStore.h"
#include "DsaKey.h"
#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "LogonAclSettings.h"
#include "MainWindow.h"
#include "SystemConfigurationModifier.h"


namespace ImcCore
{

// static data initialization
MainWindow *mainWindow = NULL;


static void configApplyError( const QString &msg )
{
	criticalMessage( MainWindow::tr( "iTALC Management Console" ), msg );
}


bool applyConfiguration( const ItalcConfiguration &c )
{
	// merge configuration
	*ItalcCore::config += c;

	// do neccessary modifications of system configuration
	if( !SystemConfigurationModifier::setServiceAutostart(
									ItalcCore::config->autostartService() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not modify the autostart property "
										"for the iTALC Service." ) );
	}

	if( !SystemConfigurationModifier::setServiceArguments(
									ItalcCore::config->serviceArguments() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not modify the service arguments "
									"for the iTALC Service." ) );
	}
	if( !SystemConfigurationModifier::enableFirewallException(
							ItalcCore::config->isFirewallExceptionEnabled() ) )
	{
		configApplyError(
			MainWindow::tr( "Could not change the firewall configuration "
									"for the iTALC Service." ) );
	}

#ifdef ITALC_BUILD_WIN32
	ItalcCore::config->removeValue( "LogonACL", "Authentication" );

	// if EncodedLogonACL is empty, nothing is done in setACL()
	LogonAclSettings().setACL(
		ItalcCore::config->value( "EncodedLogonACL", "Authentication" ) );
#endif

	// write global configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	localStore.flush( ItalcCore::config );

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
	QString priv = LocalSystem::Path::privateKeyPath( role, destDir );
	QString pub = LocalSystem::Path::publicKeyPath( role, destDir );
	LogStream() << "ImcCore: creating new key pair in" << priv << "and" << pub;

	PrivateDSAKey pkey( 1024 );
	if( !pkey.isValid() )
	{
		ilog_failed( "key generation" );
		return false;
	}
	if( !pkey.save( priv ) )
	{
		ilog_failed( "saving private key" );
		return false;
	}

	if( !PublicDSAKey( pkey ).save( pub ) )
	{
		ilog_failed( "saving public key" );
		return false;
	}

	printf( "...done, saved key-pair in\n\n%s\n\nand\n\n%s",
						priv.toUtf8().constData(),
						pub.toUtf8().constData() );
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
							const QString &pubKey, const QString &destDir )
{
	// look whether the public key file is valid
	PublicDSAKey dsaKey( pubKey );
	if( !dsaKey.isValid() )
	{
		qCritical() << "ImcCore::importPublicKey(): file" << pubKey
					<< "is not a valid public key file";
		return false;
	}

	QString pub = LocalSystem::Path::publicKeyPath( role, destDir );
	QFile destFile( pub );
	if( destFile.exists() )
	{
		destFile.setPermissions( QFile::WriteOwner );
		if( !destFile.remove() )
		{
			qCritical() << "ImcCore::importPublicKey(): could not remove "
							"existing public key file" << destFile.fileName();
			return false;
		}
	}

	// now try to copy it
	return dsaKey.save( pub );
}



QString icaFilePath()
{
	QString path = QCoreApplication::applicationDirPath() + QDir::separator() + "ica";
#ifdef ITALC_BUILD_WIN32
	path += ".exe";
#endif
	return QDTNS( path );
}




void informationMessage( const QString &title, const QString &msg )
{
	LogStream( Logger::LogLevelInfo ) << title.toUtf8().constData()
								<< ":" << msg.toUtf8().constData();
	if( QApplication::type() != QApplication::Tty &&
			!QCoreApplication::instance()->arguments().contains( "-quiet" ) )
	{
		QMessageBox::information( NULL, title, msg );
	}
}



void criticalMessage( const QString &title, const QString &msg )
{
	LogStream( Logger::LogLevelCritical ) << title.toUtf8().constData()
								<< ":" << msg.toUtf8().constData();
	if( QApplication::type() != QApplication::Tty &&
			!QCoreApplication::instance()->arguments().contains( "-quiet" ) )
	{
		QMessageBox::critical( NULL, title, msg );
	}
}



}


