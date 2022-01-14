/*
 * AuthLdapDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2010-2022 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthLdapDialog.h"
#include "AuthLdapCore.h"
#include "PlatformUserFunctions.h"

#include "ui_AuthLdapDialog.h"


AuthLdapDialog::AuthLdapDialog( const LdapConfiguration& config, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::AuthLdapDialog ),
	m_config( config )
{
	ui->setupUi( this );

	ui->username->setText( VeyonCore::platform().userFunctions().currentUser() );

	if( ui->username->text().isEmpty() == false )
	{
		ui->password->setFocus();
	}

	updateOkButton();

	VeyonCore::enforceBranding( this );
}



AuthLdapDialog::~AuthLdapDialog()
{
	delete ui;
}



QString AuthLdapDialog::username() const
{
	return ui->username->text();
}



CryptoCore::PlaintextPassword AuthLdapDialog::password() const
{
	return ui->password->text().toUtf8();
}



void AuthLdapDialog::accept()
{
	AuthLdapCore core;
	core.setUsername( username() );
	core.setPassword( password() );

	if( core.authenticate() == false )
	{
		QMessageBox::critical( window(),
							   tr( "Authentication error" ),
							   tr( "Logon failed with given username and password. Please try again!" ) );
	}
	else
	{
		QDialog::accept();
	}
}



void AuthLdapDialog::updateOkButton()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( !username().isEmpty() && !password().isEmpty() );
}
