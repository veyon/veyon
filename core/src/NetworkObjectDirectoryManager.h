// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

	NetworkObjectDirectory* createDirectory(Plugin::Uid uid, QObject* parent);

	NetworkObjectDirectory* configuredDirectory();

private:
	QMap<PluginInterface*, NetworkObjectDirectoryPluginInterface*> m_directoryPluginInterfaces{};
	NetworkObjectDirectory* m_configuredDirectory{nullptr};
};
