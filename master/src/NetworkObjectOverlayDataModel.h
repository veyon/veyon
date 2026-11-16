// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "KExtraColumnsProxyModel.h"
#include "NetworkObject.h"

class NetworkObjectOverlayDataModel : public KExtraColumnsProxyModel
{
	Q_OBJECT
public:
	explicit NetworkObjectOverlayDataModel(const QStringList& overlayDataHeaders,
										   QObject* parent = nullptr);

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::DisplayRole) override;

	QVariant extraColumnData(const QModelIndex& parent, int row, int extraColumn, int role = Qt::DisplayRole) const override;

	bool setExtraColumnData(const QModelIndex& parent, int row, int extraColumn, const QVariant& data, int role = Qt::DisplayRole) override;


private:
	NetworkObject::Uid lookupNetworkObjectId(const QModelIndex& index) const;
	NetworkObject::Uid lookupNetworkObjectId(const QModelIndex& parent, int row) const;

	int m_overlayDataRole;
	QHash<NetworkObject::Uid, QMap<int, QVariant>> m_overlayData;
	QHash<NetworkObject::Uid, QList<QVariant>> m_extraColumnsData;
};
