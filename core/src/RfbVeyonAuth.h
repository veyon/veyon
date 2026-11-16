// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

static constexpr char rfbSecTypeVeyon = 40;

class VEYON_CORE_EXPORT RfbVeyonAuth
{
	Q_GADGET
public:
	enum Type {
		// invalid/null authentication type
		Invalid,

		// client has to sign some data to verify it's authority
		KeyFile = 3,

		// authentication is performed using given username and password
		Logon,

		// client has to prove its authenticity by knowing common token
		Token,
	};

	Q_ENUM(Type)
};
