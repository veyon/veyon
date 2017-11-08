/*
 * InternetAccessControlConfiguration.h - INTERNET_ACCESS_CONTROL-specific configuration values
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

#ifndef INTERNET_ACCESS_CONTROL_CONFIGURATION_H
#define INTERNET_ACCESS_CONTROL_CONFIGURATION_H

#include "Configuration/Proxy.h"

#define FOREACH_INTERNET_ACCESS_CONTROL_CONFIG_PROPERTY(OP) \
	OP( InternetAccessControlConfiguration, m_configuration, UUID, backend, setBackend, "Backend", "InternetAccessControl" );	\


class InternetAccessControlConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	InternetAccessControlConfiguration( QObject* parent = nullptr );

	FOREACH_INTERNET_ACCESS_CONTROL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setBackend( QUuid );

} ;

#endif
