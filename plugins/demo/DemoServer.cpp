/*
 * DemoServer.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include "rfb/rfbproto.h"

#include <QTcpSocket>

#include "DemoConfiguration.h"
#include "DemoServer.h"
#include "DemoServerConnection.h"
#include "VeyonConfiguration.h"
#include "VncClientProtocol.h"


DemoServer::DemoServer( int vncServerPort, const Password& vncServerPassword, const Password& demoAccessToken,
						const DemoConfiguration& configuration, int demoServerPort, QObject *parent ) :
	QTcpServer( parent ),
	m_configuration( configuration ),
	m_memoryLimit( m_configuration.memoryLimit() * 1024*1024 ),
	m_keyFrameInterval( m_configuration.keyFrameInterval() * 1000 ),
	m_vncServerPort( vncServerPort ),
	m_demoAccessToken( demoAccessToken ),
	m_vncServerSocket( new QTcpSocket( this ) ),
	m_vncClientProtocol( new VncClientProtocol( m_vncServerSocket, vncServerPassword ) ),
	m_framebufferUpdateTimer( this ),
	m_lastFullFramebufferUpdate(),
	m_requestFullFramebufferUpdate( false ),
	m_keyFrame( 0 )
{
	connect( m_vncServerSocket, &QTcpSocket::readyRead, this, &DemoServer::readFromVncServer );
	connect( m_vncServerSocket, &QTcpSocket::disconnected, this, &DemoServer::reconnectToVncServer );

	connect( &m_framebufferUpdateTimer, &QTimer::timeout, this, &DemoServer::requestFramebufferUpdate );

	if( listen( QHostAddress::Any, demoServerPort ) == false )
	{
		vCritical() << "could not listen to demo server port";
		return;
	}

	m_framebufferUpdateTimer.start( m_configuration.framebufferUpdateInterval() );

	reconnectToVncServer();
}



DemoServer::~DemoServer()
{
	delete m_vncClientProtocol;
	delete m_vncServerSocket;
}



void DemoServer::terminate()
{
	m_vncServerSocket->disconnect( this );

	const auto connections = findChildren<DemoServerConnection *>();
	if( connections.isEmpty() )
	{
		deleteLater();
	}
	else
	{
		for( auto connection : connections )
		{
			connection->quit();
		}

		for( auto connection : connections )
		{
			connection->wait( ConnectionThreadWaitTime );
		}

		QTimer::singleShot( TerminateRetryInterval, 0, &DemoServer::terminate );
	}
}



const QByteArray& DemoServer::serverInitMessage() const
{
	return m_vncClientProtocol->serverInitMessage();
}



void DemoServer::lockDataForRead()
{
	QElapsedTimer readLockTimer;
	readLockTimer.restart();

	m_dataLock.lockForRead();

	if( readLockTimer.elapsed() > 100 )
	{
		vDebug() << "locking for read took" << readLockTimer.elapsed() << "ms in thread"
				 << QThread::currentThreadId();
	}
}



void DemoServer::incomingConnection( qintptr socketDescriptor )
{
	vDebug() << socketDescriptor;

	m_pendingConnections.append( socketDescriptor );

	if( m_vncClientProtocol->state() == VncClientProtocol::State::Running )
	{
		acceptPendingConnections();
	}
}



void DemoServer::acceptPendingConnections()
{
	while( m_pendingConnections.isEmpty() == false )
	{
		new DemoServerConnection( this, m_demoAccessToken, m_pendingConnections.takeFirst() );
	}
}



void DemoServer::reconnectToVncServer()
{
	m_vncClientProtocol->start();

	m_vncServerSocket->connectToHost( QHostAddress::LocalHost, static_cast<quint16>( m_vncServerPort ) );
}



void DemoServer::readFromVncServer()
{
	if( m_vncClientProtocol->state() != VncClientProtocol::Running )
	{
		while( m_vncClientProtocol->read() )
		{
		}

		if( m_vncClientProtocol->state() == VncClientProtocol::Running )
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
	if( m_vncClientProtocol->state() != VncClientProtocol::Running )
	{
		return;
	}

	if( m_requestFullFramebufferUpdate ||
		m_lastFullFramebufferUpdate.elapsed() >= m_keyFrameInterval )
	{
		vDebug() << "Requesting full framebuffer update";
		m_vncClientProtocol->requestFramebufferUpdate( false );
		m_lastFullFramebufferUpdate.restart();
		m_requestFullFramebufferUpdate = false;
	}
	else
	{
		m_vncClientProtocol->requestFramebufferUpdate( true );
	}
}



bool DemoServer::receiveVncServerMessage()
{
	if( m_vncClientProtocol->receiveMessage() )
	{
		if( m_vncClientProtocol->lastMessageType() == rfbFramebufferUpdate )
		{
			enqueueFramebufferUpdateMessage( m_vncClientProtocol->lastMessage() );
		}
		else
		{
			vWarning() << "skipping server message of type" << static_cast<int>( m_vncClientProtocol->lastMessageType() );
		}

		return true;
	}

	return false;
}



void DemoServer::enqueueFramebufferUpdateMessage( const QByteArray& message )
{
	QElapsedTimer writeLockTime;
	writeLockTime.start();

	m_dataLock.lockForWrite();

	if( writeLockTime.elapsed() > 10 )
	{
		vDebug() << "locking for write took" << writeLockTime.elapsed() << "ms";
	}

	const auto lastUpdatedRect = m_vncClientProtocol->lastUpdatedRect();

	const bool isFullUpdate = ( lastUpdatedRect.x() == 0 && lastUpdatedRect.y() == 0 &&
								lastUpdatedRect.width() == m_vncClientProtocol->framebufferWidth() &&
								lastUpdatedRect.height() == m_vncClientProtocol->framebufferHeight() );

	const auto queueSize = framebufferUpdateMessageQueueSize();

	if( isFullUpdate || queueSize > m_memoryLimit*2 )
	{
		if( m_keyFrameTimer.elapsed() > 1 )
		{
			const auto memTotal = queueSize / 1024;
			vDebug() << "message count:" << m_framebufferUpdateMessages.size()
					 << "queue size (KB):" << memTotal
					 << "throughput (KB/s):" << ( memTotal * 1000 ) / m_keyFrameTimer.elapsed();
		}
		m_keyFrameTimer.restart();
		++m_keyFrame;

		m_framebufferUpdateMessages.clear();
	}

	m_framebufferUpdateMessages.append( message );

	m_dataLock.unlock();

	// we're about to reach memory limits?
	if( framebufferUpdateMessageQueueSize() > m_memoryLimit )
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
	vDebug();

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

	return m_vncClientProtocol->setPixelFormat( format );
}



bool DemoServer::setVncServerEncodings()
{
	return m_vncClientProtocol->
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
