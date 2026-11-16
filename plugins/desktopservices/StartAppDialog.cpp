// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPushButton>

#include "StartAppDialog.h"

#include "ui_StartAppDialog.h"

StartAppDialog::StartAppDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::StartAppDialog),
	m_apps(),
	m_remember(false),
	m_presetName()
{
	ui->setupUi(this);

	connect(ui->appInputTextEdit, &QTextEdit::textChanged, this, &StartAppDialog::validate);
	connect(ui->rememberCheckBox, &QCheckBox::toggled, this, &StartAppDialog::validate);
	connect(ui->presetNameEdit, &QLineEdit::textChanged, this, &StartAppDialog::validate);

	validate();

	ui->appInputTextEdit->setFocus();
}



StartAppDialog::~StartAppDialog()
{
	delete ui;
}



void StartAppDialog::validate()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->appInputTextEdit->toPlainText().isEmpty() == false && (ui->rememberCheckBox->isChecked() == false || ui->presetNameEdit->text().isEmpty() == false));
}



void StartAppDialog::accept()
{
	m_apps = ui->appInputTextEdit->toPlainText();
	m_remember = ui->rememberCheckBox->isChecked();
	m_presetName = ui->presetNameEdit->text();

	QDialog::accept();
}
