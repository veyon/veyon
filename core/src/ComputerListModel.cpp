// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ComputerListModel.h"
#include "VeyonConfiguration.h"


ComputerListModel::ComputerListModel(QObject* parent) :
	QAbstractListModel(parent),
	m_displayRoleContent(VeyonCore::config().computerDisplayRoleContent()),
	m_uidRoleContent(VeyonCore::config().computerUidRoleContent()),
	m_gridSizingMode(VeyonCore::config().computerMonitoringGridSizingMode()),
	m_visibilityMode(VeyonCore::config().computerMonitoringVisibilityMode()),
	m_sortOrder(VeyonCore::config().computerMonitoringSortOrder()),
	m_aspectRatio(VeyonCore::config().computerMonitoringAspectRatio())
{
}



Qt::ItemFlags ComputerListModel::flags(const QModelIndex& index) const
{
	auto defaultFlags = QAbstractListModel::flags(index);

	if (index.isValid())
	{
		return Qt::ItemIsDragEnabled | defaultFlags;
	}

	return Qt::ItemIsDropEnabled | defaultFlags;
}



Qt::DropActions ComputerListModel::supportedDragActions() const
{
	return Qt::MoveAction;
}



Qt::DropActions ComputerListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}
