// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "UserLoginDialog.h"
#include "QuickUi.h"
UserLoginDialog::UserLoginDialog(QWidget*) : QObject(nullptr) {}
int UserLoginDialog::exec()
{
	const auto result = QuickUi::execDialog(QStringLiteral("login"), {
		{QStringLiteral("title"), tr("User logon")}, {QStringLiteral("allowEmptyPassword"), true}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_username = result.value(QStringLiteral("username")).toString();
	m_password = result.value(QStringLiteral("password")).toString().toUtf8();
	return 1;
}
