/*
 * OsXNetworkFunctions.cpp - implementation of OsXNetworkFunctions class
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

#include <QProcess>
#include <QStringList>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Logger.h"
#include "OsXNetworkFunctions.h"

OsXNetworkFunctions::PingResult OsXNetworkFunctions::ping( const QString& hostAddress )
{
	QProcess pingProcess;
	pingProcess.start( QStringLiteral("ping"),
					   { QStringLiteral("-c"), QStringLiteral("1"),
						 QStringLiteral("-W"), QString::number( PingTimeout ),
						 hostAddress } );

	if( pingProcess.waitForFinished( PingProcessTimeout ) )
	{
		switch( pingProcess.exitCode() )
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



bool OsXNetworkFunctions::configureFirewallException( const QString& applicationPath, const QString& description, bool enabled )
{
	Q_UNUSED(applicationPath)
	Q_UNUSED(description)
	Q_UNUSED(enabled)

	// There is no public API for configuring the macOS application firewall programmatically.
	return true;
}



bool OsXNetworkFunctions::configureSocketKeepalive( Socket socket, bool enabled, int idleTime, int interval, int probes )
{
	const auto fd = static_cast<int>( socket );
	int optval = enabled ? 1 : 0;

	if( setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof( optval ) ) < 0 )
	{
		vWarning() << "OsXNetworkFunctions: failed to set SO_KEEPALIVE";
		return false;
	}

	if( enabled == false )
	{
		return true;
	}

#ifdef TCP_KEEPALIVE
	optval = std::max( 1, idleTime / 1000 );
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPALIVE, &optval, sizeof( optval ) ) < 0 )
	{
		vWarning() << "OsXNetworkFunctions: failed to set TCP_KEEPALIVE";
		return false;
	}
#endif

#ifdef TCP_KEEPINTVL
	optval = std::max( 1, interval / 1000 );
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof( optval ) ) < 0 )
	{
		vWarning() << "OsXNetworkFunctions: failed to set TCP_KEEPINTVL";
		return false;
	}
#endif

#ifdef TCP_KEEPCNT
	optval = probes;
	if( setsockopt( fd, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof( optval ) ) < 0 )
	{
		vWarning() << "OsXNetworkFunctions: failed to set TCP_KEEPCNT";
		return false;
	}
#endif

	return true;
}



QNetworkInterface OsXNetworkFunctions::defaultRouteNetworkInterface()
{
	QProcess routeProcess;
	routeProcess.start( QStringLiteral("/sbin/route"), { QStringLiteral("-n"), QStringLiteral("get"), QStringLiteral("default") } );
	if( routeProcess.waitForFinished( PingProcessTimeout ) == false )
	{
		return {};
	}

	const auto output = QString::fromUtf8( routeProcess.readAllStandardOutput() );
	const auto lines = output.split( QLatin1Char('\n'), Qt::SkipEmptyParts );
	for( const auto& line : lines )
	{
		const auto parts = line.split( QLatin1Char(':') );
		if( parts.size() == 2 && parts.first().trimmed() == QStringLiteral("interface") )
		{
			return QNetworkInterface::interfaceFromName( parts.last().trimmed() );
		}
	}

	return {};
}



int OsXNetworkFunctions::networkInterfaceSpeedInMBitPerSecond( const QNetworkInterface& networkInterface )
{
	Q_UNUSED(networkInterface);
	// There's no straightforward and portable way to query the negotiated link speed on macOS.
	return 0;
}
