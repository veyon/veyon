/*
 * ServerAccessControlManager.cpp - implementation of ServerAccessControlManager
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

	case RfbVeyonAuth::Token:
		client->setAccessControlState( VncServerClient::AccessControlState::Successful );
		break;

	default:
		// reject unknown auth type
		client->setAccessControlState( VncServerClient::AccessControlState::Failed );
		client->setAccessControlDetails(tr("Requested authentication method not available"));
		break;
	}

	if( client->accessControlState() == VncServerClient::AccessControlState::Successful )
	{
		m_clients.append( client );
	}
}



void ServerAccessControlManager::removeClient( VncServerClient* client )
{
	m_clients.removeAll( client );

	// force all remaining clients to pass access control again as conditions might
	// have changed (e.g. AccessControlRule::Condition::AccessFromAlreadyConnectedUser)

	const VncServerClientList previousClients = m_clients;
	m_clients.clear();

	for( auto prevClient : std::as_const( previousClients ) )
	{
		prevClient->setAccessControlState( VncServerClient::AccessControlState::Init );
		addClient( prevClient );

		if( prevClient->accessControlState() != VncServerClient::AccessControlState::Successful &&
				prevClient->accessControlState() != VncServerClient::AccessControlState::Pending )
		{
			vDebug() << "closing connection as client does not pass access control any longer";
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
	case VncServerClient::AccessControlState::Init:
		client->accessControlTimer().restart();
		break;
	case VncServerClient::AccessControlState::Waiting:
		if( client->accessControlTimer().elapsed() < ClientWaitInterval )
		{
			return;
		}
		client->accessControlTimer().restart();
		break;
	default:
		break;
	}

	AccessControlProvider accessControlProvider;
	const auto checkResult = accessControlProvider.checkAccess(client->username(),
															   client->hostAddress(),
															   connectedUsers());

	switch (checkResult.access)
	{
	case AccessControlProvider::Access::Allow:
		client->setAccessControlState( VncServerClient::AccessControlState::Successful );
		if (checkResult.matchedRule)
		{
			client->setAccessControlDetails(tr("Access allowed by rule \"%1\"").arg(checkResult.matchedRule->name()));
		}
		break;

	case AccessControlProvider::Access::ToBeConfirmed:
		client->setAccessControlState( confirmDesktopAccess( client ) );
		break;

	default:
		client->setAccessControlState( VncServerClient::AccessControlState::Failed );
		client->setProtocolState( VncServerProtocol::Close );
		if (checkResult.matchedRule)
		{
			client->setAccessControlDetails(tr("Access denied by rule \"%1\"").arg(checkResult.matchedRule->name()));
		}
		break;
	}

	Q_EMIT finished( client );
}



VncServerClient::AccessControlState ServerAccessControlManager::confirmDesktopAccess( VncServerClient* client )
{
	const HostUserPair hostUserPair( client->username(), client->hostAddress() );

	// did we save a previous choice because user chose "always" or "never"?
	if( m_desktopAccessChoices.contains( hostUserPair ) )
	{
		if( std::as_const(m_desktopAccessChoices)[hostUserPair] == DesktopAccessDialog::ChoiceAlways )
		{
			return VncServerClient::AccessControlState::Successful;
		}

		return VncServerClient::AccessControlState::Failed;
	}

	// already an access dialog running?
	if( m_desktopAccessDialog.isBusy( &m_featureWorkerManager ) )
	{
		// then close connection so that client has to try again later
		return VncServerClient::AccessControlState::Waiting;
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

	return VncServerClient::AccessControlState::Pending;
}



void ServerAccessControlManager::finishDesktopAccessConfirmation( VncServerClient* client )
{
	// break helper connections for asynchronous desktop access control operations
	if( m_desktopAccessDialog.disconnect( client ) == false ||
			client->disconnect( this ) == false )
	{
		vCritical() << "could not break object connections";
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
		client->setAccessControlState( VncServerClient::AccessControlState::Successful );
		client->setAccessControlDetails(tr("User confirmed access."));
		m_clients.append( client );
	}
	else
	{
		client->setAccessControlState( VncServerClient::AccessControlState::Failed );
		client->setProtocolState( VncServerProtocol::Close );
		client->setAccessControlDetails(tr("User did not confirm access."));
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
