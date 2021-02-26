/*
 * WebApiAuthenticationProxy.h - declaration of WebApiAuthenticationProxy class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QUuid>
#include <QWaitCondition>

#include "AuthenticationProxy.h"
#include "veyonconfig.h"

class WebApiConfiguration;

class WebApiAuthenticationProxy : public AuthenticationProxy
{
public:
	WebApiAuthenticationProxy( const WebApiConfiguration& configuration );

#if VEYON_VERSION_MAJOR < 5
	QVector<QUuid> authenticationMethods();
#endif

	void setSelectedAuthenticationMethod( QUuid authMethod );

	AuthenticationMethod initCredentials() override;

	bool populateCredentials( QUuid authMethod, const QVariantMap& data );

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
