/*
 * main.cpp - main file for Veyon Control
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QApplication>
#include <QProcessEnvironment>

#include "CommandLinePluginInterface.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QCoreApplication* app = nullptr;

#ifdef VEYON_BUILD_LINUX
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

	if( app->arguments().last() == "-v" || app->arguments().last() == "--version" )
	{
		printf( "%s\n", VEYON_VERSION );
		return 0;
	}

	VeyonCore* core = new VeyonCore( app, "Control" );

	QMap<CommandLinePluginInterface *, QObject *> commandLinePluginInterfaces;

	for( auto pluginObject : core->pluginManager().pluginObjects() )
	{
		auto commandLinePluginInterface = qobject_cast<CommandLinePluginInterface *>( pluginObject );
		if( commandLinePluginInterface )
		{
			commandLinePluginInterfaces[commandLinePluginInterface] = pluginObject;
		}
	}

	QString command = app->arguments()[1];

	for( auto commandLinePluginInterface : commandLinePluginInterfaces.keys() )
	{
		if( commandLinePluginInterface->commandName() == command )
		{
			CommandLinePluginInterface::RunResult runResult = CommandLinePluginInterface::Unknown;

			if( app->arguments().count() > 2 )
			{
				QString subCommand = app->arguments()[2];

				if( commandLinePluginInterface->subCommands().contains( subCommand ) &&
						QMetaObject::invokeMethod( commandLinePluginInterfaces[commandLinePluginInterface],
												   QString( "handle_%1" ).arg( subCommand ).toLatin1().constData(),
												   Qt::DirectConnection,
												   Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult),
												   Q_ARG( QStringList, app->arguments().mid( 3 ) ) ) )
				{
					// runResult contains result
				}
				else
				{
					runResult = commandLinePluginInterface->runCommand( app->arguments().mid( 2 ) );
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
				for( auto subCommand : commandLinePluginInterface->subCommands() )
				{
					qCritical( "    %s - %s", subCommand.toUtf8().constData(),
							   commandLinePluginInterface->subCommandHelp( subCommand ).toUtf8().constData() );
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

	for( auto commandLinePluginInterface : commandLinePluginInterfaces.keys() )
	{
		qCritical( "    %s - %s",
				   commandLinePluginInterface->commandName().toUtf8().constData(),
				   commandLinePluginInterface->commandHelp().toUtf8().constData() );
	}

	delete app;

	return -1;
}
