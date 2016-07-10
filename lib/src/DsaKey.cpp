/*
 * DsaKey.cpp - easy to use C++ classes for dealing with DSA-keys, -signatures
 *              etc.
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

#include <italcconfig.h>

// project-headers
#include "DsaKey.h"
#include "LocalSystem.h"
#include "Logger.h"


// OpenSSL-headers
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bn.h>

// stdlib-headers
#include <memory.h>
#include <cstdlib>
#include <cstdio>

// Qt-headers
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>




/*	$OpenBSD: buffer.h,v 1.11 2002/03/04 17:27:39 stevesk Exp $	*/

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Code for manipulating FIFO buffers.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */



typedef struct {
	unsigned char	*buf;		/* Buffer for data. */
	unsigned int	 alloc;		/* Number of bytes allocated for data. */
	unsigned int	 offset;	/* Offset of first byte containing data. */
	unsigned int	 end;		/* Offset of last byte containing data. */
}       Buffer;

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Functions for manipulating fifo buffers (that can grow if needed).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#define PUT_32BIT(cp, value) do { \
  (cp)[0] = (value) >> 24; \
  (cp)[1] = (value) >> 16; \
  (cp)[2] = (value) >> 8; \
  (cp)[3] = (value); } while (0)

#define GET_32BIT(cp) (((unsigned long)(unsigned char)(cp)[0] << 24) | \
		       ((unsigned long)(unsigned char)(cp)[1] << 16) | \
		       ((unsigned long)(unsigned char)(cp)[2] << 8) | \
		       ((unsigned long)(unsigned char)(cp)[3]))


/* Initializes the buffer structure. */

void
buffer_init(Buffer *buffer)
{
	const unsigned int len = 4096;

	buffer->alloc = 0;
	buffer->buf = new unsigned char[len];
	buffer->alloc = len;
	buffer->offset = 0;
	buffer->end = 0;
}

/* Frees any memory used for the buffer. */

void
buffer_free(Buffer *buffer)
{
	if (buffer->alloc > 0) {
		memset(buffer->buf, 0, buffer->alloc);
		buffer->alloc = 0;
		delete[] buffer->buf;
	}
}

/*
 * Clears any data from the buffer, making it empty.  This does not actually
 * zero the memory.
 */

void
buffer_clear(Buffer *buffer)
{
	buffer->offset = 0;
	buffer->end = 0;
}


void *
buffer_append_space(Buffer *buffer, unsigned int len)
{
	if (len > 0x100000)
	{
		qCritical( "buffer_append_space: len %u not supported", len );
		exit( -1 );
	}

	/* If the buffer is empty, start using it from the beginning. */
	if (buffer->offset == buffer->end) {
		buffer->offset = 0;
		buffer->end = 0;
	}
restart:
	void *p;
	/* If there is enough space to store all data, store it now. */
	if (buffer->end + len < buffer->alloc) {
		p = buffer->buf + buffer->end;
		buffer->end += len;
		return p;
	}
	/*
	 * If the buffer is quite empty, but all data is at the end, move the
	 * data to the beginning and retry.
	 */
	if (buffer->offset > buffer->alloc / 2) {
		memmove(buffer->buf, buffer->buf + buffer->offset,
			buffer->end - buffer->offset);
		buffer->end -= buffer->offset;
		buffer->offset = 0;
		goto restart;
	}
	/* Increase the size of the buffer and retry. */

	unsigned int newlen = buffer->alloc + len + 32768;
	if (newlen > 0xa00000)
	{
		qCritical( "buffer_append_space: alloc %u not supported",
								newlen );
		exit( -1 );
	}
	buffer->buf = (unsigned char *)realloc(buffer->buf, newlen);
	buffer->alloc = newlen;
	goto restart;
	/* NOTREACHED */
}

/* Appends data to the buffer, expanding it if necessary. */

void
buffer_append(Buffer *buffer, const void *data, unsigned int len)
{
	void *p = buffer_append_space(buffer, len);
	memcpy(p, data, len);
}

/* Returns the number of bytes of data in the buffer. */

unsigned int
buffer_len(Buffer *buffer)
{
	return buffer->end - buffer->offset;
}

/* Gets data from the beginning of the buffer. */

bool
buffer_get(Buffer *buffer, void *buf, unsigned int len)
{
	if (len > buffer->end - buffer->offset)
	{
		qCritical( "buffer_get: trying to get more bytes %d than in "
			"buffer %d", len, buffer->end - buffer->offset );
		return false;
		//exit( -1 );
	}
	memcpy(buf, buffer->buf + buffer->offset, len);
	buffer->offset += len;
	return true;
}


