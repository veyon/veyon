/*
 * LinuxNetworkFunctions.cpp - implementation of LinuxNetworkFunctions class
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

#include "LinuxNetworkFunctions.h"

bool LinuxNetworkFunctions::ping( const QString& hostAddress )
{
	QProcess pingProcess;
	pingProcess.start( "ping", { "-W", "1", "-c", QString::number( PingTimeout / 1000 ), hostAddress } );
	pingProcess.waitForFinished( PingProcessTimeout );

	return pingProcess.exitCode() == 0;
}
