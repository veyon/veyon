/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "AccessControlProvider.h"
#include "UsersAndGroupsBackendManager.h"

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

	AccessControlProvider accessControlProvider;

	// populate comboboxes
	QStringList entityNames = m_entityNameMap.values();
	ui->entityComboBox->addItems( entityNames );
	ui->groupsComboBox->addItems( accessControlProvider.userGroups() );
	ui->commonGroupComboBox->addItems( entityNames );
	ui->computerLabsComboBox->addItems( accessControlProvider.computerLabs() );
	ui->commonComputerLabComboBox->addItems( entityNames );

	// load general settings
	ui->ruleNameLineEdit->setText( rule.name() );
	ui->ruleDescriptionLineEdit->setText( rule.description() );

	// load selected entity
	ui->entityComboBox->setCurrentText( m_entityNameMap.value( rule.entity() ) );

	// load condition invertion setting
	ui->invertConditionsCheckBox->setChecked( rule.areConditionsInverted() );

	// load conditions
	ui->isMemberOfGroupCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionMemberOfGroup ) );
	ui->hasCommonGroupsCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionGroupsInCommon ) );
	ui->isLocatedInComputerLabCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionLocatedInComputerLab ) );
	ui->isLocatedInSameComputerLabCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionLocatedInSameComputerLab ) );
	ui->isLocalHostAccessCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionAccessFromLocalHost ) );
	ui->isLocalUserAccessCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionAccessFromLocalUser ) );
	ui->isSameUserAccessCheckBox->setChecked( rule.hasCondition( AccessControlRule::ConditionAccessFromAlreadyConnectedUser ) );

	// load condition arguments
	ui->groupsComboBox->setCurrentText( rule.conditionArgument( AccessControlRule::ConditionMemberOfGroup ).toString() );

	ui->commonGroupComboBox->setCurrentText(
				m_entityNameMap.value( rule.conditionArgument( AccessControlRule::ConditionGroupsInCommon ).
									   value<AccessControlRule::Entity>() ) );

	ui->computerLabsComboBox->setCurrentText( rule.conditionArgument( AccessControlRule::ConditionLocatedInComputerLab ).toString() );

	ui->commonComputerLabComboBox->setCurrentText(
				m_entityNameMap.value( rule.conditionArgument( AccessControlRule::ConditionLocatedInSameComputerLab ).
									   value<AccessControlRule::Entity>() ) );

	// load action
	ui->actionNoneRadioButton->setChecked( rule.action() == AccessControlRule::ActionNone );
	ui->actionAllowRadioButton->setChecked( rule.action() == AccessControlRule::ActionAllow );
	ui->actionDenyRadioButton->setChecked( rule.action() == AccessControlRule::ActionDeny );
	ui->actionAskForPermissionRadioButton->setChecked( rule.action() == AccessControlRule::ActionAskForPermission );
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

	// save condition invertion setting
	m_rule.setConditionsInverted( ui->invertConditionsCheckBox->isChecked() );

	// save conditions
	m_rule.clearConditions();

	if( ui->isMemberOfGroupCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionMemberOfGroup, ui->groupsComboBox->currentText() );
	}

	if( ui->hasCommonGroupsCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionGroupsInCommon,
							 m_entityNameMap.key( ui->commonGroupComboBox->currentText() ) );
	}

	if( ui->isLocatedInComputerLabCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionLocatedInComputerLab, ui->computerLabsComboBox->currentText() );
	}

	if( ui->isLocatedInSameComputerLabCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionLocatedInSameComputerLab,
							 m_entityNameMap.key( ui->commonComputerLabComboBox->currentText() ) );
	}

	if( ui->isLocalHostAccessCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionAccessFromLocalHost, true );
	}

	if( ui->isLocalUserAccessCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionAccessFromLocalUser, true );
	}

	if( ui->isSameUserAccessCheckBox->isChecked() )
	{
		m_rule.setCondition( AccessControlRule::ConditionAccessFromAlreadyConnectedUser, true );
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
	else if( ui->actionAskForPermissionRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::ActionAskForPermission );
	}
	else
	{
		m_rule.setAction( AccessControlRule::ActionNone );
	}

	QDialog::accept();
}
