/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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
	ui->isLocatedInRoomSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectAccessingComputer], AccessControlRule::SubjectAccessingComputer );
	ui->isLocatedInRoomSubjectComboBox->addItem( m_subjectNameMap[AccessControlRule::SubjectLocalComputer], AccessControlRule::SubjectLocalComputer );

	// populate groups and rooms comboboxes
	ui->groupsComboBox->addItems( accessControlProvider.userGroups() );
	ui->roomsComboBox->addItems( accessControlProvider.rooms() );

	// load general settings
	ui->ruleNameLineEdit->setText( rule.name() );
	ui->ruleDescriptionLineEdit->setText( rule.description() );

	// load general condition processing settings
	ui->ignoreConditionsCheckBox->setChecked( rule.areConditionsIgnored() );
	ui->invertConditionsCheckBox->setChecked( rule.areConditionsInverted() );

	// load condition states
	ui->isMemberOfGroupCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionMemberOfUserGroup ) );
	ui->hasCommonGroupsCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionGroupsInCommon ) );
	ui->isLocatedInRoomCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionLocatedInRoom ) );
	ui->isLocatedInSameRoomCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionLocatedInSameRoom ) );
	ui->isLocalHostAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalHost ) );
	ui->isLocalUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalUser ) );
	ui->isSameUserAccessCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromAlreadyConnectedUser ) );
	ui->noUserLoggedOnCheckBox->setChecked( rule.isConditionEnabled( AccessControlRule::ConditionNoUserLoggedOn ) );

	// load selected condition subjects
	ui->isMemberOfGroupSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::ConditionMemberOfUserGroup ) ) );
	ui->isLocatedInRoomSubjectComboBox->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::ConditionLocatedInRoom ) ) );

	// load condition arguments
	ui->groupsComboBox->setCurrentText( rule.argument( AccessControlRule::ConditionMemberOfUserGroup ) );
	ui->roomsComboBox->setCurrentText( rule.argument( AccessControlRule::ConditionLocatedInRoom ) );

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

	// located in room
	m_rule.setConditionEnabled( AccessControlRule::ConditionLocatedInRoom,
								ui->isLocatedInRoomCheckBox->isChecked() );
	m_rule.setSubject( AccessControlRule::ConditionLocatedInRoom,
					   m_subjectNameMap.key( ui->isLocatedInRoomSubjectComboBox->currentText() ) );
	m_rule.setArgument( AccessControlRule::ConditionLocatedInRoom,
						ui->roomsComboBox->currentText() );

	// located in same room as
	m_rule.setConditionEnabled( AccessControlRule::ConditionLocatedInSameRoom,
								ui->isLocatedInSameRoomCheckBox->isChecked() );

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
