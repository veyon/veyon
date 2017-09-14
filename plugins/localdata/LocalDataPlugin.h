/*
 * LocalDataPlugin.h - declaration of LocalDataPlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef LOCAL_DATA_PLUGIN_H
#define LOCAL_DATA_PLUGIN_H

#include "AccessControlDataBackendInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "LocalDataConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class LocalDataPlugin : public QObject,
		PluginInterface,
		AccessControlDataBackendInterface,
		NetworkObjectDirectoryPluginInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.LocalData")
	Q_INTERFACES(PluginInterface AccessControlDataBackendInterface NetworkObjectDirectoryPluginInterface ConfigurationPagePluginInterface)
public:
	LocalDataPlugin( QObject* paren = nullptr );
	virtual ~LocalDataPlugin();

	Plugin::Uid uid() const override
	{
		return "14bacaaa-ebe5-449c-b881-5b382f952571";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "LocalData" );
	}

	QString description() const override
	{
		return tr( "Backends which use local data" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QString accessControlDataBackendName() const override
	{
		return tr( "Default (local users/groups and computers/rooms from configuration)" );
	}

	void reloadConfiguration() override;

	QStringList users() override;
	QStringList userGroups() override;
	QStringList groupsOfUser( const QString& username ) override;

	QStringList allRooms() override;
	QStringList roomsOfComputer( const QString& computerName ) override;

	QString directoryName() const override
	{
		return tr( "Default (store objects in local configuration)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	LocalDataConfiguration m_configuration;

};

#endif // LOCAL_DATA_PLUGIN_H
