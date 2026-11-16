// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>

#include "AccessControlRuleListModel.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


AccessControlRuleListModel::AccessControlRuleListModel(QObject* parent) :
	QAbstractListModel(parent)
{
	reload();
}



void AccessControlRuleListModel::reload()
{
	beginResetModel();

	m_accessControlRules.clear();

	const auto accessControlRules = VeyonCore::config().accessControlRules();

	for (const auto& accessControlRule : accessControlRules)
	{
		m_accessControlRules.append(AccessControlRule(accessControlRule));
	}

	endResetModel();
}



int AccessControlRuleListModel::rowCount(const QModelIndex& parent) const
{
	// For list models only the root node (an invalid parent) should return the list's size. For all
	// other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
	if (parent.isValid())
	{
		return 0;
	}

	return m_accessControlRules.count();
}



QVariant AccessControlRuleListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() >= rowCount())
	{
		return {};
	}

	const AccessControlRule& rule = m_accessControlRules[index.row()];

	if (role == Qt::DecorationRole)
	{
		switch (rule.action())
		{
		case AccessControlRule::Action::Allow: return QIcon(QStringLiteral(":/configurator/vcs-normal.png"));
		case AccessControlRule::Action::Deny: return QIcon(QStringLiteral(":/configurator/vcs-conflicting.png"));
		case AccessControlRule::Action::AskForPermission: return QIcon(QStringLiteral(":/configurator/access-rule-ask.png"));
		default: return QIcon(QStringLiteral(":/configurator/vcs-removed.png"));
		}
	}
	else if (role == Qt::DisplayRole)
	{
		return rule.name();
	}
	else if (role == Qt::ToolTipRole)
	{
		return rule.description();
	}

	return {};
}
