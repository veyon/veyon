// SPDX-FileCopyrightText: 2011-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDesktopServices>
#include <QFile>

#include "AboutDialog.h"
#include "VeyonCore.h"

#include "ui_AboutDialog.h"


AboutDialog::AboutDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	setWindowTitle(tr("About Veyon %1").arg(VeyonCore::versionString()));

	ui->versionLabel->setText(VeyonCore::versionString());

	QFile authors(QStringLiteral(":/CONTRIBUTORS"));
	if (authors.open(QFile::ReadOnly))
	{
		ui->authors->setPlainText(QString::fromUtf8(authors.readAll()));
	}

	QFile license(QStringLiteral(":/core/COPYING"));
	if (license.open(QFile::ReadOnly))
	{
		ui->license->setPlainText(QString::fromUtf8(license.readAll()));
	}
}



AboutDialog::~AboutDialog()
{
	delete ui;
}



void AboutDialog::openDonationWebsite()
{
	QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.paypal.com/cgi-bin/webscr?item_name=Donation+to+Veyon+-+OpenSource+classroom+management&cmd=_donations&business=donate%40veyon.io")));
}
