// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPushButton>

#include "UserLoginDialog.h"

#include "ui_UserLoginDialog.h"


UserLoginDialog::UserLoginDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::UserLoginDialog)
{
	ui->setupUi(this);

	if (ui->username->text().isEmpty() == false)
	{
		ui->password->setFocus();
	}

	updateOkButton();
}



UserLoginDialog::~UserLoginDialog()
{
	delete ui;
}



QString UserLoginDialog::username() const
{
	return ui->username->text();
}



CryptoCore::PlaintextPassword UserLoginDialog::password() const
{
	return ui->password->text().toUtf8();
}



void UserLoginDialog::updateOkButton()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!username().isEmpty());
}
