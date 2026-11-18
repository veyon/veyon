// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

#include "LocalCredentialsVerifier.h"
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
	// the Veyon Server of this session checks the credentials like a remote master's logon
	setEnabled(false);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto result = LocalCredentialsVerifier::verify(
		username(), Veyon::Crypto::SecureBuffer::fromByteArray(password().toByteArray()));
	QApplication::restoreOverrideCursor();
	setEnabled(true);

	switch (result)
	{
	case LocalCredentialsVerifier::Result::Valid:
		QDialog::accept();
		break;

	case LocalCredentialsVerifier::Result::Invalid:
		QMessageBox::critical(window(),
							  tr("Authentication error"),
							  tr("Logon failed with given username and password. Please try again!"));
		break;

	case LocalCredentialsVerifier::Result::Unavailable:
		// a mistyped password must not be sent to all computers unnoticed
		if (QMessageBox::question(window(), tr("Authentication"),
								  tr("The password of the account \"%1\" cannot be verified on this computer. It "
									 "will be verified by the computers you connect to. If it is wrong, access will "
									 "be denied and the account may get locked. Continue?")
									  .arg(username()),
								  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
		{
			QDialog::accept();
		}
		break;
	}
}



void PasswordDialog::updateOkButton()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!username().isEmpty() && !password().isEmpty());
}
