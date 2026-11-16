// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QObject>
#include <QUuid>

class Plugin
{
	Q_GADGET
public:
	using Uid = QUuid;

	enum PluginFlags {
		NoFlags,
		ProvidesDefaultImplementation = 0x0001,
		RequiresLicensing = 0x002,
	};

	Q_DECLARE_FLAGS(Flags, PluginFlags)
	Q_FLAG(Flags)
};

using PluginUidList = QVector<Plugin::Uid>;
