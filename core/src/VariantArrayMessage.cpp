/*
 * VariantArrayMessage.cpp - class for sending/receiving a variant array as message
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"

#include "VariantArrayMessage.h"
#include "VariantStream.h"


VariantArrayMessage::VariantArrayMessage( QIODevice* ioDevice ) :
	m_buffer(),
	m_stream( &m_buffer ),
	m_ioDevice( ioDevice )
{
	Q_ASSERT( m_ioDevice != nullptr );

	m_buffer.open( QBuffer::ReadWrite ); // Flawfinder: ignore
}



bool VariantArrayMessage::send()
{
	MessageSize messageSize = qToBigEndian<MessageSize>( static_cast<MessageSize>( m_buffer.size() ) );
	m_ioDevice->write( reinterpret_cast<const char *>( &messageSize ), sizeof(messageSize) );
	m_ioDevice->write( m_buffer.data() );

	return true;
}



bool VariantArrayMessage::isReadyForReceive()
{
	MessageSize messageSize;

	if( m_ioDevice->peek( reinterpret_cast<char *>( &messageSize ), sizeof(messageSize) ) == sizeof(messageSize) )
	{
		messageSize = qFromBigEndian(messageSize);

		return m_ioDevice->bytesAvailable() >= static_cast<qint64>( sizeof(messageSize) + messageSize );
	}

	return false;
}



bool VariantArrayMessage::receive()
{
	MessageSize messageSize;

	if( m_ioDevice->read( reinterpret_cast<char *>( &messageSize ), sizeof(messageSize) ) != sizeof(messageSize) ) // Flawfinder: ignore
	{
		qWarning( "VariantArrayMessage::receive(): could not read message size!" );
		return false;
	}

	messageSize = qFromBigEndian(messageSize);
	if( messageSize > MaxMessageSize )
	{
		qCritical() << Q_FUNC_INFO << "invalid message size" << messageSize;
		return false;
	}

	const auto data = m_ioDevice->read( messageSize ); // Flawfinder: ignore
	if( data.size() != static_cast<qint64>( messageSize ) )
	{
		qWarning( "VariantArrayMessage::receive(): could not read message data!" );
		return false;
	}

	m_buffer.close();
	m_buffer.setData( data );
	m_buffer.open( QBuffer::ReadOnly ); // Flawfinder: ignore

	return true;
}



QVariant VariantArrayMessage::read() // Flawfinder: ignore
{
	return m_stream.read(); // Flawfinder: ignore
}



VariantArrayMessage& VariantArrayMessage::write( const QVariant& v )
{
	m_stream.write( v );

	return *this;
}
