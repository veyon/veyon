/*
 * IVS.cpp - implementation of IVS, a VNC-server-abstraction for platform-
 *           independent VNC-server-usage
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QStringList>


#include "IVS.h"
#include "ItalcRfbExt.h"

int __ivs_port = PortOffsetIVS;

#ifdef ITALC_BUILD_LINUX

extern "C" int x11vnc_main( int argc, char * * argv );

#include "rfb/rfb.h"
#include "ItalcCoreServer.h"
#include "LocalSystem.h"


rfbClientPtr __client = NULL;


qint64 libvncServerDispatcher( char * _buf, const qint64 _len,
				const SocketOpCodes _op_code, void * _user )
{
	rfbClientPtr cl = (rfbClientPtr) _user;
	switch( _op_code )
	{
		case SocketRead:
			return rfbReadExact( cl, _buf, _len ) == 1 ? _len : 0;
		case SocketWrite:
			return rfbWriteExact( cl, _buf, _len ) == 1 ? _len : 0;
		case SocketGetPeerAddress:
			strncpy( _buf, cl->host, _len );
			break;
	}
	return 0;

}




static rfbBool italcCoreNewClient( struct _rfbClientRec *, void * * )
{
	return true;
}




static rfbBool italcCoreHandleMessage( struct _rfbClientRec * _client,
					void * _data,
					const rfbClientToServerMsg * _message )
{
	if( _message->type == rfbItalcCoreRequest )
	{
		return ItalcCoreServer::instance()->
				processClient( libvncServerDispatcher,
								_client );
	}
	return false;
}




static void italcCoreSecurityHandler( struct _rfbClientRec * _client )
{
	if( ItalcCoreServer::instance()->
			authSecTypeItalc( libvncServerDispatcher, _client,
								ItalcAuthDSA ) )
	{
		_client->state = rfbClientRec::RFB_INITIALISATION;
		__client = _client;
	}
}



#elif ITALC_BUILD_WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern bool Myinit( HINSTANCE hInstance );
extern int WinVNCAppMain( void );

#endif



IVS::IVS( const quint16 _ivs_port, int _argc, char * * _argv,
							bool _no_threading ) :
	QThread(),
	m_argc( _argc ),
	m_argv( _argv ),
	m_port( _ivs_port )
{
	if( _no_threading )
	{
		run();
	}
}




IVS::~IVS()
{
}




void IVS::run( void )
{
#ifdef ITALC_BUILD_LINUX
	QStringList cmdline;

	// filter some options
	for( int i = 0; i < m_argc; ++i )
	{
		QString option = m_argv[i];
		if( option == "-noshm" || option == "-solid" ||
				option == "-xrandr" || option == "-onetile" )
		{
			cmdline.append( m_argv[i] );
		}
	}

	cmdline/* << "-forever"	// do not quit after 1st conn.
		<< "-shared"	// allow multiple clients
		<< "-nopw"	// do not display warning
		<< "-repeat"	// do not disable autorepeat*/
		<< "-nosel"	// do not exchange clipboard-contents
		<< "-nosetclipboard"	// do not exchange clipboard-contents
/*		<< "-speeds" << ",5000,1"	// FB-rate: 7 MB/s
						// LAN: 5 MB/s
						// latency: 1 ms*/
/*		<< "-wait" << "25"	// time between screen-pools*/
/*		<< "-readtimeout" << "60" // timeout for disconn.*/
/*			<< "-threads"	// enable threaded libvncserver*/
/*		<< "-noremote"	// do not accept remote-cmds
		<< "-nocmds"	// do not run ext. cmds*/
		<< "-rfbport" << QString::number( m_port )
				// set port where the VNC-server should listen
		;

	char * old_av = m_argv[0];
	m_argv = new char *[cmdline.size()+1];
	m_argc = 1;
	m_argv[0] = old_av;

	for( QStringList::iterator it = cmdline.begin();
				it != cmdline.end(); ++it, ++m_argc )
	{
		m_argv[m_argc] = new char[it->length() + 1];
		strcpy( m_argv[m_argc], it->toUtf8().constData() );
	}

	// register iTALC-protocol-extension
	rfbProtocolExtension pe;
	pe.newClient = italcCoreNewClient;
	pe.init = NULL;
	pe.enablePseudoEncoding = NULL;
	pe.pseudoEncodings = NULL;
	pe.handleMessage = italcCoreHandleMessage;
	pe.close = NULL;
	pe.usage = NULL;
	pe.processArgument = NULL;
	pe.next = NULL;
	rfbRegisterProtocolExtension( &pe );

	// register handler for iTALC's security-type
	rfbSecurityHandler sh = { rfbSecTypeItalc,
					italcCoreSecurityHandler,
					NULL };
	rfbRegisterSecurityHandler( &sh );

	// run x11vnc-server
	x11vnc_main( m_argc, m_argv );

#elif ITALC_BUILD_WIN32

	// run winvnc-server
	Myinit( GetModuleHandle( NULL ) );
	WinVNCAppMain();

#endif

}


