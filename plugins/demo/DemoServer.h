/*
 * DemoServer.h - multi-threaded slim VNC-server for demo-purposes (optimized
 *                for lot of clients accessing server in read-only-mode)
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef DEMO_SERVER_H
#define DEMO_SERVER_H

#include <QElapsedTimer>
#include <QTimer>

#include "VncClientProtocol.h"

class QTcpServer;

class DemoServer : public QObject
{
	Q_OBJECT
public:
	typedef QVector<QByteArray> MessageList;

	enum {
		FramebufferUpdateInterval = 100,
		FullFramebufferUpdateInterval = FramebufferUpdateInterval*100,
		UpdateQueueMemorySoftLimit = 1024*1024*128,
		UpdateQueueMemoryHardLimit = UpdateQueueMemorySoftLimit * 2
	};

	DemoServer( const QString& vncServerPassword, const QString& demoAccessToken, QObject *parent );
	~DemoServer() override;

	const QByteArray& serverInitMessage() const
	{
		return m_vncClientProtocol.serverInitMessage();
	}

	int keyFrame() const
	{
		return m_keyFrame;
	}

	const MessageList& framebufferUpdateMessages() const
	{
		return m_framebufferUpdateMessages;
	}

private slots:
	void acceptPendingConnections();
	void reconnectToVncServer();
	void readFromVncServer();
	void requestFramebufferUpdate();

private:
	bool receiveVncServerMessage();
	void enqueueFramebufferUpdateMessage( const QByteArray& message );

	qint64 framebufferUpdateMessageQueueSize() const;

	void start();
	bool setVncServerPixelFormat();
	bool setVncServerEncodings();

	const QString m_demoAccessToken;

	QTcpServer* m_tcpServer;
	QTcpSocket* m_vncServerSocket;
	VncClientProtocol m_vncClientProtocol;

	QTimer m_framebufferUpdateTimer;
	QElapsedTimer m_lastFullFramebufferUpdate;
	QElapsedTimer m_keyFrameTimer;
	bool m_requestFullFramebufferUpdate;

	int m_keyFrame;
	MessageList m_framebufferUpdateMessages;

} ;

#endif
