/*
 * RunProgramDialog.cpp - implementation of RunProgramDialog
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "RunProgramDialog.h"

#include "ui_RunProgramDialog.h"

RunProgramDialog::RunProgramDialog( QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::RunProgramDialog ),
	m_programs(),
	m_remember( false ),
	m_presetName()
{
	ui->setupUi( this );

	connect( ui->programInputTextEdit, &QTextEdit::textChanged, this, &RunProgramDialog::validate );
	connect( ui->rememberCheckBox, &QCheckBox::toggled, this, &RunProgramDialog::validate );
	connect( ui->presetNameEdit, &QLineEdit::textChanged, this, &RunProgramDialog::validate );

	validate();
}



RunProgramDialog::~RunProgramDialog()
{
	delete ui;
}



void RunProgramDialog::validate()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(
				ui->programInputTextEdit->toPlainText().isEmpty() == false &&
				( ui->rememberCheckBox->isChecked() == false || ui->presetNameEdit->text().isEmpty() == false ) );
}



void RunProgramDialog::accept()
{
	m_programs = ui->programInputTextEdit->toPlainText();
	m_remember = ui->rememberCheckBox->isChecked();
	m_presetName = ui->presetNameEdit->text();

	QDialog::accept();
}
