/*
 * ObjectManager.h - header file for ObjectManager
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "VeyonCore.h"

template<class T>
class ObjectManager
{
public:
	ObjectManager( const QJsonArray& objects ) :
		m_objects( objects )
	{
	}

	const QJsonArray& objects() const
	{
		return m_objects;
	}

	void add( const T& object )
	{
		m_objects.append( object.toJson() );
	}

	void update( const T& object )
	{
		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			T currentObject( it->toObject() );
			if( currentObject.uid() == object.uid() )
			{
				*it = object.toJson();
				break;
			}
		}
	}

	void remove( const T& object, bool recursive = false )
	{
		if( recursive )
		{
			removeChildren( object );
		}

		for( auto it = m_objects.begin(); it != m_objects.end(); )
		{
			T currentObject( it->toObject() );
			if( currentObject.uid() == object.uid() )
			{
				it = m_objects.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	void removeChildren( const T& object )
	{
		for( auto it = m_objects.begin(); it != m_objects.end(); )
		{
			T currentObject( it->toObject() );
			if( currentObject.parentUid() == object.uid() )
			{
				removeChildren( currentObject );
				it = m_objects.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	T findByUid( typename T::Uid uid ) const
	{
		for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
		{
			T currentObject( it->toObject() );
			if( currentObject.uid() == uid )
			{
				return currentObject;
			}
		}

		return T();
	}

	T findByName( const typename T::Name& name ) const
	{
		for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
		{
			T currentObject( it->toObject() );
			if( T( it->toObject() ).name() == name )
			{
				return currentObject;
			}
		}

		return T();
	}

private:
	QJsonArray m_objects;

};

#endif // OBJECT_MANAGER_H
