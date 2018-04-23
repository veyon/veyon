/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include <QDBusReply>
#include <QEventLoop>
#include <QProcess>

#include <proc/readproc.h>

#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxServiceCore.h"


LinuxServiceCore::LinuxServiceCore( QObject* parent ) :
	QObject( parent ),
	m_loginManager( LinuxCoreFunctions::systemdLoginManager() )
{
	QDBusConnection::systemBus().connect( m_loginManager->service(), m_loginManager->path(), m_loginManager->interface(),
										 QStringLiteral("SessionNew"), this, SLOT(startServer(QString,QDBusObjectPath)) );
	QDBusConnection::systemBus().connect( m_loginManager->service(), m_loginManager->path(), m_loginManager->interface(),
										 QStringLiteral("SessionRemoved"), this, SLOT(stopServer(QString,QDBusObjectPath)) );
}



LinuxServiceCore::~LinuxServiceCore()
{

}



void LinuxServiceCore::run()
{
	const auto sessions = listSessions();

	for( const auto& s : sessions )
	{
		startServer( s, QDBusObjectPath( s ) );
	}

	QEventLoop eventLoop;
	eventLoop.exec();
}



void LinuxServiceCore::startServer( const QString& sessionId, const QDBusObjectPath& sessionObjectPath )
{
	Q_UNUSED( sessionId );

	const auto session = sessionObjectPath.path();

	qDebug() << getSessionDisplay( session );

	// do not start server for non-graphical sessions
	if( getSessionDisplay( session ).isEmpty() )
	{
		return;
	}

	const auto sessionLeader = getSessionLeaderPid( session );
	const auto sessionEnvironment = getSessionEnvironment( sessionLeader );

	if( sessionEnvironment.isEmpty() == false )
	{
		qInfo() << "Starting server for new session" << session;

		auto process = new QProcess( this );
		process->setProcessEnvironment( sessionEnvironment );
		process->start( VeyonCore::filesystem().serverFilePath() );

		m_serverProcesses[session] = process;
	}
}



void LinuxServiceCore::stopServer( const QString& sessionId, const QDBusObjectPath& sessionObjectPath )
{
	Q_UNUSED( sessionId );

	const auto session = sessionObjectPath.path();

	if( m_serverProcesses.contains( session ) )
	{
		qInfo() << "Stopping server for removed session" << session;

		auto process = m_serverProcesses[session];
		process->terminate();
		delete process;
		m_serverProcesses.remove( session );
	}
}



QStringList LinuxServiceCore::listSessions()
{
	QStringList sessions;

	const QDBusReply<QDBusArgument> reply = m_loginManager->call( QStringLiteral("ListSessions") );

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
	else
	{
		qCritical() << Q_FUNC_INFO << "Could not query sessions:" << reply.error().message();
	}

	return sessions;
}



QVariant LinuxServiceCore::getSessionProperty( const QString& session, const QString& property )
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
		qCritical() << "Could not query session property" << property << reply.error().message();
		return QVariant();
	}

	return reply.value().variant();
}



int LinuxServiceCore::getSessionLeaderPid( const QString& session )
{
	const auto leader = getSessionProperty( session, QStringLiteral("Leader") );

	if( leader.isNull() )
	{
		return -1;
	}

	return leader.toInt();
}



QString LinuxServiceCore::getSessionDisplay( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Display") ).toString();
}



QString LinuxServiceCore::getSessionId( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Id") ).toString();
}



QString LinuxServiceCore::getSessionSeat( const QString& session )
{
	return getSessionProperty( session, QStringLiteral("Seat") ).toString();
}



QProcessEnvironment LinuxServiceCore::getSessionEnvironment( int sessionLeaderPid )
{
	QProcessEnvironment sessionEnv;

	PROCTAB* proc = openproc( PROC_FILLSTATUS | PROC_FILLENV );
	proc_t* procInfo = nullptr;

	QList<int> ppids;

	while( ( procInfo = readproc( proc, nullptr ) ) )
	{
		if( ( procInfo->ppid == sessionLeaderPid || ppids.contains( procInfo->ppid ) ) &&
				procInfo->environ != nullptr )
		{
			for( int i = 0; procInfo->environ[i]; ++i )
			{
				const auto env = QString( procInfo->environ[i] ).split( '=' );
				sessionEnv.insert( env.first(), env.mid( 1 ).join( '=' ) );
			}

			ppids.append( procInfo->tid );
		}

		freeproc( procInfo );
	}

	closeproc( proc );

	return sessionEnv;
}
