/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LoginService.h"
#include "LoginInterfaces.h"
#include "LoginSession.h"
#include "LightDmSession.h"
#include "LoginPeer.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QTimer>
#include <QRegularExpression>
#include <pwd.h>
#include <unistd.h>

class LoginOperation : public QObject
{
public:
	using QObject::QObject;
	QDBusMessage successReply;
	QDBusMessage errorReply;
	QString seat;
	QString agent;
	QString requester;
	uint uid = uint(-1);
	QTimer deadline;
	QTimer poll;
};

LoginService::LoginService(QObject* parent) : QObject(parent) {}

bool LoginService::start()
{
	m_process = ProcessIdentity::read(uint(::getpid()));
	if (!m_process.isValid() || !m_process.trustedExecutable || m_process.uid != 0)
	{
		qCritical() << "Login helper executable identity is not trusted" << m_process.executable;
		return false;
	}
	auto bus = QDBusConnection::systemBus();
	bus.connect(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"),
		QStringLiteral("org.freedesktop.DBus"), QStringLiteral("NameOwnerChanged"),
		this, SLOT(nameOwnerChanged(QString,QString,QString)));
	const bool started = bus.registerObject(QString::fromLatin1(LoginHelperInterface::Path), this, QDBusConnection::ExportAllSlots) &&
		bus.registerService(QString::fromLatin1(LoginHelperInterface::Service));
	if (started)
	{
		qInfo() << "Login helper is ready on the system bus; PID" << m_process.pid;
	}
	else
	{
		qCritical() << "Login helper could not acquire its D-Bus endpoint:" << bus.lastError().message();
	}
	return started;
}

bool LoginService::RegisterAgent()
{
	const auto bus = connection();
	const auto uid = bus.interface()->serviceUid(message().service());
	const auto pid = bus.interface()->servicePid(message().service());
	if (!uid.isValid() || !pid.isValid())
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Could not identify the greeter"));
		return false;
	}
	const auto process = ProcessIdentity::read(pid.value());
	const auto parent = ProcessIdentity::read(process.parentPid);
	// Reject arbitrary callers before the more expensive logind queries. Only
	// the installed helper, directly launched by root LightDM, is eligible.
	if (!process.isLightDmWrapper(parent, m_process, process.parentPid, uid.value()))
	{
		qWarning() << "Rejected greeter registration: PID" << pid.value() << "UID" << uid.value()
			<< "executable" << process.executable << "parent PID" << process.parentPid
			<< "parent executable" << parent.executable << "parent UID" << parent.uid
			<< "trusted executable" << process.trustedExecutable;
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Caller is not a wrapper directly launched by root LightDM"));
		return false;
	}
	const auto session = LoginSession::read(bus, LoginSession::forPid(bus, pid.value()));
	if (!session.isLightDmGreeter() || !process.isLightDmWrapper(parent, m_process, session.leader, session.uid))
	{
		qWarning() << "Rejected greeter session registration:" << session.path << "leader" << session.leader
			<< "wrapper parent" << process.parentPid << "service" << session.service
			<< "class" << session.sessionClass << "type" << session.type << "state" << session.state;
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Caller does not belong to an active LightDM greeter session"));
		return false;
	}
	const auto existing = m_agents.constFind(session.path);
	if (existing != m_agents.cend() && existing->owner != message().service())
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("A greeter is already registered for this session"));
		return false;
	}
	m_agents.insert(session.path, {message().service(), process});
	qInfo() << "LightDM bridge registered for session" << session.path << "seat" << session.seat
		<< "PID" << process.pid << "UID" << session.uid;
	return true;
}

bool LoginService::verifyAgent(const Agent& agent, const LoginSession& session) const
{
	const auto bus = QDBusConnection::systemBus();
	const auto uid = bus.interface()->serviceUid(agent.owner);
	const auto pid = bus.interface()->servicePid(agent.owner);
	if (!uid.isValid() || !pid.isValid() || uid.value() != session.uid || pid.value() != agent.process.pid)
	{
		return false;
	}
	const auto process = ProcessIdentity::read(pid.value());
	const auto parent = ProcessIdentity::read(process.parentPid);
	return agent.process.sameProcess(process) &&
		process.isLightDmWrapper(parent, m_process, session.leader, session.uid) &&
		LoginSession::forPid(bus, process.pid) == session.path;
}

