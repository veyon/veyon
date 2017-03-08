/*
 * DemoServer.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "AuthenticationCredentials.h"
#include "DemoServer.h"
#include "ItalcConfiguration.h"
#include "ItalcVncConnection.h"
#include "RfbLZORLE.h"
#include "RfbItalcCursor.h"
#include "SocketDevice.h"
#include "VariantStream.h"
#include "VariantArrayMessage.h"

#include <lzo/lzo1x.h>
#include "rfb/rfb.h"

const int CURSOR_UPDATE_TIME = 35;


#ifdef LIBVNCSERVER_WORDS_BIGENDIAN
char rfbEndianTest = (1==0);
#else
char rfbEndianTest = (1==1);
#endif


DemoServer::DemoServer( const QString& vncServerToken, const QString& demoAccessToken, QObject *parent ) :
	QTcpServer( parent ),
	m_demoAccessToken( demoAccessToken ),
	m_vncConn()
{
	if( listen( QHostAddress::Any, ItalcCore::config->demoServerPort() ) == false )
	{
		qCritical( "DemoServer::DemoServer(): could not start demo server!" );
		return;
	}

	ItalcCore::authenticationCredentials->setToken( vncServerToken );
	m_vncConn.setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
	m_vncConn.setPort( ItalcCore::config->computerControlServerPort() );
	m_vncConn.setItalcAuthType( RfbItalcAuth::Token );
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




void DemoServer::incomingConnection( qintptr sock )
{
	new DemoServerClient( m_demoAccessToken, sock, &m_vncConn, this );
}




#define RAW_MAX_PIXELS 1024

DemoServerClient::DemoServerClient( const QString& demoAccessToken,
									qintptr sock,
									const ItalcVncConnection *vncConn,
									DemoServer *parent ) :
	QThread( parent ),
	m_demoAccessToken( demoAccessToken ),
	m_demoServer( parent ),
	m_dataMutex( QMutex::Recursive ),
	m_updateRequested( false ),
	m_changedRects(),
	m_fullUpdatePending( false ),
	m_cursorHotX( 0 ),
	m_cursorHotY( 0 ),
	m_cursorShapeChanged( false ),
	m_socketDescriptor( sock ),
	m_socket( NULL ),
	m_vncConn( vncConn ),
	m_otherEndianess( false ),
	m_lzoWorkMem( new char[sizeof( lzo_align_t ) *
  ( ( ( LZO1X_1_MEM_COMPRESS ) +
  ( sizeof( lzo_align_t ) - 1 ) ) /
  sizeof( lzo_align_t ) ) ] ),
	m_rawBuf( new QRgb[RAW_MAX_PIXELS] ),
	m_rleBuf( NULL ),
	m_currentRleBufSize( 0 ),
	m_lzoOutBuf( NULL ),
	m_currentLzoOutBufSize( 0 )
{
	start();
}




DemoServerClient::~DemoServerClient()
{
	exit();
	wait();

	if( m_lzoOutBuf )
	{
		delete[] m_lzoOutBuf;
	}
	if( m_rleBuf )
	{
		delete[] m_rleBuf;
	}
	delete[] m_lzoWorkMem;
	delete[] m_rawBuf;
}




void DemoServerClient::updateRect( int x, int y, int w, int h )
{
	m_dataMutex.lock();
	if( m_fullUpdatePending == false )
	{
		if( m_changedRects.size() < MaxRects )
		{
			m_changedRects += QRect( x, y, w, h );
		}
		else
		{
			m_fullUpdatePending = true;
			m_changedRects.clear();
		}
	}
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
			(uint16_t) Swap16IfLE( 1 )
		} ;

		m_socket->write( (const char *) &m, sizeof( m ) );

		const rfbRectangle rr =
		{
			(uint16_t) Swap16IfLE( m_lastCursorPos.x() ),
			(uint16_t) Swap16IfLE( m_lastCursorPos.y() ),
			(uint16_t) Swap16IfLE( 0 ),
			(uint16_t) Swap16IfLE( 0 )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			Swap32IfLE( rfbEncodingPointerPos )
		} ;

		m_socket->write( (const char *) &rh, sizeof( rh ) );
		m_socket->flush();
		m_dataMutex.unlock();
	}
}




void DemoServerClient::sendUpdates()
{
	QMutexLocker ml( &m_dataMutex );

	if( m_fullUpdatePending == false && m_changedRects.isEmpty() )// && m_cursorShapeChanged == false )
	{
		if( m_updateRequested )
		{
			QTimer::singleShot( 50, this, SLOT( sendUpdates() ) );
		}
		return;
	}

	QVector<QRect> rects;

	if( m_fullUpdatePending )
	{
		rects += QRect( QPoint( 0, 0 ), m_vncConn->framebufferSize() );
	}
	else
	{
		// extract single (non-overlapping) rects out of changed region
		// this way we avoid lot of simliar/overlapping rectangles,
		// e.g. if we didn't get an update-request for a quite long time
		// and there were a lot of updates - at the end we don't send
		// more than the whole screen one time
		QRegion region;
		for( auto rect : m_changedRects )
		{
			region += rect;
		}
		rects = region.rects();
	}

	// no we gonna post all changed rects!
	const rfbFramebufferUpdateMsg m =
	{
		rfbFramebufferUpdate,
		0,
		(uint16_t) Swap16IfLE( rects.size() +
		( m_cursorShapeChanged ? 1 : 0 ) )
	} ;

	m_socket->write( (const char *) &m, sz_rfbFramebufferUpdateMsg );

	// process each rect
	for( auto rect : rects )
	{
		const int rx = rect.x();
		const int ry = rect.y();
		const int rw = rect.width();
		const int rh = rect.height();
		const rfbRectangle rr =
		{
			(uint16_t) Swap16IfLE( rx ),
			(uint16_t) Swap16IfLE( ry ),
			(uint16_t) Swap16IfLE( rw ),
			(uint16_t) Swap16IfLE( rh )
		} ;

		const rfbFramebufferUpdateRectHeader rhdr =
		{
			rr,
			(uint32_t) Swap32IfLE( rfbEncodingLZORLE )
		} ;

		m_socket->write( (const char *) &rhdr, sizeof( rhdr ) );

		const QImage & i = m_vncConn->image();
		RfbLZORLE::Header hdr = { 0, 0, 0 } ;

		// we only compress if it's enough data, otherwise
		// there's too much overhead
		if( rw * rh > RAW_MAX_PIXELS )
		{

			hdr.compressed = 1;
			QRgb last_pix = *( (QRgb *) i.constScanLine( ry ) + rx );

			// re-allocate RLE buffer if current one is too small
			const size_t rleBufSize = rw * rh * sizeof( QRgb )+16;
			if( rleBufSize > m_currentRleBufSize )
			{
				if( m_rleBuf )
				{
					delete[] m_rleBuf;
				}
				m_rleBuf = new uint8_t[rleBufSize];
				m_currentRleBufSize = rleBufSize;
			}

			uint8_t rle_cnt = 0;
			uint8_t rle_sub = 1;
			uint8_t *out = m_rleBuf;
			uint8_t *out_ptr = out;
			for( int y = ry; y < ry+rh; ++y )
			{
				const QRgb * data = ( (const QRgb *) i.constScanLine( y ) ) + rx;
				for( int x = 0; x < rw; ++x )
				{
					if( data[x] != last_pix || rle_cnt > 254 )
					{
						*( (QRgb *) out_ptr ) = Swap24IfLE( last_pix );
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
			*( (QRgb *) out_ptr ) = Swap24IfLE( last_pix );
			*( out_ptr + 3 ) = rle_cnt;
			out_ptr += 4;
			hdr.bytesRLE = out_ptr - out;

			lzo_uint bytes_lzo = hdr.bytesRLE + hdr.bytesRLE / 16 + 67;

			// re-allocate LZO output buffer if current one is too small
			if( bytes_lzo > m_currentLzoOutBufSize )
			{
				if( m_lzoOutBuf )
				{
					delete[] m_lzoOutBuf;
				}
				m_lzoOutBuf = new uint8_t[bytes_lzo];
				m_currentLzoOutBufSize = bytes_lzo;
			}

			uint8_t *comp = m_lzoOutBuf;
			lzo1x_1_compress( (const unsigned char *) out, (lzo_uint) hdr.bytesRLE,
							  (unsigned char *) comp,
							  &bytes_lzo, m_lzoWorkMem );
			hdr.bytesRLE = Swap32IfLE( hdr.bytesRLE );
			hdr.bytesLZO = Swap32IfLE( bytes_lzo );

			m_socket->write( (const char *) &hdr, sizeof( hdr ) );
			m_socket->write( (const char *) comp, bytes_lzo );

		}
		else
		{
			m_socket->write( (const char *) &hdr, sizeof( hdr ) );
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
			m_socket->write( (const char *) m_rawBuf, rh * rw * sizeof( QRgb ) );
		}
	}

	if( m_cursorShapeChanged )
	{
		const QImage cur = m_cursorShape;
		const rfbRectangle rr =
		{
			(uint16_t) Swap16IfLE( m_cursorHotX ),
			(uint16_t) Swap16IfLE( m_cursorHotY ),
			(uint16_t) Swap16IfLE( cur.width() ),
			(uint16_t) Swap16IfLE( cur.height() )
		} ;

		const rfbFramebufferUpdateRectHeader rh =
		{
			rr,
			(uint32_t) Swap32IfLE( rfbEncodingItalcCursor )
		} ;

		m_socket->write( (const char *) &rh, sizeof( rh ) );

		VariantStream( m_socket ).write( QVariant::fromValue( cur ) );
	}

	m_socket->flush();

	// reset vars
	m_changedRects.clear();
	m_cursorShapeChanged = false;
	m_fullUpdatePending = false;

	if( m_updateRequested )
	{
		// make sure to send an update once more even if there has
		// been no update request
		QTimer::singleShot( 1000, this, SLOT( sendUpdates() ) );
	}

	m_updateRequested = false;
}



void DemoServerClient::processClient()
{
	m_dataMutex.lock();
	while( m_socket->bytesAvailable() > 0 )
	{
		rfbClientToServerMsg msg;
		if( readExact( (char *) &msg, 1 ) <= 0 )
		{
			qWarning( "DemoServerClient::processClient(): "
					  "could not read cmd" );
			continue;
		}

		switch( msg.type )
		{
		case rfbSetEncodings:
			readExact( ((char *)&msg)+1, sz_rfbSetEncodingsMsg-1 );
			msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);
			for( int i = 0; i < msg.se.nEncodings; ++i )
			{
				uint32_t enc;
				readExact( (char *) &enc, 4 );
			}
			continue;

		case rfbSetPixelFormat:
			readExact( ((char *) &msg)+1, sz_rfbSetPixelFormatMsg-1 );
			continue;
		case rfbSetServerInput:
			readExact( ((char *) &msg)+1, sz_rfbSetServerInputMsg-1 );
			continue;
		case rfbClientCutText:
			readExact( ((char *) &msg)+1, sz_rfbClientCutTextMsg-1 );
			msg.cct.length = Swap32IfLE( msg.cct.length );
			if( msg.cct.length )
			{
				char *t = new char[msg.cct.length];
				readExact( t, msg.cct.length );
				delete[] t;
			}
			continue;
		case rfbFramebufferUpdateRequest:
			readExact( ((char *) &msg)+1, sz_rfbFramebufferUpdateRequestMsg-1 );
			m_updateRequested = true;
			break;
		default:
			qWarning( "DemoServerClient::processClient(): "
					  "ignoring msg type %d", msg.type );
			continue;
		}

	}

	if( m_updateRequested )
	{
		sendUpdates();
	}

	m_dataMutex.unlock();
}




void DemoServerClient::run()
{
	QMutexLocker ml( &m_dataMutex );

	QTcpSocket _socket;
	m_socket = &_socket;
	if( !m_socket->setSocketDescriptor( m_socketDescriptor ) )
	{
		qCritical( "DemoServerClient::run(): could not set socket descriptor - aborting" );
		deleteLater();
		return;
	}

	rfbProtocolVersionMsg pv;
	sprintf( pv, rfbProtocolVersionFormat, rfbProtocolMajorVersion,
			 rfbProtocolMinorVersion );

	writeExact( pv, sz_rfbProtocolVersionMsg );
	readExact( pv, sz_rfbProtocolVersionMsg );

	const uint8_t secTypeList[2] = { 1, rfbSecTypeItalc } ;
	writeExact( (const char *) secTypeList, sizeof( secTypeList ) );

	uint8_t chosen = 0;
	readExact( (char *) &chosen, sizeof( chosen ) );

	if( chosen != rfbSecTypeItalc )
	{
		qCritical( "DemoServerClient:::run(): protocol initialization failed" );
		deleteLater();
		return;
	}

	// send list of supported authentication types
	VariantArrayMessage supportedAuthTypesMessage( m_socket );
	supportedAuthTypesMessage.write( 1 );
	supportedAuthTypesMessage.write( RfbItalcAuth::Token );
	supportedAuthTypesMessage.send();

	VariantArrayMessage authTypeMessageResponse( m_socket );
	if( ( authTypeMessageResponse.isReadyForReceive() == false &&
			m_socket->waitForReadyRead() == false ) ||
			authTypeMessageResponse.receive() == false )
	{
		qWarning("DemoServerClient::run(): demo client authentication failed\n");
		deleteLater();
		return;
	}

	auto chosenItalcAuthType = authTypeMessageResponse.read().value<RfbItalcAuth::Type>();

	if( chosenItalcAuthType != RfbItalcAuth::Token )
	{
		qWarning("DemoServerClient::run(): demo client authentication failed\n");
		deleteLater();
		return;
	}

	const QString username = authTypeMessageResponse.read().toString();
	qDebug() << "DemoServerClient::run(): authenticate for user" << username;

	VariantArrayMessage tokenMessage( m_socket );
	if( ( tokenMessage.isReadyForReceive() == false &&
		  m_socket->waitForReadyRead() == false ) ||
			tokenMessage.receive() == false )
	{
		qDebug() << "DemoServerClient::run(): could not receive token";
	}

	const QString token = tokenMessage.read().toString();
	if( token.isEmpty() == false && token != m_demoAccessToken )
	{
		qWarning("DemoServerClient::run(): demo client authentication failed\n");
		deleteLater();
		return;
	}

	uint32_t authResult = Swap32IfLE(rfbVncAuthOK);

	writeExact( (char *) &authResult, sizeof( authResult ) );

	rfbClientInitMsg ci;

	if( readExact( (char *) &ci, sz_rfbClientInitMsg ) != sz_rfbClientInitMsg )
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
	if( writeExact( ( const char *) &si, sz_rfbServerInitMsg ) != sz_rfbServerInitMsg )
	{
		qWarning( "failed writing rfbServerInitMsg" );
		deleteLater();
		return;
	}


	connect( m_vncConn, &ItalcVncConnection::cursorShapeUpdated,
			 this, &DemoServerClient::updateCursorShape, Qt::QueuedConnection );

	connect( m_vncConn, &ItalcVncConnection::imageUpdated,
			 this, &DemoServerClient::updateRect, Qt::QueuedConnection );

	ml.unlock();

	// TODO
	//updateCursorShape( m_demoServer->initialCursorShape(), 0, 0 );

	// first time send a key-frame
	QSize s = m_vncConn->framebufferSize();
	updateRect( 0, 0, s.width(), s.height() );

	connect( m_socket, &QTcpSocket::readyRead, this, &DemoServerClient::processClient, Qt::DirectConnection );
	connect( m_socket, &QTcpSocket::disconnected, this, &DemoServerClient::quit );

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

	deleteLater();
}



qint64 DemoServerClient::writeExact( const char* buffer, qint64 size )
{
	qint64 result = m_socket->write( buffer, size );
	m_socket->flush();

	return result;
}



qint64 DemoServerClient::readExact( char* buffer, qint64 size )
{
	// implement blocking read

	qint64 bytesRead = 0;
	while( bytesRead < size )
	{
		qint64 n = m_socket->read( buffer + bytesRead, size - bytesRead );
		if( n < 0 )
		{
			qDebug( "DemoServerClient::readExact(): read() returned %d while reading %d of %d bytes", (int) n, (int) bytesRead, (int) size );
			return -1;
		}
		bytesRead += n;
		if( bytesRead < size && m_socket->waitForReadyRead( 5000 ) == false )
		{
			qDebug( "DemoServerClient::readExact(): timeout after reading %d of %d bytes", (int) bytesRead, (int) size );
			return bytesRead;
		}
	}

	return bytesRead;
}
