#if 0
/*
 * demo_server.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
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


#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtGui/QCursor>


#include "demo_server.h"
#include "isd_server.h"
#include "italc_rfb_ext.h"
#include "minilzo.h"
#include "ivs.h"


const int CURSOR_UPDATE_TIME = 35;

int demoServer::s_numOfInstances = 0;


demoServer::demoServer( IVS * _ivs_conn, int _quality, quint16 _port,
							QTcpSocket * _parent ) :
	QTcpServer( _parent ),
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
	++s_numOfInstances;

	if( listen( QHostAddress::Any, _port ) == FALSE )
	{
		qCritical( "demoServer::demoServer(): "
					"could not start demo-server!" );
		return;
	}

	m_updaterThread->start(/* QThread::HighPriority*/ );

	checkForCursorMovement();
}




demoServer::~demoServer()
{
	QList<demoServerClient *> l;
	while( !( l = findChildren<demoServerClient *>() ).isEmpty() )
	{
		delete l.front();
	}
	--s_numOfInstances;

	delete m_updaterThread;
	delete m_conn;
}




void demoServer::checkForCursorMovement( void )
{
	m_cursorLock.lockForWrite();
	if( m_cursorPos != QCursor::pos() )
	{
		m_cursorPos = QCursor::pos();
	}
	m_cursorLock.unlock();
	QTimer::singleShot( CURSOR_UPDATE_TIME, this,
					SLOT( checkForCursorMovement() ) );
}




void demoServer::incomingConnection( int _sd )
{
	new demoServerClient( _sd, m_conn, this );
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
			qWarning( "demoServer::updaterThread::run(): "
					"could not connect to local IVS!" );
			sleep( 1 );
		}
		msleep( 20 );
		m_conn->handleServerMessages( ( i = ( i + 1 ) % 2 ) == 0 );
	}
	m_conn->gracefulClose();
}











demoServerClient::demoServerClient( int _sd, const ivsConnection * _conn,
							demoServer * _parent ) :
	QThread( _parent ),
	m_ds( _parent ),
	m_dataMutex(),
	m_changedRegion(),
	m_cursorShapeChanged( TRUE ),
	m_socketDescriptor( _sd ),
	m_sock( NULL ),
	m_conn( _conn ),
	m_otherEndianess( FALSE ),
	m_lzoWorkMem( new Q_UINT8[sizeof( lzo_align_t ) *
			( ( ( LZO1X_1_MEM_COMPRESS ) +
			    		( sizeof( lzo_align_t ) - 1 ) ) /
				 		sizeof( lzo_align_t ) ) ] )
{
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
	QPoint p = m_ds->cursorPos();
	if( p != m_lastCursorPos )
	{
		m_dataMutex.lock();
		m_lastCursorPos = p;
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
		m_sock->waitForBytesWritten();
		m_dataMutex.unlock();
	}
}




