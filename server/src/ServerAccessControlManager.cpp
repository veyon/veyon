/*
 * ServerAccessControlManager.cpp - implementation of ServerAccessControlManager
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

#include "VeyonCore.h"

#include "ServerAccessControlManager.h"
#include "AccessControlProvider.h"
#include "DesktopAccessDialog.h"
#include "VeyonConfiguration.h"
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



void ServerAccessControlManager::addClient( VncServerClient* client )
{
	switch( client->authType() )
	{
	case RfbVeyonAuth::KeyFile:
	case RfbVeyonAuth::Logon:
		performAccessControl( client );
		break;

	case RfbVeyonAuth::None:
	case RfbVeyonAuth::HostWhiteList:
	case RfbVeyonAuth::Token:
		client->setAccessControlState( VncServerClient::AccessControlSuccessful );
		break;

	default:
		break;
	}

	if( client->accessControlState() == VncServerClient::AccessControlSuccessful )
	{
		m_clients.append( client );
	}
}



void ServerAccessControlManager::removeClient( VncServerClient* client )
{
	m_clients.removeAll( client );

	// force all remaining clients to pass access control again as conditions might
	// have changed (e.g. AccessControlRule::ConditionAccessFromAlreadyConnectedUser)

	const VncServerClientList previousClients = m_clients;
	m_clients.clear();

	for( auto prevClient : qAsConst( previousClients ) )
	{
		prevClient->setAccessControlState( VncServerClient::AccessControlInit );
		addClient( prevClient );

		if( prevClient->accessControlState() != VncServerClient::AccessControlSuccessful &&
				prevClient->accessControlState() != VncServerClient::AccessControlPending )
		{
			qDebug( "ServerAccessControlManager::removeClient(): closing connection as client does not pass access control any longer" );
			prevClient->setProtocolState( VncServerProtocol::Close );
		}
	}
}



void ServerAccessControlManager::performAccessControl( VncServerClient* client )
{
	// implement access control wait for connections other than the one an
	// access dialog is currently active for
	switch( client->accessControlState() )
	{
	case VncServerClient::AccessControlInit:
		client->accessControlTimer().restart();
		break;
	case VncServerClient::AccessControlWaiting:
		if( client->accessControlTimer().elapsed() < ClientWaitInterval )
		{
			return;
		}
		client->accessControlTimer().restart();
		break;
	default:
		break;
	}

	const auto accessResult =
			AccessControlProvider().checkAccess( client->username(),
												 client->hostAddress(),
												 connectedUsers() );

	switch( accessResult )
	{
	case AccessControlProvider::AccessAllow:
		client->setAccessControlState( VncServerClient::AccessControlSuccessful );
		break;

	case AccessControlProvider::AccessToBeConfirmed:
		client->setAccessControlState( confirmDesktopAccess( client ) );
		break;

	default:
		client->setAccessControlState( VncServerClient::AccessControlFailed );
		client->setProtocolState( VncServerProtocol::Close );
		break;
	}
}



VncServerClient::AccessControlState ServerAccessControlManager::confirmDesktopAccess( VncServerClient* client )
{
	const HostUserPair hostUserPair( client->username(), client->hostAddress() );

	// did we save a previous choice because user chose "always" or "never"?
	if( m_desktopAccessChoices.contains( hostUserPair ) )
	{
		if( qAsConst(m_desktopAccessChoices)[hostUserPair] == DesktopAccessDialog::ChoiceAlways )
		{
			return VncServerClient::AccessControlSuccessful;
		}

		return VncServerClient::AccessControlFailed;
	}

	// already an access dialog running?
	if( m_desktopAccessDialog.isBusy( &m_featureWorkerManager ) )
	{
		// then close connection so that client has to try again later
		return VncServerClient::AccessControlWaiting;
	}

	// get notified whenever the dialog finishes - use signal indirection for
	// automatically breaking connection if VncServerClient gets deleted while
	// dialog is active (e.g. due another connection being closed and thus
	// all other connections are closed as well in order to perform access
	// control again)
	connect( &m_desktopAccessDialog, &DesktopAccessDialog::finished,
			 client, &VncServerClient::finishAccessControl );

	connect( client, &VncServerClient::accessControlFinished,
			 this, &ServerAccessControlManager::finishDesktopAccessConfirmation );

	// start the dialog (non-blocking)
	m_desktopAccessDialog.exec( &m_featureWorkerManager, client->username(), client->hostAddress() );

	return VncServerClient::AccessControlPending;
}



void ServerAccessControlManager::finishDesktopAccessConfirmation( VncServerClient* client )
{
	// break helper connections for asynchronous desktop access control operations
	if( m_desktopAccessDialog.disconnect( client ) == false ||
			client->disconnect( this ) == false )
	{
		qCritical() << Q_FUNC_INFO << "could not break object connections";
	}

	const auto choice = m_desktopAccessDialog.choice();

	// remember choices "always" and "never"
	if( choice == DesktopAccessDialog::ChoiceAlways || choice == DesktopAccessDialog::ChoiceNever )
	{
		m_desktopAccessChoices[HostUserPair( client->username(), client->hostAddress() )] = choice;
	}

	// evaluate choice and set according access control state
	if( choice == DesktopAccessDialog::ChoiceYes || choice == DesktopAccessDialog::ChoiceAlways )
	{
		client->setAccessControlState( VncServerClient::AccessControlSuccessful );
		m_clients.append( client );
	}
	else
	{
		client->setAccessControlState( VncServerClient::AccessControlFailed );
		client->setProtocolState( VncServerProtocol::Close );
	}
}



QStringList ServerAccessControlManager::connectedUsers() const
{
	QStringList users;

	users.reserve( m_clients.size() );

	for( auto client : m_clients )
	{
		users += client->username();
	}

	return users;
}
