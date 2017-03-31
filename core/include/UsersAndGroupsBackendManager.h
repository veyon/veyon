/*
 * UsersAndGroupsManager.h - header file for UsersAndGroupsManager
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

#ifndef USERS_AND_GROUPS_BACKEND_MANAGER_H
#define USERS_AND_GROUPS_BACKEND_MANAGER_H

#include "LocalUsersAndGroupsBuiltin.h"

class ITALC_CORE_EXPORT UsersAndGroupsBackendManager : public QObject
{
	Q_OBJECT
public:
	UsersAndGroupsBackendManager();

	QMap<Plugin::Uid, QString> availableBackends();

	UsersAndGroupsPluginInterface* configuredBackend()
	{
		return m_configuredBackend;
	}

private:	
	UsersAndGroupsPluginInterface* m_configuredBackend;

	QMap<Plugin::Uid, UsersAndGroupsPluginInterface *> m_usersAndGroupsPluginInterfaces;
	LocalUsersAndGroupsBuiltin m_localUsersAndGroupsBuiltin;

};

#endif // USERS_AND_GROUPS_BACKEND_MANAGER_H
