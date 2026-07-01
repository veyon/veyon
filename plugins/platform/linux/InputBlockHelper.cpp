/*
 * InputBlockHelper.cpp - Qt wrapper for veyon-input-helper daemon
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "InputBlockHelper.h"

#include <QLocalSocket>
#include <QProcess>

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
	if (m_daemon)
	{
		vDebug() << "InputBlockHelper daemon already started";
		return true;
	}

	m_daemon = new QProcess(this);
	m_daemon->start(QStringLiteral("veyon-input-helper"), QStringList());

	if (m_daemon->waitForStarted(5000) == false)
	{
		vWarning() << "Failed to start veyon-input-helper daemon:" << m_daemon->errorString();
		delete m_daemon;
		m_daemon = nullptr;
		return false;
	}

	vInfo() << "veyon-input-helper daemon started, PID" << m_daemon->processId();
	return true;
}

void InputBlockHelper::stop()
{
	if (m_daemon == nullptr)
		return;

	unblock(); // release any held grabs

	m_daemon->terminate();
	if (m_daemon->waitForFinished(5000) == false)
	{
		m_daemon->kill();
		m_daemon->waitForFinished(3000);
	}
	delete m_daemon;
	m_daemon = nullptr;
}

bool InputBlockHelper::block()
{
	if (ensureDaemonRunning() == false)
		return false;

	const bool ok = sendCommand(QStringLiteral("block"));
	if (ok)
		m_blocked = true;
	return ok;
}

bool InputBlockHelper::unblock()
{
	if (ensureDaemonRunning() == false)
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
		vWarning() << "InputBlockHelper: failed to connect to daemon socket";
		return false;
	}

	socket.write(cmd.toUtf8() + '\n');
	socket.flush();

	if (socket.waitForReadyRead(3000) == false)
	{
		vWarning() << "InputBlockHelper: no response from daemon";
		return false;
	}

	const auto response = socket.readAll().trimmed();
	return response == "ok";
}

bool InputBlockHelper::ensureDaemonRunning()
{
	// Try connecting first — daemon may already be running
	QLocalSocket test;
	test.connectToServer(SocketPath);
	if (test.waitForConnected(1000))
	{
		test.disconnectFromServer();
		return true;
	}

	// Daemon not running — start it
	return start();
}
