// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>
#include <QProcess>
#include <QRecursiveMutex>
#include <QTimer>

#include "FeatureMessage.h"
#include "IpcServer.h"

class FeatureManager;
class VeyonServerInterface;

// Starts feature workers and talks to them over one authenticated local IPC endpoint per worker
// start (docs/specs/02-ipc.md §6a): the endpoint is created before the worker starts, the
// credentials are passed via the worker's stdin only and feature messages are carried in records.
class VEYON_CORE_EXPORT FeatureWorkerManager : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManager(VeyonServerInterface& server, QObject* parent = nullptr);
	~FeatureWorkerManager() override;

	bool startManagedSystemWorker(Feature::Uid featureUid);
	bool startUnmanagedSessionWorker(Feature::Uid featureUid);

	bool stopWorker(Feature::Uid featureUid);

	void sendMessageToManagedSystemWorker(const FeatureMessage& message);
	void sendMessageToUnmanagedSessionWorker(const FeatureMessage& message);

	bool isWorkerRunning(Feature::Uid featureUid);

private:
	enum class WorkerType {
		ManagedSystemWorker,
		UnmanagedSessionWorker,
	};

	IpcServer* createWorkerEndpoint(Feature::Uid featureUid, WorkerType workerType,
									const QString& sessionUser = {});
	void handleWorkerConnection(Feature::Uid featureUid, const QString& endpointName, IpcConnection* connection);
	void handleWorkerMessage(Feature::Uid featureUid, const QByteArray& data);
	void removeWorker(Feature::Uid featureUid, const QString& endpointName);

	void sendMessage(const FeatureMessage& message);

	void sendPendingMessages();

	void enqueueSessionWorkerMessage(const FeatureMessage& message);
	void retryPendingSessionWorkerMessages();
	void watchWorkerAuthentication(Feature::Uid featureUid, const QString& endpointName);
	void removeStaleWorker(Feature::Uid featureUid, const QString& endpointName);

	static constexpr auto UnmanagedSessionProcessRetryInterval = 5000;
	static constexpr auto MaximumPendingSessionWorkerMessages = 256;
	static constexpr auto WorkerAuthenticationTimeout = 30000;

	VeyonServerInterface& m_server;

	struct Worker
	{
		QPointer<IpcServer> endpoint;
		QString endpointName;
		QPointer<IpcConnection> connection;
		QPointer<QProcess> process;
		QList<FeatureMessage> pendingMessages;
	};

	using WorkerMap = QMap<Feature::Uid, Worker>;
	WorkerMap m_workers;

	QRecursiveMutex m_workersMutex;

	QMap<Feature::Uid, QList<FeatureMessage>> m_pendingSessionWorkerMessages;
	QTimer m_sessionWorkerRetryTimer;
};
