// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>
#include <QTemporaryFile>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "ConnectionLimiter.h"
#include "VeyonConfiguration.h"

#include <memory>
#include <utility>


class ConnectionLimiterTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void burstIsLimitedPerAddress();
	void refillAddsOneTokenPerInterval();
	void bucketNeverExceedsBurst();
	void refusedAdmitConsumesNothing();
	void backoffGrowsAndIsCapped();
	void zeroBackoffCapDisablesBackoff();
	void failureWithoutAuthFailureDoesNotBackOff();
	void authenticatedResetsFailures();
	void preAuthMaxCapsConcurrentConnections();
	void closedFreesSlotAndNoLeak();
	void authenticatedFreesSlot();
	void strayReleasesDoNotUnderflow();
	void lruEvictsLeastRecentlyUsed();
	void lruKeepsRecentlyUsed();
	void loopbackIsOnlyLimitedByPreAuthMax();
	void ipv4MappedSharesIpv4State();
	void ipv6OffLinkIsGroupedBy64();
	void ipv6LinkLocalIsKeyedByFullAddress();
	void ipv6OnLinkIsKeyedByFullAddress();
	void nullAddressIsRefused();
	void authenticatedPeerKeepsSeparateBudget();
	void sharedAddressLockoutDoesNotBlockTrustedPeer();
	void trustedPeerIsExemptFromBackoff();
	void trustedPeerExpires();
	void trustIsPerAddress();
	void demoConfigurationAdmits64ViewersFromOneAddress();
	void configurationFactoryUsesNetworkConnectionLimitKeys();
	void configurationDefaultsMatchSpecification();
	void invalidConfigurationIsClamped();
	void concurrentAccessKeepsCountersConsistent();

private:
	static constexpr qint64 Second = 1000;

	struct TestClock
	{
		qint64 nowMs = 1000000;
		ConnectionLimiter::Clock clock()
		{
			return [this]() { return nowMs; };
		}
		void advanceSeconds(int seconds) { nowMs += seconds * Second; }
	};

	static ConnectionLimiter::Config defaultConfig()
	{
		return {32, 10, 6, 300};
	}

	static ConnectionLimiter::OnLinkResolver neverOnLink()
	{
		return [](const QHostAddress&) { return false; };
	}

	// admits and immediately closes a connection (without failure)
	static bool tryOnce(ConnectionLimiter& limiter, const QHostAddress& address)
	{
		if (limiter.admit(address))
		{
			limiter.closed(address, false);
			return true;
		}
		return false;
	}
};



void ConnectionLimiterTest::burstIsLimitedPerAddress()
{
	TestClock clock;
	ConnectionLimiter limiter(defaultConfig(), clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));
	const QHostAddress other(QStringLiteral("192.168.1.11"));

	for (int i = 0; i < 10; ++i)
	{
		QVERIFY(tryOnce(limiter, address));
	}
	QVERIFY(tryOnce(limiter, address) == false);

	// other addresses have their own bucket
	QVERIFY(tryOnce(limiter, other));
}



