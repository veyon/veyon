/*
 * rfbcrypto_openssl.c - Crypto wrapper (openssl version)
 */

/*
 *  Copyright (C) 2011 Gernot Tenchio
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <string.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "rfbcrypto.h"

void digestmd5(const struct iovec *iov, int iovcnt, void *dest)
{
    MD5_CTX c;
    int i;
    
    MD5_Init(&c);
    for (i = 0; i < iovcnt; i++)
	MD5_Update(&c, iov[i].iov_base, iov[i].iov_len);
    MD5_Final(dest, &c);
}

void digestsha1(const struct iovec *iov, int iovcnt, void *dest)
{
    SHA_CTX c;
    int i;
    
    SHA1_Init(&c);
    for (i = 0; i < iovcnt; i++)
	SHA1_Update(&c, iov[i].iov_base, iov[i].iov_len);
    SHA1_Final(dest, &c);
}
