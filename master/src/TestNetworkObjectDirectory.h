/*
 * TestNetworkObjectDirectory.h - provides a NetworkObjectDirectory implementation for testing
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef TEST_NETWORK_OBJECT_DIRECTORY_H
#define TEST_NETWORK_OBJECT_DIRECTORY_H

#include <QHash>

#include "NetworkObjectDirectory.h"

class TestNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	TestNetworkObjectDirectory( QObject* parent );

	virtual QList<NetworkObject> objects( const NetworkObject& parent ) override;

	void update() override;

private:
	QHash<NetworkObject, QList<NetworkObject>> m_objects;
};

#endif // TEST_NETWORK_OBJECT_DIRECTORY_H
