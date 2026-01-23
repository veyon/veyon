// Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "LdapCommon.h"

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
	explicit LdapConfigurationPage( LdapConfiguration& configuration, QWidget* parent = nullptr );
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
