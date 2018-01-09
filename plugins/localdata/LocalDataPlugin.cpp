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
#include "PlatformUserFunctions.h"

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



QStringList LocalDataPlugin::userGroups( bool queryDomainGroups )
{
	return VeyonCore::platform().userFunctions().userGroups( queryDomainGroups );
}



QStringList LocalDataPlugin::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	return VeyonCore::platform().userFunctions().groupsOfUser( username, queryDomainGroups );
}



NetworkObjectDirectory *LocalDataPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new LocalDataNetworkObjectDirectory( m_configuration, parent );
}



ConfigurationPage *LocalDataPlugin::createConfigurationPage()
{
	return new LocalDataConfigurationPage( m_configuration );
}
