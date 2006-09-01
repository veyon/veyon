/*
 * ivs_connection.cpp - class ivsConnection, an implementation of the 
 *                      RFB-protocol with iTALC-extensions for Qt
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>


#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>


#include "ivs_connection.h"
#include "qt_user_events.h"

#ifdef BUILD_ICA
#include "minilzo.h"
#endif


const rfbPixelFormat ivsConnection::s_localDisplayFormat =
{
	32, 		// bit per pixel
	32,		// depth
#ifdef WORDS_BIGENDIAN
	1,		// big endian
#else
	0,		// big endian
#endif
	1,		// true color
	255,		// red max
	255,		// green max
	255,		// blue max
	16,		// red shift
	8,		// green shift
	0,		// blue shift,
	0, 0		// only for padding
} ;




// =============================================================================
// implementation of IVS-connection
// =============================================================================


// normal ctor
ivsConnection::ivsConnection( const QString & _host, quality _q,
							QObject * _parent ) :
	isdConnection( ( _host.contains( ':' ) ? _host : _host + ":5900" ),
								_parent ),
	m_isDemoServer( FALSE ),
	m_quality( _q ),
	m_imageLock(),
	m_screen(),
	m_scaledScreen(),
	m_scaledSize(),
	m_softwareCursor( FALSE ),
	m_cursorPos( 0, 0 ),
	m_cursorShape(),
	m_rawBufferSize( -1 ),
	m_rawBuffer( NULL ),
	m_decompStreamInited( FALSE )
{
#ifdef HAVE_LIBZ
	m_zlibStreamActive[0] = m_zlibStreamActive[1] = m_zlibStreamActive[2] =
						m_zlibStreamActive[3] = FALSE;
#endif
}




ivsConnection::~ivsConnection()
{
	delete m_rawBuffer;
}




ivsConnection::states ivsConnection::protocolInitialization( void )
{
	rfbProtocolVersionMsg protocol_version;

	if( !readFromServer( protocol_version, sz_rfbProtocolVersionMsg ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	protocol_version[sz_rfbProtocolVersionMsg] = 0;

	int major, minor;
	if( sscanf( protocol_version, rfbProtocolVersionFormat, &major,
							&minor ) != 2 )
	{
		// not a standard VNC server - check whether it is an iTALC-
		// demo-server
		if( sscanf( protocol_version, idsProtocolVersionFormat, &major,
							&minor ) != 2 )
		{
			printf( "Not a server I can deal with\n" );
			return( state_ref() = InvalidServer );
		}
		m_isDemoServer = TRUE;
	}

	if( !writeToServer( protocol_version, sz_rfbProtocolVersionMsg ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	if( authAgainstServer( m_quality == QualityDemoPurposes ) !=
								Connecting )
	{
		return( state() );
	}


	const rfbClientInitMsg ci = { 1 };

	if( !writeToServer( (const char *) &ci, sizeof( ci ) ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	if( !readFromServer( (char *)&m_si, sizeof( m_si ) ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	m_si.framebufferWidth = swap16IfLE( m_si.framebufferWidth );
	m_si.framebufferHeight = swap16IfLE( m_si.framebufferHeight );
	m_si.format.redMax = swap16IfLE( m_si.format.redMax );
	m_si.format.greenMax = swap16IfLE( m_si.format.greenMax );
	m_si.format.blueMax = swap16IfLE( m_si.format.blueMax );
	m_si.nameLength = swap32IfLE( m_si.nameLength );

	char * desktop_name = new char[m_si.nameLength+1];

	if( !readFromServer( desktop_name, m_si.nameLength ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	delete[] desktop_name;

	//desktop_name[m_si.nameLength] = 0;

	//printf( "Desktop name: \"%s\"\n",desktop_name );
	//printf( "Connected to VNC server, using protocol version %d.%d\n",
	//			RFBP_MAJOR_VERSION, RFBP_MINOR_VERSION );

	rfbSetPixelFormatMsg spf;

	spf.type = rfbSetPixelFormat;
	spf.format = s_localDisplayFormat;
	spf.format.redMax = swap16IfLE( spf.format.redMax );
	spf.format.greenMax = swap16IfLE( spf.format.greenMax );
	spf.format.blueMax = swap16IfLE( spf.format.blueMax );

	if( !writeToServer( (const char *) &spf, sizeof( spf ) ) )
	{
		return( state_ref() = ConnectionFailed );
	}


	char buf[sizeof( rfbSetPixelFormatMsg ) + MAX_ENCODINGS *
							sizeof( Q_UINT32 )];
	rfbSetEncodingsMsg * se = (rfbSetEncodingsMsg *) buf;
	se->type = rfbSetEncodings;
	se->nEncodings = 0;

	Q_UINT32 * encs = (Q_UINT32 *)( &buf[sizeof(rfbSetEncodingsMsg)] );

	if( m_quality == QualityDemoPurposes )
	{
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingRaw );
	}
	else
	{
#ifdef HAVE_LIBZ
#ifdef HAVE_LIBJPEG
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingTight );
#endif
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingZlib );
#endif
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingCopyRect );
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingCoRRE );
		encs[se->nEncodings++] = swap32IfLE( rfbEncodingRaw );
		//encs[se->nEncodings++] = swap32IfLE( rfbEncodingRRE );
#ifdef HAVE_LIBZ
#ifdef HAVE_LIBJPEG
		switch( m_quality )
		{
			case QualityLow:
				encs[se->nEncodings++] = swap32IfLE(
						rfbEncodingQualityLevel4 );
				break;
			case QualityMedium:
				encs[se->nEncodings++] = swap32IfLE(
						rfbEncodingQualityLevel9 );
				break;
			case QualityHigh:
				// no JPEG
				break;
			default:
				break;
		}
#endif
		encs[se->nEncodings++] = swap32IfLE(
						rfbEncodingCompressLevel4 );
#endif
	}
	encs[se->nEncodings++] = swap32IfLE( rfbEncodingRichCursor );
	//encs[se->nEncodings++] = swap32IfLE( rfbEncodingXCursor );
	encs[se->nEncodings++] = swap32IfLE( rfbEncodingPointerPos );

	//encs[se->nEncodings++] = swap32IfLE( rfbEncodingLastRect );
	encs[se->nEncodings++] = swap32IfLE( rfbEncodingItalc );
	encs[se->nEncodings++] = swap32IfLE( rfbEncodingItalcCursor );


	unsigned int len = sizeof( rfbSetEncodingsMsg ) +
					se->nEncodings * sizeof( Q_UINT32 );

	se->nEncodings = swap16IfLE( se->nEncodings );

	if( !writeToServer( buf, len ) )
	{
		return( state_ref() = ConnectionFailed );
	}

	state_ref() = Connected;

	m_screen = QImage( m_si.framebufferWidth, m_si.framebufferHeight,
							QImage::Format_RGB32 );


	sendFramebufferUpdateRequest();

	return( state() );
}




void ivsConnection::close( void )
{
#ifdef HAVE_LIBZ
	m_zlibStreamActive[0] = m_zlibStreamActive[1] = m_zlibStreamActive[2] =
						m_zlibStreamActive[3] = FALSE;
#endif
	isdConnection::close();
}




bool ivsConnection::sendFramebufferUpdateRequest( void )
{
	return( sendFramebufferUpdateRequest( 0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, FALSE ) );
}




bool ivsConnection::sendIncrementalFramebufferUpdateRequest( void )
{
	return( sendFramebufferUpdateRequest( 0, 0, m_si.framebufferWidth,
					m_si.framebufferHeight, TRUE ) );
}




bool ivsConnection::sendFramebufferUpdateRequest( Q_UINT16 _x, Q_UINT16 _y,
				Q_UINT16 _w, Q_UINT16 _h, bool _incremental )
{
	rfbFramebufferUpdateRequestMsg fur;

	fur.type = rfbFramebufferUpdateRequest;
	fur.incremental = ( _incremental ) ? 1 : 0;
	fur.x = swap16IfLE( _x );
	fur.y = swap16IfLE( _y );
	fur.w = swap16IfLE( _w );
	fur.h = swap16IfLE( _h );

	return( writeToServer( (char *) &fur, sizeof( fur ) ) );
}




bool ivsConnection::sendPointerEvent( Q_UINT16 _x, Q_UINT16 _y,
							Q_UINT16 _button_mask )
{
	rfbPointerEventMsg pe;

	pe.type = rfbPointerEvent;
	pe.buttonMask = _button_mask;
	//if (_x < 0) _x = 0;
	//if (_y < 0) _y = 0;
	pe.x = swap16IfLE( _x );
	pe.y = swap16IfLE( _y );

	// make sure our own pointer is updated when remote-controlling
	handleCursorPos( _x, _y );

	return( writeToServer( (char *) &pe, sizeof( pe ) ) );
}




bool ivsConnection::sendKeyEvent( Q_UINT32 key, bool down )
{
	rfbKeyEventMsg ke;

	ke.type = rfbKeyEvent;
	ke.down = ( down )? 1 : 0;
	ke.key = swap32IfLE( key );

	return( writeToServer( (char *) &ke, sizeof( ke ) ) );
}




bool ivsConnection::handleServerMessages( bool _send_screen_update, int _tries )
{
	while( hasData() && --_tries >= 0 )
	{

	rfbServerToClientMsg msg;
	if( !readFromServer( (char *) &msg, sizeof( Q_UINT8 ) ) )
	{
		printf( "Reading message-type failed\n" );
		return( FALSE );
	}
	switch( msg.type )
	{
		case rfbSetColourMapEntries:
			printf( "setting colormap entries requested. "
								"Ignoring.\n" );
			break;

		case rfbFramebufferUpdate:
		{
			QWriteLocker wl( &m_imageLock );
			if( !readFromServer( ( (char *)&msg.fu ) + 1,
				sizeof( rfbFramebufferUpdateMsg ) - 1 ) )
			{
				printf( "reading framebuffer-update-msg "
								"failed\n" );
				return( FALSE );
			}

			msg.fu.nRects = swap16IfLE( msg.fu.nRects );

			QRegion updated_region;

			rfbFramebufferUpdateRectHeader rect;
			for( Q_UINT16 i = 0; i < msg.fu.nRects; i++ )
			{
				if( !readFromServer( (char *)&rect,
							sizeof( rect ) ) )
				{
					return( FALSE );
				}

				rect.r.x = swap16IfLE( rect.r.x );
				rect.r.y = swap16IfLE( rect.r.y );
				rect.r.w = swap16IfLE( rect.r.w );
				rect.r.h = swap16IfLE( rect.r.h );

				rect.encoding = swap32IfLE( rect.encoding );
				if( rect.encoding == rfbEncodingLastRect )
				{
					break;
				}

				if( ( rect.r.x + rect.r.w >
						m_si.framebufferWidth ) ||
				    ( rect.r.y + rect.r.h >
						m_si.framebufferHeight ) )
				{
					printf( "Rect too large: %dx%d at (%d, "
						"%d) (encoding: %d)\n",
							rect.r.w, rect.r.h,
							rect.r.x, rect.r.y,
							rect.encoding );
					return( FALSE );
				}

				if ( rect.encoding != rfbEncodingPointerPos &&
				rect.encoding != rfbEncodingXCursor &&
				rect.encoding != rfbEncodingRichCursor )
				{
					if( ( rect.r.h * rect.r.w ) == 0 )
					{
						printf( "Zero size rect - "
							"ignoring\n" );
						continue;
					}
					updated_region += QRect( rect.r.x,
								rect.r.y,
								rect.r.w,
								rect.r.h );
				}
				else
				{
					m_softwareCursor = TRUE;
				}

				switch( rect.encoding )
				{
					case rfbEncodingRaw:
		if( !handleRaw( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
			return( FALSE );
		}
		break;

					case rfbEncodingCopyRect:
					{
		rfbCopyRect cr;
		if( !readFromServer( (char *) &cr, sizeof( cr ) ) )
		{
			return( FALSE );
		}
		cr.srcX = swap16IfLE( cr.srcX );
		cr.srcY = swap16IfLE( cr.srcY );

		m_screen.copyExistingRect( cr.srcX, cr.srcY, rect.r.w,
						rect.r.h, rect.r.x, rect.r.y );
		break;
					}

					case rfbEncodingRRE:
		if( !handleRRE( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
			return( FALSE );
		}
		break;

					case rfbEncodingCoRRE:
		if( !handleCoRRE( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
			return( FALSE );
		}
		break;
#ifdef HAVE_LIBZ
					case rfbEncodingZlib:
		if( !handleZlib( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
				return( FALSE );
		}
		break;
					case rfbEncodingTight:
		if( !handleTight( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
				return( FALSE );
		}
		break;
#endif
					case rfbEncodingPointerPos:
		if( !handleCursorPos( rect.r.x, rect.r.y ) )
		{
				return( FALSE );
		}
		break;

					case rfbEncodingRichCursor:
					case rfbEncodingXCursor:
		if( !handleCursorShape( rect.r.x, rect.r.y, rect.r.w,
						rect.r.h, rect.encoding ) )
		{
				return( FALSE );
		}
		break;

#ifdef BUILD_ICA
					case rfbEncodingItalc:
		if( !handleItalc( rect.r.x, rect.r.y, rect.r.w, rect.r.h ) )
		{
			return( FALSE );
		}
		break;
#endif

					case rfbEncodingItalcCursor:
	{
		QRegion ch_reg = QRect( m_cursorPos - m_cursorHotSpot,
							m_cursorShape.size() );
		QDataStream ds( static_cast<QTcpSocket *>(
							socketDev().user() ) );
		ds >> m_cursorShape;
		//m_cursorShape = socketDev().user()<QPixmap>();
/*		m_cursorShape = QPixmap::fromImage( cur );
		m_cursorShape.setMask( QBitmap::fromData( QSize( _width, _height ),
							rcMask, QImage::Format_Mono ) );*/
		m_cursorHotSpot = QPoint( rect.r.x, rect.r.y );


		ch_reg += QRect( m_cursorPos - m_cursorHotSpot,
							m_cursorShape.size() );

		// make sure, area around old cursor is updated and new cursor
		// is painted
		if( parent() != NULL )
		{
			regionChangedEvent rche( ch_reg );
			QApplication::sendEvent( parent(), &rche );
		}
		break;
	}

					default:
		printf( "Unknown rect encoding %d\n", (int) rect.encoding );
		close();
		return( FALSE );
				}
			}

			if( !m_scaledSize.isEmpty() )
			{
				m_scaledScreen = m_screen.scaled(
								m_scaledSize );
			}

			if( m_quality == QualityDemoPurposes )
			{
				// if we're providing data for demo-purposes,
				// we perform a simple color-reduction for
				// better compression-results
	QVector<QRect> rects = updated_region.rects();
	for( QVector<QRect>::const_iterator it = rects.begin();
					it != rects.end(); ++it )
	{
		for( Q_UINT16 y = 0; y < it->height(); ++y )
		{
			QRgb * data = ( (QRgb *) m_screen.scanLine( y +
							it->y() ) ) + it->x();
			for( Q_UINT16 x = 0; x < it->width(); ++x )
			{
				data[x] &= 0xf8f8f8;
			}
		}
	}
			}
			if( parent() != NULL )
			{
				regionChangedEvent rche( updated_region );
				QApplication::sendEvent( parent(), &rche );
			}

			wl.unlock();
			emit regionUpdated( updated_region );

			break;
		}

		case rfbBell:
			// FIXME: bell-action
			break;

		case rfbServerCutText:
		{
			if( !readFromServer( ( (char *) &msg ) + 1,
					sizeof( rfbServerCutTextMsg ) - 1 ) )
			{
				return( FALSE );
			}
			msg.sct.length = swap32IfLE( msg.sct.length );
			char * server_cut_text = new char[msg.sct.length+1];
			if( !readFromServer( server_cut_text, msg.sct.length ) )
			{
				delete[] server_cut_text;
				return( FALSE );
			}
			delete[] server_cut_text;
			break;
    		}

		default:
			if( !isdConnection::handleServerMessage( msg.type ) )
			{
				return( FALSE );
			}
	}

	}	// end while( ... )

	if( _send_screen_update )
	{
		return( sendIncrementalFramebufferUpdateRequest() );
	}

	return( TRUE );
}





