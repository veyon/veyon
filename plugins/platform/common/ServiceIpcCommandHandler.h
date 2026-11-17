// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IpcChannel.h"
#include "IpcMessage.h"

#include <functional>
#include <map>
#include <optional>

// Service side of the IPC commands of docs/specs/02-ipc.md §6. Session data is kept per session
// ID and platform session of the writing server, can only be written and read by servers of
// sessions without logged-on user, expires after SessionDataLifetime and is erased when read.
// Only GetSessionData, GetTlsHostKey and GetExternalVncServerPassword return secrets.
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

	[[nodiscard]] IpcMessage::Response handleRequest(Client& client, const IpcMessage::Request& request);

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

	void removeExpiredSessionData();

	const Clock m_clock;
	TlsHostKeyProvider m_tlsHostKeyProvider{};
	SecretProvider m_externalVncServerPasswordProvider{};
	using SessionKey = std::pair<int, QString>;

	static SessionKey sessionKey(const Client& client)
	{
		return {client.sessionId, client.platformSessionId};
	}

	std::map<SessionKey, SessionData> m_sessionData;
};
