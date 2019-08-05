/*
 * ComputerListModel.h - data model base class for computer objects
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

#pragma once

#include <QAbstractListModel>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT ComputerListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum {
		UidRole = Qt::UserRole,
		StateRole
	};

	enum class DisplayRoleContent {
		UserAndComputerName,
		UserName,
		ComputerName,
	};
	Q_ENUM(DisplayRoleContent)

	enum class SortOrder {
		ComputerAndUserName,
		UserName,
		ComputerName,
	};
	Q_ENUM(SortOrder)

	explicit ComputerListModel( QObject* parent = nullptr );

	Qt::ItemFlags flags( const QModelIndex& index ) const override;

	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;

	DisplayRoleContent displayRoleContent() const
	{
		return m_displayRoleContent;
	}

	SortOrder sortOrder() const
	{
		return m_sortOrder;
	}

private:
	DisplayRoleContent m_displayRoleContent;
	SortOrder m_sortOrder;

};
