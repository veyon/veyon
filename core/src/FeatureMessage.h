// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QVariant>

#include "Feature.h"
#include "MessageContext.h"

class QIODevice;

class VEYON_CORE_EXPORT FeatureMessage
{
public:
	using MessageSize = quint32;
	using FeatureUid = Feature::Uid;
	using CommandType = qint32;
	using Arguments = QVariantMap;

	static constexpr unsigned char RfbMessageType = 41;

	enum class Command {
		Default = 0,
		Invalid = -1,
		Init = -2,
	};

	FeatureMessage() = default;

	explicit FeatureMessage(FeatureUid featureUid, Command command = Command::Default) :
		m_featureUid(featureUid),
		m_command(command),
		m_arguments()
	{
	}

	template<typename FeatureCommandEnum, typename = std::enable_if_t<std::is_enum_v<FeatureCommandEnum>>>
	explicit FeatureMessage(FeatureUid featureUid, FeatureCommandEnum commandEnum) :
		m_featureUid(featureUid),
		m_command(Command(commandEnum)),
		m_arguments()
	{
	}

	explicit FeatureMessage(const FeatureMessage& other) :
		m_featureUid(other.featureUid()),
		m_command(other.command()),
		m_arguments(other.arguments())
	{
	}

	~FeatureMessage() = default;

	FeatureMessage& operator=(const FeatureMessage& other)
	{
		m_featureUid = other.featureUid();
		m_command = other.command();
		m_arguments = other.arguments();

		return *this;
	}

	const FeatureUid& featureUid() const
	{
		return m_featureUid;
	}

	FeatureMessage& setCommand(Command command)
	{
		m_command = command;
		return *this;
	}

	template<typename FeatureCommandEnum, typename = std::enable_if_t<std::is_enum_v<FeatureCommandEnum>>>
	FeatureMessage& setCommand(FeatureCommandEnum command)
	{
		m_command = Command(command);
		return *this;
	}

	template<typename Enum = Command>
	Enum command() const
	{
		return Enum(m_command);
	}

	const Arguments& arguments() const
	{
		return m_arguments;
	}

	template<typename T = int>
	FeatureMessage& addArgument(T index, const QVariant& value)
	{
		m_arguments[QString::number(static_cast<int>(index))] = value;
		return *this;
	}

	template<typename T = int>
	QVariant argument(T index) const
	{
		return m_arguments[QString::number(static_cast<int>(index))];
	}

	template<typename T = int>
	bool hasArgument(T index) const
	{
		return m_arguments.contains(QString::number(static_cast<int>(index)));
	}

	bool sendPlain(QIODevice* ioDevice) const;
	bool sendAsRfbMessage(QIODevice* ioDevice) const;

	bool isReadyForReceive(QIODevice* ioDevice);

	bool receive(QIODevice* ioDevice);

private:
	FeatureUid m_featureUid{};
	Command m_command = Command::Invalid;
	Arguments m_arguments{};
};

VEYON_CORE_EXPORT QDebug operator<<(QDebug stream, const FeatureMessage& message);
