/*
 * HostAddress.h - header for HostAddress class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

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

	explicit HostAddress( const QString& address );
	~HostAddress() = default;

	Type type() const
	{
		return m_type;
	}

	bool isLocalHost() const;

	QString convert( Type targetType ) const;
	QString tryConvert( Type targetType ) const;

	QStringList lookupIpAddresses() const;

	static QString parseHost( const QString& address );
	static int parsePortNumber( const QString& address );

	static QString localFQDN();

private:
	static Type determineType( const QString& address );
	static QString toIpAddress( const QString& hostName );
	static QString toHostName( Type type, const QString& address );
	static QString toFQDN( Type type, const QString& address );
	static QString fqdnToHostName( const QString& fqdn );

	Type m_type;
	QString m_address;

} ;
