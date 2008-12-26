#if 0
/*
 * ivs_connection.h - declaration of class ivsConnection, an implementation of
 *                    the RFB-protocol with iTALC-extensions for Qt
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _IVS_CONNECTION_H
#define _IVS_CONNECTION_H

#include <italcconfig.h>

#include <QtCore/QReadWriteLock>
#include <QtGui/QImage>
#include <QtGui/QRegion>

#include <zlib.h>


namespace jpeglib
{
	extern "C"
	{		// otherwise jpeg-lib doesn't work with C++ source...
		#include <jpeglib.h>
	}
}

#include "isd_connection.h"
#include "rfb/rfbproto.h"
#include "fast_qimage.h"


class demoServerClient;


class IC_DllExport ivsConnection : public isdConnection
{
	Q_OBJECT
public:
       enum quality
       {
               QualityLow,
              QualityMedium,
               QualityHigh,
               QualityDemoLow,
               QualityDemoMedium,
               QualityDemoHigh
       } ;



	ivsConnection( const QString & _host, quality _q = QualityLow,
						bool _use_auth_file = FALSE,
						QObject * _parent = NULL );
	virtual ~ivsConnection();

	virtual void close( void );

	QImage scaledScreen( void )
	{
		QReadLocker rl( &m_scaledImageLock );
		return( m_scaledScreen );
	}

	const QImage & screen( void ) const
	{
		//QReadLocker rl( &m_imageLock );
		return( m_screen );
	}

	QSize framebufferSize( void ) const
	{
		if( m_si.framebufferWidth > 0 && m_si.framebufferHeight > 0 )
		{
			return( QSize( m_si.framebufferWidth,
						m_si.framebufferHeight ) );
		}
		return( QSize( 640, 480 ) );
	}

	void setScaledSize( const QSize & _s )
	{
		m_scaledSize = _s;
		m_scaledScreenNeedsUpdate = TRUE;
	}

	void rescaleScreen( void );

	bool handleServerMessages( bool _send_screen_update, int _tries = 5 );

	bool softwareCursor( void ) const
	{
		return( m_softwareCursor );
	}

	const QPoint & cursorPos( void ) const
	{
		return( m_cursorPos );
	}

	const QPoint & cursorHotSpot( void ) const
	{
		return( m_cursorHotSpot );
	}

	QImage cursorShape( void ) const
	{
		QReadLocker rl( &m_cursorLock );
		return( m_cursorShape );
	}

	bool takeSnapshot( void );


public slots:
	bool sendFramebufferUpdateRequest( void );
	bool sendIncrementalFramebufferUpdateRequest( void );
	bool sendPointerEvent( Q_UINT16 _x, Q_UINT16 _y, Q_UINT16
								_button_mask );
	bool sendKeyEvent( Q_UINT32 _key, bool _down );


protected:
	virtual states protocolInitialization( void );


private:
	bool sendFramebufferUpdateRequest( Q_UINT16 _x, Q_UINT16 _y,
						Q_UINT16 _w, Q_UINT16 _h,
						bool _incremental );

	void postRegionChangedEvent( const QRegion & _rgn );

	bool handleCursorPos( const Q_UINT16 _x, const Q_UINT16 _y );
	bool handleCursorShape( const Q_UINT16 _xhot, const Q_UINT16 _yhot,
				const Q_UINT16 _w, const Q_UINT16 _h,
				const Q_UINT32 _e );

	bool handleRaw( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );
	bool handleRRE( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );

	bool handleCoRRE( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );

	bool handleZlib( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );
	bool handleTight( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );
	int initFilterCopy( Q_UINT16 rw, Q_UINT16 rh );
	int initFilterPalette( Q_UINT16 rw, Q_UINT16 rh );
	int initFilterGradient( Q_UINT16 rw, Q_UINT16 rh );
	void filterCopy( Q_UINT16 num_rows, Q_UINT32 * dest_buffer );
	void filterPalette( Q_UINT16 num_rows, Q_UINT32 * dest_buffer );
	void filterGradient( Q_UINT16 num_rows, Q_UINT32 * dest_buffer );
	bool decompressJpegRect( Q_UINT16 x, Q_UINT16 y, Q_UINT16 w,
								Q_UINT16 h );
	bool handleItalc( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw, Q_UINT16 rh );

	bool m_isDemoServer;
	bool m_useAuthFile;

	quality m_quality;


	rfbServerInitMsg m_si;

	mutable QReadWriteLock m_imageLock;
	mutable QReadWriteLock m_scaledImageLock;
	fastQImage m_screen;
	QImage m_scaledScreen;
	bool m_scaledScreenNeedsUpdate;

	QSize m_scaledSize;


	mutable QReadWriteLock m_cursorLock;
	bool m_softwareCursor;
	QPoint m_cursorPos;
	QPoint m_cursorHotSpot;
	QImage m_cursorShape;

//	static const rfbPixelFormat s_localDisplayFormat;

	// Note that the CoRRE encoding uses this buffer and assumes it is
	// big enough to hold 255 * 255 * 32 bits -> 260100 bytes.
	// 640*480 = 307200 bytes.
	// Hextile also assumes it is big enough to hold 16 * 16 * 32 bits.
	// Tight encoding assumes BUFFER_SIZE is at least 16384 bytes.
	#define BUFFER_SIZE (640*480)
	char m_buffer[BUFFER_SIZE];

	// variables for the zlib-encoding
	int m_rawBufferSize;
	char * m_rawBuffer;
	z_stream m_decompStream;
	bool m_decompStreamInited;

	// Variables for the ``tight'' encoding implementation.

	// Separate buffer for compressed data.
	#define ZLIB_BUFFER_SIZE 512
	char m_zlibBuffer[ZLIB_BUFFER_SIZE];

	// Four independent compression streams for zlib library.
	z_stream m_zlibStream[4];
	bool m_zlibStreamActive[4];

	// Filter stuff. Should be initialized by filter initialization code.
	Q_UINT16 m_rectWidth, m_rectColors;
	char m_tightPalette[256*4];
	Q_UINT8 m_tightPrevRow[2048*3*sizeof(Q_UINT16)];

	// JPEG decoder state.
	jpeglib::jpeg_source_mgr m_jpegSrcManager;


	friend class demoServerClient;


signals:
	void cursorShapeChanged( void );
	void regionUpdated( const QRegion & );

} ;


#endif
#endif