void ConnectionLimiterTest::refillAddsOneTokenPerInterval()
{
	TestClock clock;
	ConnectionLimiter limiter(defaultConfig(), clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	for (int i = 0; i < 10; ++i)
	{
		QVERIFY(tryOnce(limiter, address));
	}
	QVERIFY(tryOnce(limiter, address) == false);

	clock.advanceSeconds(5);
	QVERIFY(tryOnce(limiter, address) == false);

	clock.advanceSeconds(1);
	QVERIFY(tryOnce(limiter, address));
	QVERIFY(tryOnce(limiter, address) == false);

	// two intervals give exactly two tokens
	clock.advanceSeconds(12);
	QVERIFY(tryOnce(limiter, address));
	QVERIFY(tryOnce(limiter, address));
	QVERIFY(tryOnce(limiter, address) == false);
}



void ConnectionLimiterTest::bucketNeverExceedsBurst()
{
	TestClock clock;
	ConnectionLimiter limiter(defaultConfig(), clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	QVERIFY(tryOnce(limiter, address));

	clock.advanceSeconds(100000);

	for (int i = 0; i < 10; ++i)
	{
		QVERIFY(tryOnce(limiter, address));
	}
	QVERIFY(tryOnce(limiter, address) == false);
}



void ConnectionLimiterTest::refusedAdmitConsumesNothing()
{
	TestClock clock;
	ConnectionLimiter limiter({2, 3, 6, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));
	const QHostAddress other(QStringLiteral("192.168.1.11"));

	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(address));
	// refused by preAuthMax: must not consume a token of the address
	QVERIFY(limiter.admit(address) == false);
	QCOMPARE(limiter.preAuthCount(), 2);

	limiter.closed(address, false);
	limiter.closed(address, false);

	// third token still available
	QVERIFY(limiter.admit(address));
	limiter.closed(address, false);
	QVERIFY(limiter.admit(address) == false);
	QCOMPARE(limiter.preAuthCount(), 0);

	QVERIFY(tryOnce(limiter, other));
}



void ConnectionLimiterTest::backoffGrowsAndIsCapped()
{
	TestClock clock;
	// plenty of tokens so that only the backoff matters
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	// consecutive failures: 1 s, doubled with every failure, capped at 300 s
	const int expectedBackoffSeconds[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 300, 300, 300};

	for (const auto expected : expectedBackoffSeconds)
	{
		QVERIFY(limiter.admit(address));
		limiter.closed(address, true);

		// refused until the end of the backoff period ...
		clock.nowMs += expected * Second - 1;
		QVERIFY2(limiter.admit(address) == false, qPrintable(QStringLiteral("expected backoff %1 s").arg(expected)));
		QCOMPARE(limiter.preAuthCount(), 0);

		// ... and admitted right after it
		clock.nowMs += 1;
	}

	QVERIFY(limiter.admit(address));
	limiter.closed(address, false);
}



void ConnectionLimiterTest::zeroBackoffCapDisablesBackoff()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 0}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	for (int i = 0; i < 50; ++i)
	{
		QVERIFY(limiter.admit(address));
		limiter.closed(address, true);
	}
}



void ConnectionLimiterTest::failureWithoutAuthFailureDoesNotBackOff()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	for (int i = 0; i < 50; ++i)
	{
		QVERIFY(limiter.admit(address));
		limiter.closed(address, false);
	}
}



void ConnectionLimiterTest::authenticatedResetsFailures()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	// build up a backoff of 4 s
	for (int i = 0; i < 3; ++i)
	{
		QVERIFY(limiter.admit(address));
		limiter.closed(address, true);
		clock.advanceSeconds(10);
	}

	QVERIFY(limiter.admit(address));
	limiter.authenticated(address);

	// no backoff pending right after a successful authentication
	QVERIFY(limiter.admit(address));
	limiter.closed(address, false);
}



void ConnectionLimiterTest::preAuthMaxCapsConcurrentConnections()
{
	TestClock clock;
	ConnectionLimiter limiter({3, 1000, 1, 300}, clock.clock(), neverOnLink());

	QVERIFY(limiter.admit(QHostAddress(QStringLiteral("192.168.1.1"))));
	QVERIFY(limiter.admit(QHostAddress(QStringLiteral("192.168.1.2"))));
	QVERIFY(limiter.admit(QHostAddress(QStringLiteral("192.168.1.3"))));
	QVERIFY(limiter.admit(QHostAddress(QStringLiteral("192.168.1.4"))) == false);
	QCOMPARE(limiter.preAuthCount(), 3);
}



void ConnectionLimiterTest::closedFreesSlotAndNoLeak()
{
	TestClock clock;
	ConnectionLimiter limiter({2, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));
	const QHostAddress other(QStringLiteral("192.168.1.11"));

	// many successive connections that close (also with failures) never leak a slot
	for (int i = 0; i < 100; ++i)
	{
		QVERIFY(limiter.admit(other));
		limiter.closed(other, i % 2 == 0);
		clock.advanceSeconds(1000);
	}
	QCOMPARE(limiter.preAuthCount(), 0);

	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(other) == false);

	limiter.closed(address, true);
	QCOMPARE(limiter.preAuthCount(), 1);
	QVERIFY(limiter.admit(other));
}



void ConnectionLimiterTest::authenticatedFreesSlot()
{
	TestClock clock;
	ConnectionLimiter limiter({1, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));
	const QHostAddress other(QStringLiteral("192.168.1.11"));

	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(other) == false);

	limiter.authenticated(address);
	QCOMPARE(limiter.preAuthCount(), 0);
	QVERIFY(limiter.admit(other));
}



