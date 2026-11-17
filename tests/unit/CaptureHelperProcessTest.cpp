// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

#include "CaptureHelperProcess.h"
#include "Random.h"
#include "VncClientProtocol.h"
#include "VncServerSocket.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <memory>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using Veyon::Crypto::SecureBuffer;

// veyon-capture-helper and its supervision in veyon-server (02-ipc.md §1a.5): the helper gets the
// stdin line "VEYONCAP1 <hex64> <socketName>" and nothing else, is restarted when it exits,
// listens on a Unix domain socket only and accepts peers of its own uid only, authenticates
// veyon-server with the per-start password and stays dumpable (the portal requires it) while the
// hardened server that started it stays non-dumpable. veyon-server (VncServerSocket) only talks
// to the expected listener process and never without VNC authentication.
//
// The helper refuses to run as root: when started as root, everything after helperRefusesRoot()
// runs as nobody (the test functions run in declaration order).
class CaptureHelperProcessTest : public QObject
{
	Q_OBJECT
private:
	// VncClientProtocol keeps the password in a QCA::SecureArray
	QCA::Initializer m_qcaInitializer;

	static constexpr int HelperListenTimeout = 15000;
	static constexpr int HelperExitTimeout = 15000;
	// main() returns -1
	static constexpr int HelperFailureExitCode = 255;
	static constexpr uid_t UnprivilegedId = 65534;

	enum class AuthenticationResult {
		Authenticated,
		Rejected, // after the security result
		Failed,	  // before: refused listener, protocol error, timeout
	};

	struct RealHelper
	{
		QTemporaryDir directory;
		QByteArray password;
		QString socketPath;
		std::unique_ptr<CaptureHelperProcess> process;

		qint64 processId() const
		{
			return process->processId();
		}
	};

	// root process forked before dropping privileges: connects to a given socket path as a
	// different uid than the helper and reports what it receives
	static inline pid_t s_foreignConnectorPid = -1;
	static inline int s_foreignConnectorRequest = -1;
	static inline int s_foreignConnectorReply = -1;

	static CaptureHelperCredentials randomCredentials()
	{
		auto password = CaptureHelperCredentials::generatePassword();
		const auto socketName = CaptureHelperCredentials::generateSocketName(0);
		if (password.has_value() == false || socketName.has_value() == false)
		{
			qFatal("no random credentials");
		}
		auto credentials = CaptureHelperCredentials::create(std::move(*password), *socketName);
		if (credentials.has_value() == false)
		{
			qFatal("invalid random credentials");
		}
		return std::move(*credentials);
	}

