// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDeadlineTimer>
#include <QMutex>

#include "UserGroupsBackendInterface.h"

#include <map>
#include <memory>

class DirectoryBrokerClient;

// User-groups backend of components that must not run a backend requiring secrets
// (04-directory-broker.md §3): all queries are answered by the directory broker. Only the UI
// listing userGroups() is cached (60 s); group memberships are queried every time and fail closed
// (tryQueryGroupsOfUser() returns std::nullopt) while the broker is not available.
class VEYON_CORE_EXPORT RemoteUserGroupsBackend final : public UserGroupsBackendInterface
{
public:
	RemoteUserGroupsBackend(const QString& backendName, const std::shared_ptr<DirectoryBrokerClient>& client);

	QString userGroupsBackendName() const override
	{
		return m_backendName;
	}

	void setUserGroupsBackendName(const QString& backendName)
	{
		m_backendName = backendName;
	}

	void reloadConfiguration() override;

	QStringList userGroups(bool queryDomainGroups) override;
	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override;
	QString userGroupSecurityIdentifier(const QString& groupName) override;

	std::optional<QStringList> tryQueryGroupsOfUser(const QString& username, bool queryDomainGroups) override;

private:
	static std::optional<QStringList> toStringList(const std::optional<QVariantList>& items);

	struct CachedUserGroups
	{
		QStringList groups;
		QDeadlineTimer expiry;
	};

	QString m_backendName;
	const std::shared_ptr<DirectoryBrokerClient> m_client;

	QMutex m_cacheMutex;
	std::map<bool, CachedUserGroups> m_userGroupsCache;
};
