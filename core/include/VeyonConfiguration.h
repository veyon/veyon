/*
 * VeyonConfiguration.h - a Configuration object storing system wide
 *                        configuration values
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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
#include "Configuration/Object.h"
#include "Configuration/Property.h"

#include "VeyonConfigurationProperties.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT VeyonConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	VeyonConfiguration();
	VeyonConfiguration( Configuration::Store* store );

	void upgrade();

	static QString expandPath( QString path );

	FOREACH_VEYON_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)


} ;

