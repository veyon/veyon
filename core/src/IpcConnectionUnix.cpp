// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QLocalSocket>

#include "IpcConnection.h"
#include "IpcServer.h"


QLocalSocket* IpcConnection::connectToEndpoint(const QString& directory, const QString& endpointName, int timeout)
{
	QDeadlineTimer deadline(timeout);

	auto socket = new QLocalSocket;
	socket->connectToServer(IpcServer::endpointAddress(directory, endpointName));
	if (socket->waitForConnected(int(qMax<qint64>(0, deadline.remainingTime()))) == false)
	{
		vCritical() << "could not connect to" << endpointName << socket->errorString();
		delete socket;
		return nullptr;
	}

	return socket;
}
