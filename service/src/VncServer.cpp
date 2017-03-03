/*
 * VncServer.cpp - implementation of VncServer, a VNC-server-
 *                      abstraction for platform independent VNC-server-usage
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "AccessControlProvider.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "VncServer.h"


extern "C" int x11vnc_main( int argc, char * * argv );


#ifndef ITALC_BUILD_WIN32

#if 0
static qint64 libvncServerDispatcher( char * buffer, const qint64 bytes,
									  const SocketOpCodes opCode, void * user )
{
	rfbClientPtr cl = (rfbClientPtr) user;
	switch( opCode )
	{
	case SocketRead:
		return rfbReadExact( cl, buffer, bytes ) == 1 ? bytes : 0;
	case SocketWrite:
		return rfbWriteExact( cl, buffer, bytes ) == 1 ? bytes : 0;
	case SocketGetPeerAddress:
		strncpy( buffer, cl->host, bytes );
		break;
	}
	return 0;

}


static rfbBool italcCoreNewClient( struct _rfbClientRec *, void * * )
{
	return true;
}




static rfbBool lvs_italcHandleMessage( struct _rfbClientRec *client,
					void *data,
					const rfbClientToServerMsg *message )
{
	if( message->type == rfbItalcCoreRequest )
	{
		return ComputerControlServer::instance()->handleItalcCoreMessage( libvncServerDispatcher, client );
	}
	else if( message->type == rfbItalcFeatureRequest )
	{
		return ComputerControlServer::instance()->handleItalcFeatureMessage( libvncServerDispatcher, client );
	}

	return false;
}


extern "C" void rfbClientSendString(rfbClientPtr cl, char *reason);


static void lvs_italcSecurityHandler( struct _rfbClientRec *cl )
{
	bool authOK = ComputerControlServer::instance()->
								authSecTypeItalc( libvncServerDispatcher, cl );

	uint32_t result = authOK ? rfbVncAuthOK : rfbVncAuthFailed;
	result = Swap32IfLE( result );
	rfbWriteExact( cl, (char *) &result, 4 );

	if( authOK )
	{
		cl->state = rfbClientRec::RFB_INITIALISATION;
	}
	else
	{
		rfbClientSendString( cl, (char *) "Signature verification failed!" );
    }
}
#endif

#endif


#ifdef ITALC_BUILD_WIN32

extern bool Myinit( HINSTANCE hInstance );
extern int WinVNCAppMain();

#endif



VncServer::VncServer( int serverPort ) :
	QThread(),
	m_serverPort( serverPort )
{
}



VncServer::~VncServer()
{
}



#ifdef ITALC_BUILD_LINUX
static void runX11vnc( QStringList cmdline, int port, bool plainVnc )
{
	cmdline
		<< "-nosel"	// do not exchange clipboard-contents
		<< "-nosetclipboard"	// do not exchange clipboard-contents
		<< "-rfbport" << QString::number( port )
				// set port where the VNC server should listen
		;

	// workaround for x11vnc when running in an NX session or a Thin client LTSP session
	foreach( const QString &s, QProcess::systemEnvironment() )
	{
		if( s.startsWith( "NXSESSIONID=" ) || s.startsWith( "X2GO_SESSION=" ) || s.startsWith( "LTSP_CLIENT_MAC=" ) )
		{
			cmdline << "-noxdamage";
		}
	}

	// build new C-style command line array based on cmdline-QStringList
	char **argv = new char *[cmdline.size()+1];
	argv[0] = qstrdup( QCoreApplication::arguments()[0].toUtf8().constData() );
	int argc = 1;

	for( QStringList::iterator it = cmdline.begin();
				it != cmdline.end(); ++it, ++argc )
	{
		argv[argc] = new char[it->length() + 1];
		strcpy( argv[argc], it->toUtf8().constData() );
	}

#if 0
	static bool firstTime = true;

	if( firstTime )
	{
		if( plainVnc == false )
		{
			// register iTALC protocol extension
			static rfbProtocolExtension pe;

			// initialize all pointers inside the struct to NULL
			memset( &pe, 0, sizeof( pe ) );

			// register our own handlers
			pe.newClient = italcCoreNewClient;
			pe.handleMessage = lvs_italcHandleMessage;

			rfbRegisterProtocolExtension( &pe );
		}

		// register handler for iTALC's security-type
		static rfbSecurityHandler shi = { rfbSecTypeItalc, lvs_italcSecurityHandler, NULL };
		rfbRegisterSecurityHandler( &shi );

#ifndef ITALC_BUILD_WIN32
		// register handler for MS Logon II security type
		static rfbSecurityHandler shmsl = { rfbUltraVNC_MsLogonIIAuth, lvs_msLogonIISecurityHandler, NULL };
		rfbRegisterSecurityHandler( &shmsl );
#endif

		firstTime = false;
	}
#endif

	// run x11vnc-server
	x11vnc_main( argc, argv );

}
#endif



void VncServer::run()
{
#ifdef ITALC_BUILD_LINUX
	QStringList cmdline;

	QStringList acceptedArguments;
	acceptedArguments
			<< "-noshm"
			<< "-solid"
			<< "-xrandr"
			<< "-onetile";

	foreach( const QString & arg, QCoreApplication::arguments() )
	{
		if( acceptedArguments.contains( arg ) )
		{
			cmdline.append( arg );
		}
	}

	if( ItalcCore::config->localConnectOnly() ||
			AccessControlProvider().isAccessDeniedByLocalState() )
	{
		cmdline.append( "-localhost" );
	}

	runX11vnc( cmdline, m_serverPort, false );

#elif ITALC_BUILD_WIN32

	// run winvnc-server
	Myinit( GetModuleHandle( NULL ) );
	WinVNCAppMain();

#endif

}
