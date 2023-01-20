// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

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
