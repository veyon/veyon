/*
 * rect_list.cpp - replacement for QRegion for storing lot of rectangles and
 *                 extracting a subset of non-overlapping rectangles
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


#include "rect_list.h"


rectList rectList::nonOverlappingRects( void ) const
{
	if( isEmpty() || size() == 1 )
	{
		return( *this );
	}

	if( size() == 2 )
	{
		QRect r1 = at( 0 );
		QRect r2 = at( 1 );
		// does one rect contain the other?
		if( r1 == r2 || r1.contains( r2, TRUE ) )
		{
			return( r1 );
		}
		else if( r2.contains( r1, TRUE ) )
		{
			return( r2 );
		}
		// or is there no intersection at all?
		else if( !r1.intersects( r2 ) )
		{
			return( *this );
		}

		// ok, we have an intersection, so try to do something
		// with it
		const QRect is = r1.intersect( r2 );
		for( int i = 0; i < 2; ++i )
		{
			if( is.width() == r1.width() )
			{
				if( is.y() == r1.y() )
				{
					rectList r( r2 );
					r += QRect( r1.x(), is.y()+is.height(),
								r1.width(),
						r1.height()-is.height() );
					return( r );
				}
				else if( is.bottom() == r1.bottom() )
				{
					rectList r( r2 );
					r += QRect( r1.x(), r1.y(), r1.width(),
						r1.height()-is.height() );
					return( r );
				}
			}

			if( is.height() == r1.height() )
			{
				if( is.x() == r1.x() )
				{
					rectList r( r2 );
					r += QRect( is.x()+is.width(),
							r1.y(),
							r1.width()-is.width(),
							r1.height() );
					return( r );
				}
				if( is.right() == r1.right() )
				{
					rectList r( r2 );
					r += QRect( r1.x(), r1.y(),
							r1.width()-is.width(),
								r1.height() );
					return( r );
				}
			}
		}

		rectList r( r1 );
		int xadd = 0;
		if( is.x() == r2.x() )
		{
			xadd = is.width();
		}

		// rect2 above intersection?
		if( is.y() > r2.y()  )
		{
			r += QRect( r2.x(), r2.y(), r2.width(),
							is.top() - r2.top() );
			r += QRect( r2.x()+xadd,
					r2.y()+r2.height()-is.height(),
					r2.width()-xadd, is.height() );
			if( r2.bottom() > is.bottom() )
			{
				r += QRect( r2.x(), r1.bottom()+1,
						r2.width(),
						r2.bottom()-is.bottom() );
			}
		}
		else if( is.y() == r2.y() && is.height() == r2.height() &&
							is.x() > r2.x() )
		{
			r += QRect( r2.x(), r2.y(), is.x()- r2.x(),
								r2.height() );
			r += QRect( is.right()+1, r2.y(), r2.right()-is.right(),
								r2.height() );
		}
		else
		{
			if( r2.height() > is.height() )
			{
				r += QRect( r2.x(), r2.y()+is.height(),
						r2.width(),
						r2.height()-is.height() );
			}
			r += QRect( r2.x()+xadd, r2.y(), r2.width()-xadd,
								is.height() );
		}

		return( r );
	}

	int i1, i2, n = ( size()+1 ) / 2;
	rectList rl1 = rectList( mid( 0, n ) ).nonOverlappingRects();
	rectList rl2 = rectList( mid( n ) ).nonOverlappingRects();
	if( rl1.intersects( rl2, i1, i2 ) )
	{
		QRect r1 = rl1.at( i1 ); rl1.removeAt( i1 );
		QRect r2 = rl2.at( i2 ); rl2.removeAt( i2 );
		return( rectList( rl1 + rl2 +
				rectList( rectList( r1 ) << r2 ).
					nonOverlappingRects() ).
						nonOverlappingRects() );
	}
	return( rectList( rl1+rl2 ).tryMerge() );
}



const rectList & rectList::tryMerge( void )
{
	for( int i = 0; i < size()-1; ++i )
	{
		if( at( i ).width() * at( i ).height() == 0 )
		{
			removeAt( i );
			i = -1;
			continue;
		}
		for( int j = i+1; j < size(); ++j )
		{
			const QRect r1 = at( i );
			const QRect r2 = at( j );
			const QRect r = r1.unite( r2 );
			if( r1.width()*r1.height() + r2.width()*r2.height() ==
							r.width()*r.height() )
			{
				replace( i, r );
				removeAt( j );
				i = -1;
				break;
			}
		}
	}
	return( *this );
}



bool rectList::intersects( const rectList & _other, int & i1, int & i2 )
{
	i1 = 0;
	foreach( const QRect r1, *this )
	{
		i2 = 0;
		foreach( const QRect r2, _other )
		{
			if( r1.intersects( r2 ) )
			{
				return( TRUE );
			}
			++i2;
		}
		++i1;
	}
	return( FALSE );
}




QRect rectList::boundingRect( void ) const
{
	if( isEmpty() )
	{
		return( QRect() );
	}
	QRect r = at( 0 );
	foreach( const QRect cr, *this )
	{
		r = r.unite( cr );
	}
	return( r );
}

