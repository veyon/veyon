// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT PlatformUserFunctions
{
	Q_GADGET
public:
	enum class UserProperty {
		None,
		LoginName,
		FullName,
	};
	Q_ENUM(UserProperty)

	using Password = CryptoCore::SecureArray;

	// identity of a user for the peer checks of local IPC endpoints (02-ipc.md §6a)
	struct UserIdentity
	{
		qint64 userId = -1;			  // Linux: uid
		QString securityIdentifier{}; // Windows: SID string
	};

	virtual ~PlatformUserFunctions() = default;

	virtual QString queryCurrentUserProperty(UserProperty property) = 0;

	virtual QStringList userGroups(bool queryDomainGroups) = 0;
	virtual QStringList groupsOfUser(const QString& username, bool queryDomainGroups) = 0;
	virtual QString userGroupSecurityIdentifier(const QString& groupName) = 0;

	virtual bool isAnyUserLoggedOn() = 0;

	// empty identity if the user is unknown
	virtual UserIdentity queryUserIdentity(const QString& username) = 0;

	virtual bool prepareLogon(const QString& username, const Password& password) = 0;
	virtual bool performLogon(const QString& username, const Password& password) = 0;
	virtual void logoff() = 0;

	virtual bool authenticate(const QString& username, const Password& password) = 0;
};