// =============================================================================
// functions for decoding rects
// =============================================================================


bool ivsConnection::handleRaw( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw,
								Q_UINT16 rh )
{
	const int bytes_per_line = rw * sizeof( QRgb );
	Q_UINT16 lines_to_read = BUFFER_SIZE / bytes_per_line;
	const Q_UINT16 img_width = m_screen.width();
	while( rh > 0 )
	{
		if( lines_to_read > rh )
		{
			lines_to_read = rh;
		}
		if( !readFromServer( m_buffer, bytes_per_line *
							lines_to_read ) )
		{
			return( FALSE );
		}
		const QRgb * src = (const QRgb *) m_buffer;
		QRgb * dst = (QRgb *) m_screen.scanLine( ry ) + rx;
		for( Q_UINT16 y = 0; y < lines_to_read; ++y )
		{
			memcpy( dst, src, rw * sizeof( QRgb ) );
			src += rw;
			dst += img_width;
		}
		rh -= lines_to_read;
		ry += lines_to_read;
	}
	return( TRUE );
}




bool ivsConnection::handleCoRRE( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw,
								Q_UINT16 rh )
{
	rfbRREHeader hdr;

	if( !readFromServer( (char *) &hdr, sizeof( hdr ) ) )
	{
		return( FALSE );
	}

	hdr.nSubrects = swap32IfLE( hdr.nSubrects );

	QRgb pix;
	if( !readFromServer( (char *) &pix, sizeof( pix ) ) )
	{
		return( FALSE );
	}

	m_screen.fillRect( rx, ry, rw, rh, pix );

	if( !readFromServer( m_buffer, hdr.nSubrects *
				( sizeof( rfbCoRRERectangle) +
						sizeof( Q_UINT32 ) ) ) )
	{
		return( FALSE );
	}

	Q_UINT8 * ptr = (Q_UINT8 *) m_buffer;

	for( Q_UINT32 i = 0; i < hdr.nSubrects; i++ )
	{
		pix = *(QRgb *) ptr;
		ptr += sizeof( pix );
		Q_UINT8 x = *ptr++;
		Q_UINT8 y = *ptr++;
		Q_UINT8 w = *ptr++;
		Q_UINT8 h = *ptr++;
		m_screen.fillRect( rx+x, ry+y, w, h, pix );
	}

	return( TRUE );
}



