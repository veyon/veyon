// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "CryptoCore.h"

namespace Ui {
class UserLoginDialog;
}

class UserLoginDialog : public QDialog
{
	Q_OBJECT
public:
	explicit UserLoginDialog(QWidget* parent);
	~UserLoginDialog() override;

	QString username() const;
	CryptoCore::PlaintextPassword password() const;

private Q_SLOTS:
	void updateOkButton();

private:
	Ui::UserLoginDialog* ui;
};
