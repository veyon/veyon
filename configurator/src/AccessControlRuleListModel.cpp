/*
 * AccessControlRuleListModel.cpp - data model for access control rules
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

#include <QIcon>

#include "AccessControlRuleListModel.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


AccessControlRuleListModel::AccessControlRuleListModel(QObject *parent) :
	QAbstractListModel(parent)
{
	reload();
}



void AccessControlRuleListModel::reload()
{
	beginResetModel();

	m_accessControlRules.clear();

	const auto accessControlRules = VeyonCore::config().accessControlRules();

	for( const auto& accessControlRule : accessControlRules )
	{
		m_accessControlRules.append( AccessControlRule( accessControlRule ) );
	}

	endResetModel();
}



int AccessControlRuleListModel::rowCount(const QModelIndex &parent) const
{
	// For list models only the root node (an invalid parent) should return the list's size. For all
	// other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
	if( parent.isValid() )
	{
		return 0;
	}

	return m_accessControlRules.count();
}



QVariant AccessControlRuleListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() > rowCount() )
	{
		return QVariant();
	}

	const AccessControlRule& rule = m_accessControlRules[index.row()];

	if( role == Qt::DecorationRole )
	{
		switch( rule.action() )
		{
		case AccessControlRule::Action::Allow: return QIcon( QStringLiteral(":/configurator/vcs-normal.png") );
		case AccessControlRule::Action::Deny: return QIcon( QStringLiteral(":/configurator/vcs-conflicting.png") );
		case AccessControlRule::Action::AskForPermission: return QIcon( QStringLiteral(":/configurator/access-rule-ask.png") );
		default: return QIcon( QStringLiteral(":/configurator/vcs-removed.png") );
		}
	}
	else if( role == Qt::DisplayRole )
	{
		return rule.name();
	}
	else if( role == Qt::ToolTipRole )
	{
		return rule.description();
	}

	return QVariant();
}
