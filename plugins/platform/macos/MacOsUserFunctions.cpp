/*
 * MacOsUserFunctions.cpp - implementation of MacOsUserFunctions class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QDataStream>
#include <QProcess>

#include "MacOsCoreFunctions.h"
#include "MacOsSessionFunctions.h"
#include "MacOsUserFunctions.h"
#include "VeyonConfiguration.h"

#define XK_MISCELLANY

#include <pwd.h>
#include <unistd.h>


QString MacOsUserFunctions::fullName( const QString& username )
{
	auto pw_entry = getpwnam( VeyonCore::stripDomain( username ).toUtf8().constData() );

	if( pw_entry )
	{
		auto shell = QString::fromUtf8( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
				shell.endsWith( QStringLiteral( "/true" ) ) ||
				shell.endsWith( QStringLiteral( "/null" ) ) ||
				shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			return QString::fromUtf8( pw_entry->pw_gecos ).split( QLatin1Char(',') ).first();
		}
	}

	return QString();
}



QStringList MacOsUserFunctions::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	QStringList groupList;
	QProcess dsclProcess;
	dsclProcess.start( QStringLiteral("dscl"), { QStringLiteral("."), QStringLiteral("list"), QStringLiteral("/groups") });
	dsclProcess.waitForFinished();

	const auto groups = QString::fromUtf8( dsclProcess.readAll() ).split( QLatin1Char('\n') );

	groupList.reserve(groups.size());

	for( const auto& group : groupList )
	{
		if (!group.startsWith(QLatin1Char('_'),Qt::CaseInsensitive))
		{
			groupList.append(group);
		}
		
	}

	const QStringList ignoredGroups( {
		QStringLiteral("access_bpf"),
		QStringLiteral("accessibility"),
		QStringLiteral("authedusers"),
		QStringLiteral("bin"),
		QStringLiteral("certusers"),
		QStringLiteral("com.apple.access_disabled"),
		QStringLiteral("com.apple.access_ftp"),
		QStringLiteral("com.apple.access_remote_ae"),
		QStringLiteral("com.apple.access_screensharing"),
		QStringLiteral("com.apple.access_sessionkey"),
		QStringLiteral("com.apple.access_ssh"),
		QStringLiteral("com.apple.sharepoint.group.1"),
		QStringLiteral("consoleusers"),
		QStringLiteral("daemon"),
		QStringLiteral("dialer"),
		QStringLiteral("kmem"),
		QStringLiteral("macports"),
		QStringLiteral("mail"),
		QStringLiteral("network"),
		QStringLiteral("nobody"),
		QStringLiteral("nogroup"),
		QStringLiteral("procmod"),
		QStringLiteral("procview"),
		QStringLiteral("sys"),
		QStringLiteral("tty"),
		QStringLiteral("utmp"),
		} );

	for( const auto& ignoredGroup : ignoredGroups )
	{
		groupList.removeAll( ignoredGroup );
	}

	// remove all empty entries
	groupList.removeAll( QString() );

	return groupList;
}



QStringList MacOsUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	QStringList groupList;

	const auto strippedUsername = VeyonCore::stripDomain( username );

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("groups"), { strippedUsername } );
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8( getentProcess.readAll() ).split( QLatin1Char(' ') );
	for( const auto& group : groups )
	{
			groupList += group; // clazy:exclude=reserve-candidates
	}

	groupList.removeAll( QString() );

	return groupList;
}



bool MacOsUserFunctions::isAnyUserLoggedOn()
{
	QProcess whoProcess;
	whoProcess.start( QStringLiteral("who"), QStringList{} );
	whoProcess.waitForFinished( WhoProcessTimeout );

	if( whoProcess.exitCode() != 0 )
	{
		return false;
	}

	const auto lines = whoProcess.readAll().split( '\n' );
//	for( const auto& line : lines )
//	{
////		const auto user = QString::fromUtf8( line.split( "    ").value( 0 ) );
////		const auto login = QString::fromUtf8( line.split( '   ' ).value( 1 ).line.split( '  ' ).value( 0 ) );
////		if( !user.isEmpty() && login == "console" )
////		{
////			return true;
////		}
//	}

	return false;
}



QString MacOsUserFunctions::currentUser()
{
	QString username;

	const auto envUser = qgetenv( "USER" );

	struct passwd * pw_entry = nullptr;
	if( envUser.isEmpty() == false )
	{
		pw_entry = getpwnam( envUser.constData() );
	}

	if( pw_entry == nullptr )
	{
		pw_entry = getpwuid( getuid() );
	}

	if( pw_entry )
	{
		const auto shell = QString::fromUtf8( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
				shell.endsWith( QStringLiteral( "/true" ) ) ||
				shell.endsWith( QStringLiteral( "/null" ) ) ||
				shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			username = QString::fromUtf8( pw_entry->pw_name );
		}
	}

	if( username.isEmpty() )
	{
		return QString::fromUtf8( envUser );
	}

	return username;
}



bool MacOsUserFunctions::prepareLogon( const QString& username, const Password& password )
{
//	if( m_logonHelper.prepare( username, password ) )
//	{
//		LinuxCoreFunctions::restartDisplayManagers();
//		return true;
//	}
// Maybe sudo killall -HUP WindowServer
	return false;
}



bool MacOsUserFunctions::performLogon( const QString& username, const Password& password )
{


//	input.sendString( username );
//	input.pressAndReleaseKey( XK_Tab );
//	input.sendString( QString::fromUtf8( password.toByteArray() ) );
//	input.pressAndReleaseKey( XK_Return );

	return true;
}



void MacOsUserFunctions::logoff()
{
	MacOsCoreFunctions::runAppleScript(QStringLiteral("tell application \"System Events\" to log out"));
}



bool MacOsUserFunctions::authenticate( const QString& username, const Password& password )
{
//	QProcess p;
//	p.start( QStringLiteral( "veyon-auth-helper" ), QStringList{}, QProcess::ReadWrite | QProcess::Unbuffered );
//	if( p.waitForStarted() == false )
//	{
//		vCritical() << "failed to start VeyonAuthHelper";
//		return false;
//	}
//
//	const auto pamService = LinuxPlatformConfiguration( &VeyonCore::config() ).pamServiceName();
//
//	QDataStream ds( &p );
//	ds << VeyonCore::stripDomain( username ).toUtf8();
//	ds << password.toByteArray();
//	ds << pamService.toUtf8();
//
//	p.waitForFinished( AuthHelperTimeout );
//
//	if( p.state() != QProcess::NotRunning || p.exitCode() != 0 )
//	{
//		vCritical() << "VeyonAuthHelper failed:" << p.exitCode()
//					<< p.readAllStandardOutput().trimmed() << p.readAllStandardError().trimmed();
//		return false;
//	}

	vDebug() << "User authenticated successfully";
	return true;
}



uid_t MacOsUserFunctions::userIdFromName( const QString& username )
{
	const auto pw_entry = getpwnam( username.toUtf8().constData() );

	if( pw_entry )
	{
		return pw_entry->pw_uid;
	}

	return 0;
}
