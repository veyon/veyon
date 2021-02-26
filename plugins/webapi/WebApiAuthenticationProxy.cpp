/*
 * WebApiAuthenticationProxy.cpp - implementation of WebApiAuthenticationProxy class
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

#include "WebApiAuthenticationProxy.h"
#include "WebApiConfiguration.h"


WebApiAuthenticationProxy::WebApiAuthenticationProxy( const WebApiConfiguration& configuration ) :
    m_waitConditionWaitTime( configuration.connectionAuthenticationTimeout() * 1000 )
{
}



#if VEYON_VERSION_MAJOR < 5
#include "RfbVeyonAuth.h"

QVector<QUuid> WebApiAuthenticationProxy::authenticationMethods()
{
	const auto types = authenticationTypes();
	QVector<QUuid> uuids;
	for( auto authType : types )
	{
		switch( authType )
		{
		case RfbVeyonAuth::Logon: uuids.append( m_authLogonUuid ); break;
		case RfbVeyonAuth::KeyFile: uuids.append( m_authKeysUuid ); break;
		default: break;
		}
	}

	return uuids;
}
#endif



WebApiAuthenticationProxy::AuthenticationMethod WebApiAuthenticationProxy::initCredentials()
{
	QMutexLocker l( mutex() );

	if( m_selectedAuthenticationMethod.isNull() )
	{
		l.unlock();

		QMutex credentialsWaitMutex;
		QMutexLocker credentialsWaitMutexLocker( &credentialsWaitMutex );

		if( m_credentialsPopulated.wait( &credentialsWaitMutex, QDeadlineTimer(m_waitConditionWaitTime) ) == false )
		{
			vWarning() << "waiting for credentials timed out";
#if VEYON_VERSION_MAJOR >= 5
			return {};
#else
			return RfbVeyonAuth::None;
#endif
		}
		l.relock();
	}

#if VEYON_VERSION_MAJOR >= 5
	return m_selectedAuthenticationMethod;
#else
	if( m_selectedAuthenticationMethod == m_authKeysUuid )
	{
		return RfbVeyonAuth::KeyFile;
	}

	if( m_selectedAuthenticationMethod == m_authLogonUuid )
	{
		return RfbVeyonAuth::Logon;
	}

	if( m_selectedAuthenticationMethod == m_authDummyUuid )
	{
		return RfbVeyonAuth::None;
	}

	vCritical() << "invalid authentication method selected";
	return RfbVeyonAuth::None;
#endif
}



bool WebApiAuthenticationProxy::populateCredentials( QUuid authMethod, const QVariantMap& data )
{
	AuthenticationCredentials credentials;

	if( authMethod == m_authKeysUuid )
	{
		const auto keyName = data[QStringLiteral("keyname")].toString();
		const auto privateKey = CryptoCore::PrivateKey::fromPEM( data[QStringLiteral("keydata")].toString() );

		if( keyName.isEmpty() || credentials.setPrivateKey( privateKey ) == false )
		{
			return false;
		}

		credentials.setAuthenticationKeyName( keyName );
	}
	else if( authMethod == m_authLogonUuid )
	{
		const auto username = data[QStringLiteral("username")].toString();
		const auto password = data[QStringLiteral("password")].toString();

		if( username.isEmpty() || password.isEmpty() )
		{
			return false;
		}

		credentials.setLogonUsername( username );
		credentials.setLogonPassword( password.toUtf8() );
	}

	setCredentials( credentials );

	lock();
	m_selectedAuthenticationMethod = authMethod;
	unlock();

	m_credentialsPopulated.wakeAll();

	return true;
}
