/*
 * RfbVeyonAuth.h - types and names related to veyon-specific RFB
 *                  authentication type
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

static constexpr char rfbSecTypeVeyon = 40;

class VEYON_CORE_EXPORT RfbVeyonAuth
{
	Q_GADGET
public:
	typedef enum Types
	{
		// invalid/null authentication type
		Invalid,

		// no authentication needed
		None,

		// only hosts from an internal whitelist list are allowed
		HostWhiteList,

		// client has to sign some data to verify it's authority
		KeyFile,

		// authentication is performed using given username and password
		Logon,

		// client has to prove its authenticity by knowing common token
		Token,

		AuthTypeCount

	} Type;

	Q_ENUM(Type)

};
