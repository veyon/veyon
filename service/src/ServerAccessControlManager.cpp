/*
 * ServerAccessControlManager.cpp - implementation of ServerAccessControlManager
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

#include "ItalcCore.h"

#include <QtCrypto>
#include <QHostAddress>

#include "ServerAccessControlManager.h"
#include "AccessControlProvider.h"
#include "DesktopAccessPermission.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "VariantArrayMessage.h"


ServerAccessControlManager::ServerAccessControlManager( FeatureWorkerManager& featureWorkerManager,
														DesktopAccessDialog& desktopAccessDialog,
														QObject* parent ) :
	QObject( parent ),
	m_featureWorkerManager( featureWorkerManager ),
	m_desktopAccessDialog( desktopAccessDialog )
{
}



bool ServerAccessControlManager::addClient( VncServerClient* client )
{
	bool result = false;

	switch( client->authType() )
	{
	case RfbItalcAuth::DSA:
		result = performAccessControl( client, DesktopAccessPermission::KeyAuthentication );
		break;

	case RfbItalcAuth::Logon:
		result = performAccessControl( client, DesktopAccessPermission::LogonAuthentication );
		break;

	case RfbItalcAuth::None:
	case RfbItalcAuth::HostWhiteList:
	case RfbItalcAuth::Token:
		result = true;
		break;

	default:
		break;
	}

	if( result )
	{
		m_clients.append( client );
	}

	return result;
}



void ServerAccessControlManager::removeClient( VncServerClient* client )
{
	m_clients.removeAll( client );

	// force all remaining clients to pass access control again as conditions might
	// have changed (e.g. AccessControlRule::ConditionAccessFromAlreadyConnectedUser)

	VncServerClientList previousClients = m_clients;
	m_clients.clear();

	for( auto client : previousClients )
	{
		if( addClient( client ) == false )
		{
			qDebug( "ServerAccessControlManager::removeClient(): closing connection as client does not pass access control any longer" );
			client->setProtocolState( VncServerClient::Close );
		}
	}
}



bool ServerAccessControlManager::performAccessControl( VncServerClient* client,
													   DesktopAccessPermission::AuthenticationMethod authenticationMethod )
{
	auto accessResult = AccessControlProvider().checkAccess( client->username(),
															 client->hostAddress(),
															 connectedUsers() );

	DesktopAccessPermission desktopAccessPermission( authenticationMethod );

	switch( accessResult )
	{
	case AccessControlProvider::AccessToBeConfirmed:
		return desktopAccessPermission.ask( m_featureWorkerManager, m_desktopAccessDialog,
											client->username(), client->hostAddress() );

	case AccessControlProvider::AccessAllow:
		if( desktopAccessPermission.authenticationMethodRequiresConfirmation() )
		{
			return desktopAccessPermission.ask( m_featureWorkerManager, m_desktopAccessDialog,
												client->username(), client->hostAddress() );
		}
		return true;

	default:
		break;
	}

	return false;
}



QStringList ServerAccessControlManager::connectedUsers() const
{
	QStringList users;

	for( auto client : m_clients )
	{
		users += client->username();
	}

	return users;
}
