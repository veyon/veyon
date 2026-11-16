// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT PlatformSessionFunctions
{
	Q_GADGET
public:
	using SessionId = int;
	using SessionUptime = int;
	using EnvironmentVariables = QMap<QString, QString>;

	struct SessionInfo
	{
		SessionId id = InvalidSessionId;
		SessionUptime uptime = 0;
		QString clientAddress;
		QString clientName;
		QString hostName;
		QString metaData;
		bool operator==(const SessionInfo& other) const
		{
			return other.id == id &&
				other.uptime == uptime &&
				other.clientAddress == clientAddress &&
				other.clientName == clientName &&
				other.hostName == hostName &&
				other.metaData == metaData;
		}
		bool operator!=(const SessionInfo& other) const
		{
			return !(other == *this);
		}
	};

	static constexpr SessionId DefaultSessionId = 0;
	static constexpr SessionId InvalidSessionId = -1;
	static constexpr SessionUptime InvalidSessionUptime = -1;

	enum class SessionMetaDataContent {
		None,
		EnvironmentVariable,
		RegistryKey,
	};
	Q_ENUM(SessionMetaDataContent)

	virtual ~PlatformSessionFunctions() = default;

	virtual SessionId currentSessionId() = 0;

	virtual PlatformSessionFunctions::SessionUptime currentSessionUptime() const = 0;
	virtual QString currentSessionClientAddress() const = 0;
	virtual QString currentSessionClientName() const = 0;
	virtual QString currentSessionHostName() const = 0;

	virtual QString currentSessionType() const = 0;
	virtual bool currentSessionHasUser() const = 0;

	virtual EnvironmentVariables currentSessionEnvironmentVariables() const = 0;
	virtual QVariant querySettingsValueInCurrentSession(const QString& key) const = 0;
};
