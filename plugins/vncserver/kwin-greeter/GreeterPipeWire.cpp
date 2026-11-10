/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "GreeterPipeWire.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

GreeterPipeWireProcess::GreeterPipeWireProcess() : m_parentPid(::getpid())
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	setChildProcessModifier([this]() { prepareChild(); });
#endif
}

void GreeterPipeWireProcess::prepareChild()
{
	if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != m_parentPid) { ::_exit(127); }
}

GreeterPipeWire::GreeterPipeWire(QObject* parent) : QObject(parent)
{
	m_probe.setInterval(50);
	m_deadline.setSingleShot(true);
	connect(&m_probe, &QTimer::timeout, this, &GreeterPipeWire::probe);
	connect(&m_deadline, &QTimer::timeout, this, [this]() { fail(QStringLiteral("Greeter PipeWire startup timed out")); });
	connect(&m_daemon, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
		fail(QStringLiteral("Greeter PipeWire daemon failed: ") + m_daemon.errorString());
	});
	connect(&m_daemon, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this]() {
		fail(QStringLiteral("Greeter PipeWire daemon exited"));
	});
	m_daemon.setProcessChannelMode(QProcess::ForwardedChannels);
}

GreeterPipeWire::~GreeterPipeWire()
{
	m_daemon.disconnect(this);
	if (m_daemon.state() != QProcess::NotRunning)
	{
		m_daemon.terminate();
		if (!m_daemon.waitForFinished(500)) { m_daemon.kill(); m_daemon.waitForFinished(500); }
	}
}

int GreeterPipeWire::connectSocket(const QString& path, uint expectedUid)
{
	const auto bytes = QFile::encodeName(path);
	sockaddr_un endpoint{};
	if (!QDir::isAbsolutePath(path) || bytes.contains('\0') || bytes.size() >= qsizetype(sizeof(endpoint.sun_path)))
	{
		errno = EINVAL;
		return -1;
	}
	endpoint.sun_family = AF_UNIX;
	std::memcpy(endpoint.sun_path, bytes.constData(), size_t(bytes.size()));
	const auto fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (fd < 0) { return -1; }
	bool keep = false;
	const auto guard = qScopeGuard([&]() { if (!keep) { const auto error = errno; ::close(fd); errno = error; } });
	if (::connect(fd, reinterpret_cast<const sockaddr*>(&endpoint),
		socklen_t(offsetof(sockaddr_un, sun_path) + size_t(bytes.size()) + 1)) != 0)
	{
		if (errno != EINPROGRESS && errno != EAGAIN) { return -1; }
		pollfd pending{fd, POLLOUT, 0};
		if (::poll(&pending, 1, 200) <= 0) { errno = ETIMEDOUT; return -1; }
		int error = 0;
		socklen_t size = sizeof(error);
		if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) != 0) { return -1; }
		if (error != 0) { errno = error; return -1; }
	}
	ucred peer{};
	socklen_t size = sizeof(peer);
	if (::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) != 0 ||
		size != sizeof(peer) || peer.uid != expectedUid)
	{
		errno = EACCES;
		return -1;
	}
	const auto flags = ::fcntl(fd, F_GETFL);
	if (flags < 0 || ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) != 0) { return -1; }
	keep = true;
	return fd;
}

void GreeterPipeWire::start()
{
	if (::getuid() == 0)
	{
		fail(QStringLiteral("Native greeter capture must run under the greeter account"));
		return;
	}
	const auto runtime = qEnvironmentVariable("PIPEWIRE_RUNTIME_DIR", qEnvironmentVariable("XDG_RUNTIME_DIR"));
	const auto remote = qEnvironmentVariable("PIPEWIRE_REMOTE", QStringLiteral("pipewire-0"));
	struct stat directory{};
	if (!QDir::isAbsolutePath(runtime) || ::lstat(QFile::encodeName(runtime).constData(), &directory) != 0 ||
		!S_ISDIR(directory.st_mode) || directory.st_uid != ::getuid() || (directory.st_mode & 0077) != 0 ||
		remote.isEmpty() || remote.contains(QLatin1Char('/')) || remote.contains(QChar::Null) ||
		remote == QLatin1String(".") || remote == QLatin1String(".."))
	{
		fail(QStringLiteral("Invalid or untrusted greeter PipeWire runtime directory/socket name"));
		return;
	}
	m_socketPath = QDir(runtime).filePath(remote);
	const auto fd = connectSocket(m_socketPath, uint(::getuid()));
	if (fd >= 0) { m_ready = true; Q_EMIT ready(fd); return; }
	if (errno != ENOENT && errno != ECONNREFUSED)
	{
		fail(QStringLiteral("Cannot connect to the greeter-owned PipeWire socket"));
		return;
	}
	// KWin connects to this same socket. A private alternate socket would not
	// contain its producer. Reuse any existing daemon and never unlink sockets.
	auto environment = QProcessEnvironment::systemEnvironment();
	for (const auto& name : {"PIPEWIRE_CONFIG_DIR", "PIPEWIRE_CONFIG_PREFIX", "PIPEWIRE_CONFIG_NAME", "PIPEWIRE_NO_CONFIG"})
	{
		environment.remove(QString::fromLatin1(name));
	}
	environment.insert(QStringLiteral("PIPEWIRE_RUNTIME_DIR"), runtime);
	environment.insert(QStringLiteral("PIPEWIRE_CORE"), remote);
	m_daemon.setProcessEnvironment(environment);
	m_daemon.start(QStringLiteral(VEYON_PIPEWIRE_EXECUTABLE),
		{QStringLiteral("-c"), QStringLiteral(VEYON_GREETER_PIPEWIRE_CONFIG)});
	m_deadline.start(3000);
	m_probe.start();
}

void GreeterPipeWire::probe()
{
	if (m_failed || m_ready) { return; }
	const auto fd = connectSocket(m_socketPath, uint(::getuid()));
	if (fd >= 0)
	{
		m_ready = true;
		m_probe.stop();
		m_deadline.stop();
		Q_EMIT ready(fd);
	}
	else if (errno != ENOENT && errno != ECONNREFUSED)
	{
		fail(QStringLiteral("Greeter PipeWire socket is unavailable or belongs to another account"));
	}
}

void GreeterPipeWire::fail(const QString& error)
{
	if (m_failed) { return; }
	m_failed = true;
	m_probe.stop();
	m_deadline.stop();
	Q_EMIT failed(error);
}
