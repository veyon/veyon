// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AccessControlDirectoryQueries.h"
#include "NetworkObjectDirectory.h"
#include "UserGroupsBackendInterface.h"

#include <algorithm>


AccessControlDirectoryQueries::AccessControlDirectoryQueries(UserGroupsBackendInterface* userGroupsBackend,
															 NetworkObjectDirectory* directory,
															 bool useDomainUserGroups, const Clock& clock) :
	m_userGroupsBackend(userGroupsBackend),
	m_directory(directory),
	m_useDomainUserGroups(useDomainUserGroups),
	m_clock(clock)
{
}



void AccessControlDirectoryQueries::beginPass()
{
	++m_passDepth;
}



void AccessControlDirectoryQueries::endPass()
{
	if (m_passDepth > 0 && --m_passDepth == 0)
	{
		m_groupsOfUser.clear();
		m_locationsOfComputer.clear();
	}
}



QStringList AccessControlDirectoryQueries::groupsOfUser(const QString& user)
{
	if (m_passDepth > 0)
	{
		const auto it = m_groupsOfUser.constFind(user);
		if (it != m_groupsOfUser.constEnd())
		{
			return result(it.value());
		}
	}

	const auto groups = queryGroupsOfUser(user);
	if (m_passDepth > 0)
	{
		m_groupsOfUser.insert(user, groups);
	}

	return result(groups);
}



QStringList AccessControlDirectoryQueries::locationsOfComputer(const QString& fullyQualifiedDomainName)
{
	if (m_passDepth > 0)
	{
		const auto it = m_locationsOfComputer.constFind(fullyQualifiedDomainName);
		if (it != m_locationsOfComputer.constEnd())
		{
			return result(it.value());
		}
	}

	const auto locations = queryLocationsOfComputer(fullyQualifiedDomainName);
	if (m_passDepth > 0)
	{
		m_locationsOfComputer.insert(fullyQualifiedDomainName, locations);
	}

	return result(locations);
}



std::optional<QStringList> AccessControlDirectoryQueries::queryGroupsOfUser(const QString& user)
{
	if (isBackingOff() || m_userGroupsBackend == nullptr)
	{
		return {};
	}

	const auto groups = m_userGroupsBackend->tryQueryGroupsOfUser(user, m_useDomainUserGroups);
	if (groups.has_value() == false)
	{
		vWarning() << "groups of user" << user << "not available";
		recordFailure();
	}

	return groups;
}



std::optional<QStringList> AccessControlDirectoryQueries::queryLocationsOfComputer(const QString& fullyQualifiedDomainName)
{
	if (isBackingOff() || m_directory == nullptr)
	{
		return {};
	}

	const auto computers = m_directory->tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress,
														fullyQualifiedDomainName);
	if (computers.has_value() == false)
	{
		vWarning() << "directory not available - could not query network objects for host" << fullyQualifiedDomainName;
		recordFailure();
		return {};
	}

	if (computers->isEmpty())
	{
		vWarning() << "Could not query any network objects for host" << fullyQualifiedDomainName;
		return QStringList{};
	}

	QStringList locations;
	locations.reserve(computers->size() * 3);

	for (const auto& computer : *computers)
	{
		const auto parents = m_directory->tryQueryParents(computer);
		if (parents.has_value() == false)
		{
			vWarning() << "directory not available - could not query locations of host" << fullyQualifiedDomainName;
			recordFailure();
			return {};
		}

		for (const auto& parent : *parents)
		{
			locations.append(parent.name());
		}
	}

	std::sort(locations.begin(), locations.end());

	return locations;
}



QStringList AccessControlDirectoryQueries::result(const std::optional<QStringList>& result)
{
	if (result.has_value())
	{
		return *result;
	}

	m_failed = true;

	return {};
}



void AccessControlDirectoryQueries::recordFailure()
{
	m_lastFailure = m_clock();
	m_hasFailedBefore = true;
}



bool AccessControlDirectoryQueries::isBackingOff() const
{
	return m_hasFailedBefore && m_clock() - m_lastFailure < FailureBackoff;
}