	// keeps the real helper away from the session bus, the user's portal restore token and a
	// display of the build host; the socket directory is <directory>/veyon
	static QProcessEnvironment isolatedHelperEnvironment(const QString& directory)
	{
		auto environment = QProcessEnvironment::systemEnvironment();
		environment.remove(QStringLiteral("WAYLAND_DISPLAY"));
		environment.remove(QStringLiteral("DISPLAY"));
		environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		environment.insert(QStringLiteral("HOME"), directory);
		environment.insert(QStringLiteral("XDG_STATE_HOME"), directory);
		environment.insert(QStringLiteral("XDG_DATA_HOME"), directory);
		environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), directory);
		environment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
						   QStringLiteral("unix:path=%1/no-bus").arg(directory));
		return environment;
	}

	static QSet<QByteArray> socketInodes(qint64 pid)
	{
		QSet<QByteArray> inodes;
		const QDir fdDirectory(QStringLiteral("/proc/%1/fd").arg(pid));
		const auto entries = fdDirectory.entryList(QDir::Files | QDir::System);
		for (const auto& entry : entries)
		{
			// QFile::symLinkTarget() would make the relative target "socket:[<inode>]" absolute
			char target[64]{};
			const auto targetSize =
				readlink(QFile::encodeName(fdDirectory.filePath(entry)).constData(), target, sizeof(target) - 1);
			const auto link = QByteArray(target, targetSize > 0 ? int(targetSize) : 0);
			const QByteArray prefix("socket:[");
			if (link.startsWith(prefix) && link.endsWith(']'))
			{
				inodes.insert(link.mid(prefix.size(), link.size() - prefix.size() - 1));
			}
		}
		return inodes;
	}

	// TCP and UDP sockets (any state, IPv4 and IPv6) of the process
	static int inetSocketCount(qint64 pid)
	{
		const auto inodes = socketInodes(pid);
		int count = 0;
		for (const auto& table : {QStringLiteral("/proc/net/tcp"), QStringLiteral("/proc/net/tcp6"),
								  QStringLiteral("/proc/net/udp"), QStringLiteral("/proc/net/udp6")})
		{
			QFile file(table);
			if (file.open(QFile::ReadOnly) == false)
			{
				continue;
			}
			file.readLine(); // header
			while (file.atEnd() == false)
			{
				const auto fields = file.readLine().simplified().split(' ');
				if (fields.size() > 9 && inodes.contains(fields[9]))
				{
					++count;
				}
			}
		}
		return count;
	}

	// "Num RefCount Protocol Flags Type St Inode Path", Flags __SO_ACCEPTCON (0x10000) = listening
	static bool isListeningOnUnixSocket(qint64 pid, const QString& path)
	{
		static constexpr auto AcceptConnectionsFlag = 0x10000;

		const auto inodes = socketInodes(pid);
		QFile file(QStringLiteral("/proc/net/unix"));
		if (file.open(QFile::ReadOnly) == false)
		{
			return false;
		}
		file.readLine(); // header
		while (file.atEnd() == false)
		{
			const auto fields = file.readLine().simplified().split(' ');
			if (fields.size() >= 8 && inodes.contains(fields[6]) && QString::fromLocal8Bit(fields[7]) == path &&
				(fields[3].toUInt(nullptr, 16) & AcceptConnectionsFlag))
			{
				return true;
			}
		}
		return false;
	}

	static bool startRealHelper(RealHelper& helper)
	{
		if (helper.directory.isValid() == false)
		{
			return false;
		}

		// CaptureHelperProcess uses the same private directory as the helper
		qputenv("XDG_RUNTIME_DIR", QFile::encodeName(helper.directory.path()));

		auto credentials = randomCredentials();
		helper.password = credentials.password().toUnprotectedByteArray();
		helper.socketPath = CaptureHelperProcess::socketPath(credentials.socketName());
		helper.process = std::make_unique<CaptureHelperProcess>(QStringLiteral(CAPTURE_HELPER_FILE_PATH),
																std::move(credentials));
		helper.process->setProcessEnvironment(isolatedHelperEnvironment(helper.directory.path()));
		if (helper.socketPath.isEmpty() || helper.process->startSupervised() == false)
		{
			return false;
		}

		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < HelperListenTimeout)
		{
			if (helper.process->state() == QProcess::Running &&
				isListeningOnUnixSocket(helper.processId(), helper.socketPath))
			{
				return true;
			}
			QTest::qWait(50);
		}
		return false;
	}

	// the endpoint veyon-server uses for its capture helper (VncServer::endpoint())
	static VncServerEndpoint serverEndpoint(const RealHelper& helper)
	{
		VncServerEndpoint endpoint;
		endpoint.localSocketPath = helper.socketPath;
		const auto* process = helper.process.get();
		endpoint.expectedProcessId = [process]() { return process->processId(); };
		return endpoint;
	}

	// RFB handshake exactly like VncProxyConnection (VncServerSocket, VncClientProtocol with
	// required VNC authentication)
	static AuthenticationResult authenticate(const VncServerEndpoint& endpoint, const QByteArray& password,
											 int timeout = 5000)
	{
		VncServerSocket socket(endpoint);
		VncClientProtocol protocol(socket.device(), VncClientProtocol::Password(password));
		protocol.setVncAuthenticationRequired(true);

		bool disconnected = false;
		QObject::connect(&socket, &VncServerSocket::disconnected, [&disconnected]() { disconnected = true; });

		socket.connectToServer();
		protocol.start();

		bool wasOpen = false;
		bool securityResultReached = false;
		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < timeout)
		{
			while (protocol.read())
			{
				securityResultReached |= protocol.state() >= VncClientProtocol::SecurityResult;
			}
			if (protocol.state() == VncClientProtocol::Running)
			{
				return AuthenticationResult::Authenticated;
			}
			wasOpen |= socket.device()->isOpen();
			if (disconnected || (wasOpen && socket.device()->isOpen() == false))
			{
				return securityResultReached ? AuthenticationResult::Rejected : AuthenticationResult::Failed;
			}
			QTest::qWait(10);
		}
		return AuthenticationResult::Failed;
	}

	static int connectRawSocket(const QString& path, bool nonBlocking)
	{
		const auto encodedPath = QFile::encodeName(path);
		sockaddr_un address{};
		address.sun_family = AF_UNIX;
		std::memcpy(address.sun_path, encodedPath.constData(),
					std::min(size_t(encodedPath.size()), sizeof(address.sun_path) - 1));
		const auto fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | (nonBlocking ? SOCK_NONBLOCK : 0), 0);
		if (fd < 0)
		{
			return -1;
		}
		if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 && errno != EINPROGRESS &&
			errno != EAGAIN)
		{
			close(fd);
			return -1;
		}
		return fd;
	}

	// drops CAP_SYS_PTRACE from this thread and from processes started afterwards, so that
	// /proc/<pid>/mem is subject to the dumpable flag also when the test runs as root
	static bool dropPtraceCapability()
	{
		if (prctl(PR_CAPBSET_READ, CAP_SYS_PTRACE, 0, 0, 0) == 1 && geteuid() == 0 &&
			prctl(PR_CAPBSET_DROP, CAP_SYS_PTRACE, 0, 0, 0) != 0)
		{
			return false;
		}

		__user_cap_header_struct header{_LINUX_CAPABILITY_VERSION_3, 0};
		__user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3]{};
		if (syscall(SYS_capget, &header, data) != 0)
		{
			return false;
		}
		const auto ptraceMask = ~(quint32(1) << CAP_SYS_PTRACE);
		data[0].effective &= ptraceMask;
		data[0].permitted &= ptraceMask;
		data[0].inheritable &= ptraceMask;
		return syscall(SYS_capset, &header, data) == 0;
	}

	// connection attempts of another uid (root): 'D' = got data from the helper, 'C' = closed
	// without data, 'T' = timeout, 'E' = connect failed
	static void runForeignConnector(int requestFd, int replyFd)
	{
		char path[sizeof(sockaddr_un::sun_path)];
		while (true)
		{
			size_t length = 0;
			char character = 0;
			while (length < sizeof(path) - 1 && read(requestFd, &character, 1) == 1 && character != '\n')
			{
				path[length++] = character;
			}
			if (length == 0)
			{
				_exit(0);
			}
			path[length] = 0;

			char reply = 'E';
			sockaddr_un address{};
			address.sun_family = AF_UNIX;
			std::memcpy(address.sun_path, path, length);
			const auto fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (fd >= 0 && ::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0)
			{
				pollfd pollData{fd, POLLIN, 0};
				reply = 'T';
				if (poll(&pollData, 1, 3000) == 1)
				{
					char data = 0;
					reply = read(fd, &data, 1) == 1 ? 'D' : 'C';
				}
			}
			if (fd >= 0)
			{
				close(fd);
			}
			if (write(replyFd, &reply, 1) != 1)
			{
				_exit(1);
			}
		}
	}

	static char askForeignConnector(const QString& path)
	{
		const QByteArray request = QFile::encodeName(path) + '\n';
		char reply = 0;
		if (write(s_foreignConnectorRequest, request.constData(), size_t(request.size())) != request.size() ||
			read(s_foreignConnectorReply, &reply, 1) != 1)
		{
			return 0;
		}
		return reply;
	}

	// permanently switches to nobody when running as root, after forking the foreign connector
	// and dropping CAP_SYS_PTRACE from the bounding set
	static bool runAsUnprivilegedUser()
	{
		if (geteuid() == 0)
		{
			int requestPipe[2];
			int replyPipe[2];
			if (pipe2(requestPipe, O_CLOEXEC) != 0 || pipe2(replyPipe, O_CLOEXEC) != 0)
			{
				return false;
			}
			s_foreignConnectorPid = fork();
			if (s_foreignConnectorPid == 0)
			{
				close(requestPipe[1]);
				close(replyPipe[0]);
				runForeignConnector(requestPipe[0], replyPipe[1]);
			}
			close(requestPipe[0]);
			close(replyPipe[1]);
			s_foreignConnectorRequest = requestPipe[1];
			s_foreignConnectorReply = replyPipe[0];

			if (dropPtraceCapability() == false || setgroups(0, nullptr) != 0 || setgid(UnprivilegedId) != 0 ||
				setuid(UnprivilegedId) != 0)
			{
				return false;
			}
		}

		return geteuid() != 0 && access(CAPTURE_HELPER_FILE_PATH, X_OK) == 0;
	}

	static int openProcessMemory(qint64 pid)
	{
		const QByteArray path = QByteArray("/proc/") + QByteArray::number(pid) + "/mem";
		const auto fd = open(path.constData(), O_RDONLY | O_CLOEXEC);
		if (fd >= 0)
		{
			close(fd);
			return 0;
		}
		return errno;
	}

	// forked child (without exec) which sets its dumpable flag and waits until it is killed
	static pid_t startChild(bool dumpable)
	{
		int readyPipe[2];
		if (pipe(readyPipe) != 0)
		{
			return -1;
		}
		const auto pid = fork();
		if (pid == 0)
		{
			close(readyPipe[0]);
			const char status = prctl(PR_SET_DUMPABLE, dumpable ? 1 : 0, 0, 0, 0) == 0 ? '1' : '0';
			if (write(readyPipe[1], &status, 1) != 1)
			{
				_exit(1);
			}
			while (true)
			{
				pause();
			}
		}
		close(readyPipe[1]);
		char status = 0;
		const auto ready = pid > 0 && read(readyPipe[0], &status, 1) == 1 && status == '1';
		close(readyPipe[0]);
		if (ready == false && pid > 0)
		{
			kill(pid, SIGKILL);
			waitpid(pid, nullptr, 0);
			return -1;
		}
		return pid;
	}

	static void stopChild(pid_t pid)
	{
		kill(pid, SIGKILL);
		waitpid(pid, nullptr, 0);
	}

	// runs the real helper with the given stdin data and returns its exit code (-1 on timeout)
	static int runHelperWithInput(const QByteArray& input, bool closeInput)
	{
		QTemporaryDir directory;
		QProcess helper;
		helper.setProcessEnvironment(isolatedHelperEnvironment(directory.path()));
		helper.start(QStringLiteral(CAPTURE_HELPER_FILE_PATH), QStringList{});
		if (helper.waitForStarted() == false)
		{
			return -1;
		}
		helper.write(input);
		if (closeInput)
		{
			helper.closeWriteChannel();
		}
		if (helper.waitForFinished(HelperExitTimeout) == false)
		{
			helper.kill();
			helper.waitForFinished();
			return -1;
		}
		return helper.exitStatus() == QProcess::NormalExit ? helper.exitCode() : -1;
	}

	static QString writeScript(const QTemporaryDir& directory, const QString& name, const QByteArray& content)
	{
		const auto scriptPath = directory.filePath(name);
		QFile script(scriptPath);
		if (script.open(QFile::WriteOnly) == false || script.write(content) != content.size())
		{
			return {};
		}
		script.close();
		script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
		return scriptPath;
	}

