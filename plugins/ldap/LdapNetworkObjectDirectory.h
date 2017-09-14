/*
 * LdapNetworkObjectDirectory.h - provides a NetworkObjectDirectory for LDAP
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

#ifndef LDAP_NETWORK_OBJECT_DIRECTORY_H
#define LDAP_NETWORK_OBJECT_DIRECTORY_H

#include <QHash>

#include "NetworkObjectDirectory.h"

class LdapConfiguration;
class LdapDirectory;

class LdapNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	LdapNetworkObjectDirectory( const LdapConfiguration& configuration, QObject* parent );

	QList<NetworkObject> objects( const NetworkObject& parent ) override;

private slots:
	void update() override;
	void updateComputerRoom( LdapDirectory& ldapDirectory, const QString& computerRoom );

private:
	const LdapConfiguration& m_configuration;
	QHash<NetworkObject, QList<NetworkObject>> m_objects;
};

#endif // LDAP_NETWORK_OBJECT_DIRECTORY_H
