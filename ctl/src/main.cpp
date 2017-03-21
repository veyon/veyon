/*
 * main.cpp - main file for iTALC Control
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "CommandLinePluginInterface.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QApplication app( argc, argv );

	if( app.arguments().count() < 2 )
	{
		qCritical( "ERROR: not enough arguments - please specify a command" );

		return -1;
	}

	QString command = app.arguments()[1];

	PluginManager pluginManager;
	CommandLinePluginInterfaceList commandLinePluginInterfaces;

	for( auto pluginInterface : pluginManager.pluginInterfaces() )
	{
		auto commandLinePluginInterface = dynamic_cast<CommandLinePluginInterface *>( pluginInterface );
		if( commandLinePluginInterface )
		{
			commandLinePluginInterfaces += commandLinePluginInterface;
		}
	}

	for( auto interface : commandLinePluginInterfaces )
	{
		if( interface->commandName() == command )
		{
			ItalcCore core( &app, "Control" );

			return interface->runCommand( app.arguments().mid( 2 ) ) ? 0 : -1;
		}
	}

	if( command == "help" )
	{
		qCritical( "Available commands:" );
	}
	else
	{
		qCritical( "command not found - available commands are:" );
	}

	for( auto interface : commandLinePluginInterfaces )
	{
		qCritical( "    %s - %s", interface->commandName(), interface->commandHelp() );
	}

	return -1;
}
