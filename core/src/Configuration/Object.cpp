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
#include "Configuration/LocalStore.h"
#include "Configuration/JsonStore.h"


namespace Configuration
{


Object::Object() :
	m_store( nullptr ),
	m_customStore( false ),
	m_data()
{
}



Object::Object( Store::Backend backend, Store::Scope scope, const QString& storeName ) :
	m_store( createStore( backend, scope ) ),
	m_customStore( false ),
	m_data()
{
	m_store->setName( storeName );

	reloadFromStore();
}



Object::Object( Store* store ) :
	m_store( store ),
	m_customStore( true ),
	m_data()
{
	reloadFromStore();
}



Object::Object( const Object& obj ) :
	m_store( nullptr ),
	m_customStore( false )
{
	*this = obj;
}



Object::~Object()
{
	if( m_customStore == false )
	{
		delete m_store;
	}
}



Object& Object::operator=( const Object& ref )
{
	if( &ref == this )
	{
		return *this;
	}

	if( m_customStore == false &&
			ref.m_customStore == false &&
			ref.m_store )
	{
		const auto backend = ref.m_store->backend();
		const auto scope = ref.m_store->scope();

		delete m_store;

		m_store = createStore( backend, scope );
	}

	m_data = ref.data();

	return *this;
}



// allow easy merging of two data maps - source is dominant over destination
static Object::DataMap operator+( Object::DataMap dst, Object::DataMap src )
{
	for( auto it = src.begin(), end = src.end(); it != end; ++it )
	{
		if( it.value().type() == QVariant::Map && dst.contains( it.key() ) )
		{
			dst[it.key()] = dst[it.key()].toMap() + it.value().toMap();
		}
		else
		{
			dst[it.key()] = it.value();
		}
	}
	return dst;
}



Object& Object::operator+=( const Object& ref )
{
	m_data = m_data + ref.data();

	return *this;
}



bool Object::hasValue( const QString& key, const QString& parentKey ) const
{
	// empty parentKey?
	if( parentKey.isEmpty() )
	{
		// search for key in toplevel data map
		return m_data.contains( key );
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = parentKey.split( QLatin1Char('/') );
	DataMap currentMap = m_data;

	for( const auto& level : subLevels )
	{
		if( currentMap.contains( level ) &&
				currentMap[level].type() == QVariant::Map )
		{
			currentMap = currentMap[level].toMap();
		}
		else
		{
			return false;
		}
	}

	// ok, we're there - does the current submap then contain our key?
	return currentMap.contains( key );
}




QVariant Object::value( const QString& key, const QString& parentKey, const QVariant& defaultValue ) const
{
	// empty parentKey?
	if( parentKey.isEmpty() )
	{
		// search for key in toplevel data map
		if( m_data.contains( key ) )
		{
			return m_data[key];
		}
		return defaultValue;
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = parentKey.split( QLatin1Char('/') );
	DataMap currentMap = m_data;
	for( const auto& level : subLevels )
	{
		if( currentMap.contains( level ) &&
				currentMap[level].type() == QVariant::Map )
		{
			currentMap = currentMap[level].toMap();
		}
		else
		{
			return defaultValue;
		}
	}

	// ok, we're there - does the current submap then contain our key?
	if( currentMap.contains( key ) )
	{
		return currentMap[key];
	}
	return defaultValue;
}




static Object::DataMap setValueRecursive( Object::DataMap data,
										  QStringList subLevels,
										  const QString& key,
										  const QVariant& value )
{
	if( subLevels.isEmpty() )
	{
		// search for key in toplevel data map
		if( !data.contains( key ) || data[key].type() != QVariant::Map )
		{
			data[key] = value;
		}
		else
		{
			vWarning() << "cannot replace sub data map with a value!";
		}

		return data;
	}

	const QString level = subLevels.takeFirst();
	if( data.contains( level ) )
	{
		if( data[level].type() != QVariant::Map )
		{
			vWarning() << "parent key points doesn't point to a data map!";
			return data;
		}
	}
	else
	{
		data[level] = Object::DataMap();
	}

	data[level] = setValueRecursive( data[level].toMap(), subLevels, key, value );

	return data;
}




void Object::setValue( const QString& key, const QVariant& value, const QString& parentKey )
{
	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	QStringList subLevels = parentKey.split( QLatin1Char('/') );
	DataMap data = setValueRecursive( m_data, subLevels, key, value );

	if( data != m_data )
	{
		m_data = data;
		Q_EMIT configurationChanged();
	}
}




static Object::DataMap removeValueRecursive( Object::DataMap data,
											 QStringList subLevels,
											 const QString& key )
{
	if( subLevels.isEmpty() )
	{
		// search for key in toplevel data map
		if( data.contains( key ) )
		{
			data.remove( key );
		}

		return data;
	}

	const QString level = subLevels.takeFirst();
	if( data.contains( level ) && data[level].type() == QVariant::Map )
	{
		data[level] = removeValueRecursive( data[level].toMap(), subLevels, key );
	}

	return data;
}





void Object::removeValue( const QString& key, const QString& parentKey )
{
	QStringList subLevels = parentKey.split( QLatin1Char('/') );
	DataMap data = removeValueRecursive( m_data, subLevels, key );
	if( data != m_data )
	{
		m_data = data;
		Q_EMIT configurationChanged();
	}
}




static void addSubObjectRecursive( const Object::DataMap& dataMap,
								   Object* _this,
								   const QString& parentKey )
{
	for( auto it = dataMap.begin(), end = dataMap.end(); it != end; ++it )
	{
		if( it.value().type() == QVariant::Map )
		{
			auto newParentKey = it.key();
			if( parentKey.isEmpty() == false )
			{
				newParentKey = parentKey + QLatin1Char('/') + newParentKey;
			}
			addSubObjectRecursive( it.value().toMap(), _this, newParentKey );
		}
		else
		{
			_this->setValue( it.key(), it.value(), parentKey );
		}
	}
}



void Object::addSubObject( Object* obj, const QString& parentKey )
{
	addSubObjectRecursive( obj->data(), this, parentKey );
}



Store* Object::createStore( Store::Backend backend, Store::Scope scope )
{
	switch( backend )
	{
	case Store::LocalBackend: return new LocalStore( scope );
	case Store::JsonFile: return new JsonStore( scope );
	case Store::NoBackend:
		break;
	default:
		vCritical() << "invalid store" << backend << "selected";
		break;
	}

	return nullptr;
}


}
