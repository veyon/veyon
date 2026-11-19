// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDeadlineTimer>
#include <QMutex>
#include <QWaitCondition>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerPendingRequests.h"
#include "VncConnection.h"

class ComputerControlInterface;
class FeatureMessage;
class VeyonConnection;

// Directory broker client of the master, CLI and configurator (04-directory-broker.md §3): sends
// broker feature messages over the normal authenticated Veyon connection to the session server on
// DirectoryBroker/Host, which forwards them to its service if the connection passed access control.
// The connection is opened on first use. Only the local session server is supported: a central
// DirectoryBroker/Host requires a TLS connection with verified host identity (05 §4.7), which this
// build does not provide, so requests to other hosts are refused.
//
// request() may be called from any thread (replies are received in the connection's thread),
// requestAsync() only in the thread of this object.
class VEYON_CORE_EXPORT DirectoryBrokerConnectionClient : public QObject, public DirectoryBrokerClient
{
	Q_OBJECT
public:
	explicit DirectoryBrokerConnectionClient(const QString& host, QObject* parent = nullptr);
	~DirectoryBrokerConnectionClient() override;

	Reply request(const QString& command, const QVariantMap& arguments) override;
	void requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler) override;

private:
	static constexpr int RateLimitRetryDelay = 100;
	static constexpr int ShutdownTimeout = 5000;

	void sendAsyncRequest(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler,
						  QDeadlineTimer deadline);
	void start();
	void updateState();
	void handleFeatureMessage(const FeatureMessage& message);
	void sendQueuedRequests();

	static bool isFailedState(VncConnection::State state);
	static QString stateErrorMessage(VncConnection::State state);

	struct QueuedRequest
	{
		quint32 requestId;
		QString command;
		QVariantMap arguments;
	};

	const QString m_host;
	const bool m_isLocalHost;

	// main thread only
	ComputerControlInterface* m_computerControlInterface = nullptr;
	QList<QueuedRequest> m_queuedRequests;

	mutable QMutex m_stateMutex;
	QWaitCondition m_stateChanged;
	bool m_started = false;
	VncConnection::State m_state = VncConnection::State::None;
	VeyonConnection* m_connection = nullptr;

	DirectoryBrokerPendingRequests m_pendingRequests;
};
