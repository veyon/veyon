/*
 * ItalcCore.cpp - implementation of iTALC Core
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>

#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "ItalcRfbExt.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "PasswordDialog.h"
#include "SocketDevice.h"

#include <rfb/rfbclient.h>

#include "minilzo.h"



ItalcConfiguration *ItalcCore::config = NULL;
AuthenticationCredentials *ItalcCore::authenticationCredentials = NULL;

int ItalcCore::serverPort = PortOffsetVncServer;
ItalcCore::UserRoles ItalcCore::role = ItalcCore::RoleOther;


void initResources()
{
	Q_INIT_RESOURCE(ItalcCore);
#ifndef QT_TRANSLATIONS_DIR
	Q_INIT_RESOURCE(qt_qm);
#endif
}



qint64 libvncClientDispatcher( char * _buf, const qint64 _len,
				const SocketOpCodes _op_code, void * _user )
{
	rfbClient * cl = (rfbClient *) _user;
	switch( _op_code )
	{
		case SocketRead:
			return ReadFromRFBServer( cl, _buf, _len ) != 0 ?
								_len : 0;
		case SocketWrite:
			return WriteToRFBServer( cl, _buf, _len ) != 0 ?
								_len : 0;
		case SocketGetPeerAddress:
//			strncpy( _buf, cl->host, _len );
			break;
	}
	return 0;

}



static void killWisPtis()
{
#ifdef ITALC_BUILD_WIN32
	int pid = -1;
	while( ( pid = LocalSystem::Process::findProcessId( "wisptis.exe" ) ) >= 0 )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
										PROCESS_TERMINATE |
										PROCESS_VM_READ,
										false, pid );
		if( !TerminateProcess( hProcess, 0 ) )
		{
			CloseHandle( hProcess );
			break;
		}
		CloseHandle( hProcess );
	}

	if( pid >= 0 )
	{
		LocalSystem::User user = LocalSystem::User::loggedOnUser();
		while( ( pid = LocalSystem::Process::findProcessId( "wisptis.exe", -1, &user ) ) >= 0 )
		{
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
											PROCESS_TERMINATE |
											PROCESS_VM_READ,
											false, pid );
			if( !TerminateProcess( hProcess, 0 ) )
			{
				CloseHandle( hProcess );
				break;
			}
			CloseHandle( hProcess );
		}
	}
#endif
}


bool ItalcCore::init()
{
	if( config )
	{
		return false;
	}

	killWisPtis();

	lzo_init();

	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	initResources();

	const QString loc = QLocale::system().name();

	QTranslator *tr = new QTranslator;
	tr->load( QString( ":/resources/%1.qm" ).arg( loc ) );
	QCoreApplication::installTranslator( tr );

	QTranslator *qtTr = new QTranslator;
#ifdef QT_TRANSLATIONS_DIR
	qtTr->load( QString( "qt_%1.qm" ).arg( loc ), QT_TRANSLATIONS_DIR );
#else
	qtTr->load( QString( ":/qt_%1.qm" ).arg( loc ) );
#endif
	QCoreApplication::installTranslator( qtTr );

	config = new ItalcConfiguration( ItalcConfiguration::defaultConfiguration() );
	*config += ItalcConfiguration( Configuration::Store::LocalBackend );

	serverPort = config->coreServerPort();

	return true;
}




bool ItalcCore::initAuthentication( int credentialTypes )
{
	if( authenticationCredentials )
	{
		delete authenticationCredentials;
		authenticationCredentials = NULL;
	}

	authenticationCredentials = new AuthenticationCredentials;

	bool success = true;

	if( credentialTypes & AuthenticationCredentials::UserLogon &&
			config->isLogonAuthenticationEnabled() )
	{
		if( QApplication::type() != QApplication::Tty )
		{
			PasswordDialog dlg( QApplication::activeWindow() );
			if( dlg.exec() &&
				dlg.credentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
			{
				authenticationCredentials->setLogonUsername( dlg.username() );
				authenticationCredentials->setLogonPassword( dlg.password() );

				success &= true;
			}
			else
			{
				success = false;
			}
		}
		else
		{
			success = false;
		}
	}

	if( credentialTypes & AuthenticationCredentials::PrivateKey &&
			config->isKeyAuthenticationEnabled() )
	{
		const QString privKeyFile = LocalSystem::Path::privateKeyPath( ItalcCore::role );
		qDebug() << "Loading private key" << privKeyFile << "for role" << ItalcCore::role;
		if( authenticationCredentials->loadPrivateKey( privKeyFile ) )
		{
			success &= true;
		}
		else
		{
			success = false;
		}
	}

	return success;
}




void ItalcCore::destroy()
{
	delete authenticationCredentials;
	authenticationCredentials = NULL;

	delete config;
	config = NULL;
}



static const char *userRoleNames[] =
{
	"none",
	"teacher",
	"admin",
	"supporter",
	"other"
} ;



QString ItalcCore::userRoleName( UserRole role )
{
	return userRoleNames[role];
}




namespace ItalcCore
{


bool Msg::send()
{
	QDataStream d( m_socketDevice );
	d << (uint8_t) rfbItalcCoreRequest;
	d << m_cmd;
	d << m_args;

	return true;
}



Msg &Msg::receive()
{
	QDataStream d( m_socketDevice );
	d >> m_cmd;
	d >> m_args;

	return *this;
}



const Command GetUserInformation = "GetUserInformation";
const Command UserInformation = "UserInformation";
const Command StartDemo = "StartDemo";
const Command StopDemo = "StopDemo";
const Command LockScreen = "LockScreen";
const Command UnlockScreen = "UnlockScreen";
const Command LockInput = "LockInput";
const Command UnlockInput = "UnlockInput";
const Command LogonUserCmd = "LogonUser";
const Command LogoutUser = "LogoutUser";
const Command DisplayTextMessage = "DisplayTextMessage";
const Command AccessDialog = "AccessDialog";
const Command ExecCmds = "ExecCmds";
const Command PowerOnComputer = "PowerOnComputer";
const Command PowerDownComputer = "PowerDownComputer";
const Command RestartComputer = "RestartComputer";
const Command DisableLocalInputs = "DisableLocalInputs";
const Command SetRole = "SetRole";

const Command DemoServerAllowHost = "DemoServerAllowHost";
const Command DemoServerUnallowHost = "DemoServerUnallowHost";
const Command StartDemoServer = "StartDemoServer";
const Command StopDemoServer = "StopDemoServer";

const Command ReportSlaveStateFlags = "ReportSlaveStateFlags";


} ;

