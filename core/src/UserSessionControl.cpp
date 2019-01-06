/*
 * UserSessionControl.cpp - implementation of UserSessionControl class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include "UserSessionControl.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "PlatformUserFunctions.h"


UserSessionControl::UserSessionControl( QObject* parent ) :
	QObject( parent ),
	m_userSessionInfoFeature( Feature( Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									   Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									   Feature::Uid(),
									   tr( "User session control" ), QString(), QString() ) ),
	m_userLogoutFeature( Feature::Action | Feature::Master | Feature::Service,
						 Feature::Uid( "7311d43d-ab53-439e-a03a-8cb25f7ed526" ),
						 Feature::Uid(),
						 tr( "Logout" ), QString(),
						 tr( "Click this button to logout users from all computers." ),
						 QStringLiteral( ":/resources/system-suspend-hibernate.png" ) ),
	m_features( { m_userSessionInfoFeature, m_userLogoutFeature } ),
	m_userInfoQueryThread( new QThread ),
	m_userInfoQueryTimer( new QTimer )
{
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



bool UserSessionControl::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);

	if( confirmFeatureExecution( feature, master.mainWindow() ) == false )
	{
		return false;
	}

	if( feature == m_userLogoutFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_userLogoutFeature.uid(), FeatureMessage::DefaultCommand ),
								   computerControlInterfaces );
	}

	return false;
}



bool UserSessionControl::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
											   ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master);

	if( message.featureUid() == m_userSessionInfoFeature.uid() )
	{
		computerControlInterface->setUser( message.argument( UserName ).toString() );

		return true;
	}

	return false;
}



bool UserSessionControl::handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message )
{
	if( m_userSessionInfoFeature.uid() == message.featureUid() )
	{
		FeatureMessage reply( message.featureUid(), message.command() );

		m_userDataLock.lockForRead();
		if( m_userName.isEmpty() )
		{
			queryUserInformation();
			reply.addArgument( UserName, QString() );
		}
		else
		{
			reply.addArgument( UserName, QString( QStringLiteral( "%1 (%2)" ) ).arg( m_userName, m_userFullName ) );
		}
		m_userDataLock.unlock();

		return server.sendFeatureMessageReply( message, reply );
	}
	else if( m_userLogoutFeature.uid() == message.featureUid() )
	{
		VeyonCore::platform().userFunctions().logout();
		return true;
	}

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
		const auto userName = VeyonCore::platform().userFunctions().currentUser();
		const auto userFullName = VeyonCore::platform().userFunctions().fullName( userName );
		m_userDataLock.lockForWrite();
		m_userName = userName;
		m_userFullName = userFullName;
		m_userDataLock.unlock();
	} );
}



bool UserSessionControl::confirmFeatureExecution( const Feature& feature, QWidget* parent )
{
	if( VeyonCore::config().confirmDangerousActions() == false )
	{
		return true;
	}

	if( feature == m_userLogoutFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm user logout" ),
									  tr( "Do you really want to logout the selected users?" ) ) ==
				QMessageBox::Yes;
	}

	return false;
}
