/*
 * BuiltinX11VncServer.cpp - implementation of BuiltinX11VncServer class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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



void BuiltinX11VncServer::runServer( int serverPort, const QString& password )
{
	QStringList cmdline = { QStringLiteral("-localhost"),
							QStringLiteral("-nosel"),			// do not exchange clipboard-contents
							QStringLiteral("-nosetclipboard"),	// do not exchange clipboard-contents
							QStringLiteral("-rfbport"), QString::number( serverPort ) // set port at which the VNC server should listen
						  } ;

	const auto extraArguments = m_configuration.extraArguments();

	if( extraArguments.isEmpty() == false )
	{
		cmdline.append( extraArguments.split( QLatin1Char(' ') ) );
	}

	if( m_configuration.isXDamageDisabled() )
	{
		cmdline.append( QStringLiteral("-noxdamage") );
	}
	else
	{
		// workaround for x11vnc when running in an NX session or a Thin client LTSP session
		const auto systemEnv = QProcess::systemEnvironment();
		for( const auto& s : systemEnv )
		{
			if( s.startsWith( QStringLiteral("NXSESSIONID=") ) ||
					s.startsWith( QStringLiteral("X2GO_SESSION=") ) ||
					s.startsWith( QStringLiteral("LTSP_CLIENT_MAC=") ) )
			{
				cmdline.append( QStringLiteral("-noxdamage") );
			}
		}
	}

#ifdef VEYON_X11VNC_EXTERNAL
	QTemporaryFile tempFile;
	if( tempFile.open() == false ) // Flawfinder: ignore
	{
		vCritical() << "Could not create temporary file!";
		return;
	}
	tempFile.write( password.toLocal8Bit() );
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
		QThread::msleep( 5000 );
	}
	else
	{
		x11vnc.waitForFinished( -1 );
	}
#else
	cmdline.append( { QStringLiteral("-passwd"), password } );

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
	x11vnc_main( argc, argv );

	for( int i = 0; i < argc; ++i )
	{
		delete[] argv[i];
	}

	delete[] argv;
#endif
}


IMPLEMENT_CONFIG_PROXY(X11VncConfiguration)
