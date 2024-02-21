/*
 * DemoServerProtocol.cpp - implementation of DemoServerProtocol class
 *
 * Copyright (c) 2017-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QTcpSocket>

#include "DemoAuthentication.h"
#include "DemoServerProtocol.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"


DemoServerProtocol::DemoServerProtocol( const DemoAuthentication& authentication, QTcpSocket* socket, VncServerClient* client ) :
	VncServerProtocol( socket, client ),
	m_authentication( authentication )
{
}



DemoServerProtocol::AuthMethodUids DemoServerProtocol::supportedAuthMethodUids() const
{
	return { m_authentication.pluginUid() };
}



void DemoServerProtocol::processAuthenticationMessage( VariantArrayMessage& message )
{
	if( client()->authMethodUid() == m_authentication.pluginUid() )
	{
		client()->setAuthState( performTokenAuthentication( message ) );
	}
	else
	{
		client()->setAuthState( VncServerClient::AuthState::Failed );
	}
}



void DemoServerProtocol::performAccessControl()
{
	client()->setAccessControlState( VncServerClient::AccessControlState::Successful );
}



VncServerClient::AuthState DemoServerProtocol::performTokenAuthentication( VariantArrayMessage& message )
{
	switch( client()->authState() )
	{
	case VncServerClient::AuthState::Init:
		return VncServerClient::AuthState::Stage1;

	case VncServerClient::AuthState::Stage1:
		if( message.read().toByteArray() == m_authentication.accessToken().toByteArray() )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}
