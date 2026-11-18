// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
class QWidget;
class PowerDownTimeInputDialog : public QObject
{
	Q_OBJECT
public:
	explicit PowerDownTimeInputDialog(QWidget* parent);
	int exec();
	int seconds() const { return m_seconds; }
private:
	int m_seconds = 0;
};
