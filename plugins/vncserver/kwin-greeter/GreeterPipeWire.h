/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

class GreeterPipeWireProcess : public QProcess
{
public:
	GreeterPipeWireProcess();
private:
	void prepareChild();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	void setupChildProcess() override { prepareChild(); }
#endif
	qint64 m_parentPid;
};

class GreeterPipeWire : public QObject
{
	Q_OBJECT
public:
	explicit GreeterPipeWire(QObject* parent = nullptr);
	~GreeterPipeWire() override;
	void start();
	// Returns a connected FD owned by the caller, or -1 with errno set.
	static int connectSocket(const QString& path, uint expectedUid);

Q_SIGNALS:
	void ready(int fd);
	void failed(const QString& error);

private:
	void probe();
	void fail(const QString& error);
	GreeterPipeWireProcess m_daemon;
	QTimer m_probe;
	QTimer m_deadline;
	QString m_socketPath;
	bool m_ready{false};
	bool m_failed{false};
};
