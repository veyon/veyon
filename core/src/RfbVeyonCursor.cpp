/*
 * RfbVeyonCursor.cpp - veyon cursor rectangle encoding
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QPixmap>

#include "VeyonVncConnection.h"
#include "RfbVeyonCursor.h"
#include "SocketDevice.h"
#include "VariantStream.h"

#include <rfb/rfb.h>
#include <rfb/rfbclient.h>


static rfbBool handleEncodingVeyonCursor( rfbClient *c,
										rfbFramebufferUpdateRectHeader *r )
{
	VeyonVncConnection *t = (VeyonVncConnection *) rfbClientGetClientData( c, nullptr );
	if( r->encoding != rfbEncodingVeyonCursor || t == nullptr )
	{
		return false;
	}

	SocketDevice socketDevice( VeyonVncConnection::libvncClientDispatcher, c );

	const auto cursorShape = VariantStream( &socketDevice ).read().value<QPixmap>();

	t->cursorShapeUpdatedExternal( cursorShape, r->r.x, r->r.y );

	return true;
}




static rfbClientProtocolExtension * __veyonCursorProtocolExt = nullptr;


RfbVeyonCursor::RfbVeyonCursor()
{
	if( __veyonCursorProtocolExt == nullptr )
	{
		__veyonCursorProtocolExt = new rfbClientProtocolExtension;
		__veyonCursorProtocolExt->encodings = new int[2];
		__veyonCursorProtocolExt->encodings[0] = rfbEncodingVeyonCursor;
		__veyonCursorProtocolExt->encodings[1] = 0;
		__veyonCursorProtocolExt->handleEncoding = handleEncodingVeyonCursor;
		__veyonCursorProtocolExt->handleMessage = nullptr;

		rfbClientRegisterExtension( __veyonCursorProtocolExt );
	}
}


