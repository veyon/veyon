/*
 * main.cpp - main file for Veyon Control
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <openssl/crypto.h>

#include "CommandLinePluginInterface.h"
#include "Logger.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QCoreApplication* app = nullptr;

#ifdef Q_OS_LINUX
	// do not create graphical application if DISPLAY is not available
	if( qEnvironmentVariableIsSet( "DISPLAY" ) == false )
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

	if( arguments.count() == 2 )
	{
		if( arguments.last() == QStringLiteral("-v") || arguments.last() == QStringLiteral("--version") )
		{
			printf( "%s\n", qUtf8Printable( VeyonCore::version() ) );
			delete app;
			return 0;
		}
		else if( arguments.last() == QStringLiteral("about") )
		{
			printf( "Veyon: %s (%s)\n", qUtf8Printable( VeyonCore::version() ), __DATE__ );
			printf( "Qt: %s (built against %s/%s)\n", qVersion(), QT_VERSION_STR, qUtf8Printable( QSysInfo::buildAbi() ) );
			printf( "OpenSSL: %s\n", SSLeay_version(SSLEAY_VERSION) );
			delete app;
			return 0;
		}
	}

	// disable logging at all in order to avoid clobbering
	if( qEnvironmentVariableIsEmpty( Logger::logLevelEnvironmentVariable() ) )
	{
		qputenv( Logger::logLevelEnvironmentVariable(), QByteArray::number( Logger::LogLevelNothing ) );
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

	const auto module = arguments.value( 1 );

	for( auto it = commandLinePluginInterfaces.constBegin(), end = commandLinePluginInterfaces.constEnd(); it != end; ++it )
	{
		if( it.key()->commandLineModuleName() == module )
		{
			auto runResult = CommandLinePluginInterface::Unknown;

			if( arguments.count() > 2 )
			{
				const auto handler = QStringLiteral( "handle_%1(QStringList)" ).arg( arguments[2] );
				if( it.value()->metaObject()->indexOfMethod( handler.toUtf8().constData() ) >= 0 )
				{
					QMetaObject::invokeMethod( it.value(),
											   QStringLiteral( "handle_%1" ).arg( arguments[2] ).toUtf8().constData(),
											   Qt::DirectConnection,
											   Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult),
											   Q_ARG( QStringList, arguments.mid( 3 ) ) );
				}
				else if( arguments[2] != QStringLiteral("help") )
				{
					runResult = CommandLinePluginInterface::InvalidCommand;
				}
			}
			else if( it.value()->metaObject()->indexOfMethod("handle_main()") >= 0 )
			{
				QMetaObject::invokeMethod( it.value(),
										   "handle_main",
										   Qt::DirectConnection,
										   Q_RETURN_ARG(CommandLinePluginInterface::RunResult, runResult) );
			}
			else
			{
				runResult = CommandLinePluginInterface::NotEnoughArguments;
			}

			delete core;

			switch( runResult )
			{
			case CommandLinePluginInterface::NoResult:
				return 0;
			case CommandLinePluginInterface::Successful:
				qInfo( "%s", qUtf8Printable( VeyonCore::tr( "[OK]" ) ) );
				return 0;
			case CommandLinePluginInterface::Failed:
				qInfo( "%s", qUtf8Printable( VeyonCore::tr( "[FAIL]" ) ) );
				return -1;
			case CommandLinePluginInterface::InvalidCommand:
				qCritical( "%s", qUtf8Printable( VeyonCore::tr( "Invalid command!" ) ) );
				break;
			case CommandLinePluginInterface::InvalidArguments:
				qCritical( "%s", qUtf8Printable( VeyonCore::tr( "Invalid arguments given" ) ) );
				return -1;
			case CommandLinePluginInterface::NotEnoughArguments:
				qCritical( "%s", qUtf8Printable(
							   VeyonCore::tr( "Not enough arguments given - "
											  "use \"%1 help\" for more information" ).arg( module ) ) );
				return -1;
			case CommandLinePluginInterface::Unknown:
				break;
			default:
				qCritical( "%s", qUtf8Printable( VeyonCore::tr( "Unknown result!" ) ) );
				return -1;
			}

			auto commands = it.key()->commands();
			std::sort( commands.begin(), commands.end() );

			qInfo( "%s", qUtf8Printable( VeyonCore::tr( "Available commands:" ) ) );
			for( const auto& command : commands )
			{
				qInfo( "    %s - %s",
					   qUtf8Printable( command ),
					   qUtf8Printable( it.key()->commandHelp( command ) ) );
			}
			return -1;
		}
	}

	delete core;

	int rc = -1;

	if( module == QStringLiteral("help") )
	{
		qCritical( "%s", qUtf8Printable( VeyonCore::tr( "Available modules:" ) ) );
		rc = 0;
	}
	else
	{
		qCritical( "%s", qUtf8Printable( VeyonCore::tr( "No module specified or module not found - available modules are:" ) ) );
	}

	QStringList modulesHelpStrings;
	for( auto it = commandLinePluginInterfaces.constBegin(), end = commandLinePluginInterfaces.constEnd(); it != end; ++it )
	{
		modulesHelpStrings.append( QStringLiteral( "%1 - %2" ).arg( it.key()->commandLineModuleName(),
																		it.key()->commandLineModuleHelp() ) );
	}

	std::sort( modulesHelpStrings.begin(), modulesHelpStrings.end() );
	std::for_each( modulesHelpStrings.begin(), modulesHelpStrings.end(), [](const QString& s) { qCritical( "    %s", qUtf8Printable( s ) ); } );

	delete app;

	return rc;
}
