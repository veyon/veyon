/*
 * PasswordDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <veyonconfig.h>

#include <QMessageBox>
#include <QPushButton>

#include "PasswordDialog.h"
#include "LocalSystem.h"
#include "LogonAuthentication.h"

#include "ui_PasswordDialog.h"


PasswordDialog::PasswordDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::PasswordDialog )
{
	ui->setupUi( this );

	const LocalSystem::User loggedOnUser = LocalSystem::User::loggedOnUser();
	QString username = loggedOnUser.name();
#ifdef VEYON_BUILD_WIN32
	if( !username.isEmpty() && !loggedOnUser.domain().isEmpty() )
	{
		username = loggedOnUser.domain() + "\\" + username;
	}
#endif

	ui->username->setText( username );

	if( !username.isEmpty() )
	{
		ui->password->setFocus();
	}

	updateOkButton();

	VeyonCore::enforceBranding( this );
}




PasswordDialog::~PasswordDialog()
{
}



QString PasswordDialog::username() const
{
	return ui->username->text();
}



QString PasswordDialog::password() const
{
	return ui->password->text();
}




AuthenticationCredentials PasswordDialog::credentials() const
{
	AuthenticationCredentials cred;
	cred.setLogonUsername( username() );
	cred.setLogonPassword( password() );

	return cred;
}



void PasswordDialog::accept()
{
	if( LogonAuthentication::authenticateUser( credentials() ) == false )
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



void PasswordDialog::updateOkButton()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( !username().isEmpty() && !password().isEmpty() );
}

