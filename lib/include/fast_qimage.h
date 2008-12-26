/*
 * fast_qimage.h - class fastQImage providing fast inline-QImage-manips
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _FAST_QIMAGE_H
#define _FAST_QIMAGE_H

#include <QtGui/QImage>
#include <QtGui/QPixmap>

#include "types.h"


class IC_DllExport fastQImage : public QImage
{
public:
	fastQImage() : QImage() { }
	fastQImage( const QImage & _img ) : QImage( _img ) { }
	fastQImage( const QPixmap & _pm ) : QImage( _pm.toImage() ) { }


	QPixmap toPixmap( void ) const
	{
		return( QPixmap().fromImage( *this ) );
	}


	inline void fillRect( const Q_UINT16 rx, const Q_UINT16 ry,
				const Q_UINT16 rw, const Q_UINT16 rh,
				const QRgb pix )
	{
		const Q_UINT16 img_width = width();
		QRgb * dst = (QRgb *) scanLine( ry ) + rx;
		// TODO: is it faster to fill first line and then memcpy()
		//       this line?
		for( Q_UINT16 y = 0; y < rh; ++y )
		{
			//QRgb * dest = dst_base;
			for( Q_UINT16 x = 0; x < rw; ++x )
			{
				dst[x] = pix;
			}
			dst += img_width;
		}
	}



	inline void copyRect( const Q_UINT16 rx, const Q_UINT16 ry,
				const Q_UINT16 rw, const Q_UINT16 rh,
				const QRgb * buf )
	{
		if( rh > 0 )
		{
			const Q_UINT16 img_width = width();
			QRgb * dst = (QRgb *) scanLine( ry ) + rx;
			for( Q_UINT16 y = 0; y < rh; ++y )
			{
				memcpy( dst, buf, rw * sizeof( QRgb ) );
				buf += rw;
				dst += img_width;
			}
		}
		else
		{
			qWarning( "fastQImage::copyRect(): tried to copy a rect with zero-height - ignoring" );
		}
	}



	inline void copyExistingRect( const Q_UINT16 src_x,
					const Q_UINT16 src_y,
					const Q_UINT16 rw,
					const Q_UINT16 rh,
					const Q_UINT16 dest_x,
					const Q_UINT16 dest_y )
	{
		// TODO: check whether we need to handle if dest-rect is inside
		//       src-rect
		const Q_UINT16 img_width = width();
		const QRgb * src = (const QRgb *) scanLine( src_y ) + src_x;
		QRgb * dst = (QRgb *) scanLine( dest_y ) + dest_x;
		for( Q_UINT16 y = 0; y < rh; ++y )
		{
			// TODO: vectorize
			memcpy( dst, src, rw * sizeof( QRgb ) );
			src += img_width;
			dst += img_width;
		}
	}

	// scales this image to the size, _dst has
	QImage & scaleTo( QImage & _dst ) const;

	// overload horribly slow scaled()-method
	inline QImage scaled( const QSize & size,
			Qt::AspectRatioMode arm = Qt::IgnoreAspectRatio,
			Qt::TransformationMode tm = Qt::SmoothTransformation )
									const
	{
		if( tm == Qt::SmoothTransformation &&
						arm == Qt::IgnoreAspectRatio )
		{
			QImage tmp( size, format() );
			scaleTo( tmp );
			return( tmp );
		}
		return( QImage::scaled( size, arm, tm ) );
	}


	inline QImage scaled( int width, int height,
			Qt::AspectRatioMode arm = Qt::IgnoreAspectRatio,
			Qt::TransformationMode tm = Qt::SmoothTransformation )
									const
	{
		return( scaled( QSize( width, height ), arm, tm ) );
	}


	// fill alpha-channel of image with certain value
	inline fastQImage & alphaFill( const unsigned char _alpha )
	{
		QRgb * ptr = (QRgb *) bits();
		const unsigned int pixels = width() * height();
		const QRgb mask = ((unsigned int) _alpha ) << 24;
		for( unsigned int i = 0; i < pixels; ++i )
		{
			*ptr = ( *ptr & 0x00ffffff ) | mask;
			++ptr;
		}
		return *this;
	}


	// set alpha-value of all pixels to a certain value it it is greater
	// then it (something like max<..>(...) for QImage ;-)
	inline fastQImage & alphaFillMax( const unsigned char _alpha )
	{
		unsigned char * ptr = bits() + 3;
		const unsigned int pixels = width() * height();
		for( unsigned int i = 0; i < pixels; ++i )
		{
			if( *ptr > _alpha )
			{
				*ptr = _alpha;
			}
			ptr += 4;
		}
		return( *this );
	}


	// darken whole image by _coeff (_coeff = [0;256])
	inline fastQImage & darken( const Q_UINT16 _coeff )
	{
		unsigned char * ptr = bits();
		const unsigned int pixels = width() * height();
		for( unsigned int i = 0; i < pixels; ++i )
		{
			for( int j = 0; j < 4; ++j )
			{
				ptr[j] = (  ptr[j] * _coeff ) >> 8;
			}
			ptr += 4;
		}
		return *this;
	}


	inline fastQImage & toGray( void )
	{
		QRgb * ptr = (QRgb *) bits();
		const unsigned int pixels = width() * height();
		for( unsigned int i = 0; i < pixels; ++i )
		{
			const unsigned int gray = qGray( *ptr );
			const QRgb g = gray | ( gray << 8 ) | ( gray << 16 ) | 0xff000000;
			*ptr = g;
			++ptr;
		}
		return *this;
	}


} ;




inline QPixmap scaled( const QString & _file, const int _w, const int _h )
{
	return( QPixmap::fromImage( fastQImage( QPixmap( _file ) ).
							scaled( _w, _h ) ) );
}



#endif

