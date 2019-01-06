/*
 * DemoServer.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QTcpServer>
#include <QTcpSocket>

#include "DemoConfiguration.h"
#include "DemoServer.h"
#include "DemoServerConnection.h"
#include "VeyonConfiguration.h"


DemoServer::DemoServer( int vncServerPort, const QString& vncServerPassword, const QString& demoAccessToken,
						const DemoConfiguration& configuration, QObject *parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	m_vncServerPort( vncServerPort ),
	m_demoAccessToken( demoAccessToken ),
	m_tcpServer( new QTcpServer( this ) ),
	m_vncServerSocket( new QTcpSocket( this ) ),
	m_vncClientProtocol( m_vncServerSocket, vncServerPassword ),
	m_framebufferUpdateTimer( this ),
	m_lastFullFramebufferUpdate(),
	m_requestFullFramebufferUpdate( false ),
	m_keyFrame( 0 )
{
	connect( m_tcpServer, &QTcpServer::newConnection, this, &DemoServer::acceptPendingConnections );

	connect( m_vncServerSocket, &QTcpSocket::readyRead, this, &DemoServer::readFromVncServer );
	connect( m_vncServerSocket, &QTcpSocket::disconnected, this, &DemoServer::reconnectToVncServer );

	connect( &m_framebufferUpdateTimer, &QTimer::timeout, this, &DemoServer::requestFramebufferUpdate );

	if( m_tcpServer->listen( QHostAddress::Any, VeyonCore::config().demoServerPort() ) == false )
	{
		qCritical( "DemoServer: could not listen to demo server port!" );
		return;
	}

	m_framebufferUpdateTimer.start( m_configuration.framebufferUpdateInterval() );

	reconnectToVncServer();
}



DemoServer::~DemoServer()
{
	qDebug() << Q_FUNC_INFO << "disconnecting signals";
	m_vncServerSocket->disconnect( this );
	m_tcpServer->disconnect( this );

	qDebug() << Q_FUNC_INFO << "deleting connections";

	QList<DemoServerConnection *> l;
	while( !( l = findChildren<DemoServerConnection *>() ).isEmpty() )
	{
		delete l.front();
	}

	qDebug() << Q_FUNC_INFO << "deleting server socket";
	delete m_vncServerSocket;

	qDebug() << Q_FUNC_INFO << "deleting TCP server";
	delete m_tcpServer;

	qDebug() << Q_FUNC_INFO << "finished";
}



void DemoServer::acceptPendingConnections()
{
	if( m_vncClientProtocol.state() != VncClientProtocol::Running )
	{
		return;
	}

	while( m_tcpServer->hasPendingConnections() )
	{
		new DemoServerConnection( m_demoAccessToken, m_tcpServer->nextPendingConnection(), this );
	}
}



void DemoServer::reconnectToVncServer()
{
	m_vncClientProtocol.start();

	m_vncServerSocket->connectToHost( QHostAddress::LocalHost, m_vncServerPort );
}



void DemoServer::readFromVncServer()
{
	if( m_vncClientProtocol.state() != VncClientProtocol::Running )
	{
		while( m_vncClientProtocol.read() )
		{
		}

		if( m_vncClientProtocol.state() == VncClientProtocol::Running )
		{
			start();
		}
	}
	else
	{
		while( receiveVncServerMessage() )
		{
		}
	}
}



void DemoServer::requestFramebufferUpdate()
{
	if( m_vncClientProtocol.state() != VncClientProtocol::Running )
	{
		return;
	}

	if( m_requestFullFramebufferUpdate ||
			m_lastFullFramebufferUpdate.elapsed() >= m_configuration.keyFrameInterval() * 1000 )
	{
		m_vncClientProtocol.requestFramebufferUpdate( false );
		m_lastFullFramebufferUpdate.restart();
		m_requestFullFramebufferUpdate = false;
	}
	else
	{
		m_vncClientProtocol.requestFramebufferUpdate( true );
	}
}



bool DemoServer::receiveVncServerMessage()
{
	if( m_vncClientProtocol.receiveMessage() )
	{
		if( m_vncClientProtocol.lastMessageType() == rfbFramebufferUpdate )
		{
			enqueueFramebufferUpdateMessage( m_vncClientProtocol.lastMessage() );
		}
		else
		{
			qWarning( "DemoServer: skipping server message of type %d", (int) m_vncClientProtocol.lastMessageType() );
		}

		return true;
	}

	return false;
}



void DemoServer::enqueueFramebufferUpdateMessage( const QByteArray& message )
{
	bool isFullUpdate = false;
	const auto lastUpdatedRect = m_vncClientProtocol.lastUpdatedRect();

	if( lastUpdatedRect.x() == 0 && lastUpdatedRect.y() == 0 &&
			lastUpdatedRect.width() == m_vncClientProtocol.framebufferWidth() &&
			lastUpdatedRect.height() == m_vncClientProtocol.framebufferHeight() )
	{
		isFullUpdate = true;
	}

	m_dataLock.lockForWrite();

	if( isFullUpdate || framebufferUpdateMessageQueueSize() > m_configuration.memoryLimit()*2*1024*1024  )
	{
		if( m_keyFrameTimer.elapsed() > 1 )
		{
			const auto memTotal = framebufferUpdateMessageQueueSize() / 1024;
			qDebug() << Q_FUNC_INFO
					 << "   MEMTOTAL:" << memTotal
					 << "   KB/s:" << ( memTotal * 1000 ) / m_keyFrameTimer.elapsed();
		}
		m_keyFrameTimer.restart();
		++m_keyFrame;
		m_framebufferUpdateMessages.clear();
	}

	m_framebufferUpdateMessages.append( message );

	m_dataLock.unlock();

	// we're about to reach memory limits?
	if( framebufferUpdateMessageQueueSize() > m_configuration.memoryLimit() * 1024 * 1024 )
	{
		// then request a full update so we can clear our queue
		m_requestFullFramebufferUpdate = true;
	}
}



qint64 DemoServer::framebufferUpdateMessageQueueSize() const
{
	qint64 size = 0;

	for( const auto& message : qAsConst( m_framebufferUpdateMessages ) )
	{
		size += message.size();
	}

	return size;
}



void DemoServer::start()
{
	setVncServerPixelFormat();
	setVncServerEncodings();

	m_requestFullFramebufferUpdate = true;

	requestFramebufferUpdate();

	while( receiveVncServerMessage() )
	{
	}

	acceptPendingConnections();
}



bool DemoServer::setVncServerPixelFormat()
{
	rfbPixelFormat format;

	format.bitsPerPixel = 32;
	format.depth = 32;
	format.bigEndian = qFromBigEndian<uint16_t>( 1 ) == 1 ? true : false;
	format.trueColour = 1;
	format.redShift = 16;
	format.greenShift = 8;
	format.blueShift = 0;
	format.redMax = 0xff;
	format.greenMax = 0xff;
	format.blueMax = 0xff;
	format.pad1 = 0;
	format.pad2 = 0;

	return m_vncClientProtocol.setPixelFormat( format );
}



bool DemoServer::setVncServerEncodings()
{
	return m_vncClientProtocol.
			setEncodings( {
							  rfbEncodingUltraZip,
							  rfbEncodingUltra,
							  rfbEncodingCopyRect,
							  rfbEncodingHextile,
							  rfbEncodingCoRRE,
							  rfbEncodingRRE,
							  rfbEncodingRaw,
							  rfbEncodingCompressLevel9,
							  rfbEncodingQualityLevel7,
							  rfbEncodingNewFBSize,
							  rfbEncodingLastRect
						  } );
}
