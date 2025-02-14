/*
 * NetworkObjectOverlayDataModel.cpp - overlay model for NetworkObjectModel to provide extra data
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#include <QStandardItemModel>
#endif


NetworkObjectOverlayDataModel::NetworkObjectOverlayDataModel(const QStringList& overlayDataHeaders, QObject *parent) :
	KExtraColumnsProxyModel( parent ),
	m_overlayDataRole( Qt::DisplayRole )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	// base class relies on source model being set when tested by QAbstractItemModelTester
	setSourceModel( new QStandardItemModel( this ) );
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
	for (const auto& header : overlayDataHeaders)
	{
		appendColumn(header);
	}
}



QVariant NetworkObjectOverlayDataModel::data(const QModelIndex& index, int role) const
{
	if (index.column() > 0)
	{
		return KExtraColumnsProxyModel::data(index, role);
	}

	const auto networkObjectUid = lookupNetworkObjectId(index);
	if (m_overlayData.contains(networkObjectUid))
	{
		const auto& overlayData = std::as_const(m_overlayData)[networkObjectUid];
		if (overlayData.contains(role))
		{
			const auto value = std::as_const(overlayData)[role];
			if (value.isValid())
			{
				return value;
			}
		}
	}

	return KExtraColumnsProxyModel::data(index, role);
}



bool NetworkObjectOverlayDataModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (index.column() > 0)
	{
		return KExtraColumnsProxyModel::setData(index, value, role);
	}

	const auto networkObjectUid = lookupNetworkObjectId(index);
	if (networkObjectUid.isNull())
	{
		return false;
	}

	auto& overlayData = m_overlayData[networkObjectUid]; // clazy:exclude=detaching-member
	if (overlayData.value(role) != value)
	{
		overlayData[role] = value;
		Q_EMIT dataChanged(index, index, {role});
	}

	return true;
}



QVariant NetworkObjectOverlayDataModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
	if (role != m_overlayDataRole)
	{
		return {};
	}

	const auto networkObjectUid = lookupNetworkObjectId(parent, row);

	if( networkObjectUid.isNull() == false && m_extraColumnsData.contains( networkObjectUid ) )
	{
		return m_extraColumnsData[networkObjectUid].value(extraColumn);
	}

	return {};
}



bool NetworkObjectOverlayDataModel::setExtraColumnData( const QModelIndex &parent, int row, int extraColumn, const QVariant &data, int role)
{
	if (role != m_overlayDataRole)
	{
		return false;
	}

	const auto networkObjectUid = lookupNetworkObjectId(parent, row);
	auto& rowData = m_extraColumnsData[networkObjectUid]; // clazy:exclude=detaching-member

	if (extraColumn >= rowData.count() ||
		rowData[extraColumn] != data)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		rowData.resize(extraColumn+1);
#else
		std::fill_n(std::back_inserter(rowData), extraColumn + 1 - rowData.size(), QVariant{});
#endif
		rowData[extraColumn] = data;
		m_extraColumnsData[networkObjectUid] = rowData;
		extraColumnDataChanged( parent, row, extraColumn, { m_overlayDataRole } );
	}

	return true;
}



NetworkObject::Uid NetworkObjectOverlayDataModel::lookupNetworkObjectId(const QModelIndex& index) const
{
	if (index.isValid() == false)
	{
		return {};
	}

	return KExtraColumnsProxyModel::data(index, NetworkObjectModel::UidRole).toUuid();
}



NetworkObject::Uid NetworkObjectOverlayDataModel::lookupNetworkObjectId(const QModelIndex& parent, int row) const
{
	return lookupNetworkObjectId(index(row, 0, parent));
}
