/*
 * ProcessHelper.cpp - implementation of ProcessHelper
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>
#include <QThread>

#include "ProcessHelper.h"


ProcessHelper::ProcessHelper( const QString& program, const QStringList& arguments ) :
	m_process()
{
	m_process.start( program, arguments );
}



int ProcessHelper::run()
{
	if( m_process.waitForStarted() )
	{
		m_process.waitForFinished();
		return m_process.exitCode();
	}

	return -1;
}



QByteArray ProcessHelper::runAndReadAll()
{
	if( m_process.waitForStarted() )
	{
		m_process.waitForFinished();
		return m_process.readAll();
	}

	return QByteArray();
}



bool ProcessHelper::waitForProcess( QProcess* process, int timeout, int sleepInterval )
{
	QElapsedTimer timeoutTimer;
	timeoutTimer.start();

	while( process->state() != QProcess::NotRunning )
	{
		if( timeoutTimer.elapsed() >= timeout )
		{
			return false;
		}

		QThread::msleep( static_cast<unsigned long>( sleepInterval ) );
	}

	return true;
}
