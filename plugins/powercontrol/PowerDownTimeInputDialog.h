// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class PowerDownTimeInputDialog;
}

class PowerDownTimeInputDialog : public QDialog
{
	Q_OBJECT
public:
	explicit PowerDownTimeInputDialog(QWidget* parent);
	~PowerDownTimeInputDialog() override;

	int seconds() const;

private:
	void updateSeconds();

	Ui::PowerDownTimeInputDialog* ui;
	int m_seconds;
};
