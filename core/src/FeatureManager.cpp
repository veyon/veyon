/*
 * FeatureManager.cpp - implementation of the FeatureManager class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "PluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"

Q_DECLARE_METATYPE(Feature)
Q_DECLARE_METATYPE(FeatureMessage)

// clazy:excludeall=reserve-candidates

FeatureManager::FeatureManager( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<Feature>();
	qRegisterMetaType<FeatureMessage>();

	for( const auto& pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface *>( pluginObject );

		if( featurePluginInterface )
		{
			m_pluginObjects += pluginObject;
			m_featurePluginInterfaces += featurePluginInterface;

			m_features += featurePluginInterface->featureList();
		}
	}

}



const FeatureList& FeatureManager::features( Plugin::Uid pluginUid ) const
{
	for( auto pluginObject : m_pluginObjects )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface *>( pluginObject );

		if( pluginInterface && featurePluginInterface && pluginInterface->uid() == pluginUid )
		{
			return featurePluginInterface->featureList();
		}
	}

	return m_emptyFeatureList;
}



const Feature& FeatureManager::feature( Feature::Uid featureUid ) const
{
	for( const auto& featureInterface : m_featurePluginInterfaces )
	{
		for( const auto& feature : featureInterface->featureList() )
		{
			if( feature.uid() == featureUid )
			{
				return feature;
			}
		}
	}

	return m_dummyFeature;
}



Plugin::Uid FeatureManager::pluginUid( const Feature& feature ) const
{
	for( auto pluginObject : m_pluginObjects )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface *>( pluginObject );

		if( pluginInterface && featurePluginInterface &&
				featurePluginInterface->featureList().contains( feature ) )
		{
			return pluginInterface->uid();
		}
	}

	return {};
}



void FeatureManager::controlFeature( Feature::Uid featureUid,
									FeatureProviderInterface::Operation operation,
									const QVariantMap& arguments,
									const ComputerControlInterfaceList& computerControlInterfaces )
{
	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		featureInterface->controlFeature( featureUid, operation, arguments, computerControlInterfaces );
	}
}



void FeatureManager::startFeature( VeyonMasterInterface& master,
								   const Feature& feature,
								   const ComputerControlInterfaceList& computerControlInterfaces )
{
	vDebug() << "feature" << feature.name() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		featureInterface->startFeature( master, feature, computerControlInterfaces );
	}

	if( feature.testFlag( Feature::Mode ) )
	{
		for( const auto& controlInterface : computerControlInterfaces )
		{
			controlInterface->setDesignatedModeFeature( feature.uid() );
		}
	}
}



void FeatureManager::stopFeature( VeyonMasterInterface& master,
								  const Feature& feature,
								  const ComputerControlInterfaceList& computerControlInterfaces )
{
	vDebug() << "feature" << feature.name() << feature.uid() << computerControlInterfaces;

	for( const auto& featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		featureInterface->stopFeature( master, feature, computerControlInterfaces );
	}

	for( const auto& controlInterface : computerControlInterfaces )
	{
		if( controlInterface->designatedModeFeature() == feature.uid() )
		{
			controlInterface->setDesignatedModeFeature( Feature::Uid() );
		}
	}
}



bool FeatureManager::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
										  const FeatureMessage& message )
{
	vDebug() << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	bool handled = false;

	for( const auto& featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleFeatureMessage( computerControlInterface, message ) )
		{
			handled = true;
		}
	}

	return handled;
}



bool FeatureManager::handleFeatureMessage( VeyonServerInterface& server,
										   const MessageContext& messageContext,
										   const FeatureMessage& message )
{
	vDebug() << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	if( VeyonCore::config().disabledFeatures().contains( message.featureUid().toString() ) )
	{
		vWarning() << "ignoring message as feature" << message.featureUid() << "is disabled by configuration!";
		return false;
	}

	bool handled = false;

	for( const auto& featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleFeatureMessage( server, messageContext, message ) )
		{
			handled = true;
		}
	}

	return handled;
}



bool FeatureManager::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	vDebug() << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	bool handled = false;

	for( const auto& featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleFeatureMessage( worker, message ) )
		{
			handled = true;
		}
	}

	return handled;
}
