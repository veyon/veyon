/*
 * TextMessageDialog.cpp - implementation of text message dialog class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>

#include "TextMessageDialog.h"
#include "VeyonCore.h"
#include "PlatformPluginInterface.h"
#include "PlatformUserFunctions.h"

#include "ui_TextMessageDialog.h"


TextMessageDialog::TextMessageDialog( QString &msgStr, QString &titleStr, QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::TextMessageDialog ),
	m_msgStr( msgStr ),
	m_titleStr( titleStr )
{
	ui->setupUi( this );

	auto username = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::FullName);
	if (username.isEmpty())
	{
		username = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
	}

	if (username.isEmpty())
	{
		ui->titleEdit->setPlaceholderText(tr("Message from teacher"));
	}
	else
	{
		ui->titleEdit->setPlaceholderText(tr("Message from %1").arg(username));
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
