/*
 * BuiltinDirectory.cpp - NetworkObjects from VeyonConfiguration
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#include "BuiltinDirectoryConfiguration.h"
#include "BuiltinDirectory.h"


BuiltinDirectory::BuiltinDirectory( BuiltinDirectoryConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration )
{
}



NetworkObjectList BuiltinDirectory::queryObjects( NetworkObject::Type type, const QString& name )
{
	const auto networkObjects = m_configuration.networkObjects();

	NetworkObjectList objects;

	// search for corresponding group whose UID matches parent UID of computer object
	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( ( type == NetworkObject::None || networkObject.type() == type ) &&
				( name.isEmpty() || networkObject.name().compare( name, Qt::CaseInsensitive ) == 0 ) )
		{
			objects.append( networkObject );
		}
	}

	return objects;
}



void BuiltinDirectory::update()
{
	m_configuration.reloadFromStore();

	const auto networkObjects = m_configuration.networkObjects();

	NetworkObjectUidList groupUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.type() == NetworkObject::Group )
		{
			groupUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates

			addOrUpdateObject( networkObject, NetworkObject::Root );

			updateRoom( networkObject, networkObjects );
		}
	}

	removeObjects( NetworkObject::Root, [groupUids]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Group && groupUids.contains( object.uid() ) == false; } );
}



void BuiltinDirectory::updateRoom( const NetworkObject& roomObject, const QJsonArray& networkObjects )
{
	NetworkObjectUidList computerUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.parentUid() == roomObject.uid() )
		{
			computerUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates
			addOrUpdateObject( networkObject, roomObject );
		}
	}

	removeObjects( roomObject, [computerUids]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Host && computerUids.contains( object.uid() ) == false; } );
}
