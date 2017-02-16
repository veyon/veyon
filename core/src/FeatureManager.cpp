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
#include "PluginInterface.h"
#include "PluginManager.h"

static const Feature::Uid uidPresentationFullScreen = Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" );
static const Feature::Uid uidPresentationWindow = Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" );

FeatureManager::FeatureManager( PluginManager& pluginManager ) :
	QObject( &pluginManager ),
	m_pluginManager( pluginManager ),
	m_features(),
	m_emptyFeatureList()
{
	for( auto pluginInterface : m_pluginManager.pluginInterfaces() )
	{
		auto featureInterface = dynamic_cast<FeaturePluginInterface *>( pluginInterface );
		if( featureInterface )
		{
			m_features += featureInterface->featureList();
			m_featureInterfaces += featureInterface;
		}
	}

	m_features += Feature( Feature::Mode, Feature::ScopeAll, uidPresentationFullScreen,
						   tr( "Fullscreen presentation" ), tr( "Stop presentation" ),
						   tr( "In this mode your screen is being displayed on "
							   "all computers. Furthermore the users "
							   "aren't able to do something else as all input "
							   "devices are locked in this mode." ),
						   ":/resources/presentation-fullscreen.png" );

	m_features += Feature( Feature::Mode, Feature::ScopeAll, uidPresentationWindow,
						   tr( "Window presentation" ), tr( "Stop presentation" ),
						   tr( "In this mode your screen being displayed in a "
							   "window on all computers. The users are "
							   "able to switch to other windows and thus "
							   "can continue to work." ),
						   ":/resources/presentation-window.png" );
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



void FeatureManager::startMasterFeature( const Feature& feature, const ComputerControlInterfaceList& computerControlInterfaces, QWidget* parent )
{
	qDebug() << "Run master feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : m_featureInterfaces )
	{
		featureInterface->startMasterFeature( feature, computerControlInterfaces, parent );
	}
}



void FeatureManager::stopMasterFeature( const Feature& feature, const ComputerControlInterfaceList& computerControlInterfaces, QWidget* parent )
{
	qDebug() << "Stop master feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : m_featureInterfaces )
	{
		featureInterface->stopMasterFeature( feature, computerControlInterfaces, parent );
	}
}



bool FeatureManager::handleServiceFeatureMessage( const FeatureMessage& message,
												  QIODevice* ioDevice,
												  FeatureWorkerManager& featureWorkerManager )
{
	qDebug() << "FeatureManager::handleServiceFeatureMessage():" << message.featureUid();

	bool handled = false;

	for( auto featureInterface : m_featureInterfaces )
	{
		if( featureInterface->handleServiceFeatureMessage( message, ioDevice, featureWorkerManager ) )
		{
			handled = true;
		}
	}

	return handled;
}



bool FeatureManager::handleWorkerFeatureMessage( const FeatureMessage& message, QIODevice* ioDevice )
{
	qDebug() << "FeatureManager::handleWorkerFeatureMessage():" << message.featureUid();

	bool handled = false;

	for( auto featureInterface : m_featureInterfaces )
	{
		if( featureInterface->handleWorkerFeatureMessage( message, ioDevice ) )
		{
			handled = true;
		}
	}

	return handled;
}
