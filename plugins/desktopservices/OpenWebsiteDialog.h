// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
class QWidget;
class OpenWebsiteDialog : public QObject
{
	Q_OBJECT
public:
	explicit OpenWebsiteDialog(QWidget* parent, const QString& summary = {});
	int exec();
	const QString& website() const { return m_website; }
	bool remember() const { return m_remember; }
	const QString& presetName() const { return m_presetName; }
private:
	QString m_summary, m_website, m_presetName;
	bool m_remember = false;
};
