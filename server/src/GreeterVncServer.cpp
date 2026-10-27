/*
 * GreeterVncServer.cpp - isolated greeter capture backend supervision
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDebug>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QProcess>
#include <QTcpSocket>

#ifdef Q_OS_LINUX
#include <csignal>
#include <sys/prctl.h>
#include <unistd.h>
#endif

#include "GreeterVncServer.h"
#include "VncClientProtocol.h"

namespace
{
class BackendProcess : public QProcess
{
public:
	BackendProcess()
	{
#if defined(Q_OS_LINUX) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		setChildProcessModifier([this] { prepareChild(); });
#endif
	}

	void stop()
	{
#ifdef Q_OS_LINUX
		if (m_processGroup > 0)
		{
			// Also terminate an external x11vnc spawned by the capture plugin.
			::kill(-m_processGroup, SIGKILL);
			m_processGroup = 0;
		}
#endif
		if (state() != QProcess::NotRunning)
		{
			kill();
			waitForFinished(1000);
		}
	}

	void rememberProcessGroup()
	{
#ifdef Q_OS_LINUX
		m_processGroup = pid_t(processId());
#endif
	}

private:
#ifdef Q_OS_LINUX
	void prepareChild()
	{
		if (::setpgid(0, 0) != 0 || ::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != m_supervisorPid)
		{
			::_exit(127);
		}
	}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	void setupChildProcess() override { prepareChild(); }
#endif
	const pid_t m_supervisorPid{::getpid()};
	pid_t m_processGroup{0};
#endif
};
}

QList<GreeterVncServer::Backend> GreeterVncServer::backendsForSession(const QString& sessionType)
{
	if (sessionType.compare(QLatin1String("x11"), Qt::CaseInsensitive) == 0)
	{
		return {Backend::X11, Backend::Headless};
	}
	// Desktop portals belong to a logged-in desktop, not to the greeter.
	return {Backend::Headless};
}

QString GreeterVncServer::backendName(Backend backend)
{
	return backend == Backend::X11 ? QStringLiteral("x11") : QStringLiteral("headless");
}

bool GreeterVncServer::runForSession(const QString& program, const QString& sessionType, quint16 port,
									const QByteArray& password, int startupTimeout)
{
	bool wasReady = false;
	for (const auto backend : backendsForSession(sessionType))
	{
		if (backend == Backend::Headless)
		{
			qWarning() << "Greeter capture unavailable; serving a placeholder while keeping remote login available";
		}
		wasReady = run(program, backend, port, password, startupTimeout) || wasReady;
	}
	return wasReady;
}

bool GreeterVncServer::run(const QString& program, Backend backend, quint16 port,
						   const QByteArray& password, int startupTimeout)
{
	BackendProcess process;
	process.setProcessChannelMode(QProcess::ForwardedChannels);
	process.start(program, {QStringLiteral("--greeter-vnc"), backendName(backend), QString::number(port)});
	if (!process.waitForStarted(startupTimeout))
	{
		qWarning() << "Greeter backend failed to start:" << backendName(backend) << process.errorString();
		process.stop();
		return false;
	}
	process.rememberProcessGroup();
	// Never put the internal VNC password in argv, environment or a file.
	process.write(password);
	process.closeWriteChannel();

	QElapsedTimer elapsed;
	elapsed.start();
	bool ready = false;
	while (elapsed.elapsed() < startupTimeout)
	{
		QTcpSocket probe;
		probe.connectToHost(QHostAddress::LocalHost, port);
		if (probe.waitForConnected(50))
		{
			VncClientProtocol protocol(&probe, CryptoCore::SecureArray(password));
			protocol.start();
			bool challenged = false;
			while (probe.state() == QAbstractSocket::ConnectedState && elapsed.elapsed() < startupTimeout)
			{
				while (protocol.read())
				{
					challenged |= protocol.state() == VncClientProtocol::SecurityChallenge;
					if (protocol.state() == VncClientProtocol::Running)
					{
						// A banner or an unauthenticated RFB listener is not proof
						// that our password-protected backend owns this port.
						ready = challenged;
						break;
					}
				}
				probe.flush();
				if (ready || protocol.state() == VncClientProtocol::Running)
				{
					break;
				}
				probe.waitForReadyRead(50);
			}
			if (ready)
			{
				break;
			}
		}
		if (process.waitForFinished(50))
		{
			break;
		}
	}

	if (ready)
	{
		qInfo() << "Greeter backend ready:" << backendName(backend) << "PID" << process.processId();
		process.waitForFinished(-1);
	}
	else
	{
		qWarning() << "Greeter backend did not become ready:" << backendName(backend);
	}
	process.stop();
	qWarning() << "Greeter backend stopped:" << backendName(backend)
			   << "exit code" << process.exitCode() << "status" << process.exitStatus();
	return ready;
}
