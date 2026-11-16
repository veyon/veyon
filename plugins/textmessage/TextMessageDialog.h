// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class TextMessageDialog;
}

class TextMessageDialog : public QDialog
{
	Q_OBJECT
public:
	TextMessageDialog(QString& msgStr, QString& titleStr, QWidget* parent);
	~TextMessageDialog() override;

private Q_SLOTS:
	void accept() override;


private:
	Ui::TextMessageDialog* ui;
	QString& m_msgStr;
	QString& m_titleStr;
};
