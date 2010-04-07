/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

#ifndef _X11VNC_ENC_H
#define _X11VNC_ENC_H

/* -- enc.h -- */

#if 0
:r /home/runge/uvnc/ultraSC/rc4/ultravnc_dsm_helper.c
#endif

/*
 * ultravnc_dsm_helper.c unix/openssl UltraVNC encryption encoder/decoder. 
 *                       (also a generic symmetric encryption tunnel)
 *
 * compile via:

   cc       -O -o ultravnc_dsm_helper ultravnc_dsm_helper.c -lcrypto
   cc -DDBG -O -o ultravnc_dsm_helper ultravnc_dsm_helper.c -lcrypto

 *
 * See usage below for how to run it.
 *
 * Note: since the UltraVNC DSM plugin implementation changes the RFB
 * (aka VNC) protocol (extra data is sent), you will *ALSO* need to modify
 * your VNC viewer or server to discard (or insert) this extra data.
 *
 * This tool knows nothing about the RFB protocol: it simply
 * encrypts/decrypts a stream using a symmetric cipher, arc4 and aesv2,
 * (others have been added, see usage).  It could be used as a general
 * encrypted tunnel:
 *
 *   any-client <=> ultravnc_dsm_helper <--network--> ultravnc_dsm_helper(reverse mode) <=> any-server
 *
 * e.g. to connect a non-ultra-dsm-vnc viewer to a non-ultra-dsm-vnc server
 * without using SSH or SSL.
 *
 * -----------------------------------------------------------------------
 * Copyright (C) 2008-2009 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA  or see <http://www.gnu.org/licenses/>.
 *
 *  In addition, as a special exception, Karl J. Runge gives permission
 *  to link the code of its release of ultravnc_dsm_helper with the
 *  OpenSSL project's "OpenSSL" library (or with modified versions of it
 *  that use the same license as the "OpenSSL" library), and distribute
 *  the linked executables.  You must obey the GNU General Public License
 *  in all respects for all of the code used other than "OpenSSL".  If you
 *  modify this file, you may extend this exception to your version of the
 *  file, but you are not obligated to do so.  If you do not wish to do
 *  so, delete this exception statement from your version.
 * -----------------------------------------------------------------------
 */

static char *usage = 
    "\n"
    "usage: ultravnc_dsm_helper cipher keyfile listenport remotehost:port\n"
    "\n"
    "e.g.:  ultravnc_dsm_helper arc4 ./arc4.key 5901 snoopy.net:5900\n"
    "\n"
    "       cipher: specify 'msrc4', 'msrc4_sc', 'arc4', 'aesv2',\n"
    "               'aes-cfb', 'aes256', 'blowfish', '3des',\n"
    "               'securevnc'.\n"
    "\n"
    "         'msrc4_sc' enables a workaround for UVNC SC -plugin use.\n"
    "            (it might not be required in SC circa 2009 and later; try 'msrc4'.)\n"
    "\n"
    "         use 'securevnc' for SecureVNCPlugin (RSA key exchange).  'keyfile' is\n"
    "           used as a server RSA keystore in this mode.  If 'keyfile' does not\n"
    "           exist the user is prompted whether to save the key or not (a MD5\n"
    "           hash of it is shown)  If 'keyfile' already exists the server key\n"
    "           must match its contents or the connection is dropped.\n"
    "\n"
    "           HOWEVER, if 'keyfile' ends in the string 'ClientAuth.pkey', then the\n"
    "           normal SecureVNCPlugin client key authentication is performed.\n"
    "           If you want to do both have 'keyfile' end with 'ClientAuth.pkey.rsa'\n"
    "           that file will be used for the RSA keystore, and the '.rsa' will be\n"
    "           trimmed off and the remaining name used as the Client Auth file.\n"
    "\n"
    "         use '.' to have it try to guess the cipher from the keyfile name,\n"
    "           e.g. 'arc4.key' implies arc4, 'rc4.key' implies msrc4, etc.\n"
    "\n"
    "         use 'rev:arc4', etc. to reverse the roles of encrypter and decrypter.\n"
    "           (i.e. if you want to use it for a vnc server, not vnc viewer)\n"
    "\n"
    "         use 'noultra:...' to skip steps involving salt and IV to be compatible\n"
    "           to be compatible with UltraVNC DSM, i.e. assume a normal symmetric\n"
    "           cipher at the other end.\n"
    "\n"
    "         use 'noultra:rev:...' if both are to be supplied.\n"
    "\n"
    "       keyfile: file holding the key (16 bytes for arc4 and aesv2, 87 for msrc4)\n"
    "           E.g. dd if=/dev/random of=./my.key bs=16 count=1\n"
    "           keyfile can also be pw=<string> to use \"string\" for the key.\n"
    "           Or for 'securevnc' the RSA keystore and/or ClientAuth file.\n"
    "\n"
    "       listenport: port to listen for incoming connection on. (use 0 to connect\n"
    "                   to stdio, use a negative value to force localhost)\n"
    "\n"
    "       remotehost:port: host and port to connect to. (e.g. ultravnc server)\n"
    "\n"
    "\n"
    "       Also: cipher may be cipher@n,m where n is the salt size and m is the\n"
    "       initialization vector size. E.g. aesv2@8,16  Use n=-1 to disable salt\n"
    "       and the MD5 hash (i.e. insert the keydata directly into the cipher.)\n"
    "\n"
    "       Use cipher@md+n,m to change the message digest. E.g. arc4@sha+8,16\n"
    "       Supported: 'md5', 'sha', 'sha1', 'ripemd160'.\n"
;

/*
 * We can also run as a module included into x11vnc (-enc option)
 * The includer must set ENC_MODULE and ENC_HAVE_OPENSSL.
 *
 * Note that when running as a module we still assume we have been
 * forked off of the parent process and are communicating back to it
 * via a socket.  So we *still* exit(3) at the end or on error.  And
 * the global settings won't work.
 */
#ifdef ENC_MODULE
#  define main __enc_main
static char *prog = "enc_helper";
#else
#  define ENC_HAVE_OPENSSL 1
static char *prog = "ultravnc_dsm_helper";
#endif

/* unix includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>


/* Solaris (sysv?) needs INADDR_NONE */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif

/* openssl includes */
#if ENC_HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
static const EVP_CIPHER *Cipher;
static const EVP_MD *Digest;
#endif

static char *cipher = NULL;	/* name of cipher, e.g. "aesv2" */
static int reverse = 0;		/* listening connection */
static int msrc4_sc = 0;	/* enables workaround for SC I/II */
static int noultra = 0;		/* manage salt/iv differently from ultradsm */
static int nomd = 0;		/* use the keydata directly, no md5 or salt */
static int pw_in = 0;		/* pw=.... read in */


/* The data that was read in from key file (or pw=password) */
static char keydata[1024];
static int keydata_len;

/* Size of salt and IV; based on UltraVNC DSM */
#define SALT 16
#define MSRC4_SALT 11
#define IVEC 16

