/*
 * VariantArrayMessage.cpp - class for sending/receiving a variant array as message
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

#include "VariantArrayMessage.h"


VariantArrayMessage::VariantArrayMessage( QIODevice* ioDevice ) :
	m_buffer(),
	m_stream( &m_buffer ),
	m_ioDevice( ioDevice )
{
	Q_ASSERT( m_ioDevice != nullptr );

	m_buffer.open( QBuffer::WriteOnly );
}



bool VariantArrayMessage::send()
{
	m_buffer.seek( 0 );

	MessageSize messageSize = htonl( m_buffer.size() );
	m_ioDevice->write( (const char *) &messageSize, sizeof(messageSize) );
	m_ioDevice->write( m_buffer.data() );

	return true;
}



bool VariantArrayMessage::isReadyForReceive()
{
	MessageSize messageSize;

	m_ioDevice->peek( (char *) &messageSize, sizeof(messageSize) );
	messageSize = ntohl(messageSize);

	return m_ioDevice->bytesAvailable() >= qint64( sizeof(messageSize) + messageSize );
}



bool VariantArrayMessage::receive()
{
	MessageSize messageSize;

	if( m_ioDevice->read( (char *) &messageSize, sizeof(messageSize) ) != sizeof(messageSize) )
	{
		qWarning( "VariantArrayMessage::receive(): could not read message size!" );
		return false;
	}

	messageSize = ntohl(messageSize);

	QByteArray data = m_ioDevice->read( messageSize );
	if( data.size() != (int) messageSize )
	{
		qWarning( "VariantArrayMessage::receive(): could not read message data!" );
		return false;
	}

	m_buffer.close();
	m_buffer.setData( data );
	m_buffer.open( QBuffer::ReadOnly );

	return true;
}
