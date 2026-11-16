// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMutexLocker>
#include <QNetworkInterface>

#include "ConnectionLimiter.h"
#include "VeyonConfiguration.h"

#include <algorithm>
#include <chrono>


namespace {

// the interface list is only refreshed every few seconds to keep admit() cheap under load
constexpr auto InterfaceCacheMs = 10000;

struct InterfaceCache
{
	QMutex mutex;
	qint64 refreshedMs = -InterfaceCacheMs;
	QList<QNetworkAddressEntry> entries;
};

InterfaceCache& interfaceCache()
{
	static InterfaceCache cache;
	return cache;
}

}


ConnectionLimiter::ConnectionLimiter(const Config& config, const Clock& nowMs, const OnLinkResolver& isOnLink) :
	m_config{std::max(config.preAuthMax, 1),
			 std::max(config.perIpBurst, 1),
			 std::max(config.perIpRefillSeconds, 1),
			 std::max(config.backoffMaxSeconds, 0)},
	m_nowMs(nowMs ? nowMs : Clock(steadyNow)),
	m_isOnLink(isOnLink ? isOnLink : OnLinkResolver(isOnLinkOfLocalInterface))
{
}



ConnectionLimiter::Config ConnectionLimiter::configFromConfiguration(const VeyonConfiguration& configuration)
{
	return {configuration.connectionLimitPreAuthMax(),
			configuration.connectionLimitPerIpBurst(),
			configuration.connectionLimitPerIpRefillSeconds(),
			configuration.connectionLimitAuthFailureBackoffMaxSeconds()};
}



qint64 ConnectionLimiter::steadyNow()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}



bool ConnectionLimiter::isOnLinkOfLocalInterface(const QHostAddress& address)
{
	auto& cache = interfaceCache();
	QMutexLocker locker(&cache.mutex);

	const auto now = steadyNow();
	if (now - cache.refreshedMs >= InterfaceCacheMs)
	{
		cache.entries.clear();

		const auto interfaces = QNetworkInterface::allInterfaces();
		for (const auto& networkInterface : interfaces)
		{
			if (networkInterface.flags().testFlag(QNetworkInterface::IsUp) == false ||
				networkInterface.flags().testFlag(QNetworkInterface::IsLoopBack))
			{
				continue;
			}

			const auto addressEntries = networkInterface.addressEntries();
			for (const auto& addressEntry : addressEntries)
			{
				if (addressEntry.ip().protocol() == QAbstractSocket::IPv6Protocol && addressEntry.prefixLength() > 0)
				{
					cache.entries.append(addressEntry);
				}
			}
		}

		cache.refreshedMs = now;
	}

	return std::any_of(cache.entries.cbegin(), cache.entries.cend(), [&address](const QNetworkAddressEntry& entry) {
		return address.isInSubnet(entry.ip(), entry.prefixLength());
	});
}



bool ConnectionLimiter::admit(const QHostAddress& address)
{
	if (address.isNull())
	{
		vDebug() << "refusing connection without valid source address";
		return false;
	}

	QMutexLocker locker(&m_mutex);

	if (m_preAuthCount >= m_config.preAuthMax)
	{
		vDebug() << "refusing connection from" << address.toString() << "- too many pre-auth connections";
		return false;
	}

	if (isLoopback(address))
	{
		// loopback is only subject to the global pre-auth cap
		++m_preAuthCount;
		return true;
	}

	const auto now = m_nowMs();
	auto& entry = entryFor(keyFor(address, m_isOnLink), now);

	if (isTrusted(entry, now))
	{
		if (takeToken(entry.trustedBucket, now) == false)
		{
			vDebug() << "refusing connection from" << address.toString() << "- trusted peer budget exhausted";
			return false;
		}
	}
	else
	{
		if (entry.blockedUntilMs > now)
		{
			vDebug() << "refusing connection from" << address.toString() << "- backing off after failed authentication";
			return false;
		}

		if (takeToken(entry.bucket, now) == false)
		{
			vDebug() << "refusing connection from" << address.toString() << "- rate limit exceeded";
			return false;
		}
	}

	++m_preAuthCount;
	return true;
}



void ConnectionLimiter::authenticated(const QHostAddress& address)
{
	if (address.isNull())
	{
		return;
	}

	QMutexLocker locker(&m_mutex);

	releaseSlot();

	if (isLoopback(address))
	{
		return;
	}

	const auto now = m_nowMs();
	auto& entry = entryFor(keyFor(address, m_isOnLink), now);

	if (isTrusted(entry, now) == false)
	{
		// start with a full, separate bucket so that failures of other users of this address
		// cannot have drained the budget of the authenticated peer
		entry.trustedBucket = {m_config.perIpBurst, now};
	}

	entry.trustedUntilMs = now + static_cast<qint64>(TrustedPeerSeconds) * 1000;
	entry.failures = 0;
	entry.blockedUntilMs = 0;
}