/* Set default values of salt and IV */
static int salt_size = SALT;
static int ivec_size = IVEC;

/* To track parent and child pids */
static pid_t parent, child;

/* transfer buffer size */
#define BSIZE 8192

/* Some very verbose debugging stuff I enable for testing */
#ifdef DBG
#  include "dbg.h"
#else
#  define DEC_CT_DBG(p, n)
#  define DEC_PT_DBG(p, n)
#  define ENC_CT_DBG(p, n)
#  define ENC_PT_DBG(p, n)
#  define PRINT_IVEC
#  define PRINT_KEYDATA
#  define PRINT_KEYSTR_AND_FRIENDS
#  define PRINT_LOOP_DBG1
#  define PRINT_LOOP_DBG2
#  define PRINT_LOOP_DBG3
#endif

/* SecureVNCPlugin from: http://adamwalling.com/SecureVNC/ */
#define SECUREVNC_RSA_PUBKEY_SIZE 270
#define SECUREVNC_ENCRYPTED_KEY_SIZE 256
#define SECUREVNC_SIGNATURE_SIZE 256
#define SECUREVNC_KEY_SIZE 16
#define SECUREVNC_RESERVED_SIZE 4
#define SECUREVNC_RC4_DROP_BYTES 3072
#define SECUREVNC_RAND_KEY_SOURCE 1024
static int securevnc = 0;
static int securevnc_arc4 = 0;
static char *securevnc_file = NULL;

static void enc_connections(int, char*, int);

#if !ENC_HAVE_OPENSSL

/* In case we are a module and there is no OpenSSL buildtime support */

extern void enc_do(char *ciph, char *keyfile, char *lport, char *rhp) { 
	fprintf(stderr, "%s: not compiled with OpenSSL\n", prog);
	exit(1);
}

#else

#if defined(NO_EVP_aes_256_cfb) || (defined (__SVR4) && defined (__sun) && !defined(EVP_aes_256_cfb) && !defined(ASSUME_EVP_aes_256_cfb))
/*
 * For Solaris 10 missing 192 & 256 bit crypto.
 * Note that EVP_aes_256_cfb is a macro.
 */
#undef EVP_aes_256_cfb
#define EVP_aes_256_cfb() EVP_aes_128_cfb(); {fprintf(stderr, "Not compiled with EVP_aes_256_cfb() 'aes256' support.\n"); exit(1);}
#endif

/* If we are a module, enc_do() is the only interface we export.  */


/* This works out key type & etc., reads key, calls enc_connections */

