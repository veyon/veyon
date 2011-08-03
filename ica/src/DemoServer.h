/*
 * DemoServer.h - multi-threaded slim VNC-server for demo-purposes (optimized
 *                for lot of clients accessing server in read-only-mode)
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

#ifndef _DEMO_SERVER_H
#define _DEMO_SERVER_H

#include <QtCore/QPair>
#include <QtCore/QReadWriteLock>
#include <QtNetwork/QTcpServer>

#include "ItalcVncConnection.h"


// there's one instance of a DemoServer on the iTALC master
class DemoServer : public QTcpServer
{
	Q_OBJECT
public:
	DemoServer( int srcPort, int dstPort, QObject *parent );
	virtual ~DemoServer();

	QPoint cursorPos()
	{
		m_cursorLock.lockForRead();
		QPoint p = m_cursorPos;
		m_cursorLock.unlock();
		return p;
	}

	QImage initialCursorShape()
	{
		m_cursorLock.lockForRead();
		QImage i = m_initialCursorShape;
		m_cursorLock.unlock();
		return i;
	}


private slots:
	// checks whether cursor was moved and sets according flags and
	// variables used by moveCursor() - connection has to be done in
	// GUI-thread as we're calling QCursor::pos() which at least under X11
	// must not be called from another thread than the GUI-thread
	void checkForCursorMovement();
	void updateInitialCursorShape( const QImage &img, int x, int y );


private:
	virtual void incomingConnection( int sock );

	ItalcVncConnection m_vncConn;
	QReadWriteLock m_cursorLock;
	QImage m_initialCursorShape;
	QPoint m_cursorPos;

} ;



// the demo-server creates an instance of this class for each client, i.e.
// each client is connected to a different server-thread for a maximum
// performance
class DemoServerClient : public QThread
{
	Q_OBJECT
public:
	DemoServerClient( int sock, const ItalcVncConnection *vncConn,
							DemoServer *parent );
	virtual ~DemoServerClient();


private slots:
	// connected to imageUpdated(...)-signal of demo-server's
	// VNC connection - this way we can record changes in screen, later we
	// extract single, non-overlapping rectangles out of changed region for
	// updating as less as possible of screen
	void updateRect( int x, int y, int w, int h );

	// called whenever ItalcVncConnection::cursorShapeUpdated() is emitted
	void updateCursorShape( const QImage &cursorShape, int xh, int yh );

	// called regularly for sending pointer-movement-events detected by
	// checkForCursorMovement() to clients - connection has to be done
	// in DemoServerClient-thread-context as we're writing to socket
	void moveCursor();

	// connected to readyRead()-signal of our client-socket and called as
	// soon as the clients sends something (e.g. an update-request)
	void processClient();

	// actually sends framebuffer update - if there's nothing to send but
	// an update response pending, it will start a singleshot timer
	void sendUpdates();

private:
	// thread-entry-point - does some initializations and then enters
	// event-loop of thread
	virtual void run();

	DemoServer * m_demoServer;
	QMutex m_dataMutex;
	bool m_updatesPending;
	QList<QRect> m_changedRects;
	QImage m_cursorShape;
	int m_cursorHotX;
	int m_cursorHotY;
	QPoint m_lastCursorPos;
	volatile bool m_cursorShapeChanged;

	int m_socketDescriptor;
	QTcpSocket *m_sock;
	const ItalcVncConnection *m_vncConn;
	bool m_otherEndianess;
	char *m_lzoWorkMem;
	QRgb *m_rawBuf;

} ;


#endif

