/*
 * LocalUsersAndGroupsBuiltin.cpp - implementation of UsersAndGroupsPluginInterface
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#define UNICODE
#include <winsock2.h>
#include <windef.h>
#include <wtsapi32.h>
#include <lm.h>
#endif

#include "LocalAccessControlDataBackend.h"

#include <QProcess>

#ifdef ITALC_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ITALC_HAVE_PWD_H
#include <pwd.h>
#endif


void LocalAccessControlDataBackend::reloadConfiguration()
{
}


QStringList LocalAccessControlDataBackend::users()
{
	// TODO
	return QStringList();
}



QStringList LocalAccessControlDataBackend::userGroups()
{
	QStringList groupList;

#ifdef ITALC_BUILD_LINUX
	QProcess getentProcess;
	getentProcess.start( "getent", QStringList( { "group" } ) );
	getentProcess.waitForFinished();

	for( auto group : QString( getentProcess.readAll() ).split( '\n' ) )
	{
		groupList += group.split( ':' ).first();
	}

	QStringList ignoredGroups( {
		"root",
		"daemon",
		"bin",
		"tty",
		"disk",
		"lp",
		"mail",
		"news",
		"uucp",
		"man",
		"proxy",
		"kmem",
		"dialout",
		"fax",
		"voice",
		"cdrom",
		"tape",
		"audio",
		"dip",
		"www-data",
		"backup",
		"list",
		"irc",
		"src",
		"gnats",
		"shadow",
		"utmp",
		"video",
		"sasl",
		"plugdev",
		"games",
		"users",
		"nogroup",
		"libuuid",
		"syslog",
		"fuse",
		"lpadmin",
		"ssl-cert",
		"messagebus",
		"crontab",
		"mlocate",
		"avahi-autoipd",
		"netdev",
		"saned",
		"sambashare",
		"haldaemon",
		"polkituser",
		"mysql",
		"avahi",
		"klog",
		"floppy",
		"oprofile",
		"netdev",
		"dirmngr",
		"vboxusers",
		"bluetooth",
		"colord",
		"libvirtd",
		"nm-openvpn",
		"input",
		"kvm",
		"pulse",
		"pulse-access",
		"rtkit",
		"scanner",
		"sddm",
		"systemd-bus-proxy",
		"systemd-journal",
		"systemd-network",
		"systemd-resolve",
		"systemd-timesync",
		"uuidd",
							   } );

	for( auto ignoredGroup : ignoredGroups )
	{
		groupList.removeAll( ignoredGroup );
	}
#endif

#ifdef ITALC_BUILD_WIN32
	LPBYTE outBuffer = NULL;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetLocalGroupEnum( NULL, 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, NULL ) == NERR_Success )
	{
		LOCALGROUP_INFO_0* groupInfos = (LOCALGROUP_INFO_0 *) outBuffer;

		for( DWORD i = 0; i < entriesRead; ++i )
		{
				groupList += QString::fromUtf16( (const ushort *) groupInfos[i].lgrpi0_name );
		}

		NetApiBufferFree( outBuffer );
	}
#endif

	// remove all empty entries
	groupList.removeAll( "" );

	return groupList;
}



QStringList LocalAccessControlDataBackend::groupsOfUser( const QString& userName )
{
	QStringList groupList;

#ifdef ITALC_BUILD_LINUX
	QProcess getentProcess;
	getentProcess.start( "getent", QStringList() << "group" );
	getentProcess.waitForFinished();

	for( auto group : QString( getentProcess.readAll() ).split( '\n' ) )
	{
		QStringList groupComponents = group.split( ':' );
		if( groupComponents.size() == 4 &&
				groupComponents.last().split( ',' ).contains( userName ) )
		{
			groupList += groupComponents.first();
		}
	}
#endif

#ifdef ITALC_BUILD_WIN32
	LPBYTE outBuffer = NULL;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetUserGetLocalGroups( NULL, (LPCWSTR) userName.utf16(), 0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
							   &entriesRead, &totalEntries ) == NERR_Success )
	{
		LOCALGROUP_USERS_INFO_0* localGroupUsersInfo = (LOCALGROUP_USERS_INFO_0 *) outBuffer;

		for( DWORD i = 0; i < entriesRead; ++i )
		{
				groupList += QString::fromUtf16( (const ushort *) localGroupUsersInfo[i].lgrui0_name );
		}

		NetApiBufferFree( outBuffer );
	}
#endif

	groupList.removeAll( "" );

	return groupList;
}



QStringList LocalAccessControlDataBackend::allRooms()
{
	return QStringList();
}



QStringList LocalAccessControlDataBackend::roomsOfComputer( const QString& computerName )
{
	return QStringList();
}
