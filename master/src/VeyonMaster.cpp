/*
 * VeyonMaster.cpp - management of application-global instances
 *
 * Copyright (c) 2017-2024 Tobias Junghans <tobydox@veyon.io>
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
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "BuiltinFeatures.h"
#include "ComputerControlListModel.h"
#include "ComputerImageProvider.h"
#include "ComputerManager.h"
#include "ComputerMonitoringItem.h"
#include "ComputerMonitoringModel.h"
#include "FeatureManager.h"
#include "MainWindow.h"
#include "MonitoringMode.h"
#include "PluginManager.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"


VeyonMaster::VeyonMaster( QObject* parent ) :
	VeyonMasterInterface( parent ),
	m_userConfig( new UserConfig( Configuration::Store::JsonFile ) ),
	m_features( featureList() ),
	m_featureListModel( new FeatureListModel( this ) ),
	m_computerManager( new ComputerManager( *m_userConfig, this ) ),
	m_computerControlListModel( new ComputerControlListModel( this, this ) ),
	m_computerMonitoringModel( new ComputerMonitoringModel( m_computerControlListModel, this ) ),
	m_computerSelectModel( new ComputerSelectModel( m_computerManager->computerTreeModel(), this ) ),
	m_localSessionControlInterface( Computer( NetworkObject::Uid::createUuid(),
											  QStringLiteral("localhost"),
											  QStringLiteral("%1").
											  arg( QHostAddress( QHostAddress::LocalHost ).toString() ) ),
									VeyonCore::config().veyonServerPort() + VeyonCore::sessionId(),
									this ),
	m_currentMode( VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
{
	connect(m_computerControlListModel, &ComputerControlListModel::modelAboutToBeReset,
			this, &VeyonMaster::computerControlListModelAboutToReset);

	if( VeyonCore::config().enforceSelectedModeForClients() )
	{
		connect( m_computerControlListModel, &ComputerControlListModel::activeFeaturesChanged,
				 this, &VeyonMaster::enforceDesignatedMode );
	}

	m_localSessionControlInterface.start({}, ComputerControlInterface::UpdateMode::Disabled);

	initUserInterface();
}



VeyonMaster::~VeyonMaster()
{
	shutdown();

	delete m_qmlAppEngine;
	delete m_mainWindow;

	delete m_computerManager;

	m_userConfig->flushStore();
	delete m_userConfig;
}



FeatureList VeyonMaster::subFeatures( Feature::Uid parentFeatureUid ) const
{
	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	if( disabledFeatures.contains( parentFeatureUid.toString() ) )
	{
		return {};
	}

	const auto& relatedFeatures = VeyonCore::featureManager().relatedFeatures( parentFeatureUid );

	FeatureList subFeatures;
	subFeatures.reserve( relatedFeatures.size() );

	for( const auto& relatedFeature : relatedFeatures )
	{
		if( relatedFeature.testFlag( Feature::Flag::Master ) &&
			relatedFeature.parentUid() == parentFeatureUid &&
			disabledFeatures.contains( relatedFeature.uid().toString() ) == false )
		{
			subFeatures += relatedFeature;
		}
	}

	return subFeatures;
}



FeatureList VeyonMaster::allFeatures() const
{
	FeatureList featureList;

	for( const auto& feature : std::as_const( features() ) )
	{
		featureList.append(feature); // clazy:exclude=reserve-candidates
		const auto modeSubFeatures = subFeatures( feature.uid() );
		for( const auto& subFeature : modeSubFeatures )
		{
			featureList.append( subFeature );
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
	if( m_mainWindow )
	{
		m_mainWindow->reloadSubFeatures();
	}
}



const ComputerControlInterfaceList& VeyonMaster::allComputerControlInterfaces() const
{
	return m_computerControlListModel->computerControlInterfaces();
}



ComputerControlInterfaceList VeyonMaster::selectedComputerControlInterfaces() const
{
	if( m_mainWindow )
	{
		return m_mainWindow->selectedComputerControlInterfaces();
	}

	const auto monitoringItem = m_appContainer->findChild<ComputerMonitoringItem *>();
	if( monitoringItem )
	{
		return monitoringItem->selectedComputerControlInterfaces();
	}

	return {};
}



ComputerControlInterfaceList VeyonMaster::filteredComputerControlInterfaces() const
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

	if( feature.testFlag( Feature::Flag::Mode ) )
	{
		stopAllFeatures( computerControlInterfaces );

		if( m_currentMode == feature.uid() ||
			subFeatures( feature.uid() ).contains( VeyonCore::featureManager().feature( m_currentMode ) ) )
		{
			const Feature& monitoringModeFeature = VeyonCore::builtinFeatures().monitoringMode().feature();

			VeyonCore::featureManager().startFeature( *this, monitoringModeFeature, computerControlInterfaces );
			m_currentMode = monitoringModeFeature.uid();
		}
		else
		{
			VeyonCore::featureManager().startFeature( *this, feature, computerControlInterfaces );
			m_currentMode = feature.uid();
		}
	}
	else
	{
		VeyonCore::featureManager().startFeature( *this, feature, computerControlInterfaces );
	}
}



void VeyonMaster::enforceDesignatedMode( const QModelIndex& index )
{
	auto controlInterface = m_computerControlListModel->computerControlInterface( index );
	if( controlInterface )
	{
		const auto designatedModeFeature = controlInterface->designatedModeFeature();

		if( designatedModeFeature.isNull() == false &&
			designatedModeFeature != VeyonCore::builtinFeatures().monitoringMode().feature().uid() &&
			controlInterface->activeFeatures().contains( designatedModeFeature ) == false &&
			controlInterface->activeFeatures().contains( VeyonCore::featureManager().metaFeatureUid(designatedModeFeature) ) == false )
		{
			VeyonCore::featureManager().startFeature( *this, VeyonCore::featureManager().feature(designatedModeFeature),
													  { controlInterface } );
		}
	}
}



void VeyonMaster::stopAllFeatures( const ComputerControlInterfaceList& computerControlInterfaces )
{
	const auto features = allFeatures();

	for( const auto& feature : features )
	{
		VeyonCore::featureManager().stopFeature( *this, feature, computerControlInterfaces );
	}
}



void VeyonMaster::initUserInterface()
{
	if( VeyonCore::config().modernUserInterface() )
	{
		QQuickStyle::setStyle( QStringLiteral("Material") );

		m_computerControlListModel->updateComputerScreenSize();

		const auto veyonVersion = QVersionNumber( 5, 0 );
		const auto majorVersion = veyonVersion.majorVersion();
		const auto minorVersion = veyonVersion.minorVersion();

		qmlRegisterType<ComputerMonitoringItem>( "Veyon.Master", majorVersion, minorVersion, "ComputerMonitoringItem" );

		m_qmlAppEngine = new QQmlApplicationEngine( this );
		m_qmlAppEngine->addImageProvider( m_computerControlListModel->imageProvider()->id(),
										  m_computerControlListModel->imageProvider() );
		m_qmlAppEngine->rootContext()->setContextProperty( QStringLiteral("masterCore"), this );
		m_qmlAppEngine->load( QStringLiteral(":/master/MainWindow.qml") );
	}
	else
	{
		m_mainWindow = new MainWindow( *this );
	}
}



void VeyonMaster::setAppWindow( QQuickWindow* appWindow )
{
	if( m_appWindow != appWindow )
	{
		m_appWindow = appWindow;
		Q_EMIT appWindowChanged();
	}
}



void VeyonMaster::setAppContainer( QQuickItem* appContainer )
{
	if( m_appContainer != appContainer )
	{
		m_appContainer = appContainer;
		Q_EMIT appContainerChanged();
	}
}



void VeyonMaster::shutdown()
{
	stopAllFeatures( m_computerControlListModel->computerControlInterfaces() );

	const auto allMessageQueuesEmpty = [&]() {
		for( const auto& computerControlInterface : m_computerControlListModel->computerControlInterfaces() )
		{
			if( computerControlInterface->isMessageQueueEmpty() == false )
			{
				return false;
			}
		}

		return true;
	};

	static constexpr auto MessageQueueWaitTimeout = 60 * 1000;
	static constexpr auto MessageQueuePollInterval = 10;

	QElapsedTimer messageQueueWaitTimer;
	messageQueueWaitTimer.start();

	while( allMessageQueuesEmpty() == false &&
		   messageQueueWaitTimer.elapsed() < MessageQueueWaitTimeout )
	{
		QThread::msleep(MessageQueuePollInterval);
	}

	vDebug() << "finished in" << messageQueueWaitTimer.elapsed() << "ms";
}



FeatureList VeyonMaster::featureList() const
{
	FeatureList features;

	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	auto pluginUids = VeyonCore::pluginManager().pluginUids();

	std::sort( pluginUids.begin(), pluginUids.end() );

	const auto addFeatures = [&]( const std::function<bool(const Feature&)>& extraFilter )
	{
		for(const auto& pluginUid : std::as_const(pluginUids))
		{
			for( const auto& feature : VeyonCore::featureManager().features( pluginUid ) )
			{
				if( feature.testFlag( Feature::Flag::Master ) &&
					feature.testFlag( Feature::Flag::Meta ) == false &&
					feature.parentUid().isNull() &&
					disabledFeatures.contains( feature.uid().toString() ) == false &&
					extraFilter( feature ) )
				{
					features += feature;
				}
			}
		}
	};

	addFeatures( []( const Feature& feature ) { return feature.testFlag( Feature::Flag::Mode ); } );
	addFeatures( []( const Feature& feature ) { return feature.testFlag( Feature::Flag::Mode ) == false; } );

	return features;
}
