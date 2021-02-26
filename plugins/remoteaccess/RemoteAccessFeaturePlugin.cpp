/*
 * RemoteAccessFeaturePlugin.cpp - implementation of RemoteAccessFeaturePlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QInputDialog>

#include "AuthenticationCredentials.h"
#include "RemoteAccessFeaturePlugin.h"
#include "RemoteAccessWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"


RemoteAccessFeaturePlugin::RemoteAccessFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_remoteViewFeature( QStringLiteral( "RemoteView" ),
						 Feature::Session | Feature::Master,
						 Feature::Uid( "a18e545b-1321-4d4e-ac34-adc421c6e9c8" ),
						 Feature::Uid(),
						 tr( "Remote view" ), {},
						 tr( "Open a remote view for a computer without interaction." ),
						 QStringLiteral(":/remoteaccess/kmag.png") ),
	m_remoteControlFeature( QStringLiteral( "RemoteControl" ),
							Feature::Session | Feature::Master,
							Feature::Uid( "ca00ad68-1709-4abe-85e2-48dff6ccf8a2" ),
							Feature::Uid(),
							tr( "Remote control" ), {},
							tr( "Open a remote control window for a computer." ),
							QStringLiteral(":/remoteaccess/krdc.png") ),
	m_features( { m_remoteViewFeature, m_remoteControlFeature } ),
	m_commands( {
{ QStringLiteral("view"), m_remoteViewFeature.displayName() },
{ QStringLiteral("control"), m_remoteControlFeature.displayName() },
{ QStringLiteral("help"), tr( "Show help about command" ) },
				} )
{
}



const FeatureList &RemoteAccessFeaturePlugin::featureList() const
{
	return m_features;
}



bool RemoteAccessFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
											   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( hasFeature( featureUid ) == false ||
		operation != Operation::Start )
	{
		return false;
	}

	auto viewOnly = featureUid == m_remoteViewFeature.uid();
	if( remoteControlEnabled() == false )
	{
		viewOnly = true;
	}

	Computer computer;
	computer.setHostAddress( arguments.value( argToString(Argument::HostName) ).toString() );
	computer.setName( computer.hostAddress() );

	if( computer.hostAddress().isEmpty() )
	{
		if( computerControlInterfaces.isEmpty() == false )
		{
			computer = computerControlInterfaces.first()->computer();
		}
		else
		{
			return false;
		}
	}

	new RemoteAccessWidget( ComputerControlInterface::Pointer::create( computer ), viewOnly,
							remoteViewEnabled() && remoteControlEnabled() );

	return true;
}



bool RemoteAccessFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( hasFeature( feature.uid() ) == false )
	{
		return false;
	}

	// determine which computer to access and ask if neccessary
	ComputerControlInterface::Pointer remoteAccessComputer;

	if( computerControlInterfaces.count() != 1 )
	{
		QString hostName = QInputDialog::getText( master.mainWindow(),
												  tr( "Remote access" ),
												  tr( "Please enter the hostname or IP address of the computer to access:" ) );
		if( hostName.isEmpty() )
		{
			return false;
		}

		Computer customComputer;
		customComputer.setHostAddress( hostName );
		customComputer.setName( hostName );
		remoteAccessComputer = ComputerControlInterface::Pointer::create( customComputer );
	}
	else if( computerControlInterfaces.count() >= 1 )
	{
		remoteAccessComputer = computerControlInterfaces.first();
	}

	if( remoteAccessComputer.isNull() )
	{
		return false;
	}

	auto viewOnly = feature.uid() == m_remoteViewFeature.uid();
	if( remoteControlEnabled() == false )
	{
		viewOnly = true;
	}

	new RemoteAccessWidget( remoteAccessComputer, viewOnly,
							remoteViewEnabled() && remoteControlEnabled() );

	return false;
}



QStringList RemoteAccessFeaturePlugin::commands() const
{
	return m_commands.keys();
}



QString RemoteAccessFeaturePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_view( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	if( remoteViewEnabled() == false )
	{
		return InvalidCommand;
	}

	return remoteAccess( arguments.first(), true ) ? Successful : Failed;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_control( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	if( remoteControlEnabled() == false )
	{
		return InvalidCommand;
	}

	return remoteAccess( arguments.first(), false ) ? Successful : Failed;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_help( const QStringList& arguments )
{
	if( arguments.value( 0 ) == QLatin1String("view") )
	{
		printf( "\nremoteaccess view <host>\n\n" );
		return NoResult;
	}
	else if( arguments.value( 0 ) == QLatin1String("control") )
	{
		printf( "\nremoteaccess control <host>\n}n" );
		return NoResult;
	}

	return InvalidCommand;
}



bool RemoteAccessFeaturePlugin::remoteViewEnabled() const
{
	return VeyonCore::config().disabledFeatures().contains( m_remoteViewFeature.uid().toString() ) == false;

}



bool RemoteAccessFeaturePlugin::remoteControlEnabled() const
{
	return VeyonCore::config().disabledFeatures().contains( m_remoteControlFeature.uid().toString() ) == false;
}



bool RemoteAccessFeaturePlugin::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() == false )
	{
		vWarning() << "Could not initialize authentication";
		return false;
	}

	return true;
}



bool RemoteAccessFeaturePlugin::remoteAccess( const QString& hostAddress, bool viewOnly )
{
	if( initAuthentication() == false )
	{
		return false;
	}

	Computer remoteComputer;
	remoteComputer.setName( hostAddress );
	remoteComputer.setHostAddress( hostAddress );

	if( remoteControlEnabled() == false )
	{
		viewOnly = true;
	}

	new RemoteAccessWidget( ComputerControlInterface::Pointer::create( remoteComputer ), viewOnly,
							remoteViewEnabled() && remoteControlEnabled() );

	qApp->exec();

	return true;
}
