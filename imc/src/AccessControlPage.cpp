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
#include "AccessControlProvider.h"
#include "Configuration/UiMapping.h"
#include "AccessControlRuleEditDialog.h"
#include "AccessControlRulesTestDialog.h"

#include "ui_AccessControlPage.h"

AccessControlPage::AccessControlPage() :
	ConfigurationPage(),
	ui(new Ui::AccessControlPage),
	m_accessControlRulesModel( this )
{
	ui->setupUi(this);

	ui->accessControlRulesView->setModel( &m_accessControlRulesModel );

	updateAccessGroupsLists();
	updateAccessControlRules();
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
	updateAccessControlRules();
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

	for( auto group : AccessControlProvider().userGroups() )
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
	newRule.setAction( AccessControlRule::ActionAllow );

	if( AccessControlRuleEditDialog( newRule, this ).exec() )
	{
		ItalcCore::config->setAccessControlRules( ItalcCore::config->accessControlRules() << newRule.encode() );

		updateAccessControlRules();
	}
}



void AccessControlPage::removeAccessControlRule()
{
	QStringList accessControlRules = ItalcCore::config->accessControlRules();

	accessControlRules.removeAt( ui->accessControlRulesView->currentIndex().row() );

	ItalcCore::config->setAccessControlRules( accessControlRules );

	updateAccessControlRules();
}



void AccessControlPage::editAccessControlRule()
{
	QStringList accessControlRules = ItalcCore::config->accessControlRules();

	int row = ui->accessControlRulesView->currentIndex().row();

	QString encodedRule = accessControlRules.value( row );
	if( encodedRule.isEmpty() )
	{
		return;
	}

	AccessControlRule rule( encodedRule );

	if( AccessControlRuleEditDialog( rule, this ).exec() )
	{
		accessControlRules.replace( row, rule.encode() );

		modifyAccessControlRules( accessControlRules, row );
	}
}



void AccessControlPage::moveAccessControlRuleDown()
{
	QStringList accessControlRules = ItalcCore::config->accessControlRules();

	int row = ui->accessControlRulesView->currentIndex().row();
	int newRow = row + 1;

	if( newRow < accessControlRules.size() )
	{
		accessControlRules.move( row, newRow );

		modifyAccessControlRules( accessControlRules, newRow );
	}
}



void AccessControlPage::moveAccessControlRuleUp()
{
	QStringList accessControlRules = ItalcCore::config->accessControlRules();

	int row = ui->accessControlRulesView->currentIndex().row();
	int newRow = row - 1;

	if( newRow >= 0 )
	{
		accessControlRules.move( row, newRow );

		modifyAccessControlRules( accessControlRules, newRow );
	}
}



void AccessControlPage::testUserGroupsAccessControl()
{

}



void AccessControlPage::testAccessControlRules()
{
	AccessControlRulesTestDialog( this ).exec();
}



void AccessControlPage::modifyAccessControlRules(const QStringList &accessControlRules, int selectedRow)
{
	ItalcCore::config->setAccessControlRules( accessControlRules );

	updateAccessControlRules();

	ui->accessControlRulesView->setCurrentIndex( m_accessControlRulesModel.index( selectedRow ) );
}



void AccessControlPage::updateAccessControlRules()
{
	m_accessControlRulesModel.reload();
}
