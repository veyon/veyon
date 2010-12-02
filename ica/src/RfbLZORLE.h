/*
 * RfbLZORLE.h - LZO+RLE-based RFB rectangle encoding
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * code partly taken from KRDC / vncclientthread.h:
 * Copyright (C) 2007-2008 Urs Wolfer <uwolfer @ kde.org>
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

#ifndef _RFB_LZO_RLE_H
#define _RFB_LZO_RLE_H

#include <stdint.h>

#define rfbEncodingLZORLE 30

class RfbLZORLE
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

