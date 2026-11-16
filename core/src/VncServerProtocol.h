// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "RfbVeyonAuth.h"

class QTcpSocket;

class VariantArrayMessage;
class VncServerClient;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT VncServerProtocol
{
public:
	enum class State {
		Disconnected,
		Protocol,
		SecurityInit,
		AuthenticationTypes,
		Authenticating,
		AccessControl,
		FramebufferInit,
		Running,
		Close,
		Closing,
		StateCount
	};

	VncServerProtocol(QTcpSocket* socket, VncServerClient* client);
	virtual ~VncServerProtocol() = default;

	State state() const;

	void start();
	bool read(); // Flawfinder: ignore

	void setServerInitMessage(const QByteArray& serverInitMessage)
	{
		m_serverInitMessage = serverInitMessage;
	}

protected:
	virtual QVector<RfbVeyonAuth::Type> supportedAuthTypes() const = 0;
	virtual void processAuthenticationMessage(VariantArrayMessage& message) = 0;
	virtual void performAccessControl() = 0;

	QTcpSocket* socket()
	{
		return m_socket;
	}

	VncServerClient* client()
	{
		return m_client;
	}

private:
	void setState(State state);

	bool readProtocol();
	bool sendSecurityTypes();
	bool receiveSecurityTypeResponse();
	bool sendAuthenticationTypes();
	bool receiveAuthenticationTypeResponse();
	bool receiveAuthenticationMessage();

	bool processAuthentication(VariantArrayMessage& message);
	bool processAccessControl();

	void sendFailedAccessControlDetails();

	void sendEmptyServerInitMessage();

	bool processFramebufferInit();

private:
	static constexpr auto AccessControlCloseDelay = 10000;
	static constexpr auto AccessControlDetailsSendInterval = 100;

	QTcpSocket* m_socket;
	VncServerClient* m_client;

	QByteArray m_serverInitMessage;

	QTimer m_accessControlDetailsSendTimer;
};
