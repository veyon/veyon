// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VncConnectionConfiguration
{
	Q_GADGET
public:
	enum class Quality {
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
};
