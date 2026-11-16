// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformNetworkFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxNetworkFunctions : public PlatformNetworkFunctions
{
public:
	PingResult ping(const QString& hostAddress) override;
	bool configureFirewallException(const QString& applicationPath, const QString& description, bool enabled) override;

	bool configureSocketKeepalive(Socket socket, bool enabled, int idleTime, int interval, int probes) override;

	QNetworkInterface defaultRouteNetworkInterface() override;
	int networkInterfaceSpeedInMBitPerSecond(const QNetworkInterface& networkInterface) override;
};
