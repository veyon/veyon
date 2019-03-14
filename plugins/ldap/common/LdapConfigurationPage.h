/*
 * LdapConfigurationPage.h - header for the LdapConfigurationPage class
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

#pragma once

#include "ConfigurationPage.h"

class LdapConfiguration;
class LdapDirectory;

namespace Ui {
class LdapConfigurationPage;
}

class QLineEdit;

class LdapConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	LdapConfigurationPage( LdapConfiguration& configuration, QWidget* parent = nullptr );
	~LdapConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void browseBaseDn();
	void browseObjectTree( QLineEdit* lineEdit );
	void browseAttribute( QLineEdit* lineEdit, const QString& tree );

	void testBindInteractively();
	void testBaseDn();
	void testNamingContext();
	void testUserTree();
	void testGroupTree();
	void testComputerTree();
	void testComputerGroupTree();
	void testUserLoginNameAttribute();
	void testGroupMemberAttribute();
	void testComputerDisplayNameAttribute();
	void testComputerHostNameAttribute();
	void testComputerMacAddressAttribute();
	void testComputerLocationAttribute();
	void testLocationNameAttribute();
	void testUsersFilter();
	void testUserGroupsFilter();
	void testComputersFilter();
	void testComputerGroupsFilter();
	void testComputerContainersFilter();
	void testGroupsOfUser();
	void testGroupsOfComputer();
	void testComputerObjectByIpAddress();
	void testLocationEntries();
	void testLocations();

	void browseCACertificateFile();

	bool testBindQuietly()
	{
		return testBind( true );
	}

	bool testBind( bool quiet );
	void reportLdapTreeQueryResult( const QString& name, int count, const QString& parameter,
									const QString& errorDescription );
	void reportLdapObjectQueryResults( const QString &objectsName, const QStringList& parameterNames,
									   const QStringList &results, const LdapDirectory& directory );
	void reportLdapFilterTestResult( const QString &filterObjects, int count, const QString &errorDescription );

	static QString formatResultsString( const QStringList& results );

	Ui::LdapConfigurationPage *ui;

	LdapConfiguration& m_configuration;

};
