// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlatformPluginManager.h"
#include "PluginManager.h"


PlatformPluginManager::PlatformPluginManager(PluginManager& pluginManager, QObject* parent) :
	QObject(parent),
	m_platformPlugin(nullptr)
{
	for (auto pluginObject : std::as_const(pluginManager.pluginObjects()))
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto platformPluginInterface = qobject_cast<PlatformPluginInterface*>(pluginObject);

		if (pluginInterface && platformPluginInterface)
		{
			m_platformPlugin = platformPluginInterface;
		}
	}

	if (m_platformPlugin == nullptr)
	{
		qFatal("PlatformPluginManager: no platform plugin available!");
	}
}
