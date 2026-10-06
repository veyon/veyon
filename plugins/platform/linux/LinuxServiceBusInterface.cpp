/*
 * LinuxServiceBusInterface.cpp - D-Bus interface for Veyon service
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

#include "LinuxServiceBusInterface.h"

#include "LinuxDBusInterfaces.h"
#include "VeyonCore.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFileInfo>
#include <QStringList>
#include <QVariant>

#include <utility>


LinuxServiceBusInterface::LinuxServiceBusInterface(SessionUserCheck sessionUserCheck, QObject* parent):
	QObject(parent),
	m_sessionUserCheck(std::move(sessionUserCheck))
{
}



bool LinuxServiceBusInterface::registerOnSystemBus()
{
	auto bus = QDBusConnection::systemBus();

	if(bus.isConnected() == false)
	{
		vCritical() << "system bus not connected";
		return false;
	}

	if(bus.registerObject(VeyonServiceInterface::ObjectPath, this, QDBusConnection::ExportAllSlots) == false)
	{
		vCritical() << "failed to register D-Bus object:" << bus.lastError().message();
		return false;
	}

	if(bus.registerService(VeyonServiceInterface::ServiceName) == false)
	{
		vCritical() << "failed to register D-Bus service:" << bus.lastError().message();
		return false;
	}

	return true;
}



bool LinuxServiceBusInterface::Authenticate(const QString& username, const QByteArray& password)
{
	if(verifyCaller() == false)
	{
		return false;
	}

	// Handle the request asynchronously so a slow PAM backend (e.g. LDAP or
	// SSSD) does not block the session management event loop of the service.
	const auto callMessage = message();
	setDelayedReply(true);

	QDBusInterface authHelper(AuthHelperInterface::ServiceName,
							  AuthHelperInterface::ObjectPath,
							  AuthHelperInterface::InterfaceName,
							  QDBusConnection::systemBus());
	authHelper.setTimeout(AuthenticateTimeout);

	const auto pendingCall = authHelper.asyncCall(AuthHelperInterface::AuthenticateMethod,
												   username, password);
	auto* watcher = new QDBusPendingCallWatcher(pendingCall, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this,
			 [callMessage, watcher](QDBusPendingCallWatcher*) {
		const QDBusPendingReply<bool> reply = *watcher;

		bool authenticated = false;
		if(reply.isValid())
		{
			authenticated = reply.value();
		}
		else
		{
			vCritical() << "authentication helper call failed:" << reply.error().message();
		}

		if(QDBusConnection::systemBus().send(callMessage.createReply(QVariant::fromValue(authenticated))) == false)
		{
			vCritical() << "failed to send authentication reply";
		}

		watcher->deleteLater();
	});

	return false;
}



bool LinuxServiceBusInterface::verifyCaller() const
{
	const auto sender = message().service();
	if(sender.isEmpty())
	{
		vCritical() << "could not determine D-Bus sender";
		return false;
	}

	const auto* busInterface = QDBusConnection::systemBus().interface();
	const auto uidReply = busInterface->serviceUid(sender);
	const auto pidReply = busInterface->servicePid(sender);
	if(uidReply.isValid() == false || pidReply.isValid() == false)
	{
		vCritical() << "could not resolve D-Bus sender credentials";
		return false;
	}

	const auto uid = uidReply.value();
	if(uid != 0 && m_sessionUserCheck(uid) == false)
	{
		vCritical() << "rejecting authentication request from uid" << uid
					<< "- not an active Veyon session user";
		return false;
	}

	const auto executable = QFileInfo(QStringLiteral("/proc/%1/exe").arg(pidReply.value())).canonicalFilePath();
	if(executable.isEmpty())
	{
		vCritical() << "could not resolve caller executable";
		return false;
	}

	// On systems which wrap Qt binaries (e.g. NixOS) the actual executable is
	// named ".<name>-wrapped", so normalize the name before comparing.
	auto executableName = QFileInfo(executable).fileName();
	if(executableName.startsWith(QLatin1Char('.')))
	{
		executableName.remove(0, 1);
	}
	static const auto wrappedSuffix = QLatin1String("-wrapped");
	if(executableName.endsWith(wrappedSuffix))
	{
		executableName.chop(int(wrappedSuffix.size()));
	}

	static const QStringList allowedExecutables = {
		QStringLiteral("veyon-server") + VeyonCore::executableSuffix(),
		QStringLiteral("veyon-master") + VeyonCore::executableSuffix(),
		QStringLiteral("veyon-configurator") + VeyonCore::executableSuffix(),
	};

	const auto binaryDirectory = QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath();
	if(QFileInfo(executable).absolutePath() != binaryDirectory ||
		allowedExecutables.contains(executableName) == false)
	{
		vCritical() << "rejecting authentication request from untrusted executable" << executable;
		return false;
	}

	return true;
}
