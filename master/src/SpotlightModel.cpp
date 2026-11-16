// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SpotlightModel.h"


SpotlightModel::SpotlightModel(QAbstractItemModel* sourceModel, QObject* parent) :
	QSortFilterProxyModel(parent)
{
	setSourceModel(sourceModel);
}



void SpotlightModel::setIconSize(QSize size)
{
	m_iconSize = size;

	Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0), {Qt::DisplayRole, Qt::DecorationRole});
}



void SpotlightModel::setUpdateInRealtime(bool enabled)
{
	m_updateInRealtime = enabled;

	for (const auto& controlInterface : std::as_const(m_controlInterfaces))
	{
		controlInterface->setUpdateMode(m_updateInRealtime ? ComputerControlInterface::UpdateMode::Live : ComputerControlInterface::UpdateMode::Monitoring);
	}
}



void SpotlightModel::add(const ComputerControlInterface::Pointer& controlInterface)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	beginFilterChange();
#endif

	m_controlInterfaces.append(controlInterface);

	controlInterface->setUpdateMode(m_updateInRealtime ? ComputerControlInterface::UpdateMode::Live : ComputerControlInterface::UpdateMode::Monitoring);

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
	endFilterChange(Direction::Rows);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	invalidateRowsFilter();
#else
	invalidateFilter();
#endif
}



void SpotlightModel::remove(const ComputerControlInterface::Pointer& controlInterface)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	beginFilterChange();
#endif

	m_controlInterfaces.removeAll(controlInterface);

	controlInterface->setUpdateMode(ComputerControlInterface::UpdateMode::Monitoring);

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
	endFilterChange(Direction::Rows);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	invalidateRowsFilter();
#else
	invalidateFilter();
#endif
}



QVariant SpotlightModel::data(const QModelIndex& index, int role) const
{
	const auto sourceIndex = mapToSource(index);
	if (sourceIndex.isValid() == false)
	{
		return {};
	}

	if (role == Qt::DecorationRole)
	{
		auto framebuffer = sourceModel()->data(sourceIndex, ComputerListModel::FramebufferRole).value<QImage>();
		if (framebuffer.isNull())
		{
			framebuffer = sourceModel()->data(sourceIndex, Qt::DecorationRole).value<QImage>();
		}

		return framebuffer.scaled(m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	return QSortFilterProxyModel::data(index, role);
}



bool SpotlightModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	return m_controlInterfaces.contains(sourceModel()->data(sourceModel()->index(sourceRow, 0, sourceParent),
															ComputerListModel::ControlInterfaceRole)
											.value<ComputerControlInterface::Pointer>());
}
