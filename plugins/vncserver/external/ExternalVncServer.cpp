/*
 * ExternalVncServer.cpp - implementation of ExternalVncServer class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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


ExternalVncServer::ExternalVncServer() :
	m_configuration()
{
	// sanitize configuration
	if( m_configuration.serverPort() <= 0 )
	{
		m_configuration.setServerPort( 5900 );
	}
}



ExternalVncServer::~ExternalVncServer()
{
}



QWidget* ExternalVncServer::configurationWidget()
{
	return new ExternalVncServerConfigurationWidget( m_configuration );
}



void ExternalVncServer::run( int serverPort, const QString& password )
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
	return m_configuration.password();
}