/* Returns a pointer to the first used byte in the buffer. */

void *
buffer_ptr(Buffer *buffer)
{
	return buffer->buf + buffer->offset;
}

void
buffer_put_int(Buffer *buffer, unsigned int value)
{
	char buf[4];

	PUT_32BIT(buf, value);
	buffer_append(buffer, buf, 4);
}

unsigned int
buffer_get_int(Buffer *buffer)
{
	unsigned char buf[4];

	if(buffer_get(buffer, (char *) buf, 4))
		return GET_32BIT(buf);
	return 0;
}

/*
 * Stores and arbitrary binary string in the buffer.
 */
void
buffer_put_string(Buffer *buffer, const void *buf, unsigned int len)
{
	buffer_put_int(buffer, len);
	buffer_append(buffer, buf, len);
}
void
buffer_put_cstring(Buffer *buffer, const char *s)
{
	if (s == NULL)
	{
		qCritical( "buffer_put_cstring: s == NULL" );
		exit( -1 );
	}
	buffer_put_string(buffer, s, strlen(s));
}


/*
 * Returns an arbitrary binary string from the buffer.  The string cannot
 * be longer than 256k.  The returned value points to memory allocated
 * with xmalloc; it is the responsibility of the calling function to free
 * the data.  If length_ptr is non-NULL, the length of the returned data
 * will be stored there.  A null character will be automatically appended
 * to the returned string, and is not counted in length.
 */
void *
buffer_get_string(Buffer *buffer, unsigned int *length_ptr)
{
	unsigned char *value;
	unsigned int len;

	/* Get the length. */
	len = buffer_get_int(buffer);
	if (len > 256 * 1024)
	{
		qCritical( "buffer_get_string: bad string length %u", len );
		exit( -1 );
	}
	/* Allocate space for the string.  Add one byte for a null character. */
	value = new unsigned char[len + 1];
	/* Get the string. */
	buffer_get(buffer, value, len);
	/* Append a null character to make processing easier. */
	value[len] = 0;
	/* Optionally return the length of the string. */
	if (length_ptr)
		*length_ptr = len;
	return value;
}


void buffer_get_bignum2(Buffer *buffer, BIGNUM *value)
{
	unsigned int len;
	unsigned char *bin = (unsigned char*)buffer_get_string(buffer, &len);

	if (len > 8 * 1024)
	{
		qCritical( "buffer_get_bignum2: cannot handle BN of size %d",
									len );
		exit( -1 );
	}
	BN_bin2bn(bin, len, value);
	delete[] bin;
}

void
buffer_put_bignum2(Buffer *buffer, BIGNUM *value)
{
	int bytes = BN_num_bytes(value) + 1;
	unsigned char *buf = new unsigned char[bytes];
	int oi;
	int hasnohigh = 0;

	buf[0] = '\0';
	/* Get the value of in binary */
	oi = BN_bn2bin(value, buf+1);
	if (oi != bytes-1)
	{
		qCritical( "buffer_put_bignum: BN_bn2bin() failed: oi %d "
						"!= bin_size %d", oi, bytes );
		exit( -1 );
	}
	hasnohigh = (buf[1] & 0x80) ? 0 : 1;
	if (value->neg) {
		/**XXX should be two's-complement */
		int i, carry;
		unsigned char *uc = buf;
		for (i = bytes-1, carry = 1; i>=0; i--) {
			uc[i] ^= 0xff;
			if (carry)
				carry = !++uc[i];
		}
	}
	buffer_put_string(buffer, buf+hasnohigh, bytes-hasnohigh);
	memset(buf, 0, bytes);
	delete[] buf;
}


#define INTBLOB_LEN	20
#define SIGBLOB_LEN	(2*INTBLOB_LEN)

const int DsaKey::DefaultChallengeSize = 64;


