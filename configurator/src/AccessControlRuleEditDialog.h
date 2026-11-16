// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "AccessControlRule.h"

namespace Ui {
class AccessControlRuleEditDialog;
}

class AccessControlRuleEditDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AccessControlRuleEditDialog(AccessControlRule& rule, QWidget* parent = nullptr);
	~AccessControlRuleEditDialog() override;

	void accept() override;


private:
	Ui::AccessControlRuleEditDialog* ui;
	AccessControlRule& m_rule;
	QMap<AccessControlRule::Subject, QString> m_subjectNameMap;
};
