/*
 * ComputerSortFilterProxyModel.h - header file for ComputerSortFilterProxyModel
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef COMPUTER_SORT_FILTER_PROXY_MODEL_H
#define COMPUTER_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

#include "ComputerControlInterface.h"

class ComputerSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	ComputerSortFilterProxyModel( QObject* parent );

	int stateRole() const
	{
		return m_stateRole;
	}

	void setStateRole( int role );

	ComputerControlInterface::State stateFilter() const
	{
		return m_stateFilter;
	}

	void setStateFilter( ComputerControlInterface::State state );

protected:
	bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const override;

private:
	int m_stateRole;
	ComputerControlInterface::State m_stateFilter;

};

#endif // COMPUTER_SORT_FILTER_PROXY_MODEL_H
