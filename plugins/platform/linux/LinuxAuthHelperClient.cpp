// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include "LinuxAuthHelperClient.h"
#include "LinuxDBusInterfaces.h"


void LinuxAuthHelperClient::authenticate(
	const QString& username, Veyon::Crypto::SecureBuffer&& password, QObject* context,
	const ServiceIpcCommandHandler::AuthenticationResultHandler& resultHandler)
{
	// no QDBusInterface: it introspects the helper synchronously
	auto call = QDBusMessage::createMethodCall(AuthHelperInterface::ServiceName,
											   AuthHelperInterface::ObjectPath,
											   AuthHelperInterface::InterfaceName,
											   AuthHelperInterface::AuthenticateMethod);
	// the marshalled copies of the password are not zeroised (D-Bus API)
	call << username << password.toUnprotectedByteArray();
	password.clear();

	const auto pendingCall = QDBusConnection::systemBus().asyncCall(call, AuthenticationLimiter::AuthenticateTimeout);
	auto* watcher = new QDBusPendingCallWatcher(pendingCall, context);
	const auto handleReply = [resultHandler, watcher]() {
		const QDBusPendingReply<bool> reply = *watcher;
		if (reply.isValid() == false)
		{
			vCritical() << "authentication helper call failed:" << reply.error().message();
		}

		watcher->deleteLater();

		// an error or timeout of the helper is no verdict on the password
		using AuthenticationResult = ServiceIpcCommandHandler::AuthenticationResult;
		if (reply.isValid() == false)
		{
			resultHandler(AuthenticationResult::NoVerdict);
		}
		else
		{
			resultHandler(reply.value() ? AuthenticationResult::Authenticated : AuthenticationResult::Failed);
		}
	};

	// a call that failed right away (e.g. no system bus) never emits finished()
	if (watcher->isFinished())
	{
		QMetaObject::invokeMethod(context, handleReply, Qt::QueuedConnection);
	}
	else
	{
		QObject::connect(watcher, &QDBusPendingCallWatcher::finished, context, handleReply);
	}
}
