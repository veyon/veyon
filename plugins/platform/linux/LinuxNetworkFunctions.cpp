/*
 * LinuxNetworkFunctions.cpp - implementation of LinuxNetworkFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <QProcess>

#include "LinuxNetworkFunctions.h"

bool LinuxNetworkFunctions::ping( const QString& hostAddress )
{
	QProcess pingProcess;
	pingProcess.start( QStringLiteral("ping"), { "-W", "1", "-c", QString::number( PingTimeout / 1000 ), hostAddress } );
	pingProcess.waitForFinished( PingProcessTimeout );

	return pingProcess.exitCode() == 0;
}



bool LinuxNetworkFunctions::configureFirewallException( const QString& applicationPath, const QString& description, bool enabled )
{
	Q_UNUSED(applicationPath)
	Q_UNUSED(description)
	Q_UNUSED(enabled)

	return true;
}



bool LinuxNetworkFunctions::configureSocketKeepalive( int socket, bool enabled, int idleTime, int interval, int probes )
{
	int optval;
	socklen_t optlen = sizeof(optval);

	// Try to set the option active
	optval = enabled ? 1 : 0;
	if( setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen ) < 0 )
	{
		qWarning() << Q_FUNC_INFO << "could not set SO_KEEPALIVE";
		return false;
	}

	optval = std::max<int>( 1, idleTime / 1000 );
	if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen ) < 0 )
	{
		qWarning() << Q_FUNC_INFO << "could not set TCP_KEEPIDLE";
		return false;
	}

	optval = std::max<int>( 1, interval / 1000 );
	if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen ) < 0 )
	{
		qWarning() << Q_FUNC_INFO << "could not set TCP_KEEPINTVL";
		return false;
	}

	optval = probes;
	if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen ) < 0 )
	{
		qWarning() << Q_FUNC_INFO << "could not set TCP_KEEPCNT";
		return false;
	}

	return true;
}
