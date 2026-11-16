// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QHostAddress>

#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "VeyonConfiguration.h"


FeatureWorkerManagerConnection::FeatureWorkerManagerConnection(VeyonWorkerInterface& worker,
															   Feature::Uid featureUid,
															   QObject* parent) :
	QObject(parent),
	m_worker(worker),
	m_port(VeyonCore::config().featureWorkerManagerPort() + VeyonCore::sessionId()),
	m_socket(this),
	m_featureUid(featureUid)
{
	connect(&m_connectTimer, &QTimer::timeout, this, &FeatureWorkerManagerConnection::tryConnection);

	connect(&m_socket, &QTcpSocket::connected,
			this, &FeatureWorkerManagerConnection::sendInitMessage);

	connect(&m_socket, &QTcpSocket::disconnected, this,
			[=]() {
		vDebug() << "lost connection to FeatureWorkerManager – exiting";
		QCoreApplication::instance()->exit(0);
	}, Qt::QueuedConnection);

	connect(&m_socket, &QTcpSocket::readyRead,
			this, &FeatureWorkerManagerConnection::receiveMessage);

	tryConnection();
}



void FeatureWorkerManagerConnection::setAuthToken(const QByteArray& authToken)
{
	if (m_authToken.isEmpty())
	{
		m_authToken = authToken;

		if (m_socket.state() == QTcpSocket::ConnectedState)
		{
			sendInitMessage();
		}
	}
	else
	{
		vCritical() << "auth token already set";
	}
}



bool FeatureWorkerManagerConnection::sendMessage(const FeatureMessage& message)
{
	vDebug() << message;

	return message.sendPlain(&m_socket);
}



void FeatureWorkerManagerConnection::tryConnection()
{
	if (m_socket.state() != QTcpSocket::ConnectedState)
	{
		vDebug() << "connecting to FeatureWorkerManager at port" << m_port;

		m_socket.connectToHost(QHostAddress::LocalHost, m_port);
		m_connectTimer.start(ConnectTimeout);
	}
}



void FeatureWorkerManagerConnection::sendInitMessage()
{
	m_connectTimer.stop();

	if (m_authToken.isEmpty() == false)
	{
		vDebug() << m_featureUid;

		FeatureMessage(m_featureUid, FeatureMessage::Command::Init)
			.addArgument(FeatureWorkerManager::MessageArgument::AuthToken, m_authToken)
			.sendPlain(&m_socket);
	}
}



void FeatureWorkerManagerConnection::receiveMessage()
{
	FeatureMessage featureMessage;

	while (featureMessage.isReadyForReceive(&m_socket))
	{
		if (featureMessage.receive(&m_socket))
		{
			VeyonCore::featureManager().handleFeatureMessage(m_worker, featureMessage);
		}
	}
}
