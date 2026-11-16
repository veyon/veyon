// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>

#include "AccessControlRule.h"

class AccessControlRuleListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit AccessControlRuleListModel(QObject* parent = nullptr);

	void reload();

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
	QList<AccessControlRule> m_accessControlRules;
};
