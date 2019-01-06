/*
 * SystemUserGroupsPlugin.h - declaration of SystemUserGroupsPlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef SYSTEM_USER_GROUPS_PLUGIN_H
#define SYSTEM_USER_GROUPS_PLUGIN_H

#include "UserGroupsBackendInterface.h"

class SystemUserGroupsPlugin : public QObject, PluginInterface, UserGroupsBackendInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.SystemUserGroups")
	Q_INTERFACES(PluginInterface UserGroupsBackendInterface)
public:
	SystemUserGroupsPlugin( QObject* paren = nullptr );
	virtual ~SystemUserGroupsPlugin();

	Plugin::Uid uid() const override
	{
		return QStringLiteral("2917cdeb-ac13-4099-8715-20368254a367");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "SystemUserGroups" );
	}

	QString description() const override
	{
		return tr( "User groups backend for system user groups" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QString userGroupsBackendName() const override
	{
		return tr( "Default (system user groups)" );
	}

	void reloadConfiguration() override;

	QStringList userGroups( bool queryDomainGroups ) override;
	QStringList groupsOfUser( const QString& username,  bool queryDomainGroups ) override;

};

#endif // SYSTEM_USER_GROUPS_PLUGIN_H
