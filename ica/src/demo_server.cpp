/*
 * demo_server.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
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
#include "ivs.h"


const int CURSOR_UPDATE_TIME = 30;


demoServer::demoServer( IVS * _ivs_conn, int _quality, quint16 _port ) :
	QTcpServer(),
	m_conn( new ivsConnection(
			QHostAddress( QHostAddress::LocalHost ).toString() +
					":" + QString::number(
						_ivs_conn->serverPort() ),
			static_cast<ivsConnection::quality>(
				ivsConnection::QualityDemoLow +
						qBound( 0, _quality, 2 ) ),
				_ivs_conn->runningInSeparateProcess() ) ),
	m_updaterThread( new updaterThread( m_conn ) )
{
	if( listen( QHostAddress::Any, _port ) == FALSE )
	{
		qCritical( "Could not start demo-server!\n" );
		return;
	}

	m_updaterThread->start(/* QThread::HighPriority*/ );

}




demoServer::~demoServer()
{
	delete m_updaterThread;
	delete m_conn;
}




void demoServer::incomingConnection( int _sd )
{
	new demoServerClient( _sd, m_conn );
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
	int i = 0;
	while( !m_quit )
	{
		while( !m_quit && m_conn->state() != ivsConnection::Connected &&
				m_conn->open() != ivsConnection::Connected )
		{
			qWarning( "Could not connect to local IVS!\n" );
			sleep( 1 );
		}
		msleep( 20 );
		m_conn->handleServerMessages( ( i = ( i + 1 ) % 2 ) == 0 );
	}
}











demoServerClient::demoServerClient( int _sd, const ivsConnection * _conn ) :
	QThread(),
	m_dataMutex(),
	m_changedRegion(),
	m_lastCursorPos(),
	m_cursorShapeChanged( TRUE ),
	m_cursorPosChanged( FALSE ),
	m_socketDescriptor( _sd ),
	m_sock( NULL ),
	m_conn( _conn ),
	m_lzoWorkMem( new Q_UINT8[sizeof( lzo_align_t ) *
			( ( ( LZO1X_1_MEM_COMPRESS ) +
			    		( sizeof( lzo_align_t ) - 1 ) ) /
				 		sizeof( lzo_align_t ) ) ] )
{
	QTimer::singleShot( CURSOR_UPDATE_TIME, this,
					SLOT( checkForCursorMovement() ) );
	start();
}




demoServerClient::~demoServerClient()
{
	exit();
	wait();
	delete[] m_lzoWorkMem;
}




void demoServerClient::updateRegion( const rectList & _reg )
{
	m_dataMutex.lock();
	m_changedRegion += _reg;
	m_dataMutex.unlock();
}




void demoServerClient::updateCursorShape( void )
{
	m_cursorShapeChanged = TRUE;
}




void demoServerClient::checkForCursorMovement( void )
{
	m_dataMutex.lock();
	if( m_lastCursorPos != QCursor::pos() )
	{
		m_lastCursorPos = QCursor::pos();
		m_cursorPosChanged = TRUE;
	}
	m_dataMutex.unlock();
	QTimer::singleShot( CURSOR_UPDATE_TIME, this,
					SLOT( checkForCursorMovement() ) );
}




void demoServerClient::moveCursor( void )
{
	m_dataMutex.lock();
	if( m_cursorPosChanged )
	{
		m_cursorPosChanged = FALSE;
		const rfbFramebufferUpdateMsg m =
		{
			rfbFramebufferUpdate,
			0,
			swap16IfLE( 1 )
		} ;

		m_sock->write( (const char *) &m, sizeof( m ) );

		const rfbRectangle rr =
		{
			swap16IfLE( m_lastCursorPos.x() ),
			swap16IfLE( m_lastCursorPos.y() ),
			swap16IfLE( 0 ),
			swap16IfLE( 0 )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			swap32IfLE( rfbEncodingPointerPos )
		} ;

		m_sock->write( (const char *) &rh, sizeof( rh ) );
		m_sock->flush();
	}
	m_dataMutex.unlock();
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
		//QVector<QRect> rects = m_changedRegion.rects();
		const rectList r = m_changedRegion.nonOverlappingRects();

		// no we gonna post all changed rects!
		const rfbFramebufferUpdateMsg m =
		{
			rfbFramebufferUpdate,
			0,
			swap16IfLE( r.size() +
					( m_cursorShapeChanged ? 1 : 0 ) )
		} ;

		m_sock->write( (const char *) &m, sizeof( m ) );
		// process each rect
		for( rectList::const_iterator it = r.begin();
							it != r.end(); ++it )
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
			}
		}

		if( m_cursorShapeChanged )
		{
			const QImage cur = m_conn->cursorShape();
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
		m_changedRegion.clear();
		m_cursorShapeChanged = FALSE;
	}
	//m_sock->waitForBytesWritten();
	m_sock->flush();

	m_dataMutex.unlock();
	//QTimer::singleShot( 40, this, SLOT( processClient() ) );
}




void demoServerClient::run( void )
{
	QMutexLocker ml( &m_dataMutex );

	QTcpSocket sock;
	m_sock = &sock;
	if( !m_sock->setSocketDescriptor( m_socketDescriptor ) )
	{
		printf( "could not set socket-descriptor in "
			"demoServerClient::run() - aborting.\n" );
		deleteLater();
		return;
	}

	socketDevice sd( qtcpsocketDispatcher, m_sock );
	if( !isdServer::protocolInitialization( sd,
						ItalcAuthHostBased, TRUE ) )
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

	rfbServerInitMsg si = m_conn->m_si;
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

	char * desktop_name = new char[m_conn->m_si.nameLength+1];
	desktop_name[0] = 0;

	if( !sd.write( desktop_name, m_conn->m_si.nameLength ) )
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
	qRegisterMetaType<rectList>( "rectList" );

	connect( m_conn, SIGNAL( cursorShapeChanged() ),
				this, SLOT( updateCursorShape() ) );
	connect( m_conn, SIGNAL( regionUpdated( const rectList & ) ),
			this, SLOT( updateRegion( const rectList & ) ) );

	ml.unlock();

	// first time send a key-frame
	updateRegion( m_conn->screen().rect() );

	//connect( m_sock, SIGNAL( readyRead() ), this, SLOT( processClient() ) );
	connect( m_sock, SIGNAL( disconnected() ), this,
							SLOT( deleteLater() ) );

	QTimer t;
	connect( &t, SIGNAL( timeout() ), this, SLOT( moveCursor() ),
							Qt::DirectConnection );
	t.start( CURSOR_UPDATE_TIME );
	QTimer t2;
	connect( &t2, SIGNAL( timeout() ), this, SLOT( processClient() ),
							Qt::DirectConnection );
	t2.start( 50 );
	//moveCursor();
	//processClient();
	// now run our own event-loop for optimal scheduling
	exec();
}



#include "demo_server.moc"