void ConnectionLimiter::closed(const QHostAddress& address, bool authFailed)
{
	if (address.isNull())
	{
		return;
	}

	QMutexLocker locker(&m_mutex);

	releaseSlot();

	if (authFailed == false || isLoopback(address) || m_config.backoffMaxSeconds <= 0)
	{
		return;
	}

	const auto now = m_nowMs();
	auto& entry = entryFor(keyFor(address, m_isOnLink), now);

	if (isTrusted(entry, now))
	{
		return;
	}

	entry.failures = std::min(entry.failures + 1, 62);

	// base 1 s, doubled with every consecutive failure
	const auto backoffSeconds = std::min<qint64>(m_config.backoffMaxSeconds, qint64{1} << (entry.failures - 1));
	entry.blockedUntilMs = now + backoffSeconds * 1000;

	vDebug() << "backing off" << address.toString() << "for" << backoffSeconds << "s after failed authentication";
}



int ConnectionLimiter::preAuthCount() const
{
	QMutexLocker locker(&m_mutex);

	return m_preAuthCount;
}



int ConnectionLimiter::trackedAddressCount() const
{
	QMutexLocker locker(&m_mutex);

	return static_cast<int>(m_entries.size());
}



QByteArray ConnectionLimiter::keyFor(const QHostAddress& address, const OnLinkResolver& isOnLink)
{
	if (address.protocol() == QAbstractSocket::IPv4Protocol)
	{
		const auto ipv4 = address.toIPv4Address();
		QByteArray key("4");
		for (int shift = 24; shift >= 0; shift -= 8)
		{
			key.append(static_cast<char>((ipv4 >> shift) & 0xff));
		}
		return key;
	}

	const auto ipv6 = address.toIPv6Address();

	QByteArray bytes(reinterpret_cast<const char*>(ipv6.c), 16);

	// IPv4-mapped addresses (::ffff:a.b.c.d) are accepted on dual-stack sockets and belong to the IPv4 bucket
	static const QByteArray MappedPrefix = QByteArray::fromHex("00000000000000000000ffff");
	if (bytes.startsWith(MappedPrefix))
	{
		return QByteArray("4") + bytes.mid(12, 4);
	}

	// link-local (fe80::/10) and on-link sources are keyed by full address, everything else by /64
	const bool linkLocal = (static_cast<uchar>(bytes.at(0)) == 0xfe) && ((static_cast<uchar>(bytes.at(1)) & 0xc0) == 0x80);
	if (linkLocal || (isOnLink && isOnLink(address)))
	{
		return QByteArray("6") + bytes;
	}

	return QByteArray("P") + bytes.left(8);
}



bool ConnectionLimiter::isLoopback(const QHostAddress& address)
{
	if (address.protocol() == QAbstractSocket::IPv4Protocol)
	{
		return (address.toIPv4Address() >> 24) == 127;
	}

	const auto ipv6 = address.toIPv6Address();
	const QByteArray bytes(reinterpret_cast<const char*>(ipv6.c), 16);

	if (bytes == QByteArray::fromHex("00000000000000000000000000000001"))
	{
		return true;
	}

	// ::ffff:127.0.0.0/104
	return bytes.startsWith(QByteArray::fromHex("00000000000000000000ffff7f"));
}



ConnectionLimiter::Entry& ConnectionLimiter::entryFor(const QByteArray& key, qint64 now)
{
	const auto it = m_index.constFind(key);
	if (it != m_index.constEnd())
	{
		// mark as most recently used
		m_entries.splice(m_entries.begin(), m_entries, it.value());
		return m_entries.front();
	}

	if (static_cast<int>(m_entries.size()) >= MaxTrackedAddresses)
	{
		m_index.remove(m_entries.back().key);
		m_entries.pop_back();
	}

	Entry entry;
	entry.key = key;
	entry.bucket = {m_config.perIpBurst, now};

	m_entries.push_front(entry);
	m_index.insert(key, m_entries.begin());

	return m_entries.front();
}



bool ConnectionLimiter::takeToken(Bucket& bucket, qint64 now) const
{
	const qint64 refillMs = static_cast<qint64>(m_config.perIpRefillSeconds) * 1000;

	if (bucket.tokens >= m_config.perIpBurst)
	{
		// a full bucket does not accumulate partial refill time
		bucket.lastRefillMs = now;
	}
	else
	{
		const auto elapsed = std::max<qint64>(now - bucket.lastRefillMs, 0);
		const auto refilled = elapsed / refillMs;

		if (refilled > 0)
		{
			bucket.tokens = static_cast<int>(std::min<qint64>(m_config.perIpBurst, bucket.tokens + refilled));
			bucket.lastRefillMs = bucket.tokens >= m_config.perIpBurst ? now : bucket.lastRefillMs + refilled * refillMs;
		}
	}

	if (bucket.tokens <= 0)
	{
		return false;
	}

	--bucket.tokens;

	return true;
}



bool ConnectionLimiter::isTrusted(Entry& entry, qint64 now) const
{
	if (entry.trustedUntilMs == 0)
	{
		return false;
	}

	if (now >= entry.trustedUntilMs)
	{
		entry.trustedUntilMs = 0;
		return false;
	}

	return true;
}



void ConnectionLimiter::releaseSlot()
{
	if (m_preAuthCount > 0)
	{
		--m_preAuthCount;
	}
}
