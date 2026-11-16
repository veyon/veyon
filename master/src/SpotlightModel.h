// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSortFilterProxyModel>

#include "ComputerControlListModel.h"

class SpotlightModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	static constexpr auto ControlInterfaceRole = ComputerControlListModel::ControlInterfaceRole;

	SpotlightModel(QAbstractItemModel* sourceModel, QObject* parent = nullptr);

	void setIconSize(QSize size);
	void setUpdateInRealtime(bool enabled);

	void add(const ComputerControlInterface::Pointer& controlInterface);
	void remove(const ComputerControlInterface::Pointer& controlInterface);

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
	QSize m_iconSize;
	bool m_updateInRealtime{false};

	ComputerControlInterfaceList m_controlInterfaces;
};
