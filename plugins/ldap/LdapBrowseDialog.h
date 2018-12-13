/*
 * LdapBrowseDialog.h - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2018 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"

namespace KLDAP {
class LdapModel;
};

namespace Ui {
class LdapBrowseDialog;
}

class QSortFilterProxyModel;
class LdapClient;
class LdapConfiguration;

class LdapBrowseDialog : public QDialog
{
	Q_OBJECT
public:
	LdapBrowseDialog( const LdapConfiguration& ldapConfiguration, QWidget* parent = nullptr );
	~LdapBrowseDialog();

private:
	void setModels();

	LdapClient* m_ldapClient;

	Ui::LdapBrowseDialog* ui;

	KLDAP::LdapModel* m_ldapModel;
	QSortFilterProxyModel* m_sortFilterProxyModel;

};
