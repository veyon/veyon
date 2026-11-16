// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class UserGroupsBackendInterface
{
public:
	virtual QString userGroupsBackendName() const = 0;

	virtual void reloadConfiguration() = 0;

	virtual QStringList userGroups(bool queryDomainGroups) = 0;
	virtual QStringList groupsOfUser(const QString& username, bool queryDomainGroups) = 0;
	virtual QString userGroupSecurityIdentifier(const QString& groupName) = 0;
};

using UserGroupsBackendInterfaceList = QList<UserGroupsBackendInterface>;

#define UserGroupsBackendInterface_iid "io.veyon.Veyon.Plugins.UserGroupsBackendInterface"

Q_DECLARE_INTERFACE(UserGroupsBackendInterface, UserGroupsBackendInterface_iid)
