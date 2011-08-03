/*
 * DemoServer.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtNetwork/QTcpSocket>
#include <QtCore/QTimer>
#include <QtGui/QCursor>

#include "DemoServer.h"
#include "ItalcCoreServer.h"
#include "ItalcVncConnection.h"
#include "QuadTree.h"
#include "RfbLZORLE.h"
#include "RfbItalcCursor.h"
#include "SocketDevice.h"

#include "minilzo.h"
#include "rfb/rfb.h"

const int CURSOR_UPDATE_TIME = 35;


static qint64 qtcpsocketDispatcher( char * buffer, const qint64 bytes,
									const SocketOpCodes opCode, void * user )
{
	QTcpSocket * sock = static_cast<QTcpSocket *>( user );
	qint64 ret = 0;

	QTime opStartTime;
	opStartTime.start();

	switch( opCode )
	{
		case SocketRead:
			while( ret < bytes )
			{
				qint64 bytesRead = sock->read( buffer+ret, bytes-ret );
				if( bytesRead < 0 || opStartTime.elapsed() > 5000 )
				{
					qWarning( "qtcpsocketDispatcher(...): connection closed while reading" );
					return 0;
				}
				else if( bytesRead == 0 )
				{
					if( sock->state() != QTcpSocket::ConnectedState )
					{
						qWarning( "qtcpsocketDispatcher(...): connection failed while reading "
									"state:%d  error:%d", sock->state(), sock->error() );
						return 0;
					}
					sock->waitForReadyRead( 10 );
				}
				else
				{
					ret += bytesRead;
					opStartTime.restart();
				}
			}
			break;

		case SocketWrite:
			while( ret < bytes )
			{
				qint64 written = sock->write( buffer+ret, bytes-ret );
				if( written < 0 || opStartTime.elapsed() > 5000 )
				{
					qWarning( "qtcpsocketDispatcher(...): connection closed while writing" );
					return 0;
				}
				else if( written == 0 )
				{
					if( sock->state() != QTcpSocket::ConnectedState )
					{
						qWarning( "qtcpsocketDispatcher(...): connection failed while writing  "
									"state:%d error:%d", sock->state(), sock->error() );
						return 0;
					}
				}
				else
				{
					ret += written;
					opStartTime.restart();
				}
			}
			//sock->flush();
			break;

		case SocketGetPeerAddress:
			strncpy( buffer,
		sock->peerAddress().toString().toUtf8().constData(), bytes );
			break;
	}

	return ret;
}




DemoServer::DemoServer( int srcPort, int dstPort, QObject *parent ) :
	QTcpServer( parent ),
	m_vncConn()
{
	if( listen( QHostAddress::Any, dstPort ) == false )
	{
		qCritical( "DemoServer::DemoServer(): "
					"could not start demo-server!" );
		return;
	}

	m_vncConn.setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
	m_vncConn.setPort( srcPort );
	m_vncConn.setItalcAuthType( ItalcAuthCommonSecret );
	m_vncConn.setQuality( ItalcVncConnection::DemoServerQuality );
	m_vncConn.start();

	connect( &m_vncConn, SIGNAL( cursorShapeUpdated( const QImage &, int, int ) ),
				this, SLOT( updateInitialCursorShape( const QImage &, int, int ) ) );
	checkForCursorMovement();
}




DemoServer::~DemoServer()
{
	QList<DemoServerClient *> l;
	while( !( l = findChildren<DemoServerClient *>() ).isEmpty() )
	{
		delete l.front();
	}
}




void DemoServer::checkForCursorMovement()
{
return;	// TODO
	m_cursorLock.lockForWrite();
	if( m_cursorPos != QCursor::pos() )
	{
		m_cursorPos = QCursor::pos();
	}
	m_cursorLock.unlock();
	QTimer::singleShot( CURSOR_UPDATE_TIME, this,
					SLOT( checkForCursorMovement() ) );
}




void DemoServer::updateInitialCursorShape( const QImage &img, int x, int y )
{
return;	// TODO
	m_cursorLock.lockForWrite();
	m_initialCursorShape = img;
	m_cursorLock.unlock();
}




void DemoServer::incomingConnection( int sock )
{
	new DemoServerClient( sock, &m_vncConn, this );
}




#define RAW_MAX_PIXELS 1024

DemoServerClient::DemoServerClient( int sock, const ItalcVncConnection *vncConn,
										DemoServer *parent ) :
	QThread( parent ),
	m_demoServer( parent ),
	m_dataMutex( QMutex::Recursive ),
	m_updatesPending( false ),
	m_changedRects(),
	m_cursorHotX( 0 ),
	m_cursorHotY( 0 ),
	m_cursorShapeChanged( false ),
	m_socketDescriptor( sock ),
	m_sock( NULL ),
	m_vncConn( vncConn ),
	m_otherEndianess( false ),
	m_lzoWorkMem( new char[sizeof( lzo_align_t ) *
			( ( ( LZO1X_1_MEM_COMPRESS ) +
					( sizeof( lzo_align_t ) - 1 ) ) /
						sizeof( lzo_align_t ) ) ] ),
	m_rawBuf( new QRgb[RAW_MAX_PIXELS] )
{
	start();
}




DemoServerClient::~DemoServerClient()
{
	exit();
	wait();
	delete[] m_lzoWorkMem;
	delete[] m_rawBuf;
}




void DemoServerClient::updateRect( int x, int y, int w, int h )
{
	m_dataMutex.lock();
	m_changedRects += QRect( x, y, w, h );
	m_dataMutex.unlock();
}




void DemoServerClient::updateCursorShape( const QImage &img, int x, int y )
{
return;		// TODO
	m_dataMutex.lock();
	m_cursorShape = img;
	m_cursorHotX = x;
	m_cursorHotY = y;
	m_cursorShapeChanged = true;
	m_dataMutex.unlock();
}




void DemoServerClient::moveCursor()
{
return;		// TODO
	QPoint p = m_demoServer->cursorPos();
	if( p != m_lastCursorPos )
	{
		m_dataMutex.lock();
		m_lastCursorPos = p;
		const rfbFramebufferUpdateMsg m =
		{
			rfbFramebufferUpdate,
			0,
			Swap16IfLE( 1 )
		} ;

		m_sock->write( (const char *) &m, sizeof( m ) );

		const rfbRectangle rr =
		{
			Swap16IfLE( m_lastCursorPos.x() ),
			Swap16IfLE( m_lastCursorPos.y() ),
			Swap16IfLE( 0 ),
			Swap16IfLE( 0 )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			Swap32IfLE( rfbEncodingPointerPos )
		} ;

		m_sock->write( (const char *) &rh, sizeof( rh ) );
		m_sock->flush();
		m_dataMutex.unlock();
	}
}




void DemoServerClient::sendUpdates()
{
	QMutexLocker ml( &m_dataMutex );

	if( m_changedRects.isEmpty() && m_cursorShapeChanged == false )
	{
		if( m_updatesPending )
		{
			QTimer::singleShot( 50, this, SLOT( sendUpdates() ) );
			return;
		}
	}

	// extract single (non-overlapping) rects out of changed region
	// this way we avoid lot of simliar/overlapping rectangles,
	// e.g. if we didn't get an update-request for a quite long time
	// and there were a lot of updates - at the end we don't send
	// more than the whole screen one time
	QuadTree q( 0, 0, m_vncConn->image().width()-1, m_vncConn->image().height()-1, 4 );
	QVector<QuadTreeRect> r;
	if( !m_changedRects.isEmpty() )
	{
		q.addRects( m_changedRects );
		r = q.rects();
	}

	// no we gonna post all changed rects!
	const rfbFramebufferUpdateMsg m =
	{
		rfbFramebufferUpdate,
		0,
		Swap16IfLE( r.size() +
				( m_cursorShapeChanged ? 1 : 0 ) )
	} ;

	SocketDevice sd( qtcpsocketDispatcher, m_sock );
	sd.write( (const char *) &m, sz_rfbFramebufferUpdateMsg );

	// process each rect
	for( QVector<QuadTreeRect>::const_iterator it = r.begin(); it != r.end(); ++it )
	{
		const int rx = it->x1();
		const int ry = it->y1();
		const int rw = it->x2()-it->x1()+1;
		const int rh = it->y2()-it->y1()+1;
		const rfbRectangle rr =
		{
			Swap16IfLE( rx ),
			Swap16IfLE( ry ),
			Swap16IfLE( rw ),
			Swap16IfLE( rh )
		} ;

		const rfbFramebufferUpdateRectHeader rhdr =
		{
			rr,
			Swap32IfLE( rfbEncodingLZORLE )
		} ;

		sd.write( (const char *) &rhdr, sizeof( rhdr ) );

		const QImage & i = m_vncConn->image();
		RfbLZORLE::Header hdr = { 0, 0, 0 } ;

		// we only compress if it's enough data, otherwise
		// there's too much overhead
		if( rw * rh > RAW_MAX_PIXELS )
		{

	hdr.compressed = 1;
	QRgb last_pix = *( (QRgb *) i.scanLine( ry ) + rx );
	uint8_t rle_cnt = 0;
	uint8_t rle_sub = 1;
	uint8_t *out = new uint8_t[rw * rh * sizeof( QRgb )+16];
	uint8_t *out_ptr = out;
	for( int y = ry; y < ry+rh; ++y )
	{
		const QRgb * data = ( (const QRgb *) i.scanLine( y ) ) + rx;
		for( int x = 0; x < rw; ++x )
		{
			if( data[x] != last_pix || rle_cnt > 254 )
			{
				*( (QRgb *) out_ptr ) = Swap32IfBE( last_pix );
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

	lzo_uint bytes_lzo = hdr.bytesRLE + hdr.bytesRLE / 16 + 67;
	uint8_t *comp = new uint8_t[bytes_lzo];
	lzo1x_1_compress( (const unsigned char *) out, (lzo_uint) hdr.bytesRLE,
				(unsigned char *) comp,
				&bytes_lzo, m_lzoWorkMem );
	hdr.bytesRLE = Swap32IfLE( hdr.bytesRLE );
	hdr.bytesLZO = Swap32IfLE( bytes_lzo );

	sd.write( (const char *) &hdr, sizeof( hdr ) );
	sd.write( (const char *) comp, Swap32IfLE( hdr.bytesLZO ) );
	delete[] out;
	delete[] comp;

		}
		else
		{
			sd.write( (const char *) &hdr, sizeof( hdr ) );
			QRgb *dst = m_rawBuf;
			if( m_otherEndianess )
			{
				for( int y = 0; y < rh; ++y )
				{
					const QRgb *src = (const QRgb *) i.scanLine( ry + y ) + rx;
					for( int x = 0; x < rw; ++x, ++src, ++dst )
					{
						*dst = Swap32( *src );
					}
				}
			}
			else
			{
				for( int y = 0; y < rh; ++y )
				{
					const QRgb *src = (const QRgb *) i.scanLine( ry + y ) + rx;
					for( int x = 0; x < rw; ++x, ++src, ++dst )
					{
						*dst = *src;
					}
				}
			}
			sd.write( (const char *) m_rawBuf, rh * rw * sizeof( QRgb ) );
		}
	}

	if( m_cursorShapeChanged )
	{
		const QImage cur = m_cursorShape;
		const rfbRectangle rr =
		{
			Swap16IfLE( m_cursorHotX ),
			Swap16IfLE( m_cursorHotY ),
			Swap16IfLE( cur.width() ),
			Swap16IfLE( cur.height() )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			Swap32IfLE( rfbEncodingItalcCursor )
		} ;

		sd.write( (const char *) &rh, sizeof( rh ) );

		sd.write( QVariant::fromValue( cur ) );
	}

	// reset vars
	m_changedRects.clear();
	m_cursorShapeChanged = false;

	if( m_updatesPending )
	{
		// make sure to send an update once more even if there has
		// been no update request
		QTimer::singleShot( 1000, this, SLOT( sendUpdates() ) );
	}

	m_updatesPending = false;
}



void DemoServerClient::processClient()
{
	SocketDevice sd( qtcpsocketDispatcher, m_sock );

	m_dataMutex.lock();
	while( m_sock->bytesAvailable() > 0 )
	{
		rfbClientToServerMsg msg;
		if( sd.read( (char *) &msg, 1 ) <= 0 )
		{
			qWarning( "DemoServerClient::processClient(): "
							"could not read cmd" );
			continue;
		}

		switch( msg.type )
		{
			case rfbSetEncodings:
				sd.read( ((char *)&msg)+1, sz_rfbSetEncodingsMsg-1 );
				msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);
				for( int i = 0; i < msg.se.nEncodings; ++i )
				{
					uint32_t enc;
					sd.read( (char *) &enc, 4 );
				}
				continue;

			case rfbSetPixelFormat:
				sd.read( ((char *) &msg)+1, sz_rfbSetPixelFormatMsg-1 );
				continue;
			case rfbSetServerInput:
				sd.read( ((char *) &msg)+1, sz_rfbSetServerInputMsg-1 );
				continue;
			case rfbClientCutText:
				sd.read( ((char *) &msg)+1, sz_rfbClientCutTextMsg-1 );
				msg.cct.length = Swap32IfLE( msg.cct.length );
				if( msg.cct.length )
				{
					char *t = new char[msg.cct.length];
					sd.read( t, msg.cct.length );
					delete[] t;
				}
				continue;
			case rfbFramebufferUpdateRequest:
				sd.read( ((char *) &msg)+1, sz_rfbFramebufferUpdateRequestMsg-1 );
				m_updatesPending = true;
				break;
			default:
				qWarning( "DemoServerClient::processClient(): "
							"ignoring msg type %d", msg.type );
				continue;
		}

	}

	if( m_updatesPending )
	{
		sendUpdates();
	}

	m_dataMutex.unlock();
}




void DemoServerClient::run()
{
	QMutexLocker ml( &m_dataMutex );

	QTcpSocket sock;
	m_sock = &sock;
	if( !m_sock->setSocketDescriptor( m_socketDescriptor ) )
	{
		qCritical( "DemoServerClient::run(): "
				"could not set socket-descriptor - aborting" );
		deleteLater();
		return;
	}

	SocketDevice sd( qtcpsocketDispatcher, m_sock );

	rfbProtocolVersionMsg pv;
	sprintf( pv, rfbProtocolVersionFormat, rfbProtocolMajorVersion,
											rfbProtocolMinorVersion );

	sd.write( pv, sz_rfbProtocolVersionMsg );
	sd.read( pv, sz_rfbProtocolVersionMsg );

	const uint8_t secTypeList[2] = { 1, rfbSecTypeItalc } ;
	sd.write( (const char *) secTypeList, sizeof( secTypeList ) );

	uint8_t chosen = 0;
	sd.read( (char *) &chosen, sizeof( chosen ) );

	if( chosen != rfbSecTypeItalc )
	{
		qCritical( "DemoServerClient:::run(): "
					"protocol initialization failed" );
		deleteLater();
		return;
	}

	uint32_t authResult = Swap32IfLE(rfbVncAuthFailed);

    if( ItalcCoreServer::instance()->
			authSecTypeItalc( qtcpsocketDispatcher, m_sock ) != true )
	{
		qWarning("auth of demo client failed\n");
		deleteLater();
		return;
	}

	authResult = Swap32IfLE(rfbVncAuthOK);

	sd.write( (char *) &authResult, sizeof( authResult ) );

	rfbClientInitMsg ci;

	if( !sd.read( (char *) &ci, sz_rfbClientInitMsg ) )
	{
		qWarning( "failed reading rfbClientInitMsg" );
		deleteLater();
		return;
	}

	rfbServerInitMsg si = m_vncConn->getRfbClient()->si;
	si.framebufferWidth = Swap16IfLE( si.framebufferWidth );
	si.framebufferHeight = Swap16IfLE( si.framebufferHeight );
	si.format.redMax = Swap16IfLE( si.format.redMax );
	si.format.greenMax = Swap16IfLE( si.format.greenMax );
	si.format.blueMax = Swap16IfLE( si.format.blueMax );
	si.format.bigEndian = ( QSysInfo::ByteOrder == QSysInfo::BigEndian )
									? 1 : 0;
	si.nameLength = 0;
	if( !sd.write( ( const char *) &si, sz_rfbServerInitMsg ) )
	{
		qWarning( "failed writing rfbServerInitMsg" );
		deleteLater();
		return;
	}


	connect( m_vncConn, SIGNAL( cursorShapeUpdated( const QImage &, int, int ) ),
			this, SLOT( updateCursorShape( const QImage &, int, int ) ),
							Qt::QueuedConnection );
	connect( m_vncConn, SIGNAL( imageUpdated( int, int, int, int ) ),
			this, SLOT( updateRect( int, int, int, int ) ),
							Qt::QueuedConnection );

	ml.unlock();

	// TODO
	//updateCursorShape( m_demoServer->initialCursorShape(), 0, 0 );

	// first time send a key-frame
	QSize s = m_vncConn->framebufferSize();
	updateRect( 0, 0, s.width(), s.height() );

	connect( m_sock, SIGNAL( readyRead() ),
				this, SLOT( processClient() ), Qt::DirectConnection );
	connect( m_sock, SIGNAL( disconnected() ),
						this, SLOT( deleteLater() ) );

	// TODO
/*	QTimer t;
	connect( &t, SIGNAL( timeout() ),
			this, SLOT( moveCursor() ), Qt::DirectConnection );
	t.start( CURSOR_UPDATE_TIME );*/

/*	QTimer t2;
	connect( &t2, SIGNAL( timeout() ),
			this, SLOT( processClient() ), Qt::DirectConnection );
	t2.start( 2*CURSOR_UPDATE_TIME );*/

	// now run our own event-loop for optimal scheduling
	exec();
}


