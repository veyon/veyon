// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DirectoryBrokerClient.h"
#include "Plugin.h"
#include "VeyonCore.h"

class NetworkObjectDirectory;
class NetworkObjectDirectoryPluginInterface;
class PluginInterface;

class VEYON_CORE_EXPORT NetworkObjectDirectoryManager : public QObject
{
	Q_OBJECT
public:
	explicit NetworkObjectDirectoryManager(QObject* parent = nullptr);

	QMap<Plugin::Uid, QString> availableDirectories();

	// always in this process (e.g. for the directory broker service)
	NetworkObjectDirectory* createDirectory(Plugin::Uid uid, QObject* parent);

	// in this process or broker-backed, depending on DirectoryBroker/Mode and the component
	// (04-directory-broker.md §2)
	NetworkObjectDirectory* configuredDirectory();

	using BrokerClientProvider = std::function<std::shared_ptr<DirectoryBrokerClient>()>;
	static NetworkObjectDirectory* createDirectory(NetworkObjectDirectoryPluginInterface* plugin,
												   DirectoryBrokerClient::Placement placement,
												   const BrokerClientProvider& brokerClientProvider,
												   QObject* parent);

private:
	NetworkObjectDirectoryPluginInterface* directoryPlugin(Plugin::Uid uid) const;

	QMap<PluginInterface*, NetworkObjectDirectoryPluginInterface*> m_directoryPluginInterfaces{};
	NetworkObjectDirectory* m_configuredDirectory{nullptr};
};
