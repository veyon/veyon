/*
 * AuthenticationCredentials.h - class holding credentials for authentication
 *
 * Copyright (c) 2010-2024 Tobias Junghans <tobydox@veyon.io>
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

#include "CryptoCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT AuthenticationCredentials
{
public:
	using Password = CryptoCore::PlaintextPassword;

	void setInternalVncServerPassword( const Password& password )
	{
		m_internalVncServerPassword = password;
	}

	const Password& internalVncServerPassword() const
	{
		return m_internalVncServerPassword;
	}

private:
	Password m_internalVncServerPassword;

} ;
