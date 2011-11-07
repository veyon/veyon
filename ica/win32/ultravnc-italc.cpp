#include <windows.h>

#include <QtCore/QSettings>

#include "DesktopAccessPermission.h"
#include "ItalcCore.h"
#include "ItalcCoreServer.h"
#include "ItalcConfiguration.h"
#include "Configuration/LocalStore.h"

static QSettings *__italcSettings = NULL;


BOOL ultravnc_italc_load_int( LPCSTR valname, LONG *out )
{
	if( strcmp( valname, "LocalConnectOnly" ) == 0 )
	{
		if( __italcSettings == NULL )
		{
			__italcSettings = Configuration::LocalStore( Configuration::Store::System ).createSettingsObject();
			__italcSettings->beginGroup( "Network" );
		}

		*out = __italcSettings->value( valname ).toInt();
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
		*out = ItalcCore::config->isHttpServerEnabled() ? 1 : 0;
		return true;
	}

	if( strcmp( valname, "PortNumber" ) == 0 )
	{
		*out = ItalcCore::config->coreServerPort();
		return true;
	}

	if( strcmp( valname, "HTTPPortNumber" ) == 0 )
	{
		*out = ItalcCore::config->httpServerPort();
		return true;
	}

	return false;
}


BOOL ultravnc_italc_ask_permission( const char *username, const char *host )
{
	return DesktopAccessPermission(
				DesktopAccessPermission::LogonAuthentication ).
														ask( username, host );
}



/*
 * MSVCRT string functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#define CHECK_PMT(x) (x)
#define SIZE_T_MAX	((size_t)-1)


errno_t strcat_s( char* dst, size_t elem, const char* src )
{
    size_t i, j;
    if(!dst) return EINVAL;
    if(elem == 0) return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if(dst[i] == '\0')
        {
            for(j = 0; (j + i) < elem; j++)
            {
                if((dst[j + i] = src[j]) == '\0') return 0;
            }
        }
    }
    /* Set the first element to 0, not the first element after the skipped part */
    dst[0] = '\0';
    return ERANGE;
}

errno_t strncat_s( char* dst, size_t elem, const char* src, size_t count )
{
    size_t i, j;
    if(!CHECK_PMT(dst != 0) || !CHECK_PMT(elem != 0))
        return EINVAL;
    if(!CHECK_PMT(src != 0))
    {
        dst[0] = '\0';
        return EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if(dst[i] == '\0')
        {
            for(j = 0; (j + i) < elem; j++)
            {
                if(count == SIZE_T_MAX && j + i == elem - 1)
                {
                    dst[j + i] = '\0';
                    return STRUNCATE;
                }
                if(j == count || (dst[j + i] = src[j]) == '\0')
                {
                    dst[j + i] = '\0';
                    return 0;
                }
            }
        }
    }
    /* Set the first element to 0, not the first element after the skipped part */
    dst[0] = '\0';
    return ERANGE;
}


errno_t strcpy_s(char* dst, size_t elem, const char* src)
{
    size_t i;
    if(!elem) return EINVAL;
    if(!dst) return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if((dst[i] = src[i]) == '\0') return 0;
    }
    dst[0] = '\0';
    return ERANGE;
}



/*
 * msvcrt.dll heap functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */


errno_t strncpy_s(char* dst, size_t numberOfElements, const char* src, size_t count)
{
    size_t i, end;

    if(!count)
        return 0;

    if (!CHECK_PMT(dst != NULL) || !CHECK_PMT(src != NULL) || !CHECK_PMT(numberOfElements != 0)) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if(count!=SIZE_T_MAX && count<numberOfElements)
        end = count;
    else
        end = numberOfElements-1;

    for(i=0; i<end && src[i]; i++)
        dst[i] = src[i];

    if(!src[i] || end==count || count==SIZE_T_MAX) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    *_errno() = EINVAL;
    return EINVAL;
}
