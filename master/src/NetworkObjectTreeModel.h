/*
 * NetworkObjectTreeModel.h - data model returning hierarchically grouped network objects
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "NetworkObjectModel.h"

class NetworkObjectDirectory;

class NetworkObjectTreeModel : public NetworkObjectModel
{
	Q_OBJECT
public:
	NetworkObjectTreeModel( NetworkObjectDirectory* directory, QObject *parent = nullptr);

	QModelIndex index( int row, int column,
					   const QModelIndex& parent = QModelIndex() ) const override;
	QModelIndex parent( const QModelIndex& index ) const override;

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex& parent = QModelIndex() ) const override;

	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;

	bool hasChildren( const QModelIndex& parent ) const override;
	bool canFetchMore( const QModelIndex& parent ) const override;
	void fetchMore( const QModelIndex& parent ) override;


private:
	void beginInsertObjects( const NetworkObject& parent, int index, int count );
	void endInsertObjects();

	void beginRemoveObjects( const NetworkObject& parent, int index, int count );
	void endRemoveObjects();

	void updateObject( const NetworkObject& parent, int index );

	QModelIndex objectIndex( NetworkObject::ModelId object, int column = 0 ) const;
	const NetworkObject& object( const QModelIndex& index ) const;

	NetworkObjectDirectory* m_directory;

};
