/*
 * PasswordDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <italcconfig.h>

#include <QtGui/QPushButton>

#include "PasswordDialog.h"
#include "LocalSystem.h"

#include "ui_PasswordDialog.h"


PasswordDialog::PasswordDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::PasswordDialog )
{
	ui->setupUi( this );

	const LocalSystem::User loggedOnUser = LocalSystem::User::loggedOnUser();
	QString userName = loggedOnUser.name();
#ifdef ITALC_BUILD_WIN32
	if( !userName.isEmpty() && !loggedOnUser.domain().isEmpty() )
	{
		userName = loggedOnUser.domain() + "\\" + userName;
	}
#endif

	ui->username->setText( userName );

	if( !userName.isEmpty() )
	{
		ui->password->setFocus();
	}

	updateOkButton();
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



void PasswordDialog::updateOkButton()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( !username().isEmpty() && !password().isEmpty() );
}

