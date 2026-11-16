// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