private Q_SLOTS:
	void cleanupTestCase()
	{
		if (s_foreignConnectorPid > 0)
		{
			close(s_foreignConnectorRequest);
			close(s_foreignConnectorReply);
			waitpid(s_foreignConnectorPid, nullptr, 0);
		}
	}

	void restartDelay()
	{
		QCOMPARE(CaptureHelperProcess::restartDelay(1), 1000);
		QCOMPARE(CaptureHelperProcess::restartDelay(2), 2000);
		QCOMPARE(CaptureHelperProcess::restartDelay(3), 4000);
		QCOMPARE(CaptureHelperProcess::restartDelay(5), 16000);
		QCOMPARE(CaptureHelperProcess::restartDelay(6), 30000);
		QCOMPARE(CaptureHelperProcess::restartDelay(1000), 30000);
	}

	// debug launch paths run veyon-server and thus the helper as root
	void helperRefusesRoot()
	{
		if (geteuid() != 0)
		{
			QSKIP("not running as root");
		}
		QCOMPARE(runHelperWithInput(randomCredentials().toLine().toUnprotectedByteArray(), true), HelperFailureExitCode);

		// the server side refuses as well and never uses /run/veyon for the socket
		QVERIFY(CaptureHelperProcess::socketPath(QStringLiteral("capture-0-01234567.sock")).isEmpty());
		CaptureHelperProcess helperProcess(QStringLiteral(CAPTURE_HELPER_FILE_PATH), randomCredentials());
		QVERIFY(helperProcess.startSupervised() == false);
	}

	// a fake helper records its stdin and arguments, then exits: the supervisor passes exactly the
	// credentials line followed by EOF and restarts the helper after the minimum delay
	void passesOnlyCredentialsLineAndRestarts()
	{
		QVERIFY(runAsUnprivilegedUser());

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		qputenv("XDG_RUNTIME_DIR", QFile::encodeName(directory.path()));

		const auto scriptPath =
			writeScript(directory, QStringLiteral("fake-capture-helper"),
						"#!/bin/sh\n"
						"directory=$(dirname \"$0\")\n"
						"echo $# > \"$directory/arguments-$$\"\n"
						// only complete reads count (the last instance may be stopped while reading)
						"cat > \"$directory/partial-$$\" && mv \"$directory/partial-$$\" \"$directory/stdin-$$\"\n"
						"exit 3\n");
		QVERIFY(scriptPath.isEmpty() == false);

		auto credentials = randomCredentials();
		const auto expectedLine = credentials.toLine().toUnprotectedByteArray();

		CaptureHelperProcess helperProcess(scriptPath, std::move(credentials));
		QSignalSpy startedSpy(&helperProcess, &QProcess::started);
		QVERIFY(helperProcess.startSupervised());

		// the private socket directory has been prepared
		QFileInfo socketDirectory(directory.filePath(QStringLiteral("veyon")));
		QVERIFY(socketDirectory.isDir());
		QCOMPARE(socketDirectory.permissions() & 0x7777, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);

		// first start immediately, the restart after MinimumRestartDelay
		QTRY_VERIFY_WITH_TIMEOUT(startedSpy.count() >= 2, 5000);
		QTRY_VERIFY_WITH_TIMEOUT(QDir(directory.path()).entryList({QStringLiteral("stdin-*")}).size() >= 2, 5000);
		QVERIFY(helperProcess.consecutiveFailures() >= 1);

		helperProcess.stopSupervised();
		const auto startCount = startedSpy.count();

		const auto stdinFiles = QDir(directory.path()).entryList({QStringLiteral("stdin-*")});
		for (const auto& stdinFile : stdinFiles)
		{
			QFile file(directory.filePath(stdinFile));
			QVERIFY(file.open(QFile::ReadOnly));
			QCOMPARE(file.readAll(), expectedLine);
		}
		const auto argumentFiles = QDir(directory.path()).entryList({QStringLiteral("arguments-*")});
		for (const auto& argumentFile : argumentFiles)
		{
			QFile file(directory.filePath(argumentFile));
			QVERIFY(file.open(QFile::ReadOnly));
			QCOMPARE(file.readAll().trimmed(), QByteArray("0"));
		}

		// no restarts after stopping the supervision
		QTest::qWait(CaptureHelperProcess::MinimumRestartDelay * 2);
		QCOMPARE(startedSpy.count(), startCount);
	}

	// the helper must not outlive the server: PR_SET_PDEATHSIG fires when the forking thread - in
	// veyon-server the VncServer thread - exits
	void helperDiesWithServer()
	{
		QVERIFY(runAsUnprivilegedUser());

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		qputenv("XDG_RUNTIME_DIR", QFile::encodeName(directory.path()));

		const auto scriptPath = writeScript(directory, QStringLiteral("sleeping-capture-helper"),
											"#!/bin/sh\nexec sleep 60\n");
		QVERIFY(scriptPath.isEmpty() == false);

		qint64 helperPid = 0;
		QThread serverThread;
		// deliberately leaked: its destructor would stop the helper itself
		CaptureHelperProcess* helperProcess = nullptr;
		QObject::connect(&serverThread, &QThread::started, [&]() {
			helperProcess = new CaptureHelperProcess(scriptPath, randomCredentials());
			if (helperProcess->startSupervised() && helperProcess->waitForStarted())
			{
				helperPid = helperProcess->processId();
			}
			serverThread.quit();
		});
		serverThread.start();
		QVERIFY(serverThread.wait(10000));
		QVERIFY(helperPid > 0);

		const auto helperIsGone = [helperPid]() {
			QFile stat(QStringLiteral("/proc/%1/stat").arg(helperPid));
			if (stat.open(QFile::ReadOnly) == false)
			{
				return true;
			}
			// "<pid> (<comm>) <state> ..." - a zombie has terminated already
			const auto fields = stat.readAll();
			const auto state = fields.mid(fields.lastIndexOf(')') + 2, 1);
			return state == "Z" || state == "X";
		};
		QTRY_VERIFY_WITH_TIMEOUT(helperIsGone(), 5000);
	}

	void failedStartIsRetried()
	{
		QVERIFY(runAsUnprivilegedUser());

		QTemporaryDir directory;
		qputenv("XDG_RUNTIME_DIR", QFile::encodeName(directory.path()));

		CaptureHelperProcess helperProcess(QStringLiteral("/nonexistent/veyon-capture-helper"), randomCredentials());
		QVERIFY(helperProcess.startSupervised());
		QTRY_VERIFY_WITH_TIMEOUT(helperProcess.consecutiveFailures() >= 2, 5000);
		helperProcess.stopSupervised();
	}

	// a server killed by a signal (e.g. stopped by the service) leaves its helper's socket behind:
	// the next start of the same session removes it, sockets of other sessions are kept
	void staleSocketsOfSessionAreRemoved()
	{
		QVERIFY(runAsUnprivilegedUser());

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		qputenv("XDG_RUNTIME_DIR", QFile::encodeName(directory.path()));
		const QDir socketDirectory(directory.filePath(QStringLiteral("veyon")));
		QVERIFY(QDir().mkdir(socketDirectory.path()));
		QVERIFY(QFile::setPermissions(socketDirectory.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

		const auto bindSocket = [&socketDirectory](const QString& name) {
			sockaddr_un address{};
			address.sun_family = AF_UNIX;
			const auto path = QFile::encodeName(socketDirectory.filePath(name));
			std::strncpy(address.sun_path, path.constData(), sizeof(address.sun_path) - 1);
			const auto fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
			const auto bound = fd >= 0 && bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
			if (fd >= 0)
			{
				close(fd);
			}
			return bound;
		};

		const auto staleSocket = QStringLiteral("capture-0-0123abcd.sock");
		const auto otherSessionSocket = QStringLiteral("capture-10-0123abcd.sock");
		const auto regularFile = QStringLiteral("capture-0-4567cdef.sock");
		QVERIFY(bindSocket(staleSocket));
		QVERIFY(bindSocket(otherSessionSocket));
		QFile file(socketDirectory.filePath(regularFile));
		QVERIFY(file.open(QFile::WriteOnly));
		file.close();

		CaptureHelperProcess helperProcess(QStringLiteral("/nonexistent/veyon-capture-helper"), randomCredentials());
		QVERIFY(helperProcess.startSupervised());
		helperProcess.stopSupervised();

		QVERIFY(socketDirectory.exists(staleSocket) == false);
		QVERIFY(socketDirectory.exists(otherSessionSocket));
		QVERIFY(socketDirectory.exists(regularFile));
	}

	// 02-ipc.md §8.13 analogue: the helper never runs without (or with broken) credentials
	void helperRejectsMissingOrMalformedCredentials()
	{
		QVERIFY(runAsUnprivilegedUser());

		const auto passwordHex = QByteArray(64, 'a');
		QCOMPARE(runHelperWithInput({}, true), HelperFailureExitCode);
		QCOMPARE(runHelperWithInput("VEYONCAP1 1234\n", true), HelperFailureExitCode);
		QCOMPARE(runHelperWithInput(QByteArray("VEYONIPC1 veyon-ipc-0-01234567 ") + passwordHex + '\n', true),
				 HelperFailureExitCode);
		// no socket name, path components in the socket name, line without newline
		QCOMPARE(runHelperWithInput(QByteArray("VEYONCAP1 ") + passwordHex + '\n', true), HelperFailureExitCode);
		QCOMPARE(runHelperWithInput(QByteArray("VEYONCAP1 ") + passwordHex + " ../capture-0-01234567.sock\n", true),
				 HelperFailureExitCode);
		QCOMPARE(runHelperWithInput(QByteArray("VEYONCAP1 ") + passwordHex + " /tmp/capture-0-01234567.sock\n", true),
				 HelperFailureExitCode);
		QCOMPARE(runHelperWithInput(QByteArray("VEYONCAP1 ") + passwordHex + " capture-0-01234567.sock", true),
				 HelperFailureExitCode);
	}

	// veyon-server makes itself non-dumpable as the first statement of main() and then starts the
	// helper through CaptureHelperProcess: the helper receives the line, listens on its Unix domain
	// socket only, accepts only the per-start password and is dumpable while the server stays
	// non-dumpable
	void realHelperFromHardenedServer()
	{
		QVERIFY(runAsUnprivilegedUser());
		QVERIFY(dropPtraceCapability());

		// controls: without CAP_SYS_PTRACE the dumpable flag decides about /proc/<pid>/mem
		const auto dumpableChild = startChild(true);
		const auto nonDumpableChild = startChild(false);
		QVERIFY(dumpableChild > 0 && nonDumpableChild > 0);
		const auto dumpableChildAccess = openProcessMemory(dumpableChild);
		const auto nonDumpableChildAccess = openProcessMemory(nonDumpableChild);
		stopChild(dumpableChild);
		stopChild(nonDumpableChild);
		if (dumpableChildAccess != 0)
		{
			QSKIP("/proc/<pid>/mem of a dumpable child is not accessible in this environment");
		}
		QVERIFY(nonDumpableChildAccess == EACCES || nonDumpableChildAccess == EPERM);

		// act like veyon-server (ProcessHardening::hardenCurrentProcess())
		QCOMPARE(prctl(PR_SET_DUMPABLE, 0, 0, 0, 0), 0);
		const auto restoreDumpable = qScopeGuard([]() { prctl(PR_SET_DUMPABLE, 1, 0, 0, 0); });

		RealHelper helper;
		QVERIFY2(startRealHelper(helper), "capture helper does not listen on its socket");
		const auto helperPid = helper.processId();

		// no TCP/UDP socket at all (F1/F3 of the WP16 review: loopback port reachable by every
		// local user, WebSockets), the Unix domain socket is private
		QCOMPARE(inetSocketCount(helperPid), 0);
		QFileInfo socketFile(helper.socketPath);
		QCOMPARE(socketFile.ownerId(), uint(geteuid()));
		QCOMPARE(socketFile.permissions() & 0x7777, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);

		// nothing but the stdin line carries the password: neither argv nor the environment
		QFile commandLine(QStringLiteral("/proc/%1/cmdline").arg(helperPid));
		QVERIFY(commandLine.open(QFile::ReadOnly));
		QCOMPARE(commandLine.readAll(), QByteArray(CAPTURE_HELPER_FILE_PATH) + '\0');
		QFile environment(QStringLiteral("/proc/%1/environ").arg(helperPid));
		QVERIFY(environment.open(QFile::ReadOnly));
		const auto environmentData = environment.readAll();
		QVERIFY(environmentData.contains(helper.password.toHex()) == false);
		QVERIFY(environmentData.contains(helper.password.toHex().toUpper()) == false);

		// the server authenticates with the per-start password, anything else is rejected
		const auto endpoint = serverEndpoint(helper);
		QCOMPARE(authenticate(endpoint, helper.password), AuthenticationResult::Authenticated);
		QCOMPARE(authenticate(endpoint, helper.password.toHex()), AuthenticationResult::Rejected);
		QCOMPARE(authenticate(endpoint, {}), AuthenticationResult::Rejected);
		for (int byte = 0; byte < CaptureHelperCredentials::PasswordSize; ++byte)
		{
			auto wrongPassword = helper.password;
			wrongPassword[byte] = char(wrongPassword[byte] ^ 0x01);
			// VNC authentication (DES) only uses the first 8 password bytes: changing the others
			// is not noticed - the password is defence in depth only, see 02-ipc.md §1a.5
			QCOMPARE(authenticate(endpoint, wrongPassword), byte < CaptureHelperCredentials::VncAuthenticationKeySize ? AuthenticationResult::Rejected : AuthenticationResult::Authenticated);
		}
		// the most significant bit of each key byte is ignored by VNC authentication
		auto equivalentPassword = helper.password;
		equivalentPassword[0] = char(equivalentPassword[0] ^ 0x80);
		QCOMPARE(authenticate(endpoint, equivalentPassword), AuthenticationResult::Authenticated);

		// the server stays non-dumpable, the helper is dumpable (required by xdg-desktop-portal)
		QCOMPARE(prctl(PR_GET_DUMPABLE, 0, 0, 0, 0), 0);
		QCOMPARE(openProcessMemory(helperPid), 0);

		// still the first helper instance
		QCOMPARE(helper.process->consecutiveFailures(), 0);

		helper.process->stopSupervised();
		QVERIFY(QFileInfo::exists(helper.socketPath) == false);
	}

	// veyon-server and the demo server worker talk to the expected listener only
	void serverRejectsUnexpectedListener()
	{
		QVERIFY(runAsUnprivilegedUser());

		RealHelper helper;
		QVERIFY(startRealHelper(helper));
		const auto helperPid = helper.processId();

		auto endpoint = serverEndpoint(helper);
		QCOMPARE(authenticate(endpoint, helper.password), AuthenticationResult::Authenticated);

		// a listener with another pid, e.g. a process of the session user that bound the path
		// while the helper restarted
		endpoint.expectedProcessId = [helperPid]() { return helperPid + 1; };
		QCOMPARE(authenticate(endpoint, helper.password), AuthenticationResult::Failed);
		endpoint.expectedProcessId = []() { return qint64(-1); };
		QCOMPARE(authenticate(endpoint, helper.password), AuthenticationResult::Failed);

		// the demo server worker: the listener must be a child of the worker's parent (the server)
		VncServerEndpoint workerEndpoint;
		workerEndpoint.localSocketPath = helper.socketPath;
		workerEndpoint.expectedParentProcessId = getpid();
		QCOMPARE(authenticate(workerEndpoint, helper.password), AuthenticationResult::Authenticated);
		workerEndpoint.expectedParentProcessId = getppid();
		QCOMPARE(authenticate(workerEndpoint, helper.password), AuthenticationResult::Failed);
		// neither expectation set: fail closed
		workerEndpoint.expectedParentProcessId = -1;
		QCOMPARE(authenticate(workerEndpoint, helper.password), AuthenticationResult::Failed);

		// a listener of the right uid but another process (here: this test) offering no
		// authentication at all is refused by the peer check and by the VNC client
		QLocalServer foreignServer;
		const auto foreignPath = helper.directory.filePath(QStringLiteral("veyon/foreign.sock"));
		QVERIFY(foreignServer.listen(foreignPath));
		bool securityTypeNoneSelected = false;
		QObject::connect(&foreignServer, &QLocalServer::newConnection, [&foreignServer, &securityTypeNoneSelected]() {
			auto connection = foreignServer.nextPendingConnection();
			connection->write("RFB 003.008\n");
			QObject::connect(connection, &QLocalSocket::readyRead, connection, [connection, &securityTypeNoneSelected]() {
				const auto data = connection->readAll();
				if (data.startsWith("RFB"))
				{
					// one security type: rfbSecTypeNone
					connection->write(QByteArray("\x01\x01", 2));
				}
				else if (data.startsWith('\x01'))
				{
					securityTypeNoneSelected = true;
				}
			});
		});
		VncServerEndpoint foreignEndpoint;
		foreignEndpoint.localSocketPath = foreignPath;
		foreignEndpoint.expectedProcessId = [helperPid]() { return helperPid; };
		QCOMPARE(authenticate(foreignEndpoint, helper.password), AuthenticationResult::Failed);
		// even if it passed the peer check, the client insists on VNC authentication
		foreignEndpoint.expectedProcessId = []() { return qint64(getpid()); };
		QCOMPARE(authenticate(foreignEndpoint, helper.password), AuthenticationResult::Failed);
		QTest::qWait(100);
		QVERIFY(securityTypeNoneSelected == false);

		// control: without the requirement the client would fall back to rfbSecTypeNone
		VncServerSocket controlSocket(foreignEndpoint);
		VncClientProtocol controlProtocol(controlSocket.device(), VncClientProtocol::Password(helper.password));
		QObject::connect(controlSocket.device(), &QIODevice::readyRead, &controlSocket, [&controlProtocol]() {
			while (controlProtocol.read())
			{
			}
		});
		controlSocket.connectToServer();
		controlProtocol.start();
		QTRY_VERIFY_WITH_TIMEOUT(securityTypeNoneSelected, 5000);

		helper.process->stopSupervised();
	}

	// SO_PEERCRED: connections of other uids are closed before LibVNCServer sends a single byte
	void helperRejectsForeignUid()
	{
		QVERIFY(runAsUnprivilegedUser());
		if (s_foreignConnectorPid <= 0)
		{
			QSKIP("needs to be started as root to connect as another uid");
		}

		RealHelper helper;
		QVERIFY(startRealHelper(helper));

		QCOMPARE(askForeignConnector(helper.socketPath), 'C');
		// the helper is still fine
		QCOMPARE(authenticate(serverEndpoint(helper), helper.password), AuthenticationResult::Authenticated);

		helper.process->stopSupervised();
	}

	// F2 of the WP16 review: more connections than FD_SETSIZE crashed LibVNCServer (FD_SET abort)
	void manyConnectionsDoNotCrashHelper()
	{
		static constexpr int ConnectionCount = 1100;

		QVERIFY(runAsUnprivilegedUser());

		rlimit fileLimit{};
		QVERIFY(getrlimit(RLIMIT_NOFILE, &fileLimit) == 0);
		if (fileLimit.rlim_cur < ConnectionCount + 100 && fileLimit.rlim_max >= ConnectionCount + 100)
		{
			fileLimit.rlim_cur = ConnectionCount + 100;
			setrlimit(RLIMIT_NOFILE, &fileLimit);
		}

		RealHelper helper;
		QVERIFY(startRealHelper(helper));

		QVector<int> connections;
		for (int i = 0; i < ConnectionCount; ++i)
		{
			const auto fd = connectRawSocket(helper.socketPath, true);
			if (fd < 0)
			{
				break;
			}
			connections.append(fd);
			if (i % 50 == 0)
			{
				// let the helper accept (and close the surplus)
				QTest::qWait(10);
			}
		}
		QVERIFY(connections.size() > 1024 || connections.size() > int(fileLimit.rlim_cur) - 200);
		QTest::qWait(500);

		QCOMPARE(helper.process->state(), QProcess::Running);
		QCOMPARE(helper.process->consecutiveFailures(), 0);

		for (const auto fd : std::as_const(connections))
		{
			close(fd);
		}

		QTRY_COMPARE_WITH_TIMEOUT(authenticate(serverEndpoint(helper), helper.password),
								  AuthenticationResult::Authenticated, 10000);
		QCOMPARE(helper.process->consecutiveFailures(), 0);

		helper.process->stopSupervised();
	}

	// a client sending a partial message blocks LibVNCServer for at most MaximumClientWait
	void slowClientDoesNotStallAuthentication()
	{
		QVERIFY(runAsUnprivilegedUser());

		RealHelper helper;
		QVERIFY(startRealHelper(helper));

		const auto slowClient = connectRawSocket(helper.socketPath, false);
		QVERIFY(slowClient >= 0);
		const auto closeSlowClient = qScopeGuard([slowClient]() { close(slowClient); });
		static constexpr char PartialProtocolVersion[] = "RFB 003";
		QCOMPARE(write(slowClient, PartialProtocolVersion, sizeof(PartialProtocolVersion) - 1),
				 ssize_t(sizeof(PartialProtocolVersion) - 1));
		QTest::qWait(100);

		QElapsedTimer timer;
		timer.start();
		QCOMPARE(authenticate(serverEndpoint(helper), helper.password, 15000), AuthenticationResult::Authenticated);
		QVERIFY2(timer.elapsed() < 2 * PipeWireCaptureServerMaximumClientWait + 1000,
				 qPrintable(QString::number(timer.elapsed())));

		helper.process->stopSupervised();
	}

private:
	// PipeWireCaptureServer::MaximumClientWait (the helper's sources are not part of this test)
	static constexpr int PipeWireCaptureServerMaximumClientWait = 2000;
};


QTEST_GUILESS_MAIN(CaptureHelperProcessTest)
#include "CaptureHelperProcessTest.moc"
