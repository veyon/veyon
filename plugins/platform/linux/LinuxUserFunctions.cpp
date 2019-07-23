/*
 * LinuxUserFunctions.cpp - implementation of LinuxUserFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
#include <QDBusReply>
#include <QProcess>

#include "LinuxCoreFunctions.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxUserFunctions.h"
#include "VeyonConfiguration.h"

#include <pwd.h>
#include <unistd.h>


QString LinuxUserFunctions::fullName( const QString& username )
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



QStringList LinuxUserFunctions::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8( getentProcess.readAll() ).split( QLatin1Char('\n') );

	groupList.reserve( groups.size() );

	for( const auto& group : groups )
	{
		groupList += group.split( QLatin1Char(':') ).first();
	}

	const QStringList ignoredGroups( {
		QStringLiteral("daemon"),
		QStringLiteral("bin"),
		QStringLiteral("tty"),
		QStringLiteral("disk"),
		QStringLiteral("lp"),
		QStringLiteral("mail"),
		QStringLiteral("news"),
		QStringLiteral("uucp"),
		QStringLiteral("man"),
		QStringLiteral("proxy"),
		QStringLiteral("kmem"),
		QStringLiteral("dialout"),
		QStringLiteral("fax"),
		QStringLiteral("voice"),
		QStringLiteral("cdrom"),
		QStringLiteral("tape"),
		QStringLiteral("audio"),
		QStringLiteral("dip"),
		QStringLiteral("www-data"),
		QStringLiteral("backup"),
		QStringLiteral("list"),
		QStringLiteral("irc"),
		QStringLiteral("src"),
		QStringLiteral("gnats"),
		QStringLiteral("shadow"),
		QStringLiteral("utmp"),
		QStringLiteral("video"),
		QStringLiteral("sasl"),
		QStringLiteral("plugdev"),
		QStringLiteral("games"),
		QStringLiteral("nogroup"),
		QStringLiteral("libuuid"),
		QStringLiteral("syslog"),
		QStringLiteral("fuse"),
		QStringLiteral("lpadmin"),
		QStringLiteral("ssl-cert"),
		QStringLiteral("messagebus"),
		QStringLiteral("crontab"),
		QStringLiteral("mlocate"),
		QStringLiteral("avahi-autoipd"),
		QStringLiteral("netdev"),
		QStringLiteral("saned"),
		QStringLiteral("sambashare"),
		QStringLiteral("haldaemon"),
		QStringLiteral("polkituser"),
		QStringLiteral("mysql"),
		QStringLiteral("avahi"),
		QStringLiteral("klog"),
		QStringLiteral("floppy"),
		QStringLiteral("oprofile"),
		QStringLiteral("netdev"),
		QStringLiteral("dirmngr"),
		QStringLiteral("vboxusers"),
		QStringLiteral("bluetooth"),
		QStringLiteral("colord"),
		QStringLiteral("libvirtd"),
		QStringLiteral("nm-openvpn"),
		QStringLiteral("input"),
		QStringLiteral("kvm"),
		QStringLiteral("pulse"),
		QStringLiteral("pulse-access"),
		QStringLiteral("rtkit"),
		QStringLiteral("scanner"),
		QStringLiteral("sddm"),
		QStringLiteral("systemd-bus-proxy"),
		QStringLiteral("systemd-journal"),
		QStringLiteral("systemd-network"),
		QStringLiteral("systemd-resolve"),
		QStringLiteral("systemd-timesync"),
		QStringLiteral("utempter"),
		QStringLiteral("uuidd"),
							   } );

	for( const auto& ignoredGroup : ignoredGroups )
	{
		groupList.removeAll( ignoredGroup );
	}

	// remove all empty entries
	groupList.removeAll( QString() );

	return groupList;
}



QStringList LinuxUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	const auto strippedUsername = VeyonCore::stripDomain( username );

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8( getentProcess.readAll() ).split( QLatin1Char('\n') );
	for( const auto& group : groups )
	{
		const auto groupComponents = group.split( QLatin1Char(':') );
		if( groupComponents.size() == 4 &&
				groupComponents.last().split( QLatin1Char(',') ).contains( strippedUsername ) )
		{
			groupList += groupComponents.first(); // clazy:exclude=reserve-candidates
		}
	}

	groupList.removeAll( QString() );

	return groupList;
}



bool LinuxUserFunctions::isAnyUserLoggedOn()
{
	QProcess whoProcess;
	whoProcess.start( QStringLiteral("who") );
	whoProcess.waitForFinished( WhoProcessTimeout );

	if( whoProcess.exitCode() != 0 )
	{
		return false;
	}

	const auto displayManagerUsers = LinuxPlatformConfiguration( &VeyonCore::config() ).displayManagerUsers().
			split( QLatin1Char(',') );

	const auto lines = whoProcess.readAll().split( '\n' );
	for( const auto& line : lines )
	{
		const auto user = QString::fromUtf8( line.split( ' ' ).value( 0 ) );
		if( user.isEmpty() == false &&
			displayManagerUsers.contains( user ) == false )
		{
			return true;
		}
	}

	return false;
}



QString LinuxUserFunctions::currentUser()
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



bool LinuxUserFunctions::logon( const QString& username, const QString& password )
{
	Q_UNUSED(username);
	Q_UNUSED(password);

	// TODO

	return false;
}



void LinuxUserFunctions::logoff()
{
	// logout via common session managers
	LinuxCoreFunctions::kdeSessionManager()->asyncCall( QStringLiteral("logout"),
														static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownConfirmNo ),
														static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownTypeLogout ),
														static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownModeForceNow ) );
	LinuxCoreFunctions::gnomeSessionManager()->asyncCall( QStringLiteral("Logout"),
														  static_cast<int>( LinuxDesktopIntegration::Gnome::GSM_MANAGER_LOGOUT_MODE_FORCE ) );
	LinuxCoreFunctions::mateSessionManager()->asyncCall( QStringLiteral("Logout"),
														 static_cast<int>( LinuxDesktopIntegration::Mate::GSM_LOGOUT_MODE_FORCE ) );

	// Xfce logout
	QProcess::startDetached( QStringLiteral("xfce4-session-logout --logout") );

	// LXDE logout
	QProcess::startDetached( QStringLiteral("kill -TERM %1").
							 arg( QProcessEnvironment::systemEnvironment().value( QStringLiteral("_LXSESSION_PID") ).toInt() ) );

	// terminate session via systemd
	LinuxCoreFunctions::systemdLoginManager()->asyncCall( QStringLiteral("TerminateSession"),
														  QProcessEnvironment::systemEnvironment().value( QStringLiteral("XDG_SESSION_ID") ) );

	// close session via ConsoleKit as a last resort
	LinuxCoreFunctions::consoleKitManager()->asyncCall( QStringLiteral("CloseSession"),
														QProcessEnvironment::systemEnvironment().value( QStringLiteral("XDG_SESSION_COOKIE") ) );
}



bool LinuxUserFunctions::authenticate( const QString& username, const QString& password )
{
	QProcess p;
	p.start( QStringLiteral( "veyon-auth-helper" ), QProcess::ReadWrite | QProcess::Unbuffered );
	if( p.waitForStarted() == false )
	{
		vCritical() << "failed to start VeyonAuthHelper";
		return false;
	}

	const auto pamService = LinuxPlatformConfiguration( &VeyonCore::config() ).pamServiceName();

	QDataStream ds( &p );
	ds << VeyonCore::stripDomain( username );
	ds << password;
	ds << pamService;

	p.waitForFinished();

	if( p.exitCode() != 0 )
	{
		vCritical() << "VeyonAuthHelper failed:" << p.exitCode()
					<< p.readAllStandardOutput().trimmed() << p.readAllStandardError().trimmed();
		return false;
	}

	vDebug() << "User authenticated successfully";
	return true;
}



uid_t LinuxUserFunctions::userIdFromName( const QString& username )
{
	const auto pw_entry = getpwnam( username.toUtf8().constData() );

	if( pw_entry )
	{
		return pw_entry->pw_uid;
	}

	return 0;
}