void ConnectionLimiterTest::strayReleasesDoNotUnderflow()
{
	TestClock clock;
	ConnectionLimiter limiter({1, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	limiter.closed(address, false);
	limiter.authenticated(address);
	QCOMPARE(limiter.preAuthCount(), 0);

	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(address) == false);
}



void ConnectionLimiterTest::lruEvictsLeastRecentlyUsed()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());

	// the first address earns a long backoff
	const QHostAddress victim(QStringLiteral("10.0.0.1"));
	for (int i = 0; i < 8; ++i)
	{
		QVERIFY(limiter.admit(victim));
		limiter.closed(victim, true);
		clock.advanceSeconds(300);
	}
	QVERIFY(limiter.admit(victim));
	limiter.closed(victim, true);
	QVERIFY(limiter.admit(victim) == false);

	// fill the table with other addresses
	const quint32 base = QHostAddress(QStringLiteral("10.1.0.0")).toIPv4Address();
	for (int i = 0; i < ConnectionLimiter::MaxTrackedAddresses; ++i)
	{
		QVERIFY(tryOnce(limiter, QHostAddress(base + static_cast<quint32>(i))));
	}

	QCOMPARE(limiter.trackedAddressCount(), ConnectionLimiter::MaxTrackedAddresses);

	// the victim was the least recently used entry and is gone including its backoff state
	QVERIFY(limiter.admit(victim));
	limiter.closed(victim, false);
	QCOMPARE(limiter.trackedAddressCount(), ConnectionLimiter::MaxTrackedAddresses);
}



void ConnectionLimiterTest::lruKeepsRecentlyUsed()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());

	const QHostAddress blocked(QStringLiteral("10.0.0.1"));
	QVERIFY(limiter.admit(blocked));
	limiter.closed(blocked, true);

	const quint32 base = QHostAddress(QStringLiteral("10.1.0.0")).toIPv4Address();
	for (int i = 0; i < ConnectionLimiter::MaxTrackedAddresses * 2; ++i)
	{
		QVERIFY(tryOnce(limiter, QHostAddress(base + static_cast<quint32>(i))));

		// touching the blocked address keeps it at the front of the LRU list (refused: backoff 1 s)
		if (i % 100 == 0)
		{
			QVERIFY(limiter.admit(blocked) == false);
		}
	}

	QCOMPARE(limiter.trackedAddressCount(), ConnectionLimiter::MaxTrackedAddresses);
	QVERIFY(limiter.admit(blocked) == false);
}



void ConnectionLimiterTest::loopbackIsOnlyLimitedByPreAuthMax()
{
	TestClock clock;
	ConnectionLimiter limiter({5, 2, 6, 300}, clock.clock(), neverOnLink());
	const QHostAddress loopback4(QStringLiteral("127.0.0.1"));
	const QHostAddress loopback6(QStringLiteral("::1"));

	// neither burst nor backoff apply
	for (int i = 0; i < 50; ++i)
	{
		QVERIFY(limiter.admit(loopback4));
		limiter.closed(loopback4, true);
		QVERIFY(limiter.admit(loopback6));
		limiter.closed(loopback6, true);
	}
	QCOMPARE(limiter.trackedAddressCount(), 0);

	// but preAuthMax does
	for (int i = 0; i < 5; ++i)
	{
		QVERIFY(limiter.admit(loopback4));
	}
	QVERIFY(limiter.admit(loopback4) == false);
	QVERIFY(limiter.admit(QHostAddress(QStringLiteral("192.168.1.1"))) == false);
}



void ConnectionLimiterTest::ipv4MappedSharesIpv4State()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 2, 6, 300}, clock.clock(), neverOnLink());

	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("192.168.1.10"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("::ffff:192.168.1.10"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("192.168.1.10"))) == false);
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("::ffff:192.168.1.10"))) == false);

	// mapped loopback is loopback
	ConnectionLimiter loopbackLimiter({32, 1, 6, 300}, clock.clock(), neverOnLink());
	for (int i = 0; i < 5; ++i)
	{
		QVERIFY(tryOnce(loopbackLimiter, QHostAddress(QStringLiteral("::ffff:127.0.0.1"))));
	}
}



