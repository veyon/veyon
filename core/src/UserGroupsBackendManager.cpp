// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DirectoryBrokerClient.h"
#include "PluginManager.h"
#include "RemoteUserGroupsBackend.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"


UserGroupsBackendManager::UserGroupsBackendManager(QObject* parent) :
	QObject(parent)
{
	for (auto pluginObject : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto userGroupsBackendInterface = qobject_cast<UserGroupsBackendInterface*>(pluginObject);

		if (pluginInterface && userGroupsBackendInterface)
		{
			m_backends[pluginInterface->uid()] = userGroupsBackendInterface;

			if (pluginInterface->flags().testFlag(Plugin::ProvidesDefaultImplementation))
			{
				m_defaultBackend = userGroupsBackendInterface;
			}
		}
	}

	if (m_defaultBackend == nullptr)
	{
		vCritical() << "no default plugin available!";
	}

	reloadConfiguration();
}



UserGroupsBackendManager::~UserGroupsBackendManager() = default;



QMap<Plugin::Uid, QString> UserGroupsBackendManager::availableBackends()
{
	QMap<Plugin::Uid, QString> items;

	for (auto it = m_backends.constBegin(), end = m_backends.constEnd(); it != end; ++it)
	{
		items[it.key()] = it.value()->userGroupsBackendName();
	}

	return items;
}



UserGroupsBackendInterface* UserGroupsBackendManager::configuredBackend()
{
	if (m_configuredBackend == nullptr)
	{
		reloadConfiguration();
	}

	return m_configuredBackend;
}



UserGroupsBackendInterface* UserGroupsBackendManager::configuredLocalBackend() const
{
	const auto backend = m_backends.value(VeyonCore::config().userGroupsBackend());

	return backend ? backend : m_defaultBackend;
}



void UserGroupsBackendManager::reloadConfiguration()
{
	const auto localBackend = configuredLocalBackend();

	if (localBackend &&
		DirectoryBrokerClient::configuredPlacement(localBackend->requiresSecrets()) == DirectoryBrokerClient::Placement::Broker)
	{
		// the backend (and its credentials) is never used in this process; the remote backend is
		// kept, as callers such as AccessControlProvider store the pointer
		if (m_remoteBackend == nullptr)
		{
			m_remoteBackend = std::make_unique<RemoteUserGroupsBackend>(localBackend->userGroupsBackendName(),
																		DirectoryBrokerClient::configuredClient());
		}
		m_remoteBackend->setUserGroupsBackendName(localBackend->userGroupsBackendName());
		m_remoteBackend->reloadConfiguration();
		m_configuredBackend = m_remoteBackend.get();
	}
	else
	{
		m_configuredBackend = localBackend;
	}
}
