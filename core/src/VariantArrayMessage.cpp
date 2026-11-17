// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VariantArrayMessage.h"
#include "VariantStream.h"
#include "VeyonCore.h"


VariantArrayMessage::VariantArrayMessage(QIODevice* ioDevice, MessageSize maximumMessageSize) :
	m_buffer(),
	m_stream(&m_buffer),
	m_ioDevice(ioDevice),
	m_maximumMessageSize(maximumMessageSize < MaxMessageSize ? maximumMessageSize : MaxMessageSize)
{
	Q_ASSERT(m_ioDevice != nullptr);

	m_buffer.open(QBuffer::ReadWrite); // Flawfinder: ignore
}



bool VariantArrayMessage::send()
{
	if (m_buffer.size() > m_maximumMessageSize)
	{
		vCritical() << "refusing to send oversized variant message" << m_buffer.size();
		return false;
	}

	const auto messageSize = qToBigEndian<MessageSize>(static_cast<MessageSize>(m_buffer.size()));
	return m_ioDevice->write(reinterpret_cast<const char*>(&messageSize), sizeof(messageSize)) == sizeof(messageSize) &&
		m_ioDevice->write(m_buffer.data()) == m_buffer.size();
}



bool VariantArrayMessage::isReadyForReceive()
{
	MessageSize messageSize;

	if (m_ioDevice->peek(reinterpret_cast<char*>(&messageSize), sizeof(messageSize)) == sizeof(messageSize))
	{
		messageSize = qFromBigEndian(messageSize);
		if (messageSize > m_maximumMessageSize)
		{
			vWarning() << "invalid variant message size" << messageSize;
			m_ioDevice->close();
			return false;
		}

		return m_ioDevice->bytesAvailable() >= qint64(sizeof(messageSize)) + messageSize;
	}

	return false;
}



bool VariantArrayMessage::receive()
{
	MessageSize messageSize;

	if (m_ioDevice->read(reinterpret_cast<char*>(&messageSize), sizeof(messageSize)) != sizeof(messageSize)) // Flawfinder: ignore
	{
		vDebug() << "could not read message size!";
		return false;
	}

	messageSize = qFromBigEndian(messageSize);
	if (messageSize > m_maximumMessageSize)
	{
		vDebug() << "invalid message size" << messageSize;
		m_ioDevice->close();
		return false;
	}

	const auto data = m_ioDevice->read(messageSize); // Flawfinder: ignore
	if (data.size() != static_cast<qint64>(messageSize))
	{
		vDebug() << "could not read message data!";
		return false;
	}

	m_buffer.close();
	m_buffer.setData(data);
	m_buffer.open(QBuffer::ReadOnly); // Flawfinder: ignore

	return true;
}



QVariant VariantArrayMessage::read() // Flawfinder: ignore
{
	return m_stream.read(); // Flawfinder: ignore
}



VariantArrayMessage& VariantArrayMessage::write(const QVariant& v)
{
	m_stream.write(v);

	return *this;
}
