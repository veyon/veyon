/*
 * ItalcVncServer.cpp - implementation of ItalcVncServer, a VNC-server-
 *                      abstraction for platform independent VNC-server-usage
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

#include "rfb/rfb.h"

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#include <pthread.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include "ItalcVncServer.h"
#include "ItalcCore.h"
#include "ItalcCoreServer.h"


extern "C" int x11vnc_main( int argc, char * * argv );


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




static rfbBool lvs_italcHandleMessage( struct _rfbClientRec *client,
					void *data,
					const rfbClientToServerMsg *message )
{
	if( message->type == rfbItalcCoreRequest )
	{
		return ItalcCoreServer::instance()->
				handleItalcClientMessage( libvncServerDispatcher, client );
	}
	return false;
}


extern "C" void rfbClientSendString(rfbClientPtr cl, char *reason);


static void lvs_italcSecurityHandler( struct _rfbClientRec *cl )
{
	bool authOK = ItalcCoreServer::instance()->
								authSecTypeItalc( libvncServerDispatcher, cl );

	uint32_t result = authOK ? rfbVncAuthOK : rfbVncAuthFailed;
	result = Swap32IfLE( result );
	rfbWriteExact( cl, (char *) &result, 4 );

	if( authOK )
	{
		cl->state = rfbClientRec::RFB_INITIALISATION;
		__client = cl;
	}
	else
	{
		rfbClientSendString( cl, (char *) "Signature verification failed!" );
    }
}



#ifdef ITALC_BUILD_WIN32

extern bool Myinit( HINSTANCE hInstance );
extern int WinVNCAppMain();

#endif



ItalcVncServer::ItalcVncServer() :
	QThread(),
	m_port( ItalcCore::serverPort )
{
}



ItalcVncServer::~ItalcVncServer()
{
}



static void runX11vnc( QStringList cmdline, int port, bool plainVnc )
{
	cmdline
		<< "-nosel"	// do not exchange clipboard-contents
		<< "-nosetclipboard"	// do not exchange clipboard-contents
		<< "-rfbport" << QString::number( port )
				// set port where the VNC server should listen
		;

#ifdef ITALC_BUILD_LINUX
	// workaround for x11vnc when running in an NX session
	foreach( const QString &s, QProcess::systemEnvironment() )
	{
		if( s.startsWith( "NXSESSIONID=" ) )
		{
			cmdline << "-noxdamage";
		}
	}
#endif

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

	if( plainVnc == false )
	{
		// register iTALC protocol extension
		rfbProtocolExtension pe;
		pe.newClient = italcCoreNewClient;
		pe.init = NULL;
		pe.enablePseudoEncoding = NULL;
		pe.pseudoEncodings = NULL;
		pe.handleMessage = lvs_italcHandleMessage;
		pe.close = NULL;
		pe.usage = NULL;
		pe.processArgument = NULL;
		pe.next = NULL;
		rfbRegisterProtocolExtension( &pe );
	}

	// register handler for iTALC's security-type
	rfbSecurityHandler sh = { rfbSecTypeItalc, lvs_italcSecurityHandler, NULL };
	rfbRegisterSecurityHandler( &sh );

	// run x11vnc-server
	x11vnc_main( argc, argv );

}



void ItalcVncServer::runVncReflector( int srcPort, int dstPort )
{
#ifdef ITALC_BUILD_WIN32
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif

	QStringList args;
	args << "-viewonly"
		<< "-threads"
		<< "-reflect"
		<< QString( "localhost:%1" ).arg( srcPort );

	while( 1 )
	{
		runX11vnc( args, dstPort, true );
	}

#ifdef ITALC_BUILD_WIN32
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif
}



void ItalcVncServer::run()
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

	runX11vnc( cmdline, m_port, false );

#elif ITALC_BUILD_WIN32

	// run winvnc-server
	Myinit( GetModuleHandle( NULL ) );
	WinVNCAppMain();

#endif

}