bool ivsConnection::handleRRE( Q_UINT16, Q_UINT16, Q_UINT16, Q_UINT16 )
{
	printf( "Fatal: Got RRE-encoded rect. Can't decode.\n" );
	return( TRUE );
}


#define RGB_TO_PIXEL(r,g,b)						\
  (((Q_UINT32)(r) & s_localDisplayFormat.redMax ) <<			\
				s_localDisplayFormat.redShift |		\
   ((Q_UINT32)(g) & s_localDisplayFormat.greenMax ) << 			\
				s_localDisplayFormat.greenShift |	\
   ((Q_UINT32)(b) & s_localDisplayFormat.blueMax ) <<			\
				s_localDisplayFormat.blueShift )



#ifdef HAVE_LIBZ


bool ivsConnection::handleZlib( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw,
								Q_UINT16 rh )
{
	/* First make sure we have a large enough raw buffer to hold the
	* decompressed data.  In practice, with a fixed BPP, fixed frame
	* buffer size and the first update containing the entire frame
	* buffer, this buffer allocation should only happen once, on the
	* first update.
	*/
	if( m_rawBufferSize <  (int) rw * rh * 4 )
	{
		delete m_rawBuffer;
		m_rawBufferSize = (int) rw * rh * 4;
		m_rawBuffer = new char[m_rawBufferSize];
	}

	rfbZlibHeader hdr;
	if( !readFromServer( (char *) &hdr, sz_rfbZlibHeader ) )
	{
		return( FALSE );
	}

	int remaining = swap32IfLE( hdr.nBytes );

	// Need to initialize the decompressor state
	m_decompStream.next_in   = ( Bytef * ) m_buffer;
	m_decompStream.avail_in  = 0;
	m_decompStream.next_out  = ( Bytef * ) m_rawBuffer;
	m_decompStream.avail_out = m_rawBufferSize;
	m_decompStream.data_type = Z_BINARY;

	int inflateResult;
	// Initialize the decompression stream structures on the first
	// invocation.
	if( !m_decompStreamInited )
	{
		inflateResult = inflateInit( &m_decompStream );

		if ( inflateResult != Z_OK )
		{
			printf( "inflateInit returned error: %d, msg: %s\n",
					inflateResult, m_decompStream.msg );
			return( FALSE );
		}
		m_decompStreamInited = TRUE;
	}

	inflateResult = Z_OK;

	// Process buffer full of data until no more to process, or
	// some type of inflater error, or Z_STREAM_END.
	while( remaining > 0 && inflateResult == Z_OK )
	{
		int toRead;
		if( remaining > BUFFER_SIZE )
		{
			toRead = BUFFER_SIZE;
		}
		else
		{
			toRead = remaining;
		}

		// Fill the buffer, obtaining data from the server.
		if( !readFromServer( m_buffer, toRead ) )
		{
			return( FALSE );
		}

		m_decompStream.next_in  = ( Bytef * ) m_buffer;
		m_decompStream.avail_in = toRead;

		// Need to uncompress buffer full.
		inflateResult = inflate( &m_decompStream, Z_SYNC_FLUSH );

		/* We never supply a dictionary for compression. */
		if( inflateResult == Z_NEED_DICT )
		{
			printf( "zlib inflate needs a dictionary!\n" );
			return( FALSE );
		}
		if ( inflateResult < 0 )
		{
			printf( "zlib inflate returned error: %d, msg: %s\n",
					inflateResult, m_decompStream.msg );
			return( FALSE );
		}

		// Result buffer allocated to be at least large enough.
		// We should never run out of space!
		if( m_decompStream.avail_in > 0 &&
						m_decompStream.avail_out <= 0 )
		{
			printf( "zlib inflate ran out of space!\n" );
			return( FALSE );
		}

		remaining -= toRead;

	} // while ( remaining > 0 )

	if ( inflateResult == Z_OK )
	{
		m_screen.copyRect( rx, ry, rw, rh, (QRgb *) m_rawBuffer );
	}
	else
	{
		printf( "zlib inflate returned error: %d, msg: %s\n",
					inflateResult, m_decompStream.msg );
		return( FALSE );
	}

	return( TRUE );
}



