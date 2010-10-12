/*
 * ItalcVncConnection.h - declaration of ItalcVncConnection class
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _ITALC_VNC_CONNECTION_H
#define _ITALC_VNC_CONNECTION_H

#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QReadWriteLock>
#include <QtCore/QThread>

#include "ItalcCore.h"
#include "FastQImage.h"

extern "C"
{
	#include <rfb/rfbclient.h>
}


class ClientEvent
{
public:
	virtual ~ClientEvent()
	{
	}

	virtual void fire( rfbClient * _c ) = 0;
} ;


class ItalcVncConnection: public QThread
{
	Q_OBJECT
public:
	enum QualityLevels
	{
		QualityLow,
		QualityMedium,
		QualityHigh,
		QualityDemoLow,
		QualityDemoMedium,
		QualityDemoHigh
	} ;

	explicit ItalcVncConnection( QObject * parent = 0 );
	virtual ~ItalcVncConnection();
	const QImage image( int x = 0, int y = 0, int w = 0, int h = 0 );
	void setImage( const QImage & _img );
	void emitUpdated( int x, int y, int w, int h );
	void emitGotCut( const QString & text );
	void stop( void );
	void reset( const QString & _host );
	void setHost( const QString & _host );
	void setPort( int _port );

	inline bool isConnected( void ) const
	{
		return m_connected && isRunning();
	}

	inline void setQuality( QualityLevels _q )
	{
		m_quality = _q;
	}

	inline QualityLevels quality( void ) const
	{
		return m_quality;
	}

	void enqueueEvent( ClientEvent * _e );

	inline rfbClient * getRfbClient( void )
	{
		return m_cl;
	}

	inline QSize framebufferSize( void ) const
	{
		return m_image.size();
	}

	inline void setScaledSize( const QSize & _s )
	{
		if( m_scaledSize != _s )
		{
			m_scaledSize = _s;
			m_scaledScreenNeedsUpdate = true;
		}
	}

	inline FastQImage scaledScreen( void )
	{
		rescaleScreen();
		return m_scaledScreen;
	}

	void rescaleScreen( void );


	uint8_t * frameBuffer;


signals:
	void newClient( rfbClient * _c );
	void imageUpdated( int x, int y, int w, int h );
	void gotCut( const QString & _text );
	void passwordRequest( void );
	void outputErrorMessage( const QString & _message );


public slots:
	void mouseEvent( int x, int y, int buttonMask );
	void keyEvent( int key, bool pressed );
	void clientCut( const QString & text );


protected:
	virtual void run( void );


private:
	// hooks for LibVNCClient
	static rfbBool hookNewClient( rfbClient * _cl );
	static void hookUpdateFB( rfbClient * _cl, int x, int y, int w, int h );
	static void hookCutText( rfbClient *cl, const char *text, int textlen );
	static void hookOutputHandler( const char *format, ... );
	static rfbBool hookHandleItalcMessage( rfbClient * _cl,
						rfbServerToClientMsg * _msg );

	rfbClient * m_cl;
	QualityLevels m_quality;
	QString m_host;
	int m_port;
	QMutex m_mutex;
	QReadWriteLock m_imgLock;
	QQueue<ClientEvent *> m_eventQueue;

	FastQImage m_image;
	bool m_scaledScreenNeedsUpdate;
	FastQImage m_scaledScreen;
	QSize m_scaledSize;

	volatile bool m_connected;
	volatile bool m_stopped;


private slots:
	void checkOutputErrorMessage( void );

} ;

#endif

