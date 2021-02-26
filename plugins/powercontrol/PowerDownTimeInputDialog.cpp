/*
 * PowerDownTimeInputDialog.h - implementation of PowerDownTimeInputDialog class
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

#include "QtCompat.h"
#include "PowerDownTimeInputDialog.h"

#include "ui_PowerDownTimeInputDialog.h"


PowerDownTimeInputDialog::PowerDownTimeInputDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::PowerDownTimeInputDialog ),
	m_seconds( 0 )
{
	ui->setupUi( this );

	updateSeconds();

	connect( ui->minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PowerDownTimeInputDialog::updateSeconds );
	connect( ui->secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PowerDownTimeInputDialog::updateSeconds );
}



PowerDownTimeInputDialog::~PowerDownTimeInputDialog()
{
	delete ui;
}



int PowerDownTimeInputDialog::seconds() const
{
	return m_seconds;
}



void PowerDownTimeInputDialog::updateSeconds()
{
	m_seconds = ui->minutesSpinBox->value() * 60 + ui->secondsSpinBox->value();

	ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( m_seconds > 0 );
}
