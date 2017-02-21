/*
 * LogonAuthentication.cpp - class doing logon authentication
 *
 * Copyright (c) 2011-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QDebug>
#include <QDataStream>
#include <QProcess>

#include "LogonAuthentication.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"

#ifdef ITALC_BUILD_WIN32
#include "authSSP.h"
#endif


bool LogonAuthentication::authenticateUser( const AuthenticationCredentials &cred )
{
	qDebug() << "Authenticating user" << cred.logonUsername();

	bool result = false;
#ifdef ITALC_BUILD_WIN32
#ifdef UNICODE
	return CUPSD2( (const char *) cred.logonUsername().unicode(), (const char *) cred.logonPassword().unicode() );
#else
	return CUPSD2( cred.logonUsername().toLocal8Bit().constData(), cred.logonPassword().toLocal8Bit().constData() );
#endif
#endif

#ifdef ITALC_BUILD_LINUX
	QProcess p;
	p.start( "italc-auth-helper" );
	p.waitForStarted();

	QDataStream ds( &p );
	ds << cred.logonUsername();
	ds << cred.logonPassword();

	p.closeWriteChannel();
	p.waitForFinished();

	if( p.exitCode() == 0 )
	{
		result = true;
		qDebug() << "User authenticated successfully";
	}
	else
	{
		qCritical() << "ItalcAuthHelper failed:" << p.readAll().trimmed();
	}
#endif

	return result;
}

