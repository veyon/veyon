// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <csignal>
#include <grp.h>
#include <pwd.h>
#ifdef HAVE_LIBPROCPS
#include <proc/readproc.h>
#endif
#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxServerProcess.h"
#include "LinuxUserFunctions.h"
#include "Random.h"
#include "ServiceDataManager.h"
#include "VeyonConfiguration.h"

#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


LinuxServerProcess::LinuxServerProcess(const QProcessEnvironment& processEnvironment,
									   const QString& sessionPath, int sessionId,
									   LinuxUserFunctions::UserId sessionUserId,
									   LinuxSessionFunctions::Type sessionType,
									   ServiceDataManager& dataManager,
									   QObject* parent) :
	QProcess(parent),
	m_sessionPath(sessionPath),
	m_sessionId(sessionId),
	m_sessionType(sessionType),
	m_sessionUserId(sessionUserId),
	m_dataManager(dataManager)
{
	setProcessEnvironment(processEnvironment);

	connect(this, &QProcess::started, this, &LinuxServerProcess::sendIpcCredentials);
}



LinuxServerProcess::~LinuxServerProcess()
{
	stop();
}



void LinuxServerProcess::start()
{
	if (VeyonCore::config().logToSystem())
	{
		setProcessChannelMode(QProcess::ForwardedChannels);
	}

	// Debug wrappers (VEYON_VALGRIND_SERVERS, catchsegv) run the server as root. catchsegv starts it
	// as its own child, so the server's PID differs from processId() and the endpoint's PID check
	// rejects it (fail closed, the server then exits) - there is deliberately no bypass.
	const auto catchsegv{QStringLiteral("/usr/bin/catchsegv")};
	const auto runAsSessionUser = qEnvironmentVariableIsSet("VEYON_VALGRIND_SERVERS") == false &&
		(VeyonCore::isDebugging() && QFileInfo::exists(catchsegv)) == false &&
		m_sessionType == LinuxSessionFunctions::Type::Wayland &&
		m_sessionUserId != LinuxUserFunctions::InvalidUserId;

	// a new endpoint and token for every start, created before the server starts (02 §1); a
	// server running as the session user (Wayland) connects to a socket owned by that user
	if (createIpcEndpoint(runAsSessionUser ? m_sessionUserId : 0, tlsHostKeyRefusal(runAsSessionUser)) == false)
	{
		vCritical() << "not starting server for session" << m_sessionPath << "without IPC endpoint";
		return;
	}

	if (qEnvironmentVariableIsSet("VEYON_VALGRIND_SERVERS"))
	{
		QProcess::start(QStringLiteral("/usr/bin/valgrind"),
						{QStringLiteral("--error-limit=no"),
						 QStringLiteral("--log-file=valgrind-veyon-server-%1.log").arg(m_sessionId),
						 VeyonCore::filesystem().serverFilePath()});
	}
	else if (VeyonCore::isDebugging() && QFileInfo::exists(catchsegv))
	{
		QProcess::start(catchsegv, {VeyonCore::filesystem().serverFilePath()});
	}
	else if (m_sessionType == LinuxSessionFunctions::Type::Wayland)
	{
		if (m_sessionUserId == LinuxUserFunctions::InvalidUserId)
		{
			vCritical() << "failed to determine user ID of session user for" << m_sessionPath;
		}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		setChildProcessModifier([this] { setProcessUserId(); });
#endif

		if (hasDesktopPortalRegistry())
		{
			vDebug() << "xdg-desktop-portal >= 1.20 detected, launching server directly";
			QProcess::start(VeyonCore::filesystem().serverFilePath(), QStringList{});
		}
		else
		{
			startInSystemdScope();
		}
	}
	else
	{
		QProcess::start(VeyonCore::filesystem().serverFilePath(), QStringList{});
	}
}



void LinuxServerProcess::stop()
{
	const auto sendSignalRecursively = [](pid_t pid, int sig) {
		if (pid > 0)
		{
#ifdef HAVE_LIBPROCPS
			LinuxCoreFunctions::forEachChildProcess(
				[=](proc_t* procInfo) {
				if (procInfo->tid > 0 && ::kill(procInfo->tid, sig) < 0 && errno != ESRCH)
				{
					vCritical() << "kill() failed with" << errno;
				}
				return true;
			},
				pid, 0, true);
#elif defined(HAVE_LIBPROC2)
			LinuxCoreFunctions::forEachChildProcess([=](const pids_stack* stack) {
				const pid_t tid = PIDS_VAL(0, s_int, stack);
				if (tid > 0 && ::kill(tid, sig) < 0 && errno != ESRCH)
				{
					vCritical() << "kill() failed with" << errno;
				}
				return true;
			}, pid, {}, true);
#endif

			if (::kill(pid, sig) < 0 && errno != ESRCH)
			{
				vCritical() << "kill() failed with" << errno;
			}

			// clean up process
			waitpid(pid, nullptr, WNOHANG);
		}
	};

	const auto pid = pid_t(processId());

	// manually set process state since we're managing the process termination on our own
	setProcessState(QProcess::NotRunning);

	// tell x11vnc and child processes (in case spawned via catchsegv) to shutdown
	sendSignalRecursively(pid, SIGINT);

	if (LinuxCoreFunctions::waitForProcess(pid, ServerShutdownTimeout, ServerWaitSleepInterval) == false)
	{
		sendSignalRecursively(pid, SIGTERM);

		if (LinuxCoreFunctions::waitForProcess(pid, ServerTerminateTimeout, ServerWaitSleepInterval) == false)
		{
			vWarning() << "server for session" << m_sessionPath << "still running - killing now";
			sendSignalRecursively(pid, SIGKILL);
			LinuxCoreFunctions::waitForProcess(pid, ServerKillTimeout, ServerWaitSleepInterval);
		}
	}

	removeIpcEndpoint();
}



