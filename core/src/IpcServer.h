// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>
#include <QTimer>

#include "ConnectionLimiter.h"
#include "IpcConnection.h"

#include <atomic>
#include <memory>
#include <optional>

class QLocalSocket;

// One local IPC endpoint for exactly one child process (docs/specs/02-ipc.md §1, §6a): created
// before the child starts, accepts one authenticated connection and closes the listener
// afterwards. Every connection must pass the peer check (Linux: SO_PEERCRED uid and pid; Windows:
// client process ID and session ID) before a single byte is parsed. Failed peer checks and
// handshakes close that connection; after MaximumFailedHandshakes failed handshakes the endpoint
// fails. Connections failing the peer check are dropped without counting.
class VEYON_CORE_EXPORT IpcServer : public QObject
{
	Q_OBJECT
public:
	using ProcessId = qint64;
	static constexpr ProcessId InvalidProcessId = -1;

	enum class WindowsAccess {
		SystemAndAdministrators,
		SystemOnly,
		SystemAndUser,
	};
	Q_ENUM(WindowsAccess)

	struct PeerPolicy
	{
		// Linux: required peer uid, chown target of the socket (-1: keep the creator)
		qint64 userId = -1;
		qint64 socketOwnerUserId = -1;
		// peer pid must equal the value passed to setExpectedProcessId()
		bool checkProcessId = true;
		// Windows: DACL of the pipe, required client session (-1: not checked)
		WindowsAccess windowsAccess = WindowsAccess::SystemAndAdministrators;
		QString windowsUserSid{};
		qint64 windowsSessionId = -1;
		// failed handshakes (of connections that passed the peer check) until the endpoint fails
		int maximumFailedHandshakes = IpcProtocol::MaximumFailedHandshakes;
	};

	enum class DirectoryType {
		// /run/veyon: created with mode 0711 if missing, must be owned by the effective user
		// (root) and must not be writable by group or others
		Shared,
		// $XDG_RUNTIME_DIR/veyon: created with mode 0700 if missing, must be owned by the
		// effective user and have mode 0700 exactly
		Private,
	};

	IpcServer(IpcProtocol::Role role, quint32 sessionId, QObject* parent = nullptr);
	~IpcServer() override;

	// directory: Linux socket directory (prepared with prepareDirectory()), ignored on Windows
	[[nodiscard]] bool listen(const QString& directory, const PeerPolicy& peerPolicy);

	// thread-safe: process ID of the started child
	void setExpectedProcessId(ProcessId processId)
	{
		m_expectedProcessId = processId;
	}

	const QString& endpointName() const
	{
		return m_endpointName;
	}

	// "VEYONIPC1 <name> <hex64>\n" for the child's stdin; empty once a connection is established
	[[nodiscard]] Veyon::Crypto::SecureBuffer credentialsLine() const;

	IpcConnection* connection() const
	{
		return m_connection;
	}

	bool hasFailed() const
	{
		return m_failed;
	}

	void close();

	// address used by IpcClient/QLocalSocket::connectToServer() for an endpoint
	static QString endpointAddress(const QString& directory, const QString& endpointName);

	// Linux: directory of the service ⇄ server endpoints (02 §1)
	static QString serviceEndpointDirectory();

	// Linux: directory for endpoints of workers started by this process: /run/veyon (Shared) when
	// running as root, otherwise $XDG_RUNTIME_DIR/veyon (Private); empty if unavailable (02 §6a)
	static QString workerEndpointDirectory();
	static DirectoryType workerEndpointDirectoryType();

	// Linux: effective uid; Windows: session ID of this process (-1 if not available)
	static qint64 currentUserId();
	static qint64 currentWindowsSessionId();

	// Windows: SDDL of the protected DACL of the endpoint's pipe for the given policy (nothing for an
	// invalid user SID or on other platforms)
	[[nodiscard]] static std::optional<QString> windowsPipeSecurityDescriptor(const PeerPolicy& peerPolicy);

	// Linux: creates or verifies an endpoint directory, fails closed (log line) on mismatch
	[[nodiscard]] static bool prepareDirectory(const QString& path, DirectoryType type);

	// Linux: prepareDirectory() for workerEndpointDirectory(), fails if it is unavailable
	[[nodiscard]] static bool prepareWorkerEndpointDirectory(const QString& directory);

Q_SIGNALS:
	void connectionEstablished(IpcConnection* connection);
	void failed();

private:
	struct PeerIdentity
	{
		ProcessId processId = InvalidProcessId;
		qint64 userId = -1;
		qint64 windowsSessionId = -1;
	};

	enum class PeerCheckResult {
		Accepted,
		Rejected,
		Pending,
	};

	static constexpr auto PendingPeerCheckInterval = 20;
	static constexpr auto MaximumLoggedDroppedConnections = 5;

	void acceptConnection(QLocalSocket* socket);
	void checkPendingPeer();
	PeerCheckResult checkPeer(QLocalSocket* socket) const;
	void startHandshake(QLocalSocket* socket);
	void dropConnection(QLocalSocket* socket, const char* reason, bool admitted);
	void handleFailure();
	void handleEstablished();

	// platform parts: IpcServerPlatform.h, IpcServerUnix.cpp / IpcServerWindows.cpp
	class Listener;
	class UnixListener;
	class WindowsListener;

	static std::optional<PeerIdentity> peerIdentity(QLocalSocket* socket);
	bool checkPeerUser(const PeerIdentity& peer) const;
	static PeerCheckResult checkUnregisteredPeer();

	const IpcProtocol::Role m_role;
	const quint32 m_sessionId;
	std::optional<IpcEndpointCredentials> m_credentials;
	QString m_endpointName;
	PeerPolicy m_peerPolicy{};
	std::atomic<ProcessId> m_expectedProcessId{InvalidProcessId};

	IpcChannel::NonceHistory m_nonceHistory;
	ConnectionLimiter m_connectionLimiter;
	int m_failedAttempts = 0;
	int m_droppedConnections = 0;
	bool m_failed = false;

	QPointer<QLocalSocket> m_pendingSocket;
	qint64 m_pendingSince = 0;
	QTimer m_pendingPeerCheckTimer{this};
	QPointer<IpcConnection> m_handshakeConnection;
	QPointer<IpcConnection> m_connection;

	std::unique_ptr<Listener> m_listener;
};
