/*
 * VeyonVncConnection.h - declaration of VeyonVncConnection class
 *
 * Copyright (c) 2008-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * code partly taken from KRDC / vncclientthread.h:
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

#ifndef VEYON_VNC_CONNECTION_H
#define VEYON_VNC_CONNECTION_H

#include "VeyonCore.h"

#include <QMutex>
#include <QQueue>
#include <QReadWriteLock>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QImage>

#include "RfbVeyonAuth.h"
#include "SocketDevice.h"


class MessageEvent
{
public:
	virtual ~MessageEvent() { }

	virtual void fire( rfbClient *c ) = 0;
} ;


class VEYON_CORE_EXPORT VeyonVncConnection: public QThread
{
	Q_OBJECT
public:
	enum QualityLevels
	{
		ThumbnailQuality,
		ScreenshotQuality,
		RemoteControlQuality,
		DemoServerQuality,
		DemoClientQuality,
		NumQualityLevels
	} ;

	enum States
	{
		Disconnected,
		Connecting,
		HostOffline,
		ServiceUnreachable,
		AuthenticationFailed,
		ConnectionFailed,
		Connected
	} ;
	typedef States State;

	explicit VeyonVncConnection( QObject *parent = nullptr );
	~VeyonVncConnection() override;

	const QImage image( int x = 0, int y = 0, int w = 0, int h = 0 ) const;
	void stop( bool deleteAfterFinished = false );
	void reset( const QString &host );
	void setHost( const QString &host );
	void setPort( int port );

	State state() const
	{
		return m_state;
	}

	bool isConnected() const
	{
		return state() == Connected && isRunning();
	}

	const QString &host() const
	{
		return m_host;
	}

	void setVeyonAuthType( RfbVeyonAuth::Type authType )
	{
		m_veyonAuthType = authType;
	}

	RfbVeyonAuth::Type veyonAuthType() const
	{
		return m_veyonAuthType;
	}

	void setQuality( QualityLevels qualityLevel )
	{
		m_quality = qualityLevel;
	}

	QualityLevels quality() const
	{
		return m_quality;
	}

	void enqueueEvent( MessageEvent *e );

	const rfbClient *getRfbClient() const
	{
		return m_cl;
	}

	rfbClient *getRfbClient()
	{
		return m_cl;
	}

	QSize framebufferSize() const
	{
		return m_image.size();
	}

	/** \brief Returns whether framebuffer data is valid, i.e. at least one full FB update received */
	bool hasValidFrameBuffer() const
	{
		return m_frameBufferValid;
	}

	void setScaledSize( const QSize &s )
	{
		if( m_scaledSize != s )
		{
			m_scaledSize = s;
			m_scaledScreenNeedsUpdate = true;
		}
	}

	QImage scaledScreen()
	{
		rescaleScreen();
		return m_scaledScreen;
	}

	void setFramebufferUpdateInterval( int interval );

	void rescaleScreen();

	// authentication
	static void handleSecTypeVeyon( rfbClient *client );
	static void handleMsLogonIIAuth( rfbClient *client );
	static void hookPrepareAuthentication( rfbClient *cl );

	static qint64 libvncClientDispatcher( char * buffer, const qint64 bytes,
										  SocketDevice::SocketOperation operation, void * user );

	void cursorShapeUpdatedExternal( const QImage &cursorShape, int xh, int yh )
	{
		emit cursorShapeUpdated( cursorShape, xh, yh );
	}


signals:
	void newClient( rfbClient *c );
	void imageUpdated( int x, int y, int w, int h );
	void framebufferUpdateComplete();
	void framebufferSizeChanged( int w, int h );
	void cursorPosChanged( int x, int y );
	void cursorShapeUpdated( const QImage &cursorShape, int xh, int yh );
	void gotCut( const QString &text );
	void passwordRequest();
	void outputErrorMessage( const QString &message );
	void stateChanged();


public slots:
	void mouseEvent( int x, int y, int buttonMask );
	void keyEvent( unsigned int key, bool pressed );
	void clientCut( const QString &text );


protected:
	void run() override;
	void doConnection();


private:
	enum {
		InitialFrameBufferTimeout = 15000,	/**< A server has to send an initial framebuffer within given timeout in ms */
		ThreadTerminationTimeout = 10000
	};

	void setState( State state );

	void finishFrameBufferUpdate();

	// hooks for LibVNCClient
	static int8_t hookInitFrameBuffer( rfbClient *cl );
	static void hookUpdateFB( rfbClient *cl, int x, int y, int w, int h );
	static void hookFinishFrameBufferUpdate( rfbClient *cl );
	static int8_t hookHandleCursorPos( rfbClient *cl, int x, int y );
	static void hookCursorShape( rfbClient *cl, int xh, int yh, int w, int h, int bpp );
	static void hookCutText( rfbClient *cl, const char *text, int textlen );
	static void hookOutputHandler( const char *format, ... );
	static int8_t hookHandleVeyonMessage( rfbClient *cl,
						rfbServerToClientMsg *msg );
	static void framebufferCleanup( void* framebuffer );

	bool m_serviceReachable;
	bool m_frameBufferInitialized;
	bool m_frameBufferValid;
	rfbClient *m_cl;
	RfbVeyonAuth::Type m_veyonAuthType;
	QualityLevels m_quality;
	QString m_host;
	int m_port;
	QTimer m_terminateTimer;
	QWaitCondition m_updateIntervalSleeper;
	int m_framebufferUpdateInterval;
	QMutex m_mutex;
	mutable QReadWriteLock m_imgLock;
	QQueue<MessageEvent *> m_eventQueue;

	QImage m_image;
	bool m_scaledScreenNeedsUpdate;
	QImage m_scaledScreen;
	QSize m_scaledSize;

	volatile State m_state;


} ;

#endif

