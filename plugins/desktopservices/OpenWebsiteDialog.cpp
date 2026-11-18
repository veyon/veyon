// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenWebsiteDialog.h"
#include "QuickUi.h"
OpenWebsiteDialog::OpenWebsiteDialog(QWidget*, const QString& summary) : QObject(nullptr), m_summary(summary) {}
int OpenWebsiteDialog::exec()
{
	const auto result = QuickUi::execDialog(QStringLiteral("website"), {
		{QStringLiteral("title"), tr("Open website on student computers")}, {QStringLiteral("summary"), m_summary}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_website = result.value(QStringLiteral("value")).toString();
	m_remember = result.value(QStringLiteral("remember")).toBool();
	m_presetName = result.value(QStringLiteral("presetName")).toString();
	return 1;
}