extern void enc_do(char *ciph, char *keyfile, char *lport, char *rhp) { 

	struct stat sb;
	char *q, *p, *connect_host;
	char tmp[16];
	int fd, len = 0, listen_port, connect_port, mbits;

	q = ciph;

	/* check for noultra mode: */
	if (strstr(q, "noultra:") == q) {
		noultra = 1;
		q += strlen("noultra:");
	}

	/* check for reverse mode: */
	if (strstr(q, "rev:") == q) {
		reverse = 1;
		q += strlen("rev:");
	}

	/* work out which cipher and set Cipher to the selected one. */
	if (!strcasecmp(q, "msrc4")) {
		Cipher = EVP_rc4();		cipher = "msrc4";

	} else if (!strcasecmp(q, "msrc4_sc")) {
		Cipher = EVP_rc4();		cipher = "msrc4";
		msrc4_sc = 1;			/* no salt/iv workaround */

	} else if (strstr(q, "arc4") == q) {
		Cipher = EVP_rc4();		cipher = "arc4";

	} else if (strstr(q, "aesv2") == q || strstr(q, "aes-ofb") == q) {
		Cipher = EVP_aes_128_ofb();	cipher = "aesv2";

	} else if (strstr(q, "aes-cfb") == q) {
		Cipher = EVP_aes_128_cfb();	cipher = "aes-cfb";

	} else if (strstr(q, "aes256") == q) {
		Cipher = EVP_aes_256_cfb();	cipher = "aes256";

	} else if (strstr(q, "blowfish") == q) {
		Cipher = EVP_bf_cfb();		cipher = "blowfish";

	} else if (strstr(q, "3des") == q) {
		Cipher = EVP_des_ede3_cfb();	cipher = "3des";

	} else if (strstr(q, "securevnc") == q) {
		Cipher = EVP_aes_128_ofb();	cipher = "securevnc";
		securevnc = 1;

	} else if (strstr(q, ".") == q) {
		/* otherwise, try to guess cipher from key filename: */
		if (strstr(keyfile, "arc4.key")) {
			Cipher = EVP_rc4();		cipher = "arc4";

		} else if (strstr(keyfile, "rc4.key")) {
			Cipher = EVP_rc4();		cipher = "msrc4";

		} else if (strstr(keyfile, "aesv2.key")) {
			Cipher = EVP_aes_128_ofb();	cipher = "aesv2";

		} else if (strstr(keyfile, "aes-cfb.key")) {
			Cipher = EVP_aes_128_cfb();	cipher = "aes-cfb";

		} else if (strstr(keyfile, "aes256.key")) {
			Cipher = EVP_aes_256_cfb();	cipher = "aes256";

		} else if (strstr(keyfile, "blowfish.key")) {
			Cipher = EVP_bf_cfb();		cipher = "blowfish";

		} else if (strstr(keyfile, "3des.key")) {
			Cipher = EVP_des_ede3_cfb();	cipher = "3des";

		} else if (strstr(keyfile, "securevnc.")) {
			Cipher = EVP_aes_128_ofb();	cipher = "securevnc";
			securevnc = 1;

		} else {
			fprintf(stderr, "cannot figure out cipher, supply 'msrc4', 'arc4', or 'aesv2' ...\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "cannot figure out cipher, supply 'msrc4', 'arc4', or 'aesv2' ...\n");
		exit(1);
	}

	/* set the default message digest (md5) */
	if (!securevnc) {
		Digest = EVP_md5();
	} else {
		Digest = EVP_sha1();
	}

	/*
	 * Look for user specified salt and IV sizes at the end
	 * ( ciph@salt,iv and ciph@[md+]salt,iv ):
	 */
	p = strchr(q, '@');
	if (p) {
		int s, v;
		p++;
		if (strstr(p, "md5+") == p) {
			Digest = EVP_md5();        p += strlen("md5+");
		} else if (strstr(p, "sha+") == p) {
			Digest = EVP_sha();        p += strlen("sha+");
		} else if (strstr(p, "sha1+") == p) {
			Digest = EVP_sha1();       p += strlen("sha1+");
		} else if (strstr(p, "ripe+") == p) {
			Digest = EVP_ripemd160();  p += strlen("ripe+");
		} else if (strstr(p, "ripemd160+") == p) {
			Digest = EVP_ripemd160();  p += strlen("ripemd160+");
		}
		if (sscanf(p, "%d,%d", &s, &v) == 2) {
			/* cipher@n,m */
			if (-1 <= s && s <= SALT) {
				salt_size = s;
			} else {
				fprintf(stderr, "%s: invalid salt size: %d\n",
				    prog, s);
				exit(1);
			}
			if (0 <= v && v <= EVP_MAX_IV_LENGTH) {
				ivec_size = v;
			} else {
				fprintf(stderr, "%s: invalid IV size: %d\n",
				    prog, v);
				exit(1);
			}
		} else if (sscanf(p, "%d", &s) == 1) {
			/* cipher@n */
			if (-1 <= s && s <= SALT) {
				salt_size = s;
			} else {
				fprintf(stderr, "%s: invalid salt size: %d\n",
				    prog, s);
				exit(1);
			}
		}
		if (salt_size == -1) {
			/* let salt = -1 mean skip both MD5 and salt */
			nomd = 1;
			salt_size = 0;
		}
	}

	/* port to listen on (0 => stdio, negative => localhost) */
	listen_port = atoi(lport);

	/* extract remote hostname and port */
	q = strrchr(rhp, ':');
	if (q) {
		connect_port = atoi(q+1);
		*q = '\0';
	} else {
		/* otherwise guess VNC display 0 ... */
		connect_port = 5900;
	}
	connect_host = strdup(rhp);

	/* check for and read in the key file */
	memset(keydata, 0, sizeof(keydata));

	if (securevnc) {
		/* note the keyfile for rsa verification later */
		if (keyfile != NULL && strcasecmp(keyfile, "none")) {
			securevnc_file = keyfile;
		}
		goto readed_in;
	}

	if (stat(keyfile, &sb) != 0) {
		if (strstr(keyfile, "pw=") == keyfile) {
			/* user specified key/password on cmdline */
			int i;
			len = 0;
			pw_in = 1;
			for (i=0; i < (int) strlen(keyfile); i++) {
				/* load the string to keydata: */
				int n = i + strlen("pw=");
				keydata[i] = keyfile[n];
				if (keyfile[n] == '\0') break;
				len++;
				if (i > 100) break;
			}
			goto readed_in;
		}
		/* otherwise invalid file */
		perror("stat");
		exit(1);
	}
	if (sb.st_size > 1024) {
		fprintf(stderr, "%s: key file too big.\n", prog);
		exit(1);
	}
	fd = open(keyfile, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	/* read it all in */
	len = (int) read(fd, keydata, (size_t) sb.st_size);
	if (len != sb.st_size) {
		perror("read");
		fprintf(stderr, "%s, could not read key file.\n", prog);
		exit(1);
	}
	close(fd);

	readed_in:


	/* check for ultravnc msrc4 format 'rc4.key' */
	mbits = 0;
	if (strstr(keydata, "128 bit") == keydata) {
		mbits = 128;
	} else if (strstr(keydata, " 56 bit") == keydata) {
		mbits = 56;
	} else if (strstr(keydata, " 40 bit") == keydata) {
		mbits = 40;
	}
	if (mbits > 0) {
		/* 4 is for int key length, 12 is for BLOBHEADER. */
		int i, offset = strlen("xxx bit") + 4 + 12;

		/* the key is stored in reverse order! */
		len = mbits/8;
		for (i=0; i < len; i++) {
			tmp[i] = keydata[offset + len - i - 1];
		}

		/* clear keydata and then copy the reversed bytes there: */
		memset(keydata, 0, sizeof(keydata));
		memcpy(keydata, tmp, len);
	}

	keydata_len = len;

	/* initialize random */
	RAND_poll();

	/*
	 * Setup connections, then transfer data when they are all
	 * hooked up.
	 */
	enc_connections(listen_port, connect_host, connect_port);
}
#endif

#if ENC_HAVE_OPENSSL
/*
 * Initialize cipher context and then loop till EOF doing transfer &
 * encrypt or decrypt.
 */
static void enc_xfer(int sock_fr, int sock_to, int encrypt) {
	/*
	 * We keep both E and D aspects in case we revert back to a
	 * single process calling select(2) on all fds...
	 */
	unsigned char E_keystr[EVP_MAX_KEY_LENGTH];
	unsigned char D_keystr[EVP_MAX_KEY_LENGTH];
	EVP_CIPHER_CTX E_ctx, D_ctx;
	EVP_CIPHER_CTX *ctx = NULL;

	unsigned char buf[BSIZE], out[BSIZE];
	unsigned char *psrc = NULL, *keystr;
	unsigned char salt[SALT+1];
	unsigned char ivec_real[EVP_MAX_IV_LENGTH];
	unsigned char *ivec = ivec_real;

	int i, cnt, len, m, n = 0, vb = 0, first = 1;
	int whoops = 1; /* for the msrc4 problem */
	char *encstr, *encsym;
	
	/* zero the buffers */
	memset(buf,  0, BSIZE);
	memset(out,  0, BSIZE);
	memset(salt, 0, sizeof(salt));
	memset(ivec_real, 0, sizeof(ivec_real));
	memset(E_keystr, 0, sizeof(E_keystr));
	memset(D_keystr, 0, sizeof(D_keystr));

	if (!strcmp(cipher, "msrc4")) {
		salt_size = MSRC4_SALT; /* 11 vs. 16 */
	}

	if (msrc4_sc) {
		whoops = 1;	/* force workaround in SC mode */
	}

	if (getenv("ENCRYPT_VERBOSE")) {
		vb = 1;	/* let user turn on some debugging via env. var. */
	}

	/*
	 * reverse mode, e.g. we help a vnc server instead of a viewer.
	 */
	if (reverse) {
		encrypt = (!encrypt);
	}
	encstr = encrypt ? "encrypt" : "decrypt";  /* string for messages */
	encsym = encrypt ? "+" : "-";

	/* use the encryption/decryption context variables below */
	if (encrypt) {
		ctx = &E_ctx;
		keystr = E_keystr;
	} else {
		ctx = &D_ctx;
		keystr = D_keystr;
	}

	if (securevnc) {
		first = 0;	/* no need for salt+iv on first time */
		salt_size = 0;	/* we want no salt */
		n = 0;		/* nothing read */
		ivec_size = 0;	/* we want no IV. */
		ivec = NULL;
	} else if (encrypt) {
		/* encrypter initializes the salt and initialization vector */

		/*
		 * Our salt is 16 bytes but I believe only the first 8
		 * bytes are used by EVP_BytesToKey(3).  Since we send it
		 * to the other "plugin" we need to keep it 16.  Also,
		 * the IV size can depend on the cipher type.  Again, 16.
		 */
		RAND_bytes(salt, salt_size);
		RAND_bytes(ivec, ivec_size);

		/* place them in the send buffer: */
		memcpy(buf, salt, salt_size);
		memcpy(buf+salt_size, ivec, ivec_size);

		n = salt_size + ivec_size;

		ENC_PT_DBG(buf, n);

	} else {
		/* decrypter needs to read salt + iv from the wire: */

		/* sleep 100 ms (TODO: select on fd) */
		struct timeval tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 100 * 1000;
		select(1, NULL, NULL, NULL, &tv);

		if (salt_size+ivec_size == 0) {
			n = 0;	/* no salt or iv, skip reading. */
		} else {
			n = read(sock_fr, buf, salt_size+ivec_size+96);
		}
		if (n == 0 && salt_size+ivec_size > 0) {
			fprintf(stderr, "%s: decrypt finished.\n", prog);
			goto finished;
		}
		if (n < salt_size+ivec_size) {
		    if (msrc4_sc && n == 12) {
			fprintf(stderr, "%s: only %d bytes read. Assuming "
			    "UVNC Single Click server.\n", prog, n);
		    } else {
			if (n < 0) perror("read");
			fprintf(stderr, "%s: could not read enough for salt "
			    "and ivec: n=%d\n", prog, n);
			goto finished;
		    }
		}

		DEC_CT_DBG(buf, n);

		if (msrc4_sc && n == 12) {
			; /* send it as is */
		} else {
			/* extract them to their buffers: */
			memcpy(salt, buf, salt_size);
			memcpy(ivec, buf+salt_size, ivec_size);

			/* the rest is some encrypted data: */
			n = n - salt_size - ivec_size;
			psrc = buf + salt_size + ivec_size;
		
			if (n > 0) {
				/*
				 * copy it down to the start of buf for
				 * sending below:
				 */
				for (i=0; i < n; i++) {
					buf[i] = psrc[i];
				}
			}
		}
	}

	/* debug output */
	PRINT_KEYDATA;
	PRINT_IVEC;

	if (!strcmp(cipher, "msrc4")) {
		/* special cases for MSRC4: */

		if (whoops) {
			fprintf(stderr, "%s: %s - WARNING: MSRC4 mode and IGNORING random salt\n", prog, encstr);
			fprintf(stderr, "%s: %s - WARNING: and initialization vector!!\n", prog, encstr);
			EVP_CIPHER_CTX_init(ctx);
			if (pw_in) {
			    /* for pw=xxxx a md5 hash is used */
			    EVP_BytesToKey(Cipher, Digest, NULL, (unsigned char *) keydata,
			        keydata_len, 1, keystr, NULL);
			    EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, NULL,
			        encrypt);
			} else {
			    /* otherwise keydata as is */
			    EVP_CipherInit_ex(ctx, Cipher, NULL,
			        (unsigned char *) keydata, NULL, encrypt);
			}
		} else {
			/* XXX might not be correct, just exit. */
			fprintf(stderr, "%s: %s - Not sure about msrc4 && !whoops case, exiting.\n", prog, encstr);
			exit(1);

			EVP_BytesToKey(Cipher, Digest, NULL, (unsigned char *) keydata,
			    keydata_len, 1, keystr, ivec); 
			EVP_CIPHER_CTX_init(ctx);
			EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, ivec,
			    encrypt);
		}

	} else {
		unsigned char *in_salt = NULL;

		/* check salt and IV source and size. */
		if (securevnc) {
			in_salt = NULL;
		} else if (salt_size <= 0) {
			/* let salt_size = 0 mean keep it out of the MD5 */
			fprintf(stderr, "%s: %s - WARNING: no salt\n",
			    prog, encstr);
			in_salt = NULL;
		} else {
			in_salt = salt;
		}

		if (ivec_size < Cipher->iv_len && !securevnc) {
			fprintf(stderr, "%s: %s - WARNING: short IV %d < %d\n",
			    prog, encstr, ivec_size, Cipher->iv_len);
		}

		/* make the hashed value and place in keystr */

		/*
		 * XXX N.B.: DSM plugin had count=0, and overwrote ivec
		 * by not passing NULL iv.
		 */

		if (nomd) {
			/* special mode: no salt or md5, use keydata directly */

			int sz = keydata_len < EVP_MAX_KEY_LENGTH ?
			    keydata_len : EVP_MAX_KEY_LENGTH; 

			fprintf(stderr, "%s: %s - WARNING: no-md5 specified: ignoring salt & hash\n", prog, encstr);
			memcpy(keystr, keydata, sz);

		} else if (noultra && ivec_size > 0) {
			/* "normal" mode, don't overwrite ivec. */

			EVP_BytesToKey(Cipher, Digest, in_salt, (unsigned char *) keydata,
			    keydata_len, 1, keystr, NULL);

		} else {
			/* 
			 * Ultra DSM compatibility mode.  Note that this
			 * clobbers the ivec we set up above!  Under
			 * noultra we overwrite ivec only if ivec_size=0.
			 *
			 * SecureVNC also goes through here. in_salt and ivec are NULL.
			 * And ivec is NULL below in the EVP_CipherInit_ex() call.
			 */
			EVP_BytesToKey(Cipher, Digest, in_salt, (unsigned char *) keydata,
			    keydata_len, 1, keystr, ivec);
		}


		/* initialize the context */
		EVP_CIPHER_CTX_init(ctx);


		/* set the cipher & initialize */

		/*
		 * XXX N.B.: DSM plugin implementation had encrypt=1
		 * for both (i.e. perfectly symmetric)
		 */

		EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, ivec, encrypt);
	}

	if (securevnc && securevnc_arc4) {
		/* need to discard initial 3072 bytes */
		unsigned char buf1[SECUREVNC_RC4_DROP_BYTES];
		unsigned char buf2[SECUREVNC_RC4_DROP_BYTES];
		int cnt = 0;
		EVP_CipherUpdate(ctx, buf1, &cnt, buf2, SECUREVNC_RC4_DROP_BYTES);
	}

	/* debug output */
	PRINT_KEYSTR_AND_FRIENDS;

	/* now loop forever processing the data stream */

	while (1) {
		errno = 0;
		if (first && n > 0) {
			if (encrypt && msrc4_sc) {
				/* skip sending salt+iv */
				first = 0;
				continue;
			} else {
				/* use that first block of data placed in buf */
			}
		} else if (first && n == 0 && salt_size + ivec_size == 0) {
			first = 0;
			continue;
		} else {
			/* general case of loop, read some in: */
			n = read(sock_fr, buf, BSIZE);
		}

		/* debug output: */
		if (vb) fprintf(stderr, "%s%d/%d ", encsym, n, errno);
		PRINT_LOOP_DBG1;

		if (n == 0 || (n < 0 && errno != EINTR)) {
			/* failure to read any data, it is EOF or fatal error */
			int err = errno;

			/* debug output: */
			PRINT_LOOP_DBG2;
			fprintf(stderr, "%s: %s - input stream finished: n=%d, err=%d", prog, encstr, n, err);

			/* EOF or fatal error */
			break;

		} else if (n > 0) {
			/* we read in some data, now transform it: */

			if (first && encrypt) {
				/* first time, copy the salt and ivec to out[] for sending */
				memcpy(out, buf, n);
				cnt = n;

			} else if (!EVP_CipherUpdate(ctx, out, &cnt, buf, n)) {
				/* otherwise, we transform the data */
				fprintf(stderr, "%s: enc_xfer EVP_CipherUpdate failed.\n", prog);
				break;
			}

			/* debug output: */
			if (vb) fprintf(stderr, "%sc%d/%d ", encsym, cnt, n);
			PRINT_LOOP_DBG3;

			/* write transformed data to the other end: */
			len = cnt;
			psrc = out;
			while (len > 0) {
				errno = 0;
				m = write(sock_to, psrc, len);

				/* debug output: */
				if (vb) fprintf(stderr, "m%s%d/%d ", encsym, m, errno);

				if (m > 0) {
					/* scoot them by how much was written: */
					psrc += m;
					len  -= m;
				}
				if (m < 0 && (errno == EINTR || errno == EAGAIN)) {
					/* interrupted or blocked */
					continue;
				}
				/* EOF or fatal error */
				break;
			}
		} else {
			/* this is EINTR */
		}
		first = 0;
	}

	/* transfer done (viewer exited or some error) */
	finished:

	fprintf(stderr, "\n%s: %s - close sock_to\n", prog, encstr);
	close(sock_to);

	fprintf(stderr,   "%s: %s - close sock_fr\n", prog, encstr);
	close(sock_fr);

	/* kill our partner after 2 secs. */
	sleep(2);
	if (child)  {
		if (kill(child, SIGTERM) == 0) {
			fprintf(stderr, "%s[%d]: %s - killed my partner: %d\n",
			    prog, (int) getpid(), encstr, (int) child);
		}
	} else {
		if (kill(parent, SIGTERM) == 0) {
			fprintf(stderr, "%s[%d]: %s - killed my partner: %d\n",
			    prog, (int) getpid(), encstr, (int) parent);
		}
	}
}

