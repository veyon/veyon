/*
 * WindowsServiceFunctions.cpp - implementation of WindowsServiceFunctions class
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

#include "WindowsServiceFunctions.h"
#include "WindowsService.h"
#include "WindowsServiceCore.h"


bool WindowsServiceFunctions::isRegistered( const QString& name )
{
	return WindowsService( name ).isRegistered();
}



bool WindowsServiceFunctions::isRunning( const QString& name )
{
	return WindowsService( name ).isRunning();
}



bool WindowsServiceFunctions::start( const QString& name )
{
	return WindowsService( name ).start();
}



bool WindowsServiceFunctions::stop( const QString& name )
{
	return WindowsService( name ).stop();
}



bool WindowsServiceFunctions::install( const QString& name, const QString& filePath, const QString& arguments,
                                       StartMode startMode, const QString& displayName )
{
	return WindowsService( name ).install( filePath, arguments, displayName ) &&
	        setStartMode( name, startMode );
}



bool WindowsServiceFunctions::uninstall( const QString& name )
{
	return WindowsService( name ).uninstall();
}



bool WindowsServiceFunctions::setFilePathAndArguments( const QString& name, const QString& filePath, const QString& arguments )
{
	return WindowsService( name ).setFilePathAndArguments( filePath, arguments );
}



bool WindowsServiceFunctions::setStartMode( const QString& name, PlatformServiceFunctions::StartMode startMode )
{
	return WindowsService( name ).setStartType( windowsServiceStartType( startMode ) );
}



bool WindowsServiceFunctions::runAsService( const QString& name, std::function<void(void)> serviceMain )
{
	WindowsServiceCore windowsServiceCore( name, serviceMain );
	return windowsServiceCore.runAsService();
}



void WindowsServiceFunctions::manageServerInstances()
{
	WindowsServiceCore::instance()->manageServerInstances();
}



int WindowsServiceFunctions::windowsServiceStartType( PlatformServiceFunctions::StartMode startMode )
{
	switch( startMode )
	{
	case StartModeAuto: return SERVICE_AUTO_START;
	case StartModeManual: return SERVICE_DEMAND_START;
	default: break;
	}

	return SERVICE_DISABLED;
}
