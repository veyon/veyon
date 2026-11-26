// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTest>

#include "InternetBlockAddressRanges.h"
#include "InternetBlocker.h"

class InternetBlockTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void parsesNetworks()
	{
		QStringList invalid;
		QStringList ipv6;
		const auto ranges = InternetBlockAddressRanges::parseIPv4Networks(
			InternetBlockAddressRanges::splitNetworkList(
				QStringLiteral("10.1.2.3, 192.168.5.77/24;bogus\n2001:db8::/32 300.1.1.1 1.2.3.4/33")),
			&invalid, &ipv6);

		QCOMPARE(ranges.size(), 2);
		QCOMPARE(InternetBlockAddressRanges::formatRanges(ranges),
				 QStringList({QStringLiteral("10.1.2.3"), QStringLiteral("192.168.5.0-192.168.5.255")}));
		QCOMPARE(invalid, QStringList({QStringLiteral("bogus"), QStringLiteral("300.1.1.1"), QStringLiteral("1.2.3.4/33")}));
		QCOMPARE(ipv6, QStringList({QStringLiteral("2001:db8::/32")}));
	}

	void mergesRanges()
	{
		const auto merged = InternetBlockAddressRanges::mergeRanges(
			InternetBlockAddressRanges::parseIPv4Networks({QStringLiteral("10.0.0.0/9"), QStringLiteral("10.128.0.0/9"),
														   QStringLiteral("10.5.0.0/16"), QStringLiteral("11.0.0.0/8"),
														   QStringLiteral("255.255.255.255")}));
		QCOMPARE(InternetBlockAddressRanges::formatRanges(merged),
				 QStringList({QStringLiteral("10.0.0.0-11.255.255.255"), QStringLiteral("255.255.255.255")}));
	}

	void complementOfDefaults()
	{
		const auto blocked = InternetBlockAddressRanges::blockedIPv4Ranges(
			InternetBlockAddressRanges::defaultAllowedIPv4Networks());

		QCOMPARE(blocked, QStringList({
			QStringLiteral("0.0.0.1-9.255.255.255"),
			QStringLiteral("11.0.0.0-126.255.255.255"),
			QStringLiteral("128.0.0.0-169.253.255.255"),
			QStringLiteral("169.255.0.0-172.15.255.255"),
			QStringLiteral("172.32.0.0-192.167.255.255"),
			QStringLiteral("192.169.0.0-223.255.255.255"),
			QStringLiteral("240.0.0.0-255.255.255.254"),
		}));
	}

	void complementEdgeCases()
	{
		QCOMPARE(InternetBlockAddressRanges::blockedIPv4Ranges({}),
				 QStringList({QStringLiteral("0.0.0.0-255.255.255.255")}));
		QCOMPARE(InternetBlockAddressRanges::blockedIPv4Ranges({QStringLiteral("0.0.0.0/0")}), QStringList());
		QCOMPARE(InternetBlockAddressRanges::blockedIPv4Ranges({QStringLiteral("0.0.0.0/1")}),
				 QStringList({QStringLiteral("128.0.0.0-255.255.255.255")}));
		QCOMPARE(InternetBlockAddressRanges::blockedIPv4Ranges({QStringLiteral("128.0.0.0/1")}),
				 QStringList({QStringLiteral("0.0.0.0-127.255.255.255")}));
	}

	void nftablesScript()
	{
		InternetBlocker::Settings settings;
		settings.allowedIPv4Networks = {QStringLiteral("192.168.0.0/16"), QStringLiteral("10.0.0.1")};
		settings.allowedIPv6Networks = {QStringLiteral("fe80::/10")};
		settings.blockIPv6 = true;

		const auto script = InternetBlocker::nftablesScript(settings);
		QVERIFY(script.contains(QStringLiteral("ip daddr { 10.0.0.1, 192.168.0.0-192.168.255.255 } accept")));
		QVERIFY(script.contains(QStringLiteral("ip6 daddr { fe80::/10 } accept")));
		QVERIFY(script.contains(QStringLiteral("meta nfproto ipv6 reject")));

		settings.blockIPv6 = false;
		QVERIFY(InternetBlocker::nftablesScript(settings).contains(QStringLiteral("ipv6")) == false);
	}
};

QTEST_GUILESS_MAIN(InternetBlockTest)
#include "InternetBlockTest.moc"
