/*
 * SmartCardDialog.cpp - dialog for querying logon credentials
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>
#include <QPushButton>

#include "SmartCardDialog.h"
#include "PlatformUserFunctions.h"

#include "ui_SmartCardDialog.h"


SmartCardDialog::SmartCardDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::SmartCardDialog )
{
	ui->setupUi( this );

	reloadCertificateList();

	QPushButton *button = ui->buttonBox->addButton(QString("Rescan"),QDialogButtonBox::ActionRole);
    	connect(button, SIGNAL (released()), this, SLOT (reloadCertificateList()));
	
	ui->pin->setFocus();

	updateOkButton();

	VeyonCore::enforceBranding( this );
}



SmartCardDialog::~SmartCardDialog()
{
	delete ui;
}



QString SmartCardDialog::userPrincipalName() const
{
	return VeyonCore::platform().userFunctions().smartCardCertificateUpn();
}

QVariant SmartCardDialog::certificateId() const
{
	return ui->certificateList->currentData();
}

int SmartCardDialog::certificateIndex() const
{
	return ui->certificateList->currentIndex();
}

QString SmartCardDialog::certificatePem() const
{
	return VeyonCore::platform().userFunctions().smartCardCertificate();
}

QVariant SmartCardDialog::keyIdentifier() const
{
	return VeyonCore::platform().userFunctions().smartCardKeyIdentifier();
}


QString SmartCardDialog::pin() const
{
	return ui->pin->text();
}




AuthenticationCredentials SmartCardDialog::credentials() const
{
	AuthenticationCredentials cred;

	cred.setLogonUsername( userPrincipalName() );
	cred.setLogonPassword( pin() );
	cred.setSmartCardCertificate( certificatePem() );
	cred.setSmartCardKeyIdentifier( keyIdentifier() );
	return cred;
}



void SmartCardDialog::accept()
{
	setCursor(Qt::WaitCursor);
	if( VeyonCore::platform().userFunctions().smartCardAuthenticate( certificateId(), pin() ) == false )
	{
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical( window(),
							   tr( "Authentication error" ),
							   tr( "Logon failed with given Certificate and Pin. Please try again!" ) );
	}
	else
	{
		setCursor(Qt::ArrowCursor);
		QDialog::accept();
	}

}

void SmartCardDialog::reset()
{
	reloadCertificateList();
}

void SmartCardDialog::updateOkButton()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( certificateIndex() >= 0 && !pin().isEmpty() );
}

void SmartCardDialog::reloadCertificateList()
{
	while (ui->certificateList->count() > 0){
		ui->certificateList->removeItem(0);
	}
	QMap<QString,QVariant>smartCardCertificateIds =	VeyonCore::platform().userFunctions().smartCardCertificateIds();
	
	QMap<QString, QVariant>::iterator i;
	for (i = smartCardCertificateIds.begin(); i != smartCardCertificateIds.end(); ++i)
	{
		ui->certificateList->addItem(i.key(),i.value());
	}
}


