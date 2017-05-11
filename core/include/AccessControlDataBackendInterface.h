/*
 * AccessControlDataBackendInterface.h - interface for an AccessControlDataBackend
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef ACCESS_CONTROL_DATA_BACKEND_INTERFACE_H
#define ACCESS_CONTROL_DATA_BACKEND_INTERFACE_H

#include "PluginInterface.h"

class AccessControlDataBackendInterface
{
public:
	virtual QString accessControlDataBackendName() const = 0;

	virtual void reloadConfiguration() = 0;

	virtual QStringList users() = 0;
	virtual QStringList userGroups() = 0;
	virtual QStringList groupsOfUser( QString userName ) = 0;

	virtual QStringList allRooms() = 0;
	virtual QStringList roomsOfComputer( QString computerName ) = 0;

};

typedef QList<AccessControlDataBackendInterface> AccessControlDataBackendInterfaceList;

#define AccessControlDataBackendInterface_iid "org.veyon.Veyon.Plugins.AccessControlDataBackendInterface"

Q_DECLARE_INTERFACE(AccessControlDataBackendInterface, AccessControlDataBackendInterface_iid)

#endif // ACCESS_CONTROL_DATA_BACKEND_INTERFACE_H
