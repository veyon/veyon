/*
 * AccessControlRulesTestDialog.cpp - dialog for testing access control rules
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QHostInfo>
#include <QMessageBox>

#include "AccessControlRulesTestDialog.h"
#include "AccessControlProvider.h"
#include "PlatformUserFunctions.h"

#include "ui_AccessControlRulesTestDialog.h"


AccessControlRulesTestDialog::AccessControlRulesTestDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRulesTestDialog)
{
	ui->setupUi(this);

	ui->localUserLineEdit->setText( VeyonCore::platform().userFunctions().currentUser() );
	ui->localComputerLineEdit->setText( QHostInfo::localHostName() );
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
			AccessControlProvider().processAccessControlRules( ui->accessingUserLineEdit->text(),
															   ui->accessingComputerLineEdit->text(),
															   ui->localUserLineEdit->text(),
															   ui->localComputerLineEdit->text(),
															   ui->connectedUsersLineEdit->text().split( QLatin1Char(',') ) );
	QString resultText;

	switch( result )
	{
	case AccessControlRule::ActionAllow:
		resultText = tr( "The access in the given scenario is allowed." );
		break;
	case AccessControlRule::ActionDeny:
		resultText = tr( "The access in the given scenario is denied." );
		break;
	case AccessControlRule::ActionAskForPermission:
		resultText = tr( "The access in the given scenario needs permission of the logged on user." );
		break;
	default:
		resultText = tr( "ERROR: Unknown action" );
		break;
	}

	QMessageBox::information( this, tr( "Test result" ), resultText );
}
