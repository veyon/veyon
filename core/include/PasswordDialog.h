/*
 * PasswordDialog.h - declaration of password dialog
 *
 * Copyright (c) 2010-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "AuthenticationCredentials.h"

#include <QDialog>

namespace Ui { class PasswordDialog; }

class VEYON_CORE_EXPORT PasswordDialog : public QDialog
{
	Q_OBJECT
public:
	explicit PasswordDialog( QWidget *parent );
	~PasswordDialog() override;

	QString username() const;
	CryptoCore::PlaintextPassword password() const;

	AuthenticationCredentials credentials() const;

	void accept() override;

private Q_SLOTS:
	void updateOkButton();


private:
	Ui::PasswordDialog *ui;

} ;