void LinuxServerProcess::setProcessUserId()
{
	if (m_sessionUserId != LinuxUserFunctions::InvalidUserId)
	{
		// Look up the user's full credential data before dropping privileges
		const auto pw_entry = getpwuid(m_sessionUserId);
		if (pw_entry != nullptr)
		{
			// Set supplementary groups first (needs EUID=root)
			initgroups(pw_entry->pw_name, pw_entry->pw_gid);
			// Set primary group GID
			if (setgid(pw_entry->pw_gid) != 0)
			{
				vCritical() << "failed to set GID";
			}
		}
		// Set UID last — after this the process has no more root privileges
		if (setuid(m_sessionUserId) != 0)
		{
			vCritical() << "failed to set UID";
		}
	}
}



bool LinuxServerProcess::createIpcEndpoint(LinuxUserFunctions::UserId serverUserId, const QString& tlsHostKeyRefusal)
{
	removeIpcEndpoint();

	IpcServer::PeerPolicy peerPolicy;
	peerPolicy.userId = qint64(serverUserId);
	peerPolicy.socketOwnerUserId = qint64(serverUserId);
	peerPolicy.checkProcessId = true;

	// the server's VeyonCore uses the default session ID unless in multi session mode
	const auto ipcSessionId = VeyonCore::config().multiSessionModeEnabled() ? m_sessionId :
																			  PlatformSessionFunctions::DefaultSessionId;

	// session data (logon credentials) is handed over from the login screen server to the one of
	// the new login screen after the display manager restart, which runs in a new logind session
	// on the same seat - bind it to the seat (or the session if it has none, e.g. remote sessions)
	const auto sessionPath = m_sessionPath;
	auto platformSessionId = LinuxSessionFunctions::getSessionSeat(sessionPath).id;
	if (platformSessionId.isEmpty())
	{
		platformSessionId = sessionPath;
	}

	auto endpoint = m_dataManager.createServerEndpoint(ipcSessionId, platformSessionId, peerPolicy, [sessionPath]() {
		// only a login screen session has no logged on user
		return LinuxSessionFunctions::getSessionClass(sessionPath) != LinuxSessionFunctions::Class::Greeter;
	}, tlsHostKeyRefusal);

	if (endpoint.has_value() == false)
	{
		return false;
	}

	m_ipcEndpointName = endpoint->name;
	m_ipcCredentialsLine = std::move(endpoint->credentialsLine);

	return true;
}



void LinuxServerProcess::removeIpcEndpoint()
{
	m_ipcCredentialsLine.clear();

	if (m_ipcEndpointName.isEmpty() == false)
	{
		m_dataManager.removeServerEndpoint(m_ipcEndpointName);
		m_ipcEndpointName.clear();
	}
}



void LinuxServerProcess::sendIpcCredentials()
{
	if (m_ipcEndpointName.isEmpty() == false && m_ipcCredentialsLine.isEmpty() == false)
	{
		// the endpoint only accepts this process, so register it before it can read the token
		m_dataManager.setServerProcessId(m_ipcEndpointName, processId());
		write(m_ipcCredentialsLine.constData(), m_ipcCredentialsLine.size());
	}

	m_ipcCredentialsLine.clear();
	closeWriteChannel();
}



bool LinuxServerProcess::hasDesktopPortalRegistry()
{
	const auto originalUserId = getuid();
	if (seteuid(m_sessionUserId) != 0)
	{
		vWarning() << "failed to impersonate session user - xdg-desktop-portal version detection likely will fail";
	}
	const auto revertUserIdGuard = qScopeGuard([=]() {
		if (seteuid(originalUserId) != 0)
		{
			vWarning() << "failed to revert session user impersonation";
		}
	});

	// Connect to the user's session D-Bus via the known socket path to
	// check for xdg-desktop-portal >= 1.20 (presence of the
	// org.freedesktop.host.portal.Registry service). The system service
	// runs as root and cannot rely on DBUS_SESSION_BUS_ADDRESS in its
	// own environment, so we connect directly.
	const auto busAddress = QStringLiteral("unix:path=/run/user/%1/bus").arg(m_sessionUserId);
	const auto portalBusName = QStringLiteral("portal-check");

	auto portalBus = QDBusConnection::connectToBus(busAddress, portalBusName);
	if (portalBus.isConnected() == false)
	{
		vCritical() << "could not connect to session bus";
		return false;
	}

	auto portalConnectionGuard = qScopeGuard([=]() { QDBusConnection::disconnectFromBus(portalBusName); });

	QDBusInterface introspectIface(
		QStringLiteral("org.freedesktop.portal.Desktop"),
		QStringLiteral("/org/freedesktop/portal/desktop"),
		QStringLiteral("org.freedesktop.DBus.Introspectable"),
		portalBus);


	if (!introspectIface.isValid())
	{
		return false;
	}

	QDBusReply<QString> reply = introspectIface.call(QStringLiteral("Introspect"));
	if (!reply.isValid())
	{
		return false;
	}

	return reply.value().contains(QStringLiteral("org.freedesktop.host.portal.Registry"));
}