static int securevnc_server_rsa_save_dialog(char *file, char *md5str, unsigned char* rsabuf) {
	/* since we are likely running in the background, use this kludge by running tk */
	FILE *p;
	char str[2], *q = file, *cmd = getenv("WISH") ? getenv("WISH") : "wish";
	int rc;

	memset(str, 0, sizeof(str));

	p = popen(cmd, "w");
	if (p == NULL) {
		fprintf(stderr, "checkserver_rsa: could not run: %s\n", cmd); 
		return 0;
	}

	/* start piping tk/tcl code to it: */
	fprintf(p, "wm withdraw .\n");
	fprintf(p, "set x [expr [winfo screenwidth  .]/2]\n");
	fprintf(p, "set y [expr [winfo screenheight .]/2]\n");
	fprintf(p, "wm geometry . +$x+$y; update\n");
	fprintf(p, "catch {option add *Dialog.msg.font {helvetica -14 bold}}\n");
	fprintf(p, "catch {option add *Dialog.msg.wrapLength 6i}\n");
	fprintf(p, "set ans [tk_messageBox -title \"Save and Trust UltraVNC RSA Key?\" -icon question ");
	fprintf(p, "-type yesno -message \"Save and Trust UltraVNC SecureVNCPlugin RSA Key\\n\\n");
	fprintf(p, "With MD5 sum: %s\\n\\n", md5str);
	fprintf(p, "In file: ");
	while (*q != '\0') {
		/* sanitize user supplied string: */
		str[0] = *q;
		if (strpbrk(str, "[](){}`'\"$&*|<>") == NULL) {
			fprintf(p, "%s", str);
		}
		q++;
	}
	fprintf(p, " ?\"]\n");
	fprintf(p, "if { $ans == \"yes\" } {destroy .; exit 0} else {destroy .; exit 1}\n");
	rc = pclose(p);
	if (rc == 0) {
		fprintf(stderr, "checkserver_rsa: query returned: %d.  saving it.\n", rc);
		p = fopen(file, "w");
		if (p == NULL) {
			fprintf(stderr, "checkserver_rsa: could not open %s\n", file);
			return 0;
		}
		write(fileno(p), rsabuf, SECUREVNC_RSA_PUBKEY_SIZE);
		fclose(p);
		return 2;
	} else {
		fprintf(stderr, "checkserver_rsa: query returned: %d.  NOT saving it.\n", rc);
		return -1;
	}
}

