// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformPluginInterface.h"

class VEYON_CORE_EXPORT PlatformPluginManager : public QObject
{
	Q_OBJECT
public:
	explicit PlatformPluginManager(PluginManager& pluginManager, QObject* parent = nullptr);

	PlatformPluginInterface* platformPlugin()
	{
		return m_platformPlugin;
	}

private:
	PlatformPluginInterface* m_platformPlugin;
};
