/*
 * SocketDevice.h - SocketDevice abstraction
 *
 * Copyright (c) 2010-2025 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QIODevice>

class SocketDevice : public QIODevice
{
	Q_OBJECT
public:
	enum SocketOperation
	{
		SocketOpRead,
		SocketOpWrite
	} ;

	using Dispatcher = qint64 (*)(char *, const qint64, SocketOperation, void *);

	explicit SocketDevice( Dispatcher dispatcher, void *user = nullptr, QObject* parent = nullptr ) :
		QIODevice( parent ),
		m_dispatcher( dispatcher ),
		m_user( user )
	{
		open( ReadWrite | Unbuffered ); // Flawfinder: ignore
	}

	Dispatcher dispatcher()
	{
		return m_dispatcher;
	}

	void *user()
	{
		return m_user;
	}

	void setUser( void *user )
	{
		m_user = user;
	}

	qint64 read( char *buf, qint64 bytes ) // Flawfinder: ignore
	{
		return readData( buf, bytes );
	}

	qint64 write( const char *buf, qint64 bytes )
	{
		return writeData( buf, bytes );
	}

protected:
	qint64 readData( char *buf, qint64 bytes ) override
	{
		return m_dispatcher( buf, bytes, SocketOpRead, m_user );
	}

	qint64 writeData( const char *buf, qint64 bytes ) override
	{
		return m_dispatcher( const_cast<char *>( buf ), bytes,
												SocketOpWrite, m_user );
	}


private:
	Dispatcher m_dispatcher;
	void * m_user;

} ;
