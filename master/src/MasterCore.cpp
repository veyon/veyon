/*
 * MasterCore.cpp - management of application-global instances
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

#include <QHostAddress>

#include "MasterCore.h"
#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "VeyonVncConnection.h"
#include "VeyonConfiguration.h"
#include "VeyonCoreConnection.h"
#include "ComputerManager.h"
#include "MonitoringMode.h"
#include "UserConfig.h"
#include "PluginManager.h"


MasterCore::MasterCore() :
	m_builtinFeatures( new BuiltinFeatures() ),
	m_featureManager( new FeatureManager() ),
	m_features( featureList() ),
	m_localComputer( NetworkObject::Uid::createUuid(), "localhost", QHostAddress( QHostAddress::LocalHost ).toString() ),
	m_localComputerControlInterface( m_localComputer ),
	m_userConfig( new UserConfig( Configuration::Store::JsonFile ) ),
	m_computerManager( new ComputerManager( *m_userConfig, *m_featureManager, *m_builtinFeatures, this ) ),
	m_currentMode( m_builtinFeatures->monitoringMode().feature().uid() )
{
	connect( &m_localComputerControlInterface, &ComputerControlInterface::featureMessageReceived,
			 m_featureManager, &FeatureManager::handleMasterFeatureMessage );

	m_localComputerControlInterface.start( QSize(), &m_builtinFeatures->userSessionControl() );
}



MasterCore::~MasterCore()
{
	delete m_computerManager;

	m_userConfig->flushStore();
	delete m_userConfig;

	delete m_featureManager;
}



void MasterCore::runFeature( const Feature& feature, QWidget* parent )
{
	ComputerControlInterfaceList computerControlInterfaces = m_computerManager->computerControlInterfaces();

	if( feature.testFlag( Feature::Mode ) )
	{
		// stop any previously active featues
		for( const auto& feature : qAsConst( features() ) )
		{
			if( feature.testFlag( Feature::Mode ) )
			{
				m_featureManager->stopMasterFeature( feature, computerControlInterfaces,
													 m_localComputerControlInterface, parent );
			}
		}

		if( m_currentMode == feature.uid() )
		{
			const Feature& monitoringModeFeature = m_builtinFeatures->monitoringMode().feature();

			m_featureManager->startMasterFeature( monitoringModeFeature, computerControlInterfaces,
												  m_localComputerControlInterface, parent );
			m_currentMode = monitoringModeFeature.uid();
		}
		else
		{
			m_featureManager->startMasterFeature( feature, computerControlInterfaces,
												  m_localComputerControlInterface, parent );
			m_currentMode = feature.uid();
		}
	}
	else
	{
		m_featureManager->startMasterFeature( feature, computerControlInterfaces,
											  m_localComputerControlInterface, parent );
	}
}



FeatureList MasterCore::featureList() const
{
	FeatureList features;

	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	const auto pluginUids = VeyonCore::pluginManager().pluginUids();

	for( const auto& pluginUid : pluginUids )
	{
		for( const auto& feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.testFlag( Feature::Master ) &&
					feature.testFlag( Feature::Mode ) &&
					disabledFeatures.contains( feature.uid().toString() ) == false )
			{
				features += feature;
			}
		}
	}

	for( const auto& pluginUid : pluginUids )
	{
		for( const auto& feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.testFlag( Feature::Master ) &&
					feature.testFlag( Feature::Mode ) == false &&
					disabledFeatures.contains( feature.uid().toString() ) == false )
			{
				features += feature;
			}
		}
	}

	return features;
}
