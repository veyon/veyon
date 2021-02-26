/*
 * UserLoginDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QPushButton>

#include "UserLoginDialog.h"

#include "ui_UserLoginDialog.h"


UserLoginDialog::UserLoginDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::UserLoginDialog )
{
	ui->setupUi( this );

	if( ui->username->text().isEmpty() == false )
	{
		ui->password->setFocus();
	}

	updateOkButton();

	VeyonCore::enforceBranding( this );
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
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( !username().isEmpty() && !password().isEmpty() );
}
