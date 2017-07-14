/*
 * ServiceControlPlugin.cpp - implementation of ServiceControlPlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "ServiceControlPlugin.h"
#include "ServiceControl.h"

ServiceControlPlugin::ServiceControlPlugin() :
	m_commands( {
{ "register", tr( "Register Veyon Service" ) },
{ "unregister", tr( "Unregister Veyon Service" ) },
{ "start", tr( "Start Veyon Service" ) },
{ "stop", tr( "Stop Veyon Service" ) },
{ "restart", tr( "Restart Veyon Service" ) },
{ "status", tr( "Query status of Veyon Service" ) },
				} ),
	m_serviceControl( nullptr )
{
}



ServiceControlPlugin::~ServiceControlPlugin()
{
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::runCommand( const QStringList& arguments )
{
	// all commands are handled as slots so if we land here an unsupported command has been called
	return InvalidCommand;
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_register( const QStringList& arguments )
{
	m_serviceControl.registerService();

	return m_serviceControl.isServiceRegistered() ? Successful : Failed;
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_unregister( const QStringList& arguments )
{
	m_serviceControl.unregisterService();

	return m_serviceControl.isServiceRegistered() ? Failed : Successful;
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_start( const QStringList& arguments )
{
	m_serviceControl.startService();

	return m_serviceControl.isServiceRunning() ? Successful : Failed;
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_stop( const QStringList& arguments )
{
	m_serviceControl.stopService();

	return m_serviceControl.isServiceRunning() ? Failed : Successful;
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_restart( const QStringList& arguments )
{
	handle_stop( arguments );
	return handle_start( arguments );
}



CommandLinePluginInterface::RunResult ServiceControlPlugin::handle_status( const QStringList& arguments )
{
	if( m_serviceControl.isServiceRunning() )
	{
		printf( "%s\n", qUtf8Printable( tr( "Service is running" ) ) );
	}
	else
	{
		printf( "%s\n", qUtf8Printable( tr( "Service is not running" ) ) );
	}

	return NoResult;
}
