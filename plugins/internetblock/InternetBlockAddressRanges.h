// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QPair>
#include <QStringList>

#include "VeyonCore.h"

// Platform-independent helpers that turn the list of networks which must stay
// reachable while internet access is blocked into firewall-friendly address lists.
class InternetBlockAddressRanges
{
public:
	using IPv4Range = QPair<quint32, quint32>; // inclusive [first, last]

	// networks that always stay reachable: loopback, RFC 1918 private networks,
	// link-local, multicast and limited broadcast, i.e. everything needed for
	// Veyon itself, DHCP, local DNS servers, file shares, printers etc.
	static QStringList defaultAllowedIPv4Networks();
	static QStringList defaultAllowedIPv6Networks();

	// splits a user supplied list (separated by commas, semicolons, whitespace
	// or newlines) into single entries
	static QStringList splitNetworkList(const QString& networkList);

	// parses IPv4 addresses and networks in CIDR notation ("10.1.2.3",
	// "192.168.0.0/16"); entries which can't be parsed are appended to
	// invalidEntries if non-null, IPv6 entries are appended to ipv6Entries
	static QList<IPv4Range> parseIPv4Networks(const QStringList& networks,
											  QStringList* invalidEntries = nullptr,
											  QStringList* ipv6Entries = nullptr);

	// sorts and merges overlapping or adjacent ranges
	static QList<IPv4Range> mergeRanges(QList<IPv4Range> ranges);

	// returns all IPv4 ranges NOT covered by the given (allowed) ranges
	static QList<IPv4Range> complement(const QList<IPv4Range>& allowedRanges);

	// formats ranges as "a.b.c.d-e.f.g.h" (or "a.b.c.d" for single addresses)
	// as accepted by the Windows Firewall RemoteAddresses property
	static QStringList formatRanges(const QList<IPv4Range>& ranges);

	// convenience function combining the steps above
	static QStringList blockedIPv4Ranges(const QStringList& allowedNetworks);

	static QString ipv4ToString(quint32 address);

};
