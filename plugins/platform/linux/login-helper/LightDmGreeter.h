/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "LightDmAdapter.h"

#include <QDBusContext>
#include <QDBusMessage>
#include <QProcess>
#include <QTimer>

class LightDmGreeter : public QObject, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "io.veyon.LoginAgent")
public:
	explicit LightDmGreeter(QObject* parent = nullptr);
	bool start(const QStringList& command);

public Q_SLOTS:
	bool Login(const QString& username, const QByteArray& password, const QString& session);
	void Cancel();

private:
	bool isHelperCaller() const;
	void registerAgent();
	void complete(bool success, const QString& error);
	LightDmAdapter m_adapter;
	QDBusMessage m_successReply;
	QDBusMessage m_errorReply;
	bool m_pending = false;
	bool m_registered = false;
	bool m_registering = false;
	quint64 m_registrationGeneration = 0;
	QString m_registrationError;
	QTimer m_deadline;
	QTimer m_registration;
	QProcess* m_greeter = nullptr;
};
