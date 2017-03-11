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

#include "ItalcCore.h"

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QCoreApplication>
#include <QProcess>

#include "rfb/rfbproto.h"

#include "AccessControlProvider.h"
#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "ItalcConfiguration.h"
#include "VncServer.h"


#ifdef ITALC_BUILD_LINUX
extern "C" int x11vnc_main( int argc, char * * argv );
#endif


#ifdef ITALC_BUILD_WIN32
extern bool Myinit( HINSTANCE hInstance );
extern int WinVNCAppMain();

void ultravnc_italc_load_password( char* out, int size )
{
	QByteArray password = ItalcCore::authenticationCredentials->internalVncServerPassword().toLatin1();
	memcpy( out, password.constData(), std::min<int>( size, password.length() ) );
}

static VncServer* vncServerInstance = nullptr;

BOOL ultravnc_italc_load_int( LPCSTR valname, LONG *out )
{
	if( strcmp( valname, "LoopbackOnly" ) == 0 )
	{
		*out = 1;
		return true;
	}
	if( strcmp( valname, "DisableTrayIcon" ) == 0 )
	{
		*out = 1;
		return true;
	}
	if( strcmp( valname, "AuthRequired" ) == 0 )
	{
		*out = 1;
		return true;
	}
	if( strcmp( valname, "CaptureAlphaBlending" ) == 0 )
	{
		*out = ItalcCore::config->vncCaptureLayeredWindows() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "PollFullScreen" ) == 0 )
	{
		*out = ItalcCore::config->vncPollFullScreen() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "TurboMode" ) == 0 )
	{
		*out = ItalcCore::config->vncLowAccuracy() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "NewMSLogon" ) == 0 )
	{
		*out = 1;
		return true;
	}
	if( strcmp( valname, "MSLogonRequired" ) == 0 )
	{
		*out = ItalcCore::config->isLogonAuthenticationEnabled() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "RemoveWallpaper" ) == 0 )
	{
		*out = 0;
		return true;
	}
	if( strcmp( valname, "FileTransferEnabled" ) == 0 )
	{
		*out = 0;
		return true;
	}
	if( strcmp( valname, "AllowLoopback" ) == 0 )
	{
		*out = 1;
		return true;
	}
	if( strcmp( valname, "AutoPortSelect" ) == 0 )
	{
		*out = 0;
		return true;
	}

	if( strcmp( valname, "HTTPConnect" ) == 0 )
	{
		return false;
	}

	if( strcmp( valname, "PortNumber" ) == 0 )
	{
		*out = vncServerInstance->serverPort();
		return true;
	}

	return false;
}
#endif



VncServer::VncServer( int serverPort ) :
	QThread(),
	m_password( CryptoCore::generateChallenge().toBase64().left( MAXPWLEN ) ),
	m_serverPort( serverPort )
{
	ItalcCore::authenticationCredentials->setInternalVncServerPassword( m_password );
#ifdef ITALC_BUILD_WIN32
	vncServerInstance = this;
#endif
}



VncServer::~VncServer()
{
#ifdef ITALC_BUILD_WIN32
	vncServerInstance = nullptr;
#endif
}



#ifdef ITALC_BUILD_LINUX
static void runX11vnc( QStringList cmdline, int port, const QString& password  )
{
	cmdline
		<< "-passwd" << password
		<< "-localhost"
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

	runX11vnc( cmdline, m_serverPort, m_password );

#elif ITALC_BUILD_WIN32

	// run winvnc-server
	Myinit( GetModuleHandle( NULL ) );
	WinVNCAppMain();

#endif

}
