/*
 * SocketDevice.h - SocketDevice abstraction
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _SOCKET_DEVICE_H
#define _SOCKET_DEVICE_H

#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QVariant>


typedef enum
{
	SocketRead,
	SocketWrite,
	SocketGetPeerAddress
} SocketOpCodes;


typedef qint64 ( * socketDispatcher )( char *buffer, const qint64 bytes,
									const SocketOpCodes opCode, void *user );


extern qint64 libvncClientDispatcher( char *buffer, const qint64 bytes,
									const SocketOpCodes opCode, void *user );


class SocketDevice : public QIODevice
{
public:
	SocketDevice( socketDispatcher sockDisp, void *user = NULL ) :
		QIODevice(),
		m_socketDispatcher( sockDisp ),
		m_user( user )
	{
		open( ReadWrite | Unbuffered );
	}

	QVariant read()
	{
		QDataStream d( this );
		return d;
	}

	void write( const QVariant &v )
	{
		QDataStream d( this );
		d << v;
	}

	socketDispatcher sockDispatcher()
	{
		return m_socketDispatcher;
	}

	void *user()
	{
		return m_user;
	}

	void setUser( void *user )
	{
		m_user = user;
	}

	qint64 read( char *buf, qint64 bytes )
	{
		return readData( buf, bytes );
	}

	qint64 write( const char *buf, qint64 bytes )
	{
		return writeData( buf, bytes );
	}


protected:
	qint64 readData( char *buf, qint64 bytes )
	{
		return m_socketDispatcher( buf, bytes, SocketRead, m_user );
	}

	qint64 writeData( const char *buf, qint64 bytes )
	{
		return m_socketDispatcher( const_cast<char *>( buf ), bytes,
												SocketWrite, m_user );
	}


private:
	socketDispatcher m_socketDispatcher;
	void * m_user;

} ;

#endif
