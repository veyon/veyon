/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "LdapConfiguration.h"
#include "LdapDirectory.h"
#include "LdapNetworkObjectDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory( const LdapConfiguration& ldapConfiguration,
														QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_ldapDirectory( ldapConfiguration )
{
}



NetworkObjectList LdapNetworkObjectDirectory::queryObjects( NetworkObject::Type type,
															NetworkObject::Attribute attribute, const QVariant& value )
{
	switch( type )
	{
	case NetworkObject::Type::Location: return queryLocations( attribute, value );
	case NetworkObject::Type::Host: return queryHosts( attribute, value );
	default: break;
	}

	return {};
}



NetworkObjectList LdapNetworkObjectDirectory::queryParents( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Type::Host:
		return { NetworkObject( NetworkObject::Type::Location,
								m_ldapDirectory.locationsOfComputer( object.directoryAddress() ).value( 0 ) ) };
	case NetworkObject::Type::Location:
		return { NetworkObject( NetworkObject::Type::Root ) };
	default:
		break;
	}

	return { NetworkObject( NetworkObject::Type::None ) };
}



void LdapNetworkObjectDirectory::update()
{
	const auto locations = m_ldapDirectory.computerLocations();
	const NetworkObject rootObject( NetworkObject::Type::Root );

	for( const auto& location : qAsConst( locations ) )
	{
		const NetworkObject locationObject( NetworkObject::Type::Location, location );

		addOrUpdateObject( locationObject, rootObject );

		updateLocation( locationObject );
	}

	removeObjects( NetworkObject( NetworkObject::Type::Root ), [locations]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Type::Location && locations.contains( object.name() ) == false; } );
}



void LdapNetworkObjectDirectory::updateLocation( const NetworkObject& locationObject )
{
	const auto computers = m_ldapDirectory.computerLocationEntries( locationObject.name() );

	for( const auto& computer : qAsConst( computers ) )
	{
		const auto hostObject = computerToObject( &m_ldapDirectory, computer );
		if( hostObject.type() == NetworkObject::Type::Host )
		{
			addOrUpdateObject( hostObject, locationObject );
		}
	}

	removeObjects( locationObject, [computers]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Type::Host && computers.contains( object.directoryAddress() ) == false; } );
}



NetworkObjectList LdapNetworkObjectDirectory::queryLocations( NetworkObject::Attribute attribute, const QVariant& value )
{
	QString name;

	switch( attribute )
	{
	case NetworkObject::Attribute::None:
		break;

	case NetworkObject::Attribute::Name:
		name = value.toString();
		break;

	default:
		vCritical() << "Can't query locations by attribute" << attribute;
		return {};
	}

	const auto locations = m_ldapDirectory.computerLocations( name );

	NetworkObjectList locationObjects;
	locationObjects.reserve( locations.size() );

	for( const auto& location : locations )
	{
		locationObjects.append( NetworkObject( NetworkObject::Type::Location, location ) );
	}

	return locationObjects;
}



NetworkObjectList LdapNetworkObjectDirectory::queryHosts( NetworkObject::Attribute attribute, const QVariant& value )
{
	QStringList computers;

	switch( attribute )
	{
	case NetworkObject::Attribute::None:
		computers = m_ldapDirectory.computersByHostName( {} );
		break;

	case NetworkObject::Attribute::Name:
		computers = m_ldapDirectory.computersByDisplayName( value.toString() );
		break;

	case NetworkObject::Attribute::HostAddress:
	{
		const auto hostName = m_ldapDirectory.hostToLdapFormat( value.toString() );
		if( hostName.isEmpty() )
		{
			return {};
		}
		computers = m_ldapDirectory.computersByHostName( hostName );
		break;
	}

	default:
		vCritical() << "Can't query hosts by attribute" << attribute;
		return {};
	}

	NetworkObjectList hostObjects;
	hostObjects.reserve( computers.size() );

	for( const auto& computer : computers )
	{
		const auto hostObject = computerToObject( &m_ldapDirectory, computer );
		if( hostObject.isValid() )
		{
			hostObjects.append( hostObject );
		}
	}

	return hostObjects;
}



NetworkObject LdapNetworkObjectDirectory::computerToObject( LdapDirectory* directory, const QString& computerDn )
{
	auto displayNameAttribute = directory->computerDisplayNameAttribute();
	if( displayNameAttribute.isEmpty() )
	{
		displayNameAttribute = QStringLiteral("cn");
	}

	auto hostNameAttribute = directory->computerHostNameAttribute();
	if( hostNameAttribute.isEmpty() )
	{
		hostNameAttribute = QStringLiteral("cn");
	}

	QStringList computerAttributes{ displayNameAttribute, hostNameAttribute };

	auto macAddressAttribute = directory->computerMacAddressAttribute();
	if( macAddressAttribute.isEmpty() == false )
	{
		computerAttributes.append( macAddressAttribute );
	}

	computerAttributes.removeDuplicates();

	const auto computers = directory->client().queryObjects( computerDn, computerAttributes,
															 directory->computersFilter(), LdapClient::Scope::Base );
	if( computers.isEmpty() == false )
	{
		const auto& computerDn = computers.firstKey();
		const auto& computer = computers.first();
		const auto displayName = computer[displayNameAttribute].value( 0 );
		const auto hostName = computer[hostNameAttribute].value( 0 );
		const auto macAddress = ( macAddressAttribute.isEmpty() == false ) ? computer[macAddressAttribute].value( 0 ) : QString();

		return NetworkObject( NetworkObject::Type::Host, displayName, hostName, macAddress, computerDn );
	}

	return NetworkObject( NetworkObject::Type::None );
}
