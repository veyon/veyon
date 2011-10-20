/*
 * rfbcrypto_included.c - Crypto wrapper (included version)
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
#include "md5.h"
#include "sha1.h"
#include "rfbcrypto.h"

void digestmd5(const struct iovec *iov, int iovcnt, void *dest)
{
    struct md5_ctx c;
    int i;

    __md5_init_ctx(&c);
    for (i = 0; i < iovcnt; i++)
	__md5_process_bytes(iov[i].iov_base, iov[i].iov_len, &c);
    __md5_finish_ctx(&c, dest);
}

void digestsha1(const struct iovec *iov, int iovcnt, void *dest)
{
    SHA1Context c;
    int i;

    SHA1Reset(&c);
    for (i = 0; i < iovcnt; i++)
	SHA1Input(&c, iov[i].iov_base, iov[i].iov_len);
    SHA1Result(&c, dest);
}
