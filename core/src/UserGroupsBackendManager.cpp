// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PluginManager.h"
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



void UserGroupsBackendManager::reloadConfiguration()
{
	m_configuredBackend = m_backends.value(VeyonCore::config().userGroupsBackend());

	if (m_configuredBackend == nullptr)
	{
		m_configuredBackend = m_defaultBackend;
	}
}
