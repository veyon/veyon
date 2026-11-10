/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <QByteArray>
#include <QSocketNotifier>

// Nonblocking transport for LightDM's inherited pipes. Neither a slow greeter
// nor a full pipe may block the authentication timeout or D-Bus event loop.
class FdChannel : public QObject
{
	Q_OBJECT
public:
	FdChannel(int readFd, int writeFd, QObject* parent = nullptr);
	~FdChannel() override;
	bool isValid() const { return m_valid; }
	void send(const QByteArray& data);

Q_SIGNALS:
	void received(const QByteArray& data);
	void disconnected();

private:
	void read();
	void flush();
	void fail();
	int m_readFd;
	int m_writeFd;
	QSocketNotifier* m_reader = nullptr;
	QSocketNotifier* m_writer = nullptr;
	QByteArray m_output;
	bool m_valid = false;
};
