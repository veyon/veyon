// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>

#include "DirectoryBrokerMessage.h"
#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "VariantArrayMessage.h"


bool FeatureMessage::sendPlain(QIODevice* ioDevice) const
{
	if (ioDevice)
	{
		VariantArrayMessage message(ioDevice);

		message.write(m_featureUid);
		message.write(CommandType(m_command));
		message.write(m_arguments);

		return message.send();
	}

	vCritical() << "no IO device!";

	return false;
}



bool FeatureMessage::sendAsRfbMessage(QIODevice* ioDevice) const
{
	if (ioDevice)
	{
		const char rfbMessageType = FeatureMessage::RfbMessageType;
		ioDevice->write(&rfbMessageType, sizeof(rfbMessageType));

		return sendPlain(ioDevice);
	}

	vCritical() << "no IO device!";

	return false;
}



QByteArray FeatureMessage::toByteArray() const
{
	QBuffer buffer;
	if (buffer.open(QBuffer::WriteOnly) == false || sendPlain(&buffer) == false)
	{
		return {};
	}

	return buffer.data();
}



bool FeatureMessage::fromByteArray(const QByteArray& data)
{
	auto dataCopy = data;
	QBuffer buffer(&dataCopy);
	return buffer.open(QBuffer::ReadOnly) &&
		isReadyForReceive(&buffer) &&
		receive(&buffer) &&
		buffer.atEnd();
}



bool FeatureMessage::isReadyForReceive(QIODevice* ioDevice)
{
	return ioDevice != nullptr &&
		VariantArrayMessage(ioDevice).isReadyForReceive();
}



bool FeatureMessage::receive(QIODevice* ioDevice)
{
	if (ioDevice != nullptr)
	{
		VariantArrayMessage message(ioDevice);

		if (message.receive())
		{
			m_featureUid = message.read().toUuid();					  // Flawfinder: ignore
			m_command = Command(message.read().value<CommandType>()); // Flawfinder: ignore
			m_arguments = message.read().toMap();					  // Flawfinder: ignore
			return true;
		}

		vWarning() << "could not receive message!";
	}
	else
	{
		vCritical() << "no IO device!";
	}

	return false;
}



QDebug operator<<(QDebug stream, const FeatureMessage& message)
{
	// never print the values of sensitive arguments (05-tls.md §7a); directory broker
	// messages contain user names and group memberships
	const auto& featureManager = VeyonCore::featureManager();
	const auto arguments = message.featureUid() == DirectoryBrokerMessage::featureUid() ? QStringLiteral("<redacted>") : featureManager.redactedArguments(message.featureUid(), message.arguments());

	stream << QStringLiteral("FeatureMessage(%1,%2,%3)")
				  .arg(featureManager.feature(message.featureUid()).name())
				  .arg(FeatureMessage::CommandType(message.command()))
				  .arg(arguments)
				  .toUtf8()
				  .constData();
	return stream;
}
