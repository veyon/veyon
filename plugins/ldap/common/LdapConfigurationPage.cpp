/*
 * LdapConfigurationPage.cpp - implementation of the LdapConfigurationPage class
 *
 * Copyright (c) 2016-2022 Tobias Junghans <tobydox@veyon.io>
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "LdapConfiguration.h"
#include "LdapConfigurationPage.h"
#include "LdapConfigurationTest.h"
#include "Configuration/UiMapping.h"
#include "LdapDirectory.h"
#include "LdapBrowseDialog.h"

#include "ui_LdapConfigurationPage.h"

LdapConfigurationPage::LdapConfigurationPage( LdapConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::LdapConfigurationPage),
	m_configuration( configuration )
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name)	connect( ui->name, &QPushButton::clicked, this, &LdapConfigurationPage::name );

	connect( ui->browseBaseDn, &QPushButton::clicked, this, &LdapConfigurationPage::browseBaseDn );
	connect( ui->browseUserTree, &QPushButton::clicked, this, [this]() { browseObjectTree( ui->userTree ); } );
	connect( ui->browseGroupTree, &QPushButton::clicked, this, [this]() { browseObjectTree( ui->groupTree ); } );
	connect( ui->browseComputerTree, &QPushButton::clicked, this, [this]() { browseObjectTree( ui->computerTree ); } );
	connect( ui->browseComputerGroupTree, &QPushButton::clicked, this, [this]() { browseObjectTree( ui->computerGroupTree ); } );

	connect( ui->browseUserLoginNameAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->userLoginNameAttribute, m_configuration.userTree() ); } );
	connect( ui->browseGroupMemberAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->groupMemberAttribute, m_configuration.groupTree() ); } );
	connect( ui->browseComputerDisplayNameAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->computerDisplayNameAttribute, m_configuration.computerTree() ); } );
	connect( ui->browseComputerHostNameAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->computerHostNameAttribute, m_configuration.computerTree() ); } );
	connect( ui->browseComputerMacAddressAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->computerMacAddressAttribute, m_configuration.computerTree() ); } );
	connect( ui->browseComputerLocationAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->computerLocationAttribute, m_configuration.computerTree() ); } );
	connect( ui->browseLocationNameAttribute, &QPushButton::clicked, this, [this]() { browseAttribute( ui->locationNameAttribute, m_configuration.computerTree() ); } );

	CONNECT_BUTTON_SLOT( testBind )
	CONNECT_BUTTON_SLOT( testBaseDn )
	CONNECT_BUTTON_SLOT( testNamingContext )
	CONNECT_BUTTON_SLOT( testUserTree )
	CONNECT_BUTTON_SLOT( testGroupTree )
	CONNECT_BUTTON_SLOT( testComputerTree )
	CONNECT_BUTTON_SLOT( testComputerGroupTree )

	CONNECT_BUTTON_SLOT( testUserLoginNameAttribute )
	CONNECT_BUTTON_SLOT( testGroupMemberAttribute )
	CONNECT_BUTTON_SLOT( testComputerDisplayNameAttribute )
	CONNECT_BUTTON_SLOT( testComputerHostNameAttribute )
	CONNECT_BUTTON_SLOT( testComputerMacAddressAttribute )
	CONNECT_BUTTON_SLOT( testComputerLocationAttribute )
	CONNECT_BUTTON_SLOT( testLocationNameAttribute )

	CONNECT_BUTTON_SLOT( testUsersFilter )
	CONNECT_BUTTON_SLOT( testUserGroupsFilter )
	CONNECT_BUTTON_SLOT( testComputersFilter )
	CONNECT_BUTTON_SLOT( testComputerGroupsFilter )
	CONNECT_BUTTON_SLOT( testComputerContainersFilter )

	CONNECT_BUTTON_SLOT( testGroupsOfUser )
	CONNECT_BUTTON_SLOT( testGroupsOfComputer )
	CONNECT_BUTTON_SLOT( testComputerObjectByIpAddress )
	CONNECT_BUTTON_SLOT( testLocationEntries )
	CONNECT_BUTTON_SLOT( testLocations )

	CONNECT_BUTTON_SLOT( browseCACertificateFile )

	connect( ui->tlsVerifyMode, QOverload<int>::of( &QComboBox::currentIndexChanged ), ui->tlsCACertificateFile, [=]() {
		ui->tlsCACertificateFile->setEnabled( ui->tlsVerifyMode->currentIndex() == LdapClient::TLSVerifyCustomCert );
	} );

	const auto browseButtons = findChildren<QPushButton *>( QRegularExpression( QStringLiteral("browse.*") ) );
	for( auto button : browseButtons )
	{
		button->setToolTip( tr( "Browse" ) );
	}

	const auto testButtons = findChildren<QPushButton *>( QRegularExpression( QStringLiteral("test.*") ) );
	for( auto button : testButtons )
	{
		button->setToolTip( tr( "Test" ) );
	}
}



LdapConfigurationPage::~LdapConfigurationPage()
{
	delete ui;
}



void LdapConfigurationPage::resetWidgets()
{
	FOREACH_LDAP_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)
}



void LdapConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_LDAP_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void LdapConfigurationPage::applyConfiguration()
{
}



void LdapConfigurationPage::browseBaseDn()
{
	const auto baseDn = LdapBrowseDialog( m_configuration, this ).browseBaseDn( m_configuration.baseDn() );

	if( baseDn.isEmpty() == false )
	{
		ui->baseDn->setText( baseDn );
	}
}



void LdapConfigurationPage::browseObjectTree( QLineEdit* lineEdit )
{
	auto dn = LdapClient::addBaseDn( lineEdit->text(), m_configuration.baseDn() );

	dn = LdapBrowseDialog( m_configuration, this ).browseDn( dn );

	if( dn.isEmpty() == false )
	{
		dn = LdapClient::stripBaseDn( dn, m_configuration.baseDn() );

		lineEdit->setText( dn );
	}
}



void LdapConfigurationPage::browseAttribute( QLineEdit* lineEdit, const QString& tree )
{
	const auto treeDn = LdapClient::addBaseDn( tree, m_configuration.baseDn() );

	const auto attribute = LdapBrowseDialog( m_configuration, this ).browseAttribute( treeDn );

	if( attribute.isEmpty() == false )
	{
		lineEdit->setText( attribute );
	}
}



void LdapConfigurationPage::testBind()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testBind() );
}



void LdapConfigurationPage::testBaseDn()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testBaseDn() );
}



void LdapConfigurationPage::testNamingContext()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testNamingContext() );
}



void LdapConfigurationPage::testUserTree()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testUserTree() );
}



void LdapConfigurationPage::testGroupTree()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testGroupTree() );
}



void LdapConfigurationPage::testComputerTree()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testComputerTree() );
}



void LdapConfigurationPage::testComputerGroupTree()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testComputerGroupTree() );
}



void LdapConfigurationPage::testUserLoginNameAttribute()
{
	const auto userFilter = QInputDialog::getText( this, tr( "Enter username" ),
												   tr( "Please enter a user login name (wildcards allowed) which to query:") );
	if( userFilter.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testUserLoginNameAttribute( userFilter ) );
	}
}



void LdapConfigurationPage::testGroupMemberAttribute()
{
	const auto groupFilter = QInputDialog::getText( this, tr( "Enter group name" ),
													tr( "Please enter a group name whose members to query:") );
	if( groupFilter.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testGroupMemberAttribute( groupFilter ) );
	}
}



void LdapConfigurationPage::testComputerDisplayNameAttribute()
{
	const auto computerName = QInputDialog::getText( this, tr( "Enter computer display name" ),
													 tr( "Please enter a computer display name to query:") );
	if( computerName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testComputerDisplayNameAttribute( computerName ) );
	}

}



void LdapConfigurationPage::testComputerHostNameAttribute()
{
	const auto computerName = QInputDialog::getText( this, tr( "Enter computer name" ),
													 tr( "Please enter a computer hostname to query:") );
	if( computerName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testComputerHostNameAttribute( computerName ) );
	}
}



void LdapConfigurationPage::testComputerMacAddressAttribute()
{
	const auto computerDn = QInputDialog::getText( this, tr( "Enter computer DN" ),
												   tr( "Please enter the DN of a computer whose MAC address to query:") );
	if( computerDn.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testComputerMacAddressAttribute( computerDn ) );
	}
}



void LdapConfigurationPage::testComputerLocationAttribute()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter computer location name" ),
													 tr( "Please enter the name of a computer location (wildcards allowed):") );
	if( locationName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testComputerLocationAttribute( locationName ) );
	}
}



void LdapConfigurationPage::testLocationNameAttribute()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter location name" ),
													 tr( "Please enter the name of a computer location (wildcards allowed):") );
	if( locationName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testLocationNameAttribute( locationName ) );
	}
}



void LdapConfigurationPage::testUsersFilter()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testUsersFilter() );
}



void LdapConfigurationPage::testUserGroupsFilter()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testUserGroupsFilter() );
}



void LdapConfigurationPage::testComputersFilter()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testComputersFilter() );
}



void LdapConfigurationPage::testComputerGroupsFilter()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testComputerGroupsFilter() );
}



void LdapConfigurationPage::testComputerContainersFilter()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testComputerContainersFilter() );
}



void LdapConfigurationPage::testGroupsOfUser()
{
	const auto username = QInputDialog::getText( this, tr( "Enter username" ),
												 tr( "Please enter a user login name whose group memberships to query:") );
	if( username.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testGroupsOfUser( username ) );
	}
}



void LdapConfigurationPage::testGroupsOfComputer()
{
	const auto computerHostName = QInputDialog::getText( this, tr( "Enter hostname" ),
													  tr( "Please enter a computer hostname whose group memberships to query:") );
	if( computerHostName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testGroupsOfComputer( computerHostName ) );
	}
}



void LdapConfigurationPage::testComputerObjectByIpAddress()
{
	const auto computerIpAddress = QInputDialog::getText( this, tr( "Enter computer IP address" ),
													   tr( "Please enter a computer IP address which to resolve to an computer object:") );
	if( computerIpAddress.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testComputerObjectByIpAddress( computerIpAddress ) );
	}
}



void LdapConfigurationPage::testLocationEntries()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter location name" ),
													 tr( "Please enter the name of a location whose entries to query:") );
	if( locationName.isEmpty() == false )
	{
		showTestResult( LdapConfigurationTest( m_configuration ).testLocationEntries( locationName ) );
	}
}



void LdapConfigurationPage::testLocations()
{
	showTestResult( LdapConfigurationTest( m_configuration ).testLocations() );
}



void LdapConfigurationPage::browseCACertificateFile()
{
	auto caCertFile = QFileDialog::getOpenFileName( this, tr( "Custom CA certificate file" ), {},
													tr( "Certificate files (*.pem)" ) );
	if( caCertFile.isEmpty() == false )
	{
		ui->tlsCACertificateFile->setText( caCertFile );
	}
}



void LdapConfigurationPage::showTestResult( const LdapConfigurationTest::Result& result )
{
	if( result )
	{
		QMessageBox::information( this, result.title, result.message );
	}
	else
	{

		QMessageBox::critical( this, result.title, result.message );
	}
}
