/*
 * CheckableItemProxyModel.h - proxy model for overlaying checked property
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonArray>
#include <QIdentityProxyModel>
#include <QUuid>

#include "QtCompat.h"

class CheckableItemProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	explicit CheckableItemProxyModel( int uidRole, QObject *parent = nullptr );

	void setException( int exceptionRole, const QVariant& exceptionFilterData );

	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	void updateNewRows(const QModelIndex &parent, int first, int last);

	QJsonArray saveStates();
	void loadStates( const QJsonArray& data );

private:
	QUuid indexToUuid( const QModelIndex& index ) const;
	bool setChildData( const QModelIndex &index, Qt::CheckState checkState );
	void setParentData( const QModelIndex &index, Qt::CheckState checkState );

	Qt::CheckState checkStateFromVariant( const QVariant& data ) const
	{
		return QVariantHelper<Qt::CheckState>::value( data );
	}

	int m_uidRole;
	int m_exceptionRole;
	QVariant m_exceptionData;
	QHash<QUuid, Qt::CheckState> m_checkStates;

};
