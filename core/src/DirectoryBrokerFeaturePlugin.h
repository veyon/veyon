// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerLimits.h"
#include "DirectoryBrokerRateLimiter.h"
#include "FeatureProviderInterface.h"

class VncServerClient;

// Builtin directory broker feature (04-directory-broker.md §4, §5). In the session server it
// forwards broker requests of masters to veyon-service over IPC, but only for connections that
// passed access control and authenticated with key file or logon authentication (never for Token
// connections, which skip access control); everything else is answered with "denied". Requests are
// validated and rate-limited per connection and in total before being forwarded, marked as
// forwarded (the service serves the session server's own requests first), and a connection can
// only continue the last few cursors it received. The master side replies are handled by
// DirectoryBrokerConnectionClient directly.
class VEYON_CORE_EXPORT DirectoryBrokerFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	using Clock = std::function<qint64()>;
	using ForwardFunction = std::function<void(const QString& command, const QVariantMap& arguments,
											   const DirectoryBrokerClient::ReplyHandler& replyHandler)>;

	explicit DirectoryBrokerFeaturePlugin(QObject* parent = nullptr);

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("8249f184-3a79-4b79-ab7e-6edfb88f9529")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("DirectoryBroker");
	}

	QString description() const override
	{
		return tr("Directory and user groups broker");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(operation)
		Q_UNUSED(arguments)
		Q_UNUSED(computerControlInterfaces)

		return false;
	}

	bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	// whether broker requests received on a connection may be forwarded (04 §5)
	static bool isAuthorized(const VncServerClient* client);

	// tests: replaces forwarding to veyon-service and the clock of the rate limits
	void setForwardFunction(const ForwardFunction& forwardFunction)
	{
		m_forwardFunction = forwardFunction;
	}

	void setClock(const Clock& clock)
	{
		m_clock = clock;
	}

private:
	struct Connection
	{
		quint64 serial = 0;
		DirectoryBrokerRateLimiter limiter{DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster,
										   DirectoryBrokerLimits::MaximumOutstandingRequestsPerMaster};
		// cursors returned to this connection, oldest first
		QStringList cursors{};
	};

	void sendReply(VeyonServerInterface& server, const MessageContext& messageContext,
				   const DirectoryBrokerMessage::Reply& reply);
	Connection& connectionState(QObject* connection);
	void finishRequest(QObject* connection, quint64 serial, const QString& nextCursor);

	const Feature m_feature;
	const FeatureList m_features;

	ForwardFunction m_forwardFunction{};
	Clock m_clock;

	QHash<QObject*, Connection> m_connections;
	quint64 m_nextConnectionSerial = 1;
	// all master connections together, leaving capacity for the server's own access control queries
	DirectoryBrokerRateLimiter m_forwardingLimiter{DirectoryBrokerLimits::MaximumForwardedRequestsPerSecond,
												   DirectoryBrokerLimits::MaximumForwardedOutstandingRequests};
};
