// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QPointer>
#include <QThread>

#include "DirectoryBrokerIpcClient.h"
#include "DirectoryBrokerLimits.h"
#include "IpcClient.h"


DirectoryBrokerIpcClient::DirectoryBrokerIpcClient(QObject* parent) :
	QObject(parent)
{
}



DirectoryBrokerClient::Reply DirectoryBrokerIpcClient::request(const QString& command, const QVariantMap& arguments)
{
	const auto requestId = m_nextRequestId++;

	if (isUsable() == false)
	{
		return DirectoryBrokerMessage::error(requestId, DirectoryBrokerProtocol::Error::Unavailable,
											 QStringLiteral("no IPC connection to the Veyon service"));
	}

	const auto ipcRequest = DirectoryBrokerMessage::toIpcRequest(0, command, arguments);
	const auto commandName = command.toLatin1();
	const QDeadlineTimer deadline(DirectoryBrokerLimits::RequestTimeout);

	for (;;)
	{
		const auto response = VeyonCore::serviceIpcClient()->call(QLatin1String(commandName), ipcRequest.arguments,
																  int(qMax<qint64>(1, deadline.remainingTime())));
		const auto reply = DirectoryBrokerMessage::fromIpcResponse(requestId, command, response);

		// the session's request rate is shared with forwarded requests of masters: retry access
		// control queries within their deadline instead of denying access right away
		if (reply.errorCode() == DirectoryBrokerProtocol::Error::RateLimited && deadline.remainingTime() > RateLimitRetryDelay)
		{
			QThread::msleep(RateLimitRetryDelay);
			continue;
		}

		return reply;
	}
}



void DirectoryBrokerIpcClient::requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler)
{
	const auto requestId = m_nextRequestId++;

	if (isUsable() == false)
	{
		QMetaObject::invokeMethod(this, [requestId, handler]() {
			handler(DirectoryBrokerMessage::error(requestId, DirectoryBrokerProtocol::Error::Unavailable,
												  QStringLiteral("no IPC connection to the Veyon service")));
		}, Qt::QueuedConnection);
		return;
	}

	const auto ipcRequest = DirectoryBrokerMessage::toIpcRequest(0, command, arguments);

	VeyonCore::serviceIpcClient()->callAsync(command, ipcRequest.arguments,
											 [requestId, command, handler](const IpcMessage::Response& response) {
		handler(DirectoryBrokerMessage::fromIpcResponse(requestId, command, response));
	}, DirectoryBrokerLimits::RequestTimeout);
}



bool DirectoryBrokerIpcClient::isUsable() const
{
	const auto ipcClient = VeyonCore::instance() ? VeyonCore::serviceIpcClient() : nullptr;
	if (ipcClient == nullptr || ipcClient->isConnected() == false)
	{
		vWarning() << "no IPC connection to the Veyon service - directory broker unavailable";
		return false;
	}

	if (QThread::currentThread() != ipcClient->thread())
	{
		vCritical() << "directory broker request from a thread other than the IPC client's thread";
		return false;
	}

	return true;
}
