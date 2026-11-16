// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "AuthenticationCredentials.h"

namespace Ui {
class PasswordDialog;
}

class VEYON_CORE_EXPORT PasswordDialog : public QDialog
{
	Q_OBJECT
public:
	explicit PasswordDialog(QWidget* parent);
	~PasswordDialog() override;

	QString username() const;
	CryptoCore::PlaintextPassword password() const;

	AuthenticationCredentials credentials() const;

	void accept() override;

private Q_SLOTS:
	void updateOkButton();


private:
	Ui::PasswordDialog* ui;
};
