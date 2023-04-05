/*
 * AccessControlRuleEditDialog.cpp - dialog for editing an AccessControlRule
 *
 * Copyright (c) 2016-2023 Tobias Junghans <tobydox@veyon.io>
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
#include "AuthenticationManager.h"

#include "ui_AccessControlRuleEditDialog.h"

AccessControlRuleEditDialog::AccessControlRuleEditDialog(AccessControlRule &rule, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccessControlRuleEditDialog),
	m_rule( rule ),
	m_subjectNameMap( {
{ AccessControlRule::Subject::AccessingUser, tr( "Accessing user" ) },
{ AccessControlRule::Subject::AccessingComputer, tr( "Accessing computer" ) },
{ AccessControlRule::Subject::LocalUser, tr( "User being accessed" ) },
{ AccessControlRule::Subject::LocalComputer, tr( "Computer being accessed" ) },
					  } )
{
	ui->setupUi(this);

	AccessControlProvider accessControlProvider;

	const auto authenticationMethods = VeyonCore::authenticationManager().availableMethods();
	for( auto it = authenticationMethods.constBegin(), end = authenticationMethods.constEnd(); it != end; ++it )
	{
		if( it.value().isEmpty() == false )
		{
			ui->authenticationMethods->addItem( it.value(), it.key() );
		}
	}

	// populate user subject combobox
	ui->isMemberOfGroupSubject->addItem( m_subjectNameMap[AccessControlRule::Subject::AccessingUser],
			static_cast<int>( AccessControlRule::Subject::AccessingUser ) );
	ui->isMemberOfGroupSubject->addItem( m_subjectNameMap[AccessControlRule::Subject::LocalUser],
			static_cast<int>( AccessControlRule::Subject::LocalUser ) );

	// populate computer subject comboboxes
	ui->isAtLocationSubject->addItem( m_subjectNameMap[AccessControlRule::Subject::AccessingComputer],
			static_cast<int>( AccessControlRule::Subject::AccessingComputer ) );
	ui->isAtLocationSubject->addItem( m_subjectNameMap[AccessControlRule::Subject::LocalComputer],
			static_cast<int>( AccessControlRule::Subject::LocalComputer ) );

	// populate groups and locations comboboxes
	ui->groups->addItems( accessControlProvider.userGroups() );
	ui->locations->addItems( accessControlProvider.locations() );

	// load general settings
	ui->ruleNameLineEdit->setText( rule.name() );
	ui->ruleDescriptionLineEdit->setText( rule.description() );

	// load general condition processing settings
	ui->ignoreConditionsCheckBox->setChecked( rule.areConditionsIgnored() );

	// load condition states
	const auto loadCondition = [&rule]( QCheckBox* checkBox,
												QComboBox* comboBox,
												AccessControlRule::Condition condition )
	{
		checkBox->setChecked( rule.isConditionEnabled( condition ) );
		comboBox->setCurrentIndex( rule.isConditionInverted( condition ) ? 1 : 0 );
	};

	// load condition settings
	loadCondition ( ui->isAuthenticatedViaMethod, ui->invertIsAuthenticatedViaMethod, AccessControlRule::Condition::AuthenticationMethod );
	loadCondition ( ui->isMemberOfGroup, ui->invertIsMemberOfGroup, AccessControlRule::Condition::MemberOfGroup );
	loadCondition ( ui->hasCommonGroups, ui->invertHasCommonGroups, AccessControlRule::Condition::GroupsInCommon );
	loadCondition ( ui->isAtLocation, ui->invertIsAtLocation, AccessControlRule::Condition::LocatedAt );
	loadCondition ( ui->hasCommonLocations, ui->invertHasCommonLocations, AccessControlRule::Condition::LocationsInCommon );
	loadCondition ( ui->isLocalHostAccess, ui->invertIsLocalHostAccess, AccessControlRule::Condition::AccessFromLocalHost );
	loadCondition ( ui->isSameUserAccess, ui->invertIsSameUserAccess, AccessControlRule::Condition::AccessFromSameUser );
	loadCondition ( ui->isUserConnected, ui->invertIsUserConnected, AccessControlRule::Condition::AccessFromAlreadyConnectedUser );
	loadCondition ( ui->isAccessedUserLoggedInLocally, ui->invertIsAccessedUserLoggedInLocally, AccessControlRule::Condition::AccessedUserLoggedInLocally );
	loadCondition ( ui->isNoUserLoggedInLocally, ui->invertIsNoUserLoggedInLocally, AccessControlRule::Condition::NoUserLoggedInLocally );
	loadCondition ( ui->isNoUserLoggedInRemotely, ui->invertIsNoUserLoggedInRemotely, AccessControlRule::Condition::NoUserLoggedInRemotely );
	loadCondition ( ui->isUserSession, ui->invertIsUserSession, AccessControlRule::Condition::UserSession );

	// load selected condition subjects
	ui->isMemberOfGroupSubject->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::Condition::MemberOfGroup ) ) );
	ui->isAtLocationSubject->setCurrentText( m_subjectNameMap.value( rule.subject( AccessControlRule::Condition::LocatedAt ) ) );

	// load condition arguments
	ui->authenticationMethods->setCurrentText( authenticationMethods.value(
		Plugin::Uid{ rule.argument( AccessControlRule::Condition::AuthenticationMethod ) } ) );
	ui->groups->setCurrentText( rule.argument( AccessControlRule::Condition::MemberOfGroup ) );
	ui->locations->setCurrentText( rule.argument( AccessControlRule::Condition::LocatedAt ) );

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

	// save conditions
	m_rule.clearParameters();

	const auto saveCondition = [this]( QCheckBox* checkBox,
										QComboBox* comboBox,
										AccessControlRule::Condition condition )
	{
		m_rule.setConditionEnabled( condition, checkBox->isChecked() );
		m_rule.setConditionInverted( condition, comboBox->currentIndex() > 0 );
	};

	// authentication method
	saveCondition( ui->isAuthenticatedViaMethod, ui->invertIsAuthenticatedViaMethod, AccessControlRule::Condition::AuthenticationMethod );
	m_rule.setArgument( AccessControlRule::Condition::AuthenticationMethod,
						VeyonCore::formattedUuid( ui->authenticationMethods->currentData().toUuid() ) );

	// member of user group
	saveCondition( ui->isMemberOfGroup, ui->invertIsMemberOfGroup, AccessControlRule::Condition::MemberOfGroup );
	m_rule.setSubject( AccessControlRule::Condition::MemberOfGroup,
					   m_subjectNameMap.key( ui->isMemberOfGroupSubject->currentText() ) );
	m_rule.setArgument( AccessControlRule::Condition::MemberOfGroup, ui->groups->currentText() );

	// common groups
	saveCondition( ui->hasCommonGroups, ui->invertHasCommonGroups, AccessControlRule::Condition::GroupsInCommon );

	// located at
	saveCondition( ui->isAtLocation, ui->invertIsAtLocation, AccessControlRule::Condition::LocatedAt );
	m_rule.setSubject( AccessControlRule::Condition::LocatedAt,
					   m_subjectNameMap.key( ui->isAtLocationSubject->currentText() ) );
	m_rule.setArgument( AccessControlRule::Condition::LocatedAt, ui->locations->currentText() );

	// same location
	saveCondition( ui->hasCommonLocations, ui->invertHasCommonLocations, AccessControlRule::Condition::LocationsInCommon );

	saveCondition( ui->isLocalHostAccess, ui->invertIsLocalHostAccess, AccessControlRule::Condition::AccessFromLocalHost );
	saveCondition( ui->isSameUserAccess, ui->invertIsSameUserAccess, AccessControlRule::Condition::AccessFromSameUser );
	saveCondition( ui->isUserConnected, ui->invertIsUserConnected, AccessControlRule::Condition::AccessFromAlreadyConnectedUser );
	saveCondition( ui->isAccessedUserLoggedInLocally, ui->invertIsAccessedUserLoggedInLocally, AccessControlRule::Condition::AccessedUserLoggedInLocally );

	saveCondition( ui->isNoUserLoggedInLocally, ui->invertIsNoUserLoggedInLocally, AccessControlRule::Condition::NoUserLoggedInLocally );
	saveCondition( ui->isNoUserLoggedInRemotely, ui->invertIsNoUserLoggedInRemotely, AccessControlRule::Condition::NoUserLoggedInRemotely );
	saveCondition( ui->isUserSession, ui->invertIsUserSession, AccessControlRule::Condition::UserSession );

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
