/*
 * AccessControlPage.h - header for the AccessControlPage class
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

#ifndef ACCESS_CONTROL_PAGE_H
#define ACCESS_CONTROL_PAGE_H

#include "AccessControlRuleListModel.h"
#include "AccessControlRulesTestDialog.h"
#include "ConfigurationPage.h"

namespace Ui {
class AccessControlPage;
}

class AccessControlPage : public ConfigurationPage
{
	Q_OBJECT
public:
	AccessControlPage();
	~AccessControlPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private slots:
	void addAccessGroup();
	void removeAccessGroup();
	void updateAccessGroupsLists();

	void addAccessControlRule();
	void removeAccessControlRule();
	void editAccessControlRule();
	void moveAccessControlRuleDown();
	void moveAccessControlRuleUp();

	void testUserGroupsAccessControl();
	void testAccessControlRules();

private:
	void modifyAccessControlRules( const QJsonArray& accessControlRules, int selectedRow );
	void updateAccessControlRules();

	Ui::AccessControlPage *ui;

	QStringList m_accessGroups;
	AccessControlRuleListModel m_accessControlRulesModel;

	AccessControlRulesTestDialog m_accessControlRulesTestDialog;

};

#endif // ACCESS_CONTROL_PAGE_H
