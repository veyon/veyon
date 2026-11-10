/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <memory>

class KWinScreencast : public QObject
{
	Q_OBJECT
public:
	explicit KWinScreencast(int timeoutMs = 5000, QObject* parent = nullptr);
	~KWinScreencast() override;
	// Takes ownership of a connected Wayland socket; no implicit WAYLAND_SOCKET.
	bool start(int waylandFd);
	void stop();

Q_SIGNALS:
	void streamCreated(quint32 nodeId);
	void failed(const QString& error);

private:
	class Connection;
	std::unique_ptr<Connection> m_connection;
	int m_timeoutMs;
};
