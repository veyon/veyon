/*
 * OsXSessionFunctions.cpp - implementation of OsXSessionFunctions class
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

#include <QHostInfo>
#include <QProcessEnvironment>

#include "OsXSessionFunctions.h"

PlatformSessionFunctions::SessionId OsXSessionFunctions::currentSessionId()
{
	return DefaultSessionId;
}



PlatformSessionFunctions::SessionUptime OsXSessionFunctions::currentSessionUptime() const
{
	return InvalidSessionUptime;
}



QString OsXSessionFunctions::currentSessionClientAddress() const
{
	return {};
}



QString OsXSessionFunctions::currentSessionClientName() const
{
	return {};
}



QString OsXSessionFunctions::currentSessionHostName() const
{
	return QHostInfo::localHostName();
}



QString OsXSessionFunctions::currentSessionType() const
{
	return QStringLiteral("console");
}



bool OsXSessionFunctions::currentSessionHasUser() const
{
	return currentSessionEnvironmentVariables().contains( QStringLiteral("USER") );
}



PlatformSessionFunctions::EnvironmentVariables OsXSessionFunctions::currentSessionEnvironmentVariables() const
{
	EnvironmentVariables environment;
	const auto processEnvironment = QProcessEnvironment::systemEnvironment();
	for( const auto& key : processEnvironment.keys() )
	{
		environment.insert( key, processEnvironment.value( key ) );
	}
	return environment;
}



QVariant OsXSessionFunctions::querySettingsValueInCurrentSession( const QString& key ) const
{
	Q_UNUSED(key);
	return {};
}
