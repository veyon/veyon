// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QStringList>

#include "VeyonCore.h"

// Applies/removes a system-wide outbound firewall rule that blocks all traffic
// except to the allowed (local) networks. Implemented per platform:
// - Windows: Windows Defender Firewall rule via the INetFwPolicy2 COM API
// - Linux: dedicated nftables table
// Requires administrative privileges (Veyon Server runs as SYSTEM/root).
class InternetBlocker
{
public:
	struct Settings
	{
		QStringList allowedIPv4Networks;
		QStringList allowedIPv6Networks;
		bool blockIPv6 = true;
	};

	static bool block(const Settings& settings);
	static bool unblock();
	static bool isBlocked();

	static QString ruleName()
	{
		return QStringLiteral("Veyon Internet Block");
	}

	// nftables table name used on Linux
	static QString tableName()
	{
		return QStringLiteral("veyon_internet_block");
	}

	// generates the nftables script for the given settings (Linux backend, exposed for tests)
	static QString nftablesScript(const Settings& settings);

};
