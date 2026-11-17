// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>

#include "Feature.h"
#include "FeatureMessage.h"
#include "IpcConnection.h"

class VeyonWorkerInterface;

// worker side of the authenticated server ⇄ worker channel (docs/specs/02-ipc.md §6a)
class FeatureWorkerManagerConnection : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManagerConnection(VeyonWorkerInterface& worker,
								   Feature::Uid featureUid,
								   const QString& endpointDirectory,
								   IpcEndpointCredentials&& credentials,
								   QObject* parent = nullptr);

	bool sendMessage(const FeatureMessage& message);

private:
	static constexpr auto ConnectTimeout = 3000;

	void receiveMessage(const QByteArray& data);
	void sendPendingMessages();
	void exitWorker();

	VeyonWorkerInterface& m_worker;
	Feature::Uid m_featureUid;
	QPointer<IpcConnection> m_connection;
	QList<FeatureMessage> m_pendingMessages;
};
