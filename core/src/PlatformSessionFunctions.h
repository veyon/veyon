/*
 * PlatformSessionsFunctions.h - interface class for platform session functions
 *
 * Copyright (c) 2020-2024 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformSessionFunctions
{
public:
	using SessionId = int;
	using SessionUptime = int;
	using EnvironmentVariables = QMap<QString, QString>;

	struct SessionInfo {
		SessionId id = InvalidSessionId;
		SessionUptime uptime;
		QString clientAddress;
		QString clientName;
		QString hostName;
		bool operator==(const SessionInfo& other) const
		{
			return other.id == id &&
					other.uptime == uptime &&
					other.clientAddress == clientAddress &&
					other.clientName == clientName &&
					other.hostName == hostName;
		}
		bool operator!=(const SessionInfo& other) const
		{
			return !(other == *this);
		}
	};

	static constexpr SessionId DefaultSessionId = 0;
	static constexpr SessionId InvalidSessionId = -1;
	static constexpr SessionUptime InvalidSessionUptime = -1;

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
