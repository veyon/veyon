/*
 * LdapBrowseDialog.cpp - dialog for browsing LDAP directories
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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

#include "LdapBrowseDialog.h"
#include "LdapBrowseModel.h"
#include "LdapConfiguration.h"

#include "ui_LdapBrowseDialog.h"


LdapBrowseDialog::LdapBrowseDialog( const LdapConfiguration& configuration, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::LdapBrowseDialog ),
	m_configuration( configuration )
{
	ui->setupUi( this );
}



LdapBrowseDialog::~LdapBrowseDialog()
{
	delete ui;
}



QString LdapBrowseDialog::browseBaseDn( const QString& dn )
{
	LdapBrowseModel model( LdapBrowseModel::BrowseBaseDN, m_configuration, this );

	return browse( &model, dn, false );
}



QString LdapBrowseDialog::browseDn( const QString& dn )
{
	LdapBrowseModel model( LdapBrowseModel::BrowseObjects, m_configuration, this );

	return browse( &model, dn, dn.toLower() == model.baseDn().toLower() );
}



QString LdapBrowseDialog::browseAttribute( const QString& dn )
{
	LdapBrowseModel model( LdapBrowseModel::BrowseAttributes, m_configuration, this );

	return browse( &model, dn, true );
}



QString LdapBrowseDialog::browse( LdapBrowseModel* model, const QString& dn, bool expandSelected )
{
	ui->treeView->setModel( model );

	if( dn.isEmpty() == false )
	{
		const auto dnIndex = model->dnToIndex( dn );
		ui->treeView->selectionModel()->setCurrentIndex( dnIndex, QItemSelectionModel::SelectCurrent );
		if( expandSelected )
		{
			ui->treeView->expand( dnIndex );
		}
	}

	if( exec() == QDialog::Accepted )
	{
		return model->data( ui->treeView->selectionModel()->currentIndex(), LdapBrowseModel::ItemNameRole ).toString();
	}

	return {};
}
