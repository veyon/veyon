// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LogonHelper.h"
#include "PlatformUserFunctions.h"
#include "WindowsCoreFunctions.h"

// clazy:exclude=copyable-polymorphic

class WindowsUserFunctions : public PlatformUserFunctions
{
public:
	QString queryCurrentUserProperty(UserProperty property) override;

	QStringList userGroups(bool queryDomainGroups) override;
	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override;
	QString userGroupSecurityIdentifier(const QString& groupName) override;

	bool isAnyUserLoggedOn() override;

	bool prepareLogon(const QString& username, const Password& password) override;
	bool performLogon(const QString& username, const Password& password) override;

	void logoff() override;

	bool authenticate(const QString& username, const Password& password) override;


private:
	static QString currentUserLoginName();
	static QString currentUserFullName();

	static QString domainFromUsername(const QString& username);
	static QString domainController(const QString& domainName = {});
	static QStringList domainUserGroups();
	static QStringList domainGroupsOfUser(const QString& username);

	static QStringList localUserGroups();
	static QStringList localGroupsOfUser(const QString& username);

	LogonHelper m_logonHelper{};
};
