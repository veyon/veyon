/*
 * BuiltinX11VncServer.cpp - implementation of BuiltinX11VncServer class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QProcess>
#include <QTemporaryFile>
#include <QThread>

#include "BuiltinX11VncServer.h"
#include "VeyonConfiguration.h"
#include "X11VncConfigurationWidget.h"

extern "C" int x11vnc_main( int argc, char * * argv );


BuiltinX11VncServer::BuiltinX11VncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



QWidget* BuiltinX11VncServer::configurationWidget()
{
	return new X11VncConfigurationWidget( m_configuration );
}



void BuiltinX11VncServer::prepareServer()
{
}



bool BuiltinX11VncServer::runServer( int serverPort, const Password& password )
{
	QStringList cmdline = { QStringLiteral("-localhost"),
							QStringLiteral("-nosel"),			// do not exchange clipboard-contents
							QStringLiteral("-nosetclipboard"),	// do not exchange clipboard-contents
							QStringLiteral("-rfbport"), QString::number( serverPort ), // set port at which the VNC server should listen
							QStringLiteral("-rfbportv6"), QString::number( serverPort ), // set IPv6 port at which the VNC server should listen
							QStringLiteral("-no6"),
						  } ;

	const auto extraArguments = m_configuration.extraArguments();

	if( extraArguments.isEmpty() == false )
	{
		cmdline.append( extraArguments.split( QLatin1Char(' ') ) );
	}

	const auto systemEnv = QProcessEnvironment::systemEnvironment();
	if( systemEnv.contains( QStringLiteral("XRDP_SESSION") ) )
	{
		cmdline.append( QStringLiteral("-noshm") );
	}

	if( m_configuration.isXDamageDisabled() ||
		// workaround for x11vnc when running in a NX session or a Thin client LTSP session
		systemEnv.contains( QStringLiteral("NXSESSIONID") ) ||
		systemEnv.contains( QStringLiteral("X2GO_SESSION") ) ||
		systemEnv.contains( QStringLiteral("LTSP_CLIENT_MAC") ) )
	{
		cmdline.append( QStringLiteral("-noxdamage") );
	}

#ifdef VEYON_X11VNC_EXTERNAL
	QTemporaryFile tempFile;
	if( tempFile.open() == false ) // Flawfinder: ignore
	{
		vCritical() << "Could not create temporary file!";
		return false;
	}
	tempFile.write( password.toByteArray() );
	tempFile.close();

	cmdline.append( QStringLiteral("-passwdfile") );
	cmdline.append( QStringLiteral("rm:") + tempFile.fileName() );
	cmdline.append( QStringLiteral("-forever") );
	cmdline.append( QStringLiteral("-shared") );
	cmdline.append( QStringLiteral("-nocmds") );
	cmdline.append( QStringLiteral("-noremote") );

	QProcess x11vnc;
	x11vnc.setProcessChannelMode( QProcess::ForwardedChannels );
	x11vnc.start( QStringLiteral("x11vnc"), cmdline );
	if( x11vnc.waitForStarted() == false )
	{
		vCritical() << "Could not start external x11vnc:" << x11vnc.errorString();
		vCritical() << "Please make sure x11vnc is installed and installation directory is in PATH!";
		return false;
	}
	else
	{
		x11vnc.waitForFinished( -1 );
	}

	return true;
#else
	cmdline.append( { QStringLiteral("-passwd"), QString::fromUtf8( password.toByteArray() ) } );

	// build new C-style command line array based on cmdline-QStringList
	const auto appArguments = QCoreApplication::arguments();
	auto argv = new char *[cmdline.size()+1]; // Flawfinder: ignore
	argv[0] = qstrdup( appArguments.first().toUtf8().constData() );
	int argc = 1;

	for( auto it = cmdline.begin(), end = cmdline.end(); it != end; ++it, ++argc )
	{
		const auto len = static_cast<size_t>( it->length() );
		argv[argc] = new char[len + 1];
		strncpy( argv[argc], it->toUtf8().constData(), len ); // Flawfinder: ignore
		argv[argc][len] = 0;
	}

	// run x11vnc-server
	const auto result = x11vnc_main( argc, argv );

	for( int i = 0; i < argc; ++i )
	{
		delete[] argv[i];
	}

	delete[] argv;

	return result == 0;
#endif
}


IMPLEMENT_CONFIG_PROXY(X11VncConfiguration)
