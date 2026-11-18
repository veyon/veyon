// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QCoreApplication>
#include "VeyonCore.h"

class VeyonConfiguration;

namespace Configuration
{

class VEYON_CORE_EXPORT Metadata
{
	Q_DECLARE_TR_FUNCTIONS(Configuration::Metadata)
public:
	static void initialize(VeyonConfiguration& configuration);
};

}
