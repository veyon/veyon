/*
 * NestedNetworkObjectDirectory.cpp - implementation of NestedNetworkObjectDirectory
 *
 * Copyright (c) 2021-2024 Tobias Junghans <tobydox@veyon.io>
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

#include "NestedNetworkObjectDirectory.h"


NestedNetworkObjectDirectory::NestedNetworkObjectDirectory( QObject* parent ) :
    NetworkObjectDirectory( tr("All directories"), parent )
{
}



void NestedNetworkObjectDirectory::addSubDirectory( NetworkObjectDirectory* subDirectory )
{
	m_subDirectories.append( subDirectory );
}



NetworkObjectList NestedNetworkObjectDirectory::queryObjects( NetworkObject::Type type,
												  NetworkObject::Property property, const QVariant& value )
{
	NetworkObjectList objects;

	for( auto* subDirectory : std::as_const(m_subDirectories) )
	{
		objects += subDirectory->queryObjects( type, property, value );
	}

	return objects;
}



NetworkObjectList NestedNetworkObjectDirectory::queryParents( const NetworkObject& object )
{
	if( object.directory() && object.directory() != this )
	{
		return object.directory()->queryParents( object );
	}

	return {};
}



void NestedNetworkObjectDirectory::update()
{
	QStringList subDirectoryNames;
	subDirectoryNames.reserve( m_subDirectories.count() );

	for( auto* subDirectory : std::as_const(m_subDirectories) )
	{
		subDirectoryNames.append( subDirectory->name() );
		NetworkObject subDirectoryObject{this, NetworkObject::Type::SubDirectory, subDirectory->name(), {},
											{}, rootObject().uid() };
		addOrUpdateObject( subDirectoryObject, rootObject() );

		subDirectory->update();

		replaceObjectsRecursively( subDirectory, subDirectoryObject );
	}

	removeObjects( rootObject(), [subDirectoryNames]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Type::SubDirectory &&
			   subDirectoryNames.contains( object.name() ) == false;
	} );

	setObjectPopulated( rootObject() );
}



void NestedNetworkObjectDirectory::fetchObjects( const NetworkObject& parent )
{
	if( parent.directory() && parent.directory() != this )
	{
		parent.directory()->fetchObjects( parent );
		replaceObjects( parent.directory()->objects(parent), parent );
	}

	setObjectPopulated( parent );
}



void NestedNetworkObjectDirectory::replaceObjectsRecursively( NetworkObjectDirectory* directory,
															   const NetworkObject& parent )
{
	const auto objects = directory->objects( parent.type() == NetworkObject::Type::SubDirectory
												 ? directory->rootObject() : parent );
	for( const auto& object : objects )
	{
		if( object.isPopulated() && object.isContainer() )
		{
			replaceObjectsRecursively( directory, object );
		}
	}
	replaceObjects( objects, parent );
}
