/*
 * RunProgramDialog.cpp - implementation of RunProgramDialog
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include "RunProgramDialog.h"

#include "ui_RunProgramDialog.h"

RunProgramDialog::RunProgramDialog( QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::RunProgramDialog ),
	m_programs()
{
	ui->setupUi( this );
}



RunProgramDialog::~RunProgramDialog()
{
	delete ui;
}



void RunProgramDialog::accept()
{
	m_programs = ui->programInputTextEdit->toPlainText().split( '\n' );

	QDialog::accept();
}
