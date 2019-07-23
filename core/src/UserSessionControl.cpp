/*
 * UserSessionControl.cpp - implementation of UserSessionControl class
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

#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>

#include "UserSessionControl.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "PlatformUserFunctions.h"


UserSessionControl::UserSessionControl( QObject* parent ) :
	QObject( parent ),
	m_userSessionInfoFeature( Feature( QStringLiteral( "UserSessionInfo" ),
									   Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									   Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									   Feature::Uid(),
									   tr( "User session control" ), {}, {} ) ),
	m_userLogoffFeature( QStringLiteral( "UserLogoff" ),
						 Feature::Action | Feature::Master | Feature::Service,
						 Feature::Uid( "7311d43d-ab53-439e-a03a-8cb25f7ed526" ),
						 Feature::Uid(),
						 tr( "Log off" ), {},
						 tr( "Click this button to log off users from all computers." ),
						 QStringLiteral( ":/core/system-suspend-hibernate.png" ) ),
	m_features( { m_userSessionInfoFeature, m_userLogoffFeature } )
{
}



bool UserSessionControl::getUserSessionInfo( const ComputerControlInterfaceList& computerControlInterfaces )
{
	return sendFeatureMessage( FeatureMessage( m_userSessionInfoFeature.uid(), GetInfo ),
							   computerControlInterfaces, false );
}



bool UserSessionControl::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);

	if( confirmFeatureExecution( feature, master.mainWindow() ) == false )
	{
		return false;
	}

	if( feature == m_userLogoffFeature )
	{
		return sendFeatureMessage( FeatureMessage( m_userLogoffFeature.uid(), FeatureMessage::DefaultCommand ),
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
		computerControlInterface->setUserLoginName( message.argument( UserLoginName ).toString() );
		computerControlInterface->setUserFullName( message.argument( UserFullName ).toString() );

		return true;
	}

	return false;
}



bool UserSessionControl::handleFeatureMessage( VeyonServerInterface& server,
											   const MessageContext& messageContext,
											   const FeatureMessage& message )
{
	if( m_userSessionInfoFeature.uid() == message.featureUid() )
	{
		FeatureMessage reply( message.featureUid(), message.command() );

		m_userDataLock.lockForRead();
		if( m_userLoginName.isEmpty() )
		{
			queryUserInformation();
			reply.addArgument( UserLoginName, QString() );
			reply.addArgument( UserFullName, QString() );
		}
		else
		{
			reply.addArgument( UserLoginName, m_userLoginName );
			reply.addArgument( UserFullName, m_userFullName );
		}
		m_userDataLock.unlock();

		return server.sendFeatureMessageReply( messageContext, reply );
	}
	else if( m_userLogoffFeature.uid() == message.featureUid() )
	{
		VeyonCore::platform().userFunctions().logoff();
		return true;
	}

	return false;
}



void UserSessionControl::queryUserInformation()
{
	// asynchronously query information about logged on user (which might block
	// due to domain controller queries and timeouts etc.)
	QtConcurrent::run( [=]() {
		const auto userLoginName = VeyonCore::platform().userFunctions().currentUser();
		const auto userFullName = VeyonCore::platform().userFunctions().fullName( userLoginName );
		m_userDataLock.lockForWrite();
		m_userLoginName = userLoginName;
		m_userFullName = userFullName;
		m_userDataLock.unlock();
	} );
}



bool UserSessionControl::confirmFeatureExecution( const Feature& feature, QWidget* parent )
{
	if( VeyonCore::config().confirmUnsafeActions() == false )
	{
		return true;
	}

	if( feature == m_userLogoffFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm user logoff" ),
									  tr( "Do you really want to log off the selected users?" ) ) ==
				QMessageBox::Yes;
	}

	return false;
}
