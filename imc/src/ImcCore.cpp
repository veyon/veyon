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

#include <QtGui/QMessageBox>

#include <italcconfig.h>

#include "Configuration/LocalStore.h"
#include "DsaKey.h"
#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "SystemConfigurationModifier.h"


namespace ImcCore
{

// static data initialization
MainWindow *mainWindow = NULL;


static void configApplyError( const QString &msg )
{
	QCoreApplication *app = QCoreApplication::instance();
	if( !app->arguments().contains( "-quiet" ) )
	{
		QMessageBox::critical( NULL, app->tr( "iTALC Management Console" ), msg );
	}
}


bool applyConfiguration( const ItalcConfiguration &c )
{
	QCoreApplication *app = QCoreApplication::instance();

	// merge configuration
	*ItalcCore::config += c;

	// do neccessary modifications of system configuration
	if( !SystemConfigurationModifier::setServiceAutostart(
									ItalcCore::config->autostartService() ) )
	{
		configApplyError( app->tr( "Could not modify the autostart property "
									"for the iTALC Client Service." ) );
	}

	if( !SystemConfigurationModifier::setServiceArguments(
									ItalcCore::config->serviceArguments() ) )
	{
		configApplyError( app->tr( "Could not modify the service arguments "
									"for the iTALC Client Service." ) );
	}
	if( !SystemConfigurationModifier::enableFirewallException(
							ItalcCore::config->isFirewallExceptionEnabled() ) )
	{
		configApplyError( app->tr( "Could not change the firewall configuration "
									"for the iTALC Client Service." ) );
	}

	// write global configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	localStore.flush( ItalcCore::config );
}



bool createKeyPair( ItalcCore::UserRole role, const QString &destDir )
{
	QString priv = LocalSystem::Path::privateKeyPath( role, destDir );
	QString pub = LocalSystem::Path::publicKeyPath( role, destDir );
	LogStream() << "ImcCore: creating new key pair in" << priv << "and" << pub;

	PrivateDSAKey pkey( 1024 );
	if( !pkey.isValid() )
	{
		qCritical( "ImcCore: key generation failed!" );
		return false;
	}
	pkey.save( priv );

	PublicDSAKey( pkey ).save( pub );

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





QString icaFilePath()
{
	QString path = QCoreApplication::applicationDirPath() + QDir::separator() + "ica";
#ifdef ITALC_BUILD_WIN32
	path += ".exe";
#endif
	return QDTNS( path );
}



}


