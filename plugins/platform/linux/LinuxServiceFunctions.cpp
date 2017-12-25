/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QProcess>

#include "LinuxServiceFunctions.h"

bool LinuxServiceFunctions::isRegistered(const QString &name)
{
	return false;
}

bool LinuxServiceFunctions::isRunning( const QString& name )
{
	return QProcess::execute( QStringLiteral("systemctl"), { QStringLiteral("status"), name } ) == 0;
}



bool LinuxServiceFunctions::start( const QString& name )
{
	return QProcess::execute( QStringLiteral("systemctl"), { QStringLiteral("start"), name } ) == 0;
}



bool LinuxServiceFunctions::stop( const QString& name )
{
	return QProcess::execute( QStringLiteral("systemctl"), { QStringLiteral("stop"), name } ) == 0;
}



bool LinuxServiceFunctions::install( const QString& name, const QString& filePath,
									 StartMode startMode, const QString& displayName )
{
	Q_UNUSED(name)
	Q_UNUSED(filePath)
	Q_UNUSED(startMode)
	Q_UNUSED(displayName)

	// TODO
	return false;
}



bool LinuxServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	// TODO
	return false;
}



bool LinuxServiceFunctions::setStartMode( const QString& name, PlatformServiceFunctions::StartMode startMode )
{
	Q_UNUSED(name)
	Q_UNUSED(startMode)

	// TODO
	return false;
}



bool LinuxServiceFunctions::runAsService( const QString& name, std::function<void(void)> serviceMain )
{
	Q_UNUSED(name);

	serviceMain();

	return true;
}



void LinuxServiceFunctions::manageServerInstances()
{
	QProcess::execute( VeyonCore::serverFilePath() );
}
