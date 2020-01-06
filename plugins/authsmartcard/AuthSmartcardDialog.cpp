/*
 * AuthSmartcardDialog.cpp - dialog for querying smartcard credentials
 *
 * Copyright (c) 2019 State of New Jersey
 * 
 * Maintained by Austin McGuire <austin.mcguire@jjc.nj.gov> 
 * 
 * This file is part of a fork of Veyon - https://veyon.io
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
#include <QCryptographicHash>

#include "AuthSmartcardDialog.h"
#include "AuthSmartcardConfiguration.h"

#include "AuthSmartcardPlugin.h"
#include "AuthenticationManager.h"

#include "PlatformSmartcardFunctions.h"

#include "ui_AuthSmartcardDialog.h"


AuthSmartcardDialog::AuthSmartcardDialog( QWidget *parent,AuthSmartcardConfiguration* configuration ) :
	QDialog( parent ),
	ui( new Ui::AuthSmartcardDialog ),
	_configuration(configuration ),
	_serializedEntry(tr("")),
	_pin(tr(""))
{
	ui->setupUi( this );

	
	reloadCertificateList();

	QPushButton *button = ui->buttonBox->addButton(tr("Rescan"),QDialogButtonBox::ActionRole);
	
	connect(button, SIGNAL (released()), this, SLOT (reloadCertificateList()));

	updateOkButton();

	VeyonCore::enforceBranding( this );
}

QString AuthSmartcardDialog::pin() const
{
	vDebug() << "_entry";
	return _pin;
}

AuthSmartcardDialog::~AuthSmartcardDialog()
{
	delete ui;
}

QString AuthSmartcardDialog::serializedEntry() const
{
	vDebug() << "_entry";
	return _serializedEntry;
}




void AuthSmartcardDialog::accept()
{
	vDebug() << "_entry";
	setCursor(Qt::WaitCursor);
	if (ui->certificateList->count() > 0){
		if (!ui->pin->text().isEmpty()){
			bool result = false;
			if (_configuration->validatePinOnly()){
				result = VeyonCore::platform().smartcardFunctions().smartCardVerifyPin(ui->certificateList->currentData().toString(),ui->pin->text());
			} else {
				result = VeyonCore::platform().smartcardFunctions().smartCardAuthenticate(ui->certificateList->currentData().toString(),ui->pin->text());
			}

			if ( result) {
				_serializedEntry = ui->certificateList->currentData().toString();
				_pin = ui->pin->text();
				
				AuthSmartcardPlugin* plugin = (AuthSmartcardPlugin*)VeyonCore::authenticationManager().configuredPlugin();
				QString certificatePem = VeyonCore::platform().smartcardFunctions().certificatePem(_serializedEntry);
				if (!plugin->validateCertificate(certificatePem)){
					QMessageBox::critical( window(),
					tr( "Authentication error" ),
					tr( "Unable to Validate Certificate. Are Intermidiate Certificates Configured?" ) );
					setCursor(Qt::ArrowCursor);
					return;
				}
				setCursor(Qt::ArrowCursor);
				QDialog::accept();
				return;
			} else {
				QMessageBox::critical( window(),
					tr( "Authentication error" ),
					tr( "Logon failed with given Certificate and Pin. Please try again!" ) );
			}
		} else {
			QMessageBox::critical( window(),
							tr( "Authentication error" ),
							tr( "Must enter a pin. Please try again!" ) );
		}
	} else {
		QMessageBox::critical( window(),
							tr( "Authentication error" ),
							tr( "Must select a certificate. Please click Rescan and try again!" ) );

	}
	setCursor(Qt::ArrowCursor);
}

void AuthSmartcardDialog::reset()
{
	reloadCertificateList();
	
}

void AuthSmartcardDialog::updateOkButton()
{
	vDebug() << "_entry";
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( ui->certificateList->currentIndex() >= 0 && !ui->pin->text().isEmpty() );
}

void AuthSmartcardDialog::reloadCertificateList()
{
	vDebug() << "_entry";
	while (ui->certificateList->count() > 0){
		ui->certificateList->removeItem(0);
	}

	QMap<QString,QString> availableEntries = VeyonCore::platform().smartcardFunctions().smartCardCertificateIds();

	QMapIterator<QString,QString> availableEntriesIterator(availableEntries);

	while (availableEntriesIterator.hasNext()) {
		availableEntriesIterator.next();
		ui->certificateList->addItem(availableEntriesIterator.value(),availableEntriesIterator.key());		
	}
	

}



