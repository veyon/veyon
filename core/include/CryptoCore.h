/*
 * CryptoCore.h - core functions for crypto features
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

#ifndef CRYPTO_CORE_H
#define CRYPTO_CORE_H

#include "VeyonCore.h"

#include <QtCrypto>

// clazy:excludeall=rule-of-three

class CryptoCore
{
public:
	typedef QCA::KeyGenerator KeyGenerator;
	typedef QCA::PrivateKey PrivateKey;
	typedef QCA::PublicKey PublicKey;
	typedef QCA::SecureArray SecureArray;

	enum {
		RsaKeySize = 4096,
		ChallengeSize = 128,
	};

	static constexpr QCA::EncryptionAlgorithm DefaultEncryptionAlgorithm = QCA::EME_PKCS1_OAEP;
	static constexpr QCA::SignatureAlgorithm DefaultSignatureAlgorithm = QCA::EMSA3_SHA512;

	CryptoCore();
	~CryptoCore();

	static QByteArray VEYON_CORE_EXPORT generateChallenge();

private:
	QCA::Initializer m_qcaInitializer;

};

#endif
