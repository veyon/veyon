// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>
#include <QRegularExpression>

#include <algorithm>

#include "InternetBlockAddressRanges.h"


QStringList InternetBlockAddressRanges::defaultAllowedIPv4Networks()
{
	return {
		QStringLiteral("0.0.0.0/32"),          // DHCP
		QStringLiteral("10.0.0.0/8"),          // RFC 1918
		QStringLiteral("127.0.0.0/8"),         // loopback
		QStringLiteral("169.254.0.0/16"),      // link-local
		QStringLiteral("172.16.0.0/12"),       // RFC 1918
		QStringLiteral("192.168.0.0/16"),      // RFC 1918
		QStringLiteral("224.0.0.0/4"),         // multicast (mDNS, SSDP, ...)
		QStringLiteral("255.255.255.255/32"),  // limited broadcast
	};
}



QStringList InternetBlockAddressRanges::defaultAllowedIPv6Networks()
{
	return {
		QStringLiteral("::1/128"),     // loopback
		QStringLiteral("fe80::/10"),   // link-local
		QStringLiteral("fc00::/7"),    // unique local addresses
		QStringLiteral("ff00::/8"),    // multicast
	};
}



QStringList InternetBlockAddressRanges::splitNetworkList(const QString& networkList)
{
	static const QRegularExpression separators(QStringLiteral("[,;\\s]+"));
	return networkList.split(separators, Qt::SkipEmptyParts);
}



QList<InternetBlockAddressRanges::IPv4Range> InternetBlockAddressRanges::parseIPv4Networks(const QStringList& networks,
																						   QStringList* invalidEntries,
																						   QStringList* ipv6Entries)
{
	QList<IPv4Range> ranges;

	for (const auto& entry : networks)
	{
		const auto network = entry.trimmed();
		if (network.isEmpty())
		{
			continue;
		}

		QPair<QHostAddress, int> subnet;
		if (network.contains(QLatin1Char('/')))
		{
			subnet = QHostAddress::parseSubnet(network);
		}
		else
		{
			QHostAddress address;
			if (address.setAddress(network))
			{
				subnet = {address, address.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32};
			}
			else
			{
				subnet = {QHostAddress(), -1};
			}
		}

		if (subnet.first.isNull() || subnet.second < 0)
		{
			if (invalidEntries)
			{
				invalidEntries->append(network);
			}
			continue;
		}

		if (subnet.first.protocol() == QAbstractSocket::IPv6Protocol)
		{
			if (ipv6Entries)
			{
				ipv6Entries->append(network);
			}
			continue;
		}

		const auto prefixLength = subnet.second;
		const quint32 mask = prefixLength == 0 ? 0 : (0xFFFFFFFFu << (32 - prefixLength));
		const quint32 first = subnet.first.toIPv4Address() & mask;
		const quint32 last = first | ~mask;
		ranges.append({first, last});
	}

	return ranges;
}



QList<InternetBlockAddressRanges::IPv4Range> InternetBlockAddressRanges::mergeRanges(QList<IPv4Range> ranges)
{
	std::sort(ranges.begin(), ranges.end());

	QList<IPv4Range> merged;
	for (const auto& range : std::as_const(ranges))
	{
		// merge if overlapping or directly adjacent (beware of overflow at 255.255.255.255)
		if (merged.isEmpty() == false &&
			(merged.last().second == 0xFFFFFFFFu || range.first <= merged.last().second + 1))
		{
			merged.last().second = std::max(merged.last().second, range.second);
		}
		else
		{
			merged.append(range);
		}
	}

	return merged;
}



QList<InternetBlockAddressRanges::IPv4Range> InternetBlockAddressRanges::complement(const QList<IPv4Range>& allowedRanges)
{
	const auto allowed = mergeRanges(allowedRanges);

	QList<IPv4Range> blocked;
	quint64 next = 0; // 64 bit to represent "past 255.255.255.255"

	for (const auto& range : allowed)
	{
		if (range.first > next)
		{
			blocked.append({static_cast<quint32>(next), range.first - 1});
		}
		next = quint64(range.second) + 1;
	}

	if (next <= 0xFFFFFFFFu)
	{
		blocked.append({static_cast<quint32>(next), 0xFFFFFFFFu});
	}

	return blocked;
}



QStringList InternetBlockAddressRanges::formatRanges(const QList<IPv4Range>& ranges)
{
	QStringList result;
	result.reserve(ranges.size());

	for (const auto& range : ranges)
	{
		if (range.first == range.second)
		{
			result.append(ipv4ToString(range.first));
		}
		else
		{
			result.append(ipv4ToString(range.first) + QLatin1Char('-') + ipv4ToString(range.second));
		}
	}

	return result;
}



QStringList InternetBlockAddressRanges::blockedIPv4Ranges(const QStringList& allowedNetworks)
{
	return formatRanges(complement(parseIPv4Networks(allowedNetworks)));
}



QString InternetBlockAddressRanges::ipv4ToString(quint32 address)
{
	return QHostAddress(address).toString();
}
