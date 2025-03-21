/*
 * VncConnectionConfiguration.h - declaration of VncConnectionConfiguration
 *
 * Copyright (c) 2021-2025 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VncConnectionConfiguration
{
	Q_GADGET
public:
	enum class Quality
	{
		Highest,
		High,
		Medium,
		Low,
		Lowest
	};
	Q_ENUM(Quality)

	// intervals and timeouts
	static constexpr int DefaultThreadTerminationTimeout = 30000;
	static constexpr int DefaultConnectTimeout = 10000;
	static constexpr int DefaultReadTimeout = 30000;
	static constexpr int DefaultConnectionRetryInterval = 1000;
	static constexpr int DefaultMessageWaitTimeout = 500;
	static constexpr int DefaultFastFramebufferUpdateInterval = 100;
	static constexpr int DefaultInitialFramebufferUpdateTimeout = 10000;
	static constexpr int DefaultFramebufferUpdateTimeout = 60000;
	static constexpr int DefaultSocketKeepaliveIdleTime = 1000;
	static constexpr int DefaultSocketKeepaliveInterval = 500;
	static constexpr int DefaultSocketKeepaliveCount = 5;

} ;
