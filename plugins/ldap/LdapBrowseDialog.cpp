/*:;
 * LdapBrowseDialog.cpp - class representing the LDAP directory and providing access to directory entries
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

#include "LdapClient.h"
#include "LdapBrowseDialog.h"

#include "ldapstructureproxymodel.h"
#include "ldapmodel.h"

#include "ui_LdapBrowseDialog.h"

LdapBrowseDialog::LdapBrowseDialog( const LdapConfiguration& ldapConfiguration, QWidget* parent ) :
	QDialog( parent ),
	m_ldapClient( new LdapClient( ldapConfiguration, QUrl(), this ) ),
	ui( new Ui::LdapBrowseDialog ),
	m_ldapModel( new KLDAP::LdapModel( this ) ),
	m_sortFilterProxyModel( new KLDAP::LdapStructureProxyModel( this ) )
{
	ui->setupUi( this );

	connect( m_ldapModel, &KLDAP::LdapModel::ready, this, &LdapBrowseDialog::setModels );

	m_ldapModel->setConnection( m_ldapClient->connection() );
}



LdapBrowseDialog::~LdapBrowseDialog()
{
	delete ui;
	delete m_ldapClient;
}



void LdapBrowseDialog::setModels()
{
	m_sortFilterProxyModel->setSourceModel( m_ldapModel );
	ui->treeView->setModel( m_sortFilterProxyModel );
}
