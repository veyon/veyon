/*
 * PlatformServiceFunctions.cpp - implementation of PlatformServiceFunctions class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "PlatformServiceCore.h"


PlatformServiceCore::SessionId PlatformServiceCore::openSession( const QVariant& sessionData )
{
	for( SessionId i = 0; i <= SessionIdMax; ++i )
	{
		if( m_sessions.contains( i ) == false )
		{
			m_sessions[i] = sessionData;
			return i;
		}
	}

	return SessionIdInvalid;
}



void PlatformServiceCore::closeSession( SessionId sessionId )
{
	m_sessions.remove( sessionId );
}



QVariant PlatformServiceCore::sessionDataFromId( PlatformServiceCore::SessionId sessionId ) const
{
	return m_sessions.value( sessionId );
}



PlatformServiceCore::SessionId PlatformServiceCore::sessionIdFromData( const QVariant& data ) const
{
	return m_sessions.key( data, SessionIdInvalid );
}
