/*
 * ItalcVncConnection.cpp - implementation of ItalcVncConnection class
 *
 * Copyright (c) 2008-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QHostAddress>
#include <QMutexLocker>

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "ItalcConfiguration.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "SocketDevice.h"
#include "VariantArrayMessage.h"


class KeyClientEvent : public MessageEvent
{
public:
	KeyClientEvent( unsigned int key, bool pressed ) :
		m_key( key ),
		m_pressed( pressed )
	{
	}

	virtual void fire( rfbClient *cl )
	{
		SendKeyEvent( cl, m_key, m_pressed );
	}

private:
	unsigned int m_key;
	bool m_pressed;
} ;



class PointerClientEvent : public MessageEvent
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



class ClientCutEvent : public MessageEvent
{
public:
	ClientCutEvent( const QString& text ) :
		m_text( text.toUtf8() )
	{
	}

	virtual void fire( rfbClient *cl )
	{
		SendClientCutText( cl, m_text.data(), m_text.size() );
	}

private:
	QByteArray m_text;
} ;





rfbBool ItalcVncConnection::hookInitFrameBuffer( rfbClient *cl )
{
	ItalcVncConnection * t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0) ;

	const uint64_t size = (uint64_t) cl->width * cl->height * ( cl->format.bitsPerPixel / 8 );

	cl->frameBuffer = new uint8_t[size];

	memset( cl->frameBuffer, '\0', size );

	// initialize framebuffer image which just wraps the allocated memory and ensures cleanup after last
	// image copy using the framebuffer gets destroyed
	t->m_imgLock.lockForWrite();
	t->m_image = QImage( cl->frameBuffer, cl->width, cl->height, QImage::Format_RGB32, framebufferCleanup, cl->frameBuffer );
	t->m_imgLock.unlock();

	// set up pixel format according to QImage
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
	cl->appData.useBGR233 = false;
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
			cl->appData.encodingsString = "tight zrle ultra "
							"copyrect hextile zlib "
							"corre rre raw";
			cl->appData.compressLevel = 9;
			cl->appData.qualityLevel = 5;
			cl->appData.enableJPEG = true;
			break;
		case DemoServerQuality:
			cl->appData.encodingsString = "copyrect corre rre raw";
			//cl->appData.useRemoteCursor = true;
			break;
		case DemoClientQuality:
			//cl->appData.useRemoteCursor = true;
			cl->appData.encodingsString = "tight ultra copyrect "
									"hextile zlib corre rre raw";
			cl->appData.compressLevel = 9;
			cl->appData.qualityLevel = 9;
			cl->appData.enableJPEG = true;
			break;
		default:
			cl->appData.encodingsString = "zrle ultra copyrect "
							"hextile zlib corre rre raw";
			break;
	}

	t->m_frameBufferInitialized = true;

	return true;
}




void ItalcVncConnection::hookUpdateFB( rfbClient *cl, int x, int y, int w, int h )
{
	ItalcVncConnection * t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0 );

	if( t->quality() == DemoServerQuality )
	{
		// if we're providing data for demo server, perform a simple
		// color-reduction for better compression-results
		for( int ry = y; ry < y+h; ++ry )
		{
			QRgb *data = ( (QRgb *) cl->frameBuffer ) + ry * cl->width;
			for( int rx = x; rx < x+w; ++rx )
			{
				data[rx] &= 0xfcfcfc;
			}
		}
	}

	t->imageUpdated( x, y, w, h );
}




void ItalcVncConnection::hookFinishFrameBufferUpdate( rfbClient *cl )
{
	ItalcVncConnection *t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0 );

	if( t )
	{
		t->finishFrameBufferUpdate();
	}
}




rfbBool ItalcVncConnection::hookHandleCursorPos( rfbClient *cl, int x, int y )
{
	ItalcVncConnection * t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0 );
	if( t )
	{
		t->cursorPosChanged( x, y );
	}

	return true;
}




void ItalcVncConnection::hookCursorShape( rfbClient *cl, int xh, int yh, int w, int h, int bpp )
{
	for( int i = 0; i < w*h;++i )
	{
		if( cl->rcMask[i] )
		{
			cl->rcMask[i] = 255;
		}
	}
	QImage alpha( cl->rcMask, w, h, QImage::Format_Indexed8 );

	QImage cursorShape = QImage( cl->rcSource, w, h, QImage::Format_RGB32 ).convertToFormat( QImage::Format_ARGB32 );
	cursorShape.setAlphaChannel( alpha );

	ItalcVncConnection* t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0 );
	t->cursorShapeUpdated( cursorShape, xh, yh );
}



void ItalcVncConnection::hookCutText( rfbClient *cl, const char *text,
										int textlen )
{
	QString cutText = QString::fromUtf8( text, textlen );
	if( !cutText.isEmpty() )
	{
		ItalcVncConnection *t = (ItalcVncConnection *)
										rfbClientGetClientData( cl, 0 );
		t->gotCut( cutText );
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
	qWarning() << "ItalcVncConnection: VNC message:" << message;

#if 0
	if( ( message.contains( "Couldn't convert " ) ) ||
		( message.contains( "Unable to connect to VNC server" ) ) )
	{
		outputErrorMessageString = "Server not found.";
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
#endif
}



void ItalcVncConnection::framebufferCleanup( void *framebuffer )
{
	delete[] (uchar *) framebuffer;
}




ItalcVncConnection::ItalcVncConnection( QObject *parent ) :
	QThread( parent ),
	m_hostReachable( false ),
	m_frameBufferInitialized( false ),
	m_frameBufferValid( false ),
	m_cl( NULL ),
	m_italcAuthType( RfbItalcAuth::DSA ),
	m_quality( DemoClientQuality ),
	m_port( -1 ),
	m_terminateTimer( this ),
	m_framebufferUpdateInterval( 0 ),
	m_image(),
	m_scaledScreenNeedsUpdate( false ),
	m_scaledScreen(),
	m_scaledSize(),
	m_state( Disconnected )
{
	m_terminateTimer.setSingleShot( true );
	m_terminateTimer.setInterval( ThreadTerminationTimeout );

	connect( &m_terminateTimer, &QTimer::timeout, this, &ItalcVncConnection::terminate );

	if( ItalcCore::config().isLogonAuthenticationEnabled() )
	{
		m_italcAuthType = RfbItalcAuth::Logon;
	}
}



ItalcVncConnection::~ItalcVncConnection()
{
	stop();

	if( isRunning() )
	{
		qWarning( "Waiting for VNC connection thread to finish." );
		wait( ThreadTerminationTimeout );
	}

	if( isRunning() )
	{
		qWarning( "Terminating hanging VNC connection thread!" );

		terminate();
		wait();
	}
}




void ItalcVncConnection::stop( bool deleteAfterFinished )
{
	if( isRunning() )
	{
		if( deleteAfterFinished )
		{
			connect( this, &ItalcVncConnection::finished,
					 this, &ItalcVncConnection::deleteLater );
		}

		m_scaledScreen = QImage();

		requestInterruption();

		m_updateIntervalSleeper.wakeAll();

		// terminate thread in background after timeout
		m_terminateTimer.start();

		// stop timer if thread terminates properly before timeout
		connect( this, &ItalcVncConnection::finished,
				 &m_terminateTimer, &QTimer::stop );
	}
	else if( deleteAfterFinished )
	{
		deleteLater();
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

	// is IPv6-mapped IPv4 address?
	QRegExp rx( "::[fF]{4}:(\\d+.\\d+.\\d+.\\d+)" );
	if( rx.indexIn( m_host ) == 0 )
	{
		// then use plain IPv4 address as libvncclient cannot handle
		// IPv6-mapped IPv4 addresses on Windows properly
		m_host = rx.cap( 1 );
	}
	else if( m_host == "::1" )
	{
		m_host = QHostAddress( QHostAddress::LocalHost ).toString();
	}
}




void ItalcVncConnection::setPort( int port )
{
	QMutexLocker locker( &m_mutex );
	m_port = port;
}



const QImage ItalcVncConnection::image( int x, int y, int w, int h ) const
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
	if( m_image.size().isValid() == false ||
			m_scaledSize.isNull() ||
			m_frameBufferValid == false ||
			m_scaledScreenNeedsUpdate == false )
	{
		return;
	}

	QReadLocker locker( &m_imgLock );
	m_scaledScreen = m_image.scaled( m_scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	m_scaledScreenNeedsUpdate = false;
}




void ItalcVncConnection::run()
{
	m_state = Disconnected;
	emit stateChanged();

	rfbClientLog = hookOutputHandler;
	rfbClientErr = hookOutputHandler;

	while( isInterruptionRequested() == false )
	{
		doConnection();
	}

	m_state = Disconnected;
}



void ItalcVncConnection::doConnection()
{
	QMutex sleeperMutex;

	m_state = Connecting;

	m_frameBufferValid = false;
	m_frameBufferInitialized = false;

	while( isInterruptionRequested() == false && m_state != Connected ) // try to connect as long as the server allows
	{
		m_cl = rfbGetClient( 8, 3, 4 );
		m_cl->MallocFrameBuffer = hookInitFrameBuffer;
		m_cl->canHandleNewFBSize = true;
		m_cl->GotFrameBufferUpdate = hookUpdateFB;
		m_cl->FinishedFrameBufferUpdate = hookFinishFrameBufferUpdate;
		m_cl->HandleCursorPos = hookHandleCursorPos;
		m_cl->GotCursorShape = hookCursorShape;
		m_cl->GotXCutText = hookCutText;
		rfbClientSetClientData( m_cl, 0, this );

		m_mutex.lock();

		if( m_port < 0 ) // use default port?
		{
			m_cl->serverPort = ItalcCore::config().computerControlServerPort();
		}
		else
		{
			m_cl->serverPort = m_port;
		}

		free( m_cl->serverHost );
		m_cl->serverHost = strdup( m_host.toUtf8().constData() );

		m_mutex.unlock();

		emit newClient( m_cl );

		m_hostReachable = false;

		if( rfbInitClient( m_cl, NULL, NULL ) )
		{
			emit connected();

			m_state = Connected;
			emit stateChanged();
		}
		else
		{
			// guess reason why connection failed
			if( m_hostReachable == false )
			{
				m_state = HostUnreachable;
				emit stateChanged();
			}
			else if( m_frameBufferInitialized == false )
			{
				m_state = AuthenticationFailed;
				emit stateChanged();
			}
			else
			{
				// failed for an unknown reason
				m_state = ConnectionFailed;
				emit stateChanged();
			}

			// do not sleep when already requested to stop
			if( isInterruptionRequested() )
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

	QTime connectionTime;
	connectionTime.restart();

	QTime lastFullUpdateTime;
	lastFullUpdateTime.restart();

	// Main VNC event loop
	while( isInterruptionRequested() == false )
	{
		if( m_frameBufferValid == false )
		{
			// initial framebuffer timeout exceeded?
			if( connectionTime.elapsed() < InitialFrameBufferTimeout )
			{
				// not yet so again request initial full framebuffer update
				SendFramebufferUpdateRequest( m_cl, 0, 0,
											  framebufferSize().width(), framebufferSize().height(),
											  false );
			}
			else
			{
				qDebug( "ItalcVncConnection: InitialFrameBufferTimeout exceeded - disconnecting" );
				// no so disconnect and try again
				break;
			}
		}

		const int i = WaitForMessage( m_cl, 500 );
		if( isInterruptionRequested() || i < 0 )
		{
			break;
		}
		else if( i )
		{
			// handle all available messages
			bool handledOkay = true;
			do {
				if( !HandleRFBServerMessage( m_cl ) )
				{
					handledOkay = false;
				}
			} while( handledOkay && WaitForMessage( m_cl, 0 ) );

			if( handledOkay == false )
			{
				break;
			}
		}

		// ensure that we're not missing updates due to slow update rate therefore
		// regularly request full updates
		if( m_framebufferUpdateInterval > 0 &&
					lastFullUpdateTime.elapsed() > 10*m_framebufferUpdateInterval )
		{
			SendFramebufferUpdateRequest( m_cl, 0, 0,
										  framebufferSize().width(), framebufferSize().height(),
										  false );
			lastFullUpdateTime.restart();
		}

		m_mutex.lock();

		while( !m_eventQueue.isEmpty() )
		{
			MessageEvent * clientEvent = m_eventQueue.dequeue();

			// unlock the queue mutex during the runtime of ClientEvent::fire()
			m_mutex.unlock();

			clientEvent->fire( m_cl );
			delete clientEvent;

			// and lock it again
			m_mutex.lock();
		}

		m_mutex.unlock();

		if( m_framebufferUpdateInterval > 0 && isInterruptionRequested() == false )
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

	emit stateChanged();
}



void ItalcVncConnection::finishFrameBufferUpdate()
{
	if( m_frameBufferValid == false )
	{
		m_frameBufferValid = true;

		emit framebufferSizeChanged( m_image.width(), m_image.height() );
	}

	emit framebufferUpdateComplete();

	m_scaledScreenNeedsUpdate = true;
}



void ItalcVncConnection::enqueueEvent( MessageEvent *e )
{
	QMutexLocker lock( &m_mutex );
	if( m_state != Connected )
	{
		return;
	}

	m_eventQueue.enqueue( e );
}




void ItalcVncConnection::mouseEvent( int x, int y, int buttonMask )
{
	enqueueEvent( new PointerClientEvent( x, y, buttonMask ) );
}




void ItalcVncConnection::keyEvent( unsigned int key, bool pressed )
{
	enqueueEvent( new KeyClientEvent( key, pressed ) );
}




void ItalcVncConnection::clientCut( const QString &text )
{
	enqueueEvent( new ClientCutEvent( text ) );
}




void ItalcVncConnection::handleSecTypeItalc( rfbClient *client )
{
	SocketDevice socketDevice( libvncClientDispatcher, client );
	VariantArrayMessage message( &socketDevice );
	message.receive();

	int authTypeCount = message.read().toInt();

	QList<RfbItalcAuth::Type> authTypes;
	for( int i = 0; i < authTypeCount; ++i )
	{
		authTypes.append( message.read().value<RfbItalcAuth::Type>() );
	}

	qDebug() << "ItalcVncConnection::handleSecTypeItalc(): received authentication types:" << authTypes;

	RfbItalcAuth::Type chosenAuthType = RfbItalcAuth::Token;
	if( authTypes.count() > 0 )
	{
		chosenAuthType = authTypes.first();

		// look whether the ItalcVncConnection recommends a specific
		// authentication type (e.g. ItalcAuthHostBased when running as
		// demo client)
		ItalcVncConnection *t = (ItalcVncConnection *) rfbClientGetClientData( client, 0 );

		if( t != NULL )
		{
			for( auto authType : authTypes )
			{
				if( t->italcAuthType() == authType )
				{
					chosenAuthType = authType;
				}
			}
		}
	}

	qDebug() << "ItalcVncConnection::handleSecTypeItalc(): chose authentication type" << chosenAuthType;
	VariantArrayMessage authReplyMessage( &socketDevice );

	authReplyMessage.write( chosenAuthType );

	// send username which is used when displaying an access confirm dialog
	if( ItalcCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
	{
		authReplyMessage.write( ItalcCore::authenticationCredentials().logonUsername() );
	}
	else
	{
		authReplyMessage.write( LocalSystem::User::loggedOnUser().name() );
	}

	authReplyMessage.send();

	switch( chosenAuthType )
	{
	case RfbItalcAuth::DSA:
		if( ItalcCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::PrivateKey ) )
		{
			VariantArrayMessage challengeReceiveMessage( &socketDevice );
			challengeReceiveMessage.receive();
			QByteArray challenge = challengeReceiveMessage.read().toByteArray();
			QByteArray signature = ItalcCore::authenticationCredentials().
					privateKey().signMessage( challenge, CryptoCore::DefaultSignatureAlgorithm );

			VariantArrayMessage challengeResponseMessage( &socketDevice );
			challengeResponseMessage.write( ItalcCore::instance()->userRole() );
			challengeResponseMessage.write( signature );
			challengeResponseMessage.send();
		}
		break;

	case RfbItalcAuth::HostWhiteList:
		// nothing to do - we just get accepted because the host white list contains our IP
		break;

	case RfbItalcAuth::Logon:
	{
		VariantArrayMessage publicKeyMessage( &socketDevice );
		publicKeyMessage.receive();

		CryptoCore::PublicKey publicKey = CryptoCore::PublicKey::fromPEM( publicKeyMessage.read().toString() );

		if( publicKey.canEncrypt() == false )
		{
			qCritical( "ItalcVncConnection::handleSecTypeItalc(): can't encrypt with given public key!" );
			break;
		}

		CryptoCore::SecureArray plainTextPassword( ItalcCore::authenticationCredentials().logonPassword().toUtf8() );
		CryptoCore::SecureArray encryptedPassword = publicKey.encrypt( plainTextPassword, CryptoCore::DefaultEncryptionAlgorithm );
		if( encryptedPassword.isEmpty() )
		{
			qCritical( "ItalcVncConnection::handleSecTypeItalc(): password encryption failed!" );
			break;
		}

		VariantArrayMessage passwordResponse( &socketDevice );
		passwordResponse.write( encryptedPassword.toByteArray() );
		passwordResponse.send();
		break;
	}

	case RfbItalcAuth::Token:
	{
		VariantArrayMessage tokenAuthMessage( &socketDevice );
		tokenAuthMessage.write( ItalcCore::authenticationCredentials().token() );
		tokenAuthMessage.send();
		break;
	}

	default:
		// nothing to do - we just get accepted
		break;
	}
}



void ItalcVncConnection::hookPrepareAuthentication(rfbClient *cl)
{
	ItalcVncConnection* t = (ItalcVncConnection *) rfbClientGetClientData( cl, 0 );

	// set our internal flag which indicates that we basically have communication with the client
	// which means that the host is reachable
	t->m_hostReachable = true;
}



qint64 ItalcVncConnection::libvncClientDispatcher( char* buffer, const qint64 bytes,
												   SocketDevice::SocketOperation operation, void* user )
{
	rfbClient * cl = (rfbClient *) user;
	switch( operation )
	{
	case SocketDevice::SocketOpRead:
		return ReadFromRFBServer( cl, buffer, bytes ) ? bytes : 0;

	case SocketDevice::SocketOpWrite:
		return WriteToRFBServer( cl, buffer, bytes ) ? bytes : 0;
	}

	return 0;
}



void handleSecTypeItalc( rfbClient *client )
{
	ItalcVncConnection::hookPrepareAuthentication( client );
	ItalcVncConnection::handleSecTypeItalc( client );
}
