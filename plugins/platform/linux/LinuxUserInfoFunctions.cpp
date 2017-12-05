/*
 * LinuxUserInfoFunctions.cpp - implementation of LinuxUserInfoFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QProcess>

#include "LinuxUserInfoFunctions.h"
#include "LocalSystem.h"

QStringList LinuxUserInfoFunctions::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString( getentProcess.readAll() ).split( '\n' );

	groupList.reserve( groups.size() );

	for( const auto& group : groups )
	{
		groupList += group.split( ':' ).first();
	}

	const QStringList ignoredGroups( {
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

	for( const auto& ignoredGroup : ignoredGroups )
	{
		groupList.removeAll( ignoredGroup );
	}

	// remove all empty entries
	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QStringList LinuxUserInfoFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	const auto strippedUsername = LocalSystem::User::stripDomain( username );

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString( getentProcess.readAll() ).split( '\n' );
	for( const auto& group : groups )
	{
		const auto groupComponents = group.split( ':' );
		if( groupComponents.size() == 4 &&
				groupComponents.last().split( ',' ).contains( strippedUsername ) )
		{
			groupList += groupComponents.first(); // clazy:exclude=reserve-candidates
		}
	}

	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QStringList LinuxUserInfoFunctions::loggedOnUsers()
{
	QStringList users;

	QProcess whoProcess;
	whoProcess.start( QStringLiteral("who") );
	whoProcess.waitForFinished( WhoProcessTimeout );

	if( whoProcess.exitCode() != 0 )
	{
		return users;
	}

	const auto lines = whoProcess.readAll().split( '\n' );
	for( const auto& line : lines )
	{
		const auto user = line.split( ' ' ).value( 0 );
		if( user.isEmpty() == false && users.contains( user ) == false )
		{
			users.append( user ); // clazy:exclude=reserve-candidates
		}
	}

	return users;
}