void LinuxServerProcess::startInSystemdScope()
{
	// xdg-desktop-portal < 1.20 derives the app ID from the systemd unit of the caller. Instead of
	// D-Bus activation (the server would not be our child and not get the IPC token via stdin)
	// systemd-run registers a transient scope and then executes the server itself, so it keeps
	// our PID and stdin (02 §2a.1). The suffix after the app ID must be a single alphanumeric
	// token, otherwise the portal derives a wrong app ID.
	const auto systemdRun = QStandardPaths::findExecutable(QStringLiteral("systemd-run"),
														   {QStringLiteral("/usr/bin"), QStringLiteral("/bin")});
	const auto unitSuffix = Veyon::Crypto::Random::hex(IpcProtocol::EndpointRandomSize);

	if (systemdRun.isEmpty() || unitSuffix.has_value() == false)
	{
		vWarning() << "xdg-desktop-portal < 1.20 and systemd-run not available - launching server directly;"
				   << "the portal may not detect the app ID io.veyon.veyon-server and portal permissions may not work";
		QProcess::start(VeyonCore::filesystem().serverFilePath(), QStringList{});
		return;
	}

	vDebug() << "xdg-desktop-portal < 1.20, launching server in systemd scope";

	auto environment = processEnvironment();
	environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), QStringLiteral("/run/user/%1").arg(m_sessionUserId));
	environment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
					   QStringLiteral("unix:path=/run/user/%1/bus").arg(m_sessionUserId));
	setProcessEnvironment(environment);

	const auto unitName = QStringLiteral("app-io.veyon.veyon\\x2dserver-s%1%2").arg(m_sessionId).arg(*unitSuffix);

	QProcess::start(systemdRun, {QStringLiteral("--user"), QStringLiteral("--scope"), QStringLiteral("--collect"), QStringLiteral("--unit=%1").arg(unitName), QStringLiteral("--"), VeyonCore::filesystem().serverFilePath()});
}



QString LinuxServerProcess::tlsHostKeyRefusal(bool runAsSessionUser)
{
	// a session-user server makes itself non-dumpable only in main(), so a process of the session
	// user may attach right after execve() (T-PROC-02, 02-ipc.md §1a.4) unless Yama restricts
	// ptrace or the kernel makes the process non-dumpable already at execve()
	if (runAsSessionUser == false || isPtraceRestricted())
	{
		return {};
	}

	const auto nonDumpableFromExecRefusal = nonDumpableFromExecFailure();
	if (nonDumpableFromExecRefusal.isEmpty())
	{
		return {};
	}

	return QStringLiteral("processes of the session user can attach to veyon-server during start-up "
						  "(kernel.yama.ptrace_scope is 0 and %1) - set kernel.yama.ptrace_scope >= 1 "
						  "or install veyon-server with mode 0711")
		.arg(nonDumpableFromExecRefusal);
}



bool LinuxServerProcess::isPtraceRestricted()
{
	QFile ptraceScopeFile(QStringLiteral("/proc/sys/kernel/yama/ptrace_scope"));
	return ptraceScopeFile.open(QFile::ReadOnly) && ptraceScopeFile.readAll().trimmed().toInt() >= 1;
}



QString LinuxServerProcess::nonDumpableFromExecFailure()
{
	// execve() of a binary the user cannot read sets the dumpable flag to fs.suid_dumpable, which
	// keeps the process non-dumpable unless it is 1
	const auto serverFilePath = VeyonCore::filesystem().serverFilePath();
	struct stat status{};
	if (stat(QFile::encodeName(serverFilePath).constData(), &status) != 0 || status.st_uid != 0 ||
		(status.st_mode & (S_IRGRP | S_IROTH)) != 0)
	{
		return QStringLiteral("%1 is readable by the session user").arg(serverFilePath);
	}

	QFile suidDumpableFile(QStringLiteral("/proc/sys/fs/suid_dumpable"));
	if (suidDumpableFile.open(QFile::ReadOnly) == false || suidDumpableFile.readAll().trimmed() == QByteArrayLiteral("1"))
	{
		return QStringLiteral("fs.suid_dumpable is 1");
	}

	return {};
}