#define TIGHT_MIN_TO_COMPRESS 12



// type declarations

typedef void( ivsConnection:: *filterPtr )( Q_UINT16, Q_UINT32 * );



// Definitions

bool ivsConnection::handleTight( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw,
								Q_UINT16 rh )
{
	QRgb fill_color;
	Q_UINT8 comp_ctl;

	if( !readFromServer( (char *) &comp_ctl, 1 ) )
	{
		return( FALSE );
	}

	// Flush zlib streams if we are told by the server to do so.
	for( Q_UINT8 stream_id = 0; stream_id < 4; stream_id++ )
	{
		if( ( comp_ctl & 1 ) && m_zlibStreamActive[stream_id] )
		{
			if( inflateEnd( &m_zlibStream[stream_id] ) != Z_OK &&
					m_zlibStream[stream_id].msg != NULL )
			{
				printf( "inflateEnd: %s\n",
						m_zlibStream[stream_id].msg );
			}
			m_zlibStreamActive[stream_id] = FALSE;
		}
		comp_ctl >>= 1;
	}

	// Handle solid rectangles.
	if( comp_ctl == rfbTightFill )
	{
		if( !readFromServer( (char*)&fill_color,
							sizeof( fill_color ) ) )
		{
			return( FALSE );
		}
		m_screen.fillRect( rx, ry, rw, rh, fill_color );
		return( TRUE );
	}

	if( comp_ctl == rfbTightJpeg )
	{
#ifdef HAVE_LIBJPEG
		return( decompressJpegRect( rx, ry, rw, rh ) );
#else
		return ( -1 );
#endif
	}


	// Quit on unsupported subencoding value.
	if( comp_ctl > rfbTightMaxSubencoding)
	{
		printf( "Tight encoding: bad subencoding value received.\n" );
		return( FALSE );
	}

	// Here primary compression mode handling begins.
	// Data was processed with optional filter + zlib compression.
	filterPtr filter_function;
	Q_UINT8 bits_pixel;

	// First, we should identify a filter to use.
	if( ( comp_ctl & rfbTightExplicitFilter ) != 0 )
	{
		Q_UINT8 filter_id;
		if( !readFromServer( (char*) &filter_id, 1 ) )
		{
			return( FALSE );
		}

		switch( filter_id )
		{
			case rfbTightFilterCopy:
				filter_function = &ivsConnection::filterCopy;
				bits_pixel = initFilterCopy( rw, rh );
				break;
			case rfbTightFilterPalette:
				filter_function = &ivsConnection::filterPalette;
				bits_pixel = initFilterPalette( rw, rh );
				break;
			case rfbTightFilterGradient:
				filter_function =
						&ivsConnection::filterGradient;
				bits_pixel = initFilterGradient( rw, rh );
				break;
			default:
				printf( "Tight encoding: unknown filter code "
								"received.\n" );
				return( FALSE );
		}
	}
	else
	{
		filter_function = &ivsConnection::filterCopy;
		bits_pixel = initFilterCopy( rw, rh );
	}

	if( bits_pixel == 0 )
	{
		printf( "Tight encoding: error receiving palette.\n" );
		return( FALSE );
	}


	// Determine if the data should be decompressed or just copied.
	Q_UINT16 row_size = ( (int) rw * bits_pixel + 7 ) / 8;
	if( rh * row_size < TIGHT_MIN_TO_COMPRESS )
	{
		if( !readFromServer( (char*)m_buffer, rh * row_size ) )
		{
			return( FALSE );
		}

		QRgb * buffer2 = (QRgb *) &m_buffer[TIGHT_MIN_TO_COMPRESS * 4];
		( this->*( filter_function ) )( rh, (Q_UINT32 *)buffer2 );
		m_screen.copyRect( rx, ry, rw, rh, buffer2 );
		return( TRUE );
	}

	// Read the length (1..3 bytes) of compressed data following.
	int compressed_len = (int)readCompactLen();
	if( compressed_len <= 0 )
	{
		printf( "Incorrect data received from the server.\n" );
		return( FALSE );
	}


	// Now let's initialize compression stream if needed.
	Q_UINT8 stream_id = comp_ctl & 0x03;
	z_streamp zs = &m_zlibStream[stream_id];
	if( !m_zlibStreamActive[stream_id] )
	{
		zs->zalloc = Z_NULL;
		zs->zfree = Z_NULL;
		zs->opaque = Z_NULL;
		int err = inflateInit( zs );
		if( err != Z_OK )
		{
			if( zs->msg != NULL )
			{
				printf( "InflateInit error: %s.\n", zs->msg );
			}
			return( FALSE );
		}
		m_zlibStreamActive[stream_id] = TRUE;
	}


	// Read, decode and draw actual pixel data in a loop.
	int buffer_size = BUFFER_SIZE * bits_pixel / ( bits_pixel+32 ) &
								0xfffffffc;
	if( row_size > buffer_size )
	{
		// Should be impossible when BUFFER_SIZE >= 16384
		printf( "Internal error: incorrect m_buffer size.\n" );
		return( FALSE );
	}
	QRgb * buffer2 = (QRgb *) &m_buffer[buffer_size];


	Q_UINT16 rows_processed = 0;
	int extra_bytes = 0;
	int portion_len;

	while( compressed_len > 0 )
	{
		if( compressed_len > ZLIB_BUFFER_SIZE )	
		{
			portion_len = ZLIB_BUFFER_SIZE;
		}
		else
		{
			portion_len = compressed_len;
		}

		if( !readFromServer( (char*)m_zlibBuffer, portion_len ) )
		{
			return( FALSE );
		}

		compressed_len -= portion_len;

		zs->next_in = (Bytef *)m_zlibBuffer;
		zs->avail_in = portion_len;

		do
		{
			zs->next_out = (Bytef *) &m_buffer[extra_bytes];
			zs->avail_out = buffer_size - extra_bytes;

			int err = inflate(zs, Z_SYNC_FLUSH);
			if( err == Z_BUF_ERROR )	// Input exhausted --
							// no problem.
			{
				break;
			}
			if( err != Z_OK && err != Z_STREAM_END )
			{
				if( zs->msg != NULL )
				{
					printf( "Inflate error: %s.\n",
								zs->msg );
				}
				else
				{
					printf( "Inflate error: %d.\n", err );
				}
				return( FALSE );
			}

			Q_UINT16 num_rows = (Q_UINT16)( ( buffer_size -
							zs->avail_out ) /
								(int)row_size );

			( this->*( filter_function ) )( num_rows,
							(Q_UINT32 *)buffer2 );
			extra_bytes = buffer_size - zs->avail_out - num_rows *
								row_size;
			if( extra_bytes > 0 )
			{
				memcpy( m_buffer,
						&m_buffer[num_rows * row_size],
								extra_bytes );
			}

			m_screen.copyRect( rx, ry+rows_processed, rw,
							num_rows, buffer2 );
			rows_processed += num_rows;
		}
		while( zs->avail_out == 0 );
	}

	if( rows_processed != rh )
	{
		printf( "Incorrect number of scan lines after "
							"decompression.\n" );
		return( FALSE );
	}

	return( TRUE );
}



