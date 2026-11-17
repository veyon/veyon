// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "DirectoryBrokerService.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "UserGroupsBackendInterface.h"

#include <atomic>

// Directory and user-groups backend for broker tests without LDAP (04-directory-broker.md §7):
// requires secrets like the LDAP plugin, generates locations × hosts and groups, and counts
// instantiations and queries so that tests can assert it is never used where it must not be.

// sleeps like a slow LDAP query, aborted cooperatively like LdapClient does
inline bool dummyBackendDelay(int delay)
{
	for (int elapsed = 0; elapsed < delay; elapsed += 10)
	{
		if (DirectoryBrokerService::isCurrentRequestCancelled())
		{
			return false;
		}
		QThread::msleep(10);
	}

	return true;
}



class DummyNetworkObjectDirectory : public NetworkObjectDirectory
{
public:
	DummyNetworkObjectDirectory(int locationCount, int hostsPerLocation, QObject* parent) :
		NetworkObjectDirectory(parent),
		m_locationCount(locationCount),
		m_hostsPerLocation(hostsPerLocation)
	{
	}

	static NetworkObject location(int index)
	{
		return NetworkObject(NetworkObject::Type::Location, QStringLiteral("Room %1").arg(index), {}, {},
							 QStringLiteral("ou=room%1,dc=example,dc=org").arg(index));
	}

	static NetworkObject host(int locationIndex, int hostIndex)
	{
		const auto name = QStringLiteral("pc%1-%2").arg(locationIndex).arg(hostIndex);
		return NetworkObject(NetworkObject::Type::Host, name, name + QStringLiteral(".example.org"),
							 QStringLiteral("00:11:22:33:%1:%2").arg(locationIndex % 100, 2, 10, QLatin1Char('0')).arg(hostIndex % 100, 2, 10, QLatin1Char('0')),
							 QStringLiteral("cn=%1,ou=room%2,dc=example,dc=org").arg(name).arg(locationIndex),
							 {}, location(locationIndex).uid());
	}

	void update() override
	{
		++updateCount;
		usedThreads.append(QThread::currentThread());
		const auto completed = dummyBackendDelay(updateDelay);

		// a failing directory delivers an incomplete tree
		NetworkObjectList locations;
		for (int locationIndex = 0; locationIndex < (updateFails ? m_locationCount / 2 : m_locationCount); ++locationIndex)
		{
			locations.append(location(locationIndex));
		}
		replaceObjects(locations, rootObject());
		setUpdateFailed(updateFails || completed == false);
	}

	void fetchObjects(const NetworkObject& object) override
	{
		usedThreads.append(QThread::currentThread());

		for (int locationIndex = 0; locationIndex < m_locationCount; ++locationIndex)
		{
			if (object.uid() == location(locationIndex).uid())
			{
				NetworkObjectList hosts;
				for (int hostIndex = 0; hostIndex < m_hostsPerLocation; ++hostIndex)
				{
					hosts.append(host(locationIndex, hostIndex));
				}
				replaceObjects(hosts, object);
			}
		}

		setObjectPopulated(object);
	}

	NetworkObjectList queryObjects(NetworkObject::Type type, NetworkObject::Attribute attribute, const QVariant& value) override
	{
		usedThreads.append(QThread::currentThread());
		lastQueryValue = value.toString();

		if (dummyBackendDelay(delay) == false)
		{
			++abortedQueries;
			return {};
		}

		NetworkObjectList objects;
		for (int locationIndex = 0; locationIndex < m_locationCount; ++locationIndex)
		{
			if (type == NetworkObject::Type::Location || type == NetworkObject::Type::None)
			{
				objects.append(location(locationIndex));
			}
			if (type == NetworkObject::Type::Host || type == NetworkObject::Type::None)
			{
				for (int hostIndex = 0; hostIndex < m_hostsPerLocation; ++hostIndex)
				{
					const auto hostObject = host(locationIndex, hostIndex);
					if (attribute == NetworkObject::Attribute::None ||
						hostObject.isAttributeValueEqual(attribute, value, Qt::CaseInsensitive))
					{
						objects.append(hostObject);
					}
				}
			}
		}

		return objects;
	}

