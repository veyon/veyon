// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FeatureMessage.h"
#include "VncEvents.h"

// clazy:excludeall=copyable-polymorphic

class VncFeatureMessageEvent : public VncEvent
{
public:
	explicit VncFeatureMessageEvent(const FeatureMessage& featureMessage);

	void fire(rfbClient* client) override;

private:
	FeatureMessage m_featureMessage;
};
