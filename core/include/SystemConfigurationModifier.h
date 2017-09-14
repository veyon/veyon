/*
 * SystemConfigurationModifier.h - class for easy modification of Veyon-related
 *                                 settings in the operating system
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef SYSTEM_CONFIGURATION_MODIFIER_H
#define SYSTEM_CONFIGURATION_MODIFIER_H

#include "VeyonCore.h"

class VEYON_CORE_EXPORT SystemConfigurationModifier
{
public:
	static bool setServiceAutostart( bool enabled );
	static bool setServiceArguments( const QString &serviceArgs );

	static bool enableFirewallException( bool enabled );
	static bool enableSoftwareSAS( bool enabled );

} ;

#endif
