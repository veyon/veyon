/*
 * LogonAclSettings.h - helper class for reading and setting logon ACLs
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _LOGON_ACL_SETTINGS_H
#define _LOGON_ACL_SETTINGS_H

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32

#include <windows.h>

#include <QtCore/QByteArray>

class LogonAclSettings
{
public:
	LogonAclSettings()
	{
	}

	QString acl() const
	{
		HKEY hk = NULL;
		QString r;
		if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							"Software\\iTALC Solutions\\iTALC\\Authentication",
							0, KEY_QUERY_VALUE, &hk ) != ERROR_SUCCESS )
		{
			qCritical( "Could not open ACL key for reading" );
			return r;
		}

		DWORD buflen;
		if( RegQueryValueEx( hk, "LogonACL", 0, 0, NULL, &buflen ) != ERROR_SUCCESS )
		{
			qCritical( "Could not read ACL key" );
			RegCloseKey( hk );
			return r;
		}

		BYTE *data = new BYTE[buflen];
		if( RegQueryValueEx( hk, "LogonACL", 0, 0, data, &buflen ) == ERROR_SUCCESS )
		{
			r = QByteArray( (const char *) data, buflen ).toBase64();
		}
		else
		{
			qCritical( "Could not read ACL key" );
		}

		RegCloseKey( hk );
		delete[] data;

		return r;
	}

	void setACL( const QString &base64ACL )
	{
		if( base64ACL.isEmpty() )
		{
			return;
		}

		QByteArray d = QByteArray::fromBase64( base64ACL.toAscii() );
		HKEY hk = NULL;
		if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							"Software\\iTALC Solutions\\iTALC\\Authentication",
							0, KEY_SET_VALUE, &hk ) != ERROR_SUCCESS )
		{
			qCritical( "Could not open ACL key for writing" );
			return;
		}

		if( RegSetValueEx( hk, "LogonACL", 0, REG_BINARY, (LPBYTE) d.constData(),
							d.size() ) != ERROR_SUCCESS )
		{
			qCritical( "Could not set ACL key" );
		}

		RegCloseKey( hk );
	}

} ;

#endif

#endif
