/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmGreeter.h"
#include "FdChannel.h"
#include "LoginInterfaces.h"
#include "LoginPeer.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QDebug>
#include <fcntl.h>
#include <csignal>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
class GreeterProcess : public QProcess
{
public:
	GreeterProcess(int fd, QObject* parent) : QProcess(parent), m_fd(fd), m_parentPid(::getpid())
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		const auto parentPid = m_parentPid;
		setChildProcessModifier([fd, parentPid]() { prepareChild(fd, parentPid); });
#endif
	}
protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	void setupChildProcess() override { prepareChild(m_fd, m_parentPid); }
#endif
private:
	static void prepareChild(int fd, pid_t parentPid)
	{
		// LightDM manages the wrapper PID. The original UI must also stop if
		// LightDM terminates the wrapper while opening the desktop session.
		if (::prctl(PR_SET_PDEATHSIG, SIGTERM) != 0 || ::getppid() != parentPid ||
			::fcntl(fd, F_SETFD, 0) < 0)
		{
			::_exit(1);
		}
	}
	[[maybe_unused]] int m_fd;
	pid_t m_parentPid;
};
}

LightDmGreeter::LightDmGreeter(QObject* parent) : QObject(parent), m_adapter(this)
{
	m_deadline.setSingleShot(true);
	connect(&m_deadline, &QTimer::timeout, &m_adapter, &LightDmAdapter::cancel);
	connect(&m_registration, &QTimer::timeout, this, &LightDmGreeter::registerAgent);
	connect(&m_adapter, &LightDmAdapter::ready, this, &LightDmGreeter::registerAgent);
	connect(&m_adapter, &LightDmAdapter::finished, this, &LightDmGreeter::complete);
	connect(&m_adapter, &LightDmAdapter::protocolError, this, [](const QString& error) {
		qCritical().noquote() << error;
		QCoreApplication::exit(1);
	});
}

bool LightDmGreeter::start(const QStringList& command)
{
	qInfo() << "Starting LightDM bridge; PID" << ::getpid() << "parent PID" << ::getppid();
	if (command.isEmpty())
	{
		qCritical() << "Expected the original LightDM greeter command after --";
		return false;
	}
	bool readOk = false, writeOk = false;
	int readFd = qEnvironmentVariableIntValue("LIGHTDM_FROM_SERVER_FD", &readOk);
	int writeFd = qEnvironmentVariableIntValue("LIGHTDM_TO_SERVER_FD", &writeOk);
	if (!readOk || !writeOk)
	{
		const auto pipe = qEnvironmentVariable("LIGHTDM_GREETER_PIPE");
		if (pipe.isEmpty())
		{
			qCritical() << "No private LightDM greeter connection was inherited";
			return false;
		}
		QLocalSocket socket;
		socket.connectToServer(pipe);
		if (!socket.waitForConnected(3000))
		{
			qCritical() << "Could not connect to the LightDM greeter socket";
			return false;
		}
		readFd = writeFd = ::dup(int(socket.socketDescriptor()));
	}
	// The display manager connection must never leak into the child greeter:
	// the wrapper alone multiplexes manual and remote authentication.
	auto* daemon = new FdChannel(readFd, writeFd, this);
	if (!daemon->isValid())
	{
		qCritical() << "Invalid LightDM file descriptors";
		return false;
	}
	int sockets[2];
	if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) != 0)
	{
		qCritical() << "Could not create the greeter bridge";
		return false;
	}
	auto* child = new FdChannel(sockets[0], sockets[0], this);
	if (!child->isValid())
	{
		::close(sockets[1]);
		return false;
	}
	connect(daemon, &FdChannel::received, &m_adapter, &LightDmAdapter::receiveFromDaemon);
	connect(child, &FdChannel::received, &m_adapter, &LightDmAdapter::receiveFromGreeter);
	connect(&m_adapter, &LightDmAdapter::writeToDaemon, daemon, &FdChannel::send);
	connect(&m_adapter, &LightDmAdapter::writeToGreeter, child, &FdChannel::send);
	connect(daemon, &FdChannel::disconnected, this, []() { QCoreApplication::quit(); });
	connect(child, &FdChannel::disconnected, this, []() { QCoreApplication::quit(); });
	auto bus = QDBusConnection::systemBus();
	if (!bus.registerObject(QString::fromLatin1(LoginHelperInterface::AgentPath), this, QDBusConnection::ExportAllSlots))
	{
		qWarning() << "Remote login is unavailable: could not export the greeter bridge";
	}
	auto* watcher = new QDBusServiceWatcher(QString::fromLatin1(LoginHelperInterface::Service), bus,
		QDBusServiceWatcher::WatchForUnregistration, this);
	connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this]() {
		m_registered = false;
		++m_registrationGeneration;
		m_adapter.cancel();
	});
	m_greeter = new GreeterProcess(sockets[1], this);
	auto environment = QProcessEnvironment::systemEnvironment();
	environment.remove(QStringLiteral("LIGHTDM_GREETER_PIPE"));
	environment.insert(QStringLiteral("LIGHTDM_FROM_SERVER_FD"), QString::number(sockets[1]));
	environment.insert(QStringLiteral("LIGHTDM_TO_SERVER_FD"), QString::number(sockets[1]));
	m_greeter->setProcessEnvironment(environment);
	m_greeter->setProcessChannelMode(QProcess::ForwardedChannels);
	connect(m_greeter, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
		[](int code, QProcess::ExitStatus status) { QCoreApplication::exit(status == QProcess::NormalExit ? code : 1); });
	connect(m_greeter, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart)
		{
			qCritical() << "Could not start the original LightDM greeter";
			QCoreApplication::exit(1);
		}
	});
	m_greeter->start(command.first(), command.mid(1));
	const bool started = m_greeter->waitForStarted(3000);
	::close(sockets[1]);
	if (!started)
	{
		return false;
	}
	m_registration.start(3000);
	registerAgent();
	return true;
}

