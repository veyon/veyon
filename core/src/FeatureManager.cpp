/*
 * FeatureManager.h - implementation of the FeatureManager class
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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMenu>
#include <QPluginLoader>
#include <QToolBar>

#include "FeatureManager.h"
#include "ItalcCoreConnection.h"

static const Feature::Uid uidPresentationFullScreen = Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" );
static const Feature::Uid uidPresentationWindow = Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" );
static const Feature::Uid uidComputerLock = Feature::Uid( "ccb535a2-1d24-4cc1-a709-8b47d2b2ac79" );
static const Feature::Uid uidSnapshot = Feature::Uid( "fe539932-d158-49b0-aedb-f01dc1e88cfa" );
static const Feature::Uid uidPowerOn = Feature::Uid( "f483c659-b5e7-4dbc-bd91-2c9403e70ebd" );
static const Feature::Uid uidReboot = Feature::Uid( "4f7d98f0-395a-4fff-b968-e49b8d0f748c" );
static const Feature::Uid uidPowerDown = Feature::Uid( "6f5a27a0-0e2f-496e-afcc-7aae62eede10" );
static const Feature::Uid uidRemoteControl = Feature::Uid( "fc22fa22-2469-41b9-a626-1bd9875dec41" );
static const Feature::Uid uidRemoteView = Feature::Uid( "6154bcd0-93d4-44cb-adc6-eb08edf5fae5" );

#ifdef ITALC_BUILD_WIN32
static const QStringList nameFilters("*.dll");
#else
static const QStringList nameFilters("*.so");
#endif


FeatureManager::FeatureManager( QObject* parent ) :
	QObject( parent ),
	m_monitoringModeFeature( Feature::Mode, Feature::ScopeAll,
							 Feature::Uid( "edad8259-b4ef-4ca5-90e6-f238d0fda694" ),
							 "Monitoring",
							 tr( "Monitoring" ), QString(),
							 tr( "This is the default mode and allows you to monitor all computers in the classroom." ),
							 QIcon( ":/resources/presentation-none.png" ) ),
	m_features()
{
	m_features += m_monitoringModeFeature;

	// adds a search path relative to the main executable to if the path exists.
	auto addRelativeIfExists = [this]( const QString& path )
	{
		QDir dir(qApp->applicationDirPath());
		if( !path.isEmpty() && dir.cd( path ) )
		{
			QDir::addSearchPath( "plugins", dir.absolutePath() );
		}
	};

	addRelativeIfExists( "features" );
	addRelativeIfExists( "../features" );

	for( auto fileInfo : QDir( "plugins:" ).entryInfoList( nameFilters ) )
	{
		auto featureInterface = qobject_cast<FeatureInterface *>( QPluginLoader(fileInfo.filePath()).instance() );
		if( featureInterface )
		{
			m_features += featureInterface->featureList();
			m_featureInterfaces += featureInterface;
		}
	}

	m_features += Feature( Feature::Mode, Feature::ScopeAll, uidPresentationFullScreen,
						   "Presentation",
						   tr( "Fullscreen presentation" ), tr( "Stop presentation" ),
						   tr( "In this mode your screen is being displayed on "
							   "all computers. Furthermore the users "
							   "aren't able to do something else as all input "
							   "devices are locked in this mode." ),
						   QIcon( ":/resources/presentation-fullscreen.png" ) );

	m_features += Feature( Feature::Mode, Feature::ScopeAll, uidPresentationWindow,
						   "Presentation",
						   tr( "Window presentation" ), tr( "Stop presentation" ),
						   tr( "In this mode your screen being displayed in a "
							   "window on all computers. The users are "
							   "able to switch to other windows and thus "
							   "can continue to work." ),
						   QIcon( ":/resources/presentation-window.png" ) );

	m_features += Feature( Feature::Mode, Feature::ScopeAll, uidComputerLock,
						   "ScreenLock",
						   tr( "Lock" ), tr( "Unlock" ),
						   tr( "To have all user's full attention you can lock "
							   "their computers using this button. "
							   "In this mode all input devices are locked and "
							   "the screen is black." ),
						   QIcon( ":/resources/system-lock-screen.png" ) );

	m_features += Feature( Feature::Action, Feature::ScopeMaster, uidSnapshot,
						   "Snapshot",
						   tr( "Snapshot" ), QString(),
						   tr( "Use this function to take a snapshot of all computers." ),
						   QIcon( ":/resources/camera-photo.png" ) );

	m_features += Feature( Feature::Action, Feature::ScopeAll, uidPowerOn,
						   "PowerControl",
						   tr( "Power on" ), QString(),
						   tr( "Click this button to power on all computers. "
							   "This way you do not have to power on each computer by hand." ),
						   QIcon( ":/resources/preferences-system-power-management.png" ) );

	m_features += Feature( Feature::Action, Feature::ScopeAll, uidReboot,
						   "PowerControl",
						   tr( "Reboot" ), QString(),
						   tr( "Click this button to reboot all computers." ),
						   QIcon( ":/resources/system-reboot.png" ) );

	m_features += Feature( Feature::Action, Feature::ScopeAll, uidPowerDown,
						   "PowerControl",
						   tr( "Power down" ), QString(),
						   tr( "Click this button to power down all computers. "
							   "This way you do not have to power down each computer by hand." ),
						   QIcon( ":/resources/system-shutdown.png" ) );

	m_features += Feature( Feature::Session, Feature::ScopeMaster, uidRemoteControl,
						   "RemoteControl",
						   tr( "Remote control" ), QString(),
						   tr( "Opens a remote control window" ),
						   QIcon( ":/resources/remote_control.png" ) );

	m_features += Feature( Feature::Session, Feature::ScopeMaster, uidRemoteView,
						   "RemoteControl",
						   tr( "Remote view" ), QString(),
						   tr( "Opens a remote view window" ),
						   QIcon( ":/resources/kmag.png" ) );
}



void FeatureManager::startMasterFeature( const Feature& feature, const ComputerControlInterfaceList& computerControlInterfaces, QWidget* parent )
{
	qDebug() << "Run master feature" << feature.displayName() << feature.uid() << computerControlInterfaces;

	for( auto featureInterface : m_featureInterfaces )
	{
		featureInterface->startMasterFeature( feature, computerControlInterfaces, parent );
	}

/*	if( feature.uid() == uidRemoteControl )
	{

	}
	else if( feature.uid() == uidPowerOn )
	{
		for( auto computerControlInterface : computerControlInterfaces )
		{
			if( computerControlInterface->coreConnection() )
			{
				computerControlInterface->coreConnection()->powerOnComputer( QString() );
			}
		}
	}
	else if( feature.uid() == uidPowerDown )
	{
		for( auto computerControlInterface : computerControlInterfaces )
		{
			if( computerControlInterface->coreConnection() )
			{
				computerControlInterface->coreConnection()->powerDownComputer();
			}
		}
	}*/
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
