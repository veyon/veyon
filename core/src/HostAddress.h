// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class VEYON_CORE_EXPORT HostAddress
{
	Q_GADGET
public:
	enum class Type {
		Invalid,
		IpAddress,
		HostName,
		FullyQualifiedDomainName
	};
	Q_ENUM(Type)

	explicit HostAddress(const QString& address);
	~HostAddress() = default;

	Type type() const
	{
		return m_type;
	}

	bool isLocalHost() const;

	QString convert(Type targetType) const;
	QString tryConvert(Type targetType) const;

	QStringList lookupIpAddresses() const;

	static QString parseHost(const QString& address);
	static int parsePortNumber(const QString& address);

	static QString localFQDN();

private:
	static Type determineType(const QString& address);
	static QString toIpAddress(const QString& hostName);
	static QString toHostName(Type type, const QString& address);
	static QString toFQDN(Type type, const QString& address);
	static QString fqdnToHostName(const QString& fqdn);
	static QString resolveLocalFQDN();

	static QString s_cachedLocalFQDN;

	Type m_type;
	QString m_address;
};
