/*
 * BuiltinX11VncServer.cpp - implementation of BuiltinX11VncServer class
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

#include <QCoreApplication>
#include <QProcess>

#include "BuiltinX11VncServer.h"
#include "X11VncConfigurationWidget.h"

extern "C" int x11vnc_main( int argc, char * * argv );


BuiltinX11VncServer::BuiltinX11VncServer() :
	m_configuration()
{
}



BuiltinX11VncServer::~BuiltinX11VncServer()
{
}



QWidget* BuiltinX11VncServer::configurationWidget()
{
	return new X11VncConfigurationWidget( m_configuration );
}



void BuiltinX11VncServer::run( int serverPort, const QString& password )
{
	QStringList cmdline = { "-passwd", password,
							"-localhost",
							"-nosel",			// do not exchange clipboard-contents
							"-nosetclipboard",	// do not exchange clipboard-contents
							"-rfbport", QString::number( serverPort ) // set port at which the VNC server should listen
						  } ;

	const auto extraArguments = m_configuration.extraArguments();

	if( extraArguments.isEmpty() == false )
	{
		cmdline.append( extraArguments.split( " " ) );
	}

	if( m_configuration.isXDamageDisabled() )
	{
		cmdline.append( "-noxdamage" );
	}
	else
	{
		// workaround for x11vnc when running in an NX session or a Thin client LTSP session
		const auto systemEnv = QProcess::systemEnvironment();
		for( const auto& s : systemEnv )
		{
			if( s.startsWith( "NXSESSIONID=" ) || s.startsWith( "X2GO_SESSION=" ) || s.startsWith( "LTSP_CLIENT_MAC=" ) )
			{
				cmdline.append( "-noxdamage" );
			}
		}
	}

	// build new C-style command line array based on cmdline-QStringList
	auto argv = new char *[cmdline.size()+1];
	argv[0] = qstrdup( QCoreApplication::arguments()[0].toUtf8().constData() );
	int argc = 1;

	for( QStringList::iterator it = cmdline.begin();
				it != cmdline.end(); ++it, ++argc )
	{
		argv[argc] = new char[it->length() + 1];
		strcpy( argv[argc], it->toUtf8().constData() );
	}

	// run x11vnc-server
	x11vnc_main( argc, argv );
}