static char *rsa_md5_sum(unsigned char* rsabuf) {
	EVP_MD_CTX md;
	char digest[EVP_MAX_MD_SIZE], tmp[16];
	char md5str[EVP_MAX_MD_SIZE * 8];
	unsigned int i, size = 0;

	EVP_DigestInit(&md, EVP_md5());
	EVP_DigestUpdate(&md, rsabuf, SECUREVNC_RSA_PUBKEY_SIZE);
	EVP_DigestFinal(&md, (unsigned char *)digest, &size);

	memset(md5str, 0, sizeof(md5str));
	for (i=0; i < size; i++) {
		unsigned char uc = (unsigned char) digest[i];
		sprintf(tmp, "%02x", (int) uc);
		strcat(md5str, tmp);
	}
	return strdup(md5str);
}

static int securevnc_check_server_rsa(char *file, unsigned char *rsabuf) {
	struct stat sb;
	unsigned char filebuf[SECUREVNC_RSA_PUBKEY_SIZE];
	char *md5str = rsa_md5_sum(rsabuf);

	if (!file) {
		return 0;
	}

	memset(filebuf, 0, sizeof(filebuf));
	if (stat(file, &sb) == 0) {
		int n, fd, i, ok = 1;

		if (sb.st_size != SECUREVNC_RSA_PUBKEY_SIZE) {
			fprintf(stderr, "checkserver_rsa: file is wrong size: %d != %d '%s'\n",
			    (int) sb.st_size, SECUREVNC_RSA_PUBKEY_SIZE, file);
			return 0;
		}

		fd = open(file, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "checkserver_rsa: could not open: '%s'\n", file);
			return 0;
		}

		n = (int) read(fd, filebuf, SECUREVNC_RSA_PUBKEY_SIZE);
		close(fd);
		if (n != SECUREVNC_RSA_PUBKEY_SIZE) {
			fprintf(stderr, "checkserver_rsa: could not read all of file: %d != %d '%s'\n",
			    n, SECUREVNC_RSA_PUBKEY_SIZE, file);
			return 0;
		}

		for (i=0; i < SECUREVNC_RSA_PUBKEY_SIZE; i++) {
			if (filebuf[i] != rsabuf[i]) {
				ok = 0;
			}
		}
		if (!ok) {
			char *str1 = rsa_md5_sum(rsabuf);
			char *str2 = rsa_md5_sum(filebuf);
			fprintf(stderr, "checkserver_rsa: rsa keystore contents differ for '%s'\n", file);
			fprintf(stderr, "checkserver_rsa: MD5 sum of server key: %s\n", str1);
			fprintf(stderr, "checkserver_rsa: MD5 sum of keystore:   %s\n", str2);
		}
		return ok;
	} else {

		fprintf(stderr, "checkserver_rsa: rsa keystore file does not exist: '%s'\n", file);
		fprintf(stderr, "checkserver_rsa: asking user if we should store rsa key in it.\n\n");
		fprintf(stderr, "checkserver_rsa: RSA key has MD5 sum: %s\n\n", md5str); 

		return securevnc_server_rsa_save_dialog(file, md5str, rsabuf);
	}
}

