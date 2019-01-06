/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "LinuxServiceCore.h"
#include "LinuxServiceFunctions.h"


QString LinuxServiceFunctions::veyonServiceName() const
{
	return QStringLiteral("veyon-service");
}



bool LinuxServiceFunctions::isRegistered( const QString& name )
{
	Q_UNUSED(name);

	qCritical( "Querying service registration is not supported on this platform.");

	return false;
}



bool LinuxServiceFunctions::isRunning( const QString& name )
{
	return systemctl( { QStringLiteral("status"), name } ) == 0;
}



bool LinuxServiceFunctions::start( const QString& name )
{
	return systemctl( { QStringLiteral("start"), name } ) == 0;
}



bool LinuxServiceFunctions::stop( const QString& name )
{
	return systemctl( { QStringLiteral("stop"), name } ) == 0;
}



bool LinuxServiceFunctions::install( const QString& name, const QString& filePath,
									 StartMode startMode, const QString& displayName )
{
	Q_UNUSED(name)
	Q_UNUSED(filePath)
	Q_UNUSED(startMode)
	Q_UNUSED(displayName)

	qCritical( "Registering services is not supported on this platform.");

	return false;
}



bool LinuxServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	qCritical( "Unregistering services is not supported on this platform.");

	return false;
}



bool LinuxServiceFunctions::setStartMode( const QString& name, PlatformServiceFunctions::StartMode startMode )
{
	if( startMode == StartModeAuto )
	{
		return systemctl( { QStringLiteral("enable"), name } ) == 0;
	}

	return systemctl( { QStringLiteral("disable"), name } ) == 0;
}



bool LinuxServiceFunctions::runAsService( const QString& name, const std::function<void(void)>& serviceMain )
{
	Q_UNUSED(name);

	serviceMain();

	return true;
}



void LinuxServiceFunctions::manageServerInstances()
{
	LinuxServiceCore serviceCore;
	serviceCore.run();
}



int LinuxServiceFunctions::systemctl( const QStringList& arguments )
{
	return QProcess::execute( QStringLiteral("systemctl"),
							  QStringList( { QStringLiteral("--no-pager"), QStringLiteral("-q") } ) + arguments );
}
