/*
 * PlatformCoreFunctions.h - interface class for platform plugins
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef PLATFORM_CORE_FUNCTIONS_H
#define PLATFORM_CORE_FUNCTIONS_H

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformCoreFunctions
{
public:
	virtual QString programFileExtension() const = 0;
	virtual QString personalAppDataPath() const = 0;
	virtual QString globalAppDataPath() const = 0;

};

#endif // PLATFORM_CORE_FUNCTIONS_H