static RSA *load_client_auth(char *file) {
	struct stat sb;
	int fd, n;
	char *contents;
	RSA *rsa;

	if (!file) {
		return NULL;
	}
	if (stat(file, &sb) != 0) {
		return NULL;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "load_client_auth: could not open: '%s'\n", file);
		return NULL;
	}

	contents = (char *) malloc(sb.st_size);
	n = (int) read(fd, contents, sb.st_size);
	close(fd);

	if (n != sb.st_size)  {
		fprintf(stderr, "load_client_auth: could not read all of: '%s'\n", file);
		free(contents);
		return NULL;
	}

	rsa = d2i_RSAPrivateKey(NULL, (const unsigned char **) ((void *) &contents), sb.st_size);
	if (!rsa) {
		fprintf(stderr, "load_client_auth: d2i_RSAPrivateKey failed for: '%s'\n", file);
		return NULL;
	}

	if (RSA_check_key(rsa) != 1) {
		fprintf(stderr, "load_client_auth: rsa key invalid: '%s'\n", file);
		return NULL;
	}

	return rsa;
}

static void sslexit(char *msg) {
	fprintf(stderr, "%s: %s\n", msg, ERR_error_string(ERR_get_error(), NULL));
	exit(1);
}

static void securevnc_setup(int conn1, int conn2) {
	RSA *rsa = NULL;
	EVP_CIPHER_CTX init_ctx;
	unsigned char keystr[EVP_MAX_KEY_LENGTH];
	unsigned char *rsabuf, *rsasav;
	unsigned char *encrypted_keybuf;
	unsigned char *initkey;	
	unsigned int server_flags = 0;
	unsigned char one = 1, zero = 0, sig = 16;
	unsigned char b1, b2, b3, b4;
	unsigned char buf[BSIZE], to_viewer[BSIZE];
	int to_viewer_len = 0;
	int n = 0, len, rc;
	int server = reverse ? conn1 : conn2;
	int viewer = reverse ? conn2 : conn1;
	char *client_auth = NULL;
	int client_auth_req = 0;
	int keystore_verified = 0;

	ERR_load_crypto_strings();

	/* alloc and read from server the 270 comprising the rsa public key: */
	rsabuf = (unsigned char *) calloc(SECUREVNC_RSA_PUBKEY_SIZE, 1);
	rsasav = (unsigned char *) calloc(SECUREVNC_RSA_PUBKEY_SIZE, 1);
	len = 0;
	while (len < SECUREVNC_RSA_PUBKEY_SIZE) {
		n = read(server, rsabuf + len, SECUREVNC_RSA_PUBKEY_SIZE - len);
		if (n == 0 || (n < 0 && errno != EINTR)) {
			fprintf(stderr, "securevnc_setup: fail read rsabuf: n=%d len=%d\n", n, len);
			exit(1);
		}
		len += n;
	}
	if (len != SECUREVNC_RSA_PUBKEY_SIZE) {
		fprintf(stderr, "securevnc_setup: fail final read rsabuf: n=%d len=%d\n", n, len);
		exit(1);
	}
	fprintf(stderr, "securevnc_setup: rsa data read len: %d\n", len);
	memcpy(rsasav, rsabuf, SECUREVNC_RSA_PUBKEY_SIZE);

	fprintf(stderr, "securevnc_setup: RSA key has MD5 sum: %s\n", rsa_md5_sum(rsabuf)); 
	fprintf(stderr, "securevnc_setup:\n"); 
	fprintf(stderr, "securevnc_setup: One way to print out the SecureVNC Server key MD5 sum is:\n\n"); 
	fprintf(stderr, "openssl rsa -inform DER -outform DER -pubout -in ./Server_SecureVNC.pkey | dd bs=1 skip=24 | md5sum\n\n");
	if (securevnc_file == NULL) {
		fprintf(stderr, "securevnc_setup:\n");
		fprintf(stderr, "securevnc_setup: ** WARNING: ULTRAVNC SERVER RSA KEY NOT VERIFIED.   **\n");
		fprintf(stderr, "securevnc_setup: ** WARNING: A MAN-IN-THE-MIDDLE ATTACK IS POSSIBLE. **\n");
		fprintf(stderr, "securevnc_setup:\n");
	} else {
		char *q = strrchr(securevnc_file, 'C');
		int skip = 0;
		if (q) {
			if (!strcmp(q, "ClientAuth.pkey")) {
				client_auth = strdup(securevnc_file);
				skip = 1;
			} else if (!strcmp(q, "ClientAuth.pkey.rsa")) {
				client_auth = strdup(securevnc_file);
				q = strrchr(client_auth, '.');
				*q = '\0';
			}
		}
		if (!skip) {
			rc = securevnc_check_server_rsa(securevnc_file, rsabuf);
		}
		if (skip) {
			;
		} else if (rc == 0) {
			fprintf(stderr, "securevnc_setup:\n");
			fprintf(stderr, "securevnc_setup: VERIFY_ERROR: SERVER RSA KEY DID NOT MATCH:\n");
			fprintf(stderr, "securevnc_setup:     %s\n", securevnc_file);
			fprintf(stderr, "securevnc_setup:\n");
			exit(1);
		} else if (rc == -1) {
			fprintf(stderr, "securevnc_setup: User cancelled the save and hence the connection.\n");
			fprintf(stderr, "securevnc_setup:     %s\n", securevnc_file);
			exit(1);
		} else if (rc == 1) {
			fprintf(stderr, "securevnc_setup: VERIFY SUCCESS: server rsa key matches the contents of:\n");
			fprintf(stderr, "securevnc_setup:     %s\n", securevnc_file);
			keystore_verified = 1;
		} else if (rc == 2) {
			fprintf(stderr, "securevnc_setup: Server rsa key stored in:\n");
			fprintf(stderr, "securevnc_setup:     %s\n", securevnc_file);
			keystore_verified = 2;
		}
	}

	/*
	 * read in the server flags. Note that SecureVNCPlugin sends these
	 * in little endian and not network order!!
	 */
	read(server, (char *) &b1, 1);
	read(server, (char *) &b2, 1);
	read(server, (char *) &b3, 1);
	read(server, (char *) &b4, 1);
	
	server_flags = 0;
	server_flags |= ((unsigned int) b4) << 24;
	server_flags |= ((unsigned int) b3) << 16;
	server_flags |= ((unsigned int) b2) << 8;
	server_flags |= ((unsigned int) b1) << 0;
	fprintf(stderr, "securevnc_setup: server_flags: 0x%08x\n", server_flags);

	/* check for arc4 usage: */
	if (server_flags & 0x1) {
		fprintf(stderr, "securevnc_setup: server uses AES cipher.\n");
	} else {
		fprintf(stderr, "securevnc_setup: server uses ARC4 cipher.\n");
		securevnc_arc4 = 1;
		Cipher = EVP_rc4();
	}

	/* check for client auth signature requirement: */
	if (server_flags & (sig << 24)) {
		fprintf(stderr, "securevnc_setup: server requires Client Auth signature.\n");
		client_auth_req = 1;
		if (!client_auth) {
			fprintf(stderr, "securevnc_setup: However, NO *ClientAuth.pkey keyfile was supplied on our\n");
			fprintf(stderr, "securevnc_setup: command line.  Exiting.\n");
			exit(1);
		}
	}

	/*
	 * The first packet 'RFB 003.006' is obscured with key
	 * that is a sha1 hash of public key.  So make this tmp key now:
 	 *
	 */
	initkey = (unsigned char *) calloc(SECUREVNC_KEY_SIZE, 1);
	EVP_BytesToKey(EVP_rc4(), EVP_sha1(), NULL, rsabuf, SECUREVNC_RSA_PUBKEY_SIZE, 1, initkey, NULL);

	/* expand the transported rsabuf into an rsa object */
	rsa = d2i_RSAPublicKey(NULL, (const unsigned char **) &rsabuf, SECUREVNC_RSA_PUBKEY_SIZE);
	if (rsa == NULL) {
		sslexit("securevnc_setup: failed to create rsa");
	}

	/*
	 * Back to the work involving the tmp obscuring key:
	 */
	EVP_CIPHER_CTX_init(&init_ctx);
	rc = EVP_CipherInit_ex(&init_ctx, EVP_rc4(), NULL, initkey, NULL, 1);
	if (rc == 0) {
		sslexit("securevnc_setup: EVP_CipherInit_ex(init_ctx) failed");
	}

	/* for the first obscured packet, read what we can... */
	n = read(server, (char *) buf, BSIZE);
	fprintf(stderr, "securevnc_setup: data read: %d\n", n);
	if (n < 0) {
		exit(1);
	}
	fprintf(stderr, "securevnc_setup: initial data[%d]: ", n);
	
	/* decode with the tmp key */
	if (n > 0) {
		memset(to_viewer, 0, sizeof(to_viewer));
		if (EVP_CipherUpdate(&init_ctx, to_viewer, &len, buf, n) == 0) {
			sslexit("securevnc_setup: EVP_CipherUpdate(init_ctx) failed");
			exit(1);
		}
		to_viewer_len = len;
	}
	EVP_CIPHER_CTX_cleanup(&init_ctx);
	free(initkey);

	/* print what we would send to the viewer (sent below): */
	write(2, to_viewer, 12);	/* and first 12 bytes 'RFB ...' as message */

	/* now create the random session key: */
	encrypted_keybuf = (unsigned char*) calloc(RSA_size(rsa), 1);

	fprintf(stderr, "securevnc_setup: creating random session key: %d/%d\n",
	    SECUREVNC_KEY_SIZE, SECUREVNC_RAND_KEY_SOURCE);
	keydata_len = SECUREVNC_RAND_KEY_SOURCE;

	rc = RAND_bytes((unsigned char *)keydata, SECUREVNC_RAND_KEY_SOURCE);
	if (rc <= 0) {
		fprintf(stderr, "securevnc_setup: RAND_bytes() failed: %s\n", ERR_error_string(ERR_get_error(), NULL));
		rc = RAND_pseudo_bytes((unsigned char *)keydata, SECUREVNC_RAND_KEY_SOURCE);
		fprintf(stderr, "securevnc_setup: RAND_pseudo_bytes() rc=%d\n", rc);
		if (getenv("RANDSTR")) {
			char *s = getenv("RANDSTR"); 
			fprintf(stderr, "securevnc_setup: seeding with RANDSTR len=%d\n", strlen(s));
			RAND_add(s, strlen(s), strlen(s));
		}
	}

	/* N.B. this will be repeated in enc_xfer() setup. */
	EVP_BytesToKey(Cipher, Digest, NULL, (unsigned char *) keydata, keydata_len, 1, keystr, NULL);

	/* encrypt the session key with the server's public rsa key: */
	n = RSA_public_encrypt(SECUREVNC_KEY_SIZE, keystr, encrypted_keybuf, rsa, RSA_PKCS1_PADDING);
	if (n == -1) {
		sslexit("securevnc_setup: RSA_public_encrypt() failed");
		exit(1);
	}
	fprintf(stderr, "securevnc_setup: encrypted session key size: %d. sending to server.\n", n);

	/* send it to the server: */
	write(server, encrypted_keybuf, n);
	free(encrypted_keybuf);

	/*
	 * Reply back with flags indicating cipher (same as one sent to
	 * us) and we do not want client-side auth.
	 *
	 * We send it out on the wire in little endian order:
	 */
	if (securevnc_arc4) {
		write(server, (char *)&zero, 1);
	} else {
		write(server, (char *)&one, 1);
	}
	write(server, (char *)&zero, 1);
	write(server, (char *)&zero, 1);
	if (client_auth_req) {
		write(server, (char *)&sig, 1);
	} else {
		write(server, (char *)&zero, 1);
	}

	if (client_auth_req && client_auth) {
		RSA *client_rsa = load_client_auth(client_auth);
		EVP_MD_CTX dctx;
		unsigned char digest[EVP_MAX_MD_SIZE], *signature;
		unsigned int ndig = 0, nsig = 0;

		if (0) {
			/* for testing only, use the wrong RSA key: */
			client_rsa = RSA_generate_key(2048, 0x10001, NULL, NULL);
		}
		
		if (client_rsa == NULL) {
			fprintf(stderr, "securevnc_setup: problem reading rsa key from '%s'\n", client_auth);
			exit(1);
		}

		EVP_DigestInit(&dctx, EVP_sha1());
		EVP_DigestUpdate(&dctx, keystr, SECUREVNC_KEY_SIZE);
		/*
		 * Without something like the following MITM is still possible.
		 * This is because the MITM knows keystr and can use it with
		 * the server connection as well, and then he just forwards our
		 * signed digest.  The additional information below would be the
		 * MITM's rsa public key, and so the real VNC server will notice
		 * the difference.  And MITM can't sign keystr+server_rsa.pub since
		 * he doesn't have Viewer_ClientAuth.pkey.
		 */
		if (0) {
			EVP_DigestUpdate(&dctx, rsasav, SECUREVNC_RSA_PUBKEY_SIZE);
			if (!keystore_verified) {
				fprintf(stderr, "securevnc_setup:\n");
				fprintf(stderr, "securevnc_setup: Warning: even *WITH* Client Authentication in SecureVNC,\n");
				fprintf(stderr, "securevnc_setup: an attacker may be able to trick you into connecting to his\n");
				fprintf(stderr, "securevnc_setup: fake VNC server and supplying VNC or Windows passwords, etc.\n");
				fprintf(stderr, "securevnc_setup: To increase security manually verify the Server RSA key's MD5\n");
				fprintf(stderr, "securevnc_setup: checksum and then have SSVNC save the key in its keystore to\n");
				fprintf(stderr, "securevnc_setup: be used to verify the server in subsequent connections.\n");
				fprintf(stderr, "securevnc_setup:\n");
			}
		} else {
			if (!keystore_verified) {
				fprintf(stderr, "securevnc_setup:\n");
				fprintf(stderr, "securevnc_setup: WARNING: THE FIRST VERSION OF THE SECUREVNC PROTOCOL IS\n");
				fprintf(stderr, "securevnc_setup: WARNING: BEING USED.  *EVEN* WITH CLIENT AUTHENTICATION IT\n");
				fprintf(stderr, "securevnc_setup: WARNING: IS SUSCEPTIBLE TO A MAN-IN-THE-MIDDLE ATTACK.\n");
				fprintf(stderr, "securevnc_setup: To increase security manually verify the Server RSA key's MD5\n");
				fprintf(stderr, "securevnc_setup: checksum and then have SSVNC save the key in its keystore to\n");
				fprintf(stderr, "securevnc_setup: be used to verify the server in subsequent connections.\n");
				fprintf(stderr, "securevnc_setup:\n");
			}
		}
		EVP_DigestFinal(&dctx, (unsigned char *)digest, &ndig);

		signature = (unsigned char *) calloc(RSA_size(client_rsa), 1);
		RSA_sign(NID_sha1, digest, ndig, signature, &nsig, client_rsa);

		fprintf(stderr, "securevnc_setup: sending ClientAuth.pkey signed data: %d\n", nsig);
		write(server, signature, nsig);
		free(signature);

		RSA_free(client_rsa);
	}

	fprintf(stderr, "securevnc_setup: done.\n");

	/* now send the 'RFB ...' to the viewer */
	if (to_viewer_len > 0) {
		write(viewer, to_viewer, to_viewer_len);
	}
}
/*
 * Listens on incoming port for a client, then connects to remote server.
 * Then forks into two processes one is the encrypter the other the
 * decrypter.
 */
