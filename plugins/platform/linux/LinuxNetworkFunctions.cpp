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

#include <QFile>
#include <QProcess>
#include <QRegularExpression>

#include "LinuxNetworkFunctions.h"
#include "ProcessHelper.h"

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



QNetworkInterface LinuxNetworkFunctions::defaultRouteNetworkInterface()
{
	const auto routes = ProcessHelper(QStringLiteral("ip"), {QStringLiteral("route")}).runAndReadAll().split('\n') +
						ProcessHelper(QStringLiteral("ip"), {QStringLiteral("-6"), QStringLiteral("route")} ).runAndReadAll().split('\n');

	QNetworkInterface defaultNetworkInterface;

	for (const auto& route : routes)
	{
		const auto routeArgs = route.split(' ');
		const auto routeDestination = routeArgs.value(0);

		if (routeDestination == QByteArrayLiteral("default") ||
			routeDestination == QByteArrayLiteral("0.0.0.0") ||
			routeDestination == QByteArrayLiteral("::/0"))
		{
			for (int i = 1; i < routeArgs.size()-1; ++i )
			{
				if (routeArgs[i] == QByteArrayLiteral("dev"))
				{
					return QNetworkInterface::interfaceFromName(QString::fromLatin1(routeArgs[i+1]));
				}
			}
		}
	}

	return {};
}



static int getInterfaceSpeedFromNetworkManager(const QString& interfaceName)
{
	const auto speedInfo = ProcessHelper(QStringLiteral("nmcli"), {QStringLiteral("-f"),
																   QStringLiteral("CAPABILITIES.SPEED"),
																   QStringLiteral("dev"),
																   QStringLiteral("show"),
																   interfaceName})
						   .runAndReadAll();
	static const QRegularExpression capSpeedRX(QStringLiteral("CAPABILITIES.SPEED:[^\\d]*([\\d]+) Mb/s"));
	const auto capSpeedMatch = capSpeedRX.match(QString::fromUtf8(speedInfo));
	if (capSpeedMatch.hasMatch())
	{
		return capSpeedMatch.captured(1).toInt();
	}
	return 0;
}



static int getInterfaceSpeedFromSystemd(const QString& interfaceName)
{
	const auto interfaceStatus = ProcessHelper(QStringLiteral("networkctl"), {QStringLiteral("status"), interfaceName})
						   .runAndReadAll();
	static const QRegularExpression speedRX(QStringLiteral("Speed: ([\\d.]+)([MG])bps"));
	const auto speedMatch = speedRX.match(QString::fromUtf8(interfaceStatus));
	if (speedMatch.hasMatch())
	{
		if (speedMatch.captured(2) == QStringLiteral("G"))
		{
			return speedMatch.captured(1).toFloat() * 1000;
		}
		return speedMatch.captured(1).toFloat();
	}
	return 0;
}



static int getWirelessInterfaceSpeed(const QString& interfaceName)
{
	const auto wifiLinkInfo = ProcessHelper(QStringLiteral("iw"),
											{QStringLiteral("dev"), interfaceName, QStringLiteral("link")})
							  .runAndReadAll();
	static const QRegularExpression txBitrateRX(QStringLiteral("tx bitrate: ([\\d.]+) MBit/s"));
	const auto txBitrateMatch = txBitrateRX.match(QString::fromUtf8(wifiLinkInfo));
	if (txBitrateMatch.hasMatch())
	{
		return txBitrateMatch.captured(1).toFloat();
	}
	return 0;
}



static int getInterfaceSpeedFromSysFS(const QString& interfaceName)
{
	QFile speedFile(QStringLiteral("/sys/class/net/%1/speed").arg(interfaceName));
	if (speedFile.open(QFile::ReadOnly))
	{
		return speedFile.readAll().trimmed().toInt();
	}
	return 0;
}



int LinuxNetworkFunctions::networkInterfaceSpeedInMBitPerSecond(const QNetworkInterface& networkInterface)
{
	if (networkInterface.isValid())
	{
		const auto networkInterfaceName = networkInterface.name();

		if (const auto speed = getInterfaceSpeedFromNetworkManager(networkInterfaceName); speed > 0)
		{
			return speed;
		}

		if (const auto speed = getInterfaceSpeedFromSystemd(networkInterfaceName); speed > 0)
		{
			return speed;
		}

		if (const auto speed = getWirelessInterfaceSpeed(networkInterfaceName); speed > 0)
		{
			return speed;
		}

		if (const auto speed = getInterfaceSpeedFromSysFS(networkInterfaceName); speed > 0)
		{
			return speed;
		}
	}

	return 0;
}
