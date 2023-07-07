/*
 * LinuxSessionFunctions.h - declaration of LinuxSessionFunctions class
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

#pragma once

#include <QDBusObjectPath>
#include <QProcessEnvironment>

#include "PlatformSessionFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxSessionFunctions : public PlatformSessionFunctions
{
	Q_GADGET
public:
	using LoginDBusSession = struct LoginDBusSession {
		QString id;
		quint32 uid{0};
		QString name;
		QString seatId;
		QDBusObjectPath path;
	} ;

	using LoginDBusSessionSeat = struct {
		QString id;
		QString path;
	} ;

	enum class State
	{
		Unknown,
		Offline,
		Lingering,
		Online,
		Active,
		Opening,
		Closing
	};
	Q_ENUM(State)

	enum class Class
	{
		Unknown,
		User,
		Greeter,
		LockScreen
	};
	Q_ENUM(Class)

	enum class Type
	{
		Unspecified,
		TTY,
		X11,
		Mir,
		Wayland
	};
	Q_ENUM(Type)

	SessionId currentSessionId() override;

	SessionUptime currentSessionUptime() const override;
	QString currentSessionClientAddress() const override;
	QString currentSessionClientName() const override;
	QString currentSessionHostName() const override;

	QString currentSessionType() const override;
	bool currentSessionHasUser() const override;

	static QStringList listSessions();

	static QVariant getSessionProperty(const QString& session, const QString& property, bool logErrors = true);

	static int getSessionLeaderPid( const QString& session );
	static SessionUptime getSessionUptimeSeconds( const QString& session );
	static Class getSessionClass( const QString& session );
	static Type getSessionType( const QString& session );
	static QString getSessionId(const QString& session, bool logErrors = true);
	static QString getSessionUser( const QString& session );
	static State getSessionState( const QString& session );
	static LoginDBusSessionSeat getSessionSeat( const QString& session );

	static QProcessEnvironment getSessionEnvironment( int sessionLeaderPid );

	static QString currentSessionPath(bool ignoreErrors = false);

	static QString kdeSessionVersionEnvVarName()
	{
		return QStringLiteral("KDE_SESSION_VERSION");
	}

	static QString xdgCurrentDesktopEnvVarName()
	{
		return QStringLiteral("XDG_CURRENT_DESKTOP");
	}

	static QString xdgSessionIdEnvVarName()
	{
		return QStringLiteral("XDG_SESSION_ID");
	}

	static QString xdgSessionClassEnvVarName()
	{
		return QStringLiteral("XDG_SESSION_CLASS");
	}

	static QString sessionPathEnvVarName()
	{
		return QStringLiteral("VEYON_SESSION_PATH");
	}

	static bool isOpen( const QString& session );
	static bool isGraphical( const QString& session );

};
