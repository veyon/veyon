// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QLocalSocket>

#include "IpcConnection.h"


IpcConnection::IpcConnection(QLocalSocket* socket, IpcChannel::Side side, IpcProtocol::Role role, quint32 sessionId,
							 IpcEndpointCredentials&& credentials, IpcChannel::NonceHistory* nonceHistory,
							 QObject* parent) :
	QObject(parent),
	m_socket(socket),
	m_channel(side, role, sessionId, credentials.endpointName(), credentials.takeToken(), nonceHistory)
{
	m_socket->setParent(this);

	// never buffer more than one maximum frame of unread data from the peer
	m_socket->setReadBufferSize(IpcProtocol::FrameHeaderSize + IpcProtocol::maximumFrameSize(role));

	connect(m_socket, &QLocalSocket::readyRead, this, &IpcConnection::readData);
	connect(m_socket, &QLocalSocket::disconnected, this, &IpcConnection::close);
	connect(&m_timeoutTimer, &QTimer::timeout, this, &IpcConnection::checkTimeout);
}



IpcConnection::~IpcConnection()
{
	m_socket->disconnect(this);
	m_socket->abort();
}



void IpcConnection::start()
{
	if (m_closed)
	{
		return;
	}

	m_timeoutTimer.start(HandshakeTimerInterval);

	processOutputs(m_channel.start());

	if (m_closed == false && m_socket->bytesAvailable() > 0)
	{
		readData();
	}
}



bool IpcConnection::sendMessage(const QByteArray& message)
{
	if (m_closed)
	{
		return false;
	}

	const auto frame = m_channel.encrypt(message);
	if (frame.has_value() == false)
	{
		vWarning() << "can't send message of size" << message.size();
		return false;
	}

	return m_socket->write(*frame) == frame->size();
}



bool IpcConnection::waitForEstablished(int timeout)
{
	QDeadlineTimer deadline(timeout);
	while (m_closed == false && m_channel.isEstablished() == false && deadline.hasExpired() == false)
	{
		m_socket->flush();
		if (m_socket->waitForReadyRead(int(qMin<qint64>(deadline.remainingTime(), HandshakeTimerInterval))) == false)
		{
			checkTimeout();
		}
	}

	m_socket->flush();

	return m_closed == false && m_channel.isEstablished();
}



bool IpcConnection::waitForReadyRead(int timeout)
{
	if (m_closed)
	{
		return false;
	}

	m_socket->flush();

	if (m_socket->bytesAvailable() > 0)
	{
		readData();
		return m_closed == false;
	}

	const auto result = m_socket->waitForReadyRead(timeout);
	checkTimeout();

	return result && m_closed == false;
}



void IpcConnection::close()
{
	if (m_closed)
	{
		return;
	}

	m_closed = true;
	m_timeoutTimer.stop();
	m_socket->abort();

	Q_EMIT closed();
}



void IpcConnection::readData()
{
	char buffer[ReadChunkSize];

	while (m_closed == false && m_socket->bytesAvailable() > 0)
	{
		const auto bytesRead = m_socket->read(buffer, sizeof(buffer));
		if (bytesRead <= 0)
		{
			break;
		}

		processOutputs(m_channel.feed(VeyonCrypto::ByteView(buffer, bytesRead)));
	}
}



void IpcConnection::processOutputs(const IpcChannel::Outputs& outputs)
{
	for (const auto& output : outputs)
	{
		if (m_closed)
		{
			return;
		}

		switch (output.type)
		{
		case IpcChannel::Output::Type::Send:
			m_socket->write(output.data);
			m_socket->flush();
			break;
		case IpcChannel::Output::Type::Message:
			Q_EMIT messageReceived(output.data);
			break;
		case IpcChannel::Output::Type::Established:
			m_timeoutTimer.start(FrameTimerInterval);
			Q_EMIT established();
			break;
		case IpcChannel::Output::Type::Failed:
			vWarning() << "closing IPC connection:" << m_channel.failureReason();
			close();
			break;
		}
	}
}



void IpcConnection::checkTimeout()
{
	processOutputs(m_channel.checkTimeout());
}
