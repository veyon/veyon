/*
 * rfbcrypto_gnutls.c - Crypto wrapper (gnutls version)
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
#include <gcrypt.h>
#include "rfbcrypto.h"

void digestmd5(const struct iovec *iov, int iovcnt, void *dest)
{
    gcry_md_hd_t c;
    int i;

    gcry_md_open(&c, GCRY_MD_MD5, 0);
    for (i = 0; i < iovcnt; i++)
	gcry_md_write(c, iov[i].iov_base, iov[i].iov_len);
    gcry_md_final(c);
    memcpy(dest, gcry_md_read(c, 0), gcry_md_get_algo_dlen(GCRY_MD_MD5));
}

void digestsha1(const struct iovec *iov, int iovcnt, void *dest)
{
    gcry_md_hd_t c;
    int i;

    gcry_md_open(&c, GCRY_MD_SHA1, 0);
    for (i = 0; i < iovcnt; i++)
	gcry_md_write(c, iov[i].iov_base, iov[i].iov_len);
    gcry_md_final(c);
    memcpy(dest, gcry_md_read(c, 0), gcry_md_get_algo_dlen(GCRY_MD_SHA1));
}
