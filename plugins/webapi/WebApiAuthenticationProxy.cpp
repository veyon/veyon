// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WebApiAuthenticationProxy.h"
#include "WebApiConfiguration.h"


WebApiAuthenticationProxy::WebApiAuthenticationProxy(const WebApiConfiguration& configuration) :
	m_waitConditionWaitTime(configuration.connectionAuthenticationTimeout() * 1000)
{
}



#if VEYON_VERSION_MAJOR < 5
#include "RfbVeyonAuth.h"

QVector<QUuid> WebApiAuthenticationProxy::authenticationMethods()
{
	const auto types = authenticationTypes();
	QVector<QUuid> uuids;
	for (auto authType : types)
	{
		switch (authType)
		{
		case RfbVeyonAuth::Logon: uuids.append(m_authLogonUuid); break;
		case RfbVeyonAuth::KeyFile: uuids.append(m_authKeysUuid); break;
		default: break;
		}
	}

	return uuids;
}
#endif



WebApiAuthenticationProxy::AuthenticationMethod WebApiAuthenticationProxy::initCredentials()
{
	QMutexLocker l(mutex());

	if (m_selectedAuthenticationMethod.isNull())
	{
		l.unlock();

		QMutex credentialsWaitMutex;
		QMutexLocker credentialsWaitMutexLocker(&credentialsWaitMutex);

		if (m_credentialsPopulated.wait(&credentialsWaitMutex, m_waitConditionWaitTime) == false)
		{
			vWarning() << "waiting for credentials timed out";
#if VEYON_VERSION_MAJOR >= 5
			return {};
#else
			return RfbVeyonAuth::Invalid;
#endif
		}
		l.relock();
	}

#if VEYON_VERSION_MAJOR >= 5
	return m_selectedAuthenticationMethod;
#else
	if (m_selectedAuthenticationMethod == m_authKeysUuid)
	{
		return RfbVeyonAuth::KeyFile;
	}

	if (m_selectedAuthenticationMethod == m_authLogonUuid)
	{
		return RfbVeyonAuth::Logon;
	}

	if (m_selectedAuthenticationMethod == m_authDummyUuid)
	{
		return RfbVeyonAuth::Invalid;
	}

	vCritical() << "invalid authentication method selected";
	return RfbVeyonAuth::Invalid;
#endif
}



bool WebApiAuthenticationProxy::populateCredentials(QUuid authMethod, const QVariantMap& data)
{
	AuthenticationCredentials credentials;

	if (authMethod == m_authKeysUuid)
	{
		const auto keyName = data[QStringLiteral("keyname")].toString();
		const auto privateKey = CryptoCore::PrivateKey::fromPEM(data[QStringLiteral("keydata")].toString());

		if (keyName.isEmpty() || credentials.setPrivateKey(privateKey) == false)
		{
			return false;
		}

		credentials.setAuthenticationKeyName(keyName);
	}
	else if (authMethod == m_authLogonUuid)
	{
		const auto username = data[QStringLiteral("username")].toString();
		const auto password = data[QStringLiteral("password")].toString();

		if (username.isEmpty() || password.isEmpty())
		{
			return false;
		}

		credentials.setLogonUsername(username);
		credentials.setLogonPassword(password.toUtf8());
	}

	setCredentials(credentials);

	lock();
	m_selectedAuthenticationMethod = authMethod;
	unlock();

	m_credentialsPopulated.wakeAll();

	return true;
}
