// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QNetworkInterface>

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformNetworkFunctions
{
public:
	using Socket = uintptr_t;

	static constexpr int PingTimeout = 1000;
	static constexpr int PingProcessTimeout = 5000;

	enum class PingResult {
		Unknown,
		ReplyReceived,
		TimedOut,
		NameResolutionFailed
	};

	virtual ~PlatformNetworkFunctions() = default;

	virtual PingResult ping(const QString& hostAddress) = 0;
	virtual bool configureFirewallException(const QString& applicationPath, const QString& description, bool enabled) = 0;

	virtual bool configureSocketKeepalive(Socket socket, bool enabled, int idleTime, int interval, int probes) = 0;

	virtual QNetworkInterface defaultRouteNetworkInterface() = 0;
	virtual int networkInterfaceSpeedInMBitPerSecond(const QNetworkInterface& networkInterface) = 0;
};
