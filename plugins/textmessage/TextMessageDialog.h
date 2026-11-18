// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
class QWidget;
class TextMessageDialog : public QObject
{
	Q_OBJECT
public:
	TextMessageDialog(QString& message, QString& title, QWidget* parent, const QString& summary = {});
	int exec();
private:
	QString& m_message;
	QString& m_title;
	QString m_summary;
};