bool LoginService::Login(const QString& sessionPath, const QString& username, const QByteArray& password, const QString& desktop)
{
	const auto bus = connection();
	if (!isLoginServicePeer(bus, message().service(), QString::fromLatin1(LoginHelperInterface::VeyonService)))
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Only the system Veyon service may request a login"));
		return false;
	}
	static const QRegularExpression sessionName(QStringLiteral("^[A-Za-z0-9_.+-]*$"));
	if (username.isEmpty() || username.size() > 1024 || username.contains(QLatin1Char('\0')) ||
		password.size() > 4096 || password.contains('\0') || desktop.size() > 256 || !sessionName.match(desktop).hasMatch())
	{
		sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("Invalid login parameters"));
		return false;
	}
	const auto session = LoginSession::read(bus, sessionPath);
	if (!session.isLightDmGreeter())
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("The target is not an active LightDM greeter"));
		return false;
	}
	if (m_operations.contains(session.seat))
	{
		sendErrorReply(QDBusError::LimitsExceeded, QStringLiteral("A login is already in progress on this seat"));
		return false;
	}
	const auto agent = m_agents.constFind(sessionPath);
	if (agent == m_agents.cend())
	{
		qWarning() << "Login rejected: no registered LightDM bridge for" << sessionPath;
		sendErrorReply(QDBusError::NotSupported, QStringLiteral("LightDM greeter-wrapper is not registered; configure veyon-login-helper --lightdm-greeter"));
		return false;
	}
	const auto registeredAgent = agent.value();
	bool listed = false;
	const auto sessions = LoginSession::list(bus, &listed);
	if (!listed)
	{
		sendErrorReply(QDBusError::Failed, QStringLiteral("Could not query logind sessions"));
		return false;
	}
	for (const auto& other : sessions)
	{
		if (other.seat == session.seat && other.isUser())
		{
			sendErrorReply(QDBusError::AccessDenied, QStringLiteral("A user is already logged in on this seat"));
			return false;
		}
	}
	// Resolve the account through NSS (including SSSD/domain accounts), so a
	// different graphical session cannot be mistaken for this login's success.
	const auto account = getpwnam(username.toUtf8().constData());
	if (!account)
	{
		sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("The requested account could not be resolved through NSS"));
		return false;
	}
	const auto accountUid = account->pw_uid;
	// NSS and session enumeration may take time. Revalidate the unique bus
	// connection and process identity immediately before sending credentials.
	if (!verifyAgent(registeredAgent, session))
	{
		qWarning() << "Login rejected: stale or untrusted LightDM bridge for" << sessionPath;
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("The registered LightDM wrapper identity is no longer valid"));
		return false;
	}
	if (!LightDmSession::isAvailable(desktop, session.leader))
	{
		sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("The configured LightDM desktop session is unavailable"));
		return false;
	}
	auto* operation = new LoginOperation(this);
	// Keep reply envelopes, not the incoming message containing credentials.
	operation->successReply = message().createReply(QVariant::fromValue(true));
	operation->errorReply = message().createErrorReply(QStringLiteral("io.veyon.LoginHelper.Error.LoginFailed"), QString());
	operation->seat = session.seat;
	operation->agent = registeredAgent.owner;
	operation->requester = message().service();
	operation->uid = accountUid;
	qInfo() << "Forwarding login to LightDM bridge for session" << sessionPath << "seat" << session.seat
		<< "account UID" << accountUid << "bridge PID" << registeredAgent.process.pid;
	m_operations.insert(session.seat, operation);
	setDelayedReply(true);
	operation->deadline.setSingleShot(true);
	connect(&operation->deadline, &QTimer::timeout, operation, [this, operation]() {
		finish(operation, false, QStringLiteral("Timed out waiting for the user's graphical session"));
	});
	connect(&operation->poll, &QTimer::timeout, operation, [this, operation]() { checkSession(operation); });
	operation->deadline.start(LoginHelperInterface::LoginTimeout);
	operation->poll.start(500);
	auto request = QDBusMessage::createMethodCall(operation->agent, QString::fromLatin1(LoginHelperInterface::AgentPath),
		QString::fromLatin1(LoginHelperInterface::AgentInterface), QStringLiteral("Login"));
	request.setArguments({username, password, desktop});
	auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(request, LoginHelperInterface::AuthenticationTimeout + 2000), operation);
	connect(watcher, &QDBusPendingCallWatcher::finished, operation, [this, operation, watcher]() {
		const QDBusPendingReply<bool> reply = *watcher;
		watcher->deleteLater();
		if (reply.isError())
		{
			// LightDM stops the greeter while opening the user session. Its
			// disappearance is not authentication failure; logind decides success.
			if (reply.error().type() != QDBusError::NoReply && reply.error().type() != QDBusError::ServiceUnknown &&
				reply.error().type() != QDBusError::Disconnected)
			{
				finish(operation, false, reply.error().message());
			}
		}
		else if (!reply.value())
		{
			finish(operation, false, QStringLiteral("LightDM rejected the login"));
		}
	});
	return false;
}

void LoginService::checkSession(LoginOperation* operation)
{
	bool ok = false;
	const auto sessions = LoginSession::list(QDBusConnection::systemBus(), &ok);
	if (ok)
	{
		for (const auto& session : sessions)
		{
			if (session.uid == operation->uid && session.seat == operation->seat && session.isUser())
			{
				finish(operation, true);
				return;
			}
		}
	}
}

void LoginService::finish(LoginOperation* operation, bool success, const QString& error)
{
	if (m_operations.value(operation->seat) != operation)
	{
		return;
	}
	m_operations.remove(operation->seat);
	operation->poll.stop();
	operation->deadline.stop();
	const auto bus = QDBusConnection::systemBus();
	if (success)
	{
		qInfo() << "Login completed on seat" << operation->seat << "account UID" << operation->uid;
		bus.send(operation->successReply);
	}
	else
	{
		qWarning() << "Login failed on seat" << operation->seat << "account UID" << operation->uid << error;
		operation->errorReply.setArguments({error});
		bus.send(operation->errorReply);
		auto cancel = QDBusMessage::createMethodCall(operation->agent, QString::fromLatin1(LoginHelperInterface::AgentPath),
			QString::fromLatin1(LoginHelperInterface::AgentInterface), QStringLiteral("Cancel"));
		bus.asyncCall(cancel);
	}
	operation->deleteLater();
}

void LoginService::nameOwnerChanged(const QString& name, const QString&, const QString& newOwner)
{
	if (newOwner.isEmpty())
	{
		const auto operations = m_operations.values();
		for (auto* operation : operations)
		{
			if (operation->requester == name)
			{
				finish(operation, false, QStringLiteral("The requesting Veyon service disconnected"));
			}
		}
		for (auto it = m_agents.begin(); it != m_agents.end();)
		{
			if (it->owner == name)
			{
				qInfo() << "LightDM bridge disconnected for session" << it.key();
				it = m_agents.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
