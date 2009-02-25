/*
 * fast_qimage.cpp - class fastQImage providing fast inline-QImage-manips
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
 * License aint with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "fast_qimage.h"


// QImage of Qt4 is slow as hell which makes it unusable for ops like scaling
// etc. Here's now an improved version of QImage::smoothScale() of Qt3 which is
// probably faster than anything else...
void fastQImage::scaleTo( QImage & dst ) const
{
	if( format() != Format_ARGB32 )
	{
		fastQImage( convertToFormat( Format_ARGB32 ) ).scaleTo( dst );
		return;
	}

	QRgb * xelrow = NULL;
	QRgb * tempxelrow = NULL;
	Q_UINT16 rowswritten = 0;
	const uchar maxval = 255;

	const Q_UINT16 cols = width();
	const Q_UINT16 rows = height();
	const Q_UINT16 newcols = dst.width();
	const Q_UINT16 newrows = dst.height();

	int SCALE;
	int HALFSCALE;

	if( cols > 4096 )
	{
		SCALE = 4096;
		HALFSCALE = 2048;
	}
	else
	{
		Q_UINT16 fac = 4096;
		while( cols * fac > 4096 )
		{
			fac /= 2;
		}
		SCALE = fac * cols;
		HALFSCALE = fac * cols / 2;
	}

	int sxscale = (int)( (double) newcols / (double) cols * SCALE );
	int syscale = (int)( (double) newrows / (double) rows * SCALE );

	if( newrows != rows )	// shortcut Y scaling if possible
		tempxelrow = new QRgb[cols];

	int * rs = new int[cols];
	int * gs = new int[cols];
	int * bs = new int[cols];
	int * as = NULL;

	if( format() != dst.format() )
	{
		dst = QImage( dst.width(), dst.height(), format() );
	}
	if( hasAlphaChannel() )
	{
		as = new int[cols];
		for( Q_UINT16 col = 0; col < cols; ++col )
		{
			rs[col] = gs[col] = bs[col] = as[col] = HALFSCALE;
		}
	}
	else
	{
		for( Q_UINT16 col = 0; col < cols; ++col )
		{
			rs[col] = gs[col] = bs[col] = HALFSCALE;
		}
	}


	Q_UINT16 rowsread = 0;
	register int fracrowleft = syscale;
	register Q_UINT16 needtoreadrow = 1;

	register int fracrowtofill = SCALE;
	register QRgb * xP;
	register QRgb * nxP;

	for( Q_UINT16 row = 0; row < newrows; ++row )
	{
		// First scale Y from xelrow into tempxelrow.
		if( newrows == rows )
		{
			// shortcut Y scaling if possible 
			tempxelrow = xelrow = (QRgb*)scanLine( rowsread++ );
		}
		else
		{
			while( fracrowleft < fracrowtofill )
			{
				if( needtoreadrow && rowsread < rows )
				{
					xelrow = (QRgb*)scanLine( rowsread++ );
				}
				xP = xelrow;
				if( as )
				{
				for( Q_UINT16 col = 0; col < cols; ++col, ++xP )
				{
					const int a = fracrowleft *
								qAlpha( *xP );
					as[col] += a;
					rs[col] += a * qRed( *xP ) / 255;
					gs[col] += a * qGreen( *xP ) / 255;
					bs[col] += a * qBlue( *xP ) / 255;
				}
				}
				else
				{
				for( Q_UINT16 col = 0; col < cols; ++col, ++xP )
				{
					rs[col] += fracrowleft * qRed( *xP );
					gs[col] += fracrowleft * qGreen( *xP );
					bs[col] += fracrowleft * qBlue( *xP );
				}
				}
				fracrowtofill -= fracrowleft;
				fracrowleft = syscale;
				needtoreadrow = 1;
			}
			// Now fracrowleft is >= fracrowtofill, so we can
			// produce a row.
			if( needtoreadrow && rowsread < rows )
			{
				xelrow = (QRgb*)scanLine( rowsread++ );
				needtoreadrow = 0;
			}
			xP = xelrow;
			nxP = tempxelrow;
			if( as )
			{
			for( Q_UINT16 col = 0; col < cols; ++col, ++xP, ++nxP )
			{
				const int a1 = fracrowtofill * qAlpha( *xP );
				int a = as[col] + a1;
				int r = rs[col] + a1 * qRed( *xP ) / 255;
				int g = gs[col] + a1 * qGreen( *xP ) / 255;
				int b = bs[col] + a1 * qBlue( *xP ) / 255;
				if( a )
				{
					r = r * 255 / a * SCALE;
					g = g * 255 / a * SCALE;
					b = b * 255 / a * SCALE;
				}

				r /= SCALE; if( r > maxval ) r = maxval;
				g /= SCALE; if( g > maxval ) g = maxval;
				b /= SCALE; if( b > maxval ) b = maxval;
				a /= SCALE; if( a > maxval ) a = maxval;

				*nxP = qRgba( r, g, b, a );
				rs[col] = gs[col] = bs[col] = as[col] =
								HALFSCALE;
			}
			}
			else
			{
			for( Q_UINT16 col = 0; col < cols; ++col, ++xP, ++nxP )
			{
				int r = rs[col] + fracrowtofill * qRed( *xP );
				int g = gs[col] + fracrowtofill * qGreen( *xP );
				int b = bs[col] + fracrowtofill * qBlue( *xP );

				r /= SCALE; if( r > maxval ) r = maxval;
				g /= SCALE; if( g > maxval ) g = maxval;
				b /= SCALE; if( b > maxval ) b = maxval;

				*nxP = qRgb( r, g, b );
				rs[col] = gs[col] = bs[col] = HALFSCALE;
			}
			}
			fracrowleft -= fracrowtofill;
			if( fracrowleft == 0 )
			{
				fracrowleft = syscale;
				needtoreadrow = 1;
			}
			fracrowtofill = SCALE;
		}

		// Now scale X from tempxelrow into dst and write it out.
		if( newcols == cols )
		{
			// shortcut X scaling if possible
			memcpy( dst.scanLine( rowswritten++ ), tempxelrow,
								newcols*4 );
		}
		else
		{
			int r = HALFSCALE;
			int g = HALFSCALE;
			int b = HALFSCALE;
			int fraccoltofill = SCALE, fraccolleft = 0;
			Q_UINT16 needcol = 0;

			nxP = (QRgb*)dst.scanLine( rowswritten++ );
			xP = tempxelrow;
			if( as )
			{
			int a = HALFSCALE;
			for( Q_UINT16 col = 0; col < cols; ++col, ++xP )
			{
				fraccolleft = sxscale;
				while( fraccolleft >= fraccoltofill )
				{
					if( needcol )
					{
						++nxP;
						r = g = b = a = HALFSCALE;
					}
					const int a1 = fraccoltofill *
								qAlpha( *xP );
					a += a1;
					r += a1 * qRed( *xP ) / 255;
					g += a1 * qGreen( *xP ) / 255;
					b += a1 * qBlue( *xP ) /255;
					if( a )
					{
						r = r * 255 / a * SCALE;
						g = g * 255 / a * SCALE;
						b = b * 255 / a * SCALE;
					}

					r /= SCALE; if( r > maxval ) r = maxval;
					g /= SCALE; if( g > maxval ) g = maxval;
					b /= SCALE; if( b > maxval ) b = maxval;
					a /= SCALE; if( a > maxval ) a = maxval;

					*nxP = qRgba( r, g, b, a );
					fraccolleft -= fraccoltofill;
					fraccoltofill = SCALE;
					needcol = 1;
				}
				if( fraccolleft > 0 )
				{
					if( needcol )
					{
						++nxP;
						r = g = b = a = HALFSCALE;
						needcol = 0;
					}
					const int a1 = fraccolleft *
								qAlpha( *xP );
					a += a1;
					r += a1 * qRed( *xP ) / 255;
					g += a1 * qGreen( *xP ) / 255;
					b += a1 * qBlue( *xP ) / 255;
					fraccoltofill -= fraccolleft;
				}
			}
			if( fraccoltofill > 0 )
			{
				--xP;
				a += fraccolleft * qAlpha( *xP );
				const int a1 = fraccoltofill * qAlpha( *xP );
				r += a1 * qRed( *xP ) / 255;
				g += a1 * qGreen( *xP ) / 255;
				b += a1 * qBlue( *xP ) / 255;
				if( a )
				{
					r = r * 255 / a * SCALE;
					g = g * 255 / a * SCALE;
					b = b * 255 / a * SCALE;
				}
			}
			if( !needcol )
			{
				r /= SCALE; if( r > maxval ) r = maxval;
				g /= SCALE; if( g > maxval ) g = maxval;
				b /= SCALE; if( b > maxval ) b = maxval;
				a /= SCALE; if( a > maxval ) a = maxval;

				*nxP = qRgba( r, g, b, a );
			}
			}
			else
			{
			for( Q_UINT16 col = 0; col < cols; ++col, ++xP )
			{
				fraccolleft = sxscale;
				while( fraccolleft >= fraccoltofill )
				{
					if( needcol )
					{
						++nxP;
						r = g = b = HALFSCALE;
					}
					r += fraccoltofill * qRed( *xP );
					g += fraccoltofill * qGreen( *xP );
					b += fraccoltofill * qBlue( *xP );

					r /= SCALE; if( r > maxval ) r = maxval;
					g /= SCALE; if( g > maxval ) g = maxval;
					b /= SCALE; if( b > maxval ) b = maxval;

					*nxP = qRgb( r, g, b );
					fraccolleft -= fraccoltofill;
					fraccoltofill = SCALE;
					needcol = 1;
				}
				if( fraccolleft > 0 )
				{
					if( needcol )
					{
						++nxP;
						r = g = b = HALFSCALE;
						needcol = 0;
					}
					r += fraccolleft * qRed( *xP );
					g += fraccolleft * qGreen( *xP );
					b += fraccolleft * qBlue( *xP );
					fraccoltofill -= fraccolleft;
				}
			}
			if( fraccoltofill > 0 )
			{
				--xP;
				r += fraccoltofill * qRed( *xP );
				g += fraccoltofill * qGreen( *xP );
				b += fraccoltofill * qBlue( *xP );
			}
			if( !needcol )
			{
				r /= SCALE; if( r > maxval ) r = maxval;
				g /= SCALE; if( g > maxval ) g = maxval;
				b /= SCALE; if( b > maxval ) b = maxval;

				*nxP = qRgb( r, g, b );
			}
			}
		}
	}

	// Robust, tempxelrow might be 0 one day
	if( newrows != rows && tempxelrow )
	{
		delete[] tempxelrow;
	}
	// Robust, rs might be 0 one day
	if( rs )
	{
		delete[] rs;
	}
	// Robust, gs might be 0 one day
	if( gs )
	{
		delete[] gs;
	}
	// Robust, bs might be 0 one day
	if( bs )
	{
		delete[] bs;
	}
	// Robust, bs might be 0 one day
	if( as )
	{
		delete[] as;
	}
}


