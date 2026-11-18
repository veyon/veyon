// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "StartAppDialog.h"
#include "QuickUi.h"
StartAppDialog::StartAppDialog(QWidget*, const QString& summary) : QObject(nullptr), m_summary(summary) {}
int StartAppDialog::exec()
{
	const auto result = QuickUi::execDialog(QStringLiteral("application"), {
		{QStringLiteral("title"), tr("Run application on student computers")}, {QStringLiteral("summary"), m_summary}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_apps = result.value(QStringLiteral("value")).toString();
	m_remember = result.value(QStringLiteral("remember")).toBool();
	m_presetName = result.value(QStringLiteral("presetName")).toString();
	return 1;
}