void ConnectionLimiterTest::ipv6OffLinkIsGroupedBy64()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 3, 6, 300}, clock.clock(), neverOnLink());

	// same /64, different interface identifiers
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("2001:db8:1:2::1"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("2001:db8:1:2:aaaa:bbbb:cccc:dddd"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("2001:db8:1:2::3"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("2001:db8:1:2::4"))) == false);

	// a different /64 has its own bucket
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("2001:db8:1:3::1"))));

	// failure backoff is shared inside the /64 as well
	TestClock clock2;
	ConnectionLimiter limiter2({32, 100, 6, 300}, clock2.clock(), neverOnLink());
	QVERIFY(limiter2.admit(QHostAddress(QStringLiteral("2001:db8:1:2::1"))));
	limiter2.closed(QHostAddress(QStringLiteral("2001:db8:1:2::1")), true);
	QVERIFY(limiter2.admit(QHostAddress(QStringLiteral("2001:db8:1:2::2"))) == false);
	QVERIFY(limiter2.admit(QHostAddress(QStringLiteral("2001:db8:1:9::2"))));
}



void ConnectionLimiterTest::ipv6LinkLocalIsKeyedByFullAddress()
{
	TestClock clock;
	// resolver says "not on-link": link-local sources must be keyed by full address nevertheless
	ConnectionLimiter limiter({32, 2, 6, 300}, clock.clock(), neverOnLink());

	const QHostAddress first(QStringLiteral("fe80::1"));
	const QHostAddress second(QStringLiteral("fe80::2"));

	QVERIFY(tryOnce(limiter, first));
	QVERIFY(tryOnce(limiter, first));
	QVERIFY(tryOnce(limiter, first) == false);

	QVERIFY(tryOnce(limiter, second));
	QVERIFY(tryOnce(limiter, second));
	QVERIFY(tryOnce(limiter, second) == false);

	// scope IDs do not create additional buckets for the same address
	QHostAddress scoped(QStringLiteral("fe80::1%eth0"));
	QVERIFY(tryOnce(limiter, scoped) == false);

	// febf::/10 end of the link-local range, fec0:: is outside
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("febf::1"))));
	QVERIFY(tryOnce(limiter, QHostAddress(QStringLiteral("febf::2"))));
}



void ConnectionLimiterTest::ipv6OnLinkIsKeyedByFullAddress()
{
	TestClock clock;
	QHostAddress localSubnet(QStringLiteral("2001:db8:aaaa:1::"));
	const auto isOnLink = [&localSubnet](const QHostAddress& address) {
		return address.isInSubnet(localSubnet, 64);
	};
	ConnectionLimiter limiter({32, 2, 6, 300}, clock.clock(), isOnLink);

	// on-link: each host has its own bucket and its own backoff
	const QHostAddress teacher(QStringLiteral("2001:db8:aaaa:1::10"));
	const QHostAddress student(QStringLiteral("2001:db8:aaaa:1::20"));

	QVERIFY(limiter.admit(student));
	limiter.closed(student, true);
	QVERIFY(limiter.admit(student) == false);

	QVERIFY(limiter.admit(teacher));
	limiter.closed(teacher, false);
	QVERIFY(limiter.admit(teacher));
	limiter.closed(teacher, false);

	// off-link /64 is still grouped
	const QHostAddress remote1(QStringLiteral("2001:db8:bbbb:1::1"));
	const QHostAddress remote2(QStringLiteral("2001:db8:bbbb:1::2"));
	QVERIFY(tryOnce(limiter, remote1));
	QVERIFY(tryOnce(limiter, remote2));
	QVERIFY(tryOnce(limiter, remote1) == false);
}



void ConnectionLimiterTest::nullAddressIsRefused()
{
	ConnectionLimiter limiter(defaultConfig());
	QVERIFY(limiter.admit(QHostAddress()) == false);
	QCOMPARE(limiter.preAuthCount(), 0);
}



void ConnectionLimiterTest::authenticatedPeerKeepsSeparateBudget()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 3, 6, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	// a successful authentication creates a separate bucket of perIpBurst tokens
	QVERIFY(limiter.admit(address));
	limiter.authenticated(address);

	// the regular bucket is not used any longer: exactly burst more attempts, then refused
	for (int i = 0; i < 3; ++i)
	{
		QVERIFY(tryOnce(limiter, address));
	}
	QVERIFY(tryOnce(limiter, address) == false);

	// trusted bucket refills as usual
	clock.advanceSeconds(6);
	QVERIFY(tryOnce(limiter, address));
	QVERIFY(tryOnce(limiter, address) == false);
}



