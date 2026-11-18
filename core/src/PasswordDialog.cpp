// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PasswordDialog.h"
#include "PlatformUserFunctions.h"
#include "QuickUi.h"
PasswordDialog::PasswordDialog(QWidget*) : QObject(nullptr),
	m_username(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName)) {}
int PasswordDialog::exec()
{
	const auto result = QuickUi::execDialog(QStringLiteral("authenticate"), {
		{QStringLiteral("title"), tr("Logon")}, {QStringLiteral("username"), m_username}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_username = result.value(QStringLiteral("username")).toString();
	m_password = result.value(QStringLiteral("password")).toString().toUtf8();
	return 1;
}
AuthenticationCredentials PasswordDialog::credentials() const
{
	AuthenticationCredentials result;
	result.setLogonUsername(m_username);
	result.setLogonPassword(m_password);
	return result;
}
