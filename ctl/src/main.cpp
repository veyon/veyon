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

#include <openssl/crypto.h>

#include "CommandLinePluginInterface.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QCoreApplication* app = nullptr;

#ifdef VEYON_BUILD_LINUX
	// do not create graphical application if DISPLAY is not available
	if( QProcessEnvironment::systemEnvironment().contains( QStringLiteral("DISPLAY") ) == false )
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

	const auto arguments = app->arguments();

	if( arguments.count() < 2 )
	{
		qCritical( "ERROR: not enough arguments - please specify a command" );

		return -1;
	}

	if( arguments.count() == 2 )
	{
		if( arguments.last() == QStringLiteral("-v") || arguments.last() == QStringLiteral("--version") )
		{
			printf( "%s\n", VEYON_VERSION );
			return 0;
		}
		else if( arguments.last() == QStringLiteral("about") )
		{
			printf( "Veyon: %s (%s)\n", VEYON_VERSION, __DATE__ );
			printf( "Qt: %s (built against %s/%s)\n", qVersion(), QT_VERSION_STR, qPrintable( QSysInfo::buildAbi() ) );
			printf( "OpenSSL: %s\n", SSLeay_version(SSLEAY_VERSION) );
			return 0;
		}
	}

	VeyonCore* core = new VeyonCore( app, QStringLiteral("Control") );

	QHash<CommandLinePluginInterface *, QObject *> commandLinePluginInterfaces;
	const auto pluginObjects = core->pluginManager().pluginObjects();
	for( auto pluginObject : pluginObjects )
	{
		auto commandLinePluginInterface = qobject_cast<CommandLinePluginInterface *>( pluginObject );
		if( commandLinePluginInterface )
		{
			commandLinePluginInterfaces[commandLinePluginInterface] = pluginObject;
		}
	}

	const auto module = arguments[1];

	for( auto it = commandLinePluginInterfaces.constBegin(), end = commandLinePluginInterfaces.constEnd(); it != end; ++it )
	{
		const auto commands = it.key()->commands();

		if( it.key()->commandLineModuleName() == module )
		{
			CommandLinePluginInterface::RunResult runResult = CommandLinePluginInterface::Unknown;

			if( arguments.count() > 2 )
			{
				QString command = arguments[2];

				if( commands.contains( command ) &&
						QMetaObject::invokeMethod( it.value(),
												   QStringLiteral( "handle_%1" ).arg( command ).toUtf8().constData(),
												   Qt::DirectConnection,
												   Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult),
												   Q_ARG( QStringList, arguments.mid( 3 ) ) ) )
				{
					// runResult contains result
				}
				else
				{
					runResult = it.key()->runCommand( arguments.mid( 2 ) );
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
				qInfo( "%s", qPrintable( VeyonCore::tr( "[OK]" ) ) );
				return 0;
			case CommandLinePluginInterface::Failed:
				qInfo( "%s", qPrintable( VeyonCore::tr( "[FAIL]" ) ) );
				return -1;
			case CommandLinePluginInterface::InvalidCommand:
				if( arguments.contains( QStringLiteral("help") ) == false )
				{
					qCritical( "%s", qPrintable( VeyonCore::tr( "Invalid command!" ) ) );
				}
				qCritical( "%s", qPrintable( VeyonCore::tr( "Available commands:" ) ) );
				for( const auto& command : commands )
				{
					qCritical( "    %s - %s", qPrintable( command ),
							   qPrintable( it.key()->commandHelp( command ) ) );
				}
				return -1;
			case CommandLinePluginInterface::InvalidArguments:
				qCritical( "%s", qPrintable( VeyonCore::tr( "Invalid arguments given" ) ) );
				return -1;
			case CommandLinePluginInterface::NotEnoughArguments:
				qCritical( "%s", qPrintable(
							   VeyonCore::tr( "Not enough arguments given - "
											  "use \"%1 help\" for more information" ).arg( module ) ) );
				return -1;
			case CommandLinePluginInterface::NoResult:
				return 0;
			default:
				qCritical( "%s", qPrintable( VeyonCore::tr( "Unknown result!" ) ) );
				return -1;
			}
		}
	}

	delete core;

	int rc = -1;

	if( module == QStringLiteral("help") )
	{
		qCritical( "%s", qPrintable( VeyonCore::tr( "Available modules:" ) ) );
		rc = 0;
	}
	else
	{
		qCritical( "%s", qPrintable( VeyonCore::tr( "Module not found - available modules are:" ) ) );
	}

	for( auto it = commandLinePluginInterfaces.constBegin(), end = commandLinePluginInterfaces.constEnd(); it != end; ++it )
	{
		qCritical( "    %s - %s",
				   qPrintable( it.key()->commandLineModuleName() ),
				   qPrintable( it.key()->commandLineModuleHelp() ) );
	}

	delete app;

	return rc;
}