void LightDmGreeter::registerAgent()
{
	if (m_registered || m_registering || !m_adapter.isReady())
	{
		return;
	}
	m_registering = true;
	const auto generation = m_registrationGeneration;
	const auto request = QDBusMessage::createMethodCall(QString::fromLatin1(LoginHelperInterface::Service),
		QString::fromLatin1(LoginHelperInterface::Path), QString::fromLatin1(LoginHelperInterface::Interface), QStringLiteral("RegisterAgent"));
	auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(request, 5000), this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, generation]() {
		const QDBusPendingReply<bool> reply = *watcher;
		m_registering = false;
		if (generation != m_registrationGeneration)
		{
			watcher->deleteLater();
			return;
		}
		m_registered = reply.isValid() && reply.value();
		if (reply.isError() && m_registrationError != reply.error().message())
		{
			m_registrationError = reply.error().message();
			qWarning().noquote() << "Remote login bridge registration failed:" << m_registrationError;
		}
		if (m_registered)
		{
			qInfo() << "LightDM bridge registration accepted by login helper";
			m_registrationError.clear();
		}
		watcher->deleteLater();
	});
}

bool LightDmGreeter::isHelperCaller() const
{
	return isLoginServicePeer(connection(), message().service(), QString::fromLatin1(LoginHelperInterface::Service));
}

bool LightDmGreeter::Login(const QString& username, const QByteArray& password, const QString& session)
{
	if (!isHelperCaller())
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Only the system login helper may control the greeter"));
		return false;
	}
	if (m_pending || !m_adapter.isReady() || m_adapter.isBusy())
	{
		sendErrorReply(QDBusError::LimitsExceeded, QStringLiteral("The LightDM greeter is not ready or a login is already in progress"));
		return false;
	}
	m_successReply = message().createReply(QVariant::fromValue(true));
	m_errorReply = message().createErrorReply(QStringLiteral("io.veyon.LoginHelper.Error.AuthenticationFailed"), QString());
	m_pending = true;
	qInfo() << "LightDM bridge accepted a login request";
	setDelayedReply(true);
	m_deadline.start(LoginHelperInterface::AuthenticationTimeout);
	if (!m_adapter.login(username, password, session))
	{
		complete(false, QStringLiteral("Invalid LightDM login request"));
	}
	return false;
}

void LightDmGreeter::Cancel()
{
	if (!isHelperCaller())
	{
		sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Only the system login helper may cancel a login"));
		return;
	}
	m_adapter.cancel();
}

void LightDmGreeter::complete(bool success, const QString& error)
{
	if (!m_pending)
	{
		return;
	}
	m_pending = false;
	qInfo() << "LightDM bridge login result:" << success << error;
	m_deadline.stop();
	const auto bus = QDBusConnection::systemBus();
	if (success)
	{
		bus.send(m_successReply);
	}
	else
	{
		m_errorReply.setArguments({error});
		bus.send(m_errorReply);
	}
	m_successReply = QDBusMessage();
	m_errorReply = QDBusMessage();
}
