/*
 * X11VncConfiguration.h - x11vnc-specific configuration values
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

#ifndef X11VNC_CONFIGURATION_H
#define X11VNC_CONFIGURATION_H

#include "Configuration/Proxy.h"

#define FOREACH_X11VNC_CONFIG_PROPERTY(OP) \
	OP( X11VncConfiguration, m_configuration, BOOL, isXDamageDisabled, setXDamageDisabled, "XDamageDisabled", "X11Vnc" );	\
	OP( X11VncConfiguration, m_configuration, STRING, extraArguments, setExtraArguments, "ExtraArguments", "X11Vnc" );

// clazy:excludeall=ctor-missing-parent-argument

class X11VncConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	X11VncConfiguration();

	FOREACH_X11VNC_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setXDamageDisabled( bool );
	void setExtraArguments( const QString& );

} ;

#endif
