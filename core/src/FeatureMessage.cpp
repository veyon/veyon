/*
 * FeatureMessage.cpp - implementation of a message encapsulation class for features
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
#else
#include <netinet/in.h>
#endif

#include <QBuffer>

#include "FeatureMessage.h"
#include "VariantStream.h"


bool FeatureMessage::send()
{
	return send( m_ioDevice );
}



bool FeatureMessage::send( QIODevice* ioDevice ) const
{
	if( ioDevice )
	{
		qDebug() << "FeatureMessage::send():" << featureUid() << command() << arguments();

		QBuffer buffer;
		buffer.open( QBuffer::ReadWrite );

		VariantStream stream( &buffer );
		stream.write( m_featureUid );
		stream.write( m_command );
		stream.write( m_arguments );

		buffer.seek( 0 );

		MessageSize messageSize = htonl( buffer.size() );
		ioDevice->write( (const char *) &messageSize, sizeof(messageSize) );
		ioDevice->write( buffer.data() );

		return true;
	}

	qCritical( "FeatureMessage::send(): no IO device!" );

	return false;
}



bool FeatureMessage::isReadyForReceive()
{
	MessageSize messageSize;

	m_ioDevice->peek( (char *) &messageSize, sizeof(messageSize) );
	messageSize = ntohl(messageSize);

	return m_ioDevice->bytesAvailable() >= qint64( sizeof(messageSize) + messageSize );
}



FeatureMessage &FeatureMessage::receive()
{
	if( m_ioDevice )
	{
		MessageSize messageSize;

		if( m_ioDevice->read( (char *) &messageSize, sizeof(messageSize) ) != sizeof(messageSize) )
		{
			qWarning( "FeatureMessage::receive(): could not read message size!" );
			return *this;
		}

		messageSize = ntohl(messageSize);

		VariantStream stream( m_ioDevice );
		m_featureUid = stream.read().toUuid();
		m_command = stream.read().value<Command>();
		m_arguments = stream.read().toMap();

		qDebug() << "FeatureMessage::receive():" << featureUid() << command() << arguments();
	}
	else
	{
		qCritical( "FeatureMessage::receive(): no IO device!" );
	}

	return *this;
}
