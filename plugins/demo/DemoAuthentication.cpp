/*
 * DemoAuthentication.cpp - implementation of DemoAuthentication class
 *
 * Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "DemoAuthentication.h"
#include "VariantArrayMessage.h"


DemoAuthentication::DemoAuthentication( Plugin::Uid pluginUid ) :
    m_pluginUid( pluginUid )
{
}



bool DemoAuthentication::initializeCredentials()
{
	m_accessToken = CryptoCore::generateChallenge().toBase64();

	return hasCredentials();
}



bool DemoAuthentication::hasCredentials() const
{
	return m_accessToken.isEmpty() == false &&
			QByteArray::fromBase64( m_accessToken.toByteArray() ).size() == CryptoCore::ChallengeSize;
}



bool DemoAuthentication::checkCredentials() const
{
	return true;
}



VncServerClient::AuthState DemoAuthentication::performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		return VncServerClient::AuthState::Stage1;

	case VncServerClient::AuthState::Stage1:
	{
		const CryptoCore::PlaintextPassword token = message.read().toByteArray();  // Flawfinder: ignore

		if( hasCredentials() && token == m_accessToken )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}



bool DemoAuthentication::authenticate( QIODevice* socket ) const
{
	VariantArrayMessage tokenAuthMessage( socket );
	tokenAuthMessage.write( m_accessToken.toByteArray() );
	tokenAuthMessage.send();

	return false;
}
