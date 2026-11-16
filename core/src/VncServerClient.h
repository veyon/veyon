// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>

#include "CryptoCore.h"
#include "VncServerProtocol.h"

class VEYON_CORE_EXPORT VncServerClient : public QObject
{
	Q_OBJECT
public:
	enum class AuthState {
		Init,
		Challenge,
		Password,
		Token,
		Successful,
		Failed,
	};
	Q_ENUM(AuthState)

	enum class AccessControlState {
		Init,
		Successful,
		Pending,
		Waiting,
		Failed
	};
	Q_ENUM(AccessControlState)

	explicit VncServerClient(QObject* parent = nullptr) :
		QObject(parent),
		m_protocolState(VncServerProtocol::State::Disconnected),
		m_authState(AuthState::Init),
		m_authType(RfbVeyonAuth::Invalid),
		m_accessControlState(AccessControlState::Init),
		m_username(),
		m_hostAddress(),
		m_challenge()
	{
	}

	VncServerProtocol::State protocolState() const
	{
		return m_protocolState;
	}

	void setProtocolState(VncServerProtocol::State protocolState)
	{
		m_protocolState = protocolState;
	}

	AuthState authState() const
	{
		return m_authState;
	}

	void setAuthState(AuthState authState)
	{
		m_authState = authState;
	}

	RfbVeyonAuth::Type authType() const
	{
		return m_authType;
	}

	void setAuthType(RfbVeyonAuth::Type authType)
	{
		m_authType = authType;
	}

	AccessControlState accessControlState() const
	{
		return m_accessControlState;
	}

	void setAccessControlState(AccessControlState accessControlState)
	{
		m_accessControlState = accessControlState;
	}

	const QString& accessControlDetails() const
	{
		return m_accessControlDetails;
	}

	void setAccessControlDetails(const QString& details)
	{
		m_accessControlDetails = details;
	}

	QElapsedTimer& accessControlTimer()
	{
		return m_accessControlTimer;
	}

	const QString& username() const
	{
		return m_username;
	}

	void setUsername(const QString& username)
	{
		m_username = username;
	}

	const QString& hostAddress() const
	{
		return m_hostAddress;
	}

	void setHostAddress(const QString& hostAddress)
	{
		m_hostAddress = hostAddress;
	}

	const QByteArray& challenge() const
	{
		return m_challenge;
	}

	void setChallenge(const QByteArray& challenge)
	{
		m_challenge = challenge;
	}

	const CryptoCore::PrivateKey& privateKey() const
	{
		return m_privateKey;
	}

	void setPrivateKey(const CryptoCore::PrivateKey& privateKey)
	{
		m_privateKey = privateKey;
	}

public Q_SLOTS:
	void finishAccessControl()
	{
		Q_EMIT accessControlFinished(this);
	}

Q_SIGNALS:
	void accessControlFinished(VncServerClient*);

private:
	VncServerProtocol::State m_protocolState;
	AuthState m_authState;
	RfbVeyonAuth::Type m_authType;
	AccessControlState m_accessControlState;
	QString m_accessControlDetails;
	QElapsedTimer m_accessControlTimer;
	QString m_username;
	QString m_hostAddress;
	QByteArray m_challenge;
	CryptoCore::PrivateKey m_privateKey;
};

using VncServerClientList = QList<VncServerClient*>;
