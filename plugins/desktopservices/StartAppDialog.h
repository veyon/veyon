// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
class QWidget;
class StartAppDialog : public QObject
{
	Q_OBJECT
public:
	explicit StartAppDialog(QWidget* parent, const QString& summary = {});
	int exec();
	const QString& apps() const { return m_apps; }
	bool remember() const { return m_remember; }
	const QString& presetName() const { return m_presetName; }
private:
	QString m_summary, m_apps, m_presetName;
	bool m_remember = false;
};
