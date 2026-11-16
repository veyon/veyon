// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QUuid>
#include <QWaitCondition>

#include "AuthenticationProxy.h"
#include "veyonconfig.h"

class WebApiConfiguration;

class WebApiAuthenticationProxy : public AuthenticationProxy
{
public:
	WebApiAuthenticationProxy(const WebApiConfiguration& configuration);

#if VEYON_VERSION_MAJOR < 5
	QVector<QUuid> authenticationMethods();
#endif

	void setSelectedAuthenticationMethod(QUuid authMethod);

	AuthenticationMethod initCredentials() override;

	bool populateCredentials(QUuid authMethod, const QVariantMap& data);

	QUuid dummyAuthenticationMethod() const
	{
		return m_authDummyUuid;
	}

private:
	int m_waitConditionWaitTime{0};
	QUuid m_selectedAuthenticationMethod;

	QWaitCondition m_credentialsPopulated;

	const QUuid m_authDummyUuid{QStringLiteral("0f4e6525-56e9-4660-8e4e-959e6b53f737")};
#if VEYON_VERSION_MAJOR < 5
	const QUuid m_authKeysUuid{QStringLiteral("0c69b301-81b4-42d6-8fae-128cdd113314")};
	const QUuid m_authLogonUuid{QStringLiteral("63611f7c-b457-42c7-832e-67d0f9281085")};
#endif
};
