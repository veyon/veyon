// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMessageBox>

#include "AccessControlProvider.h"
#include "AccessControlRulesTestDialog.h"
#include "HostAddress.h"
#include "PlatformUserFunctions.h"

#include "ui_AccessControlRulesTestDialog.h"


AccessControlRulesTestDialog::AccessControlRulesTestDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRulesTestDialog)
{
	ui->setupUi(this);

	ui->localUserLineEdit->setText(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName));
	ui->localComputerLineEdit->setText(HostAddress::localFQDN());
}



AccessControlRulesTestDialog::~AccessControlRulesTestDialog()
{
	delete ui;
}



int AccessControlRulesTestDialog::exec()
{
	ui->accessingUserLineEdit->setFocus();

	return QDialog::exec();
}



void AccessControlRulesTestDialog::accept()
{
	const auto result =
		AccessControlProvider().processAccessControlRules(ui->accessingUserLineEdit->text(),
														  ui->accessingComputerLineEdit->text(),
														  ui->localUserLineEdit->text(),
														  ui->localComputerLineEdit->text(),
														  ui->connectedUsersLineEdit->text().isEmpty() ?
															  QStringList() :
															  ui->connectedUsersLineEdit->text().split(QLatin1Char(',')));
	QString resultText;

	switch (result ? result->action() : AccessControlRule::Action::None)
	{
	case AccessControlRule::Action::Allow:
		resultText = tr("The access in the given scenario is allowed.");
		break;
	case AccessControlRule::Action::Deny:
		resultText = tr("The access in the given scenario is denied.");
		break;
	case AccessControlRule::Action::AskForPermission:
		resultText = tr("The access in the given scenario needs permission of the logged on user.");
		break;
	default:
		resultText = tr("There is no matching rule with a valid action. The access is therefore denied.");
		break;
	}

	QMessageBox::information(this, tr("Test result"), resultText);
}
