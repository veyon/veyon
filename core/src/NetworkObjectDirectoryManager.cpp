// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"


NetworkObjectDirectoryManager::NetworkObjectDirectoryManager(QObject* parent) :
	QObject(parent),
	m_directoryPluginInterfaces(),
	m_configuredDirectory(nullptr)
{
	for (auto pluginObject : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto directoryPluginInterface = qobject_cast<NetworkObjectDirectoryPluginInterface*>(pluginObject);

		if (pluginInterface && directoryPluginInterface)
		{
			m_directoryPluginInterfaces[pluginInterface] = directoryPluginInterface;
		}
	}
}



QMap<Plugin::Uid, QString> NetworkObjectDirectoryManager::availableDirectories()
{
	QMap<Plugin::Uid, QString> items;

	for (auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it)
	{
		items[it.key()->uid()] = it.value()->directoryName();
	}

	return items;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::configuredDirectory()
{
	if (m_configuredDirectory == nullptr)
	{
		m_configuredDirectory = createDirectory(VeyonCore::config().networkObjectDirectoryPlugin(), this);
	}

	return m_configuredDirectory;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory(Plugin::Uid uid, QObject* parent)
{
	for (auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it)
	{
		if (it.key()->uid() == uid)
		{
			auto directory = it.value()->createNetworkObjectDirectory(parent);
			if (directory)
			{
				return directory;
			}
		}
	}

	for (auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it)
	{
		if (it.key()->flags().testFlag(Plugin::ProvidesDefaultImplementation))
		{
			auto defaultDirectory = it.value()->createNetworkObjectDirectory(parent);
			if (defaultDirectory)
			{
				return defaultDirectory;
			}
		}
	}

	vCritical() << "no default plugin available! requested plugin:" << uid;
	return nullptr;
}
