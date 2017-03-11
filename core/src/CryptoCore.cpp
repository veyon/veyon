/*
 * CryptoCore.cpp - core functions for crypto features
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <openssl/bn.h>

#include "CryptoCore.h"


QByteArray CryptoCore::generateChallenge()
{
	BIGNUM * challengeBigNum = BN_new();

	if( challengeBigNum == NULL )
	{
		qCritical( "CryptoCore::generateChallenge(): BN_new() failed" );
		return QByteArray();
	}

	// generate a random challenge
	BN_rand( challengeBigNum, ChallengeSize * 8, 0, 0 );
	QByteArray chall( BN_num_bytes( challengeBigNum ), 0 );
	BN_bn2bin( challengeBigNum, (unsigned char *) chall.data() );
	BN_free( challengeBigNum );

	return chall;
}
