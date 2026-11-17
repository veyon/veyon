// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

#include <optional>

// clazy:excludeall=copyable-polymorphic

class UserGroupsBackendInterface
{
public:
	virtual QString userGroupsBackendName() const = 0;

	virtual void reloadConfiguration() = 0;

	virtual QStringList userGroups(bool queryDomainGroups) = 0;
	virtual QStringList groupsOfUser(const QString& username, bool queryDomainGroups) = 0;
	virtual QString userGroupSecurityIdentifier(const QString& groupName) = 0;

	// backends that read credentials from the SecretStore (e.g. a bind password) return true; they
	// are used through the directory broker in unprivileged components (04-directory-broker.md §2)
	virtual bool requiresSecrets() const
	{
		return false;
	}

	// like groupsOfUser(), but returns std::nullopt if the groups could not be determined (e.g.
	// directory or broker not available) so that access control can fail closed
	virtual std::optional<QStringList> tryQueryGroupsOfUser(const QString& username, bool queryDomainGroups)
	{
		return groupsOfUser(username, queryDomainGroups);
	}
};

using UserGroupsBackendInterfaceList = QList<UserGroupsBackendInterface>;

// version 2 (4.12): requiresSecrets(), tryQueryGroupsOfUser(); plugins built against the old
// interface are refused by qobject_cast instead of being called with a wrong vtable
#define UserGroupsBackendInterface_iid "io.veyon.Veyon.Plugins.UserGroupsBackendInterface/2"

Q_DECLARE_INTERFACE(UserGroupsBackendInterface, UserGroupsBackendInterface_iid)
