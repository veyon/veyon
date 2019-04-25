/*
 * DemoServerProtocol.cpp - implementation of DemoServerProtocol class
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

#include "DemoServerProtocol.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"


DemoServerProtocol::DemoServerProtocol( const QString& demoAccessToken, QTcpSocket* socket, VncServerClient* client ) :
	VncServerProtocol( socket, client ),
	m_demoAccessToken( demoAccessToken )
{
}



QVector<RfbVeyonAuth::Type> DemoServerProtocol::supportedAuthTypes() const
{
	return { RfbVeyonAuth::Token };
}



void DemoServerProtocol::processAuthenticationMessage( VariantArrayMessage& message )
{
	if( client()->authType() == RfbVeyonAuth::Token )
	{
		client()->setAuthState( performTokenAuthentication( message ) );
	}
	else
	{
		client()->setAuthState( VncServerClient::AuthFinishedFail );
	}
}



void DemoServerProtocol::performAccessControl()
{
	client()->setAccessControlState( VncServerClient::AccessControlSuccessful );
}



VncServerClient::AuthState DemoServerProtocol::performTokenAuthentication( VariantArrayMessage& message )
{
	switch( client()->authState() )
	{
	case VncServerClient::AuthInit:
		return VncServerClient::AuthToken;

	case VncServerClient::AuthToken:
		if( message.read().toString() == m_demoAccessToken )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthFinishedSuccess;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthFinishedFail;

	default:
		break;
	}

	return VncServerClient::AuthFinishedFail;
}
