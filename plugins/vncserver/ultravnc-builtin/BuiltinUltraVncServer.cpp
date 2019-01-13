/*
 * BuiltinUltraVncServer.cpp - implementation of BuiltinUltraVncServer class
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

#include <windows.h>

#include "BuiltinUltraVncServer.h"
#include "LogoffEventFilter.h"
#include "UltraVncConfiguration.h"
#include "UltraVncConfigurationWidget.h"

extern bool Myinit( HINSTANCE hInstance );
extern int WinVNCAppMain();

static BuiltinUltraVncServer* vncServerInstance = nullptr;

extern BOOL multi;
extern HINSTANCE hAppInstance;
extern DWORD mainthreadId;
extern HINSTANCE hInstResDLL;


void ultravnc_veyon_load_password( char* out, int size )
{
	const auto password = vncServerInstance->password().toUtf8();

	if( password.size() == size )
	{
		memcpy( out, password.constData(), size ); // Flawfinder: ignore
	}
	else
	{
		qFatal( "Requested password too short!");
	}
}



BOOL ultravnc_veyon_load_int( LPCSTR valname, LONG *out )
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
		*out = vncServerInstance->configuration().ultraVncCaptureLayeredWindows() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "PollFullScreen" ) == 0 )
	{
		*out = vncServerInstance->configuration().ultraVncPollFullScreen() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "TurboMode" ) == 0 )
	{
		*out = vncServerInstance->configuration().ultraVncLowAccuracy() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "DeskDupEngine" ) == 0 )
	{
		*out = vncServerInstance->configuration().ultraVncDeskDupEngineEnabled() ? 1 : 0;
		return true;
	}
	if( strcmp( valname, "NewMSLogon" ) == 0 )
	{
		*out = 0;
		return true;
	}
	if( strcmp( valname, "MSLogonRequired" ) == 0 )
	{
		*out = 0;
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
		*out = 0;
		return true;
	}

	if( strcmp( valname, "PortNumber" ) == 0 )
	{
		*out = vncServerInstance->serverPort();
		return true;
	}

	if( strcmp( valname, "secondary" ) == 0 )
	{
		*out = vncServerInstance->configuration().ultraVncMultiMonitorSupportEnabled();
		return true;
	}

	if( strcmp( valname, "autocapt" ) == 0 )
	{
		*out = 0;
		return true;
	}

	return false;
}



BuiltinUltraVncServer::BuiltinUltraVncServer() :
	m_configuration(),
	m_logoffEventFilter( nullptr )
{
	vncServerInstance = this;
}



BuiltinUltraVncServer::~BuiltinUltraVncServer()
{
	if( m_logoffEventFilter )
	{
		delete m_logoffEventFilter;
	}

	vncServerInstance = nullptr;
}



QWidget* BuiltinUltraVncServer::configurationWidget()
{
	return new UltraVncConfigurationWidget( m_configuration );
}



void BuiltinUltraVncServer::prepareServer()
{
	// initialize global instance handler and main thread ID
	hAppInstance = GetModuleHandle( nullptr );
	mainthreadId = GetCurrentThreadId();

	hInstResDLL = hAppInstance;

	m_logoffEventFilter = new LogoffEventFilter;
}



void BuiltinUltraVncServer::runServer( int serverPort, const QString& password )
{
	m_serverPort = serverPort;
	m_password = password;

	// only allow multiple instances when explicitely working with multiple
	// service instances
	if( VeyonCore::hasSessionId() )
	{
		multi = true;
	}

	// run winvnc-server
	HMODULE hUser32 = LoadLibrary( "user32.dll" );
	typedef BOOL (*SetProcessDPIAwareFunc)();
	SetProcessDPIAwareFunc setDPIAware=NULL;
	if( hUser32 )
	{
		setDPIAware = (SetProcessDPIAwareFunc) GetProcAddress( hUser32, "SetProcessDPIAware" );
		if( setDPIAware )
		{
			setDPIAware();
		}
		FreeLibrary( hUser32 );
	}

	WinVNCAppMain();
}
