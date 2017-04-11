/*
 * RfbLZORLE.h - LZO+RLE-based RFB rectangle encoding
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of veyon - http://veyon.io
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

#ifndef RFB_LZO_RLE_H
#define RFB_LZO_RLE_H

#include "VeyonCore.h"

#include <stdint.h>

#define rfbEncodingLZORLE 30

class VEYON_CORE_EXPORT RfbLZORLE
{
public:
	RfbLZORLE();

	struct Header
	{
		uint8_t compressed;
		uint32_t bytesLZO;
		uint32_t bytesRLE;
	} ;

} ;

#endif
