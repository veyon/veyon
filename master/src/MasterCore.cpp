/*
 * MasterCore.cpp - management of application-global instances
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

#include <QHostAddress>

#include "MasterCore.h"
#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "ItalcVncConnection.h"
#include "ItalcCoreConnection.h"
#include "ComputerManager.h"
#include "MonitoringMode.h"
#include "PersonalConfig.h"
#include "PluginManager.h"


MasterCore::MasterCore() :
	m_pluginManager( new PluginManager ),
	m_builtinFeatures( new BuiltinFeatures( *m_pluginManager ) ),
	m_featureManager( new FeatureManager( *m_pluginManager ) ),
	m_localDisplay( new ItalcVncConnection( this ) ),
	m_localService( new ItalcCoreConnection( m_localDisplay ) ),
	m_personalConfig( new PersonalConfig( Configuration::Store::JsonFile ) ),
	m_computerManager( new ComputerManager( *m_personalConfig, this ) ),
	m_currentMode()
{
	/*	ItalcVncConnection * conn = new ItalcVncConnection( this );
		// attach ItalcCoreConnection to it so we can send extended iTALC commands
		m_localICA = new ItalcCoreConnection( conn );

		conn->setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
		conn->setPort( ItalcCore::config->coreServerPort() );
		conn->start();
		*/
	m_localDisplay->setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
	m_localDisplay->setFramebufferUpdateInterval( -1 );
	m_localDisplay->start();
/*
	if( !m_localDisplay->waitForConnected( 5000 ) )
	{
		// TODO
	}
*/
}



MasterCore::~MasterCore()
{
	delete m_computerManager;
	delete m_localService;
	delete m_localDisplay;

	m_personalConfig->flushStore();
	delete m_personalConfig;

	delete m_featureManager;
}




FeatureList MasterCore::features() const
{
	FeatureList features;

	for( auto pluginUid : m_pluginManager->pluginUids() )
	{
		for( auto feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.type() == Feature::Mode )
			{
				features += feature;
			}
		}
	}

	for( auto pluginUid : m_pluginManager->pluginUids() )
	{
		for( auto feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.type() == Feature::Action ||
					feature.type() == Feature::Session ||
					feature.type() == Feature::Operation )
			{
				features += feature;
			}
		}
	}

	return features;
}




void MasterCore::runFeature( const Feature& feature, QWidget* parent )
{
	ComputerControlInterfaceList computerControlInterfaces = m_computerManager->computerControlInterfaces();

	if( feature.type() == Feature::Mode  )
	{
		m_featureManager->stopMasterFeature( Feature( m_currentMode ), computerControlInterfaces, parent );

		if( m_currentMode == feature.uid() )
		{
			const Feature& monitoringModeFeature = m_builtinFeatures->monitoringMode().feature();

			m_featureManager->startMasterFeature( monitoringModeFeature, computerControlInterfaces, parent );
			m_currentMode = monitoringModeFeature.uid();
		}
		else
		{
			m_featureManager->startMasterFeature( feature, computerControlInterfaces, parent );
			m_currentMode = feature.uid();
		}
	}
	else
	{
		m_featureManager->startMasterFeature( feature, computerControlInterfaces, parent );
	}
}
