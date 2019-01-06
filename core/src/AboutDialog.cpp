/*
 * AboutDialog.cpp - implementation of AboutDialog
 *
 * Copyright (c) 2011-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QDesktopServices>
#include <QFile>

#include "AboutDialog.h"
#include "VeyonCore.h"

#include "ui_AboutDialog.h"


AboutDialog::AboutDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::AboutDialog )
{
	ui->setupUi( this );

	setWindowTitle( tr( "About %1 %2" ).arg( VeyonCore::applicationName(), VeyonCore::version() ) );

	ui->versionLabel->setText( VeyonCore::version() );

	QFile authors( QStringLiteral( ":/CONTRIBUTORS" ) );
	authors.open( QFile::ReadOnly ); // Flawfinder: ignore
	ui->authors->setPlainText( QString::fromUtf8( authors.readAll() ) );

	QFile license( QStringLiteral( ":/core/COPYING" ) );
	license.open( QFile::ReadOnly ); // Flawfinder: ignore
	ui->license->setPlainText( QString::fromUtf8( license.readAll() ) );

	VeyonCore::enforceBranding( this );
}



AboutDialog::~AboutDialog()
{
	delete ui;
}



void AboutDialog::openDonationWebsite()
{
	  QDesktopServices::openUrl( QUrl( QStringLiteral( "https://www.paypal.com/cgi-bin/webscr?item_name=Donation+to+Veyon+-+OpenSource+classroom+management&cmd=_donations&business=donate%40veyon.io" ) ) );
}
