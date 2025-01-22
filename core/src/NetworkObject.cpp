/*
 * NetworkObject.cpp - data class representing a network object
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonDocument>
#include <QJsonObject>

#include "EnumHelper.h"
#include "NetworkObject.h"
#include "HostAddress.h"


const QUuid NetworkObject::networkObjectNamespace( QStringLiteral( "8a6c479e-243e-4ccb-8e5a-0ddf5cf3c7d0" ) );


NetworkObject::NetworkObject( const NetworkObject& other ) :
	m_directory( other.directory() ),
	m_properties( other.properties() ),
	m_type( other.type() ),
	m_name( other.name() ),
	m_uid( other.uid() ),
	m_parentUid( other.parentUid() ),
	m_populated( other.isPopulated() )
{
}



NetworkObject::NetworkObject( NetworkObjectDirectory* directory,
							  NetworkObject::Type type,
							  const Name& name,
							  const QVariantMap& properties,
							  Uid uid,
							  Uid parentUid ) :
	m_directory( directory ),
	m_properties( properties ),
	m_type( type ),
	m_name( name ),
	m_uid( uid ),
	m_parentUid( parentUid ),
	m_populated( false )
{
	if( m_uid.isNull() )
	{
		m_uid = calculateUid();
		m_calculatedUid = true;
	}
}



NetworkObject::NetworkObject( const QJsonObject& jsonObject, NetworkObjectDirectory* directory ) :
	m_directory( directory ),
	m_properties( jsonObject.toVariantMap() ),
	m_type( Type( jsonObject.value( propertyKey( Property::Type ) ).toInt() ) ),
	m_name( jsonObject.value( propertyKey( Property::Name ) ).toString() ),
	m_uid( jsonObject.value( propertyKey( Property::Uid ) ).toString() ),
	m_parentUid( jsonObject.value( propertyKey( Property::ParentUid ) ).toString() ),
	m_populated( false )
{
}



NetworkObject& NetworkObject::operator=( const NetworkObject& other )
{
	if( this == &other )
	{
		return *this;
	}

	m_directory = other.directory();
	m_type = other.type();
	m_name = other.name();
	m_uid = other.uid();
	m_parentUid = other.parentUid();
	m_properties = other.properties();

	return *this;
}



bool NetworkObject::operator==( const NetworkObject& other ) const
{
	return uid() == other.uid();
}



bool NetworkObject::exactMatch( const NetworkObject& other ) const
{
	return directory() == other.directory() &&
			uid() == other.uid() &&
			type() == other.type() &&
			name() == other.name() &&
			properties() == other.properties() &&
			parentUid() == other.parentUid();
}



void NetworkObject::setParentUid( Uid parentUid )
{
	m_parentUid = parentUid;

	if( m_calculatedUid )
	{
		m_uid = calculateUid();
	}
}



NetworkObject::ModelId NetworkObject::modelId() const
{
	if( type() == Type::Root )
	{
		return 0;
	}

	return ModelId(
			( quint64( uid().data1 ) << 0U ) +
			( quint64( uid().data2 ) << 32U ) +
			( quint64( uid().data3 ) << 48U ) +
			( quint64( uid().data4[0] ) << 0U ) +
			( quint64( uid().data4[1] ) << 8U ) +
			( quint64( uid().data4[2] ) << 16U ) +
			( quint64( uid().data4[3] ) << 24U ) +
			( quint64( uid().data4[4] ) << 32U ) +
			( quint64( uid().data4[5] ) << 40U ) +
			( quint64( uid().data4[6] ) << 48U ) +
			( quint64( uid().data4[7] ) << 56U )
			);
}



QVariant NetworkObject::property( Property property ) const
{
	switch( property )
	{
	case Property::Type: return QVariant::fromValue(m_type);
	case Property::Name: return m_name;
	case Property::Uid: return m_uid;
	case Property::ParentUid: return m_parentUid;
	default: break;
	}

	return m_properties.value( propertyKey(property) );
}



QString NetworkObject::propertyKey( Property property )
{
	return EnumHelper::toString(property);
}



QJsonObject NetworkObject::toJson() const
{
	auto json = QJsonObject::fromVariantMap( properties() );
	json[propertyKey(Property::Type)] = static_cast<int>( type() );
	json[propertyKey(Property::Uid)] = uid().toString();
	json[propertyKey(Property::Name)] = name();

	if( parentUid().isNull() == false )
	{
		json[propertyKey(Property::ParentUid)] = parentUid().toString();
	}

	return json;
}



bool NetworkObject::isPropertyValueEqual( NetworkObject::Property property,
										   const QVariant& value, Qt::CaseSensitivity cs ) const
{
	const auto myValue = NetworkObject::property( property );
	const auto myValueType = myValue.userType();

	if( myValueType == value.userType() &&
		myValueType == QMetaType::QString )
	{
		// make sure to compare host addresses in the same format
		if( property == NetworkObject::Property::HostAddress )
		{
			const HostAddress myHostAddress( myValue.toString() );
			const auto otherHost = HostAddress( value.toString() ).convert( myHostAddress.type() );

			return myValue.toString().compare( otherHost, cs ) == 0;
		}

		return myValue.toString().compare( value.toString(), cs ) == 0;
	}

	return myValue == value;
}



NetworkObject::Uid NetworkObject::calculateUid() const
{
	// if a directory address is set (e.g. full DN in LDAP) it should be unique and can be
	// used for hashing
	const auto directoryAddress = property( Property::DirectoryAddress ).toString();
	if( directoryAddress.isEmpty() == false )
	{
		return QUuid::createUuidV5( networkObjectNamespace, directoryAddress );
	}

	if( type() == Type::Root )
	{
		return QUuid::createUuidV5( networkObjectNamespace, QByteArrayLiteral("Root") );
	}

	auto jsonProperties = QJsonObject::fromVariantMap(properties());
	jsonProperties.remove( propertyKey( Property::Uid ) );

	return QUuid::createUuidV5( networkObjectNamespace,
								name() + parentUid().toString() +
									QString::fromUtf8( QJsonDocument( jsonProperties ).toJson() ) );
}
