// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TextMessageDialog.h"
#include "QuickUi.h"
#include "PlatformUserFunctions.h"
TextMessageDialog::TextMessageDialog(QString& message, QString& title, QWidget*, const QString& summary)
	: QObject(nullptr), m_message(message), m_title(title), m_summary(summary) {}
int TextMessageDialog::exec()
{
	auto title = m_title;
	if (title.isEmpty())
	{
		auto name = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::FullName);
		if (name.isEmpty())
			name = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
		title = name.isEmpty() ? tr("Message from teacher") : tr("Message from %1").arg(name);
	}
	const auto result = QuickUi::execDialog(QStringLiteral("message"), {
		{QStringLiteral("title"), tr("Message to students")}, {QStringLiteral("summary"), m_summary},
		{QStringLiteral("value"), m_message}, {QStringLiteral("messageTitle"), title}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_message = result.value(QStringLiteral("value")).toString();
	m_title = result.value(QStringLiteral("title")).toString().trimmed();
	return 1;
}
