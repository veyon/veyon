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
#include <QProcessEnvironment>

#include "CommandLinePluginInterface.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QCoreApplication* app = nullptr;

#ifdef ITALC_BUILD_LINUX
	// do not create graphical application if DISPLAY is not available
	if( QProcessEnvironment::systemEnvironment().contains( "DISPLAY" ) == false )
	{
		app = new QCoreApplication( argc, argv );
	}
	else
	{
		app = new QApplication( argc, argv );
	}
#else
	app = new QApplication( argc, argv );
#endif

	if( app->arguments().count() < 2 )
	{
		qCritical( "ERROR: not enough arguments - please specify a command" );

		return -1;
	}

	ItalcCore* core = new ItalcCore( app, "Control" );

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

	QString command = app->arguments()[1];

	for( auto interface : commandLinePluginInterfaces )
	{
		if( interface->commandName() == command )
		{
			CommandLinePluginInterface::RunResult runResult = CommandLinePluginInterface::Unknown;

			if( app->arguments().count() > 2 )
			{
				QString subCommand = app->arguments()[2];

				if( QMetaObject::invokeMethod( interface,
											   QString( "handle_%1" ).arg( subCommand ).toLatin1().constData(),
											   Qt::DirectConnection,
											   Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult),
											   Q_ARG( QStringList, app->arguments().mid( 3 ) ) ) )
				{
					// runResult contains result
				}
				else
				{
					runResult = interface->runCommand( app->arguments().mid( 2 ) );
				}
			}
			else
			{
				runResult = CommandLinePluginInterface::NotEnoughArguments;
			}

			delete core;

			switch( runResult )
			{
			case CommandLinePluginInterface::Successful:
				qInfo( "[OK]" );
				return 0;
			case CommandLinePluginInterface::Failed:
				qInfo( "[FAIL]" );
				return -1;
			case CommandLinePluginInterface::InvalidCommand:
				if( app->arguments().contains( "help" ) == false )
				{
					qCritical( "Invalid subcommand!" );
				}
				qCritical( "Available subcommands:" );
				for( auto subCommand : interface->subCommands() )
				{
					qCritical( "    %s - %s", subCommand.toUtf8().constData(),
							   interface->subCommandHelp( subCommand ).toUtf8().constData() );
				}
				return -1;
			case CommandLinePluginInterface::InvalidArguments:
				qCritical( "Invalid arguments specified" );
				return -1;
			case CommandLinePluginInterface::NotEnoughArguments:
				qCritical( "Not enough arguments given - use \"%s help\" for more information",
						   command.toUtf8().constData() );
				return -1;
			case CommandLinePluginInterface::NoResult:
				return 0;
			default:
				qCritical( "Unknown result!" );
				return -1;
			}
		}
	}

	delete core;

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
		qCritical( "    %s - %s",
				   interface->commandName().toUtf8().constData(),
				   interface->commandHelp().toUtf8().constData() );
	}

	delete app;

	return -1;
}
