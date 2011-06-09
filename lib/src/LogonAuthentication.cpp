/*
 * LogonAuthentication.cpp - class doing logon authentication
 *
 * Copyright (c) 2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <italcconfig.h>

#include <QtCore/QDebug>
#include <QtCore/QLibrary>
#include <QtCore/QProcess>

#include "LogonAuthentication.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"


bool LogonAuthentication::authenticateUser( const AuthenticationCredentials &cred )
{
	bool result = false;
#ifdef ITALC_BUILD_WIN32
	typedef int(*cupsdPtr_t)(const char * userin, const char *password, const char *machine);

	QLibrary authSSP( "authSSP" );
	if( authSSP.load() )
	{
		cupsdPtr_t cupsdPtr = (cupsdPtr_t) authSSP.resolve( "CUPSD" );
		if( cupsdPtr )
		{
			result = cupsdPtr( cred.logonUsername().toUtf8().constData(),
								cred.logonPassword().toUtf8().constData(),
											"127.0.0.1" ) > 0 ? true : false;
		}
	}
#endif

#ifdef ITALC_BUILD_LINUX
	QProcess p;
	p.start( "italc_auth_helper" );
	p.waitForStarted();

	QDataStream ds( &p );
	ds << cred.logonUsername();
	ds << cred.logonPassword();

	p.closeWriteChannel();
	p.waitForFinished();

	if( p.exitCode() == 0 )
	{
		QProcess p;
		p.start( "getent", QStringList() << "group" );
		p.waitForFinished();

		QStringList groups = QString( p.readAll() ).split( '\n' );
		foreach( const QString &group, groups )
		{
			QStringList groupComponents = group.split( ':' );
			if( groupComponents.size() == 4 &&
				ItalcCore::config->logonGroups().
										contains( groupComponents.first() ) &&
				groupComponents.last().split( ',' ).contains( cred.logonUsername() ) )
			{
				result = true;
			}
		}
		qCritical() << "User not in a privileged group";
	}
	else
	{
		qCritical() << "ItalcAuthHelper failed:" << p.readAll().trimmed();
	}
#endif

	return result;
}

