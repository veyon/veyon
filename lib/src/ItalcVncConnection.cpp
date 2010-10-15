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

#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>


static QString outputErrorMessageString;



class KeyClientEvent : public ClientEvent
{
public:
	KeyClientEvent( int _key, int _pressed ) :
		m_key( _key ),
		m_pressed( _pressed )
	{
	}

	virtual void fire( rfbClient * _cl )
	{
		SendKeyEvent( _cl, m_key, m_pressed );
	}

private:
	int m_key;
	int m_pressed;
} ;



class PointerClientEvent : public ClientEvent
{
public:
	PointerClientEvent( int _x, int _y, int _buttonMask ) :
		m_x( _x ),
		m_y( _y ),
		m_buttonMask( _buttonMask )
	{
	}

	virtual void fire( rfbClient * _cl )
	{
		SendPointerEvent( _cl, m_x, m_y, m_buttonMask );
	}

private:
	int m_x;
	int m_y;
	int m_buttonMask;
} ;



class ClientCutEvent : public ClientEvent
{
public:
	ClientCutEvent( char * _text ) :
	    m_text( _text )
	{
	}

	virtual void fire( rfbClient * _cl )
	{
		SendClientCutText( _cl, m_text, qstrlen( m_text ) );
	}

private:
	char * m_text;
} ;





rfbBool ItalcVncConnection::hookNewClient( rfbClient * _cl )
{
	ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( _cl, 0) ;

	const int size = (int) _cl->width * _cl->height *
					( _cl->format.bitsPerPixel / 8 );
	if( t->frameBuffer )
	{
		// do not leak if we get a new framebuffer size
		delete [] t->frameBuffer;
	}
	t->frameBuffer = new uint8_t[size];
	_cl->frameBuffer = t->frameBuffer;
	memset( _cl->frameBuffer, '\0', size );
	_cl->format.bitsPerPixel = 32;
	_cl->format.redShift = 16;
	_cl->format.greenShift = 8;
	_cl->format.blueShift = 0;
	_cl->format.redMax = 0xff;
	_cl->format.greenMax = 0xff;
	_cl->format.blueMax = 0xff;

	// only use remote cursor for remote control
	_cl->appData.useRemoteCursor = false;

	switch( t->quality() )
	{
		case SnapshotQuality:
			_cl->appData.useBGR233 = 0;
			_cl->appData.encodingsString = "raw";
			_cl->appData.compressLevel = 0;
			_cl->appData.qualityLevel = 9;
			_cl->appData.enableJPEG = false;
			break;
		case RemoteControlQuality:
			_cl->appData.useBGR233 = 0;
			_cl->appData.encodingsString = "zrle ultra copyrect "
							"hextile zlib raw";
			_cl->appData.compressLevel = 0;
			_cl->appData.qualityLevel = 9;
			_cl->appData.enableJPEG = false;
			_cl->appData.useRemoteCursor = true;
			break;
		case ThumbnailQuality:
			_cl->appData.useBGR233 = 1;
			_cl->appData.encodingsString = "tight zrle ultra "
							"copyrect hextile zlib "
							"corre rre raw";
			_cl->appData.compressLevel = 4;
			_cl->appData.qualityLevel = 4;
			_cl->appData.enableJPEG = true;
			break;
		default:
		case DemoQuality:
			_cl->appData.useBGR233 = 0;
			_cl->appData.encodingsString = "tight zrle ultra "
							"copyrect hextile zlib "
							"corre rre raw";
			_cl->appData.compressLevel = 4;
			_cl->appData.qualityLevel = 9;
			_cl->appData.enableJPEG = true;
			break;
	}

	SetFormatAndEncodings( _cl );

	return true;
}




void ItalcVncConnection::hookUpdateFB( rfbClient * _cl, int _x, int _y, int _w,
									int _h )
{
	QImage img( _cl->frameBuffer, _cl->width, _cl->height,
							QImage::Format_RGB32 );

	if( img.isNull() )
	{
		qWarning( "image not loaded" );
	}

	ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( _cl, 0 );
	t->setImage( img );
	t->m_scaledScreenNeedsUpdate = true;
	t->emitUpdated( _x, _y, _w, _h );
}



