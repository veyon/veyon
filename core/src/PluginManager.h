// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "Plugin.h"
#include "PluginInterface.h"

class QPluginLoader;

class VEYON_CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT
public:
	explicit PluginManager(QObject* parent = nullptr);
	~PluginManager();

	void loadPlatformPlugins();
	void loadPlugins();
	void upgradePlugins();

	const PluginInterfaceList& pluginInterfaces() const
	{
		return m_pluginInterfaces;
	}

	const QObjectList& pluginObjects() const
	{
		return m_pluginObjects;
	}

	void registerExtraPluginInterface(QObject* pluginObject);

	PluginUidList pluginUids() const;

	PluginInterface* pluginInterface(Plugin::Uid pluginUid);

	QString pluginName(Plugin::Uid pluginUid) const;

	// loads the plugin with all symbols resolved at load time and instantiates it only if it has been
	// built for exactly this Veyon version (see buildVersion()); logs an error (once per file and
	// process) if a plugin library cannot be loaded or was built for a different Veyon version
	static QObject* loadPluginInstance(QPluginLoader& pluginLoader);

	// the Veyon version (major.minor.patch) plugins have to be built for
	static QString buildVersion();

	// plugin files which failed to load in this process
	static QStringList incompatiblePluginFiles();

private:
	static QString pluginBuildVersion(const QString& filePath);
	static void reportIncompatiblePlugin(const QString& filePath, const QString& reason);

	void initPluginSearchPath();
	void loadPlugins(const QString& nameFilter);

	QStringList m_pluginSearchPaths;
	PluginInterfaceList m_pluginInterfaces;
	QObjectList m_pluginObjects;
	QList<QPluginLoader*> m_pluginLoaders;
	bool m_noDebugMessages;
};
