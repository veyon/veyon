// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PowerDownTimeInputDialog.h"
#include "QuickUi.h"
PowerDownTimeInputDialog::PowerDownTimeInputDialog(QWidget*) : QObject(nullptr) {}
int PowerDownTimeInputDialog::exec()
{
	const auto result = QuickUi::execDialog(QStringLiteral("schedule"), {{QStringLiteral("title"), tr("Delayed power down")}});
	if (!result.value(QStringLiteral("accepted")).toBool()) return 0;
	m_seconds = result.value(QStringLiteral("seconds")).toInt();
	return m_seconds > 0 ? 1 : 0;
}
