// SPDX-FileCopyrightText: 2008-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>

#include "RfbVeyonAuth.h"
#include "VncConnection.h"

class AuthenticationProxy;
class FeatureMessage;

class VEYON_CORE_EXPORT VeyonConnection : public QObject
{
	Q_OBJECT
public:
	using Instances = struct
	{
		QMutex mutex;
		QHash<QThread*, VeyonConnection*> connections;
	};

	VeyonConnection();

	void stopAndDeleteLater();

	VncConnection* vncConnection()
	{
		return m_vncConnection;
	}

	VncConnection::State state() const
	{
		return m_vncConnection->state();
	}

	bool isConnected() const
	{
		return m_vncConnection && m_vncConnection->isConnected();
	}

	void setVeyonAuthType(RfbVeyonAuth::Type authType)
	{
		m_veyonAuthType = authType;
	}

	RfbVeyonAuth::Type veyonAuthType() const
	{
		return m_veyonAuthType;
	}

	void setAuthenticationProxy(AuthenticationProxy* authenticationProxy)
	{
		m_authenticationProxy = authenticationProxy;
	}

	void sendFeatureMessage(const FeatureMessage& featureMessage);

	bool handleServerMessage(rfbClient* client, uint8_t msg);

	static constexpr auto VeyonConnectionTag = 0xFE14A11;


Q_SIGNALS:
	void featureMessageReceived(const FeatureMessage&);

private:
	~VeyonConnection() override;

	void registerConnection();
	void unregisterConnection();

	// authentication
	static int8_t handleSecTypeVeyon(rfbClient* client, uint32_t authScheme);
	static void hookPrepareAuthentication(rfbClient* client);

	AuthenticationCredentials authenticationCredentials() const;

	VncConnection* m_vncConnection{new VncConnection};

	RfbVeyonAuth::Type m_veyonAuthType;

	AuthenticationProxy* m_authenticationProxy{nullptr};
	QString m_accessControlMessage;
};
