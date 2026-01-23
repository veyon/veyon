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

class DemoAuthentication;
class DemoConfiguration;
class QTcpServer;
class QTcpSocket;
class VncClientProtocol;

class DemoServer : public QTcpServer
{
	Q_OBJECT
public:
	using Password = CryptoCore::PlaintextPassword;
	using MessageList = QVector<QByteArray>;
	static constexpr auto DefaultBandwidthLimit = 100;

	DemoServer( int vncServerPort, const Password& vncServerPassword, const DemoAuthentication& authentication,
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
	bool setVncServerEncodings(int quality);

	static constexpr auto ConnectionThreadWaitTime = 5000;
	static constexpr auto TerminateRetryInterval = 1000;
	static constexpr auto MinimumQuality = 0;
	static constexpr auto DefaultQuality = 6;
	static constexpr auto MaximumQuality = 9;
	static constexpr auto BytesPerKB = 1024;
	static constexpr auto BytesPerMB = BytesPerKB * BytesPerKB;

	const DemoAuthentication& m_authentication;
	const DemoConfiguration& m_configuration;
	const qint64 m_memoryLimit;
	const int m_keyFrameInterval;
	const int m_vncServerPort;

	QList<quintptr> m_pendingConnections;
	QTcpSocket* m_vncServerSocket;
	VncClientProtocol* m_vncClientProtocol;

	QReadWriteLock m_dataLock{};
	QTimer m_framebufferUpdateTimer{this};
	QElapsedTimer m_lastFullFramebufferUpdate{};
	QElapsedTimer m_keyFrameTimer{};
	bool m_requestFullFramebufferUpdate{false};

	int m_keyFrame{0};
	MessageList m_framebufferUpdateMessages{};
	int m_quality = DefaultQuality;
	int m_maxKBytesPerSecond = 0;

} ;
