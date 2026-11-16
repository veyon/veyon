// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPushButton>
#include <QUrl>

#include "OpenWebsiteDialog.h"

#include "ui_OpenWebsiteDialog.h"

OpenWebsiteDialog::OpenWebsiteDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::OpenWebsiteDialog),
	m_website(),
	m_remember(false),
	m_presetName()
{
	ui->setupUi(this);

	connect(ui->websiteLineEdit, &QLineEdit::textChanged, this, &OpenWebsiteDialog::validate);
	connect(ui->rememberCheckBox, &QCheckBox::toggled, this, &OpenWebsiteDialog::validate);
	connect(ui->presetNameEdit, &QLineEdit::textChanged, this, &OpenWebsiteDialog::validate);

	validate();

	ui->websiteLineEdit->setFocus();
}



OpenWebsiteDialog::~OpenWebsiteDialog()
{
	delete ui;
}



void OpenWebsiteDialog::validate()
{
	QUrl url(ui->websiteLineEdit->text(), QUrl::TolerantMode);
	if (url.scheme().isEmpty())
	{
		url = QUrl(QStringLiteral("http://") + ui->websiteLineEdit->text(), QUrl::TolerantMode);
	}

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(url.isEmpty() == false && url.isValid() && (ui->rememberCheckBox->isChecked() == false || ui->presetNameEdit->text().isEmpty() == false));
}



void OpenWebsiteDialog::accept()
{
	m_website = ui->websiteLineEdit->text();
	m_remember = ui->rememberCheckBox->isChecked();
	m_presetName = ui->presetNameEdit->text();

	QDialog::accept();
}
