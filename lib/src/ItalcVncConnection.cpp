/*
 * ItalcVncConnection.cpp - implementation of ItalcVncConnection class
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * code partly taken from KRDC / vncclientthread.cpp:
 * Copyright (C) 2007-2008 Urs Wolfer <uwolfer @ kde.org>
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

#include "ItalcVncConnection.h"
#include "Logger.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMutexLocker>
#include <QtCore/QTime>


static QString outputErrorMessageString;



class KeyClientEvent : public ClientEvent
{
public:
	KeyClientEvent( int key, int pressed ) :
		m_key( key ),
		m_pressed( pressed )
	{
	}

	virtual void fire( rfbClient *cl )
	{
		SendKeyEvent( cl, m_key, m_pressed );
	}

private:
	int m_key;
	int m_pressed;
} ;



class PointerClientEvent : public ClientEvent
{
public:
	PointerClientEvent( int x, int y, int buttonMask ) :
		m_x( x ),
		m_y( y ),
		m_buttonMask( buttonMask )
	{
	}

	virtual void fire( rfbClient *cl )
	{
		SendPointerEvent( cl, m_x, m_y, m_buttonMask );
	}

private:
	int m_x;
	int m_y;
	int m_buttonMask;
} ;



class ClientCutEvent : public ClientEvent
{
public:
	ClientCutEvent( char *text ) :
	    m_text( text )
	{
	}

	virtual void fire( rfbClient *cl )
	{
		SendClientCutText( cl, m_text, qstrlen( m_text ) );
	}

private:
	char * m_text;
} ;





rfbBool ItalcVncConnection::hookNewClient( rfbClient *cl )
{
	ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( cl, 0) ;

	const int size = (int) cl->width * cl->height *
					( cl->format.bitsPerPixel / 8 );
	if( t->m_frameBuffer )
	{
		// do not leak if we get a new framebuffer size
		delete [] t->m_frameBuffer;
	}
	t->m_frameBuffer = new uint8_t[size];
	t->m_framebufferInitialized = false;
	cl->frameBuffer = t->m_frameBuffer;
	memset( cl->frameBuffer, '\0', size );
	cl->format.bitsPerPixel = 32;
	cl->format.redShift = 16;
	cl->format.greenShift = 8;
	cl->format.blueShift = 0;
	cl->format.redMax = 0xff;
	cl->format.greenMax = 0xff;
	cl->format.blueMax = 0xff;

	// only use remote cursor for remote control
	cl->appData.useRemoteCursor = false;
	cl->appData.compressLevel = 0;
	cl->appData.useBGR233 = 0;
	cl->appData.qualityLevel = 9;
	cl->appData.enableJPEG = false;

	switch( t->quality() )
	{
		case SnapshotQuality:
			cl->appData.encodingsString = "raw";
			break;
		case RemoteControlQuality:
			cl->appData.encodingsString = "copyrect hextile raw";
			cl->appData.useRemoteCursor = true;
			break;
		case ThumbnailQuality:
			cl->appData.useBGR233 = 1;
			cl->appData.encodingsString = "tight zrle ultra "
							"copyrect hextile zlib "
							"corre rre raw";
			cl->appData.compressLevel = 9;
			cl->appData.qualityLevel = 5;
			cl->appData.enableJPEG = true;
			break;
		default:
		case DemoQuality:
			cl->appData.encodingsString = "zrle ultra copyrect "
							"hextile zlib corre rre raw";
			break;
	}

	SetFormatAndEncodings( cl );

	return true;
}




void ItalcVncConnection::hookUpdateFB( rfbClient *cl, int x, int y, int w, int h )
{
	QImage img( cl->frameBuffer, cl->width, cl->height, QImage::Format_RGB32 );

	if( img.isNull() )
	{
		qWarning( "image not loaded" );
		return;
	}

	ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( cl, 0 );
	t->setImage( img );
	t->m_scaledScreenNeedsUpdate = true;
	t->m_framebufferInitialized = true;
	t->emitUpdated( x, y, w, h );
}



void ItalcVncConnection::hookCursorShape( rfbClient *cl, int xh, int yh,
											int w, int h, int bpp )
{
	for( int i = 0; i < w*h;++i )
	{
		if( cl->rcMask[i] )
		{
			cl->rcMask[i] = 255;
		}
	}
	QImage alpha( cl->rcMask, w, h, QImage::Format_Indexed8 );

	QImage cursorShape = QImage( cl->rcSource, w, h, QImage::Format_RGB32 );
	cursorShape.convertToFormat( QImage::Format_ARGB32 );
	cursorShape.setAlphaChannel( alpha );

	ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( cl, 0 );
	t->emitCursorShapeUpdated( cursorShape, xh, yh );
}



void ItalcVncConnection::hookCutText( rfbClient *cl, const char *text,
								int textlen )
{
	QString cutText = QString::fromUtf8( text, textlen );
	if( !cutText.isEmpty() )
	{
	        ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( cl, 0);
		t->emitGotCut( cutText );
	}
}




void ItalcVncConnection::hookOutputHandler( const char *format, ... )
{
	va_list args;
	va_start( args, format );

	QString message;
	message.vsprintf( format, args );

	va_end(args);

	message = message.trimmed();
	ilog( Warning, "ItalcVncConnection: " + message );

	if( ( message.contains( "Couldn't convert " ) ) ||
		( message.contains( "Unable to connect to VNC server" ) ) )
	{
		outputErrorMessageString = tr( "Server not found." );
	}

	if( ( message.contains( "VNC connection failed: Authentication failed, "
							"too many tries")) ||
		( message.contains( "VNC connection failed: Too many "
						"authentication failures" ) ) )
	{
		outputErrorMessageString = tr( "VNC authentication failed "
				"because of too many authentication tries." );
	}

	if (message.contains("VNC connection failed: Authentication failed"))
		outputErrorMessageString = tr("VNC authentication failed.");

	if (message.contains("VNC server closed connection"))
		outputErrorMessageString = tr("VNC server closed connection.");

	// internal messages, not displayed to user
	if (message.contains("VNC server supports protocol version 3.889")) // see http://bugs.kde.org/162640
		outputErrorMessageString = "INTERNAL:APPLE_VNC_COMPATIBILTY";
}




ItalcVncConnection::ItalcVncConnection( QObject *parent ) :
	QThread( parent ),
	m_frameBuffer( NULL ),
	m_framebufferInitialized( false ),
	m_cl( NULL ),
	m_italcAuthType( ItalcAuthDSA ),
	m_quality( DemoQuality ),
	m_port( PortOffsetIVS ),
	m_framebufferUpdateInterval( 0 ),
	m_image(),
	m_scaledScreenNeedsUpdate( false ),
	m_scaledScreen(),
	m_scaledSize(),
	m_state( Disconnected ),
	m_stopped( false )
{
}



ItalcVncConnection::~ItalcVncConnection()
{
	stop();

	delete [] m_frameBuffer;
}




void ItalcVncConnection::checkOutputErrorMessage()
{
	if( !outputErrorMessageString.isEmpty() )
	{
//		QString errorMessage = outputErrorMessageString;
		outputErrorMessageString.clear();
	}
}




void ItalcVncConnection::stop()
{
	if( isRunning() )
	{
		m_stopped = true;
		m_updateIntervalSleeper.wakeAll();

		if( !wait( 1000 ) )
		{
			qWarning( "ItalcVncConnection::stop(): terminating thread" );
			terminate();
		}
	}
}




void ItalcVncConnection::reset( const QString &host )
{
	if( m_state != Connected && isRunning() )
	{
		setHost( host );
	}
	else
	{
		stop();
		setHost( host );
		start();
	}
}




void ItalcVncConnection::setHost( const QString &host )
{
	QMutexLocker locker( &m_mutex );
	m_host = host;
	if( m_host.contains( ':' ) )
	{
		m_port = m_host.section( ':', 1, 1 ).toInt();
		m_host = m_host.section( ':', 0, 0 );
	}
}




void ItalcVncConnection::setPort( int port )
{
	QMutexLocker locker( &m_mutex );
	m_port = port;
}




bool ItalcVncConnection::waitForConnected( int timeout ) const
{
	QTime startTime;
	startTime.start();

	while( !isConnected() && startTime.elapsed() < timeout )
	{
		if( QCoreApplication::instance() )
		{
			QCoreApplication::processEvents();
		}
	}

	return isConnected();
}




void ItalcVncConnection::setImage( const QImage & img )
{
	m_imgLock.lockForWrite();
	const QSize oldSize = m_image.size();
	m_image = img;
	m_imgLock.unlock();

	if( img.size() != oldSize )
	{
		emit framebufferSizeChanged( img.width(), img.height() );
	}
}




const QImage ItalcVncConnection::image( int x, int y, int w, int h )
{
	QReadLocker locker( &m_imgLock );

	if( w == 0 || h == 0 ) // full image requested
	{
		return m_image;
	}
	return m_image.copy( x, y, w, h );
}




void ItalcVncConnection::setFramebufferUpdateInterval( int interval )
{
	m_framebufferUpdateInterval = interval;
}




void ItalcVncConnection::rescaleScreen()
{
	if( m_scaledScreenNeedsUpdate )
	{
		if( m_scaledScreen.size() != m_scaledSize )
		{
			m_scaledScreen = QImage( m_scaledSize,
							QImage::Format_RGB32 );
		}
		QReadLocker locker( &m_imgLock );
		if( m_image.size().isValid() )
		{
			m_image.scaleTo( m_scaledScreen );
		}
		else
		{
			m_scaledScreen.fill( Qt::black );
		}
		m_scaledScreenNeedsUpdate = false;
	}
}




void ItalcVncConnection::emitUpdated( int x, int y, int w, int h )
{
	emit imageUpdated( x, y, w, h );
}




void ItalcVncConnection::emitCursorShapeUpdated( const QImage &cursorShape,
														int xh, int yh )
{
	emit cursorShapeUpdated( cursorShape, xh, yh );
}




void ItalcVncConnection::emitGotCut( const QString &text )
{
	emit gotCut( text );
}




void ItalcVncConnection::run()
{
	m_state = Disconnected;
	m_stopped = false;

	rfbClientLog = hookOutputHandler;
	rfbClientErr = hookOutputHandler;

	while( m_stopped == false )
	{
		doConnection();
	}
}



void ItalcVncConnection::doConnection()
{
	QMutex sleeperMutex;

	while( !m_stopped && m_state != Connected ) // try to connect as long as the server allows
	{
		m_cl = rfbGetClient( 8, 3, 4 );
		m_cl->MallocFrameBuffer = hookNewClient;
		m_cl->canHandleNewFBSize = true;
		m_cl->GotFrameBufferUpdate = hookUpdateFB;
		m_cl->GotCursorShape = hookCursorShape;
		m_cl->GotXCutText = hookCutText;
		rfbClientSetClientData( m_cl, 0, this );

		m_mutex.lock();

		if( m_port < 0 ) // port is invalid or empty...
		{
			m_port = PortOffsetIVS;
		}

		if( m_port >= 0 && m_port < 100 )
		{
			 // the user most likely used the short form (e.g. :1)
			m_port += PortOffsetIVS;
		}

		free( m_cl->serverHost );
		m_cl->serverHost = strdup( m_host.toUtf8().constData() );
		m_cl->serverPort = m_port;

		m_mutex.unlock();

		emit newClient( m_cl );

		int argc;
		if( rfbInitClient( m_cl, &argc, NULL ) )
		{
			emit connected();

			m_state = Connected;
			if( m_framebufferUpdateInterval < 0 )
			{
				rfbClientSetClientData( m_cl, (void *) 0x555, (void *) 1 );
			}
		}
		else
		{
			// guess reason why connection failed based on the state,
			// libvncclient left the rfbClient structure
			if( argc < 0 )
			{
				m_state = HostUnreachable;
			}
			else if( argc > 0 )
			{
				m_state = AuthenticationFailed;
			}
			else
			{
				// failed for an unknown reason
				m_state = ConnectionFailed;
			}

			// do not sleep when already requested to stop
			if( m_stopped )
			{
				break;
			}

			// wait a bit until next connect
			sleeperMutex.lock();
			if( m_framebufferUpdateInterval > 0 )
			{
				m_updateIntervalSleeper.wait( &sleeperMutex,
												m_framebufferUpdateInterval );
			}
			else
			{
				// default: retry every second
				m_updateIntervalSleeper.wait( &sleeperMutex, 1000 );
			}
			sleeperMutex.unlock();
		}
	}

	QTime lastFullUpdate = QTime::currentTime();

	// Main VNC event loop
	while( !m_stopped )
	{
		int timeout = 500;
		if( m_framebufferUpdateInterval < 0 )
		{
			timeout = 100*1000;	// 100 ms
		}
		const int i = WaitForMessage( m_cl, timeout );
		if( i < 0 )
		{
			break;
		}
		else if( i )
		{
			// read and process remaining messages many messages as available
			bool handledOkay = true;
			while( WaitForMessage( m_cl, 0 ) && handledOkay )
			{
				handledOkay = HandleRFBServerMessage( m_cl );
			}
			if( handledOkay == false )
			{
				break;
			}
		}
		else
		{
			// work around a bug in UltraVNC on Win7 where it does not handle
			// incremental updates correctly
			int msecs = lastFullUpdate.msecsTo( QTime::currentTime() );
			if( ( m_framebufferUpdateInterval > 0 &&
					msecs > 10*m_framebufferUpdateInterval ) ||
				( m_framebufferUpdateInterval == 0 && msecs > 1000 ) )
			{
				SendFramebufferUpdateRequest( m_cl, 0, 0,
						framebufferSize().width(), framebufferSize().height(),
						false );
				lastFullUpdate = QTime::currentTime();
			}
		}

		m_mutex.lock();

		while( !m_eventQueue.isEmpty() )
		{
			ClientEvent * clientEvent = m_eventQueue.dequeue();

			// unlock the queue mutex during the runtime of ClientEvent::fire()
			m_mutex.unlock();

			clientEvent->fire( m_cl );
			delete clientEvent;

			// and lock it again
			m_mutex.lock();
		}

		m_mutex.unlock();

		if( m_framebufferUpdateInterval > 0 && m_stopped == false )
		{
			sleeperMutex.lock();
			m_updateIntervalSleeper.wait( &sleeperMutex,
												m_framebufferUpdateInterval );
			sleeperMutex.unlock();
		}
	}

	if( m_state == Connected && m_cl )
	{
		rfbClientCleanup( m_cl );
	}

	m_state = Disconnected;
}




void ItalcVncConnection::enqueueEvent( ClientEvent *e )
{
	QMutexLocker lock( &m_mutex );
	if( m_stopped )
	{
		return;
	}

	m_eventQueue.enqueue( e );
}




void ItalcVncConnection::mouseEvent( int x, int y, int buttonMask )
{
	enqueueEvent( new PointerClientEvent( x, y, buttonMask ) );
}




void ItalcVncConnection::keyEvent( int key, bool pressed )
{
	enqueueEvent( new KeyClientEvent( key, pressed ) );
}




void ItalcVncConnection::clientCut( const QString &text )
{
	enqueueEvent( new ClientCutEvent( strdup( text.toUtf8() ) ) );
}