bool DsaKey::verifySignature( const QByteArray & _data,
					const QByteArray & _sig ) const
{
	if( !isValid() )
	{
		qCritical( "DsaKey::verifySignature(): invalid key" );
		return false;
	}

	// ietf-drafts
	Buffer b;
	buffer_init( &b );
	buffer_append( &b, _sig.data(), _sig.size() );
	char * ktype = (char*) buffer_get_string( &b, NULL );
	if( strcmp( "italc-dss", ktype ) != 0 && strcmp( "ssh-dss", ktype ) != 0)
	{
		qCritical( "DsaKey::verifySignature(): cannot handle type %s", ktype );
		buffer_free( &b );
		delete[] ktype;
		return false;
	}
	delete[] ktype;

	unsigned int len;
	unsigned char * sigblob = (unsigned char *) buffer_get_string( &b,
									&len );
	const unsigned int rlen = buffer_len( &b );
	buffer_free( &b );
	if( rlen != 0 )
	{
		qWarning( "DsaKey::verifySignature(): remaining bytes in signature %d", rlen );
		delete[] sigblob;
		return false;
	}

	if( len != SIGBLOB_LEN )
	{
		qCritical( "bad sigbloblen %u != SIGBLOB_LEN", len );
		return false;
	}

	DSA_SIG * sig = DSA_SIG_new();

	// parse signature
	if( sig == NULL )
	{
		qCritical( "DsaKey::verifySignature(): DSA_SIG_new failed" );
		return false;
	}

	if( ( sig->r = BN_new() ) == NULL )
	{
		qCritical( "DsaKey::verifySignature(): BN_new failed" );
		return false;
	}

	if( ( sig->s = BN_new() ) == NULL )
	{
		qCritical( "DsaKey::verifySignature(): BN_new failed" );
		return false;
	}

	BN_bin2bn( sigblob, INTBLOB_LEN, sig->r );
	BN_bin2bn( sigblob+ INTBLOB_LEN, INTBLOB_LEN, sig->s );

	memset( sigblob, 0, len );
	delete[] sigblob;

	// sha1 the data
	const EVP_MD * evp_md = EVP_sha1();
	EVP_MD_CTX md;
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int dlen;
	EVP_DigestInit( &md, evp_md );
	EVP_DigestUpdate( &md, _data.constData(), _data.size() );
	EVP_DigestFinal( &md, digest, &dlen );

	int ret = DSA_do_verify( digest, dlen, sig, m_dsa );
	memset( digest, 'd', sizeof( digest ) );

	DSA_SIG_free( sig );

	qDebug( "dsa_verify: signature %s", ret == 1 ? "correct" : ret == 0 ?
							"incorrect" : "error" );
	return ( ret == 1 );
}




QByteArray DsaKey::generateChallenge()
{
	BIGNUM * challenge_bn = BN_new();

	if( challenge_bn == NULL )
	{
		qCritical( "DsaKey::generateChallenge(): BN_new() failed" );
		return QByteArray();
	}

	// generate a random challenge
	BN_rand( challenge_bn, DefaultChallengeSize * 8, 0, 0 );
	QByteArray chall( BN_num_bytes( challenge_bn ), 0 );
	BN_bn2bin( challenge_bn, (unsigned char *) chall.data() );
	BN_free( challenge_bn );

	return chall;
}







PrivateDSAKey::PrivateDSAKey( const unsigned int _bits) :
	DsaKey( Private )
{
	m_dsa = DSA_generate_parameters( _bits, NULL, 0, NULL, NULL, NULL,
									NULL );
	if( m_dsa == NULL)
	{
		qCritical( "PrivateDSAKey::PrivateDSAKey(): DSA_generate_parameters failed" );
		return;
	}

	if( !DSA_generate_key( m_dsa ) )
	{
		qCritical( "PrivateDSAKey::PrivateDSAKey(): DSA_generate_key failed" );
		m_dsa = NULL;
		return;
	}
}




QByteArray PrivateDSAKey::sign( const QByteArray & _data ) const
{
	if( !isValid() )
	{
		qCritical( "PrivateDSAKey::sign(): invalid key" );
		return QByteArray();
	}

	const EVP_MD * evp_md = EVP_sha1();
	EVP_MD_CTX md;
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int dlen;

	EVP_DigestInit( &md, evp_md );
	EVP_DigestUpdate( &md, _data.constData(), _data.size() );
	EVP_DigestFinal( &md, digest, &dlen );

	DSA_SIG * sig = DSA_do_sign( digest, dlen, m_dsa );
	memset( digest, 'd', sizeof( digest ) );

	if( sig == NULL )
	{
		qCritical( "PrivateDSAKey::sign(): DSA_do_sign() failed" );
		return QByteArray();
	}

	unsigned int rlen = BN_num_bytes( sig->r );
	unsigned int slen = BN_num_bytes( sig->s );
	if( rlen > INTBLOB_LEN || slen > INTBLOB_LEN )
	{
		qCritical( "bad sig size %u %u", rlen, slen );
		DSA_SIG_free( sig );
		return QByteArray();
	}

	unsigned char sigblob[SIGBLOB_LEN];
	memset( sigblob, 0, SIGBLOB_LEN );
	BN_bn2bin( sig->r, sigblob + SIGBLOB_LEN - INTBLOB_LEN - rlen );
	BN_bn2bin( sig->s, sigblob + SIGBLOB_LEN - slen );
	DSA_SIG_free( sig );

	// ietf-drafts
	Buffer b;
	buffer_init( &b ) ;
	buffer_put_cstring( &b, "italc-dss" );
	buffer_put_string( &b, sigblob, SIGBLOB_LEN );

	QByteArray final_sig( (const char *) buffer_ptr( &b ),
							buffer_len( &b ) );
	buffer_free( &b );

	return final_sig;
}




