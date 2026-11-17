// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "VeyonCore.h"

#include <functional>

class QIODevice;
class QLocalSocket;
class QTcpSocket;

// Where veyon-server (and the demo server worker) find the local VNC server: a loopback TCP port
// (VNC server plugins in general) or a Unix domain socket (veyon-capture-helper, 02-ipc.md §1a.5)
struct VncServerEndpoint
{
	int port = -1;
	// used instead of the TCP port if not empty
	QString localSocketPath{};
	// local socket only: the listening process must run with the own uid and be the process
	// returned by expectedProcessId() (veyon-server: its capture helper) or a child of
	// expectedParentProcessId (demo server worker: of the server); otherwise it is rejected
	std::function<qint64()> expectedProcessId{};
	qint64 expectedParentProcessId = -1;

	bool isLocalSocket() const
	{
		return localSocketPath.isEmpty() == false;
	}
};



// Client connection to the local VNC server of a VncServerEndpoint. A local socket connection is
// aborted (before any byte is read) unless SO_PEERCRED of the listener matches the endpoint.
class VEYON_CORE_EXPORT VncServerSocket : public QObject
{
	Q_OBJECT
public:
	explicit VncServerSocket(const VncServerEndpoint& endpoint, QObject* parent = nullptr);
	~VncServerSocket() override;

	const VncServerEndpoint& endpoint() const
	{
		return m_endpoint;
	}

	// TCP or local socket, for reading/writing and the readyRead()/bytesWritten() signals
	QIODevice* device() const
	{
		return m_device;
	}

	void setReadBufferSize(qint64 size);
	void connectToServer();
	void abort();

	// for log messages
	QString peerName() const;

	// Linux: parent of the current process (-1 elsewhere)
	static qint64 parentProcessId();

	// Linux: checks the credentials of the listener behind a connected local socket descriptor
	[[nodiscard]] static bool isExpectedListener(qintptr socketDescriptor, const VncServerEndpoint& endpoint);

Q_SIGNALS:
	void disconnected();

private:
	void checkLocalPeer();

	const VncServerEndpoint m_endpoint;
	QTcpSocket* m_tcpSocket = nullptr;
	QLocalSocket* m_localSocket = nullptr;
	QIODevice* m_device = nullptr;
};
