// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLocalSocket>
#include <QProcess>
#include <QThread>

#include "InputBlockHelper.h"
#include "VeyonCore.h"

static const auto SocketPath = QStringLiteral("/run/veyon/input-block.sock");

InputBlockHelper::InputBlockHelper(QObject* parent) :
	QObject(parent)
{
}

InputBlockHelper::~InputBlockHelper()
{
	stop();
}

bool InputBlockHelper::start()
{
	if (m_process)
	{
		vDebug() << "InputBlockHelper already started";
		return true;
	}

	m_process = new QProcess(this);
	m_process->start(QStringLiteral("veyon-input-helper"),
					 QStringList{QString::number(QCoreApplication::applicationPid())});

	if (m_process->waitForStarted(5000) == false)
	{
		vWarning() << "Failed to start veyon-input-helper:" << m_process->errorString();
		delete m_process;
		m_process = nullptr;
		return false;
	}

	vInfo() << "veyon-input-helper started, PID" << m_process->processId();
	return true;
}

void InputBlockHelper::stop()
{
	if (m_process == nullptr)
		return;

	unblock(); // release any held grabs

	m_process->terminate();
	if (m_process->waitForFinished(5000) == false)
	{
		m_process->kill();
		m_process->waitForFinished(3000);
	}
	delete m_process;
	m_process = nullptr;
}

bool InputBlockHelper::block()
{
	if (ensureRunning() == false)
		return false;

	const bool ok = sendCommand(QStringLiteral("block"));
	if (ok)
		m_blocked = true;
	return ok;
}

bool InputBlockHelper::unblock()
{
	if (ensureRunning() == false)
		return false;

	const bool ok = sendCommand(QStringLiteral("unblock"));
	if (ok)
		m_blocked = false;
	return ok;
}

bool InputBlockHelper::sendCommand(const QString& cmd)
{
	QLocalSocket socket;
	socket.connectToServer(SocketPath);
	if (socket.waitForConnected(3000) == false)
	{
		vWarning() << "InputBlockHelper: failed to connect to socket";
		return false;
	}

	socket.write(cmd.toUtf8() + '\n');
	socket.flush();

	if (socket.waitForReadyRead(3000) == false)
	{
		vWarning() << "InputBlockHelper: no response";
		return false;
	}

	const auto response = socket.readAll().trimmed();
	return response == "ok";
}

bool InputBlockHelper::ensureRunning()
{
	// restart a helper which crashed or got killed - otherwise input blocking silently stops working
	if (m_process && m_process->state() == QProcess::NotRunning)
	{
		vWarning() << "veyon-input-helper is not running anymore - restarting it";
		delete m_process;
		m_process = nullptr;
	}

	if (m_process == nullptr && start() == false)
	{
		return false;
	}

	QElapsedTimer socketWaitTimer;
	socketWaitTimer.start();

	while (socketWaitTimer.elapsed() < SocketWaitTimeout)
	{
		QLocalSocket socket;
		socket.connectToServer(SocketPath);
		if (socket.waitForConnected(SocketWaitTimeout))
		{
			return true;
		}

		if (socket.error() == QLocalSocket::ServerNotFoundError)
		{
			socket.abort();
			QThread::msleep(50);
		}
		else
		{
			return false;
		}
	}

	return false;
}
