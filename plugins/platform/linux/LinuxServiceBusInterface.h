/*
 * LinuxServiceBusInterface.h - D-Bus interface for Veyon service
 *
 * Copyright (c) 2010-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QDBusContext>
#include <QObject>

#include <functional>

#include <sys/types.h>

// clazy:excludeall=copyable-polymorphic

class LinuxServiceBusInterface : public QObject, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "io.veyon.VeyonService")
public:
	using SessionUserCheck = std::function<bool(uid_t)>;

	explicit LinuxServiceBusInterface(SessionUserCheck sessionUserCheck, QObject* parent = nullptr);

	bool registerOnSystemBus();

public Q_SLOTS:
	bool Authenticate(const QString& username, const QByteArray& password);

private:
	bool verifyCaller() const;

	static constexpr auto AuthenticateTimeout = 10000;

	SessionUserCheck m_sessionUserCheck;

};
