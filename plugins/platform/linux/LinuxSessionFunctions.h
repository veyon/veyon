// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusObjectPath>
#include <QProcessEnvironment>

#include "PlatformSessionFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxSessionFunctions : public PlatformSessionFunctions
{
	Q_GADGET
public:
	using LoginDBusSession = struct LoginDBusSession
	{
		QString id;
		quint32 uid{0};
		QString name;
		QString seatId;
		QDBusObjectPath path;
	};

	using LoginDBusSessionSeat = struct
	{
		QString id;
		QString path;
	};

	enum class State {
		Unknown,
		Offline,
		Lingering,
		Online,
		Active,
		Opening,
		Closing
	};
	Q_ENUM(State)

	enum class Class {
		Unknown,
		User,
		Greeter,
		LockScreen
	};
	Q_ENUM(Class)

	enum class Type {
		Unspecified,
		TTY,
		X11,
		Mir,
		Wayland
	};
	Q_ENUM(Type)

	enum class DesktopEnvironment {
		Generic,
		KDE,
		GNOME,
		Xfce,
		LXQt,
		LXDE,
		MATE,
	};
	Q_ENUM(DesktopEnvironment)

	SessionId currentSessionId() override;

	SessionUptime currentSessionUptime() const override;
	QString currentSessionClientAddress() const override;
	QString currentSessionClientName() const override;
	QString currentSessionHostName() const override;

	QString currentSessionType() const override;
	bool currentSessionHasUser() const override;

	EnvironmentVariables currentSessionEnvironmentVariables() const override;
	QVariant querySettingsValueInCurrentSession(const QString& key) const override;

	static QStringList listSessions();

	static QVariant getSessionProperty(const QString& session, const QString& property, bool logErrors = true);

	static int getSessionLeaderPid(const QString& session);
	static SessionUptime getSessionUptimeSeconds(const QString& session);
	static Class getSessionClass(const QString& session);
	static Type getSessionType(const QString& session);
	static DesktopEnvironment getSessionDesktopEnvironment(const QString& session);
	static QString getSessionId(const QString& session, bool logErrors = true);
	static QString getSessionUser(const QString& session);
	static State getSessionState(const QString& session);
	static LoginDBusSessionSeat getSessionSeat(const QString& session);

	static QProcessEnvironment getSessionEnvironment(int sessionLeaderPid);

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

	static bool isOpen(const QString& session);
	static bool isGraphical(const QString& session);
};
