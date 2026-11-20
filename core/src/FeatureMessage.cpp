// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QJsonDocument>

#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "PluginManager.h"
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



// compact JSON of a one-element array without the brackets: strings quoted, containers expanded
static QString jsonValue(const QVariant& value)
{
	const auto json = QJsonDocument(QJsonArray{QJsonValue::fromVariant(value)}).toJson(QJsonDocument::Compact);
	return QString::fromUtf8(json.mid(1, json.size() - 2));
}



// keys can come from a peer, so they are escaped like values (no line breaks or control characters
// in the log) but without the quotes, and long keys are truncated
static QString escapedKey(const QString& key)
{
	static constexpr auto MaximumKeyLength = 64;

	const auto quoted = jsonValue(key.left(MaximumKeyLength));
	auto escaped = quoted.mid(1, quoted.size() - 2);
	if (key.size() > MaximumKeyLength)
	{
		escaped += QStringLiteral("...");
	}
	return escaped;
}



// formats arguments for log output as "{key=value, ...}" with "<redacted>" instead of the value of
// every key in sensitiveKeys (compared case-insensitively); local variant of Logger::redacted() of 4.12
static QString redacted(const QVariantMap& arguments, const QStringList& sensitiveKeys)
{
	QStringList entries;
	entries.reserve(arguments.size());

	for (auto it = arguments.constBegin(), end = arguments.constEnd(); it != end; ++it)
	{
		if (sensitiveKeys.contains(it.key(), Qt::CaseInsensitive))
		{
			entries.append(escapedKey(it.key()) + QStringLiteral("=<redacted>"));
		}
		else
		{
			entries.append(escapedKey(it.key()) + QLatin1Char('=') + jsonValue(it.value()));
		}
	}

	return QLatin1Char('{') + entries.join(QStringLiteral(", ")) + QLatin1Char('}');
}



// keys of the sensitive arguments of a feature, as declared by the feature plugins
static QStringList sensitiveArguments(Feature::Uid featureUid)
{
	QStringList keys;

	for (auto pluginObject : VeyonCore::pluginManager().pluginObjects())
	{
		const auto featureProviderInterface = qobject_cast<FeatureProviderInterface*>(pluginObject);
		if (featureProviderInterface)
		{
			keys += featureProviderInterface->sensitiveArguments(featureUid);
		}
	}

	return keys;
}



// arguments of a feature message for log output: values of sensitive arguments and all values of
// features unknown here (e.g. from a newer peer) are redacted
static QString redactedArguments(const FeatureMessage& message)
{
	const auto& arguments = message.arguments();

	if (VeyonCore::featureManager().feature(message.featureUid()).isValid() == false)
	{
		return redacted(arguments, arguments.keys());
	}

	return redacted(arguments, sensitiveArguments(message.featureUid()));
}



QDebug operator<<(QDebug stream, const FeatureMessage& message)
{
	// never print the values of sensitive arguments
	stream << QStringLiteral("FeatureMessage(%1,%2,%3)")
				  .arg(VeyonCore::featureManager().feature(message.featureUid()).name())
				  .arg(FeatureMessage::CommandType(message.command()))
				  .arg(redactedArguments(message))
				  .toUtf8()
				  .constData();
	return stream;
}
