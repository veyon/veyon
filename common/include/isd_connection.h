/*
 * isd_connection.h - declaration of class isdConnection, a client-
 *                    implementation for ISD (iTALC Service Daemon)
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

#ifndef _ISD_CONNECTION_H
#define _ISD_CONNECTION_H

#include <QtCore/QThread>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>

#include "isd_base.h"


class isdConnection : public QObject
{
public:
	enum states
	{
		Disconnected,
		Connecting,
		Connected,
		HostUnreachable,
		ConnectionRefused,
		ConnectionFailed,
		InvalidServer,
		AuthFailed,
		UnknownError
	} ;


	isdConnection( const QString & _host, QObject * _parent = 0 );
	virtual ~isdConnection();

	virtual states open( void );
	virtual void close( void );
	states reset( const QString & _host = "" );

	states state( void ) const
	{
		return( m_state );
	}

	bool hasData( void ) const
	{
		return( m_socket->bytesAvailable() > 0 );
	}


	bool handleServerMessages( bool _send_screen_update );

	inline const QString & user( void )
	{
		return( m_user );
	}

	inline const QString & userHomeDir( void )
	{
		return( m_userHomeDir );
	}

	inline QString host( void ) const
	{
		return( m_host == QHostAddress( QHostAddress::LocalHost ).
								toString() ?
					QHostInfo::localHostName() : m_host );
	}

	inline int port( void ) const
	{
		return( m_port );
	}

	inline int demoServerPort( void ) const
	{
		return( m_demoServerPort );
	}


	bool handleServerMessages( void );

	bool sendGetUserInformationRequest( void );
	bool execCmds( const QString & _cmd );
	bool startDemo( const QString & _master_ip, bool _full_screen = FALSE );
	bool stopDemo( void );
	bool lockDisplay( void );
	bool unlockDisplay( void );
	bool logoutUser( void );
	bool displayTextMessage( const QString & _msg );
	bool sendFile( const QString & _fname );
	bool collectFiles( const QString & _nfilter );
	bool remoteControlDisplay( const QString & _ip,
						bool _view_only = FALSE );

	bool wakeOtherComputer( const QString & _mac );
	bool powerDownComputer( void );
	bool restartComputer( void );

/*	bool ivsRun( const bool _host_based_auth = TRUE );
	bool ivsTerminate( void );*/

	bool demoServerRun( void );
	bool demoServerStop( void );
	bool demoServerAllowClient( const QString & _client );
	bool demoServerDenyClient( const QString & _client );

/*	bool ivsAllowClient( const QString & _client );
	bool ivsDenyClient( const QString & _client );*/

//	bool terminateISD( void );


	// read private key and/or create new key-pair if necessary
	static bool initAuthentication( void );


protected:
	bool readFromServer( char * _out, const unsigned int _n );
	bool writeToServer( const char * _buf, const unsigned int _n );
	long readCompactLen( void );

	virtual states protocolInitialization( void );

	states authAgainstServer( void );

	states & state_ref( void )
	{
		return( m_state );
	}

	bool handleServerMessage( Q_UINT8 _msg );

	socketDevice & socketDev( void )
	{
		return( m_socketDev );
	}


private:
	QTcpSocket * m_socket;
	states m_state;

	socketDevice m_socketDev;

	QString m_host;
	int m_port, m_demoServerPort;
	QString m_user;
	QString m_userHomeDir;

} ;


#endif
