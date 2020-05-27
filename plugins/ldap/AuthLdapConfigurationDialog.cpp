/*
 * AuthLdapConfigurationDialog.cpp - implementation of the authentication configuration page
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthLdapConfiguration.h"
#include "AuthLdapConfigurationDialog.h"
#include "VeyonConfiguration.h"

#include "ui_AuthLdapConfigurationDialog.h"


AuthLdapConfigurationDialog::AuthLdapConfigurationDialog( AuthLdapConfiguration& configuration ) :
	QDialog( QApplication::activeWindow() ),
	ui( new Ui::AuthLdapConfigurationDialog ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	ui->usernameToBindDnMapping->setText( configuration.usernnameBindDnMapping() );
}



AuthLdapConfigurationDialog::~AuthLdapConfigurationDialog()
{
	delete ui;
}



void AuthLdapConfigurationDialog::accept()
{
	m_configuration.setUsernameBindMapping( ui->usernameToBindDnMapping->text() );

	QDialog::accept();
}
