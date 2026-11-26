// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QProcess>
#include <QStandardPaths>

#include "InternetBlockAddressRanges.h"
#include "InternetBlocker.h"


static QString nftExecutable()
{
	auto path = QStandardPaths::findExecutable(QStringLiteral("nft"));
	if (path.isEmpty())
	{
		path = QStandardPaths::findExecutable(QStringLiteral("nft"),
											  {QStringLiteral("/usr/sbin"), QStringLiteral("/sbin"),
											   QStringLiteral("/usr/bin")});
	}
	return path;
}



static bool runNft(const QStringList& arguments, const QByteArray& input = {}, bool quiet = false)
{
	const auto nft = nftExecutable();
	if (nft.isEmpty())
	{
		vCritical() << "nft (nftables) not found - can't block or unblock internet access";
		return false;
	}

	QProcess process;
	process.start(nft, arguments);
	if (process.waitForStarted() == false)
	{
		vCritical() << "failed to start" << nft;
		return false;
	}

	if (input.isEmpty() == false)
	{
		process.write(input);
	}
	process.closeWriteChannel();

	if (process.waitForFinished() == false)
	{
		vCritical() << "nft did not finish";
		process.kill();
		return false;
	}

	if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
	{
		if (quiet == false)
		{
			vCritical() << "nft failed:" << process.exitCode() << process.readAllStandardError();
		}
		return false;
	}

	return true;
}



QString InternetBlocker::nftablesScript(const Settings& settings)
{
	const auto table = tableName();

	// canonicalize IPv4 networks (also drops invalid entries)
	const auto ipv4Ranges = InternetBlockAddressRanges::mergeRanges(
		InternetBlockAddressRanges::parseIPv4Networks(settings.allowedIPv4Networks));

	QStringList ipv4Elements;
	for (const auto& range : ipv4Ranges)
	{
		ipv4Elements.append(range.first == range.second ?
								InternetBlockAddressRanges::ipv4ToString(range.first) :
								InternetBlockAddressRanges::ipv4ToString(range.first) + QLatin1Char('-') +
									InternetBlockAddressRanges::ipv4ToString(range.second));
	}

	QStringList lines{
		// create (if missing) and delete the table first so the whole script is
		// applied atomically and repeated invocations don't fail
		QStringLiteral("table inet %1 {}").arg(table),
		QStringLiteral("delete table inet %1").arg(table),
		QStringLiteral("table inet %1 {").arg(table),
		QStringLiteral("\tchain output {"),
		QStringLiteral("\t\ttype filter hook output priority filter; policy accept;"),
		QStringLiteral("\t\toifname \"lo\" accept"),
	};

	if (ipv4Elements.isEmpty() == false)
	{
		lines.append(QStringLiteral("\t\tip daddr { %1 } accept").arg(ipv4Elements.join(QStringLiteral(", "))));
	}
	lines.append(QStringLiteral("\t\tmeta nfproto ipv4 reject"));

	if (settings.blockIPv6)
	{
		if (settings.allowedIPv6Networks.isEmpty() == false)
		{
			lines.append(QStringLiteral("\t\tip6 daddr { %1 } accept").arg(settings.allowedIPv6Networks.join(QStringLiteral(", "))));
		}
		lines.append(QStringLiteral("\t\tmeta nfproto ipv6 reject"));
	}

	lines.append(QStringLiteral("\t}"));
	lines.append(QStringLiteral("}"));

	return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}



bool InternetBlocker::block(const Settings& settings)
{
	const auto script = nftablesScript(settings);
	vDebug() << "applying nftables ruleset" << script;
	return runNft({QStringLiteral("-f"), QStringLiteral("-")}, script.toUtf8());
}



bool InternetBlocker::unblock()
{
	if (isBlocked() == false)
	{
		return true;
	}

	return runNft({QStringLiteral("delete"), QStringLiteral("table"), QStringLiteral("inet"), tableName()});
}



bool InternetBlocker::isBlocked()
{
	return runNft({QStringLiteral("list"), QStringLiteral("table"), QStringLiteral("inet"), tableName()}, {}, true);
}
