/*
 * LdapModel.h - item model for browsing LDAP directories
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QAbstractItemModel>
#include <QIcon>

class LdapClient;
class LdapConfiguration;
class LdapBrowseModelNode;

class LdapBrowseModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	using Node = LdapBrowseModelNode;

	enum Mode {
		BrowseBaseDN,
		BrowseObjects,
		BrowseAttributes
	};

	static constexpr int ItemNameRole = Qt::UserRole+1;

	LdapBrowseModel( Mode mode, const LdapConfiguration& configuration, QObject* parent = nullptr );
	~LdapBrowseModel() override;

	QModelIndex index(int row, int col, const QModelIndex &parent) const override;
	QModelIndex parent( const QModelIndex& child ) const override;
	QVariant data( const QModelIndex& index, int role ) const override;
	Qt::ItemFlags flags( const QModelIndex& index ) const override;
	int columnCount( const QModelIndex& parent ) const override;
	int rowCount( const QModelIndex& parent ) const override;
	bool hasChildren( const QModelIndex& parent ) const override;
	bool canFetchMore( const QModelIndex& parent ) const override;
	void fetchMore( const QModelIndex& parent ) override;

	QModelIndex dnToIndex( const QString& dn );

	QString baseDn() const;

private:
	void populateRoot() const;
	void populateNode( const QModelIndex& parent );

	Node* toNode( const QModelIndex& index ) const;

	const Mode m_mode;
	LdapClient* m_client;
	Node* m_root;

	QIcon m_objectIcon;
	QIcon m_ouIcon;
	QIcon m_attributeIcon;

};
