/*
 * UserSessionControl.cpp - implementation of UserSessionControl class
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

#include "UserSessionControl.h"
#include "FeatureWorkerManager.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "ItalcRfbExt.h"


UserSessionControl::UserSessionControl() :
	m_userSessionInfoFeature( Feature( Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									   Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									   QString(), QString(), QString() ) ),
	m_features()
{
	m_features += m_userSessionInfoFeature;
}



bool UserSessionControl::getUserSessionInfo( const ComputerControlInterfaceList& computerControlInterfaces )
{
	return sendFeatureMessage( FeatureMessage( m_userSessionInfoFeature.uid(), GetInfo ),
							   computerControlInterfaces );
}



bool UserSessionControl::startMasterFeature( const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces,
											 ComputerControlInterface& localComputerControlInterface,
											 QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool UserSessionControl::handleMasterFeatureMessage( const FeatureMessage& message,
													 ComputerControlInterface& computerControlInterface )
{
	if( message.featureUid() == m_userSessionInfoFeature.uid() )
	{
		computerControlInterface.setUser( message.argument( UserName ).toString() );

		return true;
	}

	return false;
}



bool UserSessionControl::stopMasterFeature( const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces,
											ComputerControlInterface& localComputerControlInterface,
											QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool UserSessionControl::handleServiceFeatureMessage( const FeatureMessage& message,
													  FeatureWorkerManager& featureWorkerManager )
{
	Q_UNUSED(featureWorkerManager);

	if( m_userSessionInfoFeature.uid() == message.featureUid() )
	{
		LocalSystem::User user = LocalSystem::User::loggedOnUser();

		FeatureMessage reply( message.featureUid(), message.command() );
		reply.addArgument( UserName, QString( "%1 (%2)" ).arg( user.name() ).arg( user.fullName() ) );
		reply.addArgument( HomeDir, user.homePath() );

		uint8_t rfbMessageType = rfbItalcFeatureMessage;
		message.ioDevice()->write( (const char *) &rfbMessageType, sizeof(rfbMessageType) );

		reply.send( message.ioDevice() );

		return true;
	}

	return false;
}



bool UserSessionControl::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}
