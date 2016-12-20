/*
 * ultravnc-italc.h - integration of UltraVNC server into iTALC service
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <windows.h>

#include <QtCore/QSettings>

#include "DesktopAccessPermission.h"
#include "ItalcCore.h"
#include "ItalcCoreServer.h"
#include "ItalcConfiguration.h"
#include "AccessControlProvider.h"
#include "Configuration/LocalStore.h"


BOOL ultravnc_italc_load_int( LPCSTR valname, LONG *out )
{
	if( strcmp( valname, "LoopbackOnly" ) == 0 )
	{
		*out = 0;
		if( ItalcCore::config->localConnectOnly() )
		{
			*out = 1;
		}
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
		*out = ItalcCore::config->coreServerPort();
		return true;
	}

	return false;
}


BOOL ultravnc_italc_access_control( const char *username, const char *host )
{
	return AccessControlProvider().checkAccess( username, host ) == AccessControlProvider::AccessAllow &&
	        DesktopAccessPermission( DesktopAccessPermission::LogonAuthentication ).ask( username, host );
}

