/*
 * StartAppDialog.cpp - implementation of StartAppDialog
 *
 * Copyright (c) 2004-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "StartAppDialog.h"

#include "ui_StartAppDialog.h"

StartAppDialog::StartAppDialog( QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::StartAppDialog ),
	m_apps(),
	m_remember( false ),
	m_presetName()
{
	ui->setupUi( this );

	connect( ui->appInputTextEdit, &QTextEdit::textChanged, this, &StartAppDialog::validate );
	connect( ui->rememberCheckBox, &QCheckBox::toggled, this, &StartAppDialog::validate );
	connect( ui->presetNameEdit, &QLineEdit::textChanged, this, &StartAppDialog::validate );

	validate();

	ui->appInputTextEdit->setFocus();
}



StartAppDialog::~StartAppDialog()
{
	delete ui;
}



void StartAppDialog::validate()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(
				ui->appInputTextEdit->toPlainText().isEmpty() == false &&
				( ui->rememberCheckBox->isChecked() == false || ui->presetNameEdit->text().isEmpty() == false ) );
}



void StartAppDialog::accept()
{
	m_apps = ui->appInputTextEdit->toPlainText();
	m_remember = ui->rememberCheckBox->isChecked();
	m_presetName = ui->presetNameEdit->text();

	QDialog::accept();
}
