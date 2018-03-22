/*
 * InternetAccessControlPlugin.cpp - implementation of InternetAccessControlPlugin class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "VeyonConfiguration.h"
#include "InternetAccessControlPlugin.h"
#include "InternetAccessControlConfigurationPage.h"
#include "InternetAccessControlBackendInterface.h"
#include "PluginManager.h"


InternetAccessControlPlugin::InternetAccessControlPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_backendInterface( nullptr ),
	m_commands( {
{ QStringLiteral("block"), tr( "Block access to the internet" ) },
{ QStringLiteral("unblock"), tr( "Allow access to the internet" ) },
{ QStringLiteral("help"), tr( "Show help about command" ) },
				} ),
	m_blockInternetFeature( Feature::Action | Feature::AllComponents,
							Feature::Uid( "c3f040c0-1a6a-4759-be32-8966fc5d3d25" ),
							Feature::Uid(),
							tr( "Block internet" ), QString(),
							tr( "Click this button to block access to the internet." ),
							QStringLiteral(":/internetaccesscontrol/internet-block.png") ),
	m_unblockInternetFeature( Feature::Action | Feature::AllComponents,
							  Feature::Uid( "804fa4f4-b577-467c-98a5-c442efdf54a7" ),
							  Feature::Uid(),
							  tr( "Unblock internet" ), QString(),
							  tr( "Click this button to allow access to the internet." ),
							  QStringLiteral(":/internetaccesscontrol/internet-unblock.png") ),
	m_features( { m_blockInternetFeature, m_unblockInternetFeature } )
{
	connect( &VeyonCore::pluginManager(), &PluginManager::pluginsLoaded,
			 this, &InternetAccessControlPlugin::initBackend );
}



InternetAccessControlPlugin::~InternetAccessControlPlugin()
{
}



QStringList InternetAccessControlPlugin::commands() const
{
	return m_commands.keys();
}



QString InternetAccessControlPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



ConfigurationPage *InternetAccessControlPlugin::createConfigurationPage()
{
	return new InternetAccessControlConfigurationPage( m_configuration );
}



CommandLinePluginInterface::RunResult InternetAccessControlPlugin::handle_block( const QStringList& arguments )
{
	Q_UNUSED(arguments);

	if( m_backendInterface )
	{
		m_backendInterface->blockInternetAccess();
		return Successful;
	}

	return Failed;
}



CommandLinePluginInterface::RunResult InternetAccessControlPlugin::handle_unblock( const QStringList& arguments )
{
	Q_UNUSED(arguments);

	if( m_backendInterface )
	{
		m_backendInterface->unblockInternetAccess();
		return Successful;
	}

	return Failed;
}




CommandLinePluginInterface::RunResult InternetAccessControlPlugin::handle_help( const QStringList& arguments )
{
	QString command = arguments.value( 0 );

	return InvalidCommand;
}



void InternetAccessControlPlugin::initBackend()
{
	InternetAccessControlBackendInterfaceList defaultInternetAccessControlBackends;

	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto backendInterface = qobject_cast<InternetAccessControlBackendInterface *>( pluginObject );

		if( pluginInterface && backendInterface )
		{
			if( pluginInterface->uid() == m_configuration.backend() )
			{
				m_backendInterface = backendInterface;
			}
			else if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
			{
				defaultInternetAccessControlBackends.append( backendInterface ); // clazy:exclude=reserve-candidates
			}
		}
	}

	if( m_backendInterface == nullptr )
	{
		if( defaultInternetAccessControlBackends.isEmpty() )
		{
			qCritical( "InternetAccessControl::initBackend(): no internet access control backend plugin found!" );
		}
		else
		{
			m_backendInterface = defaultInternetAccessControlBackends.first();
		}
	}

}



bool InternetAccessControlPlugin::startMasterFeature( const Feature& feature,
													  const ComputerControlInterfaceList& computerControlInterfaces,
													  QWidget* parent )
{
	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ), computerControlInterfaces );

	return true;
}



bool InternetAccessControlPlugin::stopMasterFeature( const Feature& feature,
													 const ComputerControlInterfaceList& computerControlInterfaces,
													 QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool InternetAccessControlPlugin::handleMasterFeatureMessage( const FeatureMessage& message,
															  ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool InternetAccessControlPlugin::handleServiceFeatureMessage( const FeatureMessage& message,
															   FeatureWorkerManager& featureWorkerManager )
{
	if( message.featureUid() == m_blockInternetFeature.uid() )
	{
	}
	else if( message.featureUid() == m_unblockInternetFeature.uid() )
	{
	}
	else
	{
		return false;
	}

	return true;
}



bool InternetAccessControlPlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}
