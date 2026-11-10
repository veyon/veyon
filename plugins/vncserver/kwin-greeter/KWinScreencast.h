/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <QSize>
#include <memory>

class KWinScreencast : public QObject
{
	Q_OBJECT
public:
	explicit KWinScreencast(int timeoutMs = 5000, QObject* parent = nullptr);
	~KWinScreencast() override;
	// Takes ownership of a connected Wayland socket; no implicit WAYLAND_SOCKET.
	bool start(int waylandFd);
	// Input owns a separate socket: its protocol errors must not stop capture.
	bool startInput(int waylandFd);
	void stop();
	void keyEvent(quintptr client, quint32 symbol, bool down);
	void pointerEvent(quintptr client, int buttons, int x, int y, const QSize& frameSize);
	void releaseClient(quintptr client);
	bool pointerInputAvailable() const;
	bool keyboardInputAvailable() const;

Q_SIGNALS:
	void streamCreated(quint32 nodeId);
	void failed(const QString& error);
	void inputAvailabilityChanged(bool pointer, bool keyboard);
	void inputFailed(const QString& error);
	void inputStopped();

private:
	class Connection;
	class InputConnection;
	void retireInput();
	std::unique_ptr<Connection> m_connection;
	std::unique_ptr<InputConnection> m_inputConnection;
	int m_timeoutMs;
	int m_retiringInput{0};
};