	std::optional<NetworkObjectList> tryQueryObjects(NetworkObject::Type type, NetworkObject::Attribute attribute,
													 const QVariant& value) override
	{
		if (available == false)
		{
			return {};
		}
		const auto objects = queryObjects(type, attribute, value);
		// an aborted query failed (like an abandoned LDAP search)
		if (DirectoryBrokerService::isCurrentRequestCancelled())
		{
			return {};
		}
		return objects;
	}

	NetworkObjectList queryParents(const NetworkObject& child) override
	{
		usedThreads.append(QThread::currentThread());

		for (int locationIndex = 0; locationIndex < m_locationCount; ++locationIndex)
		{
			if (child.parentUid() == location(locationIndex).uid())
			{
				return {location(locationIndex)};
			}
		}

		return {};
	}

	std::optional<NetworkObjectList> tryQueryParents(const NetworkObject& child) override
	{
		if (available == false)
		{
			return {};
		}
		return queryParents(child);
	}

	std::atomic<int> updateCount{0};
	std::atomic<int> updateDelay{0};
	std::atomic<bool> updateFails{false};
	std::atomic<int> abortedQueries{0};
	std::atomic<int> delay{0};
	std::atomic<bool> available{true};
	QList<QThread*> usedThreads;
	QString lastQueryValue;

private:
	const int m_locationCount;
	const int m_hostsPerLocation;
};



class DummyDirectoryPlugin : public NetworkObjectDirectoryPluginInterface, public UserGroupsBackendInterface
{
public:
	DummyDirectoryPlugin(int locationCount = 2, int hostsPerLocation = 3, int groupCount = 3) :
		m_locationCount(locationCount),
		m_hostsPerLocation(hostsPerLocation),
		m_groupCount(groupCount)
	{
	}

	QString directoryName() const override
	{
		return QStringLiteral("Dummy directory");
	}

	NetworkObjectDirectory* createNetworkObjectDirectory(QObject* parent) override
	{
		++directoryInstantiations;
		return new DummyNetworkObjectDirectory(m_locationCount, m_hostsPerLocation, parent);
	}

	bool requiresSecrets() const override
	{
		return true;
	}

	QString userGroupsBackendName() const override
	{
		return QStringLiteral("Dummy user groups");
	}

	void reloadConfiguration() override
	{
	}

	QStringList userGroups(bool queryDomainGroups) override
	{
		Q_UNUSED(queryDomainGroups)
		++userGroupsQueries;

		QStringList groups;
		for (int index = 0; index < m_groupCount; ++index)
		{
			groups.append(QStringLiteral("group%1").arg(index));
		}
		return groups;
	}

	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override
	{
		Q_UNUSED(queryDomainGroups)
		++userGroupsQueries;
		lastUsername = username;

		dummyBackendDelay(delay);

		return userGroups(false).mid(0, 2);
	}

	std::optional<QStringList> tryQueryGroupsOfUser(const QString& username, bool queryDomainGroups) override
	{
		if (available == false)
		{
			return {};
		}
		return groupsOfUser(username, queryDomainGroups);
	}

	QString userGroupSecurityIdentifier(const QString& groupName) override
	{
		++userGroupsQueries;
		return QStringLiteral("S-1-5-21-%1").arg(groupName.size());
	}

	std::atomic<int> directoryInstantiations{0};
	std::atomic<int> userGroupsQueries{0};
	std::atomic<int> delay{0};
	std::atomic<bool> available{true};
	QString lastUsername;

private:
	const int m_locationCount;
	const int m_hostsPerLocation;
	const int m_groupCount;
};
