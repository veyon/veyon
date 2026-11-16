// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#endif


ComputerMonitoringModel::ComputerMonitoringModel(QObject* parent) :
	QSortFilterProxyModel(parent)
{
#if defined(QT_TESTLIB_LIB)
	new QAbstractItemModelTester(this, QAbstractItemModelTester::FailureReportingMode::Warning, this);
#endif

	setFilterCaseSensitivity(Qt::CaseInsensitive);
	setUserLoginNameRole(ComputerControlListModel::UserLoginNameRole);
}



void ComputerMonitoringModel::setStateRole(int role)
{
	beginResetModel();
	m_stateRole = role;
	endResetModel();
}



void ComputerMonitoringModel::setUserLoginNameRole(int role)
{
	beginResetModel();
	m_userLoginNameRole = role;
	endResetModel();
}



void ComputerMonitoringModel::setStateFilter(ComputerControlInterface::State state)
{
	beginResetModel();
	m_stateFilter = state;
	endResetModel();
}



void ComputerMonitoringModel::setFilterNonEmptyUserLoginNames(bool enabled)
{
	if (enabled != m_filterNonEmptyUserLoginNames)
	{
		beginResetModel();
		m_filterNonEmptyUserLoginNames = enabled;
		endResetModel();
	}
}



bool ComputerMonitoringModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	if (m_stateFilter != ComputerControlInterface::State::None &&
		m_stateRole >= 0 &&
		sourceModel()->data(sourceModel()->index(sourceRow, 0, sourceParent),
							m_stateRole)
				.value<ComputerControlInterface::State>() != m_stateFilter)
	{
		return false;
	}

	if (m_filterNonEmptyUserLoginNames &&
		userLoginNameRole() >= 0 &&
		sourceModel()->data(sourceModel()->index(sourceRow, 0, sourceParent),
							m_userLoginNameRole)
			.toString()
			.isEmpty())
	{
		return false;
	}

	return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
