/*
 * DemoServer.h - multi-threaded slim VNC-server for demo-purposes (optimized
 *                for lot of clients accessing server in read-only-mode)
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

#pragma once

#include <QElapsedTimer>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTimer>

#include "CryptoCore.h"

class DemoConfiguration;
class QTcpServer;
class QTcpSocket;
class VncClientProtocol;

class DemoServer : public QTcpServer
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;
	using MessageList = QVector<QByteArray>;

	DemoServer( int vncServerPort, const Password& vncServerPassword, const Password& demoAccessToken,
				const DemoConfiguration& configuration, int demoServerPort, QObject *parent );
	~DemoServer() override;

	void terminate();

	const DemoConfiguration& configuration() const
	{
		return m_configuration;
	}

	const QByteArray& serverInitMessage() const;

	void lockDataForRead();

	void unlockData()
	{
		m_dataLock.unlock();
	}

	int keyFrame() const
	{
		return m_keyFrame;
	}

	const MessageList& framebufferUpdateMessages() const
	{
		return m_framebufferUpdateMessages;
	}

private:
	void incomingConnection( qintptr socketDescriptor ) override;
	void acceptPendingConnections();
	void reconnectToVncServer();
	void readFromVncServer();
	void requestFramebufferUpdate();

	bool receiveVncServerMessage();
	void enqueueFramebufferUpdateMessage( const QByteArray& message );

	qint64 framebufferUpdateMessageQueueSize() const;

	void start();
	bool setVncServerPixelFormat();
	bool setVncServerEncodings();

	static constexpr auto ConnectionThreadWaitTime = 5000;
	static constexpr auto TerminateRetryInterval = 1000;

	const DemoConfiguration& m_configuration;
	const qint64 m_memoryLimit;
	const int m_keyFrameInterval;
	const int m_vncServerPort;
	const Password m_demoAccessToken;

	QList<quintptr> m_pendingConnections;
	QTcpSocket* m_vncServerSocket;
	VncClientProtocol* m_vncClientProtocol;

	QReadWriteLock m_dataLock;
	QTimer m_framebufferUpdateTimer;
	QElapsedTimer m_lastFullFramebufferUpdate;
	QElapsedTimer m_keyFrameTimer;
	bool m_requestFullFramebufferUpdate;

	int m_keyFrame;
	MessageList m_framebufferUpdateMessages;

} ;
