/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
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

#include "AccessControlRuleEditDialog.h"

#include "ui_AccessControlRuleEditDialog.h"

AccessControlRuleEditDialog::AccessControlRuleEditDialog(AccessControlRule &rule, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRuleEditDialog),
	m_rule( rule ),
	m_entityNameMap( {
			{ AccessControlRule::EntityAccessingUser, tr( "Accessing user" ) },
			{ AccessControlRule::EntityAccessingComputer, tr( "Accessing computer" ) },
			{ AccessControlRule::EntityLocalUser, tr( "Local (logged on) user" ) },
			{ AccessControlRule::EntityLocalComputer, tr( "Local computer" ) },
		} )
{
	ui->setupUi(this);

	// populate comboboxes
	QStringList entityNames = m_entityNameMap.values();
	ui->entityComboBox->addItems( entityNames );
	ui->commonGroupComboBox->addItems( entityNames );
	ui->commonComputerPoolComboBox->addItems( entityNames );

	// load general settings
	ui->ruleNameLineEdit->setText( rule.name() );
	ui->ruleDescriptionLineEdit->setText( rule.description() );

	// load selected entity
	ui->entityComboBox->setCurrentText( m_entityNameMap.value( rule.entity() ) );

	// load conditions
	ui->isMemberOfGroupCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionMemberOfGroup ) );
	ui->isMemberOfComputerPoolCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionMemberOfComputerPool ) );
	ui->hasCommonGroupsCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionGroupsInCommon ) );
	ui->hasCommonComputerPoolsCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionComputerPoolsInCommon ) );

	// load condition arguments
	ui->groupsComboBox->setCurrentText( rule.conditionArgument( AccessControlRule::ConditionMemberOfGroup ).toString() );

	ui->computerPoolsComboBox->setCurrentText( rule.conditionArgument( AccessControlRule::ConditionMemberOfComputerPool ).toString() );

	ui->commonGroupComboBox->setCurrentText(
				m_entityNameMap.value( rule.conditionArgument( AccessControlRule::ConditionGroupsInCommon ).
									   value<AccessControlRule::Entity>() ) );

	ui->commonComputerPoolComboBox->setCurrentText(
				m_entityNameMap.value( rule.conditionArgument( AccessControlRule::ConditionComputerPoolsInCommon ).
									   value<AccessControlRule::Entity>() ) );

	// load action
	ui->actionNoneRadioButton->setChecked( rule.action() == AccessControlRule::ActionNone );
	ui->actionAllowRadioButton->setChecked( rule.action() == AccessControlRule::ActionAllow );
	ui->actionDenyRadioButton->setChecked( rule.action() == AccessControlRule::ActionDeny );
}



AccessControlRuleEditDialog::~AccessControlRuleEditDialog()
{
	delete ui;
}



void AccessControlRuleEditDialog::accept()
{
	// save general settings
	m_rule.setName( ui->ruleNameLineEdit->text() );
	m_rule.setDescription( ui->ruleDescriptionLineEdit->text() );

	// save selected entity
	m_rule.setEntity( m_entityNameMap.key( ui->entityComboBox->currentText() ) );

	// save conditions
	if( ui->isMemberOfGroupCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionMemberOfGroup, ui->groupsComboBox->currentText() );
	}

	if( ui->isMemberOfComputerPoolCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionMemberOfComputerPool, ui->computerPoolsComboBox->currentText() );
	}

	if( ui->hasCommonGroupsCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionGroupsInCommon,
							 m_entityNameMap.key( ui->commonGroupComboBox->currentText() ) );
	}

	if( ui->hasCommonComputerPoolsCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionComputerPoolsInCommon,
							 m_entityNameMap.key( ui->commonComputerPoolComboBox->currentText() ) );
	}

	// save action
	if( ui->actionAllowRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::ActionAllow );
	}
	else if( ui->actionDenyRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::ActionDeny );
	}
	else
	{
		m_rule.setAction( AccessControlRule::ActionNone );
	}

	QDialog::accept();
}