/*----------------------------------------------------------------------------
 *
 * Filter stuff.
 *
 */

Q_UINT8 ivsConnection::initFilterCopy( Q_UINT16 rw, Q_UINT16/* rh*/ )
{
	m_rectWidth = rw;

	return( 32 );
}




void ivsConnection::filterCopy( Q_UINT16 num_rows, Q_UINT32 * dst )
{
	memcpy( dst, m_buffer, num_rows * m_rectWidth * sizeof( Q_UINT32 ) );
}




Q_UINT8 ivsConnection::initFilterGradient( Q_UINT16 rw, Q_UINT16/* rh*/ )
{
	m_rectWidth = rw;
	memset( m_tightPrevRow, 0, rw * 3 * sizeof( Q_UINT16 ) );

	return( sizeof( QRgb ) );
}




void ivsConnection::filterGradient( Q_UINT16 num_rows, Q_UINT32 * dst )
{
	Q_UINT32 * src = (Q_UINT32 *) m_buffer;
	Q_UINT16 * that_row = (Q_UINT16 *) m_tightPrevRow;
	Q_UINT16 this_row[2048*3];
	Q_UINT16 pix[3];
	Q_UINT16 max[3] =
	{
		s_localDisplayFormat.redMax,
		s_localDisplayFormat.greenMax,
		s_localDisplayFormat.blueMax
	} ;
	int shift[3] =
	{
		s_localDisplayFormat.redShift,
		s_localDisplayFormat.greenShift,
		s_localDisplayFormat.blueShift
	} ;
	int est[3];


	for( Q_UINT16 y = 0; y < num_rows; y++ )
	{
		// First pixel in a row
		for( Q_UINT8 c = 0; c < 3; c++ )
		{
			pix[c] = (Q_UINT16)((src[y*m_rectWidth] >> shift[c]) +
							that_row[c] & max[c]);
			this_row[c] = pix[c];
		}
		dst[y*m_rectWidth] = RGB_TO_PIXEL( pix[0], pix[1], pix[2] );
		// Remaining pixels of a row 
		for( Q_UINT16 x = 1; x < m_rectWidth; x++ )
		{
			for( Q_UINT8 c = 0; c < 3; c++ )
			{
				est[c] = (int)that_row[x*3+c] +
						(int)pix[c] -
						(int)that_row[(x-1)*3+c];
				if( est[c] > (int)max[c] )
				{
					est[c] = (int)max[c];
				}
				else if( est[c] < 0 )
				{
					est[c] = 0;
				}
				pix[c] = (Q_UINT16)((src[y*m_rectWidth+x] >>
						shift[c]) + est[c] & max[c]);
				this_row[x*3+c] = pix[c];
			}
			dst[y*m_rectWidth+x] = RGB_TO_PIXEL( pix[0], pix[1],
								pix[2] );
		}
		memcpy( that_row, this_row, m_rectWidth * 3 *
							sizeof( Q_UINT16 ) );
	}
}




