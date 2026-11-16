// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkObjectModel.h"
#include "NetworkObjectOverlayDataModel.h"

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#include <QStandardItemModel>
#endif


NetworkObjectOverlayDataModel::NetworkObjectOverlayDataModel(const QStringList& overlayDataHeaders, QObject* parent) :
	KExtraColumnsProxyModel(parent),
	m_overlayDataRole(Qt::DisplayRole)
{
#if defined(QT_TESTLIB_LIB)
	// base class relies on source model being set when tested by QAbstractItemModelTester
	setSourceModel(new QStandardItemModel(this));
	new QAbstractItemModelTester(this, QAbstractItemModelTester::FailureReportingMode::Warning, this);
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



QVariant NetworkObjectOverlayDataModel::extraColumnData(const QModelIndex& parent, int row, int extraColumn, int role) const
{
	if (role != m_overlayDataRole)
	{
		return {};
	}

	const auto networkObjectUid = lookupNetworkObjectId(parent, row);

	if (networkObjectUid.isNull() == false && m_extraColumnsData.contains(networkObjectUid))
	{
		return m_extraColumnsData[networkObjectUid].value(extraColumn);
	}

	return {};
}



bool NetworkObjectOverlayDataModel::setExtraColumnData(const QModelIndex& parent, int row, int extraColumn, const QVariant& data, int role)
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
		if (extraColumn >= rowData.count())
		{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			rowData.resize(extraColumn + 1);
#else
			std::fill_n(std::back_inserter(rowData), extraColumn + 1 - rowData.size(), QVariant{});
#endif
		}
		rowData[extraColumn] = data;
		m_extraColumnsData[networkObjectUid] = rowData;
		extraColumnDataChanged(parent, row, extraColumn, {m_overlayDataRole});
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
