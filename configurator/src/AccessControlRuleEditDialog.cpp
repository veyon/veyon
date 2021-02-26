/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "AccessControlRuleEditDialog.h"
#include "AccessControlProvider.h"

#include "ui_AccessControlRuleEditDialog.h"

AccessControlRuleEditDialog::AccessControlRuleEditDialog(AccessControlRule &rule, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRuleEditDialog),
	m_rule( rule ),
	m_subjectNameMap( {
{ AccessControlRule::Subject::AccessingUser, tr( "Accessing user" ) },
{ AccessControlRule::Subject::AccessingComputer, tr( "Accessing computer" ) },
{ AccessControlRule::Subject::LocalUser, tr( "Local (logged on) user" ) },
{ AccessControlRule::Subject::LocalComputer, tr( "Local computer" ) },
					  } )
{
	ui->setupUi(this);

	AccessControlProvider accessControlProvider;

	// populate user subject combobox
	ui->isMemberOfGroupSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::Subject::AccessingUser],
			static_cast<int>( AccessControlRule::Subject::AccessingUser ) );
	ui->isMemberOfGroupSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::Subject::LocalUser],
			static_cast<int>( AccessControlRule::Subject::LocalUser ) );

	// populate computer subject comboboxes
	ui->isAtLocationSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::Subject::AccessingComputer],
			static_cast<int>( AccessControlRule::Subject::AccessingComputer ) );
	ui->isAtLocationSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::Subject::LocalComputer],
			static_cast<int>( AccessControlRule::Subject::LocalComputer ) );

	// populate groups and locations comboboxes
	ui->groupsComboBox->addItems( accessControlProvider.userGroups() );
	ui->locationsComboBox->addItems( accessControlProvider.locations() );

	// load general settings
	ui->ruleNameLineEdit->setText( rule.name() );
	ui->ruleDescriptionLineEdit->setText( rule.description() );

	// load general condition processing settings
	ui->ignoreConditionsCheckBox->setChecked( rule.areConditionsIgnored() );
	ui->invertConditionsCheckBox->setChecked( rule.areConditionsInverted() );

	// load condition states
	ui->isMemberOfGroupCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::MemberOfUserGroup ) );
	ui->hasCommonGroupsCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::GroupsInCommon ) );
	ui->isAtLocationCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::LocatedAt ) );
	ui->hasCommonLocationsCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::SameLocation ) );
	ui->isLocalHostAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromLocalHost ) );
	ui->isLocalUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromLocalUser ) );
	ui->isSameUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromAlreadyConnectedUser ) );
	ui->noUserLoggedOnCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::Condition::NoUserLoggedOn ) );

	// load selected condition subjects
	ui->isMemberOfGroupSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::Condition::MemberOfUserGroup ) ) );
	ui->isAtLocationSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::Condition::LocatedAt ) ) );

	// load condition arguments
	ui->groupsComboBox->setCurrentText( rule.argument( AccessControlRule::Condition::MemberOfUserGroup ) );
	ui->locationsComboBox->setCurrentText( rule.argument( AccessControlRule::Condition::LocatedAt ) );

	// load action
	ui->actionNoneRadioButton->setChecked( rule.action() == AccessControlRule::Action::None );
	ui->actionAllowRadioButton->setChecked( rule.action() == AccessControlRule::Action::Allow );
	ui->actionDenyRadioButton->setChecked( rule.action() == AccessControlRule::Action::Deny );
	ui->actionAskForPermissionRadioButton->setChecked( rule.action() == AccessControlRule::Action::AskForPermission );
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

	// save general condition processing settings
	m_rule.setConditionsIgnored( ui->ignoreConditionsCheckBox->isChecked() );
	m_rule.setConditionsInverted( ui->invertConditionsCheckBox->isChecked() );

	// save conditions
	m_rule.clearParameters();

	// member of user group
	m_rule.setConditionEnabled( AccessControlRule::Condition::MemberOfUserGroup,
								ui->isMemberOfGroupCheckBox->isChecked() );
	m_rule.setSubject( AccessControlRule::Condition::MemberOfUserGroup,
					   m_subjectNameMap.key( ui->isMemberOfGroupSubjectComboBox->currentText() ) );
	m_rule.setArgument( AccessControlRule::Condition::MemberOfUserGroup,
						ui->groupsComboBox->currentText() );

	// common groups
	m_rule.setConditionEnabled( AccessControlRule::Condition::GroupsInCommon,
								ui->hasCommonGroupsCheckBox->isChecked() );

	// located at
	m_rule.setConditionEnabled( AccessControlRule::Condition::LocatedAt,
								ui->isAtLocationCheckBox->isChecked() );
	m_rule.setSubject( AccessControlRule::Condition::LocatedAt,
					   m_subjectNameMap.key( ui->isAtLocationSubjectComboBox->currentText() ) );
	m_rule.setArgument( AccessControlRule::Condition::LocatedAt,
						ui->locationsComboBox->currentText() );

	// same location
	m_rule.setConditionEnabled( AccessControlRule::Condition::SameLocation,
								ui->hasCommonLocationsCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::Condition::AccessFromLocalHost,
								ui->isLocalHostAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::Condition::AccessFromLocalUser,
								ui->isLocalUserAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::Condition::AccessFromAlreadyConnectedUser,
								ui->isSameUserAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::Condition::NoUserLoggedOn,
								ui->noUserLoggedOnCheckBox->isChecked() );

	// save action
	if( ui->actionAllowRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::Action::Allow );
	}
	else if( ui->actionDenyRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::Action::Deny );
	}
	else if( ui->actionAskForPermissionRadioButton->isChecked() )
	{
		m_rule.setAction( AccessControlRule::Action::AskForPermission );
	}
	else
	{
		m_rule.setAction( AccessControlRule::Action::None );
	}

	QDialog::accept();
}
