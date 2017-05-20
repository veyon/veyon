/*
 * FeatureMessage.cpp - implementation of a message encapsulation class for features
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

#include "FeatureMessage.h"
#include "VariantArrayMessage.h"


bool FeatureMessage::send()
{
	return send( m_ioDevice );
}



bool FeatureMessage::send( QIODevice* ioDevice ) const
{
	if( ioDevice )
	{
		VariantArrayMessage message( ioDevice );

		message.write( m_featureUid );
		message.write( m_command );
		message.write( m_arguments );

		return message.send();
	}

	qCritical( "FeatureMessage::send(): no IO device!" );

	return false;
}



bool FeatureMessage::isReadyForReceive()
{
	return m_ioDevice != nullptr &&
			VariantArrayMessage( m_ioDevice ).isReadyForReceive();
}



FeatureMessage &FeatureMessage::receive()
{
	if( m_ioDevice )
	{
		VariantArrayMessage message( m_ioDevice );

		if( message.receive() == false )
		{
			qWarning( "FeatureMessage::receive(): could not receive message!" );
			return *this;
		}

		m_featureUid = message.read().toUuid();
		m_command = message.read().value<Command>();
		m_arguments = message.read().toMap();
	}
	else
	{
		qCritical( "FeatureMessage::receive(): no IO device!" );
	}

	return *this;
}