bool PrivateDSAKey::load( const QString & _file, QString _passphrase )
{
	if( isValid() )
	{
		DSA_free( m_dsa );
		m_dsa = NULL;
	}

	// QFile::handle() of Qt >= 4.3.0 returns -1 under win32
//#if QT_VERSION < 0x040300 || !BUILD_WIN32
	QFile infile( _file );
	if( !QFileInfo( _file ).exists() || !infile.open( QFile::ReadOnly ) )
	{
		qCritical() << "PrivateDSAKey::load(): could not open file" << _file;
		return false;
	}
	FILE * fp = fdopen( infile.handle(), "r" );
/*#else
	FILE * fp = fopen( _file.toAscii().constData(), "r" );
#endif*/
	if( fp == NULL )
	{
		qCritical( "PrivateDSAKey::load(): fdopen failed" );
		return false;
	}

	EVP_PKEY * pk = PEM_read_PrivateKey( fp, NULL, NULL,
						_passphrase.toAscii().data() );
	if( pk == NULL )
	{
		qCritical( "PEM_read_PrivateKey failed" );
		fclose( fp );
		return false;
	}
	else if( pk->type == EVP_PKEY_DSA )
	{
		m_dsa = EVP_PKEY_get1_DSA( pk );
	}
	else
	{
		qCritical( "PEM_read_PrivateKey: mismatch or "
			    "unknown EVP_PKEY save_type %d", pk->save_type );
		EVP_PKEY_free( pk );
		return false;
	}
	fclose( fp );
	EVP_PKEY_free( pk );

	return true;
}




bool PrivateDSAKey::save( const QString & _file, QString _passphrase ) const
{
	if( _passphrase.length() > 0 && _passphrase.length() <= 4 )
	{
		qWarning( "passphrase too short: need more than 4 bytes - "
						"using empty passphrase now" );
		_passphrase = QString::null;
	}

	LocalSystem::Path::ensurePathExists( QFileInfo( _file ).path() );

	QFile outfile( _file );
	if( outfile.exists() )
	{
		outfile.setPermissions( QFile::WriteOwner );
		if( !outfile.remove() )
		{
			qCritical() << "PrivateDSAKey::save(): could not remove existing" << _file;
			return false;
		}
	}
	// QFile::handle() of Qt >= 4.3.0 returns -1 under win32
//#if QT_VERSION < 0x040300 || !BUILD_WIN32
	if( !outfile.open( QFile::WriteOnly | QFile::Truncate ) )
	{
		qCritical() << "PrivateDSAKey::save(): could not save private key in" << _file;
		return false;
	}
	FILE * fp = fdopen( outfile.handle(), "w" );
/*#else
	FILE * fp = fopen( _file.toAscii().constData(), "w" );
#endif*/
	if( fp == NULL )
	{
		qCritical( "PrivateDSAKey::save(): fdopen failed" );
		return false;
	}

	const EVP_CIPHER * cipher = _passphrase.isEmpty() ?
						NULL : EVP_des_ede3_cbc();

	PEM_write_DSAPrivateKey( fp, m_dsa, cipher, _passphrase.isEmpty() ?
			NULL : (unsigned char *) _passphrase.toAscii().data(),
					_passphrase.length(), NULL, NULL );
	fclose( fp );
	outfile.close();
	outfile.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup );

	return true;
}





DSA * createNewDSA()
{
	DSA * dsa = DSA_new();
	if( dsa == NULL )
	{
		qCritical( "createNewDSA(): DSA_new failed" );
		return NULL;
	}
	if( ( dsa->p = BN_new() ) == NULL ||
		( dsa->q = BN_new() ) == NULL ||
		( dsa->g = BN_new() ) == NULL ||
		( dsa->pub_key = BN_new() ) == NULL )
	{
		qCritical( "createNewDSA(): BN_new failed" );
		return NULL;
	}
	return dsa;
}




