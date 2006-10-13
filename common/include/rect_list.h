/*
 * rect_list.h - replacement for QRegion for storing lot of rectangles and
 *               extracting a subset of non-overlapping rectangles
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *  
 * This file is part of iTALC - http://italc.sourceforge.net
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


#ifndef _RECT_LIST_H
#define _RECT_LIST_H

#include <QtCore/QList>
#include <QtCore/QRect>


typedef QList<QRect> rectListContainerBase;


class rectList : public rectListContainerBase
{
public:
	rectList( rectListContainerBase _l = rectListContainerBase() ) :
		rectListContainerBase( _l )
	{
	}

	rectList( const QRect & _r ) :
		rectListContainerBase()
	{
		append( _r );
	}

	rectList nonOverlappingRects( void ) const;

	QRect boundingRect( void ) const;


private:
	const rectList & tryMerge( void );

	bool intersects( const rectList & _other, int & i1, int & i2 );

} ;


#endif


