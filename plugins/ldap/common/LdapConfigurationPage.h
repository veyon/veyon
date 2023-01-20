// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "LdapConfigurationTest.h"
#include "LdapCommon.h"

class LdapConfiguration;
class LdapDirectory;

namespace Ui {
class LdapConfigurationPage;
}

class QLineEdit;

class LDAP_COMMON_EXPORT LdapConfigurationPage : public ConfigurationPage
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

	void testBind();
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

	void showTestResult( const LdapConfigurationTest::Result& result );

	Ui::LdapConfigurationPage *ui;

	LdapConfiguration& m_configuration;

};
