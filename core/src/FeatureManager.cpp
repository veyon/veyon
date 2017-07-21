/*
 * FeatureManager.cpp - implementation of the FeatureManager class
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

#include <QDebug>

#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "PluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"

Q_DECLARE_METATYPE(Feature)
Q_DECLARE_METATYPE(FeatureMessage)

// clazy:excludeall=reserve-candidates

FeatureManager::FeatureManager( QObject* parent ) :
	QObject( parent ),
	m_features(),
	m_emptyFeatureList(),
	m_pluginObjects(),
	m_dummyFeature()
{
	qRegisterMetaType<Feature>();
	qRegisterMetaType<FeatureMessage>();

	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto featurePluginInterface = qobject_cast<FeaturePluginInterface *>( pluginObject );

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
		auto featurePluginInterface = qobject_cast<FeaturePluginInterface *>( pluginObject );

		if( pluginInterface && featurePluginInterface && pluginInterface->uid() == pluginUid )
		{
			return featurePluginInterface->featureList();
		}
	}

	return m_emptyFeatureList;
}



const Feature& FeatureManager::feature( Feature::Uid featureUid ) const
{
	for( auto featureInterface : m_featurePluginInterfaces )
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
		auto featurePluginInterface = qobject_cast<FeaturePluginInterface *>( pluginObject );

		if( pluginInterface && featurePluginInterface &&
				featurePluginInterface->featureList().contains( feature ) )
		{
			return pluginInterface->uid();
		}
	}

	return Plugin::Uid();
}



void FeatureManager::startMasterFeature( const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces,
										 ComputerControlInterface& localComputerControlInterface,
										 QWidget* parent )
{
	qDebug() << Q_FUNC_INFO << "feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		featureInterface->startMasterFeature( feature, computerControlInterfaces, localComputerControlInterface, parent );
	}
}



void FeatureManager::stopMasterFeature( const Feature& feature,
										const ComputerControlInterfaceList& computerControlInterfaces,
										ComputerControlInterface& localComputerControlInterface,
										QWidget* parent )
{
	qDebug() << Q_FUNC_INFO << "feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		featureInterface->stopMasterFeature( feature, computerControlInterfaces, localComputerControlInterface, parent );
	}
}



bool FeatureManager::handleMasterFeatureMessage( const FeatureMessage& message,
												 ComputerControlInterface& computerControlInterface )
{
	qDebug() << Q_FUNC_INFO
			 << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	bool handled = false;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleMasterFeatureMessage( message, computerControlInterface ) )
		{
			handled = true;
		}
	}

	return handled;
}



bool FeatureManager::handleServiceFeatureMessage( const FeatureMessage& message,
												  FeatureWorkerManager& featureWorkerManager )
{
	qDebug() << Q_FUNC_INFO
			 << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	if( VeyonCore::config().disabledFeatures().contains( message.featureUid().toString() ) )
	{
		qWarning() << Q_FUNC_INFO << " ignoring message as feature"
				   << message.featureUid() << "is disabled by configuration!";
		return false;
	}

	bool handled = false;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleServiceFeatureMessage( message, featureWorkerManager ) )
		{
			handled = true;
		}
	}

	return handled;
}



bool FeatureManager::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	qDebug() << Q_FUNC_INFO
			 << "feature" << message.featureUid()
			 << "command" << message.command()
			 << "arguments" << message.arguments();

	bool handled = false;

	for( auto featureInterface : qAsConst( m_featurePluginInterfaces ) )
	{
		if( featureInterface->handleWorkerFeatureMessage( message ) )
		{
			handled = true;
		}
	}

	return handled;
}
