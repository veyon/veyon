// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QTimer>

#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"


FeatureWorkerManagerConnection::FeatureWorkerManagerConnection(VeyonWorkerInterface& worker,
															   Feature::Uid featureUid,
															   const QString& endpointDirectory,
															   IpcEndpointCredentials&& credentials,
															   QObject* parent) :
	QObject(parent),
	m_worker(worker),
	m_featureUid(featureUid)
{
	// the endpoint is created by the server before the worker is started
	auto socket = IpcConnection::connectToEndpoint(endpointDirectory, credentials.endpointName(), ConnectTimeout);
	if (socket == nullptr)
	{
		vCritical() << "could not connect to FeatureWorkerManager - exiting";
		exitWorker();
		return;
	}

	m_connection = new IpcConnection(socket, IpcChannel::Side::Client, IpcProtocol::Role::Worker,
									 quint32(qMax(0, VeyonCore::sessionId())), std::move(credentials), nullptr, this);

	connect(m_connection, &IpcConnection::established, this, &FeatureWorkerManagerConnection::sendPendingMessages);
	connect(m_connection, &IpcConnection::messageReceived, this, &FeatureWorkerManagerConnection::receiveMessage);
	connect(m_connection, &IpcConnection::closed, this, [this]() {
		vDebug() << "lost connection to FeatureWorkerManager - exiting";
		exitWorker();
	});

	m_connection->start();
}



bool FeatureWorkerManagerConnection::sendMessage(const FeatureMessage& message)
{
	vDebug() << message;

	if (m_connection.isNull() || m_connection->isClosed())
	{
		return false;
	}

	m_pendingMessages.append(message);
	sendPendingMessages();

	return true;
}



void FeatureWorkerManagerConnection::receiveMessage(const QByteArray& data)
{
	FeatureMessage featureMessage;
	if (featureMessage.fromByteArray(data) == false || featureMessage.featureUid() != m_featureUid)
	{
		vCritical() << "invalid message from FeatureWorkerManager - exiting";
		m_connection->close();
		return;
	}

	VeyonCore::featureManager().handleFeatureMessage(m_worker, featureMessage);
}



void FeatureWorkerManagerConnection::sendPendingMessages()
{
	if (m_connection.isNull() || m_connection->isEstablished() == false)
	{
		return;
	}

	while (m_pendingMessages.isEmpty() == false)
	{
		if (m_connection->sendMessage(m_pendingMessages.first().toByteArray()) == false)
		{
			vWarning() << "dropping message which could not be sent";
		}
		m_pendingMessages.removeFirst();
	}
}



void FeatureWorkerManagerConnection::exitWorker()
{
	QTimer::singleShot(0, QCoreApplication::instance(), []() { QCoreApplication::instance()->exit(0); });
}
