/*
 * VariantStream.cpp - read/write QVariant objects to/from QIODevice
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

#include <QTcpSocket>

#include "VariantStream.h"

VariantStream::VariantStream( QIODevice* ioDevice ) :
	m_dataStream( this ),
	m_ioDevice( ioDevice ),
	m_socket( nullptr )
{
	m_dataStream.setVersion( QDataStream::Qt_5_5 );

	open( QIODevice::ReadWrite | QIODevice::Unbuffered );
}



VariantStream::VariantStream( QTcpSocket* socket ) :
	m_dataStream( this ),
	m_ioDevice( socket ),
	m_socket( socket )
{
	m_dataStream.setVersion( QDataStream::Qt_5_5 );

	open( QIODevice::ReadWrite | QIODevice::Unbuffered );
}



VariantStream::~VariantStream()
{
}



QVariant VariantStream::read()
{
	QVariant v;
	m_dataStream >> v;

	if( m_socket && ( v.isValid() == false || v.isNull() ) )
	{
		m_socket->close();
	}

	return v;
}



void VariantStream::write( const QVariant &v )
{
	m_dataStream << v;
}



qint64 VariantStream::bytesAvailable() const
{
	return m_ioDevice->bytesAvailable();
}



qint64 VariantStream::readData( char* data, qint64 maxSize )
{
	// implement blocking read

	qint64 bytesRead = 0;
	while( bytesRead < maxSize )
	{
		qint64 n = m_ioDevice->read( data + bytesRead, maxSize - bytesRead );
		if( n < 0 )
		{
			qDebug( "VariantStream: read() returned %d while reading %d of %d bytes", (int) n, (int) bytesRead, (int) maxSize );
			return -1;
		}
		bytesRead += n;
		if( bytesRead < maxSize && m_ioDevice->waitForReadyRead( 5000 ) == false )
		{
			qDebug( "VariantStream: timeout after reading %d of %d bytes", (int) bytesRead, (int) maxSize );
			return bytesRead;
		}
	}

	return bytesRead;
}



qint64 VariantStream::writeData( const char* data, qint64 maxSize )
{
	return m_ioDevice->write( data, maxSize );
}
