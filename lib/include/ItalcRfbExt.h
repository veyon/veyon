/*
 * ItalcRfbExt.h - an extension of the RFB-protocol, used for communication
 *                 between master and clients
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _ITALC_RFB_EXT_H
#define _ITALC_RFB_EXT_H

#include <rfb/rfbproto.h>
#include <rfb/rfbclient.h>

// new rfb-command which tells server or client that an italc-request/response
// is following
#define rfbItalcCoreRequest		30
#define rfbItalcCoreResponse		rfbItalcCoreRequest


#define rfbEncodingItalc 30
#define rfbEncodingItalcCursor 31

#define rfbSecTypeItalc 30


enum PortOffsets
{
	PortOffsetIVS = 11100,
	PortOffsetDemoServer = PortOffsetIVS + 300
} ;


struct ItalcRectEncodingHeader
{
	uint8_t compressed;
	uint32_t bytesLZO;
	uint32_t bytesRLE;
} ;


enum ItalcAuthTypes
{
	// no authentication needed
	ItalcAuthNone,

	// only hosts in internal host-list are allowed
	ItalcAuthHostBased,

	// client has to sign some data to verify it's authority
	ItalcAuthDSA,

	NumItalcAuthTypes

} ;

typedef enum ItalcAuthTypes ItalcAuthType;


#ifdef __cplusplus
extern "C"
{
#endif
void handleSecTypeItalc( rfbClient * _cl );
#ifdef __cplusplus
}
#endif

#endif
