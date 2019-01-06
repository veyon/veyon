/*
 * CheckableItemProxyModel.h - proxy model for overlaying checked property
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CHECKABLE_ITEM_PROXY_MODEL_H
#define CHECKABLE_ITEM_PROXY_MODEL_H

#include <QJsonArray>
#include <QIdentityProxyModel>
#include <QUuid>

class CheckableItemProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	CheckableItemProxyModel( int uidRole, QObject *parent = nullptr );

	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	void updateNewRows(const QModelIndex &parent, int first, int last);
	void removeRowStates(const QModelIndex &parent, int first, int last);

	QJsonArray saveStates();
	void loadStates( const QJsonArray& data );

private:
	Qt::CheckState checkStateFromVariant( const QVariant& data )
	{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		// work around broken conversion for Q_ENUMs in QVariant of Qt 5.5
		return static_cast<Qt::CheckState>( data.toInt() );
#else
		return data.value<Qt::CheckState>();
#endif
	}

	int m_uidRole;
	QHash<QUuid, Qt::CheckState> m_checkStates;
	int m_callDepth;

};

#endif // CHECKABLE_ITEM_PROXY_MODEL_H
