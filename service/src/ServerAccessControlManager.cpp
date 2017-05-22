/*
 * ServerAccessControlManager.cpp - implementation of ServerAccessControlManager
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

#include "VeyonCore.h"

#include "ServerAccessControlManager.h"
#include "AccessControlProvider.h"
#include "DesktopAccessDialog.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "VariantArrayMessage.h"


ServerAccessControlManager::ServerAccessControlManager( FeatureWorkerManager& featureWorkerManager,
														DesktopAccessDialog& desktopAccessDialog,
														QObject* parent ) :
	QObject( parent ),
	m_featureWorkerManager( featureWorkerManager ),
	m_desktopAccessDialog( desktopAccessDialog ),
	m_clients(),
	m_desktopAccessChoices()
{
}



bool ServerAccessControlManager::addClient( VncServerClient* client )
{
	bool result = false;

	switch( client->authType() )
	{
	case RfbVeyonAuth::DSA:
		result = performAccessControl( client );
		break;

	case RfbVeyonAuth::Logon:
		result = performAccessControl( client );
		break;

	case RfbVeyonAuth::None:
	case RfbVeyonAuth::HostWhiteList:
	case RfbVeyonAuth::Token:
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
			client->setProtocolState( VncServerProtocol::Close );
		}
	}
}



bool ServerAccessControlManager::performAccessControl( VncServerClient* client )
{
	const auto accessResult =
			AccessControlProvider().checkAccess( client->username(),
												 client->hostAddress(),
												 connectedUsers() );

	switch( accessResult )
	{
	case AccessControlProvider::AccessAllow:
		return true;
	case AccessControlProvider::AccessToBeConfirmed:
		return confirmDesktopAccess( client );
	default:
		break;
	}

	return false;
}



bool ServerAccessControlManager::confirmDesktopAccess(VncServerClient *client)
{
	const HostUserPair hostUserPair( client->username(), client->hostAddress() );

	// did we save a previous choice because user chose "always" or "never"?
	if( m_desktopAccessChoices.contains( hostUserPair ) )
	{
		return m_desktopAccessChoices[hostUserPair] == DesktopAccessDialog::ChoiceAlways;
	}

	const auto result = m_desktopAccessDialog.exec( m_featureWorkerManager,
													client->username(), client->hostAddress() );

	// remember choices "always" and "never"
	if( result == DesktopAccessDialog::ChoiceAlways ||
			result == DesktopAccessDialog::ChoiceNever )
	{
		m_desktopAccessChoices[hostUserPair] = result;
	}

	return result == DesktopAccessDialog::ChoiceYes ||
			result == DesktopAccessDialog::ChoiceAlways;
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
