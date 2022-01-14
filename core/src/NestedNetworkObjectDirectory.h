/*
 * NestedNetworkObjectDirectory.cpp - header file for NestedNetworkObjectDirectory
 *
 * Copyright (c) 2021-2022 Tobias Junghans <tobydox@veyon.io>
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

#include "NetworkObjectDirectory.h"

class NestedNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	NestedNetworkObjectDirectory( QObject* parent );

	void addSubDirectory( NetworkObjectDirectory* subDirectory );

	NetworkObjectList queryObjects( NetworkObject::Type type,
									NetworkObject::Property property, const QVariant& value ) override;
	NetworkObjectList queryParents( const NetworkObject& childId ) override;

	void update() override;
	void fetchObjects( const NetworkObject& parent ) override;

private:
	void replaceObjectsRecursively( NetworkObjectDirectory* directory,
								   const NetworkObject& parent );

	QList<NetworkObjectDirectory *> m_subDirectories;

};