Q_UINT8 ivsConnection::initFilterPalette( Q_UINT16 rw, Q_UINT16/* rh*/ )
{
	Q_UINT8 num_colors;

	m_rectWidth = rw;

	if( !readFromServer( (char*)&num_colors, sizeof( num_colors ) ) )
	{
		return( 0 );
	}

	m_rectColors = (Q_UINT16) num_colors;
	if( ++m_rectColors < 2 )
	{
		return( 0 );
	}

	if( !readFromServer( (char*)&m_tightPalette, m_rectColors *
							sizeof( Q_UINT32 ) ) )
	{
		return( 0 );
	}

	return( ( m_rectColors == 2 ) ? 1 : 8 );
}




void ivsConnection::filterPalette( Q_UINT16 num_rows, Q_UINT32 * dst )
{
	Q_UINT8 * src = (Q_UINT8 *)m_buffer;
	Q_UINT32 * palette = (Q_UINT32 *) m_tightPalette;

	if( m_rectColors == 2 )
	{
		const Q_UINT16 w = (m_rectWidth + 7) / 8;
		for( Q_UINT16 y = 0; y < num_rows; ++y )
		{
			int x;
			for( x = 0; x < m_rectWidth/8; x++ )
			{
				for( Q_INT8 b = 7; b >= 0; b-- )
				{
					dst[y*m_rectWidth+x*8+7-b] =
						palette[src[y*w+x] >> b & 1];
				}
			}
			for( Q_INT8 b = 7; b >= 8 - m_rectWidth % 8; b-- )
			{
				dst[y*m_rectWidth+x*8+7-b] =
						palette[src[y*w+x] >> b & 1];
			}
		}
	}
	else
	{
		for( Q_UINT16 y = 0; y < num_rows; y++ )
		{
			for( Q_UINT16 x = 0; x < m_rectWidth; x++ )
			{
				dst[y*m_rectWidth+x] =
					palette[(int)src[y*m_rectWidth+x]];
			}
		}
	}
}


