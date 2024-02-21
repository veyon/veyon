/*
 * AuthSimpleDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2019-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>
#include <QPushButton>

#include "AuthSimpleDialog.h"
#include "AuthSimpleConfiguration.h"
#include "VeyonConfiguration.h"

#include "ui_AuthSimpleDialog.h"


AuthSimpleDialog::AuthSimpleDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::AuthSimpleDialog )
{
	ui->setupUi( this );
	ui->password->setFocus();

	connect( ui->password, &QLineEdit::textChanged, this, [this]() {
		ui->buttonBox->button( QDialogButtonBox::Ok )->setDisabled( password().isEmpty() );
	} );

	ui->buttonBox->button( QDialogButtonBox::Ok )->setDisabled( true );

	VeyonCore::enforceBranding( this );
}



AuthSimpleDialog::~AuthSimpleDialog()
{
	delete ui;
}



CryptoCore::PlaintextPassword AuthSimpleDialog::password() const
{
	return ui->password->text().toUtf8();
}



void AuthSimpleDialog::accept()
{
	AuthSimpleConfiguration config( &VeyonCore::config() );

	if( config.password().plainText() != password() )
	{
		QMessageBox::critical( window(),
							   tr( "Authentication error" ),
							   tr( "Logon failed with given password. Please try again!" ) );
	}
	else
	{
		QDialog::accept();
	}
}
