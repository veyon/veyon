// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AuthenticationLimiter.h"
#include "IpcChannel.h"
#include "IpcMessage.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>

class QObject;

// Service side of the IPC commands of docs/specs/02-ipc.md §6. Session data is kept per session
// ID and platform session of the writing server, can only be written and read by servers of
// sessions without logged-on user, expires after SessionDataLifetime and is erased when read.
// Only GetSessionData, GetTlsHostKey and GetExternalVncServerPassword return secrets.
// Authenticate (§6b) checks credentials through an injected authenticator, limited per session.
class ServiceIpcCommandHandler
{
public:
	using Clock = IpcChannel::Clock;

	struct TlsHostKey
	{
		Veyon::Crypto::SecureBuffer privateKeyDer;
		QByteArray certificateChainPem;
	};

	using TlsHostKeyProvider = std::function<std::optional<TlsHostKey>()>;
	using SecretProvider = std::function<std::optional<Veyon::Crypto::SecureBuffer>()>;
	using ResponseHandler = std::function<void(const IpcMessage::Response&)>;
	enum class AuthenticationResult {
		Authenticated,
		Failed,
		// no verdict (e.g. helper error or timeout): answered as not authenticated, but not
		// counted as failed check
		NoVerdict,
	};
	using AuthenticationResultHandler = std::function<void(AuthenticationResult result)>;
	// checks the credentials and calls resultHandler at most once, in the thread of context; when
	// context is destroyed before, the result handler is destroyed without being called
	using Authenticator = std::function<void(const QString& username, Veyon::Crypto::SecureBuffer&& password,
											 QObject* context, const AuthenticationResultHandler& resultHandler)>;

	// state of one connected session server
	struct Client
	{
		int sessionId = -1;
		// the platform session the server belongs to; it must survive the restart of the login
		// screen that a user logon triggers (Linux: the seat, Windows: the WTS session ID)
		QString platformSessionId{};
		// evaluated per GetSessionData/SetSessionData request; must return true if unsure
		std::function<bool()> hasLoggedOnUser{};
		// server bootstrapped via D-Bus activation (02 §2a.2)
		bool weakBootstrap = false;
		int tlsHostKeyRequests = 0;
	};

	explicit ServiceIpcCommandHandler(const Clock& clock = IpcChannel::steadyNow);

	// provider returns nothing while TLS is disabled or no valid host certificate exists
	void setTlsHostKeyProvider(const TlsHostKeyProvider& provider)
	{
		m_tlsHostKeyProvider = provider;
	}

	// provider returns nothing unless the external VNC server plugin is configured
	void setExternalVncServerPasswordProvider(const SecretProvider& provider)
	{
		m_externalVncServerPasswordProvider = provider;
	}

	// without an authenticator Authenticate is denied (e.g. on Windows the server checks
	// credentials itself)
	void setAuthenticator(const Authenticator& authenticator, const std::shared_ptr<AuthenticationLimiter>& limiter)
	{
		m_authenticator = authenticator;
		m_authenticationLimiter = limiter;
	}

	// answers all commands but Authenticate, which needs a response handler (denied here)
	[[nodiscard]] IpcMessage::Response handleRequest(Client& client, const IpcMessage::Request& request);

	// answers all commands through responseHandler: Authenticate later, in the thread of context,
	// all others before this function returns
	void handleRequest(Client& client, const IpcMessage::Request& request, QObject* context,
					   const ResponseHandler& responseHandler);

	// drops the session data of a session
	void removeSessionData(const Client& client);

private:
	struct SessionData
	{
		Veyon::Crypto::SecureBuffer data;
		qint64 expiryTime = 0;
	};

	IpcMessage::Response getSessionData(const Client& client, int requestId);
	IpcMessage::Response setSessionData(const Client& client, const IpcMessage::Request& request);
	IpcMessage::Response getTlsHostKey(Client& client, int requestId);
	IpcMessage::Response getExternalVncServerPassword(int requestId);
	void authenticate(const Client& client, const IpcMessage::Request& request, QObject* context,
					  const ResponseHandler& responseHandler);

	void removeExpiredSessionData();

	const Clock m_clock;
	TlsHostKeyProvider m_tlsHostKeyProvider{};
	SecretProvider m_externalVncServerPasswordProvider{};
	Authenticator m_authenticator{};
	std::shared_ptr<AuthenticationLimiter> m_authenticationLimiter{};
	using SessionKey = std::pair<int, QString>;

	static SessionKey sessionKey(const Client& client)
	{
		return {client.sessionId, client.platformSessionId};
	}

	std::map<SessionKey, SessionData> m_sessionData;
};