#ifdef HAVE_LIBJPEG
/*----------------------------------------------------------------------------
 *
 * JPEG decompression.
 *
 */


void jpegInitSource( jpeglib::j_decompress_ptr )
{
}




jpeglib::boolean jpegFillInputBuffer( jpeglib::j_decompress_ptr )
{
	printf( "ERROR: jpegFillInputBuffer(...) called (not implemented, "
					"because it should not be needed\n" );
	return( 0 );
}




void jpegSkipInputData( jpeglib::j_decompress_ptr, long )
{
	printf( "ERROR: jpegSkipInputData(...) called (not implemented, "
					"because it should not be needed\n" );
}




void jpegTermSource( jpeglib::j_decompress_ptr )
{
}



using jpeglib::jpeg_decompress_struct;

bool ivsConnection::decompressJpegRect( Q_UINT16 x, Q_UINT16 y, Q_UINT16 w,
								Q_UINT16 h )
{
	int compressed_len = (int) readCompactLen();
	if( compressed_len <= 0 )
	{
		printf( "Incorrect data received from the server.\n" );
		return( FALSE );
	}

	Q_UINT8 compressed_data[compressed_len];

	if( !readFromServer( (char*)compressed_data, compressed_len ) )
	{
		return( FALSE );
	}

	struct jpeglib::jpeg_error_mgr jerr;
	struct jpeglib::jpeg_decompress_struct cinfo;
	cinfo.err = jpeglib::jpeg_std_error( &jerr );
	jpeglib::jpeg_create_decompress( &cinfo );

	//jpegSetSrcManager (&cinfo, compressed_data, compressed_len);
	m_jpegSrcManager.init_source = jpegInitSource;
	m_jpegSrcManager.fill_input_buffer = jpegFillInputBuffer;
	m_jpegSrcManager.skip_input_data = jpegSkipInputData;
	m_jpegSrcManager.resync_to_restart = jpeglib::jpeg_resync_to_restart;
	m_jpegSrcManager.term_source = jpegTermSource;
	m_jpegSrcManager.next_input_byte = (jpeglib::JOCTET *) compressed_data;
	m_jpegSrcManager.bytes_in_buffer = (size_t) compressed_len;

	cinfo.src = &m_jpegSrcManager;


	jpeglib::jpeg_read_header( &cinfo, TRUE );
	cinfo.out_color_space = jpeglib::JCS_RGB;

	jpeglib::jpeg_start_decompress( &cinfo );
	if( cinfo.output_width != w || cinfo.output_height != h ||
						cinfo.output_components != 3 )
	{
		printf( "Tight Encoding: Wrong JPEG data received.\n" );
		jpeglib::jpeg_destroy_decompress( &cinfo );
		return( FALSE );
	}

	jpeglib::JSAMPROW row_pointer[1];
	row_pointer[0] = (jpeglib::JSAMPROW) m_buffer;
	int dy = 0;
	while( cinfo.output_scanline < cinfo.output_height )
	{
		jpeglib::jpeg_read_scanlines( &cinfo, row_pointer, 1 );
		Q_UINT32 * pixel_ptr = (Q_UINT32 *) &m_buffer[BUFFER_SIZE / 2];
		for( Q_UINT16 dx = 0; dx < w; dx++ )
		{
			*pixel_ptr++ = RGB_TO_PIXEL( m_buffer[dx*3],
							m_buffer[dx*3+1],
							m_buffer[dx*3+2] );
		}
		m_screen.copyRect( x, y+dy, w, 1, (QRgb *)
						&m_buffer[BUFFER_SIZE / 2] );
		dy++;
	}

	jpeglib::jpeg_finish_decompress( &cinfo );

	jpeglib::jpeg_destroy_decompress( &cinfo );

	return( TRUE );
}

#endif	/* LIBJPEG */

#endif	/* LIBZ */




bool ivsConnection::handleCursorPos( const Q_UINT16 _x, const Q_UINT16 _y )
{
	// move cursor and update appropriate region
	QRegion ch_reg = QRect( m_cursorPos - m_cursorHotSpot,
							m_cursorShape.size() );
	m_cursorPos = QPoint( _x, _y );
	ch_reg += QRect( m_cursorPos - m_cursorHotSpot, m_cursorShape.size() );

	if( parent() != NULL )
	{
		regionChangedEvent rche( ch_reg );
		QApplication::sendEvent( parent(), &rche );
	}

	if( m_quality != QualityDemoPurposes )
	{
		emit regionUpdated( ch_reg );
	}

	return( TRUE );
}




