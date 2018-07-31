/*
 * VncConnection.cpp - implementation of VncConnection class
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
#include "VncConnection.h"
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

	void fire( rfbClient* client ) override
	{
		SendKeyEvent( client, m_key, m_pressed );
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

	void fire( rfbClient* client ) override
	{
		SendPointerEvent( client, m_x, m_y, m_buttonMask );
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

	void fire( rfbClient* client ) override
	{
		SendClientCutText( client, m_text.data(), m_text.size() ); // clazy:exclude=detaching-member
	}

private:
	QByteArray m_text;
} ;





rfbBool VncConnection::hookInitFrameBuffer( rfbClient* client )
{
	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	if( connection )
	{
		return connection->initFrameBuffer( client );
	}

	return false;
}




void VncConnection::hookUpdateFB( rfbClient* client, int x, int y, int w, int h )
{
	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	if( connection )
	{
		emit connection->imageUpdated( x, y, w, h );
	}
}




void VncConnection::hookFinishFrameBufferUpdate( rfbClient* client )
{
	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	if( connection )
	{
		connection->finishFrameBufferUpdate();
	}
}




rfbBool VncConnection::hookHandleCursorPos( rfbClient* client, int x, int y )
{
	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	if( connection )
	{
		emit connection->cursorPosChanged( x, y );
	}

	return true;
}




void VncConnection::hookCursorShape( rfbClient* client, int xh, int yh, int w, int h, int bpp )
{
	if( bpp != 4 )
	{
		qWarning( "VncConnection: bytes per pixel != 4" );
		return;
	}

	QImage alpha( client->rcMask, w, h, QImage::Format_Indexed8 );
	alpha.setColorTable( { qRgb(255,255,255), qRgb(0,0,0) } );

	QPixmap cursorShape( QPixmap::fromImage( QImage( client->rcSource, w, h, QImage::Format_RGB32 ) ) );
	cursorShape.setMask( QBitmap::fromImage( alpha ) );

	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	emit connection->cursorShapeUpdated( cursorShape, xh, yh );
}



void VncConnection::hookCutText( rfbClient* client, const char* text, int textlen )
{
	QString cutText = QString::fromUtf8( text, textlen );
	if( !cutText.isEmpty() )
	{
		auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
		emit connection->gotCut( cutText );
	}
}




void VncConnection::hookOutputHandler( const char* format, ... )
{
	va_list args;
	va_start( args, format );

	const auto message = QString::asprintf( format, args ); // Flawfinder: ignore

	va_end(args);

	qDebug() << "VncConnection: VNC message:" << message.trimmed();
}



void VncConnection::framebufferCleanup( void* framebuffer )
{
	delete[] static_cast<uchar *>( framebuffer );
}




VncConnection::VncConnection( QObject* parent ) :
	QThread( parent ),
	m_state( Disconnected ),
	m_framebufferState( FramebufferInvalid ),
	m_controlFlags(),
	m_client( nullptr ),
	m_quality( DefaultQuality ),
	m_host(),
	m_port( -1 ),
	m_veyonAuthType( RfbVeyonAuth::Logon ),
	m_globalMutex(),
	m_controlFlagMutex(),
	m_updateIntervalSleeper(),
	m_terminateTimer( this ),
	m_framebufferUpdateInterval( 0 ),
	m_image(),
	m_scaledScreen(),
	m_scaledSize(),
	m_imgLock()
{
	rfbClientLog = hookOutputHandler;
	rfbClientErr = hookOutputHandler;

	m_terminateTimer.setSingleShot( true );
	m_terminateTimer.setInterval( ThreadTerminationTimeout );

	connect( &m_terminateTimer, &QTimer::timeout, this, &VncConnection::terminate );

	if( VeyonCore::config().authenticationMethod() == VeyonCore::KeyFileAuthentication )
	{
		m_veyonAuthType = RfbVeyonAuth::KeyFile;
	}
}



VncConnection::~VncConnection()
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




void VncConnection::stop( bool deleteAfterFinished )
{
	if( isRunning() )
	{
		if( deleteAfterFinished )
		{
			connect( this, &VncConnection::finished,
					 this, &VncConnection::deleteLater );
		}

		m_scaledScreen = QImage();

		setControlFlag( TerminateThread, true );

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
		connect( this, &VncConnection::finished,
				 &m_terminateTimer, &QTimer::stop );
	}
	else if( deleteAfterFinished )
	{
		deleteLater();
	}
}



void VncConnection::setHost( const QString& host )
{
	QMutexLocker locker( &m_globalMutex );
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
			m_port = rx2.cap( 2 ).toInt();
		}
	}
}




void VncConnection::setPort( int port )
{
	if( port >= 0 )
	{
		QMutexLocker locker( &m_globalMutex );
		m_port = port;
	}
}



QImage VncConnection::image() const
{
	QReadLocker locker( &m_imgLock );
	return m_image;
}



void VncConnection::restart()
{
	setControlFlag( RestartConnection, true );
}



void VncConnection::setFramebufferUpdateInterval( int interval )
{
	m_framebufferUpdateInterval = interval;
}




void VncConnection::rescaleScreen()
{
	if( m_image.size().isValid() == false ||
			m_scaledSize.isNull() ||
			hasValidFrameBuffer() == false ||
			isControlFlagSet( ScaledScreenNeedsUpdate ) == false )
	{
		return;
	}

	QReadLocker locker( &m_imgLock );
	m_scaledScreen = m_image.scaled( m_scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	setControlFlag( ScaledScreenNeedsUpdate, false );
}




void VncConnection::run()
{
	while( isControlFlagSet( TerminateThread ) == false )
	{
		establishConnection();
		handleConnection();
		closeConnection();
	}
}



void VncConnection::establishConnection()
{
	QMutex sleeperMutex;

	setState( Connecting );
	setControlFlag( RestartConnection, false );

	m_framebufferState = FramebufferInvalid;

	while( isControlFlagSet( TerminateThread ) == false && m_state != Connected ) // try to connect as long as the server allows
	{
		m_client = rfbGetClient( RfbBitsPerSample, RfbSamplesPerPixel, RfbBytesPerPixel );
		m_client->MallocFrameBuffer = hookInitFrameBuffer;
		m_client->canHandleNewFBSize = true;
		m_client->GotFrameBufferUpdate = hookUpdateFB;
		m_client->FinishedFrameBufferUpdate = hookFinishFrameBufferUpdate;
		m_client->HandleCursorPos = hookHandleCursorPos;
		m_client->GotCursorShape = hookCursorShape;
		m_client->GotXCutText = hookCutText;
		rfbClientSetClientData( m_client, nullptr, this );

		m_globalMutex.lock();

		if( m_port < 0 ) // use default port?
		{
			m_client->serverPort = VeyonCore::config().primaryServicePort();
		}
		else
		{
			m_client->serverPort = m_port;
		}

		free( m_client->serverHost );
		m_client->serverHost = strdup( m_host.toUtf8().constData() );

		m_globalMutex.unlock();

		emit newClient( m_client );

		setControlFlag( ServerReachable, false );

		if( rfbInitClient( m_client, nullptr, nullptr ) )
		{
			VeyonCore::platform().networkFunctions().configureSocketKeepalive( m_client->sock, true, SocketKeepaliveIdleTime,
																			   SocketKeepaliveInterval, SocketKeepaliveCount );

			setState( Connected );
		}
		else
		{
			// rfbInitClient() calls rfbClientCleanup() when failed
			m_client = nullptr;

			// guess reason why connection failed
			if( isControlFlagSet( ServerReachable ) == false )
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
			else if( m_framebufferState == FramebufferInvalid )
			{
				setState( AuthenticationFailed );
			}
			else
			{
				// failed for an unknown reason
				setState( ConnectionFailed );
			}

			// do not sleep when already requested to stop
			if( isControlFlagSet( TerminateThread ) )
			{
				break;
			}

			// wait a bit until next connect
			sleeperMutex.lock();
			if( m_framebufferUpdateInterval > 0 )
			{
				m_updateIntervalSleeper.wait( &sleeperMutex, static_cast<unsigned long>( m_framebufferUpdateInterval ) );
			}
			else
			{
				// default: retry every second
				m_updateIntervalSleeper.wait( &sleeperMutex, ConnectionRetryInterval );
			}
			sleeperMutex.unlock();
		}
	}
}



void VncConnection::handleConnection()
{
	QMutex sleeperMutex;
	QElapsedTimer loopTimer;

	while( state() == Connected &&
		   isControlFlagSet( TerminateThread ) == false &&
		   isControlFlagSet( RestartConnection ) == false )
	{
		loopTimer.start();

		const int i = WaitForMessage( m_client, MessageWaitTimeout );
		if( isControlFlagSet( TerminateThread ) || i < 0 )
		{
			break;
		}
		else if( i )
		{
			// handle all available messages
			bool handledOkay = true;
			do {
				handledOkay &= HandleRFBServerMessage( m_client );
			} while( handledOkay && WaitForMessage( m_client, 0 ) );

			if( handledOkay == false )
			{
				break;
			}
		}

		sendEvents();

		const auto remainingUpdateInterval = m_framebufferUpdateInterval - loopTimer.elapsed();

		if( m_framebufferState == FramebufferValid &&
				remainingUpdateInterval > 0 &&
				isControlFlagSet( TerminateThread ) == false )
		{
			sleeperMutex.lock();
			m_updateIntervalSleeper.wait( &sleeperMutex, static_cast<unsigned long>( remainingUpdateInterval ) );
			sleeperMutex.unlock();
		}
	}

	sendEvents();
}



void VncConnection::closeConnection()
{
	if( m_client )
	{
		rfbClientCleanup( m_client );
		m_client = nullptr;
	}

	setState( Disconnected );
}



void VncConnection::setState( State state )
{
	if( state != m_state )
	{
		m_state = state;

		emit stateChanged();
	}
}



void VncConnection::setControlFlag( VncConnection::ControlFlag flag, bool on )
{
	m_controlFlagMutex.lock();
#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0))
#warning building compat code for Qt < 5.7
	if( on )
	{
		m_controlFlags |= flag;
	}
	else
	{
		m_controlFlags &= ~static_cast<unsigned int>( flag );
	}
#else
	m_controlFlags.setFlag( flag, on );
#endif
	m_controlFlagMutex.unlock();
}



bool VncConnection::isControlFlagSet( VncConnection::ControlFlag flag )
{
	QMutexLocker locker( &m_controlFlagMutex );
	return m_controlFlags.testFlag( flag );
}




bool VncConnection::initFrameBuffer( rfbClient* client )
{
	const auto size = static_cast<uint64_t>( client->width * client->height * ( client->format.bitsPerPixel / 8 ) );

	client->frameBuffer = new uint8_t[size];

	memset( client->frameBuffer, '\0', size );

	// initialize framebuffer image which just wraps the allocated memory and ensures cleanup after last
	// image copy using the framebuffer gets destroyed
	m_imgLock.lockForWrite();
	m_image = QImage( client->frameBuffer, client->width, client->height, QImage::Format_RGB32, framebufferCleanup, client->frameBuffer );
	m_imgLock.unlock();

	// set up pixel format according to QImage
	client->format.bitsPerPixel = 32;
	client->format.redShift = 16;
	client->format.greenShift = 8;
	client->format.blueShift = 0;
	client->format.redMax = 0xff;
	client->format.greenMax = 0xff;
	client->format.blueMax = 0xff;

	client->appData.encodingsString = "zrle ultra copyrect hextile zlib corre rre raw";
	client->appData.useRemoteCursor = false;
	client->appData.compressLevel = 0;
	client->appData.useBGR233 = false;
	client->appData.qualityLevel = 9;
	client->appData.enableJPEG = false;

	switch( quality() )
	{
	case ScreenshotQuality:
		// make sure to use lossless raw encoding
		client->appData.encodingsString = "raw";
		break;
	case RemoteControlQuality:
		client->appData.useRemoteCursor = true;
		break;
	case ThumbnailQuality:
		client->appData.compressLevel = 9;
		client->appData.qualityLevel = 5;
		client->appData.enableJPEG = true;
		break;
	default:
		break;
	}

	m_framebufferState = FramebufferInitialized;

	emit framebufferSizeChanged( client->width, client->height );

	return true;
}



void VncConnection::finishFrameBufferUpdate()
{
	m_framebufferState = FramebufferValid;
	setControlFlag( ScaledScreenNeedsUpdate, true );

	emit framebufferUpdateComplete();

}



void VncConnection::sendEvents()
{
	m_globalMutex.lock();

	while( m_eventQueue.isEmpty() == false )
	{
		auto event = m_eventQueue.dequeue();

		// unlock the queue mutex during the runtime of ClientEvent::fire()
		m_globalMutex.unlock();

		event->fire( m_client );
		delete event;

		// and lock it again
		m_globalMutex.lock();
	}

	m_globalMutex.unlock();
}



void VncConnection::enqueueEvent( MessageEvent* event)
{
	QMutexLocker lock( &m_globalMutex );
	if( m_state != Connected )
	{
		return;
	}

	m_eventQueue.enqueue( event );
}



void VncConnection::mouseEvent( int x, int y, int buttonMask )
{
	enqueueEvent( new PointerClientEvent( x, y, buttonMask ) );
}



void VncConnection::keyEvent( unsigned int key, bool pressed )
{
	enqueueEvent( new KeyClientEvent( key, pressed ) );
}



void VncConnection::clientCut( const QString& text )
{
	enqueueEvent( new ClientCutEvent( text ) );
}



void VncConnection::handleSecTypeVeyon( rfbClient* client )
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

	qDebug() << "VncConnection::handleSecTypeVeyon(): received authentication types:" << authTypes;

	RfbVeyonAuth::Type chosenAuthType = RfbVeyonAuth::Token;
	if( authTypes.count() > 0 )
	{
		chosenAuthType = authTypes.first();

		// look whether the VncConnection recommends a specific
		// authentication type (e.g. VeyonAuthHostBased when running as
		// demo client)
		auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );

		if( connection != nullptr )
		{
			for( auto authType : authTypes )
			{
				if( connection->veyonAuthType() == authType )
				{
					chosenAuthType = authType;
				}
			}
		}
	}

	qDebug() << "VncConnection::handleSecTypeVeyon(): chose authentication type" << chosenAuthType;
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
				qCritical( "VncConnection::handleSecTypeVeyon(): challenge size mismatch!" );
				break;
			}

			// create local copy of private key so we can modify it within our own thread
			auto key = VeyonCore::authenticationCredentials().privateKey();
			if( key.isNull() || key.canSign() == false )
			{
				qCritical( "VncConnection::handleSecTypeVeyon(): invalid private key!" );
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
			qCritical( "VncConnection::handleSecTypeVeyon(): can't encrypt with given public key!" );
			break;
		}

		CryptoCore::SecureArray plainTextPassword( VeyonCore::authenticationCredentials().logonPassword().toUtf8() );
		CryptoCore::SecureArray encryptedPassword = publicKey.encrypt( plainTextPassword, CryptoCore::DefaultEncryptionAlgorithm );
		if( encryptedPassword.isEmpty() )
		{
			qCritical( "VncConnection::handleSecTypeVeyon(): password encryption failed!" );
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



void VncConnection::hookPrepareAuthentication( rfbClient* client )
{
	auto connection = static_cast<VncConnection *>( rfbClientGetClientData( client, nullptr ) );
	if( connection )
	{
		// set our internal flag which indicates that we basically have communication with the client
		// which means that the host is reachable
		connection->setControlFlag( ServerReachable, true );
	}
}



qint64 VncConnection::libvncClientDispatcher( char* buffer, const qint64 bytes,
												   SocketDevice::SocketOperation operation, void* user )
{
	rfbClient* client = static_cast<rfbClient *>( user );
	switch( operation )
	{
	case SocketDevice::SocketOpRead:
		return ReadFromRFBServer( client, buffer, bytes ) ? bytes : 0;

	case SocketDevice::SocketOpWrite:
		return WriteToRFBServer( client, buffer, bytes ) ? bytes : 0;
	}

	return 0;
}



void handleSecTypeVeyon( rfbClient *client )
{
	VncConnection::hookPrepareAuthentication( client );
	VncConnection::handleSecTypeVeyon( client );
}
