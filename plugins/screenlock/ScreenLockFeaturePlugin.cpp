/*
 * ScreenLockFeaturePlugin.cpp - implementation of ScreenLockFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QCoreApplication>

#include "ScreenLockFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "LockWidget.h"


ScreenLockFeaturePlugin::ScreenLockFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_screenLockFeature( Feature::Mode | Feature::AllComponents,
						 Feature::Uid( "ccb535a2-1d24-4cc1-a709-8b47d2b2ac79" ),
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



bool ScreenLockFeaturePlugin::startMasterFeature( const Feature& feature,
												  const ComputerControlInterfaceList& computerControlInterfaces,
												  ComputerControlInterface& localComputerControlInterface,
												  QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(parent);

	if( feature == m_screenLockFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_screenLockFeature.uid(), StartLockCommand ),
								   computerControlInterfaces );
	}

	return false;
}



bool ScreenLockFeaturePlugin::stopMasterFeature( const Feature& feature,
												 const ComputerControlInterfaceList& computerControlInterfaces,
												 ComputerControlInterface& localComputerControlInterface,
												 QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(parent);

	if( feature == m_screenLockFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_screenLockFeature.uid(), StopLockCommand ),
								   computerControlInterfaces );
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
														  ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool ScreenLockFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
														   FeatureWorkerManager& featureWorkerManager )
{
	if( m_screenLockFeature.uid() == message.featureUid() )
	{
		if( featureWorkerManager.isWorkerRunning( m_screenLockFeature ) == false &&
				message.command() != StopLockCommand )
		{
			featureWorkerManager.startWorker( m_screenLockFeature );
		}

		// forward message to worker
		featureWorkerManager.sendMessage( message );

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	if( m_screenLockFeature.uid() == message.featureUid() )
	{
		switch( message.command() )
		{
		case StartLockCommand:
			if( m_lockWidget == nullptr )
			{
				m_lockWidget = new LockWidget;
			}
			return true;

		case StopLockCommand:
			delete m_lockWidget;
			m_lockWidget = nullptr;

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}

	return false;
}
