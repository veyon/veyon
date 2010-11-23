/*
 * DsaKey.h - easy to use C++ classes for dealing with DSA-keys, -signatures
 *            etc.
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

 /****************************************************************************
 *
 * In addition, as a special exception, Tobias Doerffel gives permission to link
 * the code of its release of iTALC with the OpenSSL project's "OpenSSL" library
 * (or modified versions of the "OpenSSL" library that use the same license
 * as the original version), and distribute the linked executables.
 *
 * You must comply with the GNU General Public License version 2 in all
 * respects for all of the code used other than the "OpenSSL" code.  If you
 * modify this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so.  If you do not wish to do so, delete
 * this exception statement from your version of this file.
 *
 ****************************************************************************/

#ifndef _DSA_KEY_H
#define _DSA_KEY_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <openssl/dsa.h>


class DsaKey
{
public:
	enum KeyType
	{
		Public,
		Private
	} ;

	static const int DefaultChallengeSize;

	// constructor
	DsaKey( const KeyType _type ) :
		m_dsa( NULL ),
		m_type( _type )
	{
	}

	// destructor
	virtual ~DsaKey()
	{
		if( isValid() )
		{
			DSA_free( m_dsa );
		}
	}

	// return key-type
	inline KeyType type( void ) const
	{
		return m_type;
	}

	inline bool isValid( void ) const
	{
		return ( m_dsa != NULL );
	}

	virtual bool load( const QString & _file,
					QString _passphrase = QString::null ) = 0;
	virtual bool save( const QString & _file,
					QString _passphrase = QString::null ) const = 0;

	bool verifySignature( const QByteArray & _data,
					const QByteArray & _signature ) const;

	const DSA * dsaData( void ) const
	{
		return m_dsa;
	}

	static QByteArray generateChallenge( void );


protected:
	DSA * m_dsa;
	KeyType m_type;

} ;




class PrivateDSAKey : public DsaKey
{
public:
	// constructor - load private key from file
	PrivateDSAKey( const QString & _file,
				const QString & _passphrase = QString::null ) :
		DsaKey( Private )
	{
		load( _file, _passphrase );
	}

	// constructor - generate new private key with given number of bits
	PrivateDSAKey( const unsigned int _bits );

	// returns signature for data
	QByteArray sign( const QByteArray & _data ) const;

	virtual bool load( const QString & _file,
						QString _passphrase = QString::null );
	virtual bool save( const QString & _file,
						QString _passphrase = QString::null ) const;

} ;




class PublicDSAKey : public DsaKey
{
public:
	// constructor - load public key from file
	PublicDSAKey( const QString & _file ) :
		DsaKey( Public )
	{
		load( _file );
	}

	// constructor - derive public key from private key
	explicit PublicDSAKey( const PrivateDSAKey & _pkey );

	virtual bool load( const QString & _file,
						QString _passphrase = QString::null );
	virtual bool save( const QString & _file,
						QString _passphrase = QString::null ) const;

} ;


#endif
