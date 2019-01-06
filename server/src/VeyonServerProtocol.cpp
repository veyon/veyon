/*
 * VeyonServerProtocol.cpp - implementation of the VeyonServerProtocol class
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

#include "ServerAuthenticationManager.h"
#include "ServerAccessControlManager.h"
#include "VeyonServerProtocol.h"


VeyonServerProtocol::VeyonServerProtocol( QTcpSocket* socket,
										  VncServerClient* client,
										  ServerAuthenticationManager& serverAuthenticationManager,
										  ServerAccessControlManager& serverAccessControlManager ) :
	VncServerProtocol( socket, client ),
	m_serverAuthenticationManager( serverAuthenticationManager ),
	m_serverAccessControlManager( serverAccessControlManager )
{
}



QVector<RfbVeyonAuth::Type> VeyonServerProtocol::supportedAuthTypes() const
{
	return m_serverAuthenticationManager.supportedAuthTypes();
}



void VeyonServerProtocol::processAuthenticationMessage(VariantArrayMessage &message)
{
	m_serverAuthenticationManager.processAuthenticationMessage( client(), message );
}



void VeyonServerProtocol::performAccessControl()
{
	// perform access control via ServerAccessControl manager if either
	// client just entered access control or is still waiting to be
	// processed (e.g. desktop access dialog already active for a different connection)
	if( client()->accessControlState() == VncServerClient::AccessControlInit ||
			client()->accessControlState() == VncServerClient::AccessControlWaiting )
	{
		m_serverAccessControlManager.addClient( client() );
	}
}
