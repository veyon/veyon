// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

private Q_SLOTS:
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
	void modifyAccessControlRules(const QJsonArray& accessControlRules, int selectedRow);
	void updateAccessControlRules();

	Ui::AccessControlPage* ui;

	QStringList m_accessGroups;
	AccessControlRuleListModel m_accessControlRulesModel;

	AccessControlRulesTestDialog m_accessControlRulesTestDialog;
};
