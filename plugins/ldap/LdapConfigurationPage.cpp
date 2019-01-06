/*
 * LdapConfigurationPage.cpp - implementation of the LdapConfigurationPage class
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "LdapConfiguration.h"
#include "LdapConfigurationPage.h"
#include "Configuration/UiMapping.h"
#include "LdapDirectory.h"

#include "ui_LdapConfigurationPage.h"

LdapConfigurationPage::LdapConfigurationPage( LdapConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::LdapConfigurationPage),
	m_configuration( configuration )
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name)	connect( ui->name, &QAbstractButton::clicked, this, &LdapConfigurationPage::name );

	CONNECT_BUTTON_SLOT( testBindInteractively );
	CONNECT_BUTTON_SLOT( testBaseDn );
	CONNECT_BUTTON_SLOT( testNamingContext );
	CONNECT_BUTTON_SLOT( testUserTree );
	CONNECT_BUTTON_SLOT( testGroupTree );
	CONNECT_BUTTON_SLOT( testComputerTree );
	CONNECT_BUTTON_SLOT( testComputerGroupTree );

	CONNECT_BUTTON_SLOT( testUserLoginAttribute );
	CONNECT_BUTTON_SLOT( testGroupMemberAttribute );
	CONNECT_BUTTON_SLOT( testComputerHostNameAttribute );
	CONNECT_BUTTON_SLOT( testComputerMacAddressAttribute );
	CONNECT_BUTTON_SLOT( testComputerRoomAttribute );
	CONNECT_BUTTON_SLOT( testComputerRoomNameAttribute );

	CONNECT_BUTTON_SLOT( testUsersFilter );
	CONNECT_BUTTON_SLOT( testUserGroupsFilter );
	CONNECT_BUTTON_SLOT( testComputersFilter );
	CONNECT_BUTTON_SLOT( testComputerGroupsFilter );
	CONNECT_BUTTON_SLOT( testComputerContainersFilter );

	CONNECT_BUTTON_SLOT( testGroupsOfUser );
	CONNECT_BUTTON_SLOT( testGroupsOfComputer );
	CONNECT_BUTTON_SLOT( testComputerObjectByIpAddress );
	CONNECT_BUTTON_SLOT( testComputerRoomMembers );
	CONNECT_BUTTON_SLOT( testComputerRooms );

	CONNECT_BUTTON_SLOT( browseCACertificateFile );

	connect( ui->tlsVerifyMode, QOverload<int>::of( &QComboBox::currentIndexChanged ), ui->tlsCACertificateFile, [=]() {
		ui->tlsCACertificateFile->setEnabled( ui->tlsVerifyMode->currentIndex() == LdapConfiguration::TLSVerifyCustomCert );
	} );
}



LdapConfigurationPage::~LdapConfigurationPage()
{
	delete ui;
}



void LdapConfigurationPage::resetWidgets()
{
	// sanitize configuration
	if( m_configuration.serverPort() <= 0 )
	{
		m_configuration.setServerPort( 389 );
	}

	FOREACH_LDAP_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void LdapConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_LDAP_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void LdapConfigurationPage::applyConfiguration()
{
}



void LdapConfigurationPage::testBindInteractively()
{
	testBind( false );
}



void LdapConfigurationPage::testBaseDn()
{
	if( testBindQuietly() )
	{
		qDebug() << "[TEST][LDAP] Testing base DN";

		LdapDirectory ldapDirectory( m_configuration );
		QStringList entries = ldapDirectory.queryBaseDn();

		if( entries.isEmpty() )
		{
			QMessageBox::critical( this, tr( "LDAP base DN test failed"),
								   tr( "Could not query the configured base DN. "
									   "Please check the base DN parameter.\n\n"
									   "%1" ).arg( ldapDirectory.ldapErrorDescription() ) );
		}
		else
		{
			QMessageBox::information( this, tr( "LDAP base DN test successful" ),
							tr( "The LDAP base DN has been queried successfully. "
								"The following entries were found:\n\n%1" ).
									  arg( entries.join('\n') ) );
		}
	}
}



void LdapConfigurationPage::testNamingContext()
{
	if( testBindQuietly() )
	{
		qDebug() << "[TEST][LDAP] Testing naming context";

		LdapDirectory ldapDirectory( m_configuration );
		QString baseDn = ldapDirectory.queryNamingContext();

		if( baseDn.isEmpty() )
		{
			QMessageBox::critical( this, tr( "LDAP naming context test failed"),
								   tr( "Could not query the base DN via naming contexts. "
									   "Please check the naming context attribute parameter.\n\n"
									   "%1" ).arg( ldapDirectory.ldapErrorDescription() ) );
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
		qDebug() << "[TEST][LDAP] Testing user tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.users().count();

		reportLdapTreeQueryResult( tr( "user tree" ), count, ldapDirectory.ldapErrorDescription() );
	}
}



void LdapConfigurationPage::testGroupTree()
{
	if( testBindQuietly() )
	{
		qDebug() << "[TEST][LDAP] Testing group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.groups().count();

		reportLdapTreeQueryResult( tr( "group tree" ), count, ldapDirectory.ldapErrorDescription() );
	}
}



void LdapConfigurationPage::testComputerTree()
{
	if( testBindQuietly() )
	{
		qDebug() << "[TEST][LDAP] Testing computer tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computers().count();

		reportLdapTreeQueryResult( tr( "computer tree" ), count, ldapDirectory.ldapErrorDescription() );
	}
}



void LdapConfigurationPage::testComputerGroupTree()
{
	if( testBindQuietly() )
	{
		qDebug() << "[TEST][LDAP] Testing computer group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computerGroups().count();

		reportLdapTreeQueryResult( tr( "computer group tree" ), count, ldapDirectory.ldapErrorDescription() );
	}
}



void LdapConfigurationPage::testUserLoginAttribute()
{
	QString userFilter = QInputDialog::getText( this, tr( "Enter username" ),
										  tr( "Please enter a user login name (wildcards allowed) which to query:") );
	if( userFilter.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing user login attribute for" << userFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		reportLdapObjectQueryResults( tr( "user objects" ), tr( "user login attribute" ),
									  ldapDirectory.users( userFilter ), ldapDirectory );
	}
}



void LdapConfigurationPage::testGroupMemberAttribute()
{
	QString groupFilter = QInputDialog::getText( this, tr( "Enter group name" ),
										  tr( "Please enter a group name whose members to query:") );
	if( groupFilter.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing group member attribute for" << groupFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		QStringList groups = ldapDirectory.groups( groupFilter );

		if( groups.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "group members" ), tr( "group member attribute" ),
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



void LdapConfigurationPage::testComputerHostNameAttribute()
{
	QString computerName = QInputDialog::getText( this, tr( "Enter computer name" ),
										  tr( "Please enter a computer host name to query:") );
	if( computerName.isEmpty() == false )
	{
		if( m_configuration.computerHostNameAsFQDN() &&
				computerName.contains( '.' ) == false )
		{
			QMessageBox::critical( this, tr( "Invalid host name" ),
								   tr( "You configured computer host names to be stored "
									   "as fully qualified domain names (FQDN) but entered "
									   "a host name without domain." ) );
			return;
		}
		else if( m_configuration.computerHostNameAsFQDN() == false &&
				 computerName.contains( '.') )
		{
			QMessageBox::critical( this, tr( "Invalid host name" ),
								   tr( "You configured computer host names to be stored "
									   "as simple host names without a domain name but "
									   "entered a host name with a domain name part." ) );
			return;
		}

		qDebug() << "[TEST][LDAP] Testing computer host name attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		reportLdapObjectQueryResults( tr( "computer objects" ), tr( "computer host name attribute" ),
									  ldapDirectory.computers( computerName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerMacAddressAttribute()
{
	QString computerDn = QInputDialog::getText( this, tr( "Enter computer DN" ),
										  tr( "Please enter the DN of a computer whose MAC address to query:") );
	if( computerDn.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing computer MAC address attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		QString macAddress = ldapDirectory.computerMacAddress( computerDn );

		reportLdapObjectQueryResults( tr( "computer MAC addresses" ), tr( "computer MAC address attribute" ),
									  macAddress.isEmpty() ? QStringList() : QStringList( macAddress ),
									  ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerRoomAttribute()
{
	QString computerRoomName = QInputDialog::getText( this, tr( "Enter computer room name" ),
										  tr( "Please enter the name of a computer room (wildcards allowed):") );
	if( computerRoomName.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing computer room attribute for" << computerRoomName;

		LdapDirectory ldapDirectory( m_configuration );

		reportLdapObjectQueryResults( tr( "computer rooms" ), tr( "computer room attribute" ),
									  ldapDirectory.computerRooms( computerRoomName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerRoomNameAttribute()
{
	if( m_configuration.computerRoomMembersByAttribute() )
	{
		QMessageBox::information( this, tr( "Test not applicable" ),
								  tr( "Please change the computer room settings to use computer groups "
									  "or computer containers as computer rooms. Then the "
									  "specified attribute instead of the common name of computer groups "
									  "or container objects will be queried. "
									  "Otherwise you don't need to configure this attribute." ) );
		return;
	}

	testComputerRoomAttribute();
}



void LdapConfigurationPage::testUsersFilter()
{
	qDebug() << "[TEST][LDAP] Testing users filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.users().count();

	reportLdapFilterTestResult( tr( "users" ), count, ldapDirectory.ldapErrorDescription() );
}



void LdapConfigurationPage::testUserGroupsFilter()
{
	qDebug() << "[TEST][LDAP] Testing user groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.userGroups().count();

	reportLdapFilterTestResult( tr( "user groups" ), count, ldapDirectory.ldapErrorDescription() );
}



void LdapConfigurationPage::testComputersFilter()
{
	qDebug() << "[TEST][LDAP] Testing computers filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computers().count();

	reportLdapFilterTestResult( tr( "computers" ), count, ldapDirectory.ldapErrorDescription() );
}



void LdapConfigurationPage::testComputerGroupsFilter()
{
	qDebug() << "[TEST][LDAP] Testing computer groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	int count = ldapDirectory.computerGroups().count();

	reportLdapFilterTestResult( tr( "computer groups" ), count, ldapDirectory.ldapErrorDescription() );
}



void LdapConfigurationPage::testComputerContainersFilter()
{
	if( m_configuration.computerRoomMembersByContainer() == false )
	{
		QMessageBox::information( this, tr( "Test not applicable" ),
								  tr( "Please change the computer room settings below to use computer containers "
									  "as computer rooms. Otherwise you don't need to "
									  "configure this filter." ) );
		return;
	}

	testComputerRooms();
}



void LdapConfigurationPage::testGroupsOfUser()
{
	QString username = QInputDialog::getText( this, tr( "Enter username" ),
										  tr( "Please enter a user login name whose group memberships to query:") );
	if( username.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing groups of user" << username;

		LdapDirectory ldapDirectory( m_configuration );

		QStringList userObjects = ldapDirectory.users(username);

		if( userObjects.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "groups of user" ), tr( "user login attribute or group membership attribute" ),
										  ldapDirectory.groupsOfUser( userObjects.first() ), ldapDirectory );
		}
		else
		{
			QMessageBox::warning( this, tr( "User not found" ),
								  tr( "Could not find a user with the name \"%1\". "
									  "Please check the user name or the user "
									  "tree parameter.").arg( username ) );
		}
	}
}



void LdapConfigurationPage::testGroupsOfComputer()
{
	QString computerHostName = QInputDialog::getText( this, tr( "Enter host name" ),
										  tr( "Please enter a computer host name whose group memberships to query:") );
	if( computerHostName.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing groups of computer for" << computerHostName;

		LdapDirectory ldapDirectory( m_configuration );

		QStringList computerObjects = ldapDirectory.computers(computerHostName);

		if( computerObjects.isEmpty() == false )
		{
			reportLdapObjectQueryResults( tr( "groups of computer" ), tr( "computer host name attribute or group membership attribute" ),
										  ldapDirectory.groupsOfComputer( computerObjects.first() ), ldapDirectory );
		}
		else
		{
			QMessageBox::warning( this, tr( "Computer not found" ),
								  tr( "Could not find a computer with the host name \"%1\". "
									  "Please check the host name or the computer tree "
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
		qDebug() << "[TEST][LDAP] Testing computer object resolve by IP address" << computerIpAddress;

		LdapDirectory ldapDirectory( m_configuration );

		QString computerName = ldapDirectory.hostToLdapFormat( computerIpAddress );

		qDebug() << "[TEST][LDAP] Resolved IP address to computer name" << computerName;

		if( computerName.isEmpty() )
		{
			QMessageBox::critical( this, tr( "Host name lookup failed" ),
								   tr( "Could not lookup host name for IP address %1. "
									   "Please check your DNS server settings." ).arg( computerIpAddress ) );
		}
		else
		{
			reportLdapObjectQueryResults( tr( "computers" ), tr( "computer host name attribute" ),
										  ldapDirectory.computers( computerName ), ldapDirectory );
		}

	}
}



void LdapConfigurationPage::testComputerRoomMembers()
{
	QString computerRoomName = QInputDialog::getText( this, tr( "Enter computer room name" ),
													  tr( "Please enter the name of a computer room whose members to query:") );
	if( computerRoomName.isEmpty() == false )
	{
		qDebug() << "[TEST][LDAP] Testing computer room members for" << computerRoomName;

		LdapDirectory ldapDirectory( m_configuration );
		reportLdapObjectQueryResults( tr( "computer room members" ),
									  tr( "computer group filter or computer room member aggregation" ),
									  ldapDirectory.computerRoomMembers( computerRoomName ), ldapDirectory );
	}
}



void LdapConfigurationPage::testComputerRooms()
{
	qDebug() << "[TEST][LDAP] Querying all computer rooms";

	LdapDirectory ldapDirectory( m_configuration );
	reportLdapObjectQueryResults( tr( "computer rooms" ),
									  tr( "computer group filter or computer room member aggregation" ),
								  ldapDirectory.computerRooms(), ldapDirectory );
}



void LdapConfigurationPage::browseCACertificateFile()
{
	auto caCertFile = QFileDialog::getOpenFileName( this, tr( "Custom CA certificate file" ), QString(),
													tr( "Certificate files (*.pem)" ) );
	if( caCertFile.isEmpty() == false )
	{
		ui->tlsCACertificateFile->setText( caCertFile );
	}
}



bool LdapConfigurationPage::testBind( bool quiet )
{
	qDebug() << "[TEST][LDAP] Testing bind";

	LdapDirectory ldapDirectory( m_configuration );

	if( ldapDirectory.isConnected() == false )
	{
		QMessageBox::critical( this, tr( "LDAP connection failed"),
							   tr( "Could not connect to the LDAP server. "
								   "Please check the server parameters.\n\n"
								   "%1" ).arg( ldapDirectory.ldapErrorDescription() ) );
	}
	else if( ldapDirectory.isBound() == false )
	{
		QMessageBox::critical( this, tr( "LDAP bind failed"),
							   tr( "Could not bind to the LDAP server. "
								   "Please check the server parameters "
								   "and bind credentials.\n\n"
								   "%1" ).arg( ldapDirectory.ldapErrorDescription() ) );
	}
	else if( quiet == false )
	{
		QMessageBox::information( this, tr( "LDAP bind successful"),
								  tr( "Successfully connected to the LDAP "
									  "server and performed an LDAP bind. "
									  "The basic LDAP settings are "
									  "configured correctly." ) );
	}

	return ldapDirectory.isConnected() && ldapDirectory.isBound();
}




void LdapConfigurationPage::reportLdapTreeQueryResult(const QString &name, int count, const QString &errorDescription)
{
	if( count <= 0 )
	{
		QMessageBox::critical( this, tr( "LDAP %1 test failed").arg( name ),
							   tr( "Could not query any entries in configured %1. "
								   "Please check the %1 parameter.\n\n"
								   "%2" ).arg( name, errorDescription ) );
	}
	else
	{
		QMessageBox::information( this, tr( "LDAP %1 test successful" ).arg( name ),
						tr( "The %1 has been queried successfully and "
							"%2 entries were found." ).arg( name ).arg( count ) );
	}
}





void LdapConfigurationPage::reportLdapObjectQueryResults( const QString &objectsName, const QString& parameterName,
											   const QStringList& results, const LdapDirectory &directory )
{
	if( results.isEmpty() )
	{
		QMessageBox::critical( this, tr( "LDAP %1 test failed").arg( parameterName ),
							   tr( "Could not query any %1. "
								   "Please check the %2 parameter or enter the name of an existing object.\n\n"
								   "%3" ).arg( objectsName, parameterName, directory.ldapErrorDescription() ) );
	}
	else
	{
		QMessageBox::information( this, tr( "LDAP %1 test successful" ).arg( parameterName ),
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
	switch( results.count() )
	{
	case 0: return QString();
	case 1: return results.first();
	case 2: return QStringLiteral( "%1\n%2" ).arg( results[0], results[1] );
	default: break;
	}

	return QStringLiteral( "%1\n%2\n[...]" ).arg( results[0], results[1] );
}
