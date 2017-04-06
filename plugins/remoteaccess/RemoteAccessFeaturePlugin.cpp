/*
 * RemoteAccessFeaturePlugin.cpp - implementation of RemoteAccessFeaturePlugin class
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
#include <QInputDialog>

#include "RemoteAccessFeaturePlugin.h"
#include "RemoteAccessWidget.h"


RemoteAccessFeaturePlugin::RemoteAccessFeaturePlugin() :
	m_remoteViewFeature( Feature::Session | Feature::Master,
						 Feature::Uid( "a18e545b-1321-4d4e-ac34-adc421c6e9c8" ),
						 tr( "Remote view" ), QString(),
						 tr( "Open a remote view for a computer without interaction." ),
						 ":/remoteaccess/kmag.png" ),
	m_remoteControlFeature( Feature::Session | Feature::Master,
							Feature::Uid( "ca00ad68-1709-4abe-85e2-48dff6ccf8a2" ),
							tr( "Remote control" ), QString(),
							tr( "Open a remote control window for a computer." ),
							":/remoteaccess/krdc.png" ),
	m_features(),
	m_customComputer(),
	m_customComputerControlInterface( m_customComputer ),
	m_subCommands( {
				   std::pair<QString, QString>( "view", m_remoteViewFeature.displayName() ),
				   std::pair<QString, QString>( "control", m_remoteControlFeature.displayName() ),
				   std::pair<QString, QString>( "help", "show help about subcommand" ),
				   } )
{
	m_features += m_remoteViewFeature;
	m_features += m_remoteControlFeature;
}



RemoteAccessFeaturePlugin::~RemoteAccessFeaturePlugin()
{
}



const FeatureList &RemoteAccessFeaturePlugin::featureList() const
{
	return m_features;
}



bool RemoteAccessFeaturePlugin::startMasterFeature( const Feature& feature,
													const ComputerControlInterfaceList& computerControlInterfaces,
													ComputerControlInterface& localComputerControlInterface,
													QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(parent);

	// determine which computer to access and ask if neccessary
	ComputerControlInterface* remoteAccessComputer = nullptr;

	if( ( feature.uid() == m_remoteViewFeature.uid() ||
		  feature.uid() == m_remoteControlFeature.uid() ) &&
			computerControlInterfaces.count() != 1 )
	{
		QString hostName = QInputDialog::getText( parent, tr( "Remote access" ),
												  tr( "Please enter a hostname or IP address of the host to access:" ) );
		if( hostName.isEmpty() )
		{
			return false;
		}

		m_customComputer.setHostAddress( hostName );
		remoteAccessComputer = &m_customComputerControlInterface;
	}
	else
	{
		if( computerControlInterfaces.count() >= 1 )
		{
			remoteAccessComputer = computerControlInterfaces.first();
		}
	}

	if( remoteAccessComputer == nullptr )
	{
		return false;
	}

	if( feature.uid() == m_remoteViewFeature.uid() )
	{
		new RemoteAccessWidget( *remoteAccessComputer, true );

		return true;
	}
	else if( feature.uid() == m_remoteControlFeature.uid() )
	{
		new RemoteAccessWidget( *remoteAccessComputer, false );

		return true;
	}

	return false;
}



bool RemoteAccessFeaturePlugin::stopMasterFeature( const Feature& feature,
												   const ComputerControlInterfaceList& computerControlInterfaces,
												   ComputerControlInterface& localComputerControlInterface,
												   QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool RemoteAccessFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
															ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool RemoteAccessFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
															 FeatureWorkerManager& featureWorkerManager )
{
	Q_UNUSED(message);
	Q_UNUSED(featureWorkerManager);

	return false;
}



bool RemoteAccessFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}



QStringList RemoteAccessFeaturePlugin::subCommands() const
{
	return m_subCommands.keys();
}



QString RemoteAccessFeaturePlugin::subCommandHelp( const QString& subCommand ) const
{
	return m_subCommands.value( subCommand );
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::runCommand( const QStringList& arguments )
{
	return InvalidCommand;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_view( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	Computer remoteComputer;
	remoteComputer.setHostAddress( arguments.first() );

	new RemoteAccessWidget( remoteComputer, true );

	qApp->exec();

	return Successful;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_control( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	Computer remoteComputer;
	remoteComputer.setHostAddress( arguments.first() );

	new RemoteAccessWidget( remoteComputer, false );

	qApp->exec();

	return Successful;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_help( const QStringList& arguments )
{
	if( arguments.value( 0 ) == "view" )
	{
		printf( "\nremoteaccess view <host>\n\n" );
		return NoResult;
	}
	else if( arguments.value( 0 ) == "control" )
	{
		printf( "\nremoteaccess control <host>\n}n" );
		return NoResult;
	}

	return InvalidCommand;
}
