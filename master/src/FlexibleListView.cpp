/*
 * FlexibleListView.cpp - list view with flexible icon positions
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@users.sf.net>
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

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include "FlexibleListView.h"


FlexibleListView::FlexibleListView( QWidget* parent ) :
	QListView( parent ),
	m_uidRole( Qt::UserRole )
{
	connect( this, &QListView::indexesMoved, this, &FlexibleListView::updatePositions );
}



FlexibleListView::~FlexibleListView()
{
}



void FlexibleListView::setUidRole( int role )
{
	m_uidRole = role;
}



void FlexibleListView::setFlexible( bool enabled )
{
	if( enabled )
	{
		setMovement( QListView::Free );
	}
	else
	{
		setMovement( QListView::Static );
	}

	doItemsLayout();
}



bool FlexibleListView::flexible() const
{
	return movement() == QListView::Free;
}



void FlexibleListView::alignToGrid()
{
	auto m = model();

	const QSizeF gridSize = effectiveGridSize();

	for( int i = 0, count = m->rowCount(); i < count; ++i )
	{
		const auto index = m->index( i, 0 );
		const auto uid = m->data( index, m_uidRole ).toUuid();

		if( uid.isNull() == false && m_positions.contains( uid ) )
		{
			m_positions[uid] = QPoint( static_cast<int>( qRound( m_positions[uid].x() / gridSize.width() ) * gridSize.width() ),
									   static_cast<int>( qRound( m_positions[uid].y() / gridSize.height() ) * gridSize.height() ) );
			setPositionForIndex( m_positions[uid], index );
		}
	}
}



QJsonArray FlexibleListView::savePositions()
{
	QJsonArray data;

	for( auto it = m_positions.constBegin(), end = m_positions.constEnd(); it != end; ++it )
	{
		QJsonObject object;
		object[QStringLiteral("uid")] = it.key().toString();
		object[QStringLiteral("x")] = it.value().x();
		object[QStringLiteral("y")] = it.value().y();
		data += object;
	}

	return data;
}



void FlexibleListView::loadPositions( const QJsonArray& data )
{
	m_positions.clear();

	for( const auto& item : data )
	{
		const auto object = item.toObject();
		const QUuid uid( object["uid"].toString() );
		if( uid.isNull() == false )
		{
			m_positions[uid] = QPoint( object["x"].toInt(), object["y"].toInt() );
		}
	}
}



void FlexibleListView::doItemsLayout()
{
	QListView::doItemsLayout();

	if( movement() == QListView::Free )
	{
		restorePositions();
	}
}



void FlexibleListView::restorePositions()
{
	auto m = model();

	if( m == nullptr )
	{
		return;
	}

	for( int i = 0, count = m->rowCount(); i < count; ++i )
	{
		const auto index = m->index( i, 0 );
		const auto uid = m->data( index, m_uidRole ).toUuid();

		if( uid.isNull() == false && m_positions.contains( uid ) )
		{
			setPositionForIndex( m_positions[uid], index );
		}
	}
}



void FlexibleListView::updatePositions()
{
	if( movement() == QListView::Free && model() )
	{
		auto m = model();

		for( int i = 0, count = m->rowCount(); i < count; ++i )
		{
			const auto index = m->index( i, 0 );
			const auto uid = m->data( index, m_uidRole ).toUuid();

			if( uid.isNull() == false )
			{
				m_positions[uid] = rectForIndex( index ).topLeft();
			}
		}
	}
}



QSizeF FlexibleListView::effectiveGridSize() const
{
	auto m = model();

	if( m && m->rowCount() > 0 )
	{
		return rectForIndex( m->index( 0, 0 ) ).size();
	}
	else if( iconSize().isEmpty() == false )
	{
		return iconSize();
	}

	return QSizeF( 1, 1 );
}