void ConnectionLimiterTest::sharedAddressLockoutDoesNotBlockTrustedPeer()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 10, 6, 300}, clock.clock(), neverOnLink());
	const QHostAddress sharedAddress(QStringLiteral("192.168.1.10"));

	// the teacher's master authenticated from this address (e.g. terminal server or NAT)
	QVERIFY(limiter.admit(sharedAddress));
	limiter.authenticated(sharedAddress);

	// a student behind the same address fails repeatedly
	for (int i = 0; i < 5; ++i)
	{
		QVERIFY(limiter.admit(sharedAddress));
		limiter.closed(sharedAddress, true);
		clock.advanceSeconds(1);
	}

	// the master is not locked out (no backoff) ...
	QVERIFY(limiter.admit(sharedAddress));
	limiter.authenticated(sharedAddress);

	// ... also several minutes later, as long as the 30 minute window is refreshed
	clock.advanceSeconds(20 * 60);
	QVERIFY(limiter.admit(sharedAddress));
	limiter.closed(sharedAddress, true);
	QVERIFY(limiter.admit(sharedAddress));
	limiter.authenticated(sharedAddress);
	clock.advanceSeconds(20 * 60);
	QVERIFY(limiter.admit(sharedAddress));
	limiter.closed(sharedAddress, false);

	// whereas an address that never authenticated is locked out after failures
	const QHostAddress otherAddress(QStringLiteral("192.168.1.11"));
	QVERIFY(limiter.admit(otherAddress));
	limiter.closed(otherAddress, true);
	QVERIFY(limiter.admit(otherAddress) == false);
}



void ConnectionLimiterTest::trustedPeerIsExemptFromBackoff()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	// earn a backoff first
	QVERIFY(limiter.admit(address));
	limiter.closed(address, true);
	QVERIFY(limiter.admit(address) == false);
	clock.advanceSeconds(1);

	QVERIFY(limiter.admit(address));
	limiter.authenticated(address);

	for (int i = 0; i < 20; ++i)
	{
		QVERIFY(limiter.admit(address));
		limiter.closed(address, true);
	}
}



void ConnectionLimiterTest::trustedPeerExpires()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	QVERIFY(limiter.admit(address));
	limiter.authenticated(address);

	clock.nowMs += ConnectionLimiter::TrustedPeerSeconds * Second - 1;
	QVERIFY(limiter.admit(address));
	limiter.closed(address, true);
	QVERIFY(limiter.admit(address));
	limiter.closed(address, true);

	clock.nowMs += 1;

	// trust has expired: failures count and back off again
	QVERIFY(limiter.admit(address));
	limiter.closed(address, true);
	QVERIFY(limiter.admit(address) == false);
}



void ConnectionLimiterTest::trustIsPerAddress()
{
	TestClock clock;
	ConnectionLimiter limiter({32, 1000, 1, 300}, clock.clock(), neverOnLink());
	const QHostAddress teacher(QStringLiteral("192.168.1.10"));
	const QHostAddress student(QStringLiteral("192.168.1.20"));

	QVERIFY(limiter.admit(teacher));
	limiter.authenticated(teacher);

	QVERIFY(limiter.admit(student));
	limiter.closed(student, true);
	QVERIFY(limiter.admit(student) == false);
}



void ConnectionLimiterTest::demoConfigurationAdmits64ViewersFromOneAddress()
{
	TestClock clock;
	// DemoServer configuration (05 §8)
	ConnectionLimiter limiter({256, 64, 1, 0}, clock.clock(), neverOnLink());
	const QHostAddress terminalServer(QStringLiteral("192.168.1.10"));

	for (int i = 0; i < 64; ++i)
	{
		QVERIFY2(limiter.admit(terminalServer), qPrintable(QStringLiteral("viewer %1").arg(i)));
	}
	QCOMPARE(limiter.preAuthCount(), 64);

	// failures never lead to a per-IP backoff
	limiter.closed(terminalServer, true);
	clock.advanceSeconds(1);
	QVERIFY(limiter.admit(terminalServer));
}



