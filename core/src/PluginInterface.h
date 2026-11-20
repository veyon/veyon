// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QVersionNumber>

#include "Configuration/SecretProperty.h"
#include "Plugin.h"
#include "VeyonCore.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT PluginInterface
{
public:
	virtual Plugin::Uid uid() const = 0;
	virtual QVersionNumber version() const = 0;
	virtual QString name() const = 0;
	virtual QString description() const = 0;
	virtual QString vendor() const = 0;
	virtual QString copyright() const = 0;
	virtual Plugin::Flags flags() const
	{
		return Plugin::NoFlags;
	}
	virtual void upgrade(const QVersionNumber& oldVersion)
	{
		Q_UNUSED(oldVersion)
	}

	// the plugin's configuration properties holding a Configuration::SecretReference with their
	// fixed SecretStore keys (03-secretstore-config.md §3a); merged into the
	// Configuration::SecretPropertyRegistry after loading the plugins
	virtual Configuration::SecretPropertyList secretProperties() const
	{
		return {};
	}
};

using PluginInterfaceList = QList<PluginInterface*>;

// versioned since secretProperties() changed the interface in 4.12: plugins built for an older
// version don't implement this interface and are not loaded
#define PluginInterface_iid "io.veyon.Veyon.Plugins.PluginInterface/2"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
