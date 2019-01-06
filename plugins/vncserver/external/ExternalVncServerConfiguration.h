/*
 * ExternalVncServerConfiguration.h - configuration values for external VNC server
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef EXTERNAL_VNC_SERVER_CONFIGURATION_H
#define EXTERNAL_VNC_SERVER_CONFIGURATION_H

#include "Configuration/Proxy.h"
#include "CryptoCore.h"

#define FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( ExternalVncServerConfiguration, m_configuration, INT, serverPort, setServerPort, "ServerPort", "ExternalVncServer" ); \
	OP( ExternalVncServerConfiguration, m_configuration, PASSWORD, password, setPassword, "Password", "ExternalVncServer" );

// clazy:excludeall=ctor-missing-parent-argument

class ExternalVncServerConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	ExternalVncServerConfiguration();

	FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setServerPort( int port );
	void setPassword( const QString& );

} ;

#endif
