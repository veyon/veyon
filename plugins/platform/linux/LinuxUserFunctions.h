// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusConnection>

#include "LogonHelper.h"
#include "PlatformUserFunctions.h"

#include <pwd.h>

// clazy:excludeall=copyable-polymorphic

class LinuxUserFunctions : public PlatformUserFunctions
{
public:
	using UserId = uid_t;
	using GroupId = gid_t;

	static constexpr UserId InvalidUserId = -1;
	static constexpr GroupId InvalidGroupId = -1;

	QString queryCurrentUserProperty(UserProperty property) override;

	QStringList userGroups(bool queryDomainGroups) override;
	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override;
	QString userGroupSecurityIdentifier(const QString& groupName) override;

	bool isAnyUserLoggedOn() override;

	bool prepareLogon(const QString& username, const Password& password) override;
	bool performLogon(const QString& username, const Password& password) override;
	void logoff() override;

	bool authenticate(const QString& username, const Password& password) override;

	static UserId userIdFromName(const QString& username);
	static GroupId userGroupIdFromName(const QString& username);

	static QVariant getUserProperty(const QString& userPath, const QString& property, bool logErrors = true);

private:
	QString currentUserLoginName() const;
	QString currentUserFullName() const;

	QDBusConnection m_systemBus = QDBusConnection::systemBus();

	// must exceed the timeout used by the service when talking to the auth helper
	static constexpr auto AuthHelperTimeout = 15000;

	LogonHelper m_logonHelper{};
};