void demoServerClient::processClient( void )
{
	m_dataMutex.lock();
	while( m_sock->bytesAvailable() > 0 )
	{
		Q_UINT8 cmd;
		if( m_sock->read( (char *) &cmd, sizeof( cmd ) ) <= 0 )
		{
			qWarning( "demoServerClient::processClient(): "
							"could not read cmd" );
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
		const QVector<QRect> r = m_changedRegion.rects();

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
		for( QVector<QRect>::const_iterator it = r.begin();
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

			const int w = it->width();
			const int h = it->height();

			// we only compress if it's enough data, otherwise
			// there's too much overhead
			if( w * h > 1024 )
			{

	hdr.compressed = 1;
	QRgb last_pix = *( (QRgb *) i.scanLine( it->y() ) + it->x() );
	Q_UINT8 rle_cnt = 0;
	Q_UINT8 rle_sub = 1;
	Q_UINT8 * out = new Q_UINT8[w * h * sizeof( QRgb )+16];
	Q_UINT8 * out_ptr = out;
	for( int y = it->y(); y < it->y() + h; ++y )
	{
		const QRgb * data = ( (const QRgb *) i.scanLine( y ) ) +
									it->x();
		for( int x = 0; x < w; ++x )
		{
			if( data[x] != last_pix || rle_cnt > 254 )
			{
				*( (QRgb *) out_ptr ) = swap32IfBE( last_pix );
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
//printf("%d %d\n",hdr.bytesRLE, w*h*sizeof(QRgb)+16);
	lzo_uint bytes_lzo = hdr.bytesRLE + hdr.bytesRLE / 16 + 67;
	Q_UINT8 * comp = new Q_UINT8[bytes_lzo];
	lzo1x_1_compress( (const unsigned char *) out, (lzo_uint) hdr.bytesRLE,
				(unsigned char *) comp,
				&bytes_lzo, m_lzoWorkMem );
	hdr.bytesRLE = swap32IfLE( hdr.bytesRLE );
	hdr.bytesLZO = swap32IfLE( bytes_lzo );

	m_sock->write( (const char *) &hdr, sizeof( hdr ) );
	m_sock->write( (const char *) comp, swap32IfLE( hdr.bytesLZO ) );
	delete[] out;
	delete[] comp;

			}
			else
			{
	m_sock->write( (const char *) &hdr, sizeof( hdr ) );
	if( m_otherEndianess )
	{
		Q_UINT32 * buf = new Q_UINT32[w];
		for( int y = 0; y < h; ++y )
		{
			const QRgb * src = (const QRgb *) i.scanLine(
								it->y() + y ) +
									it->x();
			for( int x = 0; x < w; ++x, ++src )
			{
				buf[x] = swap32( *src );
			}
			m_sock->write( (const char *) buf, w * sizeof( QRgb ) );
		}
		delete[] buf;
	}
	else
	{
		for( int y = 0; y < h; ++y )
		{
			m_sock->write( (const char *)
				( (const QRgb *) i.scanLine( it->y() + y ) +
								it->x() ),
							w * sizeof( QRgb ) );
		}
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
		//m_changedRegion.clear();
		m_changedRegion = QRegion();
		m_cursorShapeChanged = FALSE;
	}
	m_sock->waitForBytesWritten();

	m_dataMutex.unlock();
}




void demoServerClient::run( void )
{
	QMutexLocker ml( &m_dataMutex );

	QTcpSocket sock;
	m_sock = &sock;
	if( !m_sock->setSocketDescriptor( m_socketDescriptor ) )
	{
		qCritical( "demoServerClient::run(): "
				"could not set socket-descriptor - aborting" );
		deleteLater();
		return;
	}

	socketDevice sd( qtcpsocketDispatcher, m_sock );
	if( !isdServer::protocolInitialization( sd,
						ItalcAuthHostBased, TRUE ) )
	{
		qCritical( "demoServerClient:::run(): "
					"protocol initialization failed" );
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
	si.format.bigEndian = ( QSysInfo::ByteOrder == QSysInfo::BigEndian )
									? 1 : 0;
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

	// we have to do server-side endianess-conversion in case it differs
	// between client and server
	if( spf.format.bigEndian != si.format.bigEndian )
	{
		m_otherEndianess = TRUE;
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
		qCritical( "demoServerClient::run(): "
					"client has no italc-encoding" );
		deleteLater();
		return;
	}

	// for some reason we have to do this to make the following connection
	// working

	connect( m_conn, SIGNAL( cursorShapeChanged() ),
			this, SLOT( updateCursorShape() ),
							Qt::QueuedConnection );
	connect( m_conn, SIGNAL( regionUpdated( const QRegion & ) ),
			this, SLOT( updateRegion( const QRegion & ) ),
							Qt::QueuedConnection );

	ml.unlock();

	// first time send a key-frame
	updateRegion( m_conn->screen().rect() );

	//connect( m_sock, SIGNAL( readyRead() ), this, SLOT( processClient() ) );
	connect( m_sock, SIGNAL( disconnected() ),
						this, SLOT( deleteLater() ) );

	QTimer t;
	connect( &t, SIGNAL( timeout() ),
			this, SLOT( moveCursor() ), Qt::DirectConnection );
	t.start( CURSOR_UPDATE_TIME );
	QTimer t2;
	connect( &t2, SIGNAL( timeout() ),
			this, SLOT( processClient() ), Qt::DirectConnection );
	t2.start( 2*CURSOR_UPDATE_TIME );
	//moveCursor();
	//processClient();
	// now run our own event-loop for optimal scheduling
	exec();
}


#endif
