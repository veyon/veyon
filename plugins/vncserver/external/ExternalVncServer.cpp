/*
 * ExternalVncServer.cpp - implementation of ExternalVncServer class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QEventLoop>

#include "AuthenticationCredentials.h"
#include "ExternalVncServer.h"
#include "ExternalVncServerConfigurationWidget.h"
#include "VeyonConfiguration.h"


ExternalVncServer::ExternalVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



void ExternalVncServer::upgrade( const QVersionNumber& oldVersion )
{
	if( oldVersion < QVersionNumber( 1, 1 ) )
	{
		// external VNC server password not encrypted yet?
		const auto rawPassword = m_configuration.passwordProperty().variantValue().toString();
		if( rawPassword.size() < MaximumPlaintextPasswordLength )
		{
			// setting it again will encrypt it
			m_configuration.setPassword( Configuration::Password::fromPlainText( rawPassword ) );
		}
	}
}



QWidget* ExternalVncServer::configurationWidget()
{
	return new ExternalVncServerConfigurationWidget( m_configuration );
}



void ExternalVncServer::prepareServer()
{
}



void ExternalVncServer::runServer( int serverPort, const QString& password )
{
	Q_UNUSED(serverPort);
	Q_UNUSED(password);

	QEventLoop().exec();
}



int ExternalVncServer::configuredServerPort()
{
	return m_configuration.serverPort();
}



QString ExternalVncServer::configuredPassword()
{
	return m_configuration.password().plainText();
}



IMPLEMENT_CONFIG_PROXY(ExternalVncServerConfiguration)
