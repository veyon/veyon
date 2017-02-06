/*
 * NetworkObjectDirectory.h - base class for network object directory implementations
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

#ifndef NETWORK_OBJECT_DIRECTORY_H
#define NETWORK_OBJECT_DIRECTORY_H

#include <QObject>

#include "NetworkObject.h"

class NetworkObjectDirectory : public QObject
{
	Q_OBJECT
public:
	typedef enum Backends
	{
		ConfigurationBackend,
		LdapBackend,
		TestBackend,
		BackendCount
	} Backend;

	enum {
		MinimumUpdateInterval = 10,
		DefaultUpdateInterval = 60,
		MaximumUpdateInterval = 3600
	};

	NetworkObjectDirectory( QObject* parent );

	virtual QList<NetworkObject> objects( const NetworkObject& parent ) = 0;

public slots:
	virtual void update() = 0;

signals:
	void objectsAboutToBeInserted( const NetworkObject& parent, int index, int count );
	void objectsInserted();
	void objectsAboutToBeRemoved( const NetworkObject& parent, int index, int count );
	void objectsRemoved();

};

#endif // NETWORK_OBJECT_DIRECTORY_H
