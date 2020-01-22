/*
 * LinuxServiceCore.h - declaration of LinuxServiceCore class
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

#pragma once

#include <QProcessEnvironment>

#include "LinuxCoreFunctions.h"
#include "PlatformServiceCore.h"
#include "ServiceDataManager.h"

// clazy:excludeall=copyable-polymorphic

class LinuxServiceCore : public QObject, PlatformServiceCore
{
	Q_OBJECT
public:
	explicit LinuxServiceCore( QObject* parent = nullptr );
	~LinuxServiceCore() override;

	void run();

private slots:
	void startServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath );
	void stopServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath );

private:
	static constexpr auto LoginManagerReconnectInterval = 3000;
	static constexpr auto ServerShutdownTimeout = 1000;
	static constexpr auto ServerTerminateTimeout = 3000;
	static constexpr auto ServerKillTimeout = 10000;
	static constexpr auto ServerWaitSleepInterval = 100;
	static constexpr auto SessionEnvironmentProbingInterval = 1000;
	static constexpr auto SessionUptimeSecondsMinimum = 3;
	static constexpr auto SessionUptimeProbingInterval = 1000;

	using LoginDBusSession = struct {
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

	void connectToLoginManager();
	void stopServer( const QString& sessionPath );
	void stopAllServers();

	QStringList listSessions();

	static QVariant getSessionProperty( const QString& session, const QString& property );

	static int getSessionLeaderPid( const QString& session );
	static qint64 getSessionUptimeSeconds( const QString& session );
	static QString getSessionType( const QString& session );
	static QString getSessionDisplay( const QString& session );
	static QString getSessionId( const QString& session );
	static LoginDBusSessionSeat getSessionSeat( const QString& session );

	static QProcessEnvironment getSessionEnvironment( int sessionLeaderPid );

	LinuxCoreFunctions::DBusInterfacePointer m_loginManager;
	QMap<QString, QProcess *> m_serverProcesses;

	ServiceDataManager m_dataManager;

};
