/*
 * LdapNetworkObjectDirectory.h - provides a NetworkObjectDirectory for LDAP
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

#pragma once

#include "LdapDirectory.h"
#include "NetworkObjectDirectory.h"

class LdapNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	LdapNetworkObjectDirectory( const LdapConfiguration& ldapConfiguration, QObject* parent );

	NetworkObjectList queryObjects( NetworkObject::Type type, const QString& name ) override;
	NetworkObjectList queryParents( const NetworkObject& childId ) override;

	static NetworkObject computerToObject( LdapDirectory* directory, const QString& computerDn );

private:
	void update() override;
	void updateLocation( const NetworkObject& locationObject );

	NetworkObjectList queryLocations( const QString& name );
	NetworkObjectList queryHosts( const QString& name );

	LdapDirectory m_ldapDirectory;
};
