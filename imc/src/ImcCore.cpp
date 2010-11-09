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

#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "Logger.h"
#include "SystemConfigurationModifier.h"


namespace ImcCore
{

// static data initialization
ItalcConfiguration *config = NULL;

MainWindow *mainWindow = NULL;

void init()
{
	config = new ItalcConfiguration( Configuration::Store::XmlFile );
}




void deinit()
{
	delete config;
}


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
	*config += c;

	// do neccessary modifications of system configuration
	if( !SystemConfigurationModifier::setServiceAutostart( config->autostartService() ) )
	{
		configApplyError( app->tr( "Could not modify the autostart property "
									"for the iTALC Client Service." ) );
	}

	if( !SystemConfigurationModifier::setServiceArguments( config->serviceArguments() ) )
	{
		configApplyError( app->tr( "Could not modify the service arguments "
									"for the iTALC Client Service." ) );
	}
	if( !SystemConfigurationModifier::enableFirewallException( config->isFirewallExceptionEnabled() ) )
	{
		configApplyError( app->tr( "Could not change the firewall configuration "
									"for the iTALC Client Service." ) );
	}

	// write global configuration
	config->flushStore();
}



QString icaFilePath()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + "ica";
}



}


