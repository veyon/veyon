/*
 * AccessControlDataBackendManager.h - header file for AccessControlDataBackendManager
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

#ifndef ACCESS_CONTROL_DATA_BACKEND_MANAGER_H
#define ACCESS_CONTROL_DATA_BACKEND_MANAGER_H

#include "AccessControlDataBackendInterface.h"

class ITALC_CORE_EXPORT AccessControlDataBackendManager : public QObject
{
	Q_OBJECT
public:
	AccessControlDataBackendManager( PluginManager& pluginManager );

	QMap<Plugin::Uid, QString> availableBackends();

	AccessControlDataBackendInterface* configuredBackend()
	{
		return m_configuredBackend;
	}

	void reloadConfiguration();

private:	
	QMap<Plugin::Uid, AccessControlDataBackendInterface *> m_backends;
	AccessControlDataBackendInterface* m_defaultBackend;
	AccessControlDataBackendInterface* m_configuredBackend;

};

#endif // ACCESS_CONTROL_DATA_BACKEND_MANAGER_H
