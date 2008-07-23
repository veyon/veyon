/*
 * isd_base.cpp - ISD-basics
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "isd_base.h"


qint64 qtcpsocketDispatcher( char * _buf, const qint64 _len,
				const socketOpCodes _op_code, void * _user )
{
	QTcpSocket * sock = static_cast<QTcpSocket *>( _user );
	qint64 ret = 0;
	switch( _op_code )
	{
		case SocketRead:
			while( ret < _len )
			{
				qint64 bytes_read = sock->read( _buf, _len );
				if( bytes_read < 0 )
				{
	qDebug( "qtcpsocketDispatcher(...): connection closed while reading" );
					return( 0 );
				}
				else if( bytes_read == 0 )
				{
					if( sock->state() !=
						QTcpSocket::ConnectedState )
					{
	qDebug( "qtcpsocketDispatcher(...): connection failed while writing  "
			"state:%d  error:%d", sock->state(), sock->error() );
						return( 0 );
					}
					sock->waitForReadyRead( 10 );
				}
				ret += bytes_read;
			}
			break;
		case SocketWrite:
			while( ret < _len )
			{
				qint64 written = sock->write( _buf, _len );
				if( written < 0 )
				{
	qDebug( "qtcpsocketDispatcher(...): connection closed while writing" );
					return( 0 );
				}
				else if( written == 0 )
				{
					if( sock->state() !=
						QTcpSocket::ConnectedState )
					{
	qDebug( "qtcpsocketDispatcher(...): connection failed while writing  "
			"state:%d error:%d", sock->state(), sock->error() );
						return( 0 );
					}
				}
				ret += written;
			}
			//sock->flush();
			sock->waitForBytesWritten();
			break;
		case SocketGetPeerAddress:
			strncpy( _buf,
		sock->peerAddress().toString().toAscii().constData(), _len );
			break;
	}
	return( ret );
}


