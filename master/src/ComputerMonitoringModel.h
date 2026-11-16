// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSortFilterProxyModel>

#include "ComputerControlInterface.h"

class ComputerMonitoringModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit ComputerMonitoringModel(QObject* parent);

	int stateRole() const
	{
		return m_stateRole;
	}

	void setStateRole(int role);

	int userLoginNameRole() const
	{
		return m_userLoginNameRole;
	}

	void setUserLoginNameRole(int role);

	ComputerControlInterface::State stateFilter() const
	{
		return m_stateFilter;
	}

	void setStateFilter(ComputerControlInterface::State state);

	bool filterNonEmptyUserLoginNames() const
	{
		return m_filterNonEmptyUserLoginNames;
	}

	void setFilterNonEmptyUserLoginNames(bool enabled);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
	int m_stateRole{-1};
	int m_userLoginNameRole{-1};
	ComputerControlInterface::State m_stateFilter{ComputerControlInterface::State::None};
	bool m_filterNonEmptyUserLoginNames{false};
};
