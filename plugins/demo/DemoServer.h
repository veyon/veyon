/*
 * DemoServer.h - multi-threaded slim VNC-server for demo-purposes (optimized
 *                for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
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
#include "DemoServerConnection.h"

class DemoConfiguration;
class QTcpServer;
class QTcpSocket;
class VncClientProtocol;

class DemoServer : public QTcpServer
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;
	using MessageList = QList<QPair<qint64, QByteArray>>;
	static constexpr auto DefaultBandwidthLimit = 100;

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

	const MessageList& framebufferUpdateMessages() const
	{
		return m_framebufferUpdateMessages;
	}

	qsizetype fbuSequenceNumber() const
	{
		return m_fbuSequenceNumber;
	}

	int epochId() const
	{
		return m_epochId;
	}

private:
	void incomingConnection( qintptr socketDescriptor ) override;
	void acceptPendingConnections();
	void reconnectToVncServer();
	void readFromVncServer();
	void requestFramebufferUpdate();

	bool receiveVncServerMessage();
	void enqueueFramebufferUpdateMessage( const QByteArray& message );

	void discardUnusedFramebufferUpdateMessages();
	qint64 framebufferUpdateMessageQueueSize() const;

	void start();
	bool setVncServerPixelFormat();
	bool setVncServerEncodings(int quality);

	static constexpr auto ConnectionThreadWaitTime = 5000;
	static constexpr auto TerminateRetryInterval = 1000;
	static constexpr auto QualityAdjustInterval = 15000;
	static constexpr auto FramebufferUpdateMessageMinAge = 10000;
	static constexpr auto FramebufferUpdateMessageMaxAge = 20000;
	static constexpr auto MinimumQuality = 0;
	static constexpr auto DefaultQuality = 6;
	static constexpr auto MaximumQuality = 9;
	static constexpr auto BytesPerKB = 1024;
	static constexpr auto BytesPerMB = BytesPerKB * BytesPerKB;

	const DemoConfiguration& m_configuration;
	const qint64 m_memoryLimit;
	const int m_keyFrameInterval;
	const int m_vncServerPort;
	const Password m_demoAccessToken;

	QList<quintptr> m_pendingConnectionSockets;
	QList<QPointer<DemoServerConnection>> m_connections;

	QTcpSocket* m_vncServerSocket;
	VncClientProtocol* m_vncClientProtocol;

	QReadWriteLock m_dataLock;
	QTimer m_framebufferUpdateTimer;
	QElapsedTimer m_lastFullFramebufferUpdate;
	QElapsedTimer m_qualityAdjustTimer;
	bool m_requestFullFramebufferUpdate;

	MessageList m_framebufferUpdateMessages;
	QAtomicInteger<qsizetype> m_fbuSequenceNumber = 0;
	QAtomicInt m_epochId = 0;

	int m_quality = DefaultQuality;
	int m_bytesSinceLastQualityAdjust = 0;
	int m_maxKBytesPerSecond = 0;

} ;
