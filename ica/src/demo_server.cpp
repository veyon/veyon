/*
 * demo_server.cpp - multi-threaded slim VNC-server for demo-purposes (lot of
 *                   clients accessing server in read-only-mode)
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


#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtGui/QCursor>


#include "demo_server.h"
#include "isd_server.h"
#include "italc_rfb_ext.h"
#include "minilzo.h"



demoServer::demoServer( quint16 _port ) :
	QTcpServer(),
	m_conn( new ivsConnection(
			QHostAddress( QHostAddress::LocalHost ).toString() +
					":" + QString::number( _port ),
					ivsConnection::QualityDemoPurposes ) ),
	m_updaterThread( new updaterThread( m_conn ) )
{
	if( listen() == FALSE )
	{
		qCritical( "Could not start demo-server!\n" );
		return;
	}

	m_updaterThread->start();

	connect( this, SIGNAL( newConnection() ),
			this, SLOT( acceptNewConnection() ) );

}




demoServer::~demoServer()
{
	delete m_updaterThread;
	delete m_conn;
}




void demoServer::acceptNewConnection( void )
{
	new demoServerClient( nextPendingConnection(), m_conn );
}






demoServer::updaterThread::updaterThread( ivsConnection * _ic ) :
	QThread(),
	m_conn( _ic ),
	m_quit( FALSE )
{
}




demoServer::updaterThread::~updaterThread()
{
	m_quit = TRUE;
	wait();
}




void demoServer::updaterThread::run( void )
{
	if( m_conn->open() != ivsConnection::Connected )
	{
		qCritical( "Could not connect to local IVS!\n" );
		return;
	}

	int i = 0;
	while( !m_quit )
	{
		msleep( 25 );
		m_conn->handleServerMessages( ( i = ( i + 1 ) % 2 ) == 0 );
	}
}











demoServerClient::demoServerClient( QTcpSocket * _sock,
						const ivsConnection * _conn ) :
	QThread(),
	m_changedRegion(),
	m_cursorShapeChanged( TRUE ),
	m_dataMutex(),
	m_lastCursorPos(),
	m_sock( _sock ),
	m_conn( _conn ),
	m_lzoWorkMem( new Q_UINT8[sizeof( lzo_align_t ) *
			( ( ( LZO1X_1_MEM_COMPRESS ) +
			    		( sizeof( lzo_align_t ) - 1 ) ) /
				 		sizeof( lzo_align_t ) ) ] ),
	m_bandwidthTimer( new QTime() ),
	m_bytesOut( 0 ),
	m_frames( 0 )
{
	m_bandwidthTimer->start();

	socketDevice sd( qtcpsocketDispatcher, m_sock );
	if( !isdServer::protocolInitialization( sd,
					ItalcAuthAppInternalChallenge, TRUE ) )
	{
		printf( "demoServerClient: protocol initialization failed\n" );
		deleteLater();
		return;
	}

	rfbClientInitMsg ci;

	if( !sd.read( (char *) &ci, sizeof( ci ) ) )
	{
		deleteLater();
		return;
	}

	rfbServerInitMsg si = _conn->m_si;
	si.framebufferWidth = swap16IfLE( si.framebufferWidth );
	si.framebufferHeight = swap16IfLE( si.framebufferHeight );
	si.format.redMax = swap16IfLE( si.format.redMax );
	si.format.greenMax = swap16IfLE( si.format.greenMax );
	si.format.blueMax = swap16IfLE( si.format.blueMax );
	si.nameLength = swap32IfLE( si.nameLength );

	if( !sd.write( ( const char *) &si, sizeof( si ) ) )
	{
		deleteLater();
		return;
	}

	char * desktop_name = new char[_conn->m_si.nameLength+1];
	desktop_name[0] = 0;

	if( !sd.write( desktop_name, _conn->m_si.nameLength ) )
	{
		deleteLater();
		return;
	}

	delete[] desktop_name;


	rfbSetPixelFormatMsg spf;

	if( !sd.read( (char *) &spf, sizeof( spf ) ) )
	{
		deleteLater();
		return;
	}

	char buf[sizeof( rfbSetPixelFormatMsg ) + MAX_ENCODINGS *
							sizeof( Q_UINT32 )];
	rfbSetEncodingsMsg * se = (rfbSetEncodingsMsg *) buf;

	if( !sd.read( (char *) se, sizeof( *se ) ) )
	{
		deleteLater();
		return;
	}
	se->nEncodings = swap16IfLE( se->nEncodings );

	Q_UINT32 * encs = (Q_UINT32 *)( &buf[sizeof(rfbSetEncodingsMsg)] );

	if( !sd.read( (char *) encs, se->nEncodings * sizeof( Q_UINT32 ) ) )
	{
		deleteLater();
		return;
	}

	bool has_italc_encoding = FALSE;
	for( Q_UINT32 i = 0; i < se->nEncodings; ++i )
	{
		if( swap32IfLE( encs[i] ) == rfbEncodingItalc )
		{
			has_italc_encoding = TRUE;
		}
	}

	if( !has_italc_encoding )
	{
		printf( "demoServerClient: client has no italc-encoding\n" );
		deleteLater();
		return;
	}

	// for some reason we have to do this to make the following connection
	// working
	qRegisterMetaType<QRegion>( "QRegion" );

	connect( m_conn, SIGNAL( cursorShapeChanged() ),
				this, SLOT( updateCursorShape() ) );
	connect( m_conn, SIGNAL( regionUpdated( const QRegion & ) ),
				this, SLOT( updateRegion( const QRegion & ) ) );


	// first time send a key-frame
	updateRegion( m_conn->screen().rect() );

	start();
}




demoServerClient::~demoServerClient()
{
	exit();
	wait();
	delete[] m_lzoWorkMem;
}




void demoServerClient::updateRegion( const QRegion & _reg )
{
	m_dataMutex.lock();
	m_changedRegion += _reg;
	m_dataMutex.unlock();
}




void demoServerClient::updateCursorShape( void )
{
	m_cursorShapeChanged = TRUE;
}




void demoServerClient::moveCursor( void )
{
	if( m_lastCursorPos != QCursor::pos() )
	{
		m_dataMutex.lock();
		m_lastCursorPos = QCursor::pos();
		const rfbFramebufferUpdateMsg m =
		{
			rfbFramebufferUpdate,
			0,
			swap16IfLE( 1 )
		} ;

		m_sock->write( (const char *) &m, sizeof( m ) );

		const rfbRectangle rr =
		{
			swap16IfLE( QCursor::pos().x() ),
			swap16IfLE( QCursor::pos().y() ),
			swap16IfLE( 0 ),
			swap16IfLE( 0 )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			swap32IfLE( rfbEncodingPointerPos )
		} ;

		m_sock->write( (const char *) &rh, sizeof( rh ) );

		m_dataMutex.unlock();
	}
	QTimer::singleShot( 20, this, SLOT( moveCursor() ) );
}




void demoServerClient::processClient( void )
{
	m_dataMutex.lock();
	while( m_sock->bytesAvailable() > 0 )
	{
		Q_UINT8 cmd;
		if( m_sock->read( (char *) &cmd, sizeof( cmd ) ) <= 0 )
		{
			printf( "could not read cmd\n" );
			continue;
		}

		if( cmd != rfbFramebufferUpdateRequest )
		{
			continue;
		}

		if( m_changedRegion.isEmpty() )
		{
			continue;
		}

		// extract single (non-overlapping) rects out of changed region
		// this way we avoid lot of simliar/overlapping rectangles,
		// e.g. if we didn't get an update-request for a quite long time
		// and there were a lot of updates - at the end we don't send
		// more than the whole screen one time
		QVector<QRect> rects = m_changedRegion.rects();

		// no we gonna post all changed rects!
		const rfbFramebufferUpdateMsg m =
		{
			rfbFramebufferUpdate,
			0,
			swap16IfLE( rects.size() +
					( m_cursorShapeChanged ? 1 : 0 ) )
		} ;

		m_sock->write( (const char *) &m, sizeof( m ) );

		// process each rect
		for( QVector<QRect>::const_iterator it = rects.begin();
						it != rects.end(); ++it )
		{
			const rfbRectangle rr =
			{
				swap16IfLE( it->x() ),
				swap16IfLE( it->y() ),
				swap16IfLE( it->width() ),
				swap16IfLE( it->height() )
			} ;

			const rfbFramebufferUpdateRectHeader rh =
			{
				rr,
				swap32IfLE( rfbEncodingItalc )
			} ;

			m_sock->write( (const char *) &rh, sizeof( rh ) );

			const QImage & i = m_conn->screen();
			italcRectEncodingHeader hdr = { 0, 0, 0 } ;

			const Q_UINT16 w = it->width();
			const Q_UINT16 h = it->height();

			// we only compress if it's enough data, otherwise
			// there's too much overhead
			if( w * h > 1024 )
			{

	hdr.compressed = 1;
	QRgb last_pix = *( (QRgb *) i.scanLine( it->y() ) + it->x() );
	Q_UINT8 rle_cnt = 0;
	Q_UINT8 rle_sub = 1;
	Q_UINT8 * out = new Q_UINT8[w * h * sizeof( QRgb )];
	Q_UINT8 * out_ptr = out;
	for( Q_UINT16 y = it->y(); y < it->y() + h; ++y )
	{
		const QRgb * data = ( (const QRgb *) i.scanLine( y ) ) +
									it->x();
		for( Q_UINT16 x = 0; x < w; ++x )
		{
			if( data[x] != last_pix || rle_cnt > 254 )
			{
				*( (QRgb *) out_ptr ) = last_pix;
				*( out_ptr + 3 ) = rle_cnt - rle_sub;
				out_ptr += 4;
				last_pix = data[x];
				rle_cnt = rle_sub = 0;
			}
			else
			{
				++rle_cnt;
			}
		}
	}

	// flush RLE-loop
	*( (QRgb *) out_ptr ) = last_pix;
	*( out_ptr + 3 ) = rle_cnt;
	out_ptr += 4;

	hdr.bytesRLE = out_ptr - out;
	hdr.bytesLZO = hdr.bytesRLE + hdr.bytesRLE / 16 + 67;
	Q_UINT8 * comp = new Q_UINT8[hdr.bytesLZO];
	lzo1x_1_compress( (const unsigned char *) out, (lzo_uint) hdr.bytesRLE,
				(unsigned char *) comp,
				(lzo_uint *) &hdr.bytesLZO, m_lzoWorkMem );
	m_bytesOut += hdr.bytesLZO + 32;
	hdr.bytesRLE = swap32IfLE( hdr.bytesRLE );
	hdr.bytesLZO = swap32IfLE( hdr.bytesLZO );

	m_sock->write( (const char *) &hdr, sizeof( hdr ) );
	m_sock->write( (const char *) comp, swap32IfLE( hdr.bytesLZO ) );
	delete[] out;
	delete[] comp;

			}
			else
			{

	m_sock->write( (const char *) &hdr, sizeof( hdr ) );
	for( Q_UINT16 y = 0; y < h; ++y )
	{
		m_sock->write( (const char *)
			( (const QRgb *) i.scanLine( it->y() + y ) + it->x() ),
							w * sizeof( QRgb ) );
	}
	m_bytesOut += w * h * sizeof( QRgb ) + 32;

			}
		}

		if( m_cursorShapeChanged )
		{
			const QPixmap cur = m_conn->cursorShape();
			const rfbRectangle rr =
			{
				swap16IfLE( m_conn->cursorHotSpot().x() ),
				swap16IfLE( m_conn->cursorHotSpot().y() ),
				swap16IfLE( cur.width() ),
				swap16IfLE( cur.height() )
			} ;

			const rfbFramebufferUpdateRectHeader rh =
			{
				rr,
				swap32IfLE( rfbEncodingItalcCursor )
			} ;

			m_sock->write( (const char *) &rh, sizeof( rh ) );

			QDataStream ds( m_sock );
			ds << cur;
		}

		// reset vars
		m_changedRegion = QRegion();
		m_cursorShapeChanged = FALSE;

	}

	++m_frames;
	m_dataMutex.unlock();
	m_sock->waitForBytesWritten();
	printf("kbps%d  fps:%d\n", m_bytesOut /
				( m_bandwidthTimer->elapsed() * 1024 / 1000 ),
				m_frames * 1000 / m_bandwidthTimer->elapsed() );
}




void demoServerClient::run( void )
{
	// we're now in our own thread, therefore re-parent socket for being
	// able to change it's thread-affinity afterwards
	m_sock->setParent( 0 );
	m_sock->moveToThread( this );

	connect( m_sock, SIGNAL( readyRead() ), this, SLOT( processClient() ) );
	connect( m_sock, SIGNAL( disconnected() ), this,
							SLOT( deleteLater() ) );

/*	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this, SLOT( moveCursor() ) );
	t->start( 20 );*/
	QTimer::singleShot( 20, this, SLOT( moveCursor() ) );
	//moveCursor();

	// ...and can run our own event-loop for optimal scheduling
	exec();

	// we have to delete the socket here as we affined it to this thread
	delete m_sock;
}



#include "demo_server.moc"

