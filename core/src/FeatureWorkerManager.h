// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>
#include <QProcess>
#include <QRecursiveMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "CryptoCore.h"
#include "FeatureMessage.h"

class FeatureManager;
class VeyonServerInterface;

class VEYON_CORE_EXPORT FeatureWorkerManager : public QObject
{
	Q_OBJECT
public:
	enum class MessageArgument {
		AuthToken
	};
	Q_ENUM(MessageArgument)

	FeatureWorkerManager(VeyonServerInterface& server, QObject* parent = nullptr);
	~FeatureWorkerManager() override;

	bool startManagedSystemWorker(Feature::Uid featureUid);
	bool startUnmanagedSessionWorker(Feature::Uid featureUid);

	bool stopWorker(Feature::Uid featureUid);

	void sendMessageToManagedSystemWorker(const FeatureMessage& message);
	void sendMessageToUnmanagedSessionWorker(const FeatureMessage& message);

	bool isWorkerRunning(Feature::Uid featureUid);

private:
	void acceptConnection();
	void processConnection(QTcpSocket* socket);
	void closeConnection(QTcpSocket* socket);

	void sendMessage(const FeatureMessage& message);

	void sendPendingMessages();

	void enqueueSessionWorkerMessage(const FeatureMessage& message);
	void retryPendingSessionWorkerMessages();
	void watchWorkerAuthentication(Feature::Uid featureUid, const QByteArray& token);
	void removeStaleWorker(Feature::Uid featureUid, const QByteArray& token);

	static constexpr auto UnmanagedSessionProcessRetryInterval = 5000;
	static constexpr auto MaximumPendingSessionWorkerMessages = 256;
	static constexpr auto WorkerAuthenticationTimeout = 30000;

	VeyonServerInterface& m_server;
	QTcpServer m_tcpServer;

	struct Worker
	{
		QPointer<QTcpSocket> socket;
		QPointer<QProcess> process;
		QList<FeatureMessage> pendingMessages;
		QByteArray token = CryptoCore::generateChallenge().toBase64();
		bool authenticated = false;
	};

	using WorkerMap = QMap<Feature::Uid, Worker>;
	WorkerMap m_workers;

	QRecursiveMutex m_workersMutex;

	QMap<Feature::Uid, QList<FeatureMessage>> m_pendingSessionWorkerMessages;
	QTimer m_sessionWorkerRetryTimer;
};
