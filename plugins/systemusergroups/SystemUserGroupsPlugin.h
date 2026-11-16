// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "UserGroupsBackendInterface.h"

class SystemUserGroupsPlugin : public QObject, PluginInterface, UserGroupsBackendInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.SystemUserGroups")
	Q_INTERFACES(PluginInterface UserGroupsBackendInterface)
public:
	explicit SystemUserGroupsPlugin(QObject* paren = nullptr);
	~SystemUserGroupsPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("2917cdeb-ac13-4099-8715-20368254a367")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("SystemUserGroups");
	}

	QString description() const override
	{
		return tr("User groups backend for system user groups");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QString userGroupsBackendName() const override
	{
		return tr("Default (system user groups)");
	}

	void reloadConfiguration() override;

	QStringList userGroups(bool queryDomainGroups) override;
	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override;
	QString userGroupSecurityIdentifier(const QString& groupName) override;
};
