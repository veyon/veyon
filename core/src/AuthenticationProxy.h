// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutexLocker>
#include <QWaitCondition>

#include "AuthenticationCredentials.h"
#include "RfbVeyonAuth.h"

class AuthenticationProxy
{
public:
	using AuthenticationMethod = RfbVeyonAuth::Type;
	using AuthenticationMethods = QList<AuthenticationMethod>;

	virtual ~AuthenticationProxy() = default;

	void notifyProtocolError()
	{
		lock();
		m_protocolErrorOccurred = true;
		unlock();

		m_authenticationMethodsAvailable.wakeAll();
	}

	bool protocolErrorOccurred() const
	{
		return m_protocolErrorOccurred;
	}

	void setAuthenticationTypes(const AuthenticationMethods& authenticationTypes)
	{
		lock();
		m_authenticationTypes = authenticationTypes;
		unlock();

		m_authenticationMethodsAvailable.wakeAll();
	}

	AuthenticationMethods authenticationTypes()
	{
		QMutexLocker l(&m_mutex);
		return m_authenticationTypes;
	}

	bool waitForAuthenticationMethods(int timeout)
	{
		QMutex m;
		QMutexLocker l(&m);
		if (authenticationTypes().isEmpty() == false)
		{
			return true;
		}
		return m_authenticationMethodsAvailable.wait(&m, timeout);
	}

	AuthenticationCredentials credentials()
	{
		QMutexLocker l(&m_mutex);
		return m_credentials;
	}

	void setCredentials(const AuthenticationCredentials& credentials)
	{
		QMutexLocker l(&m_mutex);
		m_credentials = credentials;
	}

	virtual RfbVeyonAuth::Type initCredentials() = 0;

protected:
	void lock()
	{
		m_mutex.lock();
	}

	void unlock()
	{
		m_mutex.unlock();
	}

	QMutex* mutex()
	{
		return &m_mutex;
	}

private:
	QMutex m_mutex;
	QAtomicInteger<bool> m_protocolErrorOccurred{false};
	AuthenticationCredentials m_credentials;
	AuthenticationMethods m_authenticationTypes;
	QWaitCondition m_authenticationMethodsAvailable;
};
