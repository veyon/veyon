// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMessageBox>

#include "PlatformPluginInterface.h"
#include "PlatformUserFunctions.h"
#include "TextMessageDialog.h"
#include "VeyonCore.h"

#include "ui_TextMessageDialog.h"


TextMessageDialog::TextMessageDialog(QString& msgStr, QString& titleStr, QWidget* parent) :
	QDialog(parent),
	ui(new Ui::TextMessageDialog),
	m_msgStr(msgStr),
	m_titleStr(titleStr)
{
	ui->setupUi(this);

	auto username = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::FullName);
	if (username.isEmpty())
	{
		username = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
	}

	if (username.isEmpty())
	{
		ui->titleEdit->setText(tr("Message from teacher"));
	}
	else
	{
		ui->titleEdit->setText(tr("Message from %1").arg(username));
	}
}



TextMessageDialog::~TextMessageDialog()
{
	delete ui;
}



void TextMessageDialog::accept()
{
	m_msgStr = ui->textEdit->toPlainText().trimmed().isEmpty() ? QString{} : ui->textEdit->toHtml();
	m_titleStr = ui->titleEdit->text().trimmed();
	QDialog::accept();
}
