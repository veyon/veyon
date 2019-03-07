/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
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

#include "AccessControlRuleEditDialog.h"
#include "AccessControlProvider.h"

#include "ui_AccessControlRuleEditDialog.h"

AccessControlRuleEditDialog::AccessControlRuleEditDialog(AccessControlRule &rule, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRuleEditDialog),
	m_rule( rule ),
	m_subjectNameMap( {
{ AccessControlRule::SubjectAccessingUser, tr( "Accessing user" ) },
{ AccessControlRule::SubjectAccessingComputer, tr( "Accessing computer" ) },
{ AccessControlRule::SubjectLocalUser, tr( "Local (logged on) user" ) },
{ AccessControlRule::SubjectLocalComputer, tr( "Local computer" ) },
					  } )
{
	ui->setupUi(this);

	AccessControlProvider accessControlProvider;

	// populate user subject combobox
	ui->isMemberOfGroupSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectAccessingUser], AccessControlRule::SubjectAccessingUser );
	ui->isMemberOfGroupSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectLocalUser], AccessControlRule::SubjectLocalUser );

	// populate computer subject comboboxes
	ui->isAtLocationSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectAccessingComputer], AccessControlRule::SubjectAccessingComputer );
	ui->isAtLocationSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectLocalComputer], AccessControlRule::SubjectLocalComputer );

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
	ui->isMemberOfGroupCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionMemberOfUserGroup ) );
	ui->hasCommonGroupsCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionGroupsInCommon ) );
	ui->isAtLocationCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionLocatedAt ) );
	ui->hasCommonLocationsCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionSameLocation ) );
	ui->isLocalHostAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalHost ) );
	ui->isLocalUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalUser ) );
	ui->isSameUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromAlreadyConnectedUser ) );
	ui->noUserLoggedOnCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionNoUserLoggedOn ) );

	// load selected condition subjects
	ui->isMemberOfGroupSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::ConditionMemberOfUserGroup ) ) );
	ui->isAtLocationSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::ConditionLocatedAt ) ) );

	// load condition arguments
	ui->groupsComboBox->setCurrentText( rule.argument( AccessControlRule::ConditionMemberOfUserGroup ) );
	ui->locationsComboBox->setCurrentText( rule.argument( AccessControlRule::ConditionLocatedAt ) );

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

	// save general condition processing settings
	m_rule.setConditionsIgnored( ui->ignoreConditionsCheckBox->isChecked() );
	m_rule.setConditionsInverted( ui->invertConditionsCheckBox->isChecked() );

	// save conditions
	m_rule.clearParameters();

	// member of user group
	m_rule.setConditionEnabled( AccessControlRule::ConditionMemberOfUserGroup,
								ui->isMemberOfGroupCheckBox->isChecked() );
	m_rule.setSubject( AccessControlRule::ConditionMemberOfUserGroup,
					   m_subjectNameMap.key( ui->isMemberOfGroupSubjectComboBox->currentText() ) );
	m_rule.setArgument( AccessControlRule::ConditionMemberOfUserGroup,
						ui->groupsComboBox->currentText() );

	// common groups
	m_rule.setConditionEnabled( AccessControlRule::ConditionGroupsInCommon,
								ui->hasCommonGroupsCheckBox->isChecked() );

	// located at
	m_rule.setConditionEnabled( AccessControlRule::ConditionLocatedAt,
								ui->isAtLocationCheckBox->isChecked() );
	m_rule.setSubject( AccessControlRule::ConditionLocatedAt,
					   m_subjectNameMap.key( ui->isAtLocationSubjectComboBox->currentText() ) );
	m_rule.setArgument( AccessControlRule::ConditionLocatedAt,
						ui->locationsComboBox->currentText() );

	// same location
	m_rule.setConditionEnabled( AccessControlRule::ConditionSameLocation,
								ui->hasCommonLocationsCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::ConditionAccessFromLocalHost,
								ui->isLocalHostAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::ConditionAccessFromLocalUser,
								ui->isLocalUserAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::ConditionAccessFromAlreadyConnectedUser,
								ui->isSameUserAccessCheckBox->isChecked() );

	m_rule.setConditionEnabled( AccessControlRule::ConditionNoUserLoggedOn,
								ui->noUserLoggedOnCheckBox->isChecked() );

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
