/*
 * OsXUserFunctions.cpp - implementation of OsXUserFunctions class
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

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QProcess>
#include <QStringList>

#include <pwd.h>
#include <unistd.h>

#include "Logger.h"
#include "OsXUserFunctions.h"
#include "VeyonCore.h"

namespace {
QStringList readProcessOutputLines( QProcess& process )
{
	if( process.waitForFinished() == false )
	{
		return {};
	}
	return QString::fromUtf8( process.readAllStandardOutput() ).split( QLatin1Char('\n'), Qt::SkipEmptyParts );
}
}

QString OsXUserFunctions::fullName( const QString& username )
{
	const auto strippedName = VeyonCore::stripDomain( username );
	const auto pwEntry = getpwnam( strippedName.toUtf8().constData() );
	if( pwEntry == nullptr )
	{
		return {};
	}

	const QString gecos = QString::fromUtf8( pwEntry->pw_gecos );
	return gecos.split( QLatin1Char(','), Qt::SkipEmptyParts ).value( 0 );
}



QStringList OsXUserFunctions::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	QProcess process;
	process.start( QStringLiteral("/usr/bin/dscl"), { QStringLiteral("."), QStringLiteral("-list"), QStringLiteral("/Groups") } );
	auto groups = readProcessOutputLines( process );
	groups.removeDuplicates();
	return groups;
}



QStringList OsXUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	const auto strippedName = VeyonCore::stripDomain( username );

	QProcess process;
	process.start( QStringLiteral("/usr/bin/id"), { QStringLiteral("-Gn"), strippedName } );
	if( process.waitForFinished() == false )
	{
		return {};
	}

	const auto output = QString::fromUtf8( process.readAllStandardOutput() );
	auto groups = output.split( QLatin1Char(' '), Qt::SkipEmptyParts );
	groups.removeDuplicates();
	return groups;
}



QString OsXUserFunctions::userGroupSecurityIdentifier( const QString& groupName )
{
	Q_UNUSED(groupName)
	return {};
}



bool OsXUserFunctions::isAnyUserLoggedOn()
{
	QProcess process;
	process.start( QStringLiteral("/usr/bin/who") );
	const auto entries = readProcessOutputLines( process );
	return entries.isEmpty() == false;
}



QString OsXUserFunctions::currentUser()
{
	const auto envUser = qEnvironmentVariable( "USER" );
	if( envUser.isEmpty() == false )
	{
		return envUser;
	}

	const auto pwEntry = getpwuid( getuid() );
	if( pwEntry )
	{
		return QString::fromUtf8( pwEntry->pw_name );
	}

	return {};
}



bool OsXUserFunctions::prepareLogon( const QString& username, const Password& password )
{
	Q_UNUSED(username)
	Q_UNUSED(password)

	vWarning() << "OsXUserFunctions: prepareLogon is not supported on this platform.";
	return false;
}



bool OsXUserFunctions::performLogon( const QString& username, const Password& password )
{
	Q_UNUSED(username)
	Q_UNUSED(password)

	vWarning() << "OsXUserFunctions: performLogon is not supported on this platform.";
	return false;
}



void OsXUserFunctions::logoff()
{
	QProcess::startDetached( QStringLiteral("/usr/bin/osascript"),
							 { QStringLiteral("-e"),
							   QStringLiteral("tell application \"System Events\" to log out") } );
}



bool OsXUserFunctions::authenticate( const QString& username, const Password& password )
{
	QProcess helper;
	helper.start( QStringLiteral("veyon-auth-helper") );
	if( helper.waitForStarted() == false )
	{
		vCritical() << "OsXUserFunctions: failed to start veyon-auth-helper";
		return false;
	}

	QDataStream stream( &helper );
	stream << VeyonCore::stripDomain( username );
	stream << QString::fromUtf8( password.toByteArray() );
	helper.closeWriteChannel();

	if( helper.waitForFinished() == false || helper.exitCode() != 0 )
	{
		vCritical() << "OsXUserFunctions: authentication helper failed:" << helper.readAllStandardError().trimmed();
		return false;
	}

	return true;
}



uid_t OsXUserFunctions::userIdFromName( const QString& username )
{
	const auto strippedName = VeyonCore::stripDomain( username );
	const auto pwEntry = getpwnam( strippedName.toUtf8().constData() );
	return pwEntry ? pwEntry->pw_uid : 0;
}
