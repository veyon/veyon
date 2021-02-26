/*
 * LdapConfigurationPage.cpp - implementation of the LdapConfigurationPage class
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "LdapConfiguration.h"
#include "LdapConfigurationPage.h"
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

	CONNECT_BUTTON_SLOT( testBindInteractively )
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



void LdapConfigurationPage::testBindInteractively()
{
	testBind( false );
}



void LdapConfigurationPage::testBaseDn()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing base DN";

		LdapClient ldapClient( m_configuration );
		QStringList entries = ldapClient.queryBaseDn();

		if( entries.isEmpty() )
		{
			QMessageBox::critical( this, tr( "LDAP base DN test failed"),
								   tr( "Could not query the configured base DN. "
									   "Please check the base DN parameter.\n\n"
									   "%1" ).arg( ldapClient.errorDescription() ) );
		}
		else
		{
			QMessageBox::information( this, tr( "LDAP base DN test successful" ),
									  tr( "The LDAP base DN has been queried successfully. "
										  "The following entries were found:\n\n%1" ).
									  arg( entries.join(QLatin1Char('\n')) ) );
		}
	}
}



void LdapConfigurationPage::testNamingContext()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing naming context";

		LdapClient ldapClient( m_configuration );
		const auto baseDn = ldapClient.queryNamingContexts().value( 0 );

		if( baseDn.isEmpty() )
		{
			QMessageBox::critical( this, tr( "LDAP naming context test failed"),
								   tr( "Could not query the base DN via naming contexts. "
									   "Please check the naming context attribute parameter.\n\n"
									   "%1" ).arg( ldapClient.errorDescription() ) );
		}
		else
		{
			QMessageBox::information( this, tr( "LDAP naming context test successful" ),
									  tr( "The LDAP naming context has been queried successfully. "
										  "The following base DN was found:\n%1" ).
									  arg( baseDn ) );
		}
	}
}



void LdapConfigurationPage::testUserTree()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing user tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.users().count();

		reportLdapTreeQueryResult( tr( "user tree" ), count, ui->userTreeLabel->text(),
								   ldapDirectory.client().errorDescription() );
	}
}



void LdapConfigurationPage::testGroupTree()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.groups().count();

		reportLdapTreeQueryResult( tr( "group tree" ), count, ui->groupTreeLabel->text(),
								   ldapDirectory.client().errorDescription() );
	}
}



void LdapConfigurationPage::testComputerTree()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing computer tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computersByHostName().count();

		reportLdapTreeQueryResult( tr( "computer tree" ), count, ui->computerTreeLabel->text(),
								   ldapDirectory.client().errorDescription() );
	}
}



void LdapConfigurationPage::testComputerGroupTree()
{
	if( testBindQuietly() )
	{
		vDebug() << "[TEST][LDAP] Testing computer group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computerGroups().count();

		reportLdapTreeQueryResult( tr( "computer group tree" ), count, ui->computerGroupTreeLabel->text(),
								   ldapDirectory.client().errorDescription() );
	}
}



void LdapConfigurationPage::testUserLoginNameAttribute()
{
	QString userFilter = QInputDialog::getText( this, tr( "Enter username" ),
												tr( "Please enter a user login name (wildcards allowed) which to query:") );
	if( userFilter.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing user login attribute for" << userFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		reportLdapObjectQueryResults( tr( "user objects" ), { ui->userLoginNameAttributeLabel->text() },
									  ldapDirectory.users( userFilter ), ldapDirectory );
	}
}



void LdapConfigurationPage::testGroupMemberAttribute()
{
	QString groupFilter = QInputDialog::getText( this, tr( "Enter group name" ),
												 tr( "Please enter a group name whose members to query:") );
	if( groupFilter.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing group member attribute for" << groupFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		QStringList groups = ldapDirectory.groups( groupFilter );

		if( groups.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "group members" ), { ui->groupMemberAttributeLabel->text() },
										  ldapDirectory.groupMembers( groups.first() ), ldapDirectory );
		}
		else
		{
			QMessageBox::warning( this, tr( "Group not found"),
								  tr( "Could not find a group with the name \"%1\". "
									  "Please check the group name or the group "
									  "tree parameter.").arg( groupFilter ) );
		}
	}
}



void LdapConfigurationPage::testComputerDisplayNameAttribute()
{
	auto computerName = QInputDialog::getText( this, tr( "Enter computer display name" ),
											   tr( "Please enter a computer display name to query:") );
	if( computerName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer display name attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		reportLdapObjectQueryResults( tr( "computer objects" ), { ui->computerDisplayNameAttributeLabel->text() },
									  ldapDirectory.computersByDisplayName( computerName ), ldapDirectory );
	}

}



void LdapConfigurationPage::testComputerHostNameAttribute()
{
	QString computerName = QInputDialog::getText( this, tr( "Enter computer name" ),
												  tr( "Please enter a computer hostname to query:") );
	if( computerName.isEmpty() == false )
	{
		if( m_configuration.computerHostNameAsFQDN() &&
			computerName.contains( QLatin1Char('.') ) == false )
		{
			QMessageBox::critical( this, tr( "Invalid hostname" ),
								   tr( "You configured computer hostnames to be stored "
									   "as fully qualified domain names (FQDN) but entered "
									   "a hostname without domain." ) );
			return;
		}
		else if( m_configuration.computerHostNameAsFQDN() == false &&
				 computerName.contains( QLatin1Char('.') ) )
		{
			QMessageBox::critical( this, tr( "Invalid hostname" ),
								   tr( "You configured computer hostnames to be stored "
									   "as simple hostnames without a domain name but "
									   "entered a hostname with a domain name part." ) );
			return;
		}

		vDebug() << "[TEST][LDAP] Testing computer hostname attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		reportLdapObjectQueryResults( tr( "computer objects" ), { ui->computerHostNameAttributeLabel->text() },
									  ldapDirectory.computersByHostName( computerName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerMacAddressAttribute()
{
	QString computerDn = QInputDialog::getText( this, tr( "Enter computer DN" ),
												tr( "Please enter the DN of a computer whose MAC address to query:") );
	if( computerDn.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer MAC address attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		QString macAddress = ldapDirectory.computerMacAddress( computerDn );

		reportLdapObjectQueryResults( tr( "computer MAC addresses" ), { ui->computerMacAddressAttributeLabel->text() },
									  macAddress.isEmpty() ? QStringList() : QStringList( macAddress ),
									  ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerLocationAttribute()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter computer location name" ),
													 tr( "Please enter the name of a computer location (wildcards allowed):") );
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer location attribute for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );

		reportLdapObjectQueryResults( tr( "computer locations" ), { ui->computerLocationAttributeLabel->text() },
									  ldapDirectory.computerLocations( locationName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testLocationNameAttribute()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter location name" ),
													 tr( "Please enter the name of a computer location (wildcards allowed):") );
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing location name attribute for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );

		reportLdapObjectQueryResults( tr( "computer locations" ), { ui->locationNameAttributeLabel->text() },
									  ldapDirectory.computerLocations( locationName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testUsersFilter()
{
	vDebug() << "[TEST][LDAP] Testing users filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.users().count();

	reportLdapFilterTestResult( tr( "users" ), count, ldapDirectory.client().errorDescription() );
}



void LdapConfigurationPage::testUserGroupsFilter()
{
	vDebug() << "[TEST][LDAP] Testing user groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.userGroups().count();

	reportLdapFilterTestResult( tr( "user groups" ), count, ldapDirectory.client().errorDescription() );
}



void LdapConfigurationPage::testComputersFilter()
{
	vDebug() << "[TEST][LDAP] Testing computers filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computersByHostName().count();

	reportLdapFilterTestResult( tr( "computers" ), count, ldapDirectory.client().errorDescription() );
}



void LdapConfigurationPage::testComputerGroupsFilter()
{
	vDebug() << "[TEST][LDAP] Testing computer groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.computerGroups().count();

	reportLdapFilterTestResult( tr( "computer groups" ), count, ldapDirectory.client().errorDescription() );
}



void LdapConfigurationPage::testComputerContainersFilter()
{
	vDebug() << "[TEST][LDAP] Testing computer containers filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computerLocations().count();

	reportLdapFilterTestResult( tr( "computer containers" ), count, ldapDirectory.client().errorDescription() );
}



void LdapConfigurationPage::testGroupsOfUser()
{
	QString username = QInputDialog::getText( this, tr( "Enter username" ),
											  tr( "Please enter a user login name whose group memberships to query:") );
	if( username.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing groups of user" << username;

		LdapDirectory ldapDirectory( m_configuration );

		QStringList userObjects = ldapDirectory.users(username);

		if( userObjects.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "groups of user" ), { ui->userLoginNameAttributeLabel->text(),
																	ui->groupMemberAttributeLabel->text() },
										  ldapDirectory.groupsOfUser( userObjects.first() ), ldapDirectory );
		}
		else
		{
			QMessageBox::warning( this, tr( "User not found" ),
								  tr( "Could not find a user with the name \"%1\". Please check the username "
									  "or the user tree parameter.").arg( username ) );
		}
	}
}



void LdapConfigurationPage::testGroupsOfComputer()
{
	QString computerHostName = QInputDialog::getText( this, tr( "Enter hostname" ),
													  tr( "Please enter a computer hostname whose group memberships to query:") );
	if( computerHostName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing groups of computer for" << computerHostName;

		LdapDirectory ldapDirectory( m_configuration );

		QStringList computerObjects = ldapDirectory.computersByHostName(computerHostName);

		if( computerObjects.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "groups of computer" ), { ui->computerHostNameAttributeLabel->text(),
																		ui->groupMemberAttributeLabel->text() },
										  ldapDirectory.groupsOfComputer( computerObjects.first() ), ldapDirectory );
		}
		else
		{
			QMessageBox::warning( this, tr( "Computer not found" ),
								  tr( "Could not find a computer with the hostname \"%1\". "
									  "Please check the hostname or the computer tree "
									  "parameter.").arg( computerHostName ) );
		}
	}
}



void LdapConfigurationPage::testComputerObjectByIpAddress()
{
	QString computerIpAddress = QInputDialog::getText( this, tr( "Enter computer IP address" ),
													   tr( "Please enter a computer IP address which to resolve to an computer object:") );
	if( computerIpAddress.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer object resolve by IP address" << computerIpAddress;

		LdapDirectory ldapDirectory( m_configuration );

		QString computerName = ldapDirectory.hostToLdapFormat( computerIpAddress );

		vDebug() << "[TEST][LDAP] Resolved IP address to computer name" << computerName;

		if( computerName.isEmpty() )
		{
			QMessageBox::critical( this, tr( "Hostname lookup failed" ),
								   tr( "Could not lookup hostname for IP address %1. "
									   "Please check your DNS server settings." ).arg( computerIpAddress ) );
		}
		else
		{
			reportLdapObjectQueryResults( tr( "computers" ), { ui->computerHostNameAttributeLabel->text() },
										  ldapDirectory.computersByHostName( computerName ), ldapDirectory );
		}

	}
}



void LdapConfigurationPage::testLocationEntries()
{
	const auto locationName = QInputDialog::getText( this, tr( "Enter location name" ),
													 tr( "Please enter the name of a location whose entries to query:") );
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing location entries for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );
		reportLdapObjectQueryResults( tr( "location entries" ), { ui->computerGroupsFilterLabel->text(),
																  ui->computerLocationsIdentifications->title() },
									  ldapDirectory.computerLocationEntries( locationName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testLocations()
{
	vDebug() << "[TEST][LDAP] Querying all locations";

	LdapDirectory ldapDirectory( m_configuration );
	reportLdapObjectQueryResults( tr( "location entries" ), { ui->computerGroupsFilterLabel->text(),
															  ui->computerLocationsIdentifications->title() },
								  ldapDirectory.computerLocations(), ldapDirectory );
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



bool LdapConfigurationPage::testBind( bool quiet )
{
	vDebug() << "[TEST][LDAP] Testing bind";

	LdapClient ldapClient( m_configuration );

	if( ldapClient.isConnected() == false )
	{
		QMessageBox::critical( this, tr( "LDAP connection failed"),
							   tr( "Could not connect to the LDAP server. "
								   "Please check the server parameters.\n\n"
								   "%1" ).arg( ldapClient.errorDescription() ) );
	}
	else if( ldapClient.isBound() == false )
	{
		QMessageBox::critical( this, tr( "LDAP bind failed"),
							   tr( "Could not bind to the LDAP server. "
								   "Please check the server parameters "
								   "and bind credentials.\n\n"
								   "%1" ).arg( ldapClient.errorDescription() ) );
	}
	else if( quiet == false )
	{
		QMessageBox::information( this, tr( "LDAP bind successful"),
								  tr( "Successfully connected to the LDAP "
									  "server and performed an LDAP bind. "
									  "The basic LDAP settings are "
									  "configured correctly." ) );
	}

	return ldapClient.isConnected() && ldapClient.isBound();
}




void LdapConfigurationPage::reportLdapTreeQueryResult( const QString& name, int count,
													   const QString& parameter, const QString& errorDescription )
{
	if( count <= 0 )
	{
		QMessageBox::critical( this, tr( "LDAP %1 test failed").arg( name ),
							   tr( "Could not query any entries in configured %1. "
								   "Please check the parameter \"%2\".\n\n"
								   "%3" ).arg( name, parameter, errorDescription ) );
	}
	else
	{
		QMessageBox::information( this, tr( "LDAP %1 test successful" ).arg( name ),
								  tr( "The %1 has been queried successfully and "
									  "%2 entries were found." ).arg( name ).arg( count ) );
	}
}





void LdapConfigurationPage::reportLdapObjectQueryResults( const QString &objectsName, const QStringList& parameterNames,
														  const QStringList& results, const LdapDirectory &directory )
{
	if( results.isEmpty() )
	{
		QStringList parameters;
		parameters.reserve( parameterNames.count() );

		for( const auto& parameterName : parameterNames )
		{
			parameters += QStringLiteral("\"%1\"").arg( parameterName );
		}

		QMessageBox::critical( this, tr( "LDAP test failed"),
							   tr( "Could not query any %1. "
								   "Please check the parameter(s) %2 and enter the name of an existing object.\n\n"
								   "%3" ).arg( objectsName, parameters.join( QStringLiteral(" %1 ").arg( tr("and") ) ),
											   directory.client().errorDescription() ) );
	}
	else
	{
		QMessageBox::information( this, tr( "LDAP test successful" ),
								  tr( "%1 %2 have been queried successfully:\n\n%3" ).
								  arg( results.count() ).
								  arg( objectsName, formatResultsString( results ) ) );
	}
}





void LdapConfigurationPage::reportLdapFilterTestResult( const QString &filterObjects, int count, const QString &errorDescription )
{
	if( count <= 0 )
	{
		QMessageBox::critical( this, tr( "LDAP filter test failed"),
							   tr( "Could not query any %1 using the configured filter. "
								   "Please check the LDAP filter for %1.\n\n"
								   "%2" ).arg( filterObjects, errorDescription ) );
	}
	else
	{
		QMessageBox::information( this, tr( "LDAP filter test successful" ),
								  tr( "%1 %2 have been queried successfully using the configured filter." ).
								  arg( count ).arg( filterObjects ) );
	}
}



QString LdapConfigurationPage::formatResultsString( const QStringList &results )
{
	static constexpr auto FirstResult = 0;
	static constexpr auto MaxResults = 3;

	if( results.count() <= MaxResults )
	{
		return results.join(QLatin1Char('\n'));
	}

	return QStringLiteral( "%1\n[...]" ).arg( results.mid( FirstResult, MaxResults ).join( QLatin1Char('\n') ) );
}
