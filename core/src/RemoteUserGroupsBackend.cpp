// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerLimits.h"
#include "RemoteUserGroupsBackend.h"


using namespace DirectoryBrokerProtocol;


RemoteUserGroupsBackend::RemoteUserGroupsBackend(const QString& backendName, const std::shared_ptr<DirectoryBrokerClient>& client) :
	m_backendName(backendName),
	m_client(client)
{
}



void RemoteUserGroupsBackend::reloadConfiguration()
{
	QMutexLocker locker(&m_cacheMutex);
	m_userGroupsCache.clear();
}



QStringList RemoteUserGroupsBackend::userGroups(bool queryDomainGroups)
{
	m_cacheMutex.lock();
	const auto it = m_userGroupsCache.find(queryDomainGroups);
	if (it != m_userGroupsCache.end() && it->second.expiry.hasExpired() == false)
	{
		const auto groups = it->second.groups;
		m_cacheMutex.unlock();
		return groups;
	}
	m_cacheMutex.unlock();

	const auto groups = toStringList(m_client->requestList(Command::UserGroupsList, {{Key::QueryDomainGroups, queryDomainGroups}}));
	if (groups.has_value() == false)
	{
		vWarning() << "user groups not available";
		return {};
	}

	QMutexLocker locker(&m_cacheMutex);
	m_userGroupsCache[queryDomainGroups] = CachedUserGroups{*groups, QDeadlineTimer(DirectoryBrokerLimits::UserGroupsCacheLifetime)};

	return *groups;
}



QStringList RemoteUserGroupsBackend::groupsOfUser(const QString& username, bool queryDomainGroups)
{
	return tryQueryGroupsOfUser(username, queryDomainGroups).value_or(QStringList{});
}



QString RemoteUserGroupsBackend::userGroupSecurityIdentifier(const QString& groupName)
{
	if (groupName.isEmpty() || DirectoryBrokerMessage::isValidString(groupName) == false)
	{
		vWarning() << "invalid group name";
		return {};
	}

	const auto reply = m_client->request(Command::UserGroupsGroupSecurityIdentifier, {{Key::GroupName, groupName}});
	const auto sid = reply.arguments.value(Key::Sid);
	if (reply.isError() || sid.userType() != QMetaType::QString ||
		DirectoryBrokerMessage::isValidString(sid.toString()) == false)
	{
		vWarning() << "security identifier of group not available";
		return {};
	}

	return sid.toString();
}



std::optional<QStringList> RemoteUserGroupsBackend::tryQueryGroupsOfUser(const QString& username, bool queryDomainGroups)
{
	if (username.isEmpty() || DirectoryBrokerMessage::isValidString(username) == false)
	{
		vWarning() << "invalid user name";
		return {};
	}

	// never cached: used by access control
	return toStringList(m_client->requestList(Command::UserGroupsGroupsOfUser,
											  {{Key::Username, username}, {Key::QueryDomainGroups, queryDomainGroups}}));
}



std::optional<QStringList> RemoteUserGroupsBackend::toStringList(const std::optional<QVariantList>& items)
{
	if (items.has_value() == false)
	{
		return {};
	}

	QStringList strings;
	strings.reserve(items->size());

	for (const auto& item : *items)
	{
		if (item.userType() != QMetaType::QString || item.toString().size() > DirectoryBrokerLimits::MaximumReplyStringSize)
		{
			vWarning() << "malformed group name in directory broker reply";
			return {};
		}
		strings.append(item.toString());
	}

	return strings;
}
