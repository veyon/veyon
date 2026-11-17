// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IpcClient.h"
#include "LogonHelper.h"
#include "PersistentLogonCredentials.h"
#include "PlatformPluginInterface.h"
#include "PlatformUserFunctions.h"


LogonHelper::LogonHelper(QObject* parent) :
	QObject(parent)
{
	connect(VeyonCore::instance(), &VeyonCore::applicationLoaded,
			this, &LogonHelper::checkPendingLogonTasks);
}



bool LogonHelper::prepare(const QString& username, const Password& password)
{
	if (VeyonCore::platform().userFunctions().isAnyUserLoggedOn())
	{
		vInfo() << "Skipping user logon as a user is already logged on";
		return false;
	}

	return PersistentLogonCredentials::write(username, password);
}



void LogonHelper::checkPendingLogonTasks()
{
	if (VeyonCore::component() == VeyonCore::Component::Server &&
		VeyonCore::serviceIpcClient() &&
		VeyonCore::serviceIpcClient()->isConnected() &&
		VeyonCore::platform().userFunctions().isAnyUserLoggedOn() == false)
	{
		vDebug() << "Reading logon credentials";
		QString username;
		Password password;
		// the service hands out the credentials only once
		if (PersistentLogonCredentials::read(&username, &password))
		{
			VeyonCore::platform().userFunctions().performLogon(username, password);
		}
	}
}
