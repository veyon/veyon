/*
 * NetworkObjectOverlayDataModel.h - overlay model for NetworkObjectModel to provide extra data
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

#ifndef NETWORK_OBJECT_OVERLAY_DATA_MODEL_H
#define NETWORK_OBJECT_OVERLAY_DATA_MODEL_H

#include <QIdentityProxyModel>

#include "NetworkObject.h"

class NetworkObjectOverlayDataModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	NetworkObjectOverlayDataModel( int overlayDataColumn,
								   int overlayDataRole,
								   const QVariant& overlayDataColumnHeaderData,
								   QObject *parent = nullptr );

	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role) override;


private:
	int m_overlayDataColumn;
	int m_overlayDataRole;
	QVariant m_overlayDataColumnHeaderData;
	QHash<NetworkObject::Uid, QVariant> m_overlayData;

};

#endif // NETWORK_OBJECT_OVERLAY_DATA_MODEL_H
