/*
 * LinuxSessionFunctions.cpp - implementation of LinuxSessionFunctions class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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
#include <QProcessEnvironment>



#include "MacOsSessionFunctions.h"
#include "PlatformSessionManager.h"


MacOsSessionFunctions::SessionId MacOsSessionFunctions::currentSessionId()
{
	return PlatformSessionManager::resolveSessionId( getSessionId( currentSessionPath() ) );
}



QVariant MacOsSessionFunctions::getSessionProperty( const QString& session, const QString& property )
{
//	QDBusInterface loginManager( QStringLiteral("org.freedesktop.login1"),
//								 session,
//								 QStringLiteral("org.freedesktop.DBus.Properties"),
//								 QDBusConnection::systemBus() );
//
//	const QDBusReply<QDBusVariant> reply = loginManager.call( QStringLiteral("Get"),
//															  QStringLiteral("org.freedesktop.login1.Session"),
//															  property );
//
//	if( reply.isValid() == false )
//	{
//		vCritical() << "Could not query session property" << property << reply.error().message();
//		return {};
//	}

	return QVariant();
}



int MacOsSessionFunctions::getSessionLeaderPid( const QString& session )
{
	const auto leader = getSessionProperty( session, QStringLiteral("Leader") );

	if( leader.isNull() )
	{
		return -1;
	}

	return leader.toInt();
}



qint64 MacOsSessionFunctions::getSessionUptimeSeconds( const QString& session )
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

	return currentTimestamp - qint64( sessionUptimeUsec.toLongLong() / ( 1000 * 1000 ) );
}



QString MacOsSessionFunctions::getSessionType( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Type") ).toString();
}



QString MacOsSessionFunctions::getSessionId( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Id") ).toString();
}



MacOsSessionFunctions::State MacOsSessionFunctions::getSessionState( const QString& session )
{
	static const QMap<QString, State> stateMap{
		{ QStringLiteral("offline"), State::Offline },
		{ QStringLiteral("lingering"), State::Lingering },
		{ QStringLiteral("online"), State::Online },
		{ QStringLiteral("active"), State::Active },
		{ QStringLiteral("opening"), State::Opening },
		{ QStringLiteral("closing"), State::Closing },
	};

	const auto stateString = getSessionProperty( session, QStringLiteral("State") ).toString();
	const auto state = stateMap.value( stateString, State::Unknown );
	if( state == State::Unknown )
	{
		vDebug() << stateString;
	}

	return state;
}


QProcessEnvironment MacOsSessionFunctions::getSessionEnvironment( int sessionLeaderPid )
{
	QProcessEnvironment sessionEnv;

	return sessionEnv;
}



QString MacOsSessionFunctions::currentSessionType() const
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

	return getSessionType( currentSessionPath() );
}



QString MacOsSessionFunctions::currentSessionPath()
{

	return QStringLiteral("/org/freedesktop/login1/session/%1");
}
