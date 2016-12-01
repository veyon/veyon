/*
 * AccessControlPage.cpp - implementation of the access control page in IMC
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "AccessControlPage.h"
#include "AccessControlRule.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "Configuration/UiMapping.h"
#include "AccessControlRuleEditDialog.h"

#include "ui_AccessControlPage.h"

AccessControlPage::AccessControlPage() :
	ConfigurationPage(),
	ui(new Ui::AccessControlPage),
	m_accessControlListModel( this )
{
	ui->setupUi(this);

	ui->accessControlListView->setModel( &m_accessControlListModel );

	updateAccessGroupsLists();
	updateAccessControlList();
}



AccessControlPage::~AccessControlPage()
{
	delete ui;
}



void AccessControlPage::resetWidgets()
{
	FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	m_accessGroups = ItalcCore::config->logonGroups();

	updateAccessGroupsLists();
}



void AccessControlPage::connectWidgetsToProperties()
{
	FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void AccessControlPage::addAccessGroup()
{
	foreach( QListWidgetItem *item, ui->allGroupsList->selectedItems() )
	{
		m_accessGroups.removeAll( item->text() );
		m_accessGroups += item->text();
	}

	ItalcCore::config->setLogonGroups( m_accessGroups );

	updateAccessGroupsLists();
}



void AccessControlPage::removeAccessGroup()
{
	foreach( QListWidgetItem *item, ui->accessGroupsList->selectedItems() )
	{
		m_accessGroups.removeAll( item->text() );
	}

	ItalcCore::config->setLogonGroups( m_accessGroups );

	updateAccessGroupsLists();
}



void AccessControlPage::updateAccessGroupsLists()
{
	ui->accessGroupsList->setUpdatesEnabled( false );

	ui->allGroupsList->clear();
	ui->accessGroupsList->clear();

	for( auto group : LocalSystem::userGroups() )
	{
		if( m_accessGroups.contains( group ) )
		{
			ui->accessGroupsList->addItem( group );
		}
		else
		{
			ui->allGroupsList->addItem( group );
		}
	}

	ui->accessGroupsList->setUpdatesEnabled( true );
}


void AccessControlPage::addAccessControlRule()
{
	AccessControlRule newRule;

	if( AccessControlRuleEditDialog( newRule, this ).exec() )
	{
		ItalcCore::config->setAccessControlList( ItalcCore::config->accessControlList() << newRule.encode() );

		updateAccessControlList();
	}
}



void AccessControlPage::removeAccessControlRule()
{
	QStringList accessControlList = ItalcCore::config->accessControlList();

	accessControlList.removeAt( ui->accessControlListView->currentIndex().row() );

	ItalcCore::config->setAccessControlList( accessControlList );

	updateAccessControlList();
}



void AccessControlPage::editAccessControlRule()
{
	QStringList accessControlList = ItalcCore::config->accessControlList();

	int row = ui->accessControlListView->currentIndex().row();

	QString encodedRule = accessControlList.value( row );
	if( encodedRule.isEmpty() )
	{
		return;
	}

	AccessControlRule rule( encodedRule );

	if( AccessControlRuleEditDialog( rule, this ).exec() )
	{
		accessControlList.replace( row, rule.encode() );

		modifyAccessControlList( accessControlList, row );
	}
}



void AccessControlPage::moveAccessControlRuleDown()
{
	QStringList accessControlList = ItalcCore::config->accessControlList();

	int row = ui->accessControlListView->currentIndex().row();
	int newRow = row + 1;

	if( newRow < accessControlList.size() )
	{
		accessControlList.move( row, newRow );

		modifyAccessControlList( accessControlList, newRow );
	}
}



void AccessControlPage::moveAccessControlRuleUp()
{
	QStringList accessControlList = ItalcCore::config->accessControlList();

	int row = ui->accessControlListView->currentIndex().row();
	int newRow = row - 1;

	if( newRow >= 0 )
	{
		accessControlList.move( row, newRow );

		modifyAccessControlList( accessControlList, newRow );
	}
}



void AccessControlPage::modifyAccessControlList(const QStringList &accessControlList, int selectedRow)
{
	ItalcCore::config->setAccessControlList( accessControlList );

	updateAccessControlList();

	ui->accessControlListView->setCurrentIndex( m_accessControlListModel.index( selectedRow ) );
}



void AccessControlPage::updateAccessControlList()
{
	QStringList ruleNames;

	for( auto encodedRule : ItalcCore::config->accessControlList() )
	{
		ruleNames += AccessControlRule( encodedRule ).name();
	}

	m_accessControlListModel.setStringList( ruleNames );
}
