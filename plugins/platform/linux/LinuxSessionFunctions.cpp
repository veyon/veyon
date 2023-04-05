/*
 * LinuxSessionFunctions.cpp - implementation of LinuxSessionFunctions class
 *
 * Copyright (c) 2020-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QDateTime>
#include <QDBusReply>
#include <QHostInfo>
#include <QProcessEnvironment>

#ifdef HAVE_LIBPROCPS
#include <proc/readproc.h>
#endif

#include "LinuxCoreFunctions.h"
#include "LinuxSessionFunctions.h"
#include "PlatformSessionManager.h"


LinuxSessionFunctions::SessionId LinuxSessionFunctions::currentSessionId()
{
	return PlatformSessionManager::resolveSessionId( currentSessionPath() );
}



LinuxSessionFunctions::SessionUptime LinuxSessionFunctions::currentSessionUptime() const
{
	return getSessionUptimeSeconds(currentSessionPath());
}



QString LinuxSessionFunctions::currentSessionClientAddress() const
{
	return getSessionProperty(currentSessionPath(), QStringLiteral("RemoteHost")).toString();
}



QString LinuxSessionFunctions::currentSessionClientName() const
{
	return currentSessionClientAddress();
}



QString LinuxSessionFunctions::currentSessionHostName() const
{
	return QHostInfo::localHostName();
}



QString LinuxSessionFunctions::currentSessionType() const
{
	const auto env = QProcessEnvironment::systemEnvironment();

	if( env.contains( QStringLiteral("WAYLAND_DISPLAY") ) )
	{
		return QStringLiteral("wayland");
	}
	else if( env.contains( QStringLiteral("DISPLAY") ) )
	{
		return QStringLiteral("x11");
	}

	return getSessionProperty( currentSessionPath(), QStringLiteral("Type") ).toString();
}



bool LinuxSessionFunctions::currentSessionHasUser() const
{
	return getSessionClass( currentSessionPath() ) == Class::User;
}



bool LinuxSessionFunctions::currentSessionIsRemote() const
{
	return isRemote( currentSessionPath() );
}



QStringList LinuxSessionFunctions::listSessions()
{
	QStringList sessions;

	const QDBusReply<QDBusArgument> reply = LinuxCoreFunctions::systemdLoginManager()->call( QStringLiteral("ListSessions") );

	if( reply.isValid() )
	{
		const auto data = reply.value();

		data.beginArray();
		while( data.atEnd() == false )
		{
			LoginDBusSession session;

			data.beginStructure();
			data >> session.id >> session.uid >> session.name >> session.seatId >> session.path;
			data.endStructure();

			sessions.append( session.path.path() );
		}
		return sessions;
	}

	vCritical() << "Could not query sessions:" << reply.error().message();

	return sessions;
}



QVariant LinuxSessionFunctions::getSessionProperty(const QString& session, const QString& property, bool logErrors)
{
	QDBusInterface loginManager( QStringLiteral("org.freedesktop.login1"),
								 session,
								 QStringLiteral("org.freedesktop.DBus.Properties"),
								 QDBusConnection::systemBus() );

	const QDBusReply<QDBusVariant> reply = loginManager.call( QStringLiteral("Get"),
															  QStringLiteral("org.freedesktop.login1.Session"),
															  property );

	if( reply.isValid() == false )
	{
		if (logErrors)
		{
			vCritical() << "Could not query property" << property
						<< "of session" << session
						<< "error:" << reply.error().message();
		}
		return {};
	}

	return reply.value().variant();
}



int LinuxSessionFunctions::getSessionLeaderPid( const QString& session )
{
	const auto leader = getSessionProperty( session, QStringLiteral("Leader") );

	if( leader.isNull() )
	{
		return -1;
	}

	return leader.toInt();
}



LinuxSessionFunctions::SessionUptime LinuxSessionFunctions::getSessionUptimeSeconds( const QString& session )
{
	const auto sessionUptimeUsec = getSessionProperty( session, QStringLiteral("Timestamp") );

	if( sessionUptimeUsec.isNull() )
	{
		return -1;
	}

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
	const auto currentTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
#else
	const auto currentTimestamp = QDateTime::currentSecsSinceEpoch();
#endif

	return SessionUptime(currentTimestamp - sessionUptimeUsec.toLongLong() / (1000 * 1000));
}



LinuxSessionFunctions::Class LinuxSessionFunctions::getSessionClass( const QString& session )
{
	auto sessionClass = getSessionProperty(session, QStringLiteral("Class")).toString();
	if (sessionClass.isEmpty() && session == currentSessionPath())
	{
		sessionClass = QProcessEnvironment::systemEnvironment().value(LinuxSessionFunctions::xdgSessionClassEnvVarName());
	}

	if( sessionClass == QLatin1String("user") )
	{
		return Class::User;
	}
	else if( sessionClass == QLatin1String("greeter") )
	{
		return Class::Greeter;
	}
	else if( sessionClass == QLatin1String("lock-screen") )
	{
		return Class::LockScreen;
	}

	return Class::Unknown;
}



LinuxSessionFunctions::Type LinuxSessionFunctions::getSessionType( const QString& session )
{
	const auto type = getSessionProperty( session, QStringLiteral("Type") ).toString();
	if( type == QLatin1String("tty") )
	{
		return Type::TTY;
	}
	else if( type == QLatin1String("x11") )
	{
		return Type::X11;
	}
	else if( type == QLatin1String("mir") )
	{
		return Type::Mir;
	}
	else if( type == QLatin1String("wayland") )
	{
		return Type::Wayland;
	}
	else if (type == QLatin1String("unspecified"))
	{
		return Type::Unspecified;
	}

	if( type.isEmpty() == false )
	{
		vWarning() << "unknown session type" << type;
	}

	return Type::Unspecified;
}



QString LinuxSessionFunctions::getSessionId(const QString& session, bool logErrors)
{
	return getSessionProperty(session, QStringLiteral("Id"), logErrors).toString();
}



QString LinuxSessionFunctions::getSessionUser(const QString& session)
{
	quint32 uid{0};
	QString userObjectPath;

	const auto reply = getSessionProperty(session, QStringLiteral("User"));
	if (reply.isValid())
	{
		const auto replyData = reply.value<QDBusArgument>();
		replyData.beginStructure();
		replyData >> uid >> userObjectPath;
		replyData.endStructure();

		return userObjectPath;
	}

	return {};
}



LinuxSessionFunctions::State LinuxSessionFunctions::getSessionState( const QString& session )
{
	static const QMap<QString, State> stateMap{
		{ QStringLiteral("offline"), State::Offline },
		{ QStringLiteral("lingering"), State::Lingering },
		{ QStringLiteral("online"), State::Online },
		{ QStringLiteral("active"), State::Active },
		{ QStringLiteral("opening"), State::Opening },
		{ QStringLiteral("closing"), State::Closing }
	};

	const auto stateString = getSessionProperty( session, QStringLiteral("State") ).toString();
	const auto state = stateMap.value( stateString, State::Unknown );
	if( state == State::Unknown && stateString.isEmpty() == false )
	{
		vDebug() << stateString;
	}

	return state;
}



LinuxSessionFunctions::LoginDBusSessionSeat LinuxSessionFunctions::getSessionSeat( const QString& session )
{
	const auto seatArgument = getSessionProperty( session, QStringLiteral("Seat") ).value<QDBusArgument>();

	LoginDBusSessionSeat seat;
	seatArgument.beginStructure();
	seatArgument >> seat.id;
	seatArgument >> seat.path;
	seatArgument.endStructure();

	return seat;
}



QProcessEnvironment LinuxSessionFunctions::getSessionEnvironment( int sessionLeaderPid )
{
	QProcessEnvironment sessionEnv;

#ifdef HAVE_LIBPROCPS
	LinuxCoreFunctions::forEachChildProcess(
		[&sessionEnv]( proc_t* procInfo ) {
			if( procInfo->environ != nullptr )
			{
				for( int i = 0; procInfo->environ[i]; ++i )
				{
					const auto env = QString::fromUtf8( procInfo->environ[i] );
					const auto separatorPos = env.indexOf( QLatin1Char('=') );
					if( separatorPos > 0 )
					{
						sessionEnv.insert( env.left( separatorPos ), env.mid( separatorPos+1 ) );
					}
				}

				return true;
			}

			return false;
		},
		sessionLeaderPid, PROC_FILLENV, true );
#elif defined(HAVE_LIBPROC2)
	LinuxCoreFunctions::forEachChildProcess([&sessionEnv](const pids_stack* stack, const pids_info* info)
	{
		Q_UNUSED(info)
		static constexpr auto EnvironItemIndex = 2;
		const auto environ = PIDS_VAL(EnvironItemIndex, strv, stack, info);

		if (environ != nullptr)
		{
			for (int i = 0; environ[i]; ++i)
			{
				const auto env = QString::fromUtf8(environ[i]);
				const auto separatorPos = env.indexOf(QLatin1Char('='));
				if (separatorPos > 0)
				{
					sessionEnv.insert(env.left(separatorPos), env.mid(separatorPos+1));
				}
			}

			return true;
		}

		return false;
	},
	sessionLeaderPid, {PIDS_ENVIRON_V}, true);
#endif

	return sessionEnv;
}



QString LinuxSessionFunctions::currentSessionPath()
{
	const auto xdgSessionPath = QProcessEnvironment::systemEnvironment().value( sessionPathEnvVarName() );
	if (xdgSessionPath.isEmpty() == false)
	{
		return xdgSessionPath;
	}

	const auto sessionAuto = QStringLiteral("/org/freedesktop/login1/session/auto");
	const auto sessionSelf = QStringLiteral("/org/freedesktop/login1/session/self"); // systemd < 243
	static QString sessionPathFromXdgSessionId;

	static bool hasSessionAuto = false;
	static bool hasSessionSelf = false;

	if (hasSessionAuto)
	{
		return sessionAuto;
	}

	if (hasSessionSelf)
	{
		return sessionSelf;
	}

	if (sessionPathFromXdgSessionId.isEmpty() == false)
	{
		return sessionPathFromXdgSessionId;
	}

	if (getSessionId(sessionAuto, false).isNull() == false)
	{
		hasSessionAuto = true;
		return sessionAuto;
	}

	if (getSessionId(sessionSelf, false).isNull() == false)
	{
		hasSessionSelf = true;
		return sessionSelf;
	}

	const auto xdgSessionId = QProcessEnvironment::systemEnvironment().value(xdgSessionIdEnvVarName());
	if (xdgSessionId.isEmpty() == false)
	{
		const QDBusReply<QDBusObjectPath> reply = LinuxCoreFunctions::systemdLoginManager()->call(
			QDBus::Block, QStringLiteral("GetSession"), xdgSessionId);

		if (reply.isValid())
		{
			sessionPathFromXdgSessionId = reply.value().path();
			return sessionPathFromXdgSessionId;
		}
	}

	vWarning() << "could not determine dbus object path of current session – please make sure systemd is "
				  "up to date and/or the environment variable" << xdgSessionIdEnvVarName() << "is set";
	return {};
}



bool LinuxSessionFunctions::isOpen( const QString& session )
{
	const auto state = getSessionState( session );

	return state == State::Active ||
		   state == State::Online ||
		   state == State::Opening;
}


bool LinuxSessionFunctions::isGraphical( const QString& session )
{
	const auto type = getSessionType( session );

	return type == Type::X11 ||
		   type == Type::Wayland ||
		   type == Type::Mir;
}



bool LinuxSessionFunctions::isRemote( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Remote") ).toBool();
}
