/*
 * ConfigurationObject.cpp - implementation of ConfigurationObject
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <QtCore/QStringList>

#include "Configuration/Object.h"
#include "Configuration/LocalStore.h"
#include "Configuration/XmlStore.h"


namespace Configuration
{


Object::Object( Store::Backend _backend, Store::Scope _scope ) :
	m_store( NULL )
{
	switch( _backend )
	{
		case Store::LocalBackend:
			m_store = new LocalStore( _scope );
			break;
		case Store::XmlFile:
			m_store = new XmlStore( _scope );
			break;
		default:
			qCritical( "Invalid Store::Backend %d selected in "
					"Object::Object()", _backend );
			break;
	}

	m_store->load( this );
}




Object::~Object()
{
	delete m_store;
}




QString Object::value( const QString & _key, const QString & _parentKey ) const
{
	// empty parentKey?
	if( _parentKey.isEmpty() )
	{
		// search for key in toplevel data map
		if( m_data.contains( _key ) )
		{
			return m_data[_key].toString();
		}
		return QString();
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = _parentKey.split( '/' );
	DataMap currentMap = m_data;
	foreach( const QString & _level, subLevels )
	{
		if( currentMap.contains( _level ) &&
			currentMap[_level].type() == QVariant::Map )
		{
			currentMap = currentMap[_level].toMap();
		}
		else
		{
			return QString();
		}
	}

	// ok, we're there - does the current submap then contain our key?
	if( currentMap.contains( _key ) )
	{
		return currentMap[_key].toString();
	}
	return QString();
}




void Object::setValue( const QString & _key,
			const QString & _value,
			const QString & _parentKey )
{
	if( _parentKey.isEmpty() )
	{
		// search for key in toplevel data map
		if( m_data.contains( _key ) && m_data[_key].type() !=
							QVariant::String )
		{
			qWarning( "cannot replace sub data map with a "
					"string value!" );
			return;
		}
		m_data[_key] = _value;
		return;
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = _parentKey.split( '/' );
	DataMap currentMap = m_data;
	foreach( const QString & _level, subLevels )
	{
		if( currentMap.contains( _level ) )
		{
			if( currentMap[_level].type() == QVariant::Map )
			{
				currentMap = currentMap[_level].toMap();
			}
			else
			{
				qWarning( "parent-key points to a string value "
						"rather than a sub data map!" );
				return;
			}
		}
		else
		{
			currentMap[_level] = DataMap();
		}
	}

	currentMap[_key] = _value;
}


}

