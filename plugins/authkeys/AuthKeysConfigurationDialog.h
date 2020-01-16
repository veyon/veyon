/*
 * AuthKeysConfigurationDialog.h - header for the AuthKeysConfigurationDialog class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QDialog>

#include "AuthKeysTableModel.h"

namespace Ui {
class AuthKeysConfigurationDialog;
}

class AuthKeysConfiguration;
class AuthKeysManager;

class AuthKeysConfigurationDialog : public QDialog
{
	Q_OBJECT
public:
	AuthKeysConfigurationDialog( AuthKeysConfiguration& configuration, AuthKeysManager& manager );
	~AuthKeysConfigurationDialog() override;

	void accept() override;

private:
	void openPublicKeyBaseDir();
	void openPrivateKeyBaseDir();
	void createKeyPair();
	void deleteKey();
	void importKey();
	void exportKey();
	void setAccessGroup();
	void reloadKeyTable();

	QString selectedKey() const;
	void showResultMessage( bool success, const QString& title, const QString& message );

	Ui::AuthKeysConfigurationDialog* ui;
	AuthKeysConfiguration& m_configuration;
	AuthKeysManager& m_manager;
	AuthKeysTableModel m_authKeyTableModel;
	const QString m_keyFilesFilter;

};
