// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

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
