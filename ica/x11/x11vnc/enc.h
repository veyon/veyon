#ifndef _X11VNC_ENC_H
#define _X11VNC_ENC_H

/* -- enc.h -- */

#if 0
:r /home/runge/ultraSC/rc4/ultravnc_dsm_helper.c
#endif

/*
 * ultravnc_dsm_helper.c unix/openssl UltraVNC encryption encoder/decoder. 
 *                       (also a generic symmetric encryption tunnel)
 *
 * compile via:

   cc       -O -o ultravnc_dsm_helper ultravnc_dsm_helper.c -lssl -lcrypto
   cc -DDBG -O -o ultravnc_dsm_helper ultravnc_dsm_helper.c -lssl -lcrypto

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
 * Copyright (c) 2008 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
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
 * -----------------------------------------------------------------------
 */

static char *usage = 
    "\n"
    "usage: ultravnc_dsm_helper cipher keyfile listenport remotehost:port\n"
    "\n"
    "e.g.:  ultravnc_dsm_helper arc4 ./arc4.key 5901 snoopy.com:5900\n"
    "\n"
    "       cipher: specify 'msrc4', 'msrc4_sc', 'arc4', 'aesv2',\n"
    "               'aes-cfb', 'aes256', 'blowfish', or '3des'.\n"
    "\n"
    "         'msrc4_sc' enables a workaround for UVNC SC -plugin use.\n"
    "\n"
    "         use '.' to have it try to guess the cipher from the keyfile name.\n"
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
    "                E.g. dd if=/dev/random of=./my.key bs=16 count=1\n"
    "                keyfile can also be pw=<string> to use \"string\" for the key.\n"
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
	int fd, len, listen_port, connect_port, mbits;

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

		} else {
			fprintf(stderr, "cannot figure out cipher, supply 'msrc4', 'arc4', or 'aesv2' ...\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "cannot figure out cipher, supply 'msrc4', 'arc4', or 'aesv2' ...\n");
		exit(1);
	}

	/* set the default message digest (md5) */
	Digest = EVP_md5();

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
	if (stat(keyfile, &sb) != 0) {
		if (strstr(keyfile, "pw=") == keyfile) {
			/* user specified key/password on cmdline */
			int i;
			len = 0;
			pw_in = 1;
			for (i=0; i < strlen(keyfile); i++) {
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
	EVP_CIPHER_CTX *ctx;

	unsigned char buf[BSIZE], out[BSIZE];
	unsigned char *psrc = NULL, *keystr;
	unsigned char salt[SALT+1];
	unsigned char ivec[EVP_MAX_IV_LENGTH];

	int i, cnt, len, m, n = 0, vb = 0, pa = 1, first = 1;
	int whoops = 1; /* for the msrc4 problem */
	char *encstr, *encsym;
	
	/* zero the buffers */
	memset(buf,  0, BSIZE);
	memset(out,  0, BSIZE);
	memset(salt, 0, sizeof(salt));
	memset(ivec, 0, sizeof(ivec));
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

	if (encrypt) {
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

		/* use the encryption context variables below */
		ctx = &E_ctx;
		keystr = E_keystr;

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

		/* use the decryption context variables below */
		ctx = &D_ctx;
		keystr = D_keystr;
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
			    EVP_BytesToKey(Cipher, Digest, NULL, keydata,
			        keydata_len, 1, keystr, NULL);
			    EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, NULL,
			        encrypt);
			} else {
			    /* otherwise keydata as is */
			    EVP_CipherInit_ex(ctx, Cipher, NULL,
			        (unsigned char *) keydata, NULL, encrypt);
			}
		} else {
			/* XXX might not be correct */
			exit(1);
			EVP_BytesToKey(Cipher, Digest, NULL, keydata,
			    keydata_len, 1, keystr, ivec); 
			EVP_CIPHER_CTX_init(ctx);
			EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, ivec,
			    encrypt);
		}

	} else {
		unsigned char *in_salt;

		/* check salt and IV source and size. */
		if (salt_size <= 0) {
			/* let salt_size = 0 mean keep it out of the MD5 */
			fprintf(stderr, "%s: %s - WARNING: no salt\n",
			    prog, encstr);
			in_salt = NULL;
		} else {
			in_salt = salt;
		}
		if (ivec_size < Cipher->iv_len) {
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

			EVP_BytesToKey(Cipher, Digest, in_salt, keydata,
			    keydata_len, 1, keystr, NULL);

		} else {
			/* 
			 * Ultra DSM compatibility mode.  Note that this
			 * clobbers the ivec we set up above!  Under
			 * noultra we overwrite ivec only if ivec_size=0.
			 */
			EVP_BytesToKey(Cipher, Digest, in_salt, keydata,
			    keydata_len, 1, keystr, ivec);
		}


		/* initialize the context */
		EVP_CIPHER_CTX_init(ctx);


		/* set the cipher & initialize */

		/*
		 * XXX N.B.: DSM plugin had encrypt=1 for both
		 * (i.e. perfectly symmetric)
		 */

		EVP_CipherInit_ex(ctx, Cipher, NULL, keystr, ivec, encrypt);
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

/*
 * Listens on incoming port for a client, then connects to remote server.
 * Then forks into two processes one is the encrypter the other the
 * decrypter.
 */
static void enc_connections(int listen_port, char *connect_host, int connect_port) {
	int listen_fd, conn1, conn2, ret, n, one = 1;
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
		fprintf(stderr, "%s\n", usage);
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
