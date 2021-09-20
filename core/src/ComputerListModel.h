/*
 * ComputerListModel.h - data model base class for computer objects
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
		StateRole,
		ImageIdRole,
		GroupsRole,
		ScreenRole,
		ControlInterfaceRole
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

	enum class AspectRatio {
		Auto,
		AR16_9,
		AR16_10,
		AR3_2,
		AR4_3
	};
	Q_ENUM(AspectRatio)

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

	AspectRatio aspectRatio() const
	{
		return m_aspectRatio;
	}

private:
	DisplayRoleContent m_displayRoleContent;
	SortOrder m_sortOrder;
	AspectRatio m_aspectRatio;

};
