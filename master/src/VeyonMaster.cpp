/*
 * VeyonMaster.cpp - management of application-global instances
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

#include <QHostAddress>

#include "VeyonMaster.h"
#include "BuiltinFeatures.h"
#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"
#include "FeatureManager.h"
#include "VncConnection.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "MainWindow.h"
#include "ComputerManager.h"
#include "MonitoringMode.h"
#include "UserConfig.h"
#include "PluginManager.h"


VeyonMaster::VeyonMaster( QObject* parent ) :
	VeyonMasterInterface( parent ),
	m_userConfig( new UserConfig( Configuration::Store::JsonFile ) ),
	m_featureManager( new FeatureManager() ),
	m_features( featureList() ),
	m_computerManager( new ComputerManager( *m_userConfig, this ) ),
	m_computerControlListModel( new ComputerControlListModel( this, this ) ),
	m_computerMonitoringModel( new ComputerMonitoringModel( this ) ),
	m_localSessionControlInterface( Computer( NetworkObject::Uid::createUuid(),
											  QStringLiteral("localhost"),
											  QStringLiteral("%1:%2").
											  arg( QHostAddress( QHostAddress::LocalHost ).toString() ).
											  arg( VeyonCore::config().veyonServerPort() + VeyonCore::sessionId() ) ),
									this ),
	m_mainWindow( nullptr ),
	m_currentMode( VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
{
	if( VeyonCore::config().enforceSelectedModeForClients() )
	{
		connect( m_computerControlListModel, &ComputerControlListModel::activeFeaturesChanged,
				 this, &VeyonMaster::enforceDesignatedMode );
	}

	connect( &m_localSessionControlInterface, &ComputerControlInterface::featureMessageReceived,
			 this, [=]( const FeatureMessage& featureMessage, const ComputerControlInterface::Pointer& computerControlInterface ) {
			 m_featureManager->handleFeatureMessage( computerControlInterface, featureMessage );
	} );

	m_localSessionControlInterface.start();

	// attach computer list model to proxy model
	m_computerMonitoringModel->setSourceModel( m_computerControlListModel );
	m_computerMonitoringModel->setSortRole( Qt::InitialSortOrderRole );
	m_computerMonitoringModel->setStateRole( ComputerControlListModel::StateRole );
	m_computerMonitoringModel->sort( 0 );

	m_mainWindow = new MainWindow( *this );
}



VeyonMaster::~VeyonMaster()
{
	stopAllModeFeatures( m_computerControlListModel->computerControlInterfaces() );

	delete m_mainWindow;

	delete m_computerManager;

	m_userConfig->flushStore();
	delete m_userConfig;

	delete m_featureManager;
}



FeatureList VeyonMaster::subFeatures( Feature::Uid parentFeatureUid ) const
{
	FeatureList features;

	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	const auto pluginUids = VeyonCore::pluginManager().pluginUids();

	for( const auto& pluginUid : pluginUids )
	{
		for( const auto& feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.testFlag( Feature::Master ) &&
				feature.parentUid() == parentFeatureUid &&
				disabledFeatures.contains( parentFeatureUid.toString() ) == false &&
				disabledFeatures.contains( feature.uid().toString() ) == false )
			{
				features += feature;
			}
		}
	}

	return features;
}



FeatureUidList VeyonMaster::subFeaturesUids( Feature::Uid parentFeatureUid ) const
{
	FeatureUidList featureUids;

	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	const auto pluginUids = VeyonCore::pluginManager().pluginUids();

	for( const auto& pluginUid : pluginUids )
	{
		for( const auto& feature : m_featureManager->features( pluginUid ) )
		{
			if( feature.testFlag( Feature::Master ) &&
				feature.parentUid() == parentFeatureUid &&
				disabledFeatures.contains( parentFeatureUid.toString() ) == false &&
				disabledFeatures.contains( feature.uid().toString() ) == false )
			{
				featureUids += feature.uid();
			}
		}
	}

	return featureUids;
}



FeatureUidList VeyonMaster::metaFeaturesUids( const FeatureUidList& featureUids ) const
{
	FeatureUidList metaFeatureUids;
	metaFeatureUids.reserve( featureUids.size() );

	for( const auto& featureUid : featureUids )
	{
		const auto metaFeatureUid = m_featureManager->metaFeatureUid( featureUid );
		if( metaFeatureUid.isNull() == false )
		{
			metaFeatureUids.append( metaFeatureUid );
		}
	}

	return metaFeatureUids;
}



FeatureList VeyonMaster::modeFeatures() const
{
	FeatureList featureList;

	for( const auto& feature : qAsConst( features() ) )
	{
		if( feature.testFlag( Feature::Mode ) )
		{
			featureList.append( feature );
			const auto modeSubFeatures = subFeatures( feature.uid() );
			for( const auto& subFeature : modeSubFeatures )
			{
				featureList.append( subFeature );
			}
		}
	}

	return featureList;
}



QWidget* VeyonMaster::mainWindow()
{
	return m_mainWindow;
}



Configuration::Object* VeyonMaster::userConfigurationObject()
{
	return m_userConfig;
}



void VeyonMaster::reloadSubFeatures()
{
	m_mainWindow->reloadSubFeatures();
}



ComputerControlInterfaceList VeyonMaster::selectedComputerControlInterfaces() const
{
	return m_mainWindow->selectedComputerControlInterfaces();
}



ComputerControlInterfaceList VeyonMaster::filteredComputerControlInterfaces()
{
	const auto rowCount = m_computerMonitoringModel->rowCount();

	ComputerControlInterfaceList computerControlInterfaces;
	computerControlInterfaces.reserve( rowCount );

	for( int i = 0; i < rowCount; ++i )
	{
		const auto index = m_computerMonitoringModel->index( i, 0 );
		const auto sourceIndex = m_computerMonitoringModel->mapToSource( index );
		computerControlInterfaces.append( m_computerControlListModel->computerControlInterface( sourceIndex ) );
	}

	return computerControlInterfaces;
}



void VeyonMaster::runFeature( const Feature& feature )
{
	const auto computerControlInterfaces = filteredComputerControlInterfaces();

	if( feature.testFlag( Feature::Mode ) )
	{
		stopAllModeFeatures( computerControlInterfaces );

		if( m_currentMode == feature.uid() ||
			subFeatures( feature.uid() ).contains( m_featureManager->feature( m_currentMode ) ) )
		{
			const Feature& monitoringModeFeature = VeyonCore::builtinFeatures().monitoringMode().feature();

			m_featureManager->startFeature( *this, monitoringModeFeature, computerControlInterfaces );
			m_currentMode = monitoringModeFeature.uid();
		}
		else
		{
			m_featureManager->startFeature( *this, feature, computerControlInterfaces );
			m_currentMode = feature.uid();
		}
	}
	else
	{
		m_featureManager->startFeature( *this, feature, computerControlInterfaces );
	}
}



void VeyonMaster::enforceDesignatedMode( const QModelIndex& index )
{
	auto controlInterface = m_computerControlListModel->computerControlInterface( index );
	if( controlInterface )
	{
		const auto designatedModeFeature = controlInterface->designatedModeFeature();

		if( designatedModeFeature != VeyonCore::builtinFeatures().monitoringMode().feature().uid() &&
			controlInterface->activeFeatures().contains( designatedModeFeature ) == false &&
			controlInterface->activeFeatures().contains( m_featureManager->metaFeatureUid(designatedModeFeature) ) == false )
		{
			featureManager().startFeature( *this, m_featureManager->feature(designatedModeFeature), { controlInterface } );
		}
	}
}



void VeyonMaster::stopAllModeFeatures( const ComputerControlInterfaceList& computerControlInterfaces )
{
	const auto features = modeFeatures();

	for( const auto& feature : features )
	{
		m_featureManager->stopFeature( *this, feature, computerControlInterfaces );
	}
}



FeatureList VeyonMaster::featureList() const
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
				feature.testFlag( Feature::Meta ) == false &&
				feature.parentUid().isNull() &&
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
				feature.testFlag( Feature::Meta ) == false &&
				feature.parentUid().isNull() &&
				disabledFeatures.contains( feature.uid().toString() ) == false )
			{
				features += feature;
			}
		}
	}

	return features;
}
