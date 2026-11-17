// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFile>
#include <QHostAddress>
#include <QLocalSocket>
#include <QTcpSocket>

#include "VncServerSocket.h"

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <unistd.h>
#endif


VncServerSocket::VncServerSocket(const VncServerEndpoint& endpoint, QObject* parent) :
	QObject(parent),
	m_endpoint(endpoint)
{
	if (m_endpoint.isLocalSocket())
	{
		m_localSocket = new QLocalSocket(this);
		m_device = m_localSocket;
		// connected before any other slot: nothing is read before the peer check
		connect(m_localSocket, &QLocalSocket::connected, this, &VncServerSocket::checkLocalPeer);
		connect(m_localSocket, &QLocalSocket::disconnected, this, &VncServerSocket::disconnected);
	}
	else
	{
		m_tcpSocket = new QTcpSocket(this);
		m_device = m_tcpSocket;
		connect(m_tcpSocket, &QTcpSocket::disconnected, this, &VncServerSocket::disconnected);
	}
}



VncServerSocket::~VncServerSocket()
{
	m_device->disconnect(this);
}



void VncServerSocket::setReadBufferSize(qint64 size)
{
	if (m_localSocket)
	{
		m_localSocket->setReadBufferSize(size);
	}
	else
	{
		m_tcpSocket->setReadBufferSize(size);
	}
}



void VncServerSocket::connectToServer()
{
	if (m_localSocket)
	{
		m_localSocket->connectToServer(m_endpoint.localSocketPath);
	}
	else
	{
		m_tcpSocket->connectToHost(QHostAddress::LocalHost, quint16(m_endpoint.port));
	}
}



void VncServerSocket::abort()
{
	if (m_localSocket)
	{
		m_localSocket->abort();
	}
	else
	{
		m_tcpSocket->abort();
	}
}



QString VncServerSocket::peerName() const
{
	if (m_localSocket)
	{
		return m_endpoint.localSocketPath;
	}

	return QStringLiteral("localhost:%1").arg(m_endpoint.port);
}



qint64 VncServerSocket::parentProcessId()
{
#ifdef Q_OS_LINUX
	return qint64(getppid());
#else
	return -1;
#endif
}



bool VncServerSocket::isExpectedListener(qintptr socketDescriptor, const VncServerEndpoint& endpoint)
{
#ifdef Q_OS_LINUX
	// for a connected Unix socket SO_PEERCRED reports the process that called listen()
	ucred credentials{};
	socklen_t credentialsSize = sizeof(credentials);
	if (socketDescriptor < 0 ||
		getsockopt(int(socketDescriptor), SOL_SOCKET, SO_PEERCRED, &credentials, &credentialsSize) != 0 ||
		credentialsSize != sizeof(credentials))
	{
		return false;
	}

	if (credentials.uid != geteuid() || credentials.pid <= 0)
	{
		return false;
	}

	if (endpoint.expectedProcessId)
	{
		return qint64(credentials.pid) == endpoint.expectedProcessId();
	}

	if (endpoint.expectedParentProcessId > 0)
	{
		QFile status(QStringLiteral("/proc/%1/status").arg(credentials.pid));
		if (status.open(QFile::ReadOnly) == false)
		{
			return false;
		}
		static const QByteArray parentProcessIdField("PPid:");
		const auto lines = status.readAll().split('\n');
		for (const auto& line : lines)
		{
			if (line.startsWith(parentProcessIdField))
			{
				bool ok = false;
				const auto parentProcessId = line.mid(parentProcessIdField.size()).trimmed().toLongLong(&ok);
				return ok && parentProcessId == endpoint.expectedParentProcessId;
			}
		}
	}

	return false;
#else
	Q_UNUSED(socketDescriptor)
	Q_UNUSED(endpoint)
	return false;
#endif
}



void VncServerSocket::checkLocalPeer()
{
	if (isExpectedListener(m_localSocket->socketDescriptor(), m_endpoint) == false)
	{
		vCritical() << "listener of" << m_endpoint.localSocketPath << "is not the expected VNC server - disconnecting";
		m_localSocket->abort();
	}
}