static void enc_connections(int listen_port, char *connect_host, int connect_port) {
	int listen_fd, conn1, conn2, ret, one = 1;
	socklen_t clen;
	struct hostent *hp;
	struct sockaddr_in client, server;

	/* zero means use stdio (preferably from socketpair()) */
	if (listen_port == 0) {
		conn1 = fileno(stdin);
		goto use_stdio;
	}

	/* fd=n,m means use the supplied already established sockets */
	if (sscanf(connect_host, "fd=%d,%d", &conn1, &conn2) == 2) {
		goto use_input_fds;
	}

	/* create the listening socket: */
	memset(&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	if (listen_port < 0) {
		/* negative port means use loopback */
		client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		client.sin_port = htons(-listen_port);
	} else {
		client.sin_addr.s_addr = htonl(INADDR_ANY);
		client.sin_port = htons(listen_port);
	}

	listen_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (listen_fd < 0) {
		perror("socket");
		exit(1);
	}

	ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
	    (char *)&one, sizeof(one));
	if (ret < 0) {
		perror("setsockopt");
		exit(1);
	}

	ret = bind(listen_fd, (struct sockaddr *) &client, sizeof(client));
	if (ret < 0) {
		perror("bind");
		exit(1);
	}

	ret = listen(listen_fd, 2);
	if (ret < 0) {
		perror("listen");
		exit(1);
	}

	fprintf(stderr, "%s: waiting for connection on port: %d\n",
	    prog, listen_port);

	/* wait for a connection: */
	clen = sizeof(client);
	conn1 = accept(listen_fd, (struct sockaddr *) &client, &clen);
	if (conn1 < 0) {
		perror("accept");
		exit(1);
	}

	/* done with the listening socket: */
	close(listen_fd);

	use_stdio:

	fprintf(stderr, "%s: got connection: %d\n", prog, conn1);

	/* now connect to remote server: */
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(connect_port);

	if ((server.sin_addr.s_addr = inet_addr(connect_host)) == htonl(INADDR_NONE)) {
		if (!(hp = gethostbyname(connect_host))) {
			perror("gethostbyname");
			close(conn1);
			exit(1);
		}
		server.sin_addr.s_addr = *(unsigned long *)hp->h_addr;
	}

	conn2 = socket(AF_INET, SOCK_STREAM, 0);
	if (conn2 < 0) {
		perror("socket");
		close(conn1);
		exit(1);
	}

	if (connect(conn2, (struct sockaddr *)&server, (sizeof(server))) < 0) {
		perror("connect");
		close(conn1);
		exit(1);
	}

	use_input_fds:

	if (securevnc) {
		securevnc_setup(conn1, conn2);
	}

	/* fork into two processes; one for each direction: */
	parent = getpid();
	
	child = fork();
	
	if (child == (pid_t) -1) {
		/* couldn't fork... */
		perror("fork");
		close(conn1);
		close(conn2);
		exit(1);
	}

	/* Do transfer/encode/decode loop: */

	if (child == 0) {
		/* encrypter: local-viewer -> remote-server */
		enc_xfer(conn1, conn2, 1);
	} else {
		/* decrypter: remote-server -> local-viewer */
		enc_xfer(conn2, conn1, 0);
	}
}
#endif /* ENC_HAVE_OPENSSL */

