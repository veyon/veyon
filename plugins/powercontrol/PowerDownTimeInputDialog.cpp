// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPushButton>

#include "PowerDownTimeInputDialog.h"

#include "ui_PowerDownTimeInputDialog.h"


PowerDownTimeInputDialog::PowerDownTimeInputDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::PowerDownTimeInputDialog),
	m_seconds(0)
{
	ui->setupUi(this);

	updateSeconds();

	connect(ui->minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PowerDownTimeInputDialog::updateSeconds);
	connect(ui->secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PowerDownTimeInputDialog::updateSeconds);
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

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_seconds > 0);
}
