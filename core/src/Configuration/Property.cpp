/*
 * ConfigurationObject.cpp - implementation of ConfigurationObject
 *
 * Copyright (c) 2009-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "Configuration/Proxy.h"


namespace Configuration
{

Configuration::Property::Property( Object* object, const QString& key, const QString& parentKey,
								   const QVariant& defaultValue, Flags flags ) :
	QObject( object ),
	m_object( object ),
	m_proxy( nullptr ),
	m_key( key ),
	m_parentKey( parentKey ),
	m_defaultValue( defaultValue ),
	m_flags( flags )
{
}



Configuration::Property::Property( Proxy* proxy, const QString& key, const QString& parentKey,
								   const QVariant& defaultValue, Flags flags ) :
	QObject( proxy->object() ),
	m_object( nullptr ),
	m_proxy( proxy ),
	m_key( key ),
	m_parentKey( parentKey ),
	m_defaultValue( defaultValue ),
	m_flags( flags )
{
}



QObject* Property::lambdaContext() const
{
	if( m_object )
	{
		return m_object;
	}

	return m_proxy;
}



QVariant Property::variantValue() const
{
	if( m_object )
	{
		return m_object->value( m_key, m_parentKey, m_defaultValue );
	}
	else if( m_proxy )
	{
		return m_proxy->value( m_key, m_parentKey, m_defaultValue );
	}

	return m_defaultValue;
}



void Property::setVariantValue( const QVariant& value ) const
{
	if( m_object )
	{
		m_object->setValue( m_key, value, m_parentKey );
	}
	else if( m_proxy )
	{
		m_proxy->setValue( m_key, value, m_parentKey );
	}
	else
	{
		qFatal("%s", Q_FUNC_INFO);
	}
}



Property* Property::find( QObject* parent, const QString& key, const QString& parentKey )
{
	const auto properties = parent->findChildren<Property *>();
	for( auto property : properties )
	{
		if( property->m_key == key && property->m_parentKey == parentKey )
		{
			return property;
		}
	}

	return nullptr;
}



template<>
Password Configuration::TypedProperty<Password>::value() const
{
	return Password::fromEncrypted( variantValue().toString() );
}



template<>
void Configuration::TypedProperty<Password>::setValue( const Password& value ) const
{
	setVariantValue( value.encrypted() );
}


}
