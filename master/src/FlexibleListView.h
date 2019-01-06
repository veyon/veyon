/*
 * FlexibleListView.h - list view with flexible icon positions
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

#ifndef FLEXIBLE_LIST_VIEW_H
#define FLEXIBLE_LIST_VIEW_H

#include <QListView>

class FlexibleListView : public QListView
{
	Q_OBJECT
public:
	FlexibleListView( QWidget *parent = nullptr );
	~FlexibleListView() override;

	void setUidRole( int role );

	void setFlexible( bool enabled );
	bool flexible() const;

	void alignToGrid();

	QJsonArray savePositions();
	void loadPositions( const QJsonArray& data );

public:
	void doItemsLayout() override;

private:
	void restorePositions();
	void updatePositions();

private:
	QSizeF effectiveGridSize() const;
	QPointF toGridPoint( const QPoint& pos ) const;
	QPoint toItemPosition( const QPointF& gridPoint ) const;

	int m_uidRole;
	QHash<QUuid, QPointF> m_positions;

};

#endif // FLEXIBLE_LIST_VIEW_H
