/*
 * AuthSmardcardDialog.h - declaration of Smartcard dialog
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

#pragma once

//#include "AuthenticationCredentials.h"

#include <QDialog>
#include <QtCrypto>

#include "VeyonCore.h"
#include "AuthSmartcardConfiguration.h"

namespace Ui { class AuthSmartcardDialog; }

class AuthSmartcardDialog : public QDialog
{
	Q_OBJECT
public:
	AuthSmartcardDialog( QWidget *parent, AuthSmartcardConfiguration* configuration);
	~AuthSmartcardDialog() override;

	void accept() override;
	void reset();

	QString serializedEntry() const;
	QString pin() const;

private slots:
	void updateOkButton();
	void reloadCertificateList();

private:
	Ui::AuthSmartcardDialog *ui;
	AuthSmartcardConfiguration* _configuration;
	QString _serializedEntry;
	QString _pin;
} ;

