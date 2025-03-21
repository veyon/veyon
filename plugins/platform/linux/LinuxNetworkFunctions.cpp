/*
 * LinuxNetworkFunctions.cpp - implementation of LinuxNetworkFunctions class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <QProcess>

#include "LinuxNetworkFunctions.h"

LinuxNetworkFunctions::PingResult LinuxNetworkFunctions::ping(const QString& hostAddress)
{
	QProcess pingProcess;
	pingProcess.start( QStringLiteral("ping"), { QStringLiteral("-c"), QStringLiteral("1"), QStringLiteral("-w"), QString::number( PingTimeout / 1000 ), hostAddress } );
	if (pingProcess.waitForFinished(PingProcessTimeout))
	{
		switch (pingProcess.exitCode())
		{
		case 0: return PingResult::ReplyReceived;
		case 1: return PingResult::TimedOut;
		case 2: return PingResult::NameResolutionFailed;
		default:
			break;
		}
	}

	return PingResult::Unknown;
}



bool LinuxNetworkFunctions::configureFirewallException( const QString& applicationPath, const QString& description, bool enabled )
{
	Q_UNUSED(applicationPath)
	Q_UNUSED(description)
	Q_UNUSED(enabled)

	return true;
}



bool LinuxNetworkFunctions::configureSocketKeepalive( Socket socket, bool enabled, int idleTime, int interval, int probes )
{
	int optval;
	socklen_t optlen = sizeof(optval);
	auto fd = static_cast<int>( socket );

	// Try to set the option active
	optval = enabled ? 1 : 0;
	if( setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen ) < 0 )
	{
		vWarning() << "could not set SO_KEEPALIVE";
		return false;
	}

	optval = std::max<int>( 1, idleTime / 1000 );
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen ) < 0 )
	{
		vWarning() << "could not set TCP_KEEPIDLE";
		return false;
	}

	optval = std::max<int>( 1, interval / 1000 );
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen ) < 0 )
	{
		vWarning() << "could not set TCP_KEEPINTVL";
		return false;
	}

	optval = probes;
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen ) < 0 )
	{
		vWarning() << "could not set TCP_KEEPCNT";
		return false;
	}

	return true;
}
