// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHostAddress>
#include <QVector>

#include "NetworkObject.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT Computer
{
	Q_GADGET
public:
	enum class NameSource {
		Default,
		HostAddress,
		SessionClientAddress,
		SessionClientName,
		SessionHostName,
		SessionMetaData,
		UserFullName,
		UserLoginName,
	};
	Q_ENUM(NameSource)

	explicit Computer(NetworkObject::Uid networkObjectUid = NetworkObject::Uid(),
					  const QString& displayName = {},
					  const QString& hostAddress = {},
					  const QString& macAddress = {},
					  const QString& location = {});

	bool operator==(const Computer& other) const
	{
		return networkObjectUid() == other.networkObjectUid();
	}

	bool operator!=(const Computer& other) const
	{
		return !(*this == other);
	}

	NetworkObject::Uid networkObjectUid() const
	{
		return m_networkObjectUid;
	}

	void setDisplayName(const QString& displayName)
	{
		m_displayName = displayName;
	}

	const QString& displayName() const
	{
		return m_displayName;
	}

	void setHostAddress(const QString& hostAddress)
	{
		m_hostName = hostAddress;
		m_hostAddress = QHostAddress{hostAddress};
	}

	const QHostAddress& hostAddress() const
	{
		return m_hostAddress;
	}

	const QString& hostName() const
	{
		return m_hostName;
	}

	void setMacAddress(const QString& macAddress)
	{
		m_macAddress = macAddress;
	}

	QString macAddress() const
	{
		return m_macAddress;
	}

	void setLocation(const QString& location)
	{
		m_location = location;
	}

	const QString& location() const
	{
		return m_location;
	}

private:
	NetworkObject::Uid m_networkObjectUid;
	QString m_displayName;
	QString m_hostName;
	QHostAddress m_hostAddress;
	QString m_macAddress;
	QString m_location;
};

using ComputerList = QVector<Computer>;
