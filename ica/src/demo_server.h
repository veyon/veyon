/*
 * demo_server.h - multi-threaded slim VNC-server for demo-purposes (lot of
 *                 clients accessing server in read-only-mode)
 *           
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QMutex>
#include <QtCore/QPair>
#include <QtCore/QThread>
#include <QtGui/QRegion>
#include <QtNetwork/QTcpServer>

#include "ivs_connection.h"



// there's one instance of a demo-server on the iTALC-master
class demoServer : public QTcpServer
{
	Q_OBJECT
public:
	demoServer( quint16 _port );
	virtual ~demoServer();

/*
private slots:
	void acceptNewConnection( void );
*/

private:
	virtual void incomingConnection( int _sd );

	ivsConnection * m_conn;

	// this thread is just responsible for updating IVS-connection's screen
	class updaterThread : public QThread
	{
	public:
		updaterThread( ivsConnection * _ic );
		~updaterThread();

	private:
		virtual void run( void );
		ivsConnection * m_conn;
		volatile bool m_quit;
	} * m_updaterThread;


	friend class updaterThread;

} ;



class QTime;
// the demo-server creates an instance of this class for each client, i.e.
// each client is connected to a different server-thread for a maximum
// performance
class demoServerClient : public QThread
{
	Q_OBJECT
public:
	demoServerClient( int _sd, const ivsConnection * _conn );
	virtual ~demoServerClient();


private slots:
	// connected to regionUpdated(...)-signal of demo-server's
	// IVS-connection - this way we can record changes in screen, later we
	// extract single, non-overlapping rectangles out of changed region for
	// updating as less as possible of screen
	void updateRegion( const QRegion & _reg );

	// called whenever ivsConnection::cursorShapeChanged() is emitted
	void updateCursorShape( void );

	// connected to cursorMoved(...)-signal of demo-server's IVS-connection
	void moveCursor( void );

	// connected to readyRead()-signal of our client-socket and called as
	// soon as the clients sends something (e.g. an update-request)
	void processClient( void );


private:
	// thread-entry-point - does some initializations and then enters
	// event-loop of thread
	virtual void run( void );

	QRegion m_changedRegion;
	bool m_cursorShapeChanged;
	QMutex m_dataMutex;

	QPoint m_lastCursorPos;

	int m_socketDescriptor;
	QTcpSocket * m_sock;
	const ivsConnection * m_conn;

	Q_UINT8 * m_lzoWorkMem;

	QTime * m_bandwidthTimer;
	Q_UINT32 m_bytesOut;
	Q_UINT32 m_frames;

} ;


#endif

