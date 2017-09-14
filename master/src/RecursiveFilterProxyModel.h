/*
 * RecursiveFilterProxyModel.h - proxy model for recursive filtering
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef RECURSIVE_FILTER_PROXY_MODEL_H
#define RECURSIVE_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class RecursiveFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	RecursiveFilterProxyModel( QObject* parent = nullptr );

	bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const override;

};

#endif // RECURSIVE_FILTER_PROXY_MODEL_H
