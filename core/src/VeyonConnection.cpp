// SPDX-FileCopyrightText: 2008-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthenticationCredentials.h"
#include "AuthenticationProxy.h"
#include "CryptoCore.h"
#include "PlatformUserFunctions.h"
#include "SocketDevice.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncFeatureMessageEvent.h"
#include "rfb/rfbclient.h"


static rfbClientProtocolExtension* __veyonProtocolExt = nullptr;
static const uint32_t __veyonSecurityTypes[2] = {rfbSecTypeVeyon, 0};


rfbBool handleVeyonMessage(rfbClient* client, rfbServerToClientMsg* msg)
{
	auto connection = reinterpret_cast<VeyonConnection*>(VncConnection::clientData(client, VeyonConnection::VeyonConnectionTag));
	if (connection)
	{
		return connection->handleServerMessage(client, msg->type) ? TRUE : FALSE;
	}

	return FALSE;
}



VeyonConnection::VeyonConnection() :
	m_veyonAuthType(RfbVeyonAuth::Logon)
{
	if (__veyonProtocolExt == nullptr)
	{
		__veyonProtocolExt = new rfbClientProtocolExtension;
		__veyonProtocolExt->encodings = nullptr;
		__veyonProtocolExt->handleEncoding = nullptr;
		__veyonProtocolExt->handleMessage = handleVeyonMessage;
		__veyonProtocolExt->securityTypes = __veyonSecurityTypes;
		__veyonProtocolExt->handleAuthentication = handleSecTypeVeyon;

		rfbClientRegisterExtension(__veyonProtocolExt);
	}

	// the only authentication type this connection uses (05-tls.md §6): the configured method, or the
	// demo access token in the demo client, which is the only place where a token is set
	if (VeyonCore::hasInitializationFlag(VeyonCore::InitializationFlag::AuthenticationCredentials) &&
		VeyonCore::authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::Token))
	{
		m_veyonAuthType = RfbVeyonAuth::Token;
	}
	else if (VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication)
	{
		m_veyonAuthType = RfbVeyonAuth::KeyFile;
	}

	connect(m_vncConnection, &VncConnection::connectionPrepared, this, &VeyonConnection::registerConnection, Qt::DirectConnection);
	connect(m_vncConnection, &VncConnection::destroyed, VeyonCore::instance(), [this]() {
		delete this;
	});
}



VeyonConnection::~VeyonConnection()
{
	delete m_authenticationProxy;
}



void VeyonConnection::stopAndDeleteLater()
{
	unregisterConnection();

	if (m_vncConnection)
	{
		m_vncConnection->stopAndDeleteLater();
		m_vncConnection = nullptr;
	}
}



void VeyonConnection::sendFeatureMessage(const FeatureMessage& featureMessage)
{
	if (m_vncConnection)
	{
		m_vncConnection->enqueueEvent(new VncFeatureMessageEvent(featureMessage));
	}
}



bool VeyonConnection::handleServerMessage(rfbClient* client, uint8_t msg)
{
	if (msg == FeatureMessage::RfbMessageType)
	{
		SocketDevice socketDev(VncConnection::libvncClientDispatcher, client);
		FeatureMessage featureMessage;
		if (featureMessage.receive(&socketDev) == false)
		{
			vDebug() << "could not receive feature message";

			return false;
		}

		vDebug() << qUtf8Printable(QStringLiteral("%1:%2").arg(QString::fromUtf8(client->serverHost)).arg(client->serverPort))
				 << featureMessage;

		Q_EMIT featureMessageReceived(featureMessage);

		return true;
	}
	else
	{
		vCritical() << "unknown message type" << static_cast<int>(msg)
					<< "from server. Closing connection. Will re-open it later.";
	}

	return false;
}



void VeyonConnection::registerConnection()
{
	if (m_vncConnection)
	{
		m_vncConnection->setClientData(VeyonConnectionTag, this);
	}
}



void VeyonConnection::unregisterConnection()
{
	if (m_vncConnection)
	{
		m_vncConnection->setClientData(VeyonConnectionTag, nullptr);
	}
}



