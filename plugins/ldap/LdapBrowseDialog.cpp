// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

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
