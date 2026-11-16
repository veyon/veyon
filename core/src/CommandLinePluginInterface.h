// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT CommandLinePluginInterface
{
	Q_GADGET
public:
	enum RunResult {
		Unknown,
		Successful,
		Failed,
		InvalidArguments,
		NotEnoughArguments,
		EmptyCommand,
		InvalidCommand,
		NotLicensed,
		NoResult,
		RunResultCount
	};

	Q_ENUM(RunResult)

	virtual QString commandLineModuleName() const = 0;
	virtual QString commandLineModuleHelp() const = 0;
	virtual QStringList commands() const = 0;
	virtual QString commandHelp(const QString& command) const = 0;
};

using CommandLinePluginInterfaceList = QList<CommandLinePluginInterface*>;

#define CommandLinePluginInterface_iid "io.veyon.Veyon.Plugins.CommandLinePluginInterface"

Q_DECLARE_INTERFACE(CommandLinePluginInterface, CommandLinePluginInterface_iid)
