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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "ItalcCore.h"
#include "DsaKey.h"
#include "ItalcConfiguration.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "Logger.h"

#include "minilzo.h"



ItalcConfiguration *ItalcCore::config = NULL;

static PrivateDSAKey * privDSAKey = NULL;

int ItalcCore::serverPort = PortOffsetIVS;
ItalcCore::UserRoles ItalcCore::role = ItalcCore::RoleOther;


void initResources()
{
	Q_INIT_RESOURCE(italc_core);
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



bool ItalcCore::initAuthentication()
{
	if( privDSAKey )
	{
		delete privDSAKey;
		privDSAKey = NULL;
	}

	const QString privKeyFile = LocalSystem::Path::privateKeyPath( role );
	qDebug() << "Loading private key" << privKeyFile << "for role" << role;
	if( privKeyFile.isEmpty() )
	{
		return false;
	}
	privDSAKey = new PrivateDSAKey( privKeyFile );

	return privDSAKey->isValid();
}




bool ItalcCore::init()
{
	if( config )
	{
		return false;
	}

	lzo_init();

	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	initResources();

	ItalcConfiguration dc = ItalcConfiguration::defaultConfiguration();
	config = new ItalcConfiguration( dc );
	*config += ItalcConfiguration( Configuration::Store::LocalBackend );

	return true;
}




void ItalcCore::destroy()
{
	delete config;
	config = NULL;

	delete privDSAKey;
	privDSAKey = NULL;
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




void handleSecTypeItalc( rfbClient *client )
{
	SocketDevice socketDev( libvncClientDispatcher, client );

	// read list of supported authentication types
	QMap<QString, QVariant> supportedAuthTypes = socketDev.read().toMap();

	int chosenAuthType = ItalcAuthDSA;
	if( !supportedAuthTypes.isEmpty() )
	{
		chosenAuthType = supportedAuthTypes.values().first().toInt();

		// look whether the ItalcVncConnection recommends a specific
		// authentication type (e.g. ItalcAuthHostBased when running as
		// demo client)
		ItalcVncConnection *t = (ItalcVncConnection *)
										rfbClientGetClientData( client, 0 );

		if( t != NULL )
		{
			foreach( const QVariant &v, supportedAuthTypes )
			{
				if( t->italcAuthType() == v.toInt() )
				{
					chosenAuthType = v.toInt();
				}
			}
		}
	}

	socketDev.write( QVariant( chosenAuthType ) );

	if( chosenAuthType == ItalcAuthDSA )
	{
		QByteArray chall = socketDev.read().toByteArray();
		socketDev.write( QVariant( (int) ItalcCore::role ) );
		if( !privDSAKey )
		{
			ItalcCore::initAuthentication();
		}
		socketDev.write( privDSAKey->sign( chall ) );
	}
	else if( chosenAuthType == ItalcAuthHostBased )
	{
		// nothing to do - we just get accepted if our IP is in the list of
		// allowed hosts
	}
	else if( chosenAuthType == ItalcAuthNone )
	{
		// nothing to do - we just get accepted
	}
}




namespace ItalcCore
{

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

