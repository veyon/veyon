/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QDBusContext>
#include <QDBusMessage>
#include <QHash>
#include <QObject>
#include "ProcessIdentity.h"

class LoginOperation;
struct LoginSession;

class LoginService : public QObject, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "io.veyon.LoginHelper")
public:
	explicit LoginService(QObject* parent = nullptr);
	bool start();

public Q_SLOTS:
	bool RegisterAgent();
	bool Login(const QString& sessionPath, const QString& username, const QByteArray& password, const QString& desktop);

private Q_SLOTS:
	void nameOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner);

private:
	struct Agent { QString owner; ProcessIdentity process; };
	bool verifyAgent(const Agent& agent, const LoginSession& session) const;
	void finish(LoginOperation* operation, bool success, const QString& error = {});
	void checkSession(LoginOperation* operation);
	QHash<QString, Agent> m_agents;
	QHash<QString, LoginOperation*> m_operations;
	ProcessIdentity m_process;
};
