/*
 * FeatureListModel.cpp - data model for features
 *
 * Copyright (c) 2019-2022 Tobias Junghans <tobydox@veyon.io>
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

#include "FeatureListModel.h"
#include "VeyonMaster.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


FeatureListModel::FeatureListModel( VeyonMaster* masterCore, QObject* parent ) :
	QAbstractListModel( parent ),
	m_master( masterCore )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
}



int FeatureListModel::rowCount( const QModelIndex& parent ) const
{
	if( parent.isValid() )
	{
		return 0;
	}

	return m_master->features().count();
}



QVariant FeatureListModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return {};
	}

	if( index.row() >= m_master->features().count() )
	{
		vCritical() << "index out of range!";
	}

	const auto feature = m_master->features()[index.row()];

	switch( DataRole(role) )
	{
	case DataRole::Name: return feature.name();
	case DataRole::DisplayName: return feature.displayName();
	case DataRole::DisplayNameActive: return feature.displayNameActive();
	case DataRole::IconUrl: return QString{ QStringLiteral("qrc") + feature.iconUrl() };
	case DataRole::Description: return feature.description();
	case DataRole::Uid: return feature.uid();
	}

	return {};
}



QHash<int, QByteArray> FeatureListModel::roleNames() const
{
	return {
		{ int(DataRole::Name), "name" },
		{ int(DataRole::DisplayName), "displayName" },
		{ int(DataRole::DisplayNameActive), "displayNameActive" },
		{ int(DataRole::IconUrl), "iconUrl" },
		{ int(DataRole::Description), "description" },
		{ int(DataRole::Uid), "uid" },
	};
}
