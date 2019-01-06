/*
 * BuiltinDirectoryConfiguration.h - configuration values for BuiltinDirectory plugin
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

#ifndef BUILTIN_DIRECTORY_CONFIGURATION_H
#define BUILTIN_DIRECTORY_CONFIGURATION_H

#include "Configuration/Proxy.h"

#define FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(OP) \
	OP( BuiltinDirectoryConfiguration, m_configuration, JSONARRAY, networkObjects, setNetworkObjects, "NetworkObjects", "BuiltinDirectory" );	\
	/* legacy properties required for upgrade */ \
	OP( BuiltinDirectoryConfiguration, m_configuration, JSONARRAY, localDataNetworkObjects, setLocalDataNetworkObjects, "NetworkObjects", "LocalData" );	\

// clazy:excludeall=ctor-missing-parent-argument

class BuiltinDirectoryConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	BuiltinDirectoryConfiguration();

	FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setNetworkObjects( const QJsonArray& );
	void setLocalDataNetworkObjects( const QJsonArray& );

} ;

#endif
