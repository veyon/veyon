/*
 * AuthKeysTableModel.h - declaration of AuthKeysTableModel class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QAbstractTableModel>

class AuthKeysManager;

class AuthKeysTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum Columns {
		ColumnKeyName,
		ColumnKeyType,
		ColumnKeyPairID,
		ColumnAccessGroup,
		ColumnCount
	};

	explicit AuthKeysTableModel( QObject* parent = nullptr );
	~AuthKeysTableModel() override;

	void reload();

	const QString& key( int row ) const
	{
		return m_keys[row];
	}

	int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
	QVariant data( const QModelIndex& index, int role ) const override;
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

private:
	AuthKeysManager* m_manager;
	QStringList m_keys;

};
