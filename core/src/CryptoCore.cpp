/*
 * CryptoCore.cpp - core functions for crypto features
 *
 * Copyright (c) 2017-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <openssl/bn.h>

#include <QNetworkInterface>

#include "CryptoCore.h"
#include "HostAddress.h"

CryptoCore::CryptoCore()
{
	const auto features = QCA::supportedFeatures();

	vDebug() << "CryptoCore instance created - features supported by QCA" << qcaVersionStr() << features;

	if( features.contains( QStringLiteral( "rsa" ) ) == false )
	{
		qFatal( "CryptoCore: RSA not supported! Please install a QCA plugin which provides RSA support "
				"(e.g. packages such as libqca-qt5-2-plugins or qca-qt5-ossl)." );
	}

	m_defaultPrivateKey = PrivateKey::fromPEMFile( QStringLiteral(":/core/default-pkey.pem") );
}



CryptoCore::~CryptoCore()
{
	vDebug();
}



QByteArray CryptoCore::generateChallenge()
{
	const auto challengeBigNum = BN_new();

	if( challengeBigNum == nullptr )
	{
		vCritical() << "BN_new() failed";
		return QByteArray();
	}

	// generate a random challenge
	BN_rand( challengeBigNum, ChallengeSize * BitsPerByte, 0, 0 );
	QByteArray chall( BN_num_bytes( challengeBigNum ), 0 );
	BN_bn2bin( challengeBigNum, reinterpret_cast<unsigned char *>( chall.data() ) );
	BN_free( challengeBigNum );

	return chall;
}



QString CryptoCore::encryptPassword( const PlaintextPassword& password ) const
{
	return QString::fromLatin1( m_defaultPrivateKey.toPublicKey().
								encrypt( password, DefaultEncryptionAlgorithm ).toByteArray().toHex() );
}



CryptoCore::PlaintextPassword CryptoCore::decryptPassword( const QString& encryptedPassword ) const
{
	PlaintextPassword decryptedPassword;

	if( PrivateKey( m_defaultPrivateKey ).decrypt( QByteArray::fromHex( encryptedPassword.toUtf8() ),
												   &decryptedPassword, DefaultEncryptionAlgorithm ) )
	{
		return decryptedPassword;
	}

	vCritical() << "failed to decrypt password!";

	return {};
}



CryptoCore::PrivateKey CryptoCore::createPrivateKey()
{
	return m_keyGenerator.createRSA( CryptoCore::RsaKeySize );
}



CryptoCore::Certificate CryptoCore::createSelfSignedHostCertificate( const PrivateKey& privateKey )
{
	QCA::CertificateInfo certInfo{
		{ QCA::CertificateInfoTypeKnown::CommonName, HostAddress::localFQDN() },
		{ QCA::CertificateInfoTypeKnown::DNS, HostAddress::localFQDN() }
	};

	const auto allAddresses = QNetworkInterface::allAddresses();
	for( const auto& address : allAddresses )
	{
		certInfo.insert( QCA::CertificateInfoTypeKnown::IPAddress, address.toString() );
	}

	QCA::CertificateOptions certOpts;
	certOpts.setInfo( certInfo );
	certOpts.setConstraints( { QCA::ConstraintTypeKnown::DigitalSignature,
							   QCA::ConstraintTypeKnown::KeyCertificateSign,
							   QCA::ConstraintTypeKnown::KeyEncipherment,
							   QCA::ConstraintTypeKnown::ServerAuth } );

	certOpts.setValidityPeriod( QDateTime::currentDateTime(),
								QDateTime::currentDateTime().addDays(DefaultCertificateValidity) );

	return QCA::Certificate{certOpts, privateKey};
}
