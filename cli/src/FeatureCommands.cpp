/*
 * FeatureCommands.cpp - implementation of FeatureCommands class
 *
 * Copyright (c) 2021-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>

#include "ComputerControlInterface.h"
#include "FeatureCommands.h"
#include "FeatureManager.h"
#include "PluginManager.h"


FeatureCommands::FeatureCommands( QObject* parent ) :
	QObject( parent ),
	m_commands( {
		{ listCommand(), tr( "List names of all available features" ) },
		{ showCommand(), tr( "Show table with details of all available features" ) },
		{ startCommand(), tr( "Start a feature on a remote host" ) },
		{ stopCommand(), tr( "Stop a feature on a remote host" ) }
	} )
{
}



QStringList FeatureCommands::commands() const
{
	return m_commands.keys();
}



QString FeatureCommands::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult FeatureCommands::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );
	if( command.isEmpty() )
	{
		error( tr("Please specify the command to display help for.") );
		return NoResult;
	}

	if( command == listCommand() )
	{
		printUsage( commandLineModuleName(), listCommand(), {}, {} );
		printDescription( tr("Displays a list with the names of all available features.") );
		return NoResult;
	}

	if( command == showCommand() )
	{
		printUsage( commandLineModuleName(), showCommand(), {}, {} );
		printDescription( tr("Displays a table with detailed information about all available features. "
							  "This information include a description, the UID, the name of the plugin "
							  "providing the respective feature and some other implementation-related details.") );
		return NoResult;
	}

	if( command == startCommand() )
	{
		printUsage( commandLineModuleName(), startCommand(),
					{ { tr("HOST ADDRESS"), {} }, { tr("FEATURE"), {} } },
					{ { tr("ARGUMENTS"), {} } } );

		printDescription( tr("Starts the specified feature on the specified host by connecting to "
							  "the Veyon Server running remotely. The feature can be specified by name "
							  "or UID. Use the ``show`` command to see all available features. "
							  "Depending on the feature, additional arguments (such as the text message to display) "
							  "encoded as a single JSON string have to be specified. Please refer to "
							  "the developer documentation for more information") );

		printExamples( commandLineModuleName(), startCommand(),
					   {
						   { tr( "Lock the screen" ),
							   { QStringLiteral("192.168.1.2"), QStringLiteral("ScreenLock") }
						   },
						   { tr( "Display a text message" ),
							   { QStringLiteral("192.168.1.2"), QStringLiteral("TextMessage"),
								 QStringLiteral("\"{\\\"text\\\":\\\"%1\\\"}\"").arg( tr("Test message") ) }
						   },
						   { tr( "Start an application" ),
							   { QStringLiteral("192.168.1.2"), QStringLiteral("da9ca56a-b2ad-4fff-8f8a-929b2927b442"),
								 QStringLiteral("\"{\\\"applications\\\":\\\"notepad\\\"}\"") }
						   }
					   } );

		return NoResult;
	}

	if( command == stopCommand() )
	{
		printUsage( commandLineModuleName(), stopCommand(),
					{ { tr("HOST ADDRESS"), {} }, { tr("FEATURE"), {} } }, {} );

		printDescription( tr("Stops the specified feature on the specified host by connecting to "
							  "the Veyon Server running remotely. The feature can be specified by name "
							  "or UID. Use the ``show`` command to see all available features.") );

		printExamples( commandLineModuleName(), stopCommand(),
					   {
						   { tr( "Unlock the screen" ),
							   { QStringLiteral("192.168.1.2"), QStringLiteral("ScreenLock") }
						   }
					   } );

		return NoResult;
	}

	error( tr("The specified command does not exist or no help is available for it.") );

	return NoResult;
}


CommandLinePluginInterface::RunResult FeatureCommands::handle_list( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	FeatureManager featureManager;
	for( const auto& feature : std::as_const(featureManager.features()) )
	{
		print( feature.name() );
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult FeatureCommands::handle_show( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	FeatureManager featureManager;
	const auto& features = featureManager.features();

	TableHeader tableHeader( { tr("Name"), tr("Description"), tr("Master"), tr("Service"), tr("Worker"), tr("UID"), tr("Plugin") } );
	TableRows tableRows;
	tableRows.reserve( features.count() );

	for( const auto& feature : features )
	{
		if( feature.testFlag( Feature::Flag::Meta) )
		{
			continue;
		}

		tableRows.append( {
			feature.name(),
				feature.displayName(),
				feature.testFlag(Feature::Flag::Master) ? QLatin1String("x") : QString(),
				feature.testFlag(Feature::Flag::Service) ? QLatin1String("x") : QString(),
				feature.testFlag(Feature::Flag::Worker) ? QLatin1String("x") : QString(),
				feature.uid().toString(QUuid::WithoutBraces),
				VeyonCore::pluginManager().pluginName( featureManager.pluginUid(feature.uid()) )
			} );
	}

	std::sort( tableRows.begin(), tableRows.end(), []( const TableRow& a, const TableRow& b ) {
		return a.first() < b.first();
	}) ;

	printTable( Table( tableHeader, tableRows ) );

	return NoResult;
}



CommandLinePluginInterface::RunResult FeatureCommands::handle_start( const QStringList& arguments )
{
	return controlComputer( FeatureProviderInterface::Operation::Start, arguments );
}



CommandLinePluginInterface::RunResult FeatureCommands::handle_stop( const QStringList& arguments )
{
	return controlComputer( FeatureProviderInterface::Operation::Stop, arguments );
}



CommandLinePluginInterface::RunResult FeatureCommands::controlComputer( FeatureProviderInterface::Operation operation,
									  const QStringList& arguments )
{
	if( arguments.count() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto host = arguments[0];
	const auto featureNameOrUid = arguments[1];
	const auto featureArguments = arguments.value(2);

	FeatureManager featureManager;

	Feature::Uid featureUid{featureNameOrUid};
	if( featureUid.isNull() )
	{
		for( const auto& feature : featureManager.features() )
		{
			if( feature.name() == featureNameOrUid )
			{
				featureUid = feature.uid();
				break;
			}
		}
	}
	else if( featureManager.feature( featureUid ).isValid() == false )
	{
		featureUid = Feature::Uid{};
	}

	if( featureUid.isNull() )
	{
		error( tr("Invalid feature name or UID specified") );
		return InvalidArguments;
	}

	QJsonParseError parseError;
	const auto featureArgsJson = QJsonDocument::fromJson( featureArguments.toUtf8(), &parseError );

	if( featureArguments.isEmpty() == false && parseError.error != QJsonParseError::NoError )
	{
		error( tr("Error parsing the JSON-encoded arguments: %1").arg( parseError.errorString() ) );
		return InvalidArguments;
	}

	if( VeyonCore::instance()->initAuthentication() == false  )
	{
		error( tr("Failed to initialize credentials") );
		return Failed;
	}

	Computer computer;
	computer.setHostAddress(host);

	auto computerControlInterface = ComputerControlInterface::Pointer::create( computer );
	computerControlInterface->start();

	static constexpr auto ConnectTimeout = 30 * 1000;
	QTimer timeoutTimer;
	timeoutTimer.start( ConnectTimeout );

	QEventLoop eventLoop;
	connect( &timeoutTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit );
	connect( computerControlInterface.data(), &ComputerControlInterface::stateChanged, this,
			 [&]() {
				 if( computerControlInterface->state() == ComputerControlInterface::State::Connected )
				 {
					 eventLoop.quit();
				 }
			 } );

	eventLoop.exec();

	if( computerControlInterface->state() != ComputerControlInterface::State::Connected )
	{
		error( tr("Could not establish a connection to host %1").arg( host ) );
		return Failed;
	}

	featureManager.controlFeature( featureUid, operation,
								   featureArgsJson.toVariant().toMap(),
								   { computerControlInterface } );

	static constexpr auto MessageQueueWaitTimeout = 10 * 1000;
	static constexpr auto MessageQueuePollInterval = 10;

	QElapsedTimer messageQueueWaitTimer;
	messageQueueWaitTimer.start();

	while( computerControlInterface->isMessageQueueEmpty() == false &&
		   messageQueueWaitTimer.elapsed() < MessageQueueWaitTimeout )
	{
		QThread::msleep(MessageQueuePollInterval);
	}

	if( messageQueueWaitTimer.elapsed() >= MessageQueueWaitTimeout )
	{
		error( tr("Failed to send feature control message to host %1").arg( host ) );
		return Failed;
	}

	return Successful;
}
