/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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



NetworkObjectList LdapNetworkObjectDirectory::queryObjects( NetworkObject::Type type, const QString& name )
{
	switch( type )
	{
	case NetworkObject::Location: return queryLocations( name );
	case NetworkObject::Host: return queryHosts( name );
	default: break;
	}

	return {};
}



NetworkObjectList LdapNetworkObjectDirectory::queryParents( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Host:
		return { NetworkObject( NetworkObject::Location,
								m_ldapDirectory.locationsOfComputer( object.directoryAddress() ).value( 0 ) ) };
	case NetworkObject::Location:
		return { NetworkObject::Root };
	default:
		break;
	}

	return { NetworkObject::None };
}



void LdapNetworkObjectDirectory::update()
{
	const auto locations = m_ldapDirectory.computerLocations();
	const NetworkObject rootObject( NetworkObject::Root );

	for( const auto& location : qAsConst( locations ) )
	{
		const NetworkObject locationObject( NetworkObject::Location, location );

		addOrUpdateObject( locationObject, rootObject );

		updateLocation( locationObject );
	}

	removeObjects( NetworkObject::Root, [locations]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Location && locations.contains( object.name() ) == false; } );
}



void LdapNetworkObjectDirectory::updateLocation( const NetworkObject& locationObject )
{
	const auto computers = m_ldapDirectory.computerLocationEntries( locationObject.name() );

	for( const auto& computer : qAsConst( computers ) )
	{
		const auto hostObject = computerToObject( &m_ldapDirectory, computer );
		if( hostObject.type() == NetworkObject::Host )
		{
			addOrUpdateObject( hostObject, locationObject );
		}
	}

	removeObjects( locationObject, [computers]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Host && computers.contains( object.directoryAddress() ) == false; } );
}



NetworkObjectList LdapNetworkObjectDirectory::queryLocations( const QString& name )
{
	const auto locations = m_ldapDirectory.computerLocations( name );

	NetworkObjectList locationObjects;
	locationObjects.reserve( locations.size() );

	for( const auto& location : locations )
	{
		locationObjects.append( NetworkObject( NetworkObject::Location, location ) );
	}

	return locationObjects;
}



NetworkObjectList LdapNetworkObjectDirectory::queryHosts( const QString& name )
{
	const auto computers = m_ldapDirectory.computersByHostName( name );

	NetworkObjectList hostObjects;
	hostObjects.reserve( computers.size() );

	for( const auto& computer : computers )
	{
		hostObjects.append( computerToObject( &m_ldapDirectory, computer ) );
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

		return NetworkObject( NetworkObject::Host, displayName, hostName, macAddress, computerDn );
	}

	return NetworkObject::None;
}
