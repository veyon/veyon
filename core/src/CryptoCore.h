/*
 * CryptoCore.h - core functions for crypto features
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QtCrypto>

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT CryptoCore
{
public:
	using Certificate = QCA::Certificate;
	using PrivateKey = QCA::PrivateKey;
	using PublicKey = QCA::PublicKey;
	using SecureArray = QCA::SecureArray;
	using PlaintextPassword = SecureArray;

	static constexpr auto ChallengeSize = 128;
	static constexpr auto BitsPerByte = 8;

	static constexpr auto DefaultEncryptionAlgorithm = QCA::EME_PKCS1_OAEP;
	static constexpr auto DefaultSignatureAlgorithm = QCA::EMSA3_SHA512;

	static constexpr auto DefaultCertificateValidity = 30;

	CryptoCore();
	~CryptoCore();

	static QByteArray generateChallenge();

	QString encryptPassword( const PlaintextPassword& password ) const;
	PlaintextPassword decryptPassword( const QString& encryptedPassword ) const;

	PrivateKey createPrivateKey();
	Certificate createSelfSignedHostCertificate( const PrivateKey& privateKey );

private:
	static constexpr auto RsaKeySize = 4096;

	QCA::Initializer m_qcaInitializer{};
	QCA::KeyGenerator m_keyGenerator{};
	PrivateKey m_defaultPrivateKey{};

};
