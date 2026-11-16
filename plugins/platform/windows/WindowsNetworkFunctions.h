// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformNetworkFunctions.h"

#include <winbase.h>

// clazy:exclude=copyable-polymorphic

class WindowsNetworkFunctions : public PlatformNetworkFunctions
{
public:
	WindowsNetworkFunctions();

	PingResult ping(const QString& hostAddress) override;
	bool configureFirewallException(const QString& applicationPath, const QString& description, bool enabled) override;

	bool configureSocketKeepalive(Socket socket, bool enabled, int idleTime, int interval, int probes) override;

	QNetworkInterface defaultRouteNetworkInterface() override;
	int networkInterfaceSpeedInMBitPerSecond(const QNetworkInterface& networkInterface) override;

	static constexpr auto WindowsFirewallServiceError = HRESULT(0x800706D9);

private:
	bool pingIPv4Address(const QString& hostAddress, PingResult* result);
	bool pingIPv6Address(const QString& hostAddress, PingResult* result);
	bool pingViaUtility(const QString& hostAddress, PingResult* result);
};
