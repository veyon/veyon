// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

class QLocalSocket;
class QProcess;

class InputBlockHelper : public QObject
{
	Q_OBJECT
public:
	explicit InputBlockHelper(QObject* parent = nullptr);
	~InputBlockHelper() override;

	bool start();
	void stop();
	bool block();
	bool unblock();

	bool isBlocked() const { return m_blocked; }

private:
	static constexpr auto SocketWaitTimeout = 5000;

	bool sendCommand(const QString& cmd);
	bool ensureRunning();

	QProcess* m_process{nullptr};
	bool m_blocked{false};
};
