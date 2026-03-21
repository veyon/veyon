/*
 * VncServerProtocol.cpp - implementation of the VncServerProtocol class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "rfb/rfbproto.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpSocket>

#include "AccessControlProvider.h"
#include "BuiltinFeatures.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"



VncServerProtocol::VncServerProtocol(QTcpSocket* socket, VncServerClient* client) :
	m_socket( socket ),
	m_client( client ),
	m_serverInitMessage()
{
	m_client->setHostAddress(m_socket->peerAddress().toString());
	m_client->setAccessControlState( VncServerClient::AccessControlState::Init );
}



VncServerProtocol::State VncServerProtocol::state() const
{
	return m_client->protocolState();
}



void VncServerProtocol::start()
{
	if( state() == State::Disconnected )
	{
		rfbProtocolVersionMsg protocol{}; // Initialize to zero

		snprintf( protocol, sizeof(protocol), rfbProtocolVersionFormat, 3, 8 );

		m_socket->write( protocol, sz_rfbProtocolVersionMsg );

		setState( State::Protocol );
	}
}



bool VncServerProtocol::read()
{
	switch( state() )
	{
	case State::Protocol:
		return readProtocol();

	case State::SecurityInit:
		return receiveSecurityTypeResponse();

	case State::AuthenticationTypes:
		return receiveAuthenticationTypeResponse();

	case State::Authenticating:
		return receiveAuthenticationMessage();

	case State::AccessControl:
		return processAccessControl();

	case State::FramebufferInit:
		return processFramebufferInit();

	case State::Close:
		vDebug() << "closing connection per protocol state";
		m_socket->close();
		break;

	case State::Closing:
		if (m_socket->isOpen())
		{
			m_socket->readAll();
		}
		else
		{
			setState(State::Close);
		}
		return false;

	default:
		break;
	}

	return false;
}



void VncServerProtocol::setState( VncServerProtocol::State state )
{
	m_client->setProtocolState( state );
}



bool VncServerProtocol::readProtocol()
{
	if( m_socket->bytesAvailable() == sz_rfbProtocolVersionMsg )
	{
		const auto protocol = m_socket->read( sz_rfbProtocolVersionMsg );

		if( protocol.size() != sz_rfbProtocolVersionMsg )
		{
			vCritical() << "protocol initialization failed";
			m_socket->close();
			return false;
		}

		static const QRegularExpression rfbRX{QStringLiteral("RFB (\\d\\d\\d)\\.(\\d\\d\\d)\n")};
		if (rfbRX.match(QString::fromUtf8(protocol)).hasMatch() == false)
		{
			vCritical() << "invalid protocol version";
			m_socket->close();
			return false;
		}

		setState( State::SecurityInit );

		return sendSecurityTypes();
	}

	return false;
}



bool VncServerProtocol::sendSecurityTypes()
{
	// send list of supported security types
	const char securityTypeList[2] = { 1, rfbSecTypeVeyon }; // Flawfinder: ignore
	m_socket->write( securityTypeList, sizeof( securityTypeList ) );

	return true;
}



bool VncServerProtocol::receiveSecurityTypeResponse()
{
	if( m_socket->bytesAvailable() >= 1 )
	{
		char chosenSecurityType = 0;

		if( m_socket->read( &chosenSecurityType, sizeof(chosenSecurityType) ) != sizeof(chosenSecurityType) ||
				chosenSecurityType != rfbSecTypeVeyon )
		{
			vCritical() << "protocol initialization failed";
			m_socket->close();

			return false;
		}

		setState(State::AuthenticationTypes);

		return sendAuthenticationTypes();
	}

	return false;
}



bool VncServerProtocol::sendAuthenticationTypes()
{
	const auto authTypes = supportedAuthTypes();

	VariantArrayMessage message( m_socket );
	message.write(int(authTypes.count()));

	for( auto authType : authTypes )
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
		const auto chosenAuthType = message.read().value<RfbVeyonAuth::Type>();

		if( supportedAuthTypes().contains( chosenAuthType ) == false )
		{
			vCritical() << "unsupported authentication type chosen by client!";
			m_socket->close();

			return false;
		}

		const auto username = message.read().toString();

		// Validate username length to prevent DoS attacks
		constexpr int MaxUsernameLength = 256;
		if( username.length() > MaxUsernameLength )
		{
			vCritical() << "username exceeds maximum length of" << MaxUsernameLength;
			m_socket->close();
			return false;
		}

		m_client->setAuthType( chosenAuthType );
		m_client->setUsername( username );

		setState( State::Authenticating );

		// send auth ack message
		VariantArrayMessage( m_socket ).send();

		// init authentication
		VariantArrayMessage dummyMessage( m_socket );
		processAuthentication( dummyMessage );
	}

	return false;
}



bool VncServerProtocol::receiveAuthenticationMessage()
{
	VariantArrayMessage message( m_socket );

	if( message.isReadyForReceive() && message.receive() )
	{
		return processAuthentication( message );
	}

	return false;
}



bool VncServerProtocol::processAuthentication( VariantArrayMessage& message )
{
	processAuthenticationMessage( message );

	switch( m_client->authState() )
	{
	case VncServerClient::AuthState::Successful:
	{
		const auto authResult = qToBigEndian<uint32_t>(rfbVncAuthOK);
		m_socket->write( reinterpret_cast<const char *>( &authResult ), sizeof(authResult) );

		setState( State::AccessControl );
		return true;
	}

	case VncServerClient::AuthState::Failed:
		vCritical() << "authentication failed - closing connection";
		m_socket->close();

		return false;

	default:
		break;
	}

	return false;
}



bool VncServerProtocol::processAccessControl()
{
	performAccessControl();

	switch( m_client->accessControlState() )
	{
	case VncServerClient::AccessControlState::Successful:
		setState( State::FramebufferInit );
		return true;

	case VncServerClient::AccessControlState::Pending:
	case VncServerClient::AccessControlState::Waiting:
		break;

	default:
		vCritical() << "access control failed - closing connection";
		m_socket->read(sz_rfbClientInitMsg);
		sendEmptyServerInitMessage();
		sendFailedAccessControlDetails();
		return true;
	}

	return false;
}



void VncServerProtocol::sendFailedAccessControlDetails()
{
	VeyonCore::builtinFeatures().accessControlProvider().sendDetails(m_socket, client()->accessControlDetails());

	QObject::connect (&m_accessControlDetailsSendTimer, &QTimer::timeout, m_socket, [this]() {
		VeyonCore::builtinFeatures().accessControlProvider().sendDetails(m_socket, client()->accessControlDetails());
	});
	QTimer::singleShot(AccessControlCloseDelay, m_socket, &QAbstractSocket::close);
	m_accessControlDetailsSendTimer.start(AccessControlDetailsSendInterval);

	setState(State::Closing);
}



void VncServerProtocol::sendEmptyServerInitMessage()
{
	rfbServerInitMsg message{};
	m_socket->write(reinterpret_cast<const char *>(&message), sizeof(message));
}



bool VncServerProtocol::processFramebufferInit()
{
	if( m_socket->bytesAvailable() >= sz_rfbClientInitMsg &&
			m_serverInitMessage.isEmpty() == false )
	{
		// just read client init message without further evaluation
		m_socket->read( sz_rfbClientInitMsg );

		m_socket->write( m_serverInitMessage );

		setState( State::Running );

		return true;
	}

	return false;
}
