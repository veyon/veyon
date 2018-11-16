/*
 * DemoConfiguration.h - configuration values for Demo plugin
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"

#define FOREACH_DEMO_CONFIG_PROPERTY(OP) \
	OP( DemoConfiguration, m_configuration, BOOL, multithreadingEnabled, setMultithreadingEnabled, "MultithreadingEnabled", "Demo", true )	\
	OP( DemoConfiguration, m_configuration, INT, framebufferUpdateInterval, setFramebufferUpdateInterval, "FramebufferUpdateInterval", "Demo", 100 )	\
	OP( DemoConfiguration, m_configuration, INT, keyFrameInterval, setKeyFrameInterval, "KeyFrameInterval", "Demo", 10 )	\
	OP( DemoConfiguration, m_configuration, INT, memoryLimit, setMemoryLimit, "MemoryLimit", "Demo", 128 )	\

DECLARE_CONFIG_PROXY(DemoConfiguration, FOREACH_DEMO_CONFIG_PROPERTY)
