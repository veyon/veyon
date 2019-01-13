/*
 * UltraVncConfiguration.h - UltraVNC-specific configuration values
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

#ifndef ULTRAVNC_CONFIGURATION_H
#define ULTRAVNC_CONFIGURATION_H

#include "Configuration/Proxy.h"

#define FOREACH_ULTRAVNC_CONFIG_INIT_PROPERTY(OP) \
	OP( UltraVncConfiguration, m_configuration, BOOL, isUltraVncConfigured, setUltraVncConfigured, "Configured", "UltraVNC" );

#define FOREACH_ULTRAVNC_CONFIG_PROPERTY(OP) \
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncCaptureLayeredWindows, setUltraVncCaptureLayeredWindows, "CaptureLayeredWindows", "UltraVNC" );	\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncMultiMonitorSupportEnabled, setUltraVncMultiMonitorSupportEnabled, "DualMonitorSupportEnabled", "UltraVNC" );	\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncPollFullScreen, setUltraVncPollFullScreen, "PollFullScreen", "UltraVNC" );			\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncLowAccuracy, setUltraVncLowAccuracy, "LowAccuracy", "UltraVNC" );					\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncDeskDupEngineEnabled, setUltraVncDeskDupEngineEnabled, "DeskDupEngine", "UltraVNC" );	\

class UltraVncConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	UltraVncConfiguration();

	FOREACH_ULTRAVNC_CONFIG_INIT_PROPERTY(DECLARE_CONFIG_PROPERTY)
	FOREACH_ULTRAVNC_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setUltraVncConfigured( bool );
	void setUltraVncCaptureLayeredWindows( bool );
	void setUltraVncMultiMonitorSupportEnabled( bool );
	void setUltraVncPollFullScreen( bool );
	void setUltraVncLowAccuracy( bool );
	void setUltraVncDeskDupEngineEnabled( bool );

} ;

#endif
