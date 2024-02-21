/*
 * ComputerMonitoringModel.cpp - implementation of ComputerMonitoringModel
 *
 * Copyright (c) 2018-2024 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


ComputerMonitoringModel::ComputerMonitoringModel( QObject* parent ) :
	QSortFilterProxyModel( parent )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif

	setFilterCaseSensitivity( Qt::CaseInsensitive );
	setUserLoginNameRole( ComputerControlListModel::UserLoginNameRole );
}



void ComputerMonitoringModel::setStateRole( int role )
{
	beginResetModel();
	m_stateRole = role;
	endResetModel();
}



void ComputerMonitoringModel::setUserLoginNameRole( int role )
{
	beginResetModel();
	m_userLoginNameRole = role;
	endResetModel();
}



void ComputerMonitoringModel::setStateFilter( ComputerControlInterface::State state )
{
	beginResetModel();
	m_stateFilter = state;
	endResetModel();
}



void ComputerMonitoringModel::setFilterNonEmptyUserLoginNames( bool enabled )
{
	if( enabled != m_filterNonEmptyUserLoginNames )
	{
		beginResetModel();
		m_filterNonEmptyUserLoginNames = enabled;
		endResetModel();
	}
}



bool ComputerMonitoringModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
	if( m_stateFilter != ComputerControlInterface::State::None &&
		m_stateRole >= 0 &&
		sourceModel()->data( sourceModel()->index( sourceRow, 0, sourceParent ),
							 m_stateRole ).value<ComputerControlInterface::State>() != m_stateFilter )
	{
		return false;
	}

	if( m_filterNonEmptyUserLoginNames &&
		userLoginNameRole() >= 0 &&
		sourceModel()->data( sourceModel()->index( sourceRow, 0, sourceParent ),
							 m_userLoginNameRole ).toString().isEmpty() )
	{
		return false;
	}

	return QSortFilterProxyModel::filterAcceptsRow( sourceRow, sourceParent );
}
