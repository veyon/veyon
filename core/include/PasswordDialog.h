/*
 * PasswordDialog.h - declaration of password dialog
 *
 * Copyright (c) 2010-2016 Tobias Junghans <tobydox@veyon.io>
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

#ifndef PASSWORD_DIALOG_H
#define PASSWORD_DIALOG_H

#include "AuthenticationCredentials.h"

#include <QDialog>

namespace Ui { class PasswordDialog; }

class VEYON_CORE_EXPORT PasswordDialog : public QDialog
{
	Q_OBJECT
public:
	PasswordDialog( QWidget *parent );
	~PasswordDialog() override;

	QString username() const;
	QString password() const;

	AuthenticationCredentials credentials() const;

	void accept() override;

private slots:
	void updateOkButton();


private:
	Ui::PasswordDialog *ui;

} ;

#endif