extern int main (int argc, char *argv[]) {
	char *kf, *q;

	if (argc < 4) {
		fprintf(stdout, "%s\n", usage);
		exit(1);
	}

	/* guard against pw= on cmdline (e.g. linux) */
	kf = strdup(argv[2]);
	q = strstr(argv[2], "pw=");
	if (q) {
		while (*q != '\0') {
			*q = '\0';	/* now ps(1) won't show it */
			q++;
		}
	}

	enc_do(argv[1], kf, argv[3], argv[4]);

	return 0;
}

/*
 * a crude utility to have this work "keyless" i.e. the vnc password
 * is used instead of a pre-shared key file.
 */

/*

#!/usr/bin/perl
#
# md5_to_rc4key.pl
#
# This program requires md5sum(1) installed on your machine.
#
# It translates a VNC password to a ultravnc dsm plugin 
# compatible key file.
#
# Supply VNC password on cmdline, capture in key file:
#
#	md5_to_rc4key.pl swordfish    > rc4.key
#	md5_to_rc4key.pl -a swordfish > arc4.key
#
# Use rc4.key with ultravnc_dsm_helper in msrc4 mode,
# or arc4.key in either arc4 or aesv4 mode.
#
#
$rfmt = 1;
if ($ARGV[0] eq '-a') {
	$rfmt = 0;
	shift;
}

# n.b. this is not super secure against bad locals...

$pw = shift;
$tmp = "/tmp/md5out.$$";

open(MD5, "| md5sum > $tmp");
print MD5 $pw;
close MD5;

$md5 = `cat $tmp`;
unlink $tmp;

($md5, $junk) = split(/\s/, $md5);

print "128 bit" if $rfmt;
print 'a' x 4	if $rfmt;
print 'b' x 12	if $rfmt;

$str = '';
foreach $d (split(//, $md5)) {
	$str .= $d;
	if (length($str) == 2) {
		push @key, $str;
		$str = '';
	}
}

@key = (reverse @key) if $rfmt;

foreach $h (@key) {
	$c = pack('c', hex("0x$h"));	
	print $c;
}

print 'c' x 48	if $rfmt;

*/
#endif /* _X11VNC_ENC_H */
