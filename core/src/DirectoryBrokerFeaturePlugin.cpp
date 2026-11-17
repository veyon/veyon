// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPointer>

#include "DirectoryBrokerFeaturePlugin.h"
#include "FeatureMessage.h"
#include "IpcChannel.h"
#include "VeyonServerInterface.h"
#include "VncServerClient.h"


using namespace DirectoryBrokerProtocol;


DirectoryBrokerFeaturePlugin::DirectoryBrokerFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_feature(QLatin1String(staticMetaObject.className()),
			  Feature::Flag::Meta | Feature::Flag::Builtin,
			  DirectoryBrokerMessage::featureUid(), {}, {}, {}, {}, {}),
	m_features({m_feature}),
	m_clock(&IpcChannel::steadyNow)
{
}



bool DirectoryBrokerFeaturePlugin::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
														const FeatureMessage& message)
{
	Q_UNUSED(computerControlInterface)

	// replies are received by DirectoryBrokerConnectionClient in the connection's thread
	return message.featureUid() == m_feature.uid();
}



bool DirectoryBrokerFeaturePlugin::handleFeatureMessage(VeyonServerInterface& server,
														const MessageContext& messageContext,
														const FeatureMessage& message)
{
	if (message.featureUid() != m_feature.uid())
	{
		return false;
	}

	const auto request = DirectoryBrokerMessage::requestFromFeatureMessage(message);
	const auto requestId = request.has_value() ? request->requestId : 0;
	const auto client = server.vncServerClient(messageContext);

	if (isAuthorized(client) == false)
	{
		vWarning() << "denying directory broker request of unauthorized connection";
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::Denied));
		return true;
	}

	if (request.has_value() == false)
	{
		vWarning() << "malformed directory broker request from" << client->hostAddress();
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::BadRequest));
		return true;
	}

	auto& connection = connectionState(messageContext.connection());
	const auto now = m_clock();

	// count the request only if both limits accept it
	if (connection.limiter.wouldAcceptRequest(now) == false || m_forwardingLimiter.wouldAcceptRequest(now) == false)
	{
		vWarning() << "directory broker request rate of" << client->hostAddress() << "exceeded";
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::RateLimited));
		return true;
	}
	(void)connection.limiter.acceptRequest(now);
	(void)m_forwardingLimiter.acceptRequest(now);

	const auto validationError = DirectoryBrokerMessage::validateRequest(*request);
	if (validationError.has_value())
	{
		vWarning() << "invalid directory broker request" << request->command << "from" << client->hostAddress()
				   << validationError->errorMessage();
		sendReply(server, messageContext, *validationError);
		return true;
	}

	// a master can only continue its own cursors (the service's cursors are per session)
	const auto cursor = request->arguments.value(Key::Cursor).toString();
	if (cursor.isEmpty() == false && connection.cursors.removeAll(cursor) == 0)
	{
		vWarning() << "unknown cursor from" << client->hostAddress();
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::BadRequest, QStringLiteral("unknown or expired cursor")));
		return true;
	}

	if (connection.limiter.acquireOutstandingRequest() == false)
	{
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::RateLimited));
		return true;
	}

	if (m_forwardingLimiter.acquireOutstandingRequest() == false)
	{
		connection.limiter.releaseOutstandingRequest();
		sendReply(server, messageContext, DirectoryBrokerMessage::error(requestId, Error::RateLimited));
		return true;
	}

	vDebug() << request->command << "for" << client->username() << "at" << client->hostAddress();

	const auto forward = m_forwardFunction ? m_forwardFunction : [](const QString& command, const QVariantMap& arguments, const DirectoryBrokerClient::ReplyHandler& replyHandler) {
		DirectoryBrokerClient::configuredClient()->requestAsync(command, arguments, replyHandler);
	};

	const QPointer<DirectoryBrokerFeaturePlugin> plugin(this);
	const auto connectionObject = messageContext.connection();
	const auto connectionSerial = connection.serial;
	const auto serverPointer = &server;
	const auto command = request->command;

	// queued behind the session server's own requests by the service; overwrites a master's value
	auto arguments = request->arguments;
	arguments[Key::Forwarded] = true;

	forward(command, arguments,
			[=](const DirectoryBrokerMessage::Reply& serviceReply) {
		if (plugin.isNull())
		{
			return;
		}

		plugin->m_forwardingLimiter.releaseOutstandingRequest();
		plugin->finishRequest(connectionObject, connectionSerial, serviceReply.arguments.value(Key::NextCursor).toString());

		auto reply = serviceReply;
		reply.requestId = requestId;
		if (reply.isError() == false)
		{
			reply.command = command;
		}

		// the context's device is gone if the master disconnected meanwhile
		plugin->sendReply(*serverPointer, messageContext, reply);
	});

	return true;
}



bool DirectoryBrokerFeaturePlugin::isAuthorized(const VncServerClient* client)
{
	if (client == nullptr ||
		client->accessControlState() != VncServerClient::AccessControlState::Successful)
	{
		return false;
	}

	// Token authentication skips access control (ServerAccessControlManager)
	switch (client->authType())
	{
	case RfbVeyonAuth::KeyFile:
	case RfbVeyonAuth::Logon:
		return true;
	default:
		break;
	}

	return false;
}



void DirectoryBrokerFeaturePlugin::sendReply(VeyonServerInterface& server, const MessageContext& messageContext,
											 const DirectoryBrokerMessage::Reply& reply)
{
	if (messageContext.ioDevice())
	{
		FeatureMessage message{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(reply, message);
		server.sendFeatureMessageReply(messageContext, message);
	}
}



DirectoryBrokerFeaturePlugin::Connection& DirectoryBrokerFeaturePlugin::connectionState(QObject* connection)
{
	auto it = m_connections.find(connection);
	if (it == m_connections.end())
	{
		it = m_connections.insert(connection, Connection{});
		it->serial = m_nextConnectionSerial++;

		if (connection)
		{
			connect(connection, &QObject::destroyed, this, [this, connection]() { m_connections.remove(connection); });
		}
	}

	return it.value();
}



void DirectoryBrokerFeaturePlugin::finishRequest(QObject* connection, quint64 serial, const QString& nextCursor)
{
	const auto it = m_connections.find(connection);
	if (it == m_connections.end() || it->serial != serial)
	{
		return;
	}

	it->limiter.releaseOutstandingRequest();

	if (nextCursor.isEmpty() == false)
	{
		// the oldest cursor of the connection is given up (and eventually evicted by the service)
		it->cursors.append(nextCursor);
		while (it->cursors.size() > DirectoryBrokerLimits::MaximumCursorsPerMaster)
		{
			it->cursors.removeFirst();
		}
	}
}
