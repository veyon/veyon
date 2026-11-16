// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class AccessControlRulesTestDialog;
}

class AccessControlRulesTestDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AccessControlRulesTestDialog(QWidget* parent = nullptr);
	~AccessControlRulesTestDialog() override;

	int exec() override;
	void accept() override;

private:
	Ui::AccessControlRulesTestDialog* ui;
};
