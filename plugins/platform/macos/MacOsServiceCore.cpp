/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
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

#include <QEventLoop>
#include <QProcess>
#include <QTimer>

#include <csignal>
#include <sys/types.h>

#include "Filesystem.h"
#include "MacOsCoreFunctions.h"
#include "MacOsServiceCore.h"
#include "MacOsSessionFunctions.h"
#include "ProcessHelper.h"


MacOsServiceCore::MacOsServiceCore( QObject* parent ) :
	QObject( parent )
{

}



MacOsServiceCore::~MacOsServiceCore()
{
	stopServer();
}



void MacOsServiceCore::run()
{
	vInfo() << "SeviceInit";
	startServer();

	QEventLoop eventLoop;
	eventLoop.exec();
}



void MacOsServiceCore::startServer()
{
	vInfo() << "SeviceInit";
	if( m_sessionManager.multiSession() == false && m_serverProcess->pid() >0 )
	{
		const auto veyonSessionId = m_sessionManager.openSession( QStringLiteral("1") );

		vInfo() << "Starting server" << veyonSessionId;

		m_serverProcess = new QProcess( this );
		m_serverProcess->start( VeyonCore::filesystem().serverFilePath(), QStringList{} );
	}else{
		vCritical() << "MultiSession not supported as of yet";
	}
}



void MacOsServiceCore::stopServer( )
{
	if( m_serverProcess->pid() > 0 )
	{
		m_serverProcess->terminate();
	}
}





