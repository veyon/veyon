/*
 * ScreenLockFeaturePlugin.cpp - implementation of ScreenLockFeaturePlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
	m_features( { m_screenLockFeature } ),
	m_lockWidget( nullptr )
{
}



ScreenLockFeaturePlugin::~ScreenLockFeaturePlugin()
{
	delete m_lockWidget;
}



bool ScreenLockFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);

	if( feature == m_screenLockFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_screenLockFeature.uid(), StartLockCommand ),
								   computerControlInterfaces );
	}

	return false;
}



bool ScreenLockFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);

	if( feature == m_screenLockFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_screenLockFeature.uid(), StopLockCommand ),
								   computerControlInterfaces );
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
													ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master);
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_screenLockFeature.uid() == message.featureUid() )
	{
		if( server.featureWorkerManager().isWorkerRunning( m_screenLockFeature ) == false &&
				message.command() != StopLockCommand )
		{
			server.featureWorkerManager().startWorker( m_screenLockFeature, FeatureWorkerManager::ManagedSystemProcess );
		}

		// forward message to worker
		server.featureWorkerManager().sendMessage( message );

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);

	if( m_screenLockFeature.uid() == message.featureUid() )
	{
		switch( message.command() )
		{
		case StartLockCommand:
			if( m_lockWidget == nullptr )
			{
				VeyonCore::platform().coreFunctions().disableScreenSaver();

				m_lockWidget = new LockWidget( LockWidget::BackgroundPixmap,
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
