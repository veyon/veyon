/*
 * HostAddress.cpp - implementation of HostAddress class
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

#include <QHostInfo>
#include <QNetworkInterface>
#include <QUrl>

#include "HostAddress.h"


HostAddress::HostAddress( const QString& address ) :
	m_type( determineType( address ) ),
	m_address( address )
{
}



bool HostAddress::isLocalHost() const
{
	if( type() == Type::Invalid || m_address.isEmpty() )
	{
		return false;
	}

	const auto allLocalAddresses = QNetworkInterface::allAddresses();

	if( type() == Type::IpAddress )
	{
		QHostAddress hostAddress( m_address );
		return hostAddress.isLoopback() || allLocalAddresses.contains( hostAddress );
	}

	const auto addresses = QHostInfo::fromName( m_address ).addresses();
	for( const auto& address : addresses )
	{
		if( address.isLoopback() || allLocalAddresses.contains( address ) )
		{
			return true;
		}
	}

	return false;
}



QString HostAddress::convert( HostAddress::Type targetType ) const
{
	if( m_type == targetType )
	{
		return m_address;
	}

	switch( targetType )
	{
	case Type::Invalid: return {};
	case Type::IpAddress: return toIpAddress( m_address );
	case Type::HostName: return toHostName( m_type, m_address );
	case Type::FullyQualifiedDomainName: return toFQDN( m_type, m_address );
	}

	vWarning() << "invalid address type" << targetType;
	return {};
}



QString HostAddress::tryConvert( HostAddress::Type targetType ) const
{
	const auto address = convert( targetType );
	if( address.isEmpty() )
	{
		return m_address;
	}

	return address;
}



QStringList HostAddress::lookupIpAddresses() const
{
	const auto hostName = convert( Type::FullyQualifiedDomainName );
	const auto hostInfo = QHostInfo::fromName( hostName );
	if( hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty() )
	{
		vWarning() << "could not lookup IP addresses of host" << hostName << "error:" << hostInfo.errorString();
		return {};
	}

	QStringList ipAddresses;

	const auto addresses = hostInfo.addresses();
	ipAddresses.reserve( addresses.size() );

	for( const auto& address : addresses )
	{
		ipAddresses.append( address.toString() );
	}

	return ipAddresses;
}


static QUrl parseAddressToUrl( const QString& address )
{
	const auto colonCount = address.count( QLatin1Char(':') );
	if( colonCount > 1 )
	{
		const auto parts = address.split( QLatin1Char(':') );
		return QUrl( QStringLiteral("scheme://[%1]:%2").arg( parts.mid( 0, colonCount ).join( QLatin1Char(':') ),
															   parts.last() ) );
	}

	return QUrl( QStringLiteral("scheme://%1").arg( address ) );
}



QString HostAddress::parseHost( const QString& address )
{
	return parseAddressToUrl( address ).host();
}



int HostAddress::parsePortNumber( const QString& address )
{
	return parseAddressToUrl( address ).port();
}



QString HostAddress::localFQDN()
{
	const auto localHostName = QHostInfo::localHostName();
	const auto type = determineType( localHostName );

	switch( type )
	{
	case Type::HostName:
		if( QHostInfo::localDomainName().isEmpty() == false )
		{
			return localHostName + QStringLiteral( "." ) + QHostInfo::localDomainName();
		}

	case Type::FullyQualifiedDomainName:
		return localHostName;

	default:
		vWarning() << "Could not determine local host name:" << localHostName;
		break;
	}

	return HostAddress( localHostName ).tryConvert( Type::FullyQualifiedDomainName );
}



HostAddress::Type HostAddress::determineType( const QString& address )
{
	if( address.isEmpty() )
	{
		return Type::Invalid;
	}

	QHostAddress hostAddress( address );

	if( hostAddress.isNull() ||
		hostAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol )
	{
		if( address.contains( QLatin1Char( '.' ) ) )
		{
			return Type::FullyQualifiedDomainName;
		}

		return Type::HostName;
	}

	return Type::IpAddress;
}



QString HostAddress::toIpAddress( const QString& hostName )
{
	if( hostName.isEmpty() )
	{
		vWarning() << "empty hostname";
		return {};
	}

	// then try to resolve ist first
	const auto hostInfo = QHostInfo::fromName( hostName );
	if( hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty() )
	{
		vWarning() << "could not lookup IP address of host" << hostName << "error:" << hostInfo.errorString();
		return {};
	}

#if QT_VERSION < 0x050600
	const auto ipAddress = hostInfo.addresses().value( 0 ).toString();
#else
	const auto ipAddress = hostInfo.addresses().constFirst().toString();
#endif
	vDebug() << "Resolved IP address of host" << hostName << "to" << ipAddress;

	return ipAddress;
}



QString HostAddress::toHostName( HostAddress::Type type, const QString& address )
{
	if( address.isEmpty() )
	{
		vWarning() << "empty address";
		return {};
	}

	switch( type )
	{
	case Type::FullyQualifiedDomainName:
		return fqdnToHostName( address );

	case Type::IpAddress:
	{
		const auto hostInfo = QHostInfo::fromName( address );
		if( hostInfo.error() != QHostInfo::NoError )
		{
			vWarning() << "could not lookup hostname for IP address" << address << "error:" << hostInfo.errorString();
			return {};
		}

		return fqdnToHostName( hostInfo.hostName() );
	}

	case Type::HostName:
		return address;

	case Type::Invalid:
		break;
	}

	return {};
}


QString HostAddress::toFQDN( HostAddress::Type type, const QString& address )
{
	if( address.isEmpty() )
	{
		vWarning() << "empty address";
		return {};
	}

	switch( type )
	{
	case Type::HostName:
		return toFQDN( Type::IpAddress, toIpAddress( address ) );

	case Type::IpAddress:
	{
		const auto hostInfo = QHostInfo::fromName( address );
		if( hostInfo.error() != QHostInfo::NoError )
		{
			vWarning() << "could not lookup hostname for IP address" << address << "error:" << hostInfo.errorString();
			return {};
		}

		return hostInfo.hostName();
	}

	case Type::FullyQualifiedDomainName:
		return address;

	case Type::Invalid:
		break;
	}

	return {};
}



QString HostAddress::fqdnToHostName( const QString& fqdn )
{
#if QT_VERSION < 0x050600
		return fqdn.split( QLatin1Char( '.' ) ).value( 0 );
#else
		return fqdn.split( QLatin1Char( '.' ) ).constFirst();
#endif
}