bool ivsConnection::handleCursorShape( const Q_UINT16 _xhot,
					const Q_UINT16 _yhot,
					const Q_UINT16 _width,
					const Q_UINT16 _height,
					const Q_UINT32 _enc )
{
	int bytesPerPixel = s_localDisplayFormat.bitsPerPixel / 8;
	size_t bytesPerRow = ( _width + 7 ) / 8;
	size_t bytesMaskData = bytesPerRow * _height;

	if( _width * _height == 0 )
	{
		return( TRUE );
	}

	// Allocate memory for pixel data and temporary mask data.

	Q_UINT8 * rcSource = new Q_UINT8[_width * _height * bytesPerPixel];
	if( rcSource == NULL )
	{
		return( FALSE );
	}

	Q_UINT8 * rcMask = new Q_UINT8[bytesMaskData];
	if( rcMask == NULL )
	{
		delete[] rcSource;
		return( FALSE );
	}

	// Read and decode cursor pixel data, depending on the encoding type.

	if( _enc == rfbEncodingXCursor )
	{
		rfbXCursorColors rgb;
		// Read and convert background and foreground colors.
		if( !readFromServer( (char *) &rgb, sz_rfbXCursorColors ) )
		{
			delete[] rcSource;
			delete[] rcMask;
			return( FALSE );
		}
    		Q_UINT32 colors[2] = {
	RGB_TO_PIXEL( rgb.backRed, rgb.backGreen, rgb.backBlue ),
	RGB_TO_PIXEL( rgb.foreRed, rgb.foreGreen, rgb.foreBlue )
					} ;

		// Read 1bpp pixel data into a temporary buffer.
		if( !readFromServer( (char*) rcMask, bytesMaskData ) )
		{
			delete[] rcSource;
			delete[] rcMask;
			return( FALSE );
		}

		// Convert 1bpp data to byte-wide color indices.
		Q_UINT8 * ptr = rcSource;
		for( int y = 0; y < _height; ++y )
		{
			int x = 0;
			for( ; x < _width / 8; ++x )
			{
				for( int b = 7; b >= 0; --b )
				{
					*ptr = rcMask[y * bytesPerRow + x]
								>> b & 1;
					ptr += bytesPerPixel;
				}
			}
			for( int b = 7; b > 7 - _width % 8; --b )
			{
				*ptr = rcMask[y * bytesPerRow + x] >> b & 1;
				ptr += bytesPerPixel;
			}
		}

		// Convert indices into the actual pixel values.
		switch( bytesPerPixel )
		{
			case 1:
			{
				for( int x = 0; x < _width * _height; ++x )
				{
					rcSource[x] = (Q_UINT8)
							colors[rcSource[x]];
				}
				break;
			}
			case 2:
			{
				for( int x = 0; x < _width * _height; ++x )
				{
					((Q_UINT16*) rcSource)[x] =
						(Q_UINT16)colors[rcSource[x*2]];
				}
				break;
			}
			case 4:
			{
				for( int x = 0; x < _width * _height; ++x )
				{
					((Q_UINT32 *) rcSource)[x] =
							colors[rcSource[x * 4]];
					break;
				}
			}
		}
	}
	else	// rich-cursor encoding
	{
		if( !readFromServer((char *) rcSource, _width * _height *
							bytesPerPixel ) )
		{
			delete[] rcSource;
			delete[] rcMask;
			return( FALSE );
		}
	}

	// Read mask data.

	if( !readFromServer( (char*) rcMask, bytesMaskData ) )
	{
		delete[] rcSource;
		delete[] rcMask;
		return( FALSE );
	}

	QRegion ch_reg = QRect( m_cursorPos - m_cursorHotSpot,
							m_cursorShape.size() );
	const QImage cur( rcSource, _width, _height, QImage::Format_RGB32 );
	m_cursorShape = QPixmap::fromImage( cur );
	m_cursorShape.setMask( QBitmap::fromData( QSize( _width, _height ),
						rcMask, QImage::Format_Mono ) );
	m_cursorHotSpot = QPoint( _xhot, _yhot );


	ch_reg += QRect( m_cursorPos - m_cursorHotSpot, m_cursorShape.size() );

	// make sure, area around old cursor is updated and new cursor is
	// painted
	if( parent() != NULL )
	{
		regionChangedEvent rche( ch_reg );
		QApplication::sendEvent( parent(), &rche );
	}

	emit cursorShapeChanged();
	if( m_quality != QualityDemoPurposes )
	{
		emit regionUpdated( ch_reg );
	}

	delete[] rcSource;
	delete[] rcMask;


	return( TRUE );
}



#ifdef BUILD_ICA
bool ivsConnection::handleItalc( Q_UINT16 rx, Q_UINT16 ry, Q_UINT16 rw,
								Q_UINT16 rh )
{
	italcRectEncodingHeader hdr;
	if( !readFromServer( (char *) &hdr, sizeof( hdr ) ) )
	{
		return( FALSE );
	}

	if( !hdr.compressed )
	{
		return( handleRaw( rx, ry, rw, rh ) );
	}

	hdr.bytesLZO = swap32IfLE( hdr.bytesLZO );
	hdr.bytesRLE = swap32IfLE( hdr.bytesRLE );

	Q_UINT8 * lzo_data = new Q_UINT8[hdr.bytesLZO];

	if( !readFromServer( (char *) lzo_data, hdr.bytesLZO ) )
	{
		return( FALSE );
	}

	Q_UINT8 * rle_data = new Q_UINT8[hdr.bytesRLE];

	lzo_uint decomp_bytes = 0;
	lzo1x_decompress( (const unsigned char *) lzo_data,
				(lzo_uint) hdr.bytesLZO,
				(unsigned char *) rle_data,
				(lzo_uint *) &decomp_bytes, NULL );
	if( decomp_bytes != hdr.bytesRLE )
	{
		printf( "expected and real size of decompressed data do not "
				"match!\n" );
	}

	QRgb * dst = (QRgb *) m_screen.scanLine( ry ) + rx;
	Q_UINT16 dx = 0;
	for( Q_UINT32 i = 0; i < hdr.bytesRLE; i+=4 )
	{
		const QRgb val = *( (QRgb*)( rle_data + i ) ) & 0xffffff;
		for( Q_UINT16 j = 0; j <= rle_data[i+3]; ++j )
		{
			*dst = val;//[i+1];
			if( ++dx >= rw )
			{
				dst = (QRgb *) m_screen.scanLine( ++ry ) + rx;
				dx = 0;
			}
			else
			{
				++dst;
			}
		}
	}

	if( dx != 0 )
	{
		printf("dx(%d) != 0\n",dx);
	}

	delete[] lzo_data;
	delete[] rle_data;

	return( TRUE );
}
#endif



#include "ivs_connection.moc"

