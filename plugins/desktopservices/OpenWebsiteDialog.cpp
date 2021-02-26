/*
 * OpenWebsiteDialog.cpp - implementation of OpenWebsiteDialog
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QPushButton>
#include <QUrl>

#include "OpenWebsiteDialog.h"

#include "ui_OpenWebsiteDialog.h"

OpenWebsiteDialog::OpenWebsiteDialog( QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::OpenWebsiteDialog ),
	m_website(),
	m_remember( false ),
	m_presetName()
{
	ui->setupUi( this );

	connect( ui->websiteLineEdit, &QLineEdit::textChanged, this, &OpenWebsiteDialog::validate );
	connect( ui->rememberCheckBox, &QCheckBox::toggled, this, &OpenWebsiteDialog::validate );
	connect( ui->presetNameEdit, &QLineEdit::textChanged, this, &OpenWebsiteDialog::validate );

	validate();

	ui->websiteLineEdit->setFocus();
}



OpenWebsiteDialog::~OpenWebsiteDialog()
{
	delete ui;
}



void OpenWebsiteDialog::validate()
{
	QUrl url( ui->websiteLineEdit->text(), QUrl::TolerantMode );
	if( url.scheme().isEmpty() )
	{
		url = QUrl( QStringLiteral("http://") + ui->websiteLineEdit->text(), QUrl::TolerantMode );
	}

	ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(
				url.isEmpty() == false && url.isValid() &&
				( ui->rememberCheckBox->isChecked() == false || ui->presetNameEdit->text().isEmpty() == false ) );
}



void OpenWebsiteDialog::accept()
{
	m_website = ui->websiteLineEdit->text();
	m_remember = ui->rememberCheckBox->isChecked();
	m_presetName = ui->presetNameEdit->text();

	QDialog::accept();
}
