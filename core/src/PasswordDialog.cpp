// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMessageBox>
#include <QPushButton>

#include "PasswordDialog.h"
#include "PlatformUserFunctions.h"

#include "ui_PasswordDialog.h"


PasswordDialog::PasswordDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::PasswordDialog)
{
	ui->setupUi(this);

	ui->username->setText(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName));

	if (ui->username->text().isEmpty() == false)
	{
		ui->password->setFocus();
	}

	updateOkButton();
}



PasswordDialog::~PasswordDialog()
{
	delete ui;
}



QString PasswordDialog::username() const
{
	return ui->username->text();
}



CryptoCore::PlaintextPassword PasswordDialog::password() const
{
	return ui->password->text().toUtf8();
}



AuthenticationCredentials PasswordDialog::credentials() const
{
	AuthenticationCredentials cred;
	cred.setLogonUsername(username());
	cred.setLogonPassword(password());

	return cred;
}



void PasswordDialog::accept()
{
	if (VeyonCore::platform().userFunctions().authenticate(username(), password()) == false)
	{
		QMessageBox::critical(window(),
							  tr("Authentication error"),
							  tr("Logon failed with given username and password. Please try again!"));
	}
	else
	{
		QDialog::accept();
	}
}



void PasswordDialog::updateOkButton()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!username().isEmpty() && !password().isEmpty());
}
