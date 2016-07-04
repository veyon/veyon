/*
 * QtSlaveLauncher.cpp - class Ipc::QtSlaveLauncher providing mechanisms for
 *                       launching a slave application via QProcess
 *
 * Copyright (c) 2010-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include "Ipc/QtSlaveLauncher.h"

#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "Logger.h"

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#define DEV_NULL "NUL"
#else
#define DEV_NULL "/dev/null"
#endif

namespace Ipc
{

QtSlaveLauncher::QtSlaveLauncher( const QString &applicationFilePath ) :
	SlaveLauncher( applicationFilePath ),
	m_processMutex( QMutex::Recursive ),
	m_process( NULL )
{
}



QtSlaveLauncher::~QtSlaveLauncher()
{
	// base class destructor calls stop()
}



void QtSlaveLauncher::start( const QStringList &arguments )
{
	stop();

	QMutexLocker l( &m_processMutex );

	m_process = new QProcess;

	// forward stdout/stderr from slave to master
	m_process->setProcessChannelMode( QProcess::ForwardedChannels );

#if QT_VERSION >= 0x050000
	QObject::connect( m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
					  m_process, &QProcess::deleteLater );
	QObject::connect( m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
					  this, &QtSlaveLauncher::finished );
#else
	connect( m_process, SIGNAL(finished(int)), m_process, SLOT(deleteLater()) );
	connect( m_process, SIGNAL(finished(int)), this, SIGNAL(finished()) );
#endif

#ifndef DEBUG
	m_process->start( applicationFilePath(), arguments );
#else
	qWarning() << applicationFilePath() << arguments;
#endif
}



void QtSlaveLauncher::stop()
{
	QMutexLocker l( &m_processMutex );

	// process still running?
	if( isRunning() )
	{
		// then register some logic for asynchronously stopping process after timeout
		QTimer* killTimer = new QTimer( m_process );
		connect( killTimer, SIGNAL(timeout()), m_process, SLOT(terminate()) );
		connect( killTimer, SIGNAL(timeout()), m_process, SLOT(kill()) );
		killTimer->start( 5000 );
	}
}



bool QtSlaveLauncher::isRunning()
{
	QMutexLocker l( &m_processMutex );

	// guarded pointer still valid?
	if( m_process )
	{
		// then process is running
		return true;
	}
	return false;
}


}
