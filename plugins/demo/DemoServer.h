// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTimer>

#include "CryptoCore.h"
#include "DemoServerConnection.h"
#include "VncServerSocket.h"

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

	DemoServer(const VncServerEndpoint& vncServerEndpoint, const Password& vncServerPassword, const Password& demoAccessToken,
			   const DemoConfiguration& configuration, int demoServerPort, QObject* parent);
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
	void incomingConnection(qintptr socketDescriptor) override;
	void acceptPendingConnections();
	void reconnectToVncServer();
	void readFromVncServer();
	void requestFramebufferUpdate();

	bool receiveVncServerMessage();
	void enqueueFramebufferUpdateMessage(const QByteArray& message);

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
	const Password m_demoAccessToken;

	QList<quintptr> m_pendingConnectionSockets;
	QList<QPointer<DemoServerConnection>> m_connections;

	VncServerSocket* m_vncServerSocket;
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
	qint64 m_bytesSinceLastQualityAdjust = 0;
	int m_maxKBytesPerSecond = 0;
};
