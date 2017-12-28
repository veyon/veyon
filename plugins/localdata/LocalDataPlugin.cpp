/*
 * ConfigCommandLinePlugin.cpp - implementation of ConfigCommandLinePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "LocalDataConfigurationPage.h"
#include "LocalDataNetworkObjectDirectory.h"
#include "LocalDataPlugin.h"
#include "PlatformPluginInterface.h"
#include "PlatformUserInfoFunctions.h"

LocalDataPlugin::LocalDataPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration()
{
}



LocalDataPlugin::~LocalDataPlugin()
{
}



void LocalDataPlugin::reloadConfiguration()
{
}



QStringList LocalDataPlugin::users()
{
	// TODO
	return QStringList();
}



QStringList LocalDataPlugin::userGroups( bool queryDomainGroups )
{
	return VeyonCore::platform().userInfoFunctions().userGroups( queryDomainGroups );
}



QStringList LocalDataPlugin::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	return VeyonCore::platform().userInfoFunctions().groupsOfUser( username, queryDomainGroups );
}



QStringList LocalDataPlugin::allRooms()
{
	QStringList rooms;

	const auto networkObjects = m_configuration.networkObjects();
	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );
		if( networkObject.type() == NetworkObject::Group )
		{
			rooms.append( networkObject.name() ); // clazy:exclude=reserve-candidates
		}
	}

	return rooms;
}



QStringList LocalDataPlugin::roomsOfComputer( const QString& computerName )
{
	auto networkObjects = m_configuration.networkObjects();

	NetworkObject computerObject;

	// search for computer object
	for( auto networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );
		if( networkObject.type() == NetworkObject::Host &&
				networkObject.hostAddress().toLower() == computerName.toLower() )
		{
			computerObject = networkObject;
			break;
		}
	}

	// return empty list if computer not found
	if( computerObject.type() != NetworkObject::Host )
	{
		return QStringList();
	}

	// search for corresponding group whose UID matches parent UID of computer object
	for( auto networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );
		if( networkObject.type() == NetworkObject::Group &&
				networkObject.uid() == networkObject.parentUid() )
		{
			return QStringList( { networkObject.name() } );
		}
	}

	return QStringList();
}



NetworkObjectDirectory *LocalDataPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new LocalDataNetworkObjectDirectory( m_configuration, parent );
}



ConfigurationPage *LocalDataPlugin::createConfigurationPage()
{
	return new LocalDataConfigurationPage( m_configuration );
}
