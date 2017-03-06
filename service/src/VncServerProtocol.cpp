/*
 * VncServerProtocol.cpp - implementation of the VncServerProtocol class
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

#ifdef ITALC_BUILD_WIN32
#include <winsock2.h>
#endif

#include <QHostAddress>
#include <QTcpSocket>

#include "rfb/rfbproto.h"

#include "AuthenticationCredentials.h"
#include "ServerAuthenticationManager.h"
#include "VariantArrayMessage.h"
#include "VncServerProtocol.h"


VncServerProtocol::VncServerProtocol( QTcpSocket* socket, ServerAuthenticationManager& serverAuthenticationManager ) :
	m_socket( socket ),
	m_serverAuthenticationManager( serverAuthenticationManager ),
	m_state( Disconnected ),
	m_authClient( nullptr )
{
}



VncServerProtocol::~VncServerProtocol()
{
	delete m_authClient;
}



void VncServerProtocol::start()
{
	if( m_state == Disconnected )
	{
		char protocol[sz_rfbProtocolVersionMsg+1];

		sprintf( protocol, rfbProtocolVersionFormat, 3, 8 );

		m_socket->write( protocol, sz_rfbProtocolVersionMsg );

		m_state = Protocol;
	}
}



bool VncServerProtocol::read()
{
	switch( m_state )
	{
	case Protocol:
		return readProtocol();

	case SecurityInit:
		return receiveSecurityTypeResponse();

	case AuthenticationTypes:
		return receiveAuthenticationTypeResponse();

	case Authenticating:
		return receiveAuthenticationMessage();

	default:
		break;
	}

	return false;
}



bool VncServerProtocol::readProtocol()
{
	if( m_socket->bytesAvailable() == sz_rfbProtocolVersionMsg )
	{
		char protocol[sz_rfbProtocolVersionMsg+1];
		m_socket->read( protocol, sz_rfbProtocolVersionMsg );
		protocol[sz_rfbProtocolVersionMsg] = 0;

		int protocolMajor = 0, protocolMinor = 0;

		if( sscanf( protocol, rfbProtocolVersionFormat, &protocolMajor, &protocolMinor ) != 2 )
		{
			qCritical( "VncServerProtocol:::readProtocol(): protocol initialization failed" );
			m_socket->close();
			return false;
		}

		m_state = SecurityInit;

		return sendSecurityTypes();
	}
	else
	{
		qDebug( "VncServerProtocol::readProtocol(): not enough data available (%d)", (int) m_socket->bytesAvailable() );
	}

	return false;
}



bool VncServerProtocol::sendSecurityTypes()
{
	// send list of supported security types
	const char securityTypeList[2] = { 1, rfbSecTypeItalc };
	m_socket->write( securityTypeList, sizeof( securityTypeList ) );

	return true;
}



bool VncServerProtocol::receiveSecurityTypeResponse()
{
	if( m_socket->bytesAvailable() >= 1 )
	{
		char chosenSecurityType = 0;

		if( m_socket->read( &chosenSecurityType, sizeof(chosenSecurityType) ) != sizeof(chosenSecurityType) ||
				chosenSecurityType != rfbSecTypeItalc )
		{
			qCritical( "VncServerProtocol:::receiveSecurityTypeResponse(): protocol initialization failed" );
			m_socket->close();

			return false;
		}

		m_state = AuthenticationTypes;

		return sendAuthenticationTypes();
	}
	else
	{
		qDebug( "VncServerProtocol::receiveSecurityTypeResponse(): not enough data available (%d)", (int) m_socket->bytesAvailable() );
	}

	return false;
}



bool VncServerProtocol::sendAuthenticationTypes()
{
	VariantArrayMessage message( m_socket );
	message.write( m_serverAuthenticationManager.supportedAuthTypes().count() );

	for( auto authType : m_serverAuthenticationManager.supportedAuthTypes() )
	{
		message.write( authType );
	}

	return message.send();
}



bool VncServerProtocol::receiveAuthenticationTypeResponse()
{
	VariantArrayMessage message( m_socket );

	if( message.isReadyForReceive() && message.receive() )
	{
		auto chosenAuthType = message.read().value<RfbItalcAuth::Type>();

		if( m_serverAuthenticationManager.supportedAuthTypes().contains( chosenAuthType ) == false )
		{
			qCritical( "VncServerProtocol:::receiveAuthenticationTypeResponse(): unsupported authentication type chosen by client!" );
			m_socket->close();

			return false;
		}

		if( chosenAuthType == RfbItalcAuth::None )
		{
			qWarning( "VncServerProtocol::receiveAuthenticationTypeResponse(): allowing access without authentication." );
			m_state = Authenticated;
			return true;
		}

		const QString username = message.read().toString();

		m_authClient = new ServerAuthenticationManager::Client( chosenAuthType, username, m_socket->peerAddress().toString() );

		m_state = Authenticating;

		// init authentication
		VariantArrayMessage dummyMessage( m_socket );
		processAuthentication( dummyMessage );

	}

	return false;
}



bool VncServerProtocol::receiveAuthenticationMessage()
{
	if( m_authClient == nullptr )
	{
		qCritical( "VncServerProtocol:::receiveAuthenticationMessage(): auth client is NULL!" );

		m_socket->close();
		return false;
	}

	VariantArrayMessage message( m_socket );

	if( message.isReadyForReceive() && message.receive() )
	{
		return processAuthentication( message );
	}

	return false;
}



bool VncServerProtocol::processAuthentication( VariantArrayMessage& message )
{
	m_serverAuthenticationManager.processAuthenticationMessage( m_authClient, message );

	switch( m_authClient->state() )
	{
	case ServerAuthenticationManager::Client::FinishedSuccess:
	{
		uint32_t authResult = htonl(rfbVncAuthOK);
		m_socket->write( (char *) &authResult, sizeof(authResult) );

		m_state = Authenticated;
		break;
	}

	case ServerAuthenticationManager::Client::FinishedFail:
		qCritical( "VncServerProtocol:::receiveAuthenticationMessage(): authentication failed - closing connection" );
		m_socket->close();

		return false;
		break;

	default:
		break;
	}

	return false;
}
