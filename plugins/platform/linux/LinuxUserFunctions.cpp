/*
 * LinuxUserFunctions.cpp - implementation of LinuxUserFunctions class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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
#include <QRegularExpression>

#include "LinuxCoreFunctions.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxKeyboardInput.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"
#include "VeyonConfiguration.h"

#define XK_MISCELLANY

#include <X11/keysymdef.h>
#include <X11/Xlib.h>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>


QString LinuxUserFunctions::fullName( const QString& username )
{
	const auto pw_entry = getpwnam(username.toUtf8().constData());

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
	Q_UNUSED(queryDomainGroups)

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
	Q_UNUSED(queryDomainGroups)

	QStringList groupList;

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8( getentProcess.readAll() ).split( QLatin1Char('\n') );
	for( const auto& group : groups )
	{
		const auto groupComponents = group.split( QLatin1Char(':') );
		if( groupComponents.size() == 4 &&
			groupComponents.last().split( QLatin1Char(',') ).contains(username))
		{
			groupList += groupComponents.first(); // clazy:exclude=reserve-candidates
		}
	}

	groupList.removeAll( QString() );

	return groupList;
}



QString LinuxUserFunctions::userGroupSecurityIdentifier(const QString& groupName)
{
	const auto group = getgrnam(groupName.toUtf8().constData());
	if (group)
	{
		  return QString::number(group->gr_gid);
	}

	return {};
}



bool LinuxUserFunctions::isAnyUserLoggedOn()
{
	const auto sessions = LinuxSessionFunctions::listSessions();
	for( const auto& session : sessions )
	{
		if( LinuxSessionFunctions::isOpen( session ) &&
			LinuxSessionFunctions::isGraphical( session ) &&
			LinuxSessionFunctions::getSessionClass( session ) == LinuxSessionFunctions::Class::User )
		{
			return true;
		}
	}

	return false;
}



QString LinuxUserFunctions::currentUser()
{
	QString username;

	if (VeyonCore::component() != VeyonCore::Component::CLI && m_systemBus.isConnected())
	{
		const auto sessionPath = LinuxSessionFunctions::currentSessionPath(true);
		if (sessionPath.isEmpty() == false)
		{
			const auto sessionUserPath = LinuxSessionFunctions::getSessionUser(sessionPath);
			if (sessionUserPath.isEmpty() == false)
			{
				username = getUserProperty(sessionUserPath, QStringLiteral("Name")).toString();
				if (username.isEmpty() == false)
				{
					return username;
				}
			}
		}
	}

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



bool LinuxUserFunctions::prepareLogon( const QString& username, const Password& password )
{
	if( m_logonHelper.prepare( username, password ) )
	{
		LinuxCoreFunctions::restartDisplayManagers();
		return true;
	}

	return false;
}



bool LinuxUserFunctions::performLogon( const QString& username, const Password& password )
{
	LinuxKeyboardInput input;

	const LinuxPlatformConfiguration config(&VeyonCore::config());
	const auto textInputKeyPressInterval = config.userLoginTextInputKeyPressInterval();
	const auto controlKeyPressInterval = config.userLoginControlKeyPressInterval();

	auto sequence = config.userLoginKeySequence();

	if( sequence.isEmpty() == true )
	{
		sequence = QStringLiteral("%username%<Tab>%password%<Return>");
	}

	static const QRegularExpression keySequenceRX(QStringLiteral("(<[\\w\\d_]+>|%username%|%password%|[\\w\\d]+)"));
	auto matchIterator = keySequenceRX.globalMatch(sequence);
	if( matchIterator.hasNext() == false )
	{
		vCritical() << "invalid user login key sequence";
		return false;
	}

	QThread::msleep(config.userLoginInputStartDelay());

	while( matchIterator.hasNext() )
	{
		const auto token = matchIterator.next().captured(0);
		if( token == QStringLiteral("%username%") )
		{
			input.sendString(username, textInputKeyPressInterval);
		}
		else if( token == QStringLiteral("%password%") )
		{
			input.sendString(QString::fromUtf8(password.toByteArray()), textInputKeyPressInterval);
		}
		else if( token.startsWith( QLatin1Char('<') ) && token.endsWith( QLatin1Char('>') ) )
		{
			const auto keysymString = token.mid( 1, token.length() - 2 );
			const auto keysym = XStringToKeysym( keysymString.toLatin1().constData() );
			if( keysym != NoSymbol )
			{
				input.pressAndReleaseKey( keysym );
				QThread::msleep(controlKeyPressInterval);
			}
			else
			{
				vCritical() << "unresolved keysym" << keysymString;
				return false;
			}
		}
		else if( token.isEmpty() == false )
		{
			input.sendString(token, textInputKeyPressInterval);
		}
	}

	return true;
}



void LinuxUserFunctions::logoff()
{
	LinuxCoreFunctions::prepareSessionBusAccess();

	// logout via common session managers
	// logout via common session managers
	for( const auto& call : std::initializer_list<std::function<QDBusMessage()>>
		 {
			 []() {
				 return LinuxCoreFunctions::kdeSessionManager()
					 ->call( QStringLiteral("logout"),
							 LinuxDesktopIntegration::KDE::ShutdownConfirmNo,
							 LinuxDesktopIntegration::KDE::ShutdownTypeLogout,
							 LinuxDesktopIntegration::KDE::ShutdownModeForceNow );
			 },
			 []() {
				 return LinuxCoreFunctions::gnomeSessionManager()
					 ->call( QStringLiteral("Logout"),
							 LinuxDesktopIntegration::Gnome::GSM_MANAGER_LOGOUT_MODE_FORCE );
			 },
			 []() {
				 return LinuxCoreFunctions::mateSessionManager()
					 ->call( QStringLiteral("Logout"),
							 LinuxDesktopIntegration::Mate::GSM_LOGOUT_MODE_FORCE );
			 }
		 } )
	{
		// call successful?
		if( call().type() == QDBusMessage::ReplyMessage )
		{
			return;
		}
	}

	// Xfce logout
	QProcess::startDetached( QStringLiteral("xfce4-session-logout --logout"), {} );

	// LXDE logout
	QProcess::startDetached( QStringLiteral("kill -TERM %1").
							 arg( QProcessEnvironment::systemEnvironment().value( QStringLiteral("_LXSESSION_PID") ).toInt() ), {} );

	// terminate session via systemd
	LinuxCoreFunctions::systemdLoginManager()->asyncCall( QStringLiteral("TerminateSession"),
														  QProcessEnvironment::systemEnvironment().value( LinuxSessionFunctions::xdgSessionIdEnvVarName() ) );

	// close session via ConsoleKit as a last resort
	LinuxCoreFunctions::consoleKitManager()->asyncCall( QStringLiteral("CloseSession"),
														QProcessEnvironment::systemEnvironment().value( QStringLiteral("XDG_SESSION_COOKIE") ) );
}



bool LinuxUserFunctions::authenticate( const QString& username, const Password& password )
{
	QProcess p;
	p.start( QStringLiteral( "veyon-auth-helper" ), QStringList{}, QProcess::ReadWrite | QProcess::Unbuffered );
	if( p.waitForStarted() == false )
	{
		vCritical() << "failed to start VeyonAuthHelper";
		return false;
	}

	const auto pamService = LinuxPlatformConfiguration( &VeyonCore::config() ).pamServiceName();

	QDataStream ds( &p );
	ds << username.toUtf8();
	ds << password.toByteArray();
	ds << pamService.toUtf8();

	p.waitForFinished( AuthHelperTimeout );

	if( p.state() != QProcess::NotRunning || p.exitCode() != 0 )
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

	return -1;
}



gid_t LinuxUserFunctions::userGroupIdFromName( const QString& username )
{
	const auto pw_entry = getpwnam( username.toUtf8().constData() );

	if( pw_entry )
	{
		return pw_entry->pw_gid;
	}

	return -1;
}



QVariant LinuxUserFunctions::getUserProperty(const QString& userPath, const QString& property, bool logErrors)
{
	QDBusInterface loginManager(QStringLiteral("org.freedesktop.login1"),
								 userPath,
								 QStringLiteral("org.freedesktop.DBus.Properties"),
								 QDBusConnection::systemBus());

	if (loginManager.connection().isConnected() == false)
	{
		vDebug() << "system bus not connected";
		return {};
	}

	const QDBusReply<QDBusVariant> reply = loginManager.call(QStringLiteral("Get"),
															  QStringLiteral("org.freedesktop.login1.User"),
															  property);

	if( reply.isValid() == false )
	{
		if (logErrors)
		{
			vCritical() << "Could not query property" << property
						<< "of user" << userPath
						<< "error:" << reply.error().message();
		}
		return {};
	}

	return reply.value().variant();
}
