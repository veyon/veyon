// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>

#include "Configuration/Proxy.h"

#define FOREACH_HEADLESS_VNC_CONFIG_PROPERTY(OP) \
	OP(HeadlessVncConfiguration, m_configuration, QColor, backgroundColor, setBackgroundColor, "BackgroundColor", "HeadlessVncServer", QColor(QStringLiteral("#198cb3")), Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(HeadlessVncConfiguration, FOREACH_HEADLESS_VNC_CONFIG_PROPERTY)
