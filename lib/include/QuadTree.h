/*
 * QuadTree.h - QuadTree, a data structure for fast fuzzy rectangle merging
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _QUAD_TREE_H
#define _QUAD_TREE_H

#include <stdint.h>
#include <QtCore/QList>
#include <QtCore/QRect>
#include <QtCore/QVector>

typedef QList<QRect> RectList;

struct QuadTreeRect
{
	QuadTreeRect( uint16_t x1 = 0, uint16_t y1 = 0, uint16_t x2 = 0, uint16_t y2 = 0 ) :
		m_x1( x1 ),
		m_x2( x2 ),
		m_y1( y1 ),
		m_y2( y2 )
	{
	}

	uint16_t x1() const
	{
		return m_x1;
	}

	uint16_t y1() const
	{
		return m_y1;
	}

	uint16_t x2() const
	{
		return m_x2;
	}

	uint16_t y2() const
	{
		return m_y2;
	}

	uint16_t m_x1, m_x2, m_y1, m_y2;
} ;


class QuadTree
{
public:
	QuadTree( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t level, bool topLevel = true ) :
		m_x1( x1 ),
		m_y1( y1 ),
		m_x2( x2 ),
		m_y2( y2 ),
		m_level( level ),
		m_topLevel( topLevel ),
		m_marked( false )
	{
		if( m_level > 0 )
		{
			uint16_t wh = (m_x2-m_x1+1)/2;
			uint16_t hh = (m_y2-m_y1+1)/2;
			m_subRects[0][0] = new QuadTree( x1, y1, x1+wh-1, y1+hh-1, level-1, false );
			m_subRects[1][0] = new QuadTree( x1+wh, y1, x2, y1+hh-1, level-1, false );
			m_subRects[0][1] = new QuadTree( x1, y1+hh, x1+wh-1, y2, level-1, false );
			m_subRects[1][1] = new QuadTree( x1+wh, y1+hh, x2, y2, level-1, false );
		}
	}

	~QuadTree()
	{
		if( m_level > 0 )
		{
			delete m_subRects[0][0];
			delete m_subRects[0][1];
			delete m_subRects[1][0];
			delete m_subRects[1][1];
		}
	}

	void reset()
	{
		m_marked = false;
		if( m_level > 0 )
		{
			m_subRects[0][0]->reset();
			m_subRects[0][1]->reset();
			m_subRects[1][0]->reset();
			m_subRects[1][1]->reset();
		}
	}

	bool isMarked() const
	{
		return m_marked;
	}

#define likely(x)	__builtin_expect((x),1)
#define unlikely(x)	__builtin_expect((x),0)

	bool intersects( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) const
	{
		if ( m_x1 > x2 || x1 > m_x2 ) return false;
		if ( m_y1 > y2 || y1 > m_y2 ) return false;
		return true;
	}

	void addRects( const QList<QRect> &qrects )
	{
		foreach( const QRect &r, qrects )
		{
			addRect( r.x(), r.y(), r.x()+r.width()-1, r.y()+r.height()-1 );
		}
	}

	bool addRect( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 )
	{
		if( isMarked() )
		{
			return true;
		}
		else if( intersects( x1, y1, x2, y2 ) )
		{
			if( likely( m_level > 0 ) )
			{
				bool x = true;
				x &= m_subRects[0][0]->addRect( x1, y1, x2, y2 );
				x &= m_subRects[0][1]->addRect( x1, y1, x2, y2 );
				x &= m_subRects[1][0]->addRect( x1, y1, x2, y2 );
				x &= m_subRects[1][1]->addRect( x1, y1, x2, y2 );
				m_marked = x;
			}
			else
			{
				//printf("mark %d:%d - %d:%d\n", m_x1, m_y1, m_x2, m_y2 );
				m_marked = true;
			}
		}
		return isMarked();
	}

	int numMarkedAllSubRects() const
	{
		if( unlikely( m_level == 0 ) )
		{
			return isMarked() ? 1 : 0;
		}
		return m_subRects[0][0]->numMarkedAllSubRects() +
				m_subRects[0][1]->numMarkedAllSubRects() +
				m_subRects[1][0]->numMarkedAllSubRects() +
				m_subRects[1][1]->numMarkedAllSubRects();
	}

	int numMarkedSubRects() const
	{
		if( unlikely( m_level == 0 ) )
		{
			return 0;
		}
		return ( m_subRects[0][0]->isMarked() ? 1 : 0 ) +
				( m_subRects[0][1]->isMarked() ? 1 : 0 ) +
				( m_subRects[1][0]->isMarked() ? 1 : 0 ) +
				( m_subRects[1][1]->isMarked() ? 1 : 0 );
	}

	inline int totalSubRects() const
	{
		int n = 1 << m_level;
		return n*n;
	}

	operator QuadTreeRect() const
	{
		return QuadTreeRect( m_x1, m_y1, m_x2, m_y2 );
	}

	QVector<QuadTreeRect> rects() const
	{
		if( isMarked() )
		{
			return QVector<QuadTreeRect>( 1 , (QuadTreeRect)*this);
		}
		else if( m_level == 0 )
		{
			return QVector<QuadTreeRect>();
		}
		const int x = 1<<(m_level+1);
		if( numMarkedAllSubRects() >= totalSubRects()*(x-1)/x )
		{
			return QVector<QuadTreeRect>( 1, (QuadTreeRect)*this );
		}

		if( numMarkedSubRects() == 2 )
		{
			for( int i = 0; i < 2; ++i )
			{
				// subrects at same row?
				if( m_subRects[0][i]->isMarked() && m_subRects[1][i]->isMarked() )
				{
					QVector<QuadTreeRect> l( m_subRects[0][i ? 0 : 1]->rects() +
								m_subRects[1][i ? 0 : 1]->rects() );
					l += QuadTreeRect( m_x1, m_subRects[0][i]->m_y1,
												m_x2, m_subRects[0][i]->m_y2 );
					return l;
				}
				// subrects at same col?
				if( m_subRects[i][0]->isMarked() && m_subRects[i][1]->isMarked() )
				{
					QVector<QuadTreeRect> l( m_subRects[i ? 0 : 1][0]->rects() +
										m_subRects[i ? 0 : 1][1]->rects() );
					l += QuadTreeRect( m_subRects[i][0]->m_x1, m_y1,
												m_subRects[i][0]->m_x2, m_y2 );
					return l;
				}
			}
		}

		QVector<QuadTreeRect> l( m_subRects[0][0]->rects() +
				m_subRects[0][1]->rects() +
				m_subRects[1][0]->rects() +
				m_subRects[1][1]->rects() );

		// merge all adjacent rectangles
		if( m_topLevel )
		{
			bool found = true;
			while( found )
			{
				found = false;
				for( QVector<QuadTreeRect>::Iterator it = l.begin(); it != l.end(); ++it )
				{
					for( QVector<QuadTreeRect>::Iterator jt = it+1; jt != l.end(); )
					{
						if( ( it->y1() == jt->y1() && it->y2() == jt->y2() &&
								( it->x2()+1 == jt->x1() || it->x1() == jt->x2()+1 ) ) ||
							( it->x1() == jt->x1() && it->x2() == jt->x2() &&
								( it->y2()+1 == jt->y1() || it->y1() == jt->y2()+1 ) ) )
						{
							*it = QuadTreeRect( qMin( it->x1(), jt->x1() ), qMin( it->y1(), jt->y1() ),
											qMax( it->x2(), jt->x2() ), qMax( it->y2(), jt->y2() ) );
							jt = l.erase( jt );
							found = true;
						}
						else
						{
							++jt;
						}
					}
				}
			}
		}
		return l;
	}

private:
	const uint16_t m_x1, m_y1, m_x2, m_y2;
	const uint8_t m_level;
	const bool m_topLevel;
	bool m_marked;
	QuadTree *m_subRects[2][2];
} ;

#endif