rfbBool VeyonConnection::handleSecTypeVeyon(rfbClient* client, uint32_t authScheme)
{
	if (authScheme != rfbSecTypeVeyon)
	{
		return FALSE;
	}

	hookPrepareAuthentication(client);

	auto connection = static_cast<VeyonConnection*>(VncConnection::clientData(client, VeyonConnectionTag));
	if (connection == nullptr)
	{
		return FALSE;
	}

	SocketDevice socketDevice(VncConnection::libvncClientDispatcher, client);

	return authenticate(connection, socketDevice) ? TRUE : FALSE;
}



bool VeyonConnection::authenticate(VeyonConnection* connection, QIODevice& socketDevice)
{
	const auto proxy = connection->m_authenticationProxy;

	// The server is not authenticated yet (and on plain connections not even identified), so all of
	// its messages are parsed and checked before anything is sent that depends on them: each message
	// is capped at MaxAuthenticationMessageSize, and the server cannot choose the authentication type
	// (05-tls.md §6).
	static constexpr auto MaxMessageSize = VariantArrayMessage::MaxAuthenticationMessageSize;

	VariantArrayMessage message(&socketDevice, MaxMessageSize);
	if (message.receive() == false)
	{
		vDebug() << QThread::currentThreadId() << "invalid authentication message received";
		if (proxy)
		{
			proxy->notifyProtocolError();
		}
		return false;
	}

	bool authTypeCountValid = false;
	const auto authTypeCount = message.read().toInt(&authTypeCountValid);

	if (authTypeCountValid == false || authTypeCount <= 0 || authTypeCount > MaxAuthTypeCount)
	{
		vDebug() << QThread::currentThreadId() << "no or too many auth types received";
		if (proxy)
		{
			proxy->notifyProtocolError();
		}
		return false;
	}

	QList<RfbVeyonAuth::Type> authTypes;
	authTypes.reserve(authTypeCount);

	for (int i = 0; i < authTypeCount; ++i)
	{
		// types unknown here (e.g. from a newer server) are kept in the list, but never chosen
		bool authTypeValid = false;
		const auto authType = RfbVeyonAuth::Type(message.read().toInt(&authTypeValid));
		if (authTypeValid == false || authType == RfbVeyonAuth::Type::Invalid)
		{
			vDebug() << QThread::currentThreadId() << "invalid auth type received";
			if (proxy)
			{
				proxy->notifyProtocolError();
			}
			return false;
		}
		authTypes.append(authType);
	}

	if (proxy)
	{
		proxy->setAuthenticationTypes(authTypes);
	}

	vDebug() << QThread::currentThreadId() << "received authentication types:" << authTypes;

	// only the type of the configured method (or the one requested through the authentication proxy),
	// never a type the server prefers
	const auto chosenAuthType = proxy ? proxy->initCredentials() : connection->veyonAuthType();
	if (chosenAuthType == RfbVeyonAuth::Invalid)
	{
		return false;
	}

	if (authTypes.contains(chosenAuthType) == false)
	{
		vWarning() << QThread::currentThreadId() << "server does not offer authentication type" << chosenAuthType
				   << "- offered:" << authTypes;
		return false;
	}

	vDebug() << QThread::currentThreadId() << "chose authentication type:" << chosenAuthType;

	VariantArrayMessage authReplyMessage(&socketDevice, MaxMessageSize);

	authReplyMessage.write(chosenAuthType);

	// send username which is used when displaying an access confirm dialog
	if (connection->authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::UserLogon))
	{
		authReplyMessage.write(connection->authenticationCredentials().logonUsername());
	}
	else
	{
		authReplyMessage.write(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName));
	}

	if (authReplyMessage.send() == false)
	{
		vWarning() << QThread::currentThreadId() << "failed to send authentication type";
		return false;
	}

	VariantArrayMessage authAckMessage(&socketDevice, MaxMessageSize);
	if (authAckMessage.receive() == false)
	{
		vWarning() << QThread::currentThreadId() << "failed to receive authentication acknowledge";
		return false;
	}

	switch (chosenAuthType)
	{
	case RfbVeyonAuth::KeyFile:
		if (connection->authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::PrivateKey))
		{
			VariantArrayMessage challengeReceiveMessage(&socketDevice, MaxMessageSize);
			if (challengeReceiveMessage.receive() == false)
			{
				vWarning() << QThread::currentThreadId() << "failed to receive challenge";
				return false;
			}

			const auto challengeData = challengeReceiveMessage.read();
			if (challengeData.userType() != QMetaType::QByteArray)
			{
				vCritical() << QThread::currentThreadId() << "invalid challenge!";
				return false;
			}

			const auto challenge = challengeData.toByteArray();

			if (challenge.size() != CryptoCore::ChallengeSize)
			{
				vCritical() << QThread::currentThreadId() << "challenge size mismatch!";
				return false;
			}

			// create local copy of private key so we can modify it within our own thread
			auto key = connection->authenticationCredentials().privateKey();
			if (key.isNull() || key.canSign() == false)
			{
				vCritical() << QThread::currentThreadId() << "invalid private key!";
				return false;
			}

			const auto signature = key.signMessage(challenge, CryptoCore::DefaultSignatureAlgorithm);

			VariantArrayMessage challengeResponseMessage(&socketDevice, MaxMessageSize);
			challengeResponseMessage.write(connection->authenticationCredentials().authenticationKeyName());
			challengeResponseMessage.write(signature);
			if (challengeResponseMessage.send() == false)
			{
				vWarning() << QThread::currentThreadId() << "failed to send challenge response";
				return false;
			}
		}
		else
		{
			vCritical() << QThread::currentThreadId() << "no private key available for key file authentication";
			return false;
		}
		break;

	case RfbVeyonAuth::Logon:
	{
		VariantArrayMessage publicKeyMessage(&socketDevice, MaxMessageSize);
		if (publicKeyMessage.receive() == false)
		{
			vWarning() << QThread::currentThreadId() << "failed to receive public key";
			return false;
		}

		CryptoCore::PublicKey publicKey = CryptoCore::PublicKey::fromPEM(publicKeyMessage.read().toString());

		if (publicKey.canEncrypt() == false)
		{
			vCritical() << QThread::currentThreadId() << "can't encrypt with given public key!";
			return false;
		}

		CryptoCore::SecureArray plainTextPassword(connection->authenticationCredentials().logonPassword());
		CryptoCore::SecureArray encryptedPassword = publicKey.encrypt(plainTextPassword, CryptoCore::DefaultEncryptionAlgorithm);
		if (encryptedPassword.isEmpty())
		{
			vCritical() << QThread::currentThreadId() << "password encryption failed!";
			return false;
		}

		VariantArrayMessage passwordResponse(&socketDevice, MaxMessageSize);
		passwordResponse.write(encryptedPassword.toByteArray());
		if (passwordResponse.send() == false)
		{
			vWarning() << QThread::currentThreadId() << "failed to send password";
			return false;
		}
		break;
	}

	case RfbVeyonAuth::Token:
	{
		VariantArrayMessage tokenAuthMessage(&socketDevice, MaxMessageSize);
		tokenAuthMessage.write(connection->authenticationCredentials().token().toByteArray());
		if (tokenAuthMessage.send() == false)
		{
			vWarning() << QThread::currentThreadId() << "failed to send token";
			return false;
		}
		break;
	}

	default:
		// not reached: only the configured type is chosen, and it is one of the above
		vCritical() << QThread::currentThreadId() << "unsupported authentication type" << chosenAuthType;
		return false;
	}

	return true;
}



void VeyonConnection::hookPrepareAuthentication(rfbClient* client)
{
	auto connection = static_cast<VncConnection*>(VncConnection::clientData(client, VncConnection::VncConnectionTag));
	if (connection)
	{
		// set our internal flag which indicates that we basically have communication with the client
		// which means that the host is reachable
		connection->setServerReachable();
	}
}



AuthenticationCredentials VeyonConnection::authenticationCredentials() const
{
	if (m_authenticationProxy)
	{
		return m_authenticationProxy->credentials();
	}

	return VeyonCore::authenticationCredentials();
}
