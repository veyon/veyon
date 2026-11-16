// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "VeyonConfigurationProperties.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT VeyonConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	VeyonConfiguration();
	explicit VeyonConfiguration(Configuration::Store* store);

	void upgrade();

	static QString expandPath(QString path);

	FOREACH_VEYON_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)
};
