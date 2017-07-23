/*
 * LinuxUserSessionFunctions.cpp - implementation of LinuxUserSessionFunctions class
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

#include <QProcess>

#include "LinuxUserSessionFunctions.h"

QStringList LinuxUserSessionFunctions::loggedOnUsers()
{
	QStringList users;

	QProcess whoProcess;
	whoProcess.start( QStringLiteral("who") );
	whoProcess.waitForFinished( WhoProcessTimeout );

	if( whoProcess.exitCode() != 0 )
	{
		return users;
	}

	const auto lines = whoProcess.readAll().split( '\n' );
	for( const auto& line : lines )
	{
		const auto user = line.split( ' ' ).value( 0 );
		if( user.isEmpty() == false && users.contains( user ) == false )
		{
			users.append( user ); // clazy:exclude=reserve-candidates
		}
	}

	return users;
}
