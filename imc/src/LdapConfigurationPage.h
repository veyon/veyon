/*
 * LdapConfigurationPage.h - header for the LdapConfigurationPage class
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef LDAP_CONFIGURATION_PAGE_H
#define LDAP_CONFIGURATION_PAGE_H

#include "ConfigurationPage.h"

class LdapDirectory;

namespace Ui {
class LdapConfigurationPage;
}

class LdapConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	LdapConfigurationPage();
	~LdapConfigurationPage();

	virtual void resetWidgets();
	virtual void connectWidgetsToProperties();

private slots:
	bool testLdapBind(bool reportSuccess = true);
	void testLdapBaseDn();
	void testLdapNamingContext();
	void testLdapUserTree();
	void testLdapGroupTree();
	void testLdapComputerTree();
	void testLdapUserLoginAttribute();
	void testLdapGroupMemberAttribute();
	void testLdapComputerHostNameAttribute();
	void testLdapUsersFilter();
	void testLdapUserGroupsFilter();
	void testLdapComputerGroupsFilter();
	void testLdapComputerPoolAttribute();
	void testLdapGroupsOfUser();
	void testLdapGroupsOfComputer();
	void testLdapComputerObjectByIpAddress();
	void testLdapComputerPoolMembers();
	void testLdapCommonAggregations();

private:
	void reportLdapTreeQueryResult( const QString& name, int count, const QString& errorDescription );
	void reportLdapObjectQueryResults( const QString &objectsName, const QString& parameterName,
									   const QStringList &results, const LdapDirectory& directory );
	void reportLdapFilterTestResult( const QString &filterObjects, int count, const QString &errorDescription );

	static QString formatResultsString( const QStringList& results );

	Ui::LdapConfigurationPage *ui;
};

#endif // LDAP_CONFIGURATION_PAGE_H
