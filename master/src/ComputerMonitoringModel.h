/*
 * ComputerSortFilterProxyModel.h - header file for ComputerSortFilterProxyModel
 *
 * Copyright (c) 2018-2023 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QSortFilterProxyModel>

#include "ComputerControlInterface.h"

class ComputerControlListModel;

class ComputerMonitoringModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit ComputerMonitoringModel( ComputerControlListModel* sourceModel, QObject* parent );

	int stateRole() const
	{
		return m_stateRole;
	}

	void setStateRole( int role );

	int userLoginNameRole() const
	{
		return m_userLoginNameRole;
	}

	void setUserLoginNameRole( int role );

	int groupsRole() const
	{
		return m_groupsRole;
	}

	void setGroupsRole( int role );

	ComputerControlInterface::State stateFilter() const
	{
		return m_stateFilter;
	}

	void setStateFilter( ComputerControlInterface::State state );

	bool filterNonEmptyUserLoginNames() const
	{
		return m_filterNonEmptyUserLoginNames;
	}

	void setFilterNonEmptyUserLoginNames( bool enabled );

	const QSet<QString>& groupsFilter() const
	{
		return m_groupsFilter;
	}

	void setGroupsFilter( const QStringList& groups );

protected:
	bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const override;

private:
	int m_stateRole{-1};
	int m_userLoginNameRole{-1};
	int m_groupsRole{-1};
	ComputerControlInterface::State m_stateFilter{ComputerControlInterface::State::None};
	bool m_filterNonEmptyUserLoginNames{false};
	QSet<QString> m_groupsFilter;

};
