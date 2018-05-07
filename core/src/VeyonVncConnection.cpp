/*
 * VeyonVncConnection.cpp - implementation of VeyonVncConnection class
 *
 * Copyright (c) 2008-2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QBitmap>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QMutexLocker>
#include <QPixmap>
#include <QTime>

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "PlatformNetworkFunctions.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonVncConnection.h"
#include "SocketDevice.h"
#include "VariantArrayMessage.h"

extern "C"
{
	#include <rfb/rfbclient.h>
}

// clazy:excludeall=copyable-polymorphic

class KeyClientEvent : public MessageEvent
{
public:
	KeyClientEvent( unsigned int key, bool pressed ) :
		m_key( key ),
		m_pressed( pressed )
	{
	}

	void fire( rfbClient *cl ) override
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

	void fire( rfbClient *cl ) override
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

	void fire( rfbClient *cl ) override
	{
		SendClientCutText( cl, m_text.data(), m_text.size() ); // clazy:exclude=detaching-member
	}

private:
	QByteArray m_text;
} ;





rfbBool VeyonVncConnection::hookInitFrameBuffer( rfbClient *cl )
{
	VeyonVncConnection * t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr) ;

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

	cl->appData.useRemoteCursor = false;
	cl->appData.compressLevel = 0;
	cl->appData.useBGR233 = false;
	cl->appData.qualityLevel = 9;
	cl->appData.enableJPEG = false;

	switch( t->quality() )
	{
		case ScreenshotQuality:
			cl->appData.encodingsString = "raw";
			break;
		case RemoteControlQuality:
			cl->appData.encodingsString = "copyrect hextile raw";
			//cl->appData.useRemoteCursor = true;
			break;
		case ThumbnailQuality:
			cl->appData.encodingsString = "zrle ultra "
							"copyrect hextile zlib "
							"corre rre raw";
			cl->appData.compressLevel = 9;
			cl->appData.qualityLevel = 5;
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




void VeyonVncConnection::hookUpdateFB( rfbClient *cl, int x, int y, int w, int h )
{
	VeyonVncConnection * t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr );

	if( t )
	{
		emit t->imageUpdated( x, y, w, h );
	}
}




void VeyonVncConnection::hookFinishFrameBufferUpdate( rfbClient *cl )
{
	VeyonVncConnection *t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr );

	if( t )
	{
		t->finishFrameBufferUpdate();
	}
}




rfbBool VeyonVncConnection::hookHandleCursorPos( rfbClient *cl, int x, int y )
{
	VeyonVncConnection * t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr );
	if( t )
	{
		emit t->cursorPosChanged( x, y );
	}

	return true;
}




void VeyonVncConnection::hookCursorShape( rfbClient *cl, int xh, int yh, int w, int h, int bpp )
{
	if( bpp != 4 )
	{
		qWarning( "VeyonVncConnection: bytes per pixel != 4" );
		return;
	}

	QImage alpha( cl->rcMask, w, h, QImage::Format_Indexed8 );
	alpha.setColorTable( { qRgb(255,255,255), qRgb(0,0,0) } );

	QPixmap cursorShape( QPixmap::fromImage( QImage( cl->rcSource, w, h, QImage::Format_RGB32 ) ) );
	cursorShape.setMask( QBitmap::fromImage( alpha ) );

	VeyonVncConnection* t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr );
	emit t->cursorShapeUpdated( cursorShape, xh, yh );
}



void VeyonVncConnection::hookCutText( rfbClient *cl, const char *text,
										int textlen )
{
	QString cutText = QString::fromUtf8( text, textlen );
	if( !cutText.isEmpty() )
	{
		VeyonVncConnection *t = (VeyonVncConnection *)
										rfbClientGetClientData( cl, nullptr );
		emit t->gotCut( cutText );
	}
}




void VeyonVncConnection::hookOutputHandler( const char *format, ... )
{
	va_list args;
	va_start( args, format );

	QString message;
	message.vsprintf( format, args );

	va_end(args);

	message = message.trimmed();
	qDebug() << "VeyonVncConnection: VNC message:" << message;

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



void VeyonVncConnection::framebufferCleanup( void *framebuffer )
{
	delete[] (uchar *) framebuffer;
}




VeyonVncConnection::VeyonVncConnection( QObject *parent ) :
	QThread( parent ),
	m_serviceReachable( false ),
	m_frameBufferInitialized( false ),
	m_frameBufferValid( false ),
	m_cl( nullptr ),
	m_veyonAuthType( RfbVeyonAuth::Logon ),
	m_quality( DefaultQuality ),
	m_port( -1 ),
	m_terminateTimer( this ),
	m_framebufferUpdateInterval( 0 ),
	m_image(),
	m_scaledScreenNeedsUpdate( false ),
	m_scaledScreen(),
	m_scaledSize(),
	m_state( Disconnected )
{
	rfbClientLog = hookOutputHandler;
	rfbClientErr = hookOutputHandler;

	m_terminateTimer.setSingleShot( true );
	m_terminateTimer.setInterval( ThreadTerminationTimeout );

	connect( &m_terminateTimer, &QTimer::timeout, this, &VeyonVncConnection::terminate );

	if( VeyonCore::config().authenticationMethod() == VeyonCore::KeyFileAuthentication )
	{
		m_veyonAuthType = RfbVeyonAuth::KeyFile;
	}
}



VeyonVncConnection::~VeyonVncConnection()
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




void VeyonVncConnection::stop( bool deleteAfterFinished )
{
	if( isRunning() )
	{
		if( deleteAfterFinished )
		{
			connect( this, &VeyonVncConnection::finished,
					 this, &VeyonVncConnection::deleteLater );
		}

		m_scaledScreen = QImage();

		requestInterruption();

		m_updateIntervalSleeper.wakeAll();

		// thread termination causes deadlock when calling any QThread functions such as isRunning()
		// or the destructor if the thread itself is stuck in a blocking (e.g. network) function
		// therefore do not terminate the thread on windows but let it run in background as long
		// as the blocking function is running
#ifndef Q_OS_WIN32
		// terminate thread in background after timeout
		m_terminateTimer.start();
#endif

		// stop timer if thread terminates properly before timeout
		connect( this, &VeyonVncConnection::finished,
				 &m_terminateTimer, &QTimer::stop );
	}
	else if( deleteAfterFinished )
	{
		deleteLater();
	}
}




void VeyonVncConnection::reset( const QString &host )
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




void VeyonVncConnection::setHost( const QString &host )
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
	else if( m_host == QStringLiteral( "::1" ) )
	{
		m_host = QHostAddress( QHostAddress::LocalHost ).toString();
	}
	else if( m_host.count( ':' ) == 1 )
	{
		// host name + port number?
		QRegExp rx2( "(.*[^:]):(\\d+)$" );
		if( rx2.indexIn( m_host ) == 0 )
		{
			m_host = rx2.cap( 1 );
			m_port = rx2.cap( 2 ).toUInt();
		}
	}
}




void VeyonVncConnection::setPort( int port )
{
	if( port >= 0 )
	{
		QMutexLocker locker( &m_mutex );
		m_port = port;
	}
}



QImage VeyonVncConnection::image() const
{
	QReadLocker locker( &m_imgLock );
	return m_image;
}



void VeyonVncConnection::setFramebufferUpdateInterval( int interval )
{
	m_framebufferUpdateInterval = interval;
}




void VeyonVncConnection::rescaleScreen()
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




void VeyonVncConnection::run()
{
	while( isInterruptionRequested() == false )
	{
		establishConnection();
		handleConnection();
		closeConnection();
	}

	setState( Disconnected );
}



void VeyonVncConnection::establishConnection()
{
	QMutex sleeperMutex;

	setState( Connecting );

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
		rfbClientSetClientData( m_cl, nullptr, this );

		m_mutex.lock();

		if( m_port < 0 ) // use default port?
		{
			m_cl->serverPort = VeyonCore::config().primaryServicePort();
		}
		else
		{
			m_cl->serverPort = m_port;
		}

		free( m_cl->serverHost );
		m_cl->serverHost = strdup( m_host.toUtf8().constData() );

		m_mutex.unlock();

		emit newClient( m_cl );

		m_serviceReachable = false;

		if( rfbInitClient( m_cl, nullptr, nullptr ) )
		{
			setState( Connected );
		}
		else
		{
			// rfbInitClient() calls rfbClientCleanup() when failed
			m_cl = nullptr;

			// guess reason why connection failed
			if( m_serviceReachable == false )
			{
				if( VeyonCore::platform().networkFunctions().ping( m_host ) == false )
				{
					setState( HostOffline );
				}
				else
				{
					setState( ServiceUnreachable );
				}
			}
			else if( m_frameBufferInitialized == false )
			{
				setState( AuthenticationFailed );
			}
			else
			{
				// failed for an unknown reason
				setState( ConnectionFailed );
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
}



void VeyonVncConnection::handleConnection()
{
	QMutex sleeperMutex;

	QElapsedTimer updateTimer;
	QElapsedTimer connectionTime;

	connectionTime.start();

	// Main VNC event loop
	while( isInterruptionRequested() == false )
	{
		updateTimer.start();

		const int i = WaitForMessage( m_cl, MessageWaitTimeout );
		if( isInterruptionRequested() || i < 0 )
		{
			break;
		}
		else if( i )
		{
			// handle all available messages
			bool handledOkay = true;
			do {
				handledOkay &= HandleRFBServerMessage( m_cl );
			} while( handledOkay && WaitForMessage( m_cl, 0 ) );

			if( handledOkay == false )
			{
				break;
			}
		}

		if( m_frameBufferValid == false )
		{
			// initial framebuffer timeout exceeded?
			if( connectionTime.hasExpired( InitialFrameBufferTimeout ) )
			{
				// no so disconnect and try again
				qDebug( "VeyonVncConnection: InitialFrameBufferTimeout exceeded - disconnecting" );
				break;
			}
			else
			{
				// not yet so again request initial full framebuffer update
				SendFramebufferUpdateRequest( m_cl, 0, 0, framebufferSize().width(), framebufferSize().height(), false );
			}
		}
		else
		{
			SendFramebufferUpdateRequest( m_cl, 0, 0, framebufferSize().width(), framebufferSize().height(), true );
		}

		sendEvents();

		auto remainingUpdateInterval = m_framebufferUpdateInterval - updateTimer.elapsed();

		if( remainingUpdateInterval > 0 && isInterruptionRequested() == false )
		{
			sleeperMutex.lock();
			m_updateIntervalSleeper.wait( &sleeperMutex, remainingUpdateInterval );
			sleeperMutex.unlock();
		}
	}

	sendEvents();
}



void VeyonVncConnection::closeConnection()
{
	if( m_cl )
	{
		rfbClientCleanup( m_cl );
		m_cl = nullptr;
	}

	setState( Disconnected );
}



void VeyonVncConnection::setState( State state )
{
	if( state != m_state )
	{
		m_state = state;

		emit stateChanged();
	}
}



void VeyonVncConnection::finishFrameBufferUpdate()
{
	if( m_frameBufferValid == false )
	{
		m_frameBufferValid = true;

		emit framebufferSizeChanged( m_image.width(), m_image.height() );
	}

	emit framebufferUpdateComplete();

	m_scaledScreenNeedsUpdate = true;
}



void VeyonVncConnection::sendEvents()
{
	m_mutex.lock();

	while( m_eventQueue.isEmpty() == false )
	{
		auto event = m_eventQueue.dequeue();

		// unlock the queue mutex during the runtime of ClientEvent::fire()
		m_mutex.unlock();

		event->fire( m_cl );
		delete event;

		// and lock it again
		m_mutex.lock();
	}

	m_mutex.unlock();
}



void VeyonVncConnection::enqueueEvent( MessageEvent *e )
{
	QMutexLocker lock( &m_mutex );
	if( m_state != Connected )
	{
		return;
	}

	m_eventQueue.enqueue( e );
}




void VeyonVncConnection::mouseEvent( int x, int y, int buttonMask )
{
	enqueueEvent( new PointerClientEvent( x, y, buttonMask ) );
}




void VeyonVncConnection::keyEvent( unsigned int key, bool pressed )
{
	enqueueEvent( new KeyClientEvent( key, pressed ) );
}




void VeyonVncConnection::clientCut( const QString &text )
{
	enqueueEvent( new ClientCutEvent( text ) );
}




void VeyonVncConnection::handleSecTypeVeyon( rfbClient *client )
{
	SocketDevice socketDevice( libvncClientDispatcher, client );
	VariantArrayMessage message( &socketDevice );
	message.receive();

	int authTypeCount = message.read().toInt();

	QList<RfbVeyonAuth::Type> authTypes;
	authTypes.reserve( authTypeCount );

	for( int i = 0; i < authTypeCount; ++i )
	{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		authTypes.append( static_cast<RfbVeyonAuth::Type>( message.read().toInt() ) );
#else
		authTypes.append( message.read().value<RfbVeyonAuth::Type>() );
#endif
	}

	qDebug() << "VeyonVncConnection::handleSecTypeVeyon(): received authentication types:" << authTypes;

	RfbVeyonAuth::Type chosenAuthType = RfbVeyonAuth::Token;
	if( authTypes.count() > 0 )
	{
		chosenAuthType = authTypes.first();

		// look whether the VeyonVncConnection recommends a specific
		// authentication type (e.g. VeyonAuthHostBased when running as
		// demo client)
		VeyonVncConnection *t = (VeyonVncConnection *) rfbClientGetClientData( client, nullptr );

		if( t != nullptr )
		{
			for( auto authType : authTypes )
			{
				if( t->veyonAuthType() == authType )
				{
					chosenAuthType = authType;
				}
			}
		}
	}

	qDebug() << "VeyonVncConnection::handleSecTypeVeyon(): chose authentication type" << chosenAuthType;
	VariantArrayMessage authReplyMessage( &socketDevice );

	authReplyMessage.write( chosenAuthType );

	// send username which is used when displaying an access confirm dialog
	if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
	{
		authReplyMessage.write( VeyonCore::authenticationCredentials().logonUsername() );
	}
	else
	{
		authReplyMessage.write( VeyonCore::platform().userFunctions().currentUser() );
	}

	authReplyMessage.send();

	VariantArrayMessage authAckMessage( &socketDevice );
	authAckMessage.receive();

	switch( chosenAuthType )
	{
	case RfbVeyonAuth::KeyFile:
		if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::PrivateKey ) )
		{
			VariantArrayMessage challengeReceiveMessage( &socketDevice );
			challengeReceiveMessage.receive();
			const auto challenge = challengeReceiveMessage.read().toByteArray();

			if( challenge.size() != CryptoCore::ChallengeSize )
			{
				qCritical( "VeyonVncConnection::handleSecTypeVeyon(): challenge size mismatch!" );
				break;
			}

			// create local copy of private key so we can modify it within our own thread
			auto key = VeyonCore::authenticationCredentials().privateKey();
			if( key.isNull() || key.canSign() == false )
			{
				qCritical( "VeyonVncConnection::handleSecTypeVeyon(): invalid private key!" );
				break;
			}

			const auto signature = key.signMessage( challenge, CryptoCore::DefaultSignatureAlgorithm );

			VariantArrayMessage challengeResponseMessage( &socketDevice );
			challengeResponseMessage.write( VeyonCore::instance()->authenticationKeyName() );
			challengeResponseMessage.write( signature );
			challengeResponseMessage.send();
		}
		break;

	case RfbVeyonAuth::HostWhiteList:
		// nothing to do - we just get accepted because the host white list contains our IP
		break;

	case RfbVeyonAuth::Logon:
	{
		VariantArrayMessage publicKeyMessage( &socketDevice );
		publicKeyMessage.receive();

		CryptoCore::PublicKey publicKey = CryptoCore::PublicKey::fromPEM( publicKeyMessage.read().toString() );

		if( publicKey.canEncrypt() == false )
		{
			qCritical( "VeyonVncConnection::handleSecTypeVeyon(): can't encrypt with given public key!" );
			break;
		}

		CryptoCore::SecureArray plainTextPassword( VeyonCore::authenticationCredentials().logonPassword().toUtf8() );
		CryptoCore::SecureArray encryptedPassword = publicKey.encrypt( plainTextPassword, CryptoCore::DefaultEncryptionAlgorithm );
		if( encryptedPassword.isEmpty() )
		{
			qCritical( "VeyonVncConnection::handleSecTypeVeyon(): password encryption failed!" );
			break;
		}

		VariantArrayMessage passwordResponse( &socketDevice );
		passwordResponse.write( encryptedPassword.toByteArray() );
		passwordResponse.send();
		break;
	}

	case RfbVeyonAuth::Token:
	{
		VariantArrayMessage tokenAuthMessage( &socketDevice );
		tokenAuthMessage.write( VeyonCore::authenticationCredentials().token() );
		tokenAuthMessage.send();
		break;
	}

	default:
		// nothing to do - we just get accepted
		break;
	}
}



void VeyonVncConnection::hookPrepareAuthentication(rfbClient *cl)
{
	VeyonVncConnection* t = (VeyonVncConnection *) rfbClientGetClientData( cl, nullptr );

	// set our internal flag which indicates that we basically have communication with the client
	// which means that the host is reachable
	t->m_serviceReachable = true;
}



qint64 VeyonVncConnection::libvncClientDispatcher( char* buffer, const qint64 bytes,
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



void handleSecTypeVeyon( rfbClient *client )
{
	VeyonVncConnection::hookPrepareAuthentication( client );
	VeyonVncConnection::handleSecTypeVeyon( client );
}
