/*
 * UserSessionControl.cpp - implementation of UserSessionControl class
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

#include <QThread>
#include <QTimer>

#include "UserSessionControl.h"
#include "FeatureWorkerManager.h"
#include "LocalSystem.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "VeyonRfbExt.h"


UserSessionControl::UserSessionControl( QObject* parent ) :
	QObject( parent ),
	m_userSessionInfoFeature( Feature( Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									   Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									   tr( "User session control" ), QString(), QString() ) ),
	m_userLogoutFeature( Feature::Action | Feature::Master | Feature::Service,
						 Feature::Uid( "7311d43d-ab53-439e-a03a-8cb25f7ed526" ),
						 tr( "Logout user" ), QString(),
						 tr( "Click this button to logout users from all computers." ),
						 QStringLiteral( ":/resources/system-suspend-hibernate.png" ) ),
	m_features(),
	m_userInfoQueryThread( new QThread( this ) ),
	m_userInfoQueryTimer( new QTimer )
{
	m_features += m_userSessionInfoFeature;
	m_features += m_userLogoutFeature;

	// initialize user info query timer and thread
	m_userInfoQueryTimer->moveToThread( m_userInfoQueryThread );
	connect( m_userInfoQueryThread, &QThread::finished, m_userInfoQueryThread, &QObject::deleteLater );
}



UserSessionControl::~UserSessionControl()
{
	m_userInfoQueryThread->quit();
	m_userInfoQueryTimer->deleteLater();
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
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(parent);

	if( feature == m_userLogoutFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_userLogoutFeature.uid(), FeatureMessage::DefaultCommand ),
								   computerControlInterfaces );
	}

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
		FeatureMessage reply( message.featureUid(), message.command() );

		m_userDataLock.lockForRead();
		if( m_userName.isEmpty() )
		{
			queryUserInformation();
			reply.addArgument( UserName, QString() );
			reply.addArgument( HomeDir, QString() );
		}
		else
		{
			reply.addArgument( UserName, QString( QStringLiteral( "%1 (%2)" ) ).arg( m_userName, m_userFullName ) );
			reply.addArgument( HomeDir, m_userHomePath );
		}
		m_userDataLock.unlock();

		char rfbMessageType = rfbVeyonFeatureMessage;
		message.ioDevice()->write( &rfbMessageType, sizeof(rfbMessageType) );
		reply.send( message.ioDevice() );

		return true;
	}
	else if( m_userLogoutFeature.uid() == message.featureUid() )
	{
		LocalSystem::logoutUser();
	}

	return false;
}



bool UserSessionControl::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}



void UserSessionControl::queryUserInformation()
{
	if( m_userInfoQueryThread->isRunning() == false )
	{
		m_userInfoQueryThread->start();
	}

	// asynchronously query information about logged on user (which might block
	// due to domain controller queries and timeouts etc.)
	m_userInfoQueryTimer->singleShot( 0, m_userInfoQueryTimer, [=]() {
		auto user = LocalSystem::User::loggedOnUser();
		const auto name = user.name();
		const auto fullName = user.fullName();
		const auto homePath = user.homePath();
		m_userDataLock.lockForWrite();
		m_userName = name;
		m_userFullName = fullName;
		m_userHomePath = homePath;
		m_userDataLock.unlock();
	} );
}
