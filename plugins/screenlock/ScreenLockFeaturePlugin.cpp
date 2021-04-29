/*
 * ScreenLockFeaturePlugin.cpp - implementation of ScreenLockFeaturePlugin class
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

#include <QCoreApplication>

#include "ScreenLockFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "PlatformSessionFunctions.h"
#include "VeyonServerInterface.h"


ScreenLockFeaturePlugin::ScreenLockFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_screenLockFeature( QStringLiteral( "ScreenLock" ),
						 Feature::Mode | Feature::AllComponents,
						 Feature::Uid( "ccb535a2-1d24-4cc1-a709-8b47d2b2ac79" ),
						 Feature::Uid(),
						 tr( "Lock" ), tr( "Unlock" ),
						 tr( "To reclaim all user's full attention you can lock "
							 "their computers using this button. "
							 "In this mode all input devices are locked and "
							 "the screens are blacked." ),
						 QStringLiteral(":/screenlock/system-lock-screen.png") ),
	m_lockInputDevicesFeature( QStringLiteral( "InputDevicesLock" ),
							   Feature::Mode | Feature::AllComponents | Feature::Meta,
							   Feature::Uid( "e4a77879-e544-4fec-bc18-e534f33b934c" ),
							   {},
							   tr( "Lock input devices" ), tr( "Unlock input devices" ),
							   tr( "To reclaim all user's full attention you can lock "
								   "their computers using this button. "
								   "In this mode all input devices are locked while the desktop is still visible." ),
							   QStringLiteral(":/screenlock/system-lock-screen.png") ),
	m_features( { m_screenLockFeature, m_lockInputDevicesFeature } ),
	m_lockWidget( nullptr )
{
}



ScreenLockFeaturePlugin::~ScreenLockFeaturePlugin()
{
	delete m_lockWidget;
}



bool ScreenLockFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( hasFeature( featureUid ) == false )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, StartLockCommand }, computerControlInterfaces );

		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, StopLockCommand }, computerControlInterfaces );

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_screenLockFeature.uid() ||
		message.featureUid() == m_lockInputDevicesFeature.uid() )
	{
		if( server.featureWorkerManager().isWorkerRunning( message.featureUid() ) == false &&
			message.command() == StopLockCommand )
		{
			return true;
		}

		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false )
		{
			vDebug() << "not locking screen since not running in a user session";
			return true;
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToManagedSystemWorker( message );

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);

	if( message.featureUid() == m_screenLockFeature.uid() ||
		message.featureUid() == m_lockInputDevicesFeature.uid() )
	{
		switch( message.command() )
		{
		case StartLockCommand:
			if( m_lockWidget == nullptr )
			{
				VeyonCore::platform().coreFunctions().disableScreenSaver();

				auto mode = LockWidget::BackgroundPixmap;
				if( message.featureUid() == m_lockInputDevicesFeature.uid() )
				{
					mode = LockWidget::DesktopVisible;
				}

				m_lockWidget = new LockWidget( mode,
											   QPixmap( QStringLiteral(":/screenlock/locked-screen-background.png" ) ) );
			}
			return true;

		case StopLockCommand:
			delete m_lockWidget;
			m_lockWidget = nullptr;

			VeyonCore::platform().coreFunctions().restoreScreenSaverSettings();

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}

	return false;
}