DSA * keyFromBlob( const QByteArray & _ba )
{
	Buffer b;
	DSA * dsa = NULL;

	buffer_init( &b );
	buffer_append( &b, _ba.constData(), _ba.size() );
	char * ktype = (char*)buffer_get_string( &b, NULL );

	if( strcmp(ktype, "dsa") == 0 || strcmp(ktype, "italc-dss" ) == 0 || strcmp(ktype, "ssh-dss" ) == 0 )
	{
		dsa = createNewDSA();
		buffer_get_bignum2(&b, dsa->p);
		buffer_get_bignum2(&b, dsa->q);
		buffer_get_bignum2(&b, dsa->g);
		buffer_get_bignum2(&b, dsa->pub_key);
	}
	else
	{
		qCritical( "keyFromBlob: cannot handle type %s", ktype );
		return NULL;
	}
	//int rlen = buffer_len( &b );
	//if(key != NULL && rlen != 0)
	//	error("key_from_blob: remaining bytes in key blob %d", rlen);
	delete[] ktype;
	buffer_free( &b );
	return dsa;
}




PublicDSAKey::PublicDSAKey( const PrivateDSAKey & _pk ) :
	DsaKey( Public )
{
	if( !_pk.isValid() )
	{
		qCritical( "PublicDSAKey::PublicDSAKey(): invalid private key to derive from!" );
	}
	m_dsa = createNewDSA();
	if( m_dsa != NULL )
	{
		BN_copy( m_dsa->p, _pk.dsaData()->p );
		BN_copy( m_dsa->q, _pk.dsaData()->q );
		BN_copy( m_dsa->g, _pk.dsaData()->g );
		BN_copy( m_dsa->pub_key, _pk.dsaData()->pub_key );
	}
}




bool PublicDSAKey::load( const QString & _file, QString )
{
	if( isValid() )
	{
		DSA_free( m_dsa );
		m_dsa = NULL;
	}

	QFile infile( _file );
	if( !QFileInfo( _file ).exists() || !infile.open( QFile::ReadOnly ) )
	{
		qCritical() << "PublicDSAKey::load(): could not open file" << _file;
		return false;
	}

	QTextStream ts( &infile );
	QString line;

	while( !( line = ts.readLine() ).isNull() )
	{
		line = line.trimmed();
		if( line[0] != '#' )
		{
			if( line.section( ' ', 0, 0 ) != "italc-dss" && line.section( ' ', 0, 0 ) != "ssh-dss")
			{
				qCritical( "PublicDSAKey::load(): missing keytype" );
				continue;
			}
			m_dsa = keyFromBlob( QByteArray::fromBase64(
					line.section( ' ', 1, 1 ).toAscii() ) );
			if( m_dsa == NULL )
			{
				qCritical( "PublicDSAKey::load(): keyFromBlob failed" );
				continue;
			}
			return true;
		}
	}

	qCritical( "PublicDSAKey::load(): error while reading public key!" );

	return false;
}




bool PublicDSAKey::save( const QString & _file, QString ) const
{
	if( !isValid() )
	{
		qCritical( "PublicDSAKey::save(): key not valid!" );
		return false;
	}

	LocalSystem::Path::ensurePathExists( QFileInfo( _file ).path() );

	QFile outfile( _file );
	if( outfile.exists() )
	{
		outfile.setPermissions( QFile::WriteOwner );
		if( !outfile.remove() )
		{
			qCritical() << "PublicDSAKey::save(): could remove existing file" << _file;
			return false;
		}
	}
	if( !outfile.open( QFile::WriteOnly | QFile::Truncate ) )
	{
		qCritical() << "PublicDSAKey::save(): could not save public key in" << _file;
		return false;
	}

	Buffer b;
	buffer_init( &b );
	buffer_put_cstring( &b, "italc-dss" );
	buffer_put_bignum2( &b, m_dsa->p );
	buffer_put_bignum2( &b, m_dsa->q );
	buffer_put_bignum2( &b, m_dsa->g );
	buffer_put_bignum2( &b, m_dsa->pub_key );

	char * p = (char *) buffer_ptr( &b );
	const int len = buffer_len( &b );
	QTextStream( &outfile ) << QString( "italc-dss %1" ).
							arg( QString( QByteArray( p, len ).toBase64() ) );
	memset( p, 0, len );
	buffer_free( &b );
	outfile.close();
	outfile.setPermissions( QFile::ReadOwner | QFile::ReadUser |
							QFile::ReadGroup | QFile::ReadOther );

	return true;
}


