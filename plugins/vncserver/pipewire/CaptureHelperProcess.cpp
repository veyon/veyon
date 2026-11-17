// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "CaptureHelperProcess.h"
#include "IpcServer.h"
#include "VeyonCore.h"

#include <algorithm>
#include <csignal>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>


CaptureHelperProcess::CaptureHelperProcess(const QString& helperFilePath, CaptureHelperCredentials&& credentials,
										   QObject* parent) :
	QProcess(parent),
	m_helperFilePath(helperFilePath),
	m_credentials(std::move(credentials)),
	m_socketPath(socketPath(m_credentials.socketName())),
	m_parentProcessId(getpid())
{
	m_restartTimer.setSingleShot(true);
	connect(&m_restartTimer, &QTimer::timeout, this, &CaptureHelperProcess::startHelper);

	connect(this, &QProcess::started, this, &CaptureHelperProcess::sendCredentials);
	connect(this, &QProcess::finished, this, [this]() { handleExit(false); });
	connect(this, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
		// no finished() signal follows in this case
		if (error == QProcess::FailedToStart)
		{
			handleExit(true);
		}
	});

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	// defence in depth: Qt and Veyon open their descriptors with O_CLOEXEC anyway
	setUnixProcessParameters(QProcess::UnixProcessFlag::CloseFileDescriptors);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	setChildProcessModifier([parentProcessId = m_parentProcessId]() { prepareChildProcess(parentProcessId); });
#endif
}



CaptureHelperProcess::~CaptureHelperProcess()
{
	stopSupervised();
}



QString CaptureHelperProcess::defaultHelperFilePath()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("veyon-capture-helper");
}



QString CaptureHelperProcess::socketPath(const QString& socketName)
{
	if (geteuid() == 0)
	{
		return {};
	}

	const auto directory = IpcServer::workerEndpointDirectory();
	if (directory.isEmpty())
	{
		return {};
	}

	return directory + QLatin1Char('/') + socketName;
}



int CaptureHelperProcess::restartDelay(int consecutiveFailures)
{
	auto delay = MinimumRestartDelay;
	for (int i = 1; i < consecutiveFailures && delay < MaximumRestartDelay; ++i)
	{
		delay *= 2;
	}

	return std::min(delay, MaximumRestartDelay);
}



bool CaptureHelperProcess::startSupervised()
{
	// the helper refuses to run as root, too
	if (geteuid() == 0 || m_socketPath.isEmpty())
	{
		vCritical() << "can't run the capture helper as root or without runtime directory";
		return false;
	}

	if (IpcServer::prepareDirectory(IpcServer::workerEndpointDirectory(), IpcServer::DirectoryType::Private) == false)
	{
		return false;
	}

	removeStaleSockets();

	m_supervised = true;
	m_consecutiveFailures = 0;

	startHelper();

	return true;
}



void CaptureHelperProcess::stopSupervised()
{
	m_supervised = false;
	m_restartTimer.stop();

	if (state() != QProcess::NotRunning)
	{
		terminate();
		if (waitForFinished(MinimumRestartDelay) == false)
		{
			kill();
			waitForFinished(MinimumRestartDelay);
		}
	}

	removeSocket();
}



void CaptureHelperProcess::removeSocket()
{
	// a socket left behind by a previous instance would make the helper's bind() fail; only our
	// own sockets are removed (a foreign file can't exist in the private directory anyway)
	struct stat status{};
	const auto encodedPath = QFile::encodeName(m_socketPath);
	if (m_socketPath.isEmpty() == false && lstat(encodedPath.constData(), &status) == 0 && S_ISSOCK(status.st_mode) &&
		status.st_uid == geteuid())
	{
		unlink(encodedPath.constData());
	}
}



void CaptureHelperProcess::removeStaleSockets()
{
	// neither the server nor the helper can clean up when being killed by a signal (e.g. when
	// the service stops the server), so remove sockets of previous instances for the same
	// session - there's only one server per session, sockets of other sessions are left alone
	const auto socketName = m_credentials.socketName();
	const auto sessionPrefix = socketName.left(socketName.lastIndexOf(QLatin1Char('-')) + 1);
	const QDir directory(IpcServer::workerEndpointDirectory());

	const auto entries = directory.entryList({sessionPrefix + QStringLiteral("*.sock")}, QDir::System | QDir::Hidden);
	for (const auto& entry : entries)
	{
		if (entry == socketName || CaptureHelperCredentials::isValidSocketName(entry) == false)
		{
			continue;
		}

		struct stat status{};
		const auto encodedPath = QFile::encodeName(directory.filePath(entry));
		if (lstat(encodedPath.constData(), &status) == 0 && S_ISSOCK(status.st_mode) && status.st_uid == geteuid())
		{
			vDebug() << "removing stale socket" << entry;
			unlink(encodedPath.constData());
		}
	}
}



void CaptureHelperProcess::startHelper()
{
	if (m_supervised == false || state() != QProcess::NotRunning)
	{
		return;
	}

	vDebug() << "starting" << m_helperFilePath;

	removeSocket();

	m_runTime.start();
	// the helper inherits the environment of the server (session bus, Wayland display, session ID)
	// and determines its RFB port from the configuration like VncServer::serverPort()
	start(m_helperFilePath, QStringList{});
}



void CaptureHelperProcess::sendCredentials()
{
	const auto line = m_credentials.toLine();
	if (line.isEmpty() || write(line.constData(), line.size()) != line.size())
	{
		vCritical() << "failed to pass the VNC password to the capture helper";
	}

	// EOF right after the line: the helper exits on anything else
	closeWriteChannel();
}



void CaptureHelperProcess::handleExit(bool failedToStart)
{
	if (m_supervised == false)
	{
		return;
	}

	if (m_runTime.isValid() && m_runTime.elapsed() >= StableRunTime)
	{
		m_consecutiveFailures = 0;
	}
	++m_consecutiveFailures;

	const auto delay = restartDelay(m_consecutiveFailures);

	if (failedToStart)
	{
		vWarning() << "failed to start capture helper" << m_helperFilePath << "- retrying in" << delay << "ms";
	}
	else
	{
		vWarning() << "capture helper exited with code" << exitCode() << exitStatus() << "- restarting in" << delay << "ms";
	}

	m_restartTimer.start(delay);
}



void CaptureHelperProcess::prepareChildProcess(pid_t parentProcessId)
{
	// only async-signal-safe calls between fork() and execve()
	if (prctl(PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0) != 0 || getppid() != parentProcessId)
	{
		_exit(127);
	}
}