void ConnectionLimiterTest::configurationFactoryUsesNetworkConnectionLimitKeys()
{
	QTemporaryFile file;
	QVERIFY(file.open());
	file.write(R"({"Network":{"ConnectionLimit":{"PreAuthMax":7,"PerIpBurst":8,"PerIpRefillSeconds":9,"AuthFailureBackoffMaxSeconds":11,"PreAuthTimeoutSeconds":13}}})");
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));

	QCOMPARE(configuration.connectionLimitPreAuthMax(), 7);
	QCOMPARE(configuration.connectionLimitPerIpBurst(), 8);
	QCOMPARE(configuration.connectionLimitPerIpRefillSeconds(), 9);
	QCOMPARE(configuration.connectionLimitAuthFailureBackoffMaxSeconds(), 11);
	QCOMPARE(configuration.connectionLimitPreAuthTimeoutSeconds(), 13);

	const auto config = ConnectionLimiter::configFromConfiguration(configuration);
	QCOMPARE(config.preAuthMax, 7);
	QCOMPARE(config.perIpBurst, 8);
	QCOMPARE(config.perIpRefillSeconds, 9);
	QCOMPARE(config.backoffMaxSeconds, 11);
}



void ConnectionLimiterTest::configurationDefaultsMatchSpecification()
{
	QTemporaryFile file;
	QVERIFY(file.open());
	file.write("{}");
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));

	QCOMPARE(configuration.connectionLimitPreAuthMax(), 32);
	QCOMPARE(configuration.connectionLimitPerIpBurst(), 10);
	QCOMPARE(configuration.connectionLimitPerIpRefillSeconds(), 6);
	QCOMPARE(configuration.connectionLimitAuthFailureBackoffMaxSeconds(), 300);
	QCOMPARE(configuration.connectionLimitPreAuthTimeoutSeconds(), 10);

	const auto config = ConnectionLimiter::configFromConfiguration(configuration);
	QCOMPARE(config.preAuthMax, 32);
	QCOMPARE(config.perIpBurst, 10);
	QCOMPARE(config.perIpRefillSeconds, 6);
	QCOMPARE(config.backoffMaxSeconds, 300);
}



void ConnectionLimiterTest::invalidConfigurationIsClamped()
{
	TestClock clock;
	// zero/negative values must neither crash (division by zero) nor open the limiter
	ConnectionLimiter limiter({0, 0, 0, -5}, clock.clock(), neverOnLink());
	const QHostAddress address(QStringLiteral("192.168.1.10"));

	QVERIFY(limiter.admit(address));
	QVERIFY(limiter.admit(address) == false);
	limiter.closed(address, true);
	QCOMPARE(limiter.preAuthCount(), 0);
	QVERIFY(limiter.admit(address) == false);
	clock.advanceSeconds(1);
	QVERIFY(limiter.admit(address));
}



void ConnectionLimiterTest::concurrentAccessKeepsCountersConsistent()
{
	// default clock and on-link resolver: exercises the real code paths from several threads
	ConnectionLimiter limiter({1000000, 1000000, 1, 0});

	constexpr int ThreadCount = 4;
	constexpr int Iterations = 5000;

	QList<QThread*> threads;
	for (int t = 0; t < ThreadCount; ++t)
	{
		threads.append(QThread::create([&limiter, t]() {
			const QHostAddress address(QStringLiteral("10.0.%1.1").arg(t));
			const QHostAddress ipv6Address(QStringLiteral("2001:db8:%1::1").arg(t));
			for (int i = 0; i < Iterations; ++i)
			{
				const auto& current = (i % 2 == 0) ? address : ipv6Address;
				if (limiter.admit(current))
				{
					if (i % 3 == 0)
					{
						limiter.authenticated(current);
					}
					else
					{
						limiter.closed(current, i % 3 == 1);
					}
				}
			}
		}));
	}

	for (auto* thread : std::as_const(threads))
	{
		thread->start();
	}
	for (auto* thread : std::as_const(threads))
	{
		QVERIFY(thread->wait(60000));
		delete thread;
	}

	QCOMPARE(limiter.preAuthCount(), 0);
	QVERIFY(limiter.trackedAddressCount() <= 2 * ThreadCount);
}


QTEST_MAIN(ConnectionLimiterTest)

#include "ConnectionLimiterTest.moc"