void ItalcVncConnection::hookCursorShape( rfbClient *cl, int xh, int yh,
											int w, int h, int bpp )
{
	const int bytesPerRow = (w + 7) / 8;
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



void ItalcVncConnection::hookCutText( rfbClient * _cl, const char * _text,
								int _textlen )
{
	QString cutText = QString::fromUtf8( _text, _textlen );
	if( !cutText.isEmpty() )
	{
	        ItalcVncConnection * t = (ItalcVncConnection *)
					rfbClientGetClientData( _cl, 0);
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




ItalcVncConnection::ItalcVncConnection( QObject * _parent ) :
	QThread( _parent ),
	frameBuffer( NULL ),
	m_stopped( false ),
	m_connected( false ),
	m_cl( NULL ),
	m_quality( DemoQuality ),
	m_port( PortOffsetIVS ),
	m_framebufferUpdateInterval( 0 ),
	m_image(),
	m_scaledScreenNeedsUpdate( false ),
	m_scaledScreen(),
	m_scaledSize()
{
	m_stopped = false;
}



ItalcVncConnection::~ItalcVncConnection()
{
	stop();

	delete [] frameBuffer;
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
	m_stopped = true;
	if( !wait( 1000 ) )
	{
		terminate();
	}
}




void ItalcVncConnection::reset( const QString & _host )
{
	if( !m_connected && isRunning() )
	{
		setHost( _host );
	}
	else
	{
		stop();
		setHost( _host );
		start();
	}
}




void ItalcVncConnection::setHost( const QString & _host )
{
	QMutexLocker locker( &m_mutex );
	m_host = _host;
	if( m_host.contains( ':' ) )
	{
		m_port = m_host.section( ':', 1, 1 ).toInt();
		m_host = m_host.section( ':', 0, 0 );
	}
}




void ItalcVncConnection::setPort( int _port )
{
	QMutexLocker locker( &m_mutex );
	m_port = _port;
}




void ItalcVncConnection::setImage( const QImage & _img )
{
	m_imgLock.lockForWrite();
	m_image = _img;
	m_imgLock.unlock();
}




const QImage ItalcVncConnection::image( int _x, int _y, int _w, int _h )
{
	QReadLocker locker( &m_imgLock );

	if( _w == 0 || _h == 0 ) // full image requested
	{
		return m_image;
	}
	return m_image.copy( _x, _y, _w, _h );
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




void ItalcVncConnection::emitUpdated( int _x, int _y, int _w, int _h )
{
	emit imageUpdated( _x, _y, _w, _h );
}




void ItalcVncConnection::emitCursorShapeUpdated( const QImage &cursorShape,
														int xh, int yh )
{
	emit cursorShapeUpdated( cursorShape, xh, yh );
}




void ItalcVncConnection::emitGotCut( const QString & _text )
{
	emit gotCut( _text );
}




void ItalcVncConnection::run()
{
	m_stopped = false;

	rfbClientLog = hookOutputHandler;
	rfbClientErr = hookOutputHandler;

	while( !m_stopped && !m_connected ) // try to connect as long as the server allows
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

		if( rfbInitClient( m_cl, 0, 0 ) )
		{
			emit connected();

			m_connected = true;
			if( m_framebufferUpdateInterval < 0 )
			{
				rfbClientSetClientData( m_cl, (void *) 0x555, (void *) 1 );
			}
		}
	}


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
			if( !HandleRFBServerMessage( m_cl ) )
			{
				break;
			}
		}

		m_mutex.lock();

		while( !m_eventQueue.isEmpty() )
		{
			ClientEvent * clientEvent = m_eventQueue.dequeue();
			clientEvent->fire( m_cl );
			delete clientEvent;
		}

		m_mutex.unlock();

		if( m_framebufferUpdateInterval > 0 )
		{
			msleep( m_framebufferUpdateInterval );
		}
	}

	if( m_connected && m_cl )
	{
		rfbClientCleanup( m_cl );
		m_connected = false;
	}

	// Cleanup allocated resources
	m_stopped = true;
}




void ItalcVncConnection::enqueueEvent( ClientEvent * _e )
{
	QMutexLocker lock( &m_mutex );
	if( m_stopped )
	{
		return;
	}

	m_eventQueue.enqueue( _e );
}




void ItalcVncConnection::mouseEvent( int _x, int _y, int _buttonMask )
{
	enqueueEvent( new PointerClientEvent( _x, _y, _buttonMask ) );
}




void ItalcVncConnection::keyEvent( int _key, bool _pressed )
{
	enqueueEvent( new KeyClientEvent( _key, _pressed ) );
}




void ItalcVncConnection::clientCut( const QString & _text )
{
	enqueueEvent( new ClientCutEvent( strdup( _text.toUtf8() ) ) );
}


