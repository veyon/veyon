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

#include "MacOsCoreFunctions.h"
#include "MacOsServiceCore.h"
#include "MacOsServiceFunctions.h"


QString MacOsServiceFunctions::veyonServiceName() const
{
	return QStringLiteral("veyon");
}



bool MacOsServiceFunctions::isRegistered( const QString& name )
{
	Q_UNUSED(name)

	vCritical() << "Querying service registration is not supported on this platform.";

	return false;
}



bool MacOsServiceFunctions::isRunning( const QString& name )
{
//	return MacOsCoreFunctions::systemctl( { QStringLiteral("status"), name } ) == 0;
	return false;
}



bool MacOsServiceFunctions::start( const QString& name )
{
	//return LinuxCoreFunctions::systemctl( { QStringLiteral("start"), name } ) == 0;
	return true;
}



bool MacOsServiceFunctions::stop( const QString& name )
{
	//return LinuxCoreFunctions::systemctl( { QStringLiteral("stop"), name } ) == 0;
	return true;
}



bool MacOsServiceFunctions::install( const QString& name, const QString& filePath,
									 StartMode startMode, const QString& displayName )
{
	Q_UNUSED(name)
	Q_UNUSED(filePath)
	Q_UNUSED(startMode)
	Q_UNUSED(displayName)

	vCritical() << "Registering services is not supported on this platform.";

	return false;
}



bool MacOsServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	vCritical() << "Unregistering services is not supported on this platform.";

	return false;
}



bool MacOsServiceFunctions::setStartMode( const QString& name, PlatformServiceFunctions::StartMode startMode )
{
//	if( startMode == StartMode::Auto )
//	{
//		return LinuxCoreFunctions::systemctl( { QStringLiteral("enable"), name } ) == 0;
//	}
//
//	return LinuxCoreFunctions::systemctl( { QStringLiteral("disable"), name } ) == 0;
	return true;
}



bool MacOsServiceFunctions::runAsService( const QString& name, const ServiceEntryPoint& serviceEntryPoint )
{
	Q_UNUSED(name)

	serviceEntryPoint();

	return true;
}



void MacOsServiceFunctions::manageServerInstances()
{
	MacOsServiceCore serviceCore;
	serviceCore.run();
}
