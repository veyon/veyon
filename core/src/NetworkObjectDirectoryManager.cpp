// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginManager.h"
#include "RemoteNetworkObjectDirectory.h"
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
		const auto plugin = directoryPlugin(VeyonCore::config().networkObjectDirectoryPlugin());
		if (plugin == nullptr)
		{
			vCritical() << "no directory plugin available";
			return nullptr;
		}

		m_configuredDirectory = createDirectory(plugin, DirectoryBrokerClient::configuredPlacement(plugin->requiresSecrets()),
												&DirectoryBrokerClient::configuredClient, this);
	}

	return m_configuredDirectory;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory(NetworkObjectDirectoryPluginInterface* plugin,
																	   DirectoryBrokerClient::Placement placement,
																	   const BrokerClientProvider& brokerClientProvider,
																	   QObject* parent)
{
	if (placement == DirectoryBrokerClient::Placement::Broker)
	{
		// the plugin's directory (and its credentials) is never instantiated in this process
		return new RemoteNetworkObjectDirectory(brokerClientProvider(), parent);
	}

	return plugin ? plugin->createNetworkObjectDirectory(parent) : nullptr;
}



NetworkObjectDirectoryPluginInterface* NetworkObjectDirectoryManager::directoryPlugin(Plugin::Uid uid) const
{
	for (auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it)
	{
		if (it.key()->uid() == uid)
		{
			return it.value();
		}
	}

	for (auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it)
	{
		if (it.key()->flags().testFlag(Plugin::ProvidesDefaultImplementation))
		{
			return it.value();
		}
	}

	return nullptr;
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
