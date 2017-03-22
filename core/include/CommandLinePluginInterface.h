/*
 * CommandLinePluginInterface.h - interface class for command line plugins
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef COMMAND_LINE_PLUGIN_INTERFACE_H
#define COMMAND_LINE_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

class ITALC_CORE_EXPORT CommandLinePluginInterface : public QObject, public PluginInterface
{
	Q_OBJECT
public:
	typedef enum RunResults
	{
		Unknown,
		Successful,
		Failed,
		NotEnoughArguments,
		InvalidCommand,
		NoResult,
		RunResultCount
	} RunResult;

	Q_ENUM(RunResult)

	virtual ~CommandLinePluginInterface() {}

	virtual QString commandName() const = 0;
	virtual QString commandHelp() const = 0;
	virtual QStringList subCommands() const = 0;
	virtual QString subCommandHelp( const QString& command ) const = 0;
	virtual RunResult runCommand( const QStringList& arguments ) = 0;

};

typedef QList<CommandLinePluginInterface *> CommandLinePluginInterfaceList;

#define CommandLinePluginInterface_iid "org.italc-solutions.iTALC.Plugins.CommandLinePluginInterface"

Q_DECLARE_INTERFACE(CommandLinePluginInterface, CommandLinePluginInterface_iid)

#endif // COMMAND_LINE_PLUGIN_INTERFACE_H
