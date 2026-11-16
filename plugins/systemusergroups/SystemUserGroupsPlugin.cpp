// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlatformPluginInterface.h"
#include "PlatformUserFunctions.h"
#include "SystemUserGroupsPlugin.h"


SystemUserGroupsPlugin::SystemUserGroupsPlugin(QObject* parent) :
	QObject(parent)
{
}



void SystemUserGroupsPlugin::reloadConfiguration()
{
}



QStringList SystemUserGroupsPlugin::userGroups(bool queryDomainGroups)
{
	return VeyonCore::platform().userFunctions().userGroups(queryDomainGroups);
}



QStringList SystemUserGroupsPlugin::groupsOfUser(const QString& username, bool queryDomainGroups)
{
	return VeyonCore::platform().userFunctions().groupsOfUser(username, queryDomainGroups);
}



QString SystemUserGroupsPlugin::userGroupSecurityIdentifier(const QString& groupName)
{
	return VeyonCore::platform().userFunctions().userGroupSecurityIdentifier(groupName);
}
