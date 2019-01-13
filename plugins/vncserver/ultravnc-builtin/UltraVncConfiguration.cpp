/*
 * UltraVncConfiguration.cpp - UltraVNC-specific configuration values
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

#include "VeyonConfiguration.h"
#include "UltraVncConfiguration.h"


UltraVncConfiguration::UltraVncConfiguration() :
    Configuration::Proxy( &VeyonCore::config() )
{
	if( isUltraVncConfigured() == false )
	{
		setUltraVncCaptureLayeredWindows( true );
		setUltraVncPollFullScreen( true );
		setUltraVncLowAccuracy( true );
		setUltraVncDeskDupEngineEnabled( true );

		setUltraVncConfigured( true );
	}
}


FOREACH_ULTRAVNC_CONFIG_INIT_PROPERTY(IMPLEMENT_CONFIG_SET_PROPERTY)
FOREACH_ULTRAVNC_CONFIG_PROPERTY(IMPLEMENT_CONFIG_SET_PROPERTY)
