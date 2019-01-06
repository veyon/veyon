/*
 * NetworkObjectOverlayDataModel.cpp - overlay model for NetworkObjectModel to provide extra data
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

#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#include <QStandardItemModel>
#endif


NetworkObjectOverlayDataModel::NetworkObjectOverlayDataModel( const QString& overlayDataHeader,
															  QObject *parent ) :
	KExtraColumnsProxyModel( parent ),
	m_overlayDataRole( Qt::DisplayRole )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	// base class relies on source model being set when tested by QAbstractItemModelTester
	setSourceModel( new QStandardItemModel( this ) );
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
	appendColumn( overlayDataHeader );
}



QVariant NetworkObjectOverlayDataModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
	if( extraColumn != 0 || role != m_overlayDataRole )
	{
		return QVariant();
	}

	NetworkObject::Uid networkObjectUid = data( index( row, 0, parent ), NetworkObjectModel::UidRole ).toUuid();

	if( networkObjectUid.isNull() == false && m_overlayData.contains( networkObjectUid ) )
	{
		return m_overlayData[networkObjectUid];
	}

	return QVariant();
}


bool NetworkObjectOverlayDataModel::setExtraColumnData( const QModelIndex &parent, int row, int extraColumn, const QVariant &data, int role)
{
	if( extraColumn != 0 || role != m_overlayDataRole )
	{
		return false;
	}

	const auto networkObjectUid = KExtraColumnsProxyModel::data( index( row, 0, parent ), NetworkObjectModel::UidRole ).toUuid();

	if( m_overlayData[networkObjectUid] != data )
	{
		m_overlayData[networkObjectUid] = data;
		extraColumnDataChanged( parent, row, extraColumn, { m_overlayDataRole } );
	}

	return true;
}
