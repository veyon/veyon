/*
 * FeatureManager.cpp - implementation of the FeatureManager class
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

#include <QDebug>

#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "PluginInterface.h"
#include "PluginManager.h"

Q_DECLARE_METATYPE(Feature)
Q_DECLARE_METATYPE(FeatureMessage)


FeatureManager::FeatureManager( PluginManager& pluginManager ) :
	QObject( &pluginManager ),
	m_pluginManager( pluginManager ),
	m_features(),
	m_emptyFeatureList(),
	m_dummyFeature()
{
	qRegisterMetaType<Feature>();
	qRegisterMetaType<FeatureMessage>();

	for( auto pluginInterface : m_pluginManager.pluginInterfaces() )
	{
		auto featureInterface = dynamic_cast<FeaturePluginInterface *>( pluginInterface );
		if( featureInterface )
		{
			m_features += featureInterface->featureList();
			m_featureInterfaces += featureInterface;
		}
	}

}



const FeatureList& FeatureManager::features( const Plugin::Uid& pluginUid ) const
{
	for( auto featureInterface : m_featureInterfaces )
	{
		if( featureInterface->uid() == pluginUid )
		{
			return featureInterface->featureList();
		}
	}

	return m_emptyFeatureList;
}



const Feature& FeatureManager::feature( const Feature::Uid& featureUid ) const
{
	for( auto featureInterface : m_featureInterfaces )
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
	for( auto featureInterface : m_featureInterfaces )
	{
		if( featureInterface->featureList().contains( feature ) )
		{
			return featureInterface->uid();
		}
	}

	return Plugin::Uid();
}



void FeatureManager::startMasterFeature( const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces,
										 ComputerControlInterface& localComputerControlInterface,
										 QWidget* parent )
{
	qDebug() << "Run master feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : m_featureInterfaces )
	{
		featureInterface->startMasterFeature( feature, computerControlInterfaces, localComputerControlInterface, parent );
	}
}



void FeatureManager::stopMasterFeature( const Feature& feature,
										const ComputerControlInterfaceList& computerControlInterfaces,
										ComputerControlInterface& localComputerControlInterface,
										QWidget* parent )
{
	qDebug() << "Stop master feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : m_featureInterfaces )
	{
		featureInterface->stopMasterFeature( feature, computerControlInterfaces, localComputerControlInterface, parent );
	}
}



bool FeatureManager::handleMasterFeatureMessage( const FeatureMessage& message,
												 ComputerControlInterface& computerControlInterface )
{
	qDebug() << "FeatureManager::handleMasterFeatureMessage():"
			 << message.featureUid()
			 << message.command()
			 << message.arguments();

	bool handled = false;

	for( auto featureInterface : m_featureInterfaces )
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
	qDebug() << "FeatureManager::handleServiceFeatureMessage():"
			 << message.featureUid()
			 << message.command()
			 << message.arguments();

	bool handled = false;

	for( auto featureInterface : m_featureInterfaces )
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
	qDebug() << "FeatureManager::handleWorkerFeatureMessage():" << message.featureUid() << message.command() << message.arguments();

	bool handled = false;

	for( auto featureInterface : m_featureInterfaces )
	{
		if( featureInterface->handleWorkerFeatureMessage( message ) )
		{
			handled = true;
		}
	}

	return handled;
}
