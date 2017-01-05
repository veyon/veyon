#include <string.h>
#include <polarssl/md5.h>
#include <polarssl/sha1.h>
#include "rfbcrypto.h"

void digestmd5(const struct iovec *iov, int iovcnt, void *dest)
{
    md5_context c;
    int i;
    
    md5_starts(&c);
    for (i = 0; i < iovcnt; i++)
	md5_update(&c, iov[i].iov_base, iov[i].iov_len);
    md5_finish(&c, dest);
}

void digestsha1(const struct iovec *iov, int iovcnt, void *dest)
{
    sha1_context c;
    int i;
    
    sha1_starts(&c);
    for (i = 0; i < iovcnt; i++)
	sha1_update(&c, iov[i].iov_base, iov[i].iov_len);
    sha1_finish(&c, dest);
}
