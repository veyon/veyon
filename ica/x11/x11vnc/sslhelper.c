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

/* -- sslhelper.c -- */

#include "x11vnc.h"
#include "inet.h"
#include "cleanup.h"
#include "screen.h"
#include "scan.h"
#include "connections.h"
#include "sslcmds.h"
#include "unixpw.h"
#include "user.h"

#define OPENSSL_INETD   1
#define OPENSSL_VNC     2
#define OPENSSL_VNC6    3
#define OPENSSL_HTTPS   4
#define OPENSSL_HTTPS6  5
#define OPENSSL_REVERSE 6

#define DO_DH 0

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID
#define FORK_OK
#endif
#endif

int openssl_sock = -1;
int openssl_sock6 = -1;
int openssl_port_num = 0;
int https_sock = -1;
int https_sock6 = -1;
pid_t openssl_last_helper_pid = 0;
char *openssl_last_ip = NULL;

static char *certret = NULL;
static int certret_fd = -1;
static mode_t omode;
char *certret_str = NULL;

static char *dhret = NULL;
static int dhret_fd = -1;
char *dhret_str = NULL;
char *new_dh_params = NULL;

void raw_xfer(int csock, int s_in, int s_out);

/* openssl(1) pem related functions: */
char *get_saved_pem(char *string, int create);
char *find_openssl_bin(void);
char *get_ssl_verify_file(char *str_in);
char *create_tmp_pem(char *path, int prompt);

static char *get_input(char *tag, char **in);

char *get_saved_pem(char *save, int create) {
	char *s = NULL, *path, *cdir, *tmp;
	int prompt = 0, len;
	struct stat sbuf;

	if (! save) {
		rfbLog("get_saved_pem: save string is null.\n");
		clean_up_exit(1);
	}

	if (strstr(save, "SAVE_PROMPT") == save) {
		prompt = 1;
		s = save + strlen("SAVE_PROMPT");
	} else if (strstr(save, "SAVE_NOPROMPT") == save) {
		set_env("GENCERT_NOPROMPT", "1");
		s = save + strlen("SAVE_NOPROMPT");
	} else if (strstr(save, "SAVE") == save) {
		s = save + strlen("SAVE");
	} else {
		rfbLog("get_saved_pem: invalid save string: %s\n", save);
		clean_up_exit(1);
	}
	if (strchr(s, '/')) {
		rfbLog("get_saved_pem: invalid save string: %s\n", s);
		clean_up_exit(1);
	}


	cdir = get_Cert_dir(NULL, &tmp);
	if (! cdir || ! tmp) {
		rfbLog("get_saved_pem: could not find Cert dir.\n");
		clean_up_exit(1);
	}

	len = strlen(cdir) + strlen("/server.pem") + strlen(s) + 1;

	path = (char *) malloc(len);
	sprintf(path, "%s/server%s.pem", cdir, s);

	if (stat(path, &sbuf) != 0) {
		char *new_name = NULL;
		if (create) {
			if (inetd || opts_bg) {
				set_env("GENCERT_NOPROMPT", "1");
			}
			new_name = create_tmp_pem(path, prompt);
			if (!getenv("X11VNC_SSL_NO_PASSPHRASE") && !inetd && !opts_bg) {
				sslEncKey(new_name, 0);
			}
		}
		return new_name;
	}

	if (! quiet) {
		char line[1024];
		int on = 0;
		FILE *in = fopen(path, "r");
		if (in != NULL) {
			rfbLog("\n");
			rfbLog("Using SSL Certificate:\n");
			fprintf(stderr, "\n");
			while (fgets(line, 1024, in) != NULL) {
				if (strstr(line, "BEGIN CERTIFICATE")) {
					on = 1;
				}
				if (on) {
					fprintf(stderr, "%s", line);
				}
				if (strstr(line, "END CERTIFICATE")) {
					on = 0;
				}
				if (strstr(line, "PRIVATE KEY")) {
					on = 0;
				}
			}
			fprintf(stderr, "\n");
			fclose(in);
		}
	}
	return strdup(path);
}

static char *get_input(char *tag, char **in) {
	char line[1024], *str;

	if (! tag || ! in || ! *in) {
		return NULL;
	}

	fprintf(stderr, "%s:\n     [%s] ", tag, *in);
	if (fgets(line, 1024, stdin) == NULL) {
		rfbLog("could not read stdin!\n");
		rfbLogPerror("fgets");
		clean_up_exit(1);
	}
	if ((str = strrchr(line, '\n')) != NULL) {
		*str = '\0';
	}
	str = lblanks(line);
	if (!strcmp(str, "")) {
		return *in;
	} else {
		return strdup(line);
	}
}

char *find_openssl_bin(void) {
	char *path, *exe, *p, *gp;
	struct stat sbuf;
	int found_openssl = 0;
	char extra[] = ":/usr/bin:/bin:/usr/sbin:/usr/local/bin"
	    ":/usr/local/sbin:/usr/sfw/bin";
	
	gp = getenv("PATH");
	if (! gp) {
		fprintf(stderr, "could not find openssl(1) program in PATH. (null)\n");
		return NULL;
	}

	path = (char *) malloc(strlen(gp) + strlen(extra) + 1);
	strcpy(path, gp);
	strcat(path, extra);

	/* find openssl binary: */
	exe = (char *) malloc(strlen(path) + strlen("/openssl") + 1);
	p = strtok(path, ":");

	while (p) {
		sprintf(exe, "%s/openssl", p);
		if (stat(exe, &sbuf) == 0) {
			if (! S_ISDIR(sbuf.st_mode)) {
				found_openssl = 1;
				break;
			}
		}
		p = strtok(NULL, ":");
	}
	free(path);

	if (! found_openssl) {
		fprintf(stderr, "could not find openssl(1) program in PATH.\n");
		fprintf(stderr, "PATH=%s\n", gp);
		fprintf(stderr, "(also checked: %s)\n", extra);
		return NULL;
	}
	return exe;
}

/* uses /usr/bin/openssl to create a tmp cert */

char *create_tmp_pem(char *pathin, int prompt) {
	pid_t pid, pidw;
	FILE *in, *out;
	char cnf[] = "/tmp/x11vnc-cnf.XXXXXX";
	char pem[] = "/tmp/x11vnc-pem.XXXXXX";
	char str[8*1024], line[1024], *exe;
	int cnf_fd, pem_fd, status, show_cert = 1;
	char *days;
	char *C, *L, *OU, *O, *CN, *EM;
	char tmpl[] = 
"[ req ]\n"
"prompt = no\n"
"default_bits = 2048\n"
"encrypt_key = yes\n"
"distinguished_name = req_dn\n"
"x509_extensions = cert_type\n"
"\n"
"[ req_dn ]\n"
"countryName=%s\n"
"localityName=%s\n"
"organizationalUnitName=%s\n"
"organizationName=%s\n"
"commonName=%s\n"
"emailAddress=%s\n"
"\n"
"[ cert_type ]\n"
"nsCertType = server\n"
;

	C = strdup("AU");
#ifdef WIN32
	L = strdup("Win32");
	snprintf(line, 1024, "%s-%f", "unknown-node", dnow());
#else
	L = strdup(UT.sysname ? UT.sysname : "unknown-os");
	snprintf(line, 1024, "%s-%f", UT.nodename ? UT.nodename :
	    "unknown-node", dnow());
#endif
	line[1024-1] = '\0';

	OU = strdup(line);
	O = strdup("x11vnc");
	if (pathin) {
		snprintf(line, 1024, "x11vnc-SELF-SIGNED-CERT-%d", getpid());
	} else {
		snprintf(line, 1024, "x11vnc-SELF-SIGNED-TEMPORARY-CERT-%d",
		    getpid());
	}
	line[1024-1] = '\0';
	CN = strdup(line);
	EM = strdup("x11vnc@server.nowhere");

	/* ssl */
	if (no_external_cmds || !cmd_ok("ssl")) {
		rfbLog("create_tmp_pem: cannot run external commands.\n");	
		return NULL;
	}

	rfbLog("\n");	
	if (pathin) {
		rfbLog("Creating a self-signed PEM certificate...\n");	
	} else {
		rfbLog("Creating a temporary, self-signed PEM certificate...\n");	
	}

	rfbLog("\n");	
	rfbLog("This will NOT prevent Man-In-The-Middle attacks UNLESS you\n");	
	rfbLog("get the certificate information to the VNC viewers SSL\n");	
	rfbLog("tunnel configuration or you take the extra steps to sign it\n");
	rfbLog("with a CA key. However, it will prevent passive network\n");
	rfbLog("sniffing.\n");	
	rfbLog("\n");	
	rfbLog("The cert inside -----BEGIN CERTIFICATE-----\n");	
	rfbLog("                           ....\n");	
	rfbLog("                -----END CERTIFICATE-----\n");	
	rfbLog("printed below may be used on the VNC viewer-side to\n");	
	rfbLog("authenticate this server for this session.  See the -ssl\n");
	rfbLog("help output and the FAQ for how to create a permanent\n");
	rfbLog("server certificate.\n");	
	rfbLog("\n");	

	exe = find_openssl_bin();
	if (! exe) {
		return NULL;
	}

	/* create template file with our made up stuff: */
	if (prompt) {
		fprintf(stderr, "\nReply to the following prompts to set"
		    " your Certificate parameters.\n");
		fprintf(stderr, "(press Enter to accept the default in [...], "
		    "or type in the value you want)\n\n");
		C = get_input("CountryName", &C);
		L = get_input("LocalityName", &L);
		OU = get_input("OrganizationalUnitName", &OU);
		O = get_input("OrganizationalName", &O);
		CN = get_input("CommonName", &CN);
		EM = get_input("EmailAddress", &EM);
	}
	sprintf(str, tmpl, C, L, OU, O, CN, EM);

	cnf_fd = mkstemp(cnf);
	if (cnf_fd < 0) {
		return NULL;
	}
	pem_fd = mkstemp(pem);
	if (pem_fd < 0) {
		close(cnf_fd);
		return NULL;
	}

	close(pem_fd);

	write(cnf_fd, str, strlen(str));
	close(cnf_fd);

	if (pathin) {
		days = "365";
	} else {
		days = "30";
	}

#ifndef FORK_OK
	rfbLog("not compiled with fork(2)\n");
	clean_up_exit(1);
#else
	/* make RSA key */
	pid = fork();
	if (pid < 0) {
		return NULL;
	} else if (pid == 0) {
		int i;
		for (i=0; i<256; i++) {
			close(i);
		}
		execlp(exe, exe, "req", "-new", "-x509", "-nodes",
		    "-days", days, "-config", cnf, "-out", pem,
		    "-keyout", pem, (char *)0);
		exit(1);
	}
	pidw = waitpid(pid, &status, 0); 
	if (pidw != pid) {
		return NULL;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		;
	} else {
		return NULL;
	}

#if DO_DH
	/* make DH parameters */
	pid = fork();
	if (pid < 0) {
		return NULL;
	} else if (pid == 0) {
		int i;
		for (i=0; i<256; i++) {
			close(i);
		}
		/* rather slow at 1024 */
		execlp(exe, exe, "dhparam", "-out", cnf, "512", (char *)0);
		exit(1);
	}
	pidw = waitpid(pid, &status, 0); 
	if (pidw != pid) {
		return NULL;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		;
	} else {
		return NULL;
	}

	/* append result: */
	in = fopen(cnf, "r");
	if (in == NULL) {
		return NULL;
	}
	out = fopen(pem, "a");
	if (out == NULL) {
		fclose(in);
		return NULL;
	}
	while (fgets(line, 1024, in) != NULL) {
		fprintf(out, "%s", line);
	}
	fclose(in);
	fclose(out);
#endif

#endif	/* FORK_OK */

	unlink(cnf);
	free(exe);

	if (pathin != NULL) {
		char *q, *pathcrt = strdup(pathin);
		FILE *crt = NULL;
		int on = 0;

		q = strrchr(pathcrt, '/');
		if (q) {
			q = strstr(q, ".pem");
			if (q) {
				*(q+1) = 'c';
				*(q+2) = 'r';
				*(q+3) = 't';
				crt = fopen(pathcrt, "w");
			}
		}
		if (crt == NULL) {
			rfbLog("could not open: %s\n", pathcrt);
			rfbLogPerror("fopen");
			return NULL;
		}

		out = fopen(pathin, "w");
		chmod(pathin,  0600);
		if (out == NULL) {
			rfbLog("could not open: %s\n", pathin);
			rfbLogPerror("fopen");
			fclose(crt);
			return NULL;
		}

		in = fopen(pem, "r");
		if (in == NULL) {
			rfbLog("could not open: %s\n", pem);
			rfbLogPerror("fopen");
			fclose(out);
			fclose(crt);
			unlink(pathin);
			unlink(pathcrt);
			return NULL;
		}
		while (fgets(line, 1024, in) != NULL) {
			if (strstr(line, "BEGIN CERTIFICATE")) {
				on = 1;
			}
			fprintf(out, "%s", line);
			if (on) {
				fprintf(crt, "%s", line);
				if (!quiet) {
					fprintf(stderr, "%s", line);
				}
			}
			if (strstr(line, "END CERTIFICATE")) {
				on = 0;
			}
			if (strstr(line, "PRIVATE KEY")) {
				on = 0;
			}
		}
		fclose(in);
		fclose(out);
		fclose(crt);
	}

	if (show_cert) {
		exe = find_openssl_bin();
		if (!exe) {
			exe = strdup("openssl");
		}
		if (strlen(pem) + strlen(exe) < 4000) {
			char cmd[5000];
			if (inetd) {
				sprintf(cmd, "%s x509 -text -in '%s' 1>&2", exe, pem);
			} else {
				sprintf(cmd, "%s x509 -text -in '%s'", exe, pem);
			}
			fprintf(stderr, "\n");
			system(cmd);
			fprintf(stderr, "\n");
		}
		free(exe);
	}

	if (pathin) {
		unlink(pem);
		return strdup(pathin);
	} else {
		return strdup(pem);
	}
}

static int appendfile(FILE *out, char *infile) {
	char line[1024];
	FILE *in;

	if (! infile) {
		rfbLog("appendfile: null infile.\n");
		return 0;
	}
	if (! out) {
		rfbLog("appendfile: null out handle.\n");
		return 0;
	}

	in = fopen(infile, "r");

	if (in == NULL) {
		rfbLog("appendfile: %s\n", infile);
		rfbLogPerror("fopen");
		return 0;
	}

	while (fgets(line, 1024, in) != NULL) {
		fprintf(out, "%s", line);
	}
	fclose(in);
	return 1;
}
	
char *get_ssl_verify_file(char *str_in) {
	char *p, *str, *cdir, *tmp;
	char *tfile, *tfile2;
	FILE *file;
	struct stat sbuf;
	int count = 0, fd;

	if (! str_in) {
		rfbLog("get_ssl_verify_file: no filename\n");
		exit(1);
	}

	if (stat(str_in, &sbuf) == 0) {
		/* assume he knows what he is doing. */
		return str_in;
	}

	cdir = get_Cert_dir(NULL, &tmp);
	if (! cdir || ! tmp) {
		rfbLog("get_ssl_verify_file: invalid cert-dir.\n");
		exit(1);
	}

	tfile  = (char *) malloc(strlen(tmp) + 1024);
	tfile2 = (char *) malloc(strlen(tmp) + 1024);

	sprintf(tfile, "%s/sslverify-tmp-load-%d.crts.XXXXXX", tmp, getpid());

	fd = mkstemp(tfile);
	if (fd < 0) {
		rfbLog("get_ssl_verify_file: %s\n", tfile);
		rfbLogPerror("mkstemp");
		exit(1);
	}
	close(fd);

	file = fopen(tfile, "w");
	chmod(tfile, 0600);
	if (file == NULL) {
		rfbLog("get_ssl_verify_file: %s\n", tfile);
		rfbLogPerror("fopen");
		exit(1);
	}

	str = strdup(str_in);
	p = strtok(str, ",");

	while (p) {
		if (!strcmp(p, "CA")) {
			sprintf(tfile2, "%s/CA/cacert.pem", cdir);
			if (! appendfile(file, tfile2)) {
				unlink(tfile);
				exit(1);
			}
			rfbLog("sslverify: loaded %s\n", tfile2);
			count++;

		} else if (!strcmp(p, "clients")) {
			DIR *dir;
			struct dirent *dp;

			sprintf(tfile2, "%s/clients", cdir);
			dir = opendir(tfile2);
			if (! dir) {
				rfbLog("get_ssl_verify_file: %s\n", tfile2);
				rfbLogPerror("opendir");
				unlink(tfile);
				exit(1);
			}
			while ( (dp = readdir(dir)) != NULL) {
				char *n = dp->d_name;
				char *q = strstr(n, ".crt");

				if (! q || strlen(q) != strlen(".crt")) {
					continue;
				}
				if (strlen(n) > 512) {
					continue;
				}

				sprintf(tfile2, "%s/clients/%s", cdir, n);
				if (! appendfile(file, tfile2)) {
					unlink(tfile);
					exit(1);
				}
				rfbLog("sslverify: loaded %s\n",
				    tfile2);
				count++;
			}
			closedir(dir);
			
		} else {
			if (strlen(p) > 512) {
				unlink(tfile);
				exit(1);
			}
			sprintf(tfile2, "%s/clients/%s.crt", cdir, p);
			if (stat(tfile2, &sbuf) != 0) {
				sprintf(tfile2, "%s/clients/%s", cdir, p);
			}
			if (! appendfile(file, tfile2)) {
				unlink(tfile);
				exit(1);
			}
			rfbLog("sslverify: loaded %s\n", tfile2);
			count++;
		}
		p = strtok(NULL, ",");
	}
	fclose(file);
	free(tfile2);
	free(str);

	rfbLog("sslverify: using %d client certs in\n", count);
	rfbLog("sslverify: %s\n", tfile);

	return tfile;
}

int openssl_present(void);
void openssl_init(int isclient);
void openssl_port(int restart);
void https_port(int restart);
void check_openssl(void);
void check_https(void);
void ssl_helper_pid(pid_t pid, int sock);
void accept_openssl(int mode, int presock);

static void lose_ram(void);
#define ABSIZE 16384

static int vencrypt_selected = 0;
static int anontls_selected = 0;

/* to test no openssl libssl */
#if 0
#undef LIBVNCSERVER_HAVE_LIBSSL
#define LIBVNCSERVER_HAVE_LIBSSL 0
#endif

#if !LIBVNCSERVER_HAVE_LIBSSL

static void badnews(char *name) {
	use_openssl = 0;
	use_stunnel = 0;
	rfbLog("** %s: not compiled with libssl OpenSSL support **\n", name ? name : "???");
	clean_up_exit(1);
}

int openssl_present(void) {return 0;}
void openssl_init(int isclient) {badnews("openssl_init");}

#define SSL_ERROR_NONE 0

static int ssl_init(int s_in, int s_out, int skip_vnc_tls, double last_https) {
	if (enc_str != NULL) {
		return 1;
	}
	badnews("ssl_init");
	return 0;
}

static void ssl_xfer(int csock, int s_in, int s_out, int is_https) {
	if (enc_str != NULL && !strcmp(enc_str, "none")) {
		usleep(250*1000);
		rfbLog("doing '-enc none' raw transfer (no encryption)\n"); 
		raw_xfer(csock, s_in, s_out);
	} else {
		badnews("ssl_xfer");
	}
}

#else 	/* LIBVNCSERVER_HAVE_LIBSSL */

/*
 * This is because on older systems both zlib.h and ssl.h define
 * 'free_func' nothing we do below (currently) induces an external
 * dependency on 'free_func'.
 */
#define free_func my_jolly_little_free_func

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

static SSL_CTX *ctx = NULL;
static RSA *rsa_512 = NULL;
static RSA *rsa_1024 = NULL;
static SSL *ssl = NULL;
static X509_STORE *revocation_store = NULL;


static void init_prng(void);
static void sslerrexit(void);
static int  ssl_init(int s_in, int s_out, int skip_vnc_tls, double last_https);
static void ssl_xfer(int csock, int s_in, int s_out, int is_https);

#ifndef FORK_OK
void openssl_init(int isclient) {
	rfbLog("openssl_init: fork is not supported. cannot create"
	    " ssl helper process.\n");
	clean_up_exit(1);
}
int openssl_present(void) {return 0;}

#else

int openssl_present(void) {return 1;}

static void sslerrexit(void) {
	unsigned long err = ERR_get_error();
	
	if (err) {
		char str[256];
		ERR_error_string(err, str);
		fprintf(stderr, "ssl error: %s\n", str);
	}
	clean_up_exit(1);
}

static int pem_passwd_callback(char *buf, int size, int rwflag,
    void *userdata) {
	char *q, line[1024];

	if (! buf) {
		exit(1);
	}

	fprintf(stderr, "\nA passphrase is needed to unlock an OpenSSL "
	    "private key (PEM file).\n");
	fprintf(stderr, "Enter passphrase> ");
	system("stty -echo");
	if(fgets(line, 1024, stdin) == NULL) {
		fprintf(stdout, "\n");
		system("stty echo");
		exit(1);
	}
	system("stty echo");
	fprintf(stdout, "\n\n");
	q = strrchr(line, '\n');
	if (q) {
		*q = '\0';
	}
	line[1024 - 1] = '\0';
	strncpy(buf, line, size);                                  
	buf[size - 1] = '\0';

	if (0) rwflag = 0;	/* compiler warning. */
	if (0) userdata = 0;	/* compiler warning. */

	return strlen(buf);
}

/* based on mod_ssl */
static int crl_callback(X509_STORE_CTX *callback_ctx) {
	X509_STORE_CTX store_ctx;
	X509_OBJECT obj;
	X509_NAME *subject;
	X509_NAME *issuer;
	X509 *xs;
	X509_CRL *crl;
	X509_REVOKED *revoked;
	EVP_PKEY *pubkey;
	long serial;
	BIO *bio;
	int i, n, rc;
	char *cp, *cp2;
	ASN1_TIME *t;
	
	/* Determine certificate ingredients in advance */
	xs      = X509_STORE_CTX_get_current_cert(callback_ctx);
	subject = X509_get_subject_name(xs);
	issuer  = X509_get_issuer_name(xs);
	
	/* Try to retrieve a CRL corresponding to the _subject_ of
	* the current certificate in order to verify it's integrity. */
	memset((char *)&obj, 0, sizeof(obj));
	X509_STORE_CTX_init(&store_ctx, revocation_store, NULL, NULL);
	rc=X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, subject, &obj);
	X509_STORE_CTX_cleanup(&store_ctx);
	crl=obj.data.crl;

	if(rc>0 && crl) {
		/* Log information about CRL
		 * (A little bit complicated because of ASN.1 and BIOs...) */
		bio=BIO_new(BIO_s_mem());
		BIO_printf(bio, "lastUpdate: ");
		ASN1_UTCTIME_print(bio, X509_CRL_get_lastUpdate(crl));
		BIO_printf(bio, ", nextUpdate: ");
		ASN1_UTCTIME_print(bio, X509_CRL_get_nextUpdate(crl));
		n=BIO_pending(bio);
		cp=malloc(n+1);
		n=BIO_read(bio, cp, n);
		cp[n]='\0';
		BIO_free(bio);
		cp2=X509_NAME_oneline(subject, NULL, 0);
		rfbLog("CA CRL: Issuer: %s, %s\n", cp2, cp);
		OPENSSL_free(cp2);
		free(cp);

		/* Verify the signature on this CRL */
		pubkey=X509_get_pubkey(xs);
		if(X509_CRL_verify(crl, pubkey)<=0) {
			rfbLog("Invalid signature on CRL\n");
			X509_STORE_CTX_set_error(callback_ctx,
				X509_V_ERR_CRL_SIGNATURE_FAILURE);
			X509_OBJECT_free_contents(&obj);
			if(pubkey)
				EVP_PKEY_free(pubkey);
			return 0; /* Reject connection */
		}
		if(pubkey)
			EVP_PKEY_free(pubkey);

		/* Check date of CRL to make sure it's not expired */
		t=X509_CRL_get_nextUpdate(crl);
		if(!t) {
			rfbLog("Found CRL has invalid nextUpdate field\n");
			X509_STORE_CTX_set_error(callback_ctx,
				X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD);
			X509_OBJECT_free_contents(&obj);
			return 0; /* Reject connection */
		}
		if(X509_cmp_current_time(t)<0) {
			rfbLog("Found CRL is expired - "
				"revoking all certificates until you get updated CRL\n");
			X509_STORE_CTX_set_error(callback_ctx, X509_V_ERR_CRL_HAS_EXPIRED);
			X509_OBJECT_free_contents(&obj);
			return 0; /* Reject connection */
		}
		X509_OBJECT_free_contents(&obj);
	}

	/* Try to retrieve a CRL corresponding to the _issuer_ of
	 * the current certificate in order to check for revocation. */
	memset((char *)&obj, 0, sizeof(obj));
	X509_STORE_CTX_init(&store_ctx, revocation_store, NULL, NULL);
	rc=X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, issuer, &obj);
	X509_STORE_CTX_cleanup(&store_ctx);
	crl=obj.data.crl;

	if(rc>0 && crl) {
		/* Check if the current certificate is revoked by this CRL */
		n=sk_X509_REVOKED_num(X509_CRL_get_REVOKED(crl));
		for(i=0; i<n; i++) {
			revoked=sk_X509_REVOKED_value(X509_CRL_get_REVOKED(crl), i);
			if(ASN1_INTEGER_cmp(revoked->serialNumber,
					X509_get_serialNumber(xs)) == 0) {
				serial=ASN1_INTEGER_get(revoked->serialNumber);
				cp=X509_NAME_oneline(issuer, NULL, 0);
				rfbLog("Certificate with serial %ld (0x%lX) "
					"revoked per CRL from issuer %s\n", serial, serial, cp);
				OPENSSL_free(cp);
				X509_STORE_CTX_set_error(callback_ctx, X509_V_ERR_CERT_REVOKED);
				X509_OBJECT_free_contents(&obj);
				return 0; /* Reject connection */
			}
		}
		X509_OBJECT_free_contents(&obj);
	}

	return 1; /* Accept connection */
}

static int verify_callback(int ok, X509_STORE_CTX *callback_ctx) {
	if (!ssl_verify) {
		rfbLog("CRL_check: skipped.\n");
		return ok;
	}
	if (!ssl_crl) {
		rfbLog("CRL_check: skipped.\n");
		return ok;
	}
	if (!ok) {
		rfbLog("CRL_check: client cert is already rejected.\n");
		return ok;
	}
	if (revocation_store) {
		if (crl_callback(callback_ctx)) {
			rfbLog("CRL_check: succeeded.\n");
			return 1;
		} else {
			rfbLog("CRL_check: did not pass.\n");
			return 0;
		}
	}
	/* NOTREACHED */
	return 1;
}

#define rfbSecTypeAnonTls  18
#define rfbSecTypeVencrypt 19

#define rfbVencryptPlain	256
#define rfbVencryptTlsNone	257
#define rfbVencryptTlsVnc	258
#define rfbVencryptTlsPlain	259
#define rfbVencryptX509None	260
#define rfbVencryptX509Vnc	261
#define rfbVencryptX509Plain	262

static int ssl_client_mode = 0;

static int switch_to_anon_dh(void);

void openssl_init(int isclient) {
	int db = 0, tmp_pem = 0, do_dh;
	FILE *in;
	double ds;
	long mode;
	static int first = 1;

	do_dh = DO_DH;

	if (enc_str != NULL) {
		if (first) {
			init_prng();
		}
		first = 0;
		return;
	}

	if (! quiet) {
		rfbLog("\n");
		rfbLog("Initializing SSL (%s connect mode).\n", isclient ? "client":"server");
	}
	if (first) {
		if (db) fprintf(stderr, "\nSSL_load_error_strings()\n");

		SSL_load_error_strings();

		if (db) fprintf(stderr, "SSL_library_init()\n");

		SSL_library_init();

		if (db) fprintf(stderr, "init_prng()\n");

		init_prng();

		first = 0;
	}

	if (isclient) {
		ssl_client_mode = 1;
	} else {
		ssl_client_mode = 0;
	}

	if (ssl_client_mode) {
		if (db) fprintf(stderr, "SSLv23_client_method()\n");
		ctx = SSL_CTX_new( SSLv23_client_method() );
	} else {
		if (db) fprintf(stderr, "SSLv23_server_method()\n");
		ctx = SSL_CTX_new( SSLv23_server_method() );
	}

	if (ctx == NULL) {
		rfbLog("openssl_init: SSL_CTX_new failed.\n");	
		sslerrexit();
	}

	ds = dnow();
	rsa_512 = RSA_generate_key(512, RSA_F4, NULL, NULL);
	if (rsa_512 == NULL) {
		rfbLog("openssl_init: RSA_generate_key(512) failed.\n");	
		sslerrexit();
	}

	rfbLog("created  512 bit temporary RSA key: %.3fs\n", dnow() - ds);

	ds = dnow();
	rsa_1024 = RSA_generate_key(1024, RSA_F4, NULL, NULL);
	if (rsa_1024 == NULL) {
		rfbLog("openssl_init: RSA_generate_key(1024) failed.\n");	
		sslerrexit();
	}

	rfbLog("created 1024 bit temporary RSA key: %.3fs\n", dnow() - ds);

	if (db) fprintf(stderr, "SSL_CTX_set_tmp_rsa()\n");

	if (! SSL_CTX_set_tmp_rsa(ctx, rsa_1024)) {
		rfbLog("openssl_init: SSL_CTX_set_tmp_rsa(1024) failed.\n");	
		sslerrexit();
	}

	mode = 0;
	mode |= SSL_MODE_ENABLE_PARTIAL_WRITE;
	mode |= SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
	SSL_CTX_set_mode(ctx, mode);

#define ssl_cache 0
#if ssl_cache
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout(ctx, 300);
#else
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
	SSL_CTX_set_timeout(ctx, 1);
#endif

	ds = dnow();
	if (! openssl_pem) {
		openssl_pem = create_tmp_pem(NULL, 0);
		if (! openssl_pem) {
			rfbLog("openssl_init: could not create temporary,"
			    " self-signed PEM.\n");	
			clean_up_exit(1);
		}
		tmp_pem = 1;

	} else if (!strcmp(openssl_pem, "ANON")) {
		if (ssl_verify) {
			rfbLog("openssl_init: Anonymous Diffie-Hellman cannot"
			    " be used in -sslverify mode.\n");	
			clean_up_exit(1);
		}
		if (ssl_crl) {
			rfbLog("openssl_init: Anonymous Diffie-Hellman cannot"
			    " be used in -sslCRL mode.\n");	
			clean_up_exit(1);
		}
		/* n.b. new ctx */
		if (!switch_to_anon_dh()) {
			rfbLog("openssl_init: Anonymous Diffie-Hellman setup"
			    " failed.\n");	
			clean_up_exit(1);
		}
	} else if (strstr(openssl_pem, "SAVE") == openssl_pem) {
		openssl_pem = get_saved_pem(openssl_pem, 1);
		if (! openssl_pem) {
			rfbLog("openssl_init: could not create or open"
			    " saved PEM: %s\n", openssl_pem);	
			clean_up_exit(1);
		}
		tmp_pem = 0;
	}

	rfbLog("using PEM %s  %.3fs\n", openssl_pem, dnow() - ds);

	SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_callback);

	if (do_dh) {
		DH *dh;
		BIO *bio;

		ds = dnow();
		in = fopen(openssl_pem, "r");
		if (in == NULL) {
			rfbLogPerror("fopen");
			clean_up_exit(1);
		}
		bio = BIO_new_fp(in, BIO_CLOSE|BIO_FP_TEXT);
		if (! bio) {
			rfbLog("openssl_init: BIO_new_fp() failed.\n");	
			sslerrexit();
		}
		dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		if (dh == NULL) {
			rfbLog("openssl_init: PEM_read_bio_DHparams() failed.\n");	
			BIO_free(bio);
			sslerrexit();
		}
		BIO_free(bio);
		SSL_CTX_set_tmp_dh(ctx, dh);
		rfbLog("loaded Diffie Hellman %d bits, %.3fs\n",
		    8*DH_size(dh), dnow()-ds);
		DH_free(dh);
	}

	if (strcmp(openssl_pem, "ANON")) {
		if (! SSL_CTX_use_certificate_chain_file(ctx, openssl_pem)) {
			rfbLog("openssl_init: SSL_CTX_use_certificate_chain_file() failed.\n");	
			sslerrexit();
		}
		if (! SSL_CTX_use_RSAPrivateKey_file(ctx, openssl_pem,
		    SSL_FILETYPE_PEM)) {
			rfbLog("openssl_init: SSL_CTX_set_tmp_rsa(1024) failed.\n");	
			sslerrexit();
		}
		if (! SSL_CTX_check_private_key(ctx)) {
			rfbLog("openssl_init: SSL_CTX_set_tmp_rsa(1024) failed.\n");	
			sslerrexit();
		}
	}

	if (tmp_pem && ! getenv("X11VNC_KEEP_TMP_PEM")) {
		if (getenv("X11VNC_SHOW_TMP_PEM")) {
			FILE *in = fopen(openssl_pem, "r");
			if (in != NULL) {
				char line[128];
				fprintf(stderr, "\n");
				while (fgets(line, 128, in) != NULL) {
					fprintf(stderr, "%s", line);
				}
				fprintf(stderr, "\n");
				fclose(in);
			}
		}
		unlink(openssl_pem);
		free(openssl_pem);
		openssl_pem = NULL;
	}

	if (ssl_crl) {
		struct stat sbuf;
		X509_LOOKUP *lookup;

		if (stat(ssl_crl, &sbuf) != 0) {
			rfbLog("openssl_init: -sslCRL does not exist %s.\n",
			    ssl_crl ? ssl_crl : "null");	
			rfbLogPerror("stat");
			clean_up_exit(1);
		}

		revocation_store = X509_STORE_new();
		if (!revocation_store) {
			rfbLog("openssl_init: X509_STORE_new failed.\n");
			sslerrexit();
		}
		if (! S_ISDIR(sbuf.st_mode)) {
			lookup = X509_STORE_add_lookup(revocation_store, X509_LOOKUP_file());
			if (!lookup) {
				rfbLog("openssl_init: X509_STORE_add_lookup failed.\n");
				sslerrexit();
			}
			if (!X509_LOOKUP_load_file(lookup, ssl_crl, X509_FILETYPE_PEM))  {
				rfbLog("openssl_init: X509_LOOKUP_load_file failed.\n");
				sslerrexit();
			}
		} else {
			lookup = X509_STORE_add_lookup(revocation_store, X509_LOOKUP_hash_dir());
			if (!lookup) {
				rfbLog("openssl_init: X509_STORE_add_lookup failed.\n");
				sslerrexit();
			}
			if (!X509_LOOKUP_add_dir(lookup, ssl_crl, X509_FILETYPE_PEM))  {
				rfbLog("openssl_init: X509_LOOKUP_add_dir failed.\n");
				sslerrexit();
			}
		}
		rfbLog("loaded CRL file: %s\n", ssl_crl);
	}

	if (ssl_verify) {
		struct stat sbuf;
		char *file;
		int lvl;

		file = get_ssl_verify_file(ssl_verify);

		if (!file || stat(file, &sbuf) != 0) {
			rfbLog("openssl_init: -sslverify does not exist %s.\n",
			    file ? file : "null");	
			rfbLogPerror("stat");
			clean_up_exit(1);
		}
		if (! S_ISDIR(sbuf.st_mode)) {
			if (! SSL_CTX_load_verify_locations(ctx, file, NULL)) {
				rfbLog("openssl_init: SSL_CTX_load_verify_"
				    "locations() failed.\n");	
				sslerrexit();
			}
		} else {
			if (! SSL_CTX_load_verify_locations(ctx, NULL, file)) {
				rfbLog("openssl_init: SSL_CTX_load_verify_"
				    "locations() failed.\n");	
				sslerrexit();
			}
		}

		lvl = SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_PEER;
		if (ssl_crl == NULL) {
			SSL_CTX_set_verify(ctx, lvl, NULL);
		} else {
			SSL_CTX_set_verify(ctx, lvl, verify_callback);
		}
		if (strstr(file, "/sslverify-tmp-load-")) {
			/* temporary file */
			unlink(file);
		}
	} else {
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	}

	rfbLog("\n");
}

static int read_exact(int sock, char *buf, int len) {
	int n, fail = 0;
	if (sock < 0) {
		return 0;
	}
	while (len > 0) {
		n = read(sock, buf, len);
		if (n > 0) {
			buf += n;
			len -= n;
		} else if (n == 0) {
			fail = 1;
			break;
		} else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			usleep(10*1000);
		} else if (n < 0 && errno != EINTR) {
			fail = 1;
			break;
		}
	}
	if (fail) {
		return 0;
	} else {
		return 1;
	}
}

static int write_exact(int sock, char *buf, int len) {
	int n, fail = 0;
	if (sock < 0) {
		return 0;
	}
	while (len > 0) {
		n = write(sock, buf, len);
		if (n > 0) {
			buf += n;
			len -= n;
		} else if (n == 0) {
			fail = 1;
			break;
		} else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			usleep(10*1000);
		} else if (n < 0 && errno != EINTR) {
			fail = 1;
			break;
		}
	}
	if (fail) {
		return 0;
	} else {
		return 1;
	}
}

/* XXX not in rfb.h: */
void rfbClientSendString(rfbClientPtr cl, char *reason);

static int finish_auth(rfbClientPtr client, char *type) {
	int security_result, ret;

	ret = 0;

if (getenv("X11VNC_DEBUG_TLSPLAIN")) fprintf(stderr, "finish_auth type=%s\n", type);

	if (!strcmp(type, "None")) {
		security_result = 0;	/* success */
		if (write_exact(client->sock, (char *) &security_result, 4)) {
			ret = 1;
		}
		rfbLog("finish_auth: using auth 'None'\n");
		client->state = RFB_INITIALISATION;

	} else if (!strcmp(type, "Vnc")) {
		RAND_bytes(client->authChallenge, CHALLENGESIZE);
		if (write_exact(client->sock, (char *) &client->authChallenge, CHALLENGESIZE)) {
			ret = 1;
		}
		rfbLog("finish_auth: using auth 'Vnc', sent challenge.\n");
		client->state = RFB_AUTHENTICATION;

	} else if (!strcmp(type, "Plain")) {
		if (!unixpw) {
			rfbLog("finish_auth: *Plain not allowed outside unixpw mode.\n");
			ret = 0;
		} else {
			char *un, *pw;
			int unlen, pwlen;

if (getenv("X11VNC_DEBUG_TLSPLAIN")) fprintf(stderr, "*Plain begin: onHold=%d client=%p unixpw_client=%p\n", client->onHold, (void *) client, (void *) unixpw_client);

			if (!read_exact(client->sock, (char *)&unlen, 4)) goto fail;
			unlen = Swap32IfLE(unlen);

if (getenv("X11VNC_DEBUG_TLSPLAIN")) fprintf(stderr, "unlen: %d\n", unlen);

			if (!read_exact(client->sock, (char *)&pwlen, 4)) goto fail;
			pwlen = Swap32IfLE(pwlen);

if (getenv("X11VNC_DEBUG_TLSPLAIN")) fprintf(stderr, "pwlen: %d\n", pwlen);

			un = (char *) malloc(unlen+1);
			memset(un, 0, unlen+1);

			pw = (char *) malloc(pwlen+2);
			memset(pw, 0, pwlen+2);

			if (!read_exact(client->sock, un, unlen)) goto fail;
			if (!read_exact(client->sock, pw, pwlen)) goto fail;

if (getenv("X11VNC_DEBUG_TLSPLAIN")) fprintf(stderr, "*Plain: %d %d '%s' ... \n", unlen, pwlen, un);
			strcat(pw, "\n");

			if (unixpw_verify(un, pw)) {
				security_result = 0;	/* success */
				if (write_exact(client->sock, (char *) &security_result, 4)) {
					ret = 1;
					unixpw_verify_screen(un, pw);
				}
				client->onHold = FALSE;
				client->state = RFB_INITIALISATION;
			}
			if (ret == 0) {
				rfbClientSendString(client, "unixpw failed");
			}

			memset(un, 0, unlen+1);
			memset(pw, 0, pwlen+2);
			free(un);
			free(pw);
		}
	} else {
		rfbLog("finish_auth: unknown sub-type: %s\n", type);
		ret = 0;
	}

	fail:
	return ret;
}

static int finish_vencrypt_auth(rfbClientPtr client, int subtype) {

	if (subtype == rfbVencryptTlsNone || subtype == rfbVencryptX509None) {
		return finish_auth(client, "None");
	} else if (subtype == rfbVencryptTlsVnc || subtype == rfbVencryptX509Vnc) {
		return finish_auth(client, "Vnc");
	} else if (subtype == rfbVencryptTlsPlain || subtype == rfbVencryptX509Plain) {
		return finish_auth(client, "Plain");
	} else {
		rfbLog("finish_vencrypt_auth: unknown sub-type: %d\n", subtype);
		return 0;
	}
}


static int add_anon_dh(void) {
	pid_t pid, pidw;
	char cnf[] = "/tmp/x11vnc-dh.XXXXXX";
	char *infile = NULL;
	int status, cnf_fd;
	DH *dh;
	BIO *bio;
	FILE *in;
	double ds;
	/*
	 * These are dh parameters (prime, generator), not dh keys.
	 * Evidently it is ok for them to be publicly known.
	 * openssl dhparam -out dh.out 1024
	 */
	char *fixed_dh_params = 
"-----BEGIN DH PARAMETERS-----\n"
"MIGHAoGBAL28w69ZnLYBvp8R2OeqtAIms+oatY19iBL4WhGI/7H1OMmkJjIe+OHs\n"
"PXoJfe5ucrnvno7Xm+HJZYa1jnPGQuWoa/VJKXdVjYdJVNzazJKM2daKKcQA4GDc\n"
"msFS5DxLbzUR5jy1n12K3EcbvpyFqDYVTJJXm7NuNuiWRfz3wTozAgEC\n"
"-----END DH PARAMETERS-----\n";

	if (dhparams_file != NULL) {
		infile = dhparams_file;
		rfbLog("add_anon_dh: using %s\n", dhparams_file);
		goto readin;
	}

	cnf_fd = mkstemp(cnf);
	if (cnf_fd < 0) {
		return 0;
	}
	infile = cnf;

	if (create_fresh_dhparams) {

		if (new_dh_params != NULL) {
			write(cnf_fd, new_dh_params, strlen(new_dh_params));
			close(cnf_fd);
		} else {
			char *exe = find_openssl_bin();
			struct stat sbuf;

			if (no_external_cmds || !cmd_ok("ssl")) {
				rfbLog("add_anon_dh: cannot run external commands.\n");	
				return 0;
			}

			close(cnf_fd);
			if (exe == NULL) {
				return 0;
			}
			ds = dnow();
			pid = fork();
			if (pid < 0) {
				return 0;
			} else if (pid == 0) {
				int i;
				for (i=0; i<256; i++) {
					if (i == 2) continue;
					close(i);
				}
				/* rather slow at 1024 */
				execlp(exe, exe, "dhparam", "-out", cnf, "1024", (char *)0);
				exit(1);
			}
			pidw = waitpid(pid, &status, 0); 
			if (pidw != pid) {
				return 0;
			}
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
				;
			} else {
				return 0;
			}
			rfbLog("add_anon_dh: created new DH params in %.3f secs\n", dnow() - ds);

			if (stat(cnf, &sbuf) == 0 && sbuf.st_size > 0) {
				/* save it to reuse during our process's lifetime: */
				int d = open(cnf, O_RDONLY);
				if (d >= 0) {
					int n, len = sbuf.st_size;
					new_dh_params = (char *) calloc(len+1, 1);
					n = read(d, new_dh_params, len);
					close(d);
					if (n != len) {
						free(new_dh_params);
						new_dh_params = NULL;
					} else if (dhret != NULL) {
						d = open(dhret, O_WRONLY);
						if (d >= 0) {
							write(d, new_dh_params, strlen(new_dh_params));
							close(d);
						}
					}
				}
			}
		}
	} else {
		write(cnf_fd, fixed_dh_params, strlen(fixed_dh_params));
		close(cnf_fd);
	}

	readin:

	ds = dnow();
	in = fopen(infile, "r");

	if (in == NULL) {
		rfbLogPerror("fopen");
		unlink(cnf);
		return 0;
	}
	bio = BIO_new_fp(in, BIO_CLOSE|BIO_FP_TEXT);
	if (! bio) {
		rfbLog("openssl_init: BIO_new_fp() failed.\n");	
		unlink(cnf);
		return 0;
	}
	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
	if (dh == NULL) {
		rfbLog("openssl_init: PEM_read_bio_DHparams() failed.\n");	
		unlink(cnf);
		BIO_free(bio);
		return 0;
	}
	BIO_free(bio);
	SSL_CTX_set_tmp_dh(ctx, dh);
	rfbLog("loaded Diffie Hellman %d bits, %.3fs\n", 8*DH_size(dh), dnow()-ds);
	DH_free(dh);

	unlink(cnf);
	return 1;
}

static int switch_to_anon_dh(void) {
	long mode;
	
	rfbLog("Using Anonymous Diffie-Hellman mode.\n");
	rfbLog("WARNING: Anonymous Diffie-Hellman uses encryption but is\n");
	rfbLog("WARNING: susceptible to a Man-In-The-Middle attack.\n");
	if (ssl_client_mode) {
		ctx = SSL_CTX_new( SSLv23_client_method() );
	} else {
		ctx = SSL_CTX_new( SSLv23_server_method() );
	}
	if (ctx == NULL) {
		return 0;
	}
	if (ssl_client_mode) {
		return 1;
	}
	if (!SSL_CTX_set_cipher_list(ctx, "ADH:@STRENGTH")) {
		return 0;
	}
	if (!add_anon_dh()) {
		return 0;
	}

	mode = 0;
	mode |= SSL_MODE_ENABLE_PARTIAL_WRITE;
	mode |= SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
	SSL_CTX_set_mode(ctx, mode);

	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout(ctx, 300);
	SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_callback);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

	return 1;
}

static int anontls_dialog(int s_in, int s_out) {

	if (s_in || s_out) {}
	anontls_selected = 1;

	if (!switch_to_anon_dh()) {
		rfbLog("anontls: Anonymous Diffie-Hellman failed.\n");	
		return 0;
	}

	/* continue with SSL/TLS */
	return 1;
}

/*
 * Using spec:
 * http://www.mail-archive.com/qemu-devel@nongnu.org/msg08681.html
 */
static int vencrypt_dialog(int s_in, int s_out) {
	char buf[256], buf2[256];
	int subtypes[16];
	int n, i, ival, ok, nsubtypes = 0;

	vencrypt_selected = 0;

	/* send version 0.2 */
	buf[0] = 0;
	buf[1] = 2;

	if (!write_exact(s_out, buf, 2)) {
		close(s_in); close(s_out);
		return 0;
	}

	/* read client version 0.2 */
	memset(buf, 0, sizeof(buf));
	if (!read_exact(s_in, buf, 2)) {
		close(s_in); close(s_out);
		return 0;
	}
	rfbLog("vencrypt: received %d.%d client version.\n", (int) buf[0], (int) buf[1]);

	/* close 0.0 */
	if (buf[0] == 0 && buf[1] == 0) {
		rfbLog("vencrypt: received 0.0 version, closing connection.\n");
		close(s_in); close(s_out);
		return 0;
	}

	/* accept only 0.2 */
	if (buf[0] != 0 || buf[1] != 2) {
		rfbLog("vencrypt: unsupported VeNCrypt version, closing connection.\n");
		buf[0] = (char) 255;
		write_exact(s_out, buf, 1);
		close(s_in); close(s_out);
		return 0;
	}

	/* tell them OK */
	buf[0] = 0;
	if (!write_exact(s_out, buf, 1)) {
		close(s_in); close(s_out);
		return 0;
	}

	if (getenv("X11VNC_ENABLE_VENCRYPT_PLAIN_LOGIN")) {
		vencrypt_enable_plain_login = atoi(getenv("X11VNC_ENABLE_VENCRYPT_PLAIN_LOGIN"));
	}

	/* load our list of sub-types: */
	n = 0;
	if (!ssl_verify && vencrypt_kx != VENCRYPT_NODH) {
		if (screen->authPasswdData != NULL) {
			subtypes[n++] = rfbVencryptTlsVnc;
		} else {
			if (vencrypt_enable_plain_login && unixpw) {
				subtypes[n++] = rfbVencryptTlsPlain;
			} else {
				subtypes[n++] = rfbVencryptTlsNone;
			}
		}
	}
	if (vencrypt_kx != VENCRYPT_NOX509) {
		if (screen->authPasswdData != NULL) {
			subtypes[n++] = rfbVencryptX509Vnc;
		} else {
			if (vencrypt_enable_plain_login && unixpw) {
				subtypes[n++] = rfbVencryptX509Plain;
			} else {
				subtypes[n++] = rfbVencryptX509None;
			}
		}
	}

	nsubtypes = n;
	for (i = 0; i < nsubtypes; i++) {
		((uint32_t *)buf)[i] = Swap32IfLE(subtypes[i]);
	}

	/* send number first: */
	buf2[0] = (char) nsubtypes;
	if (!write_exact(s_out, buf2, 1)) {
		close(s_in); close(s_out);
		return 0;
	}
	/* and now the list: */
	if (!write_exact(s_out, buf, 4*n)) {
		close(s_in); close(s_out);
		return 0;
	}

	/* read client's selection: */
	if (!read_exact(s_in, (char *)&ival, 4)) {
		close(s_in); close(s_out);
		return 0;
	}
	ival = Swap32IfLE(ival);

	/* zero means no dice: */
	if (ival == 0) {
		rfbLog("vencrypt: client selected no sub-type, closing connection.\n");
		close(s_in); close(s_out);
		return 0;
	}

	/* check if he selected a valid one: */
	ok = 0;
	for (i = 0; i < nsubtypes; i++) {
		if (ival == subtypes[i]) {
			ok = 1;
		}
	}

	if (!ok) {
		rfbLog("vencrypt: client selected invalid sub-type: %d\n", ival);
		close(s_in); close(s_out);
		return 0;
	} else {
		char *st = "unknown!!";
		if (ival == rfbVencryptTlsNone)	  st = "rfbVencryptTlsNone";
		if (ival == rfbVencryptTlsVnc)    st = "rfbVencryptTlsVnc";
		if (ival == rfbVencryptTlsPlain)  st = "rfbVencryptTlsPlain";
		if (ival == rfbVencryptX509None)  st = "rfbVencryptX509None";
		if (ival == rfbVencryptX509Vnc)   st = "rfbVencryptX509Vnc";
		if (ival == rfbVencryptX509Plain) st = "rfbVencryptX509Plain";
		rfbLog("vencrypt: client selected sub-type: %d (%s)\n", ival, st);
	}

	vencrypt_selected = ival;

	/* not documented in spec, send OK: */
	buf[0] = 1;
	if (!write_exact(s_out, buf, 1)) {
		close(s_in); close(s_out);
		return 0;
	}

	if (vencrypt_selected == rfbVencryptTlsNone ||
	    vencrypt_selected == rfbVencryptTlsVnc  ||
	    vencrypt_selected == rfbVencryptTlsPlain) {
		/* these modes are Anonymous Diffie-Hellman */
		if (!switch_to_anon_dh()) {
			rfbLog("vencrypt: Anonymous Diffie-Hellman failed.\n");	
			return 0;
		}
	}

	/* continue with SSL/TLS */
	return 1;
}

static int check_vnc_tls_mode(int s_in, int s_out, double last_https) {
	double waited = 0.0, waitmax = 1.4, dt = 0.01, start = dnow();
	struct timeval tv;
	int input = 0, i, n, ok;
	int major, minor, sectype = -1;
	char *proto = "RFB 003.008\n";
	char *stype = "unknown";
	char buf[256];
	
	vencrypt_selected = 0;
	anontls_selected = 0;

	if (vencrypt_mode == VENCRYPT_NONE && anontls_mode == ANONTLS_NONE) {
		/* only normal SSL */
		return 1;
	}
	if (ssl_client_mode) {
		if (vencrypt_mode == VENCRYPT_FORCE || anontls_mode == ANONTLS_FORCE) {
			rfbLog("check_vnc_tls_mode: VENCRYPT_FORCE/ANONTLS_FORCE in client\n");
			rfbLog("check_vnc_tls_mode: connect mode.\n");
			/* this is OK, continue on below for dialog. */
		} else {
			/* otherwise we must assume normal SSL (we send client hello) */
			return 1;
		}
	}
	if (ssl_verify && vencrypt_mode != VENCRYPT_FORCE && anontls_mode == ANONTLS_FORCE) {
		rfbLog("check_vnc_tls_mode: Cannot use ANONTLS_FORCE with -sslverify (Anon DH only)\n");
		/* fallback to normal SSL */
		return 1;
	}

	if (last_https > 0.0) {
		double now = dnow();
		if (now < last_https + 5.0) {
			waitmax = 20.0;
		} else if (now < last_https + 15.0) {
			waitmax = 10.0;
		} else if (now < last_https + 30.0) {
			waitmax = 5.0;
		} else if (now < last_https + 60.0) {
			waitmax = 2.5;
		}
	}

	while (waited < waitmax) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(s_in, &rfds);
		tv.tv_sec = 0; 
		tv.tv_usec = 0; 
		select(s_in+1, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(s_in, &rfds)) {
			input = 1;
			break;
		}
		usleep((int) (1000 * 1000 * dt));
		waited += dt;
	}
	rfbLog("check_vnc_tls_mode: waited: %f / %.2f input: %s\n",
	    dnow() - start, waitmax, input ? "SSL Handshake" : "(future) RFB Handshake");

	if (input) {
		/* got SSL client hello, can only assume normal SSL */
		if (vencrypt_mode == VENCRYPT_FORCE || anontls_mode == ANONTLS_FORCE) {
			rfbLog("check_vnc_tls_mode: VENCRYPT_FORCE/ANONTLS_FORCE prevents normal SSL\n");
			return 0;
		}
		return 1;
	}

	/* send RFB 003.008 -- there is no turning back from this point... */
	if (!write_exact(s_out, proto, strlen(proto))) {
		close(s_in); close(s_out);
		return 0;
	}

	memset(buf, 0, sizeof(buf));
	if (!read_exact(s_in, buf, 12)) {
		close(s_in); close(s_out);
		return 0;
	}

	if (sscanf(buf, "RFB %03d.%03d\n", &major, &minor) != 2) {
		int i;
		rfbLog("check_vnc_tls_mode: abnormal handshake: '%s'\nbytes: ", buf);
		for (i=0; i < 12; i++) {
			fprintf(stderr, "%d.", (unsigned char) buf[i]);
		}
		fprintf(stderr, "\n");
		close(s_in); close(s_out);
		return 0;
	}
	rfbLog("check_vnc_tls_mode: version: %d.%d\n", major, minor);
	if (major != 3 || minor < 8) {
		rfbLog("check_vnc_tls_mode: invalid version: '%s'\n", buf);
		close(s_in); close(s_out);
		return 0;
	}

	n = 1;
	if (vencrypt_mode == VENCRYPT_FORCE) {
		buf[n++] = rfbSecTypeVencrypt;
	} else if (anontls_mode == ANONTLS_FORCE && !ssl_verify) {
		buf[n++] = rfbSecTypeAnonTls;
	} else if (vencrypt_mode == VENCRYPT_SOLE) {
		buf[n++] = rfbSecTypeVencrypt;
	} else if (anontls_mode == ANONTLS_SOLE && !ssl_verify) {
		buf[n++] = rfbSecTypeAnonTls;
	} else {
		if (vencrypt_mode == VENCRYPT_SUPPORT) {
			buf[n++] = rfbSecTypeVencrypt;
		}
		if (anontls_mode == ANONTLS_SUPPORT && !ssl_verify) {
			buf[n++] = rfbSecTypeAnonTls;
		}
	}

	n--;
	buf[0] = (char) n;
	if (!write_exact(s_out, buf, n+1)) {
		close(s_in); close(s_out);
		return 0;
	}
	if (0) fprintf(stderr, "wrote[%d] %d %d %d\n", n, buf[0], buf[1], buf[2]);

	buf[0] = 0;
	if (!read_exact(s_in, buf, 1)) {
		close(s_in); close(s_out);
		return 0;
	}

	if (buf[0] == rfbSecTypeVencrypt) stype = "VeNCrypt";
	if (buf[0] == rfbSecTypeAnonTls)  stype = "ANONTLS";

	rfbLog("check_vnc_tls_mode: reply: %d (%s)\n", (int) buf[0], stype);

	ok = 0;
	for (i=1; i < n+1; i++) {
		if (buf[0] == buf[i]) {
			ok = 1;
		}
	}
	if (!ok) {
		char *msg = "check_vnc_tls_mode: invalid security-type";
		int len = strlen(msg);
		rfbLog("%s: %d\n", msg, (int) buf[0]);
		((uint32_t *)buf)[0] = Swap32IfLE(len);
		write_exact(s_out, buf, 4);
		write_exact(s_out, msg, strlen(msg));
		close(s_in); close(s_out);
		return 0;
	}

	sectype = (int) buf[0];

	if (sectype == rfbSecTypeVencrypt) {
		return vencrypt_dialog(s_in, s_out);
	} else if (sectype == rfbSecTypeAnonTls) {
		return anontls_dialog(s_in, s_out);
	} else {
		return 0;
	}
}

static void pr_ssl_info(int verb) {
	SSL_CIPHER *c;
	SSL_SESSION *s;
	char *proto = "unknown";

	if (verb) {}

	if (ssl == NULL) {
		return;
	}
	c = SSL_get_current_cipher(ssl);
	s = SSL_get_session(ssl);

	if (s == NULL) {
		proto = "nosession";
	} else if (s->ssl_version == SSL2_VERSION) {
		proto = "SSLv2";
	} else if (s->ssl_version == SSL3_VERSION) {
		proto = "SSLv3";
	} else if (s->ssl_version == TLS1_VERSION) {
		proto = "TLSv1";
	}
	if (c != NULL) {
		rfbLog("SSL: ssl_helper[%d]: Cipher: %s %s Proto: %s\n", getpid(),
		    SSL_CIPHER_get_version(c), SSL_CIPHER_get_name(c), proto);
	} else {
		rfbLog("SSL: ssl_helper[%d]: Proto: %s\n", getpid(),
		    proto);
	}
}

static void ssl_timeout (int sig) {
	int i;
	rfbLog("sig: %d, ssl_init[%d] timed out.\n", sig, getpid());
	rfbLog("To increase the SSL initialization timeout use, e.g.:\n");
	rfbLog("   -env SSL_INIT_TIMEOUT=120        (for 120 seconds)\n");
	for (i=0; i < 256; i++) {
		close(i);
	}
	exit(1);
}

static int ssl_init(int s_in, int s_out, int skip_vnc_tls, double last_https) {
	unsigned char *sid = (unsigned char *) "x11vnc SID";
	char *name = NULL;
	int peerport = 0;
	int db = 0, rc, err;
	int ssock = s_in;
	double start = dnow();
	int timeout = 20;

	if (enc_str != NULL) {
		return 1;
	}
	if (getenv("SSL_DEBUG")) {
		db = atoi(getenv("SSL_DEBUG"));
	}
	usleep(100 * 1000);
	if (getenv("SSL_INIT_TIMEOUT")) {
		timeout = atoi(getenv("SSL_INIT_TIMEOUT"));
	} else if (client_connect != NULL && strstr(client_connect, "repeater")) {
		rfbLog("SSL: ssl_init[%d]: detected 'repeater' in connect string.\n", getpid());
		rfbLog("SSL: setting timeout to 1 hour: -env SSL_INIT_TIMEOUT=3600\n");
		rfbLog("SSL: use that option to set a different timeout value,\n");
		rfbLog("SSL: however note that with Windows UltraVNC repeater it\n");
		rfbLog("SSL: may timeout before your setting due to other reasons.\n");
		timeout = 3600;
	}

	if (skip_vnc_tls) {
		rfbLog("SSL: ssl_helper[%d]: HTTPS mode, skipping check_vnc_tls_mode()\n",
		    getpid());
	} else if (!check_vnc_tls_mode(s_in, s_out, last_https)) {
		return 0;
	}
	rfbLog("SSL: ssl_init[%d]: %d/%d initialization timeout: %d secs.\n",
	    getpid(), s_in, s_out, timeout);

	ssl = SSL_new(ctx);
	if (ssl == NULL) {
		fprintf(stderr, "SSL_new failed\n");
		return 0;
	}
	if (db > 1) fprintf(stderr, "ssl_init: 1\n");

	SSL_set_session_id_context(ssl, sid, strlen((char *)sid));

	if (s_in == s_out) {
		if (! SSL_set_fd(ssl, ssock)) {
			fprintf(stderr, "SSL_set_fd failed\n");
			return 0;
		}
	} else {
		if (! SSL_set_rfd(ssl, s_in)) {
			fprintf(stderr, "SSL_set_rfd failed\n");
			return 0;
		}
		if (! SSL_set_wfd(ssl, s_out)) {
			fprintf(stderr, "SSL_set_wfd failed\n");
			return 0;
		}
	}
	if (db > 1) fprintf(stderr, "ssl_init: 2\n");

	if (ssl_client_mode) {
		SSL_set_connect_state(ssl);
	} else {
		SSL_set_accept_state(ssl);
	}

	if (db > 1) fprintf(stderr, "ssl_init: 3\n");

	name = get_remote_host(ssock);
	peerport = get_remote_port(ssock);

	if (!strcmp(name, "0.0.0.0") && openssl_last_ip != NULL) {
		name = strdup(openssl_last_ip);
	}

	if (db > 1) fprintf(stderr, "ssl_init: 4\n");

	while (1) {

		signal(SIGALRM, ssl_timeout);
		alarm(timeout);

		if (ssl_client_mode) {
			if (db) fprintf(stderr, "calling SSL_connect...\n");
			rc = SSL_connect(ssl);
		} else {
			if (db) fprintf(stderr, "calling SSL_accept...\n");
			rc = SSL_accept(ssl);
		}
		err = SSL_get_error(ssl, rc);

		alarm(0);
		signal(SIGALRM, SIG_DFL);

		if (ssl_client_mode) {
			if (db) fprintf(stderr, "SSL_connect %d/%d\n", rc, err);
		} else {
			if (db) fprintf(stderr, "SSL_accept %d/%d\n", rc, err);
		}
		if (err == SSL_ERROR_NONE) {
			break;
		} else if (err == SSL_ERROR_WANT_READ) {

			if (db) fprintf(stderr, "got SSL_ERROR_WANT_READ\n");
			rfbLog("SSL: ssl_helper[%d]: %s() failed for: %s:%d 1\n",
			    getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept", name, peerport);
			pr_ssl_info(1);
			return 0;
			
		} else if (err == SSL_ERROR_WANT_WRITE) {

			if (db) fprintf(stderr, "got SSL_ERROR_WANT_WRITE\n");
			rfbLog("SSL: ssl_helper[%d]: %s() failed for: %s:%d 2\n",
			    getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept", name, peerport);
			pr_ssl_info(1);
			return 0;

		} else if (err == SSL_ERROR_SYSCALL) {

			if (db) fprintf(stderr, "got SSL_ERROR_SYSCALL\n");
			rfbLog("SSL: ssl_helper[%d]: %s() failed for: %s:%d 3\n",
			    getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept", name, peerport);
			pr_ssl_info(1);
			return 0;

		} else if (err == SSL_ERROR_ZERO_RETURN) {

			if (db) fprintf(stderr, "got SSL_ERROR_ZERO_RETURN\n");
			rfbLog("SSL: ssl_helper[%d]: %s() failed for: %s:%d 4\n",
			    getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept", name, peerport);
			pr_ssl_info(1);
			return 0;

		} else if (rc < 0) {
			unsigned long err;
			int cnt = 0;

			rfbLog("SSL: ssl_helper[%d]: %s() *FATAL: %d SSL FAILED\n",
			    getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept", rc);
			while ((err = ERR_get_error()) != 0) {
				rfbLog("SSL: %s\n", ERR_error_string(err, NULL));
				if (cnt++ > 100) {
					break;
				}
			}
			pr_ssl_info(1);
			return 0;

		} else if (dnow() > start + 3.0) {

			rfbLog("SSL: ssl_helper[%d]: timeout looping %s() "
			    "fatal.\n", getpid(), ssl_client_mode ? "SSL_connect" : "SSL_accept");
			pr_ssl_info(1);
			return 0;

		} else {
			BIO *bio = SSL_get_rbio(ssl);
			if (bio == NULL) {
				rfbLog("SSL: ssl_helper[%d]: ssl BIO is null. "
				    "fatal.\n", getpid());
				pr_ssl_info(1);
				return 0;
			}
			if (BIO_eof(bio)) {
				rfbLog("SSL: ssl_helper[%d]: ssl BIO is EOF. "
				    "fatal.\n", getpid());
				pr_ssl_info(1);
				return 0;
			}
		}
		usleep(10 * 1000);
	}

	if (ssl_client_mode) {
		rfbLog("SSL: ssl_helper[%d]: SSL_connect() succeeded for: %s:%d\n", getpid(), name, peerport);
	} else {
		rfbLog("SSL: ssl_helper[%d]: SSL_accept() succeeded for: %s:%d\n", getpid(), name, peerport);
	}

	pr_ssl_info(0);

	if (SSL_get_verify_result(ssl) == X509_V_OK) {
		X509 *x;
		FILE *cr = NULL;
		if (certret != NULL) {
			cr = fopen(certret, "w");
		}
		
		x = SSL_get_peer_certificate(ssl);	
		if (x == NULL) {
			rfbLog("SSL: ssl_helper[%d]: accepted client %s x509 peer cert is null\n", getpid(), name);
			if (cr != NULL) {
				fprintf(cr, "NOCERT\n");
				fclose(cr);
			}
		} else {
			rfbLog("SSL: ssl_helper[%d]: accepted client %s x509 cert is:\n", getpid(), name);
#if LIBVNCSERVER_HAVE_X509_PRINT_EX_FP
			X509_print_ex_fp(stderr, x, 0, XN_FLAG_MULTILINE);
#endif
			if (cr != NULL) {
#if LIBVNCSERVER_HAVE_X509_PRINT_EX_FP
				X509_print_ex_fp(cr, x, 0, XN_FLAG_MULTILINE);
#else
				rfbLog("** not compiled with libssl X509_print_ex_fp() function **\n");
				if (users_list && strstr(users_list, "sslpeer=")) {
					rfbLog("** -users sslpeer= will not work! **\n");
				}
#endif
				fclose(cr);
			}
		}
	}
	free(name);

	return 1;
}

static void symmetric_encryption_xfer(int csock, int s_in, int s_out);

static void ssl_xfer(int csock, int s_in, int s_out, int is_https) {
	int dbxfer = 0, db = 0, check_pending, fdmax, nfd, n, i, err;
	char cbuf[ABSIZE], sbuf[ABSIZE];
	int  cptr, sptr, c_rd, c_wr, s_rd, s_wr;
	fd_set rd, wr;
	struct timeval tv;
	int ssock, cnt = 0, ndata = 0;

	/*
	 * we want to switch to a longer timeout for long term VNC
	 * connections (in case the network is not working for periods of
	 * time), but we also want the timeout shorter at the beginning
	 * in case the client went away.
	 */
	double start, now;
	int tv_https_early = 60;
	int tv_https_later = 20;
	int tv_vnc_early = 40;
	int tv_vnc_later = 43200;	/* was 300, stunnel: 43200 */
	int tv_cutover = 70;
	int tv_closing = 60;
	int tv_use;

	if (dbxfer) {
		raw_xfer(csock, s_in, s_out);
		return;
	}
	if (enc_str != NULL) {
		if (!strcmp(enc_str, "none")) {
			usleep(250*1000);
			rfbLog("doing '-enc none' raw transfer (no encryption)\n"); 
			raw_xfer(csock, s_in, s_out);
		} else {
			symmetric_encryption_xfer(csock, s_in, s_out);
		}
		return;
	}

	if (getenv("SSL_DEBUG")) {
		db = atoi(getenv("SSL_DEBUG"));
	}

	if (db) fprintf(stderr, "ssl_xfer begin\n");

	start = dnow();
	if (is_https) {
		tv_use = tv_https_early;
	} else {
		tv_use = tv_vnc_early;
	}
	

	/*
	 * csock: clear text socket with libvncserver.    "C"
	 * ssock: ssl data socket with remote vnc viewer. "S"
	 *
	 * to cover inetd mode, we have s_in and s_out, but in non-inetd
	 * mode they both ssock.
	 *
	 * cbuf[] is data from csock that we have read but not passed on to ssl 
	 * sbuf[] is data from ssl that we have read but not passed on to csock 
	 */
	for (i=0; i<ABSIZE; i++) {
		cbuf[i] = '\0';
		sbuf[i] = '\0';
	}
		
	if (s_out > s_in) {
		ssock = s_out;
	} else {
		ssock = s_in;
	}

	if (csock > ssock) {
		fdmax = csock; 
	} else {
		fdmax = ssock; 
	}

	c_rd = 1;	/* clear text (libvncserver) socket open for reading */
	c_wr = 1;	/* clear text (libvncserver) socket open for writing */
	s_rd = 1;	/* ssl data (remote client)  socket open for reading */
	s_wr = 1;	/* ssl data (remote client)  socket open for writing */

	cptr = 0;	/* offsets into ABSIZE buffers */
	sptr = 0;

	if (vencrypt_selected > 0 || anontls_selected > 0) {
		char tmp[16];
		/* read and discard the extra RFB version */
		memset(tmp, 0, sizeof(tmp));
		read(csock, tmp, 12);
		if (0) fprintf(stderr, "extra: %s\n", tmp);
	}

	while (1) {
		int c_to_s, s_to_c, closing;

		if ( s_wr && (c_rd || cptr > 0) ) {
			/* 
			 * S is writable and 
			 * C is readable or some cbuf data remaining
			 */
			c_to_s = 1;
		} else {
			c_to_s = 0;
		}

		if ( c_wr && (s_rd || sptr > 0) ) {
			/* 
			 * C is writable and 
			 * S is readable or some sbuf data remaining
			 */
			s_to_c = 1;
		} else {
			s_to_c = 0;
		}

		if (! c_to_s && ! s_to_c) {
			/*
			 * nothing can be sent either direction.
			 * break out of the loop to finish all work.
			 */
			break;
		}
		cnt++;

		/* set up the fd sets for the two sockets for read & write: */

		FD_ZERO(&rd);

		if (c_rd && cptr < ABSIZE) {
			/* we could read more from C since cbuf is not full */
			FD_SET(csock, &rd);
		}
		if (s_rd) {
			/*
			 * we could read more from S since sbuf not full,
			 * OR ssl is waiting for more BIO to be able to
			 * read and we have some C data still buffered.
			 */
			if (sptr < ABSIZE || (cptr > 0 && SSL_want_read(ssl))) {
				FD_SET(s_in, &rd);
			}
		}
		
		FD_ZERO(&wr);

		if (c_wr && sptr > 0) {
			/* we could write more to C since sbuf is not empty */
			FD_SET(csock, &wr);
		}
		if (s_wr) {
			/*
			 * we could write more to S since cbuf not empty,
			 * OR ssl is waiting for more BIO to be able
			 * write and we haven't filled up sbuf yet.
			 */
			if (cptr > 0 || (sptr < ABSIZE && SSL_want_write(ssl))) {
				FD_SET(s_out, &wr);
			}
		}

		now = dnow();
		if (tv_cutover && now > start + tv_cutover) {
			rfbLog("SSL: ssl_xfer[%d]: tv_cutover: %d\n", getpid(),
			    tv_cutover);
			tv_cutover = 0;
			if (is_https) {
				tv_use = tv_https_later;
			} else {
				tv_use = tv_vnc_later;
			}
			/* try to clean out some zombies if we can. */
			ssl_helper_pid(0, -2);
		}
		if (ssl_timeout_secs > 0) {
			tv_use = ssl_timeout_secs;
		}

		if ( (s_rd && c_rd) || cptr || sptr) {
			closing = 0;
		} else {
			closing = 1;
			tv_use = tv_closing;
		}

		tv.tv_sec  = tv_use;
		tv.tv_usec = 0;

		/*  do the select, repeat if interrupted */
		do {
			if (ssl_timeout_secs == 0) {
				nfd = select(fdmax+1, &rd, &wr, NULL, NULL);
			} else {
				nfd = select(fdmax+1, &rd, &wr, NULL, &tv);
			}
		} while (nfd < 0 && errno == EINTR);

		if (db > 1) fprintf(stderr, "nfd: %d\n", nfd);

if (0) fprintf(stderr, "nfd[%d]: %d  w/r csock: %d %d s_in: %d %d\n", getpid(), nfd, FD_ISSET(csock, &wr), FD_ISSET(csock, &rd), FD_ISSET(s_out, &wr), FD_ISSET(s_in, &rd)); 

		if (nfd < 0) {
			rfbLog("SSL: ssl_xfer[%d]: select error: %d\n", getpid(), nfd);
			perror("select");
			/* connection finished */
			goto done;	
		}

		if (nfd == 0) {
			if (!closing && tv_cutover && ndata > 25000) {
				static int cn = 0;
				/* probably ok, early windows iconify */
				if (cn++ < 2) {
					rfbLog("SSL: ssl_xfer[%d]: early time"
					    "out: %d\n", getpid(), ndata);
				}
				continue;
			}
			rfbLog("SSL: ssl_xfer[%d]: connection timedout. %d  tv_use: %d\n",
			    getpid(), ndata, tv_use);
			/* connection finished */
			goto done;
		}

		/* used to see if SSL_pending() should be checked: */
		check_pending = 0;
/* AUDIT */

		if (c_wr && FD_ISSET(csock, &wr)) {

			/* try to write some of our sbuf to C: */
			n = write(csock, sbuf, sptr); 

			if (n < 0) {
				if (errno != EINTR) {
					/* connection finished */
					goto done;
				}
				/* proceed */
			} else if (n == 0) {
				/* connection finished XXX double check */
				goto done;
			} else {
				/* shift over the data in sbuf by n */
				memmove(sbuf, sbuf + n, sptr - n);
				if (sptr == ABSIZE) {
					check_pending = 1;
				}
				sptr -= n;

				if (! s_rd && sptr == 0) {
					/* finished sending last of sbuf */
					shutdown(csock, SHUT_WR);
					c_wr = 0;
				}
				ndata += n;
			}
		}

		if (s_wr) {
			if ((cptr > 0 && FD_ISSET(s_out, &wr)) ||
			    (SSL_want_read(ssl) && FD_ISSET(s_in, &rd))) {

				/* try to write some of our cbuf to S: */

				n = SSL_write(ssl, cbuf, cptr);
				err = SSL_get_error(ssl, n);

				if (err == SSL_ERROR_NONE) {
					/* shift over the data in cbuf by n */
					memmove(cbuf, cbuf + n, cptr - n);
					cptr -= n;

					if (! c_rd && cptr == 0 && s_wr) {
						/* finished sending last cbuf */
						SSL_shutdown(ssl);
						s_wr = 0;
					}
					ndata += n;

				} else if (err == SSL_ERROR_WANT_WRITE
					|| err == SSL_ERROR_WANT_READ
					|| err == SSL_ERROR_WANT_X509_LOOKUP) {

						;	/* proceed */

				} else if (err == SSL_ERROR_SYSCALL) {
					if (n < 0 && errno != EINTR) {
						/* connection finished */
						goto done;
					}
					/* proceed */
				} else if (err == SSL_ERROR_ZERO_RETURN) {
					/* S finished */
					s_rd = 0;
					s_wr = 0;
				} else if (err == SSL_ERROR_SSL) {
					/* connection finished */
					goto done;
				}
			}
		}

		if (c_rd && FD_ISSET(csock, &rd)) {


			/* try to read some data from C into our cbuf */

			n = read(csock, cbuf + cptr, ABSIZE - cptr);

			if (n < 0) {
				if (errno != EINTR) {
					/* connection finished */
					goto done;
				}
				/* proceed */
			} else if (n == 0) {
				/* C is EOF */
				c_rd = 0;
				if (cptr == 0 && s_wr) {
					/* and no more in cbuf to send */
					SSL_shutdown(ssl);
					s_wr = 0;
				}
			} else {
				/* good */

				cptr += n;
				ndata += n;
			}
		}

		if (s_rd) {
			if ((sptr < ABSIZE && FD_ISSET(s_in, &rd)) ||
			    (SSL_want_write(ssl) && FD_ISSET(s_out, &wr)) ||
			    (check_pending && SSL_pending(ssl))) {

				/* try to read some data from S into our sbuf */

				n = SSL_read(ssl, sbuf + sptr, ABSIZE - sptr);
				err = SSL_get_error(ssl, n);

				if (err == SSL_ERROR_NONE) {
					/* good */

					sptr += n;
					ndata += n;

				} else if (err == SSL_ERROR_WANT_WRITE
					|| err == SSL_ERROR_WANT_READ
					|| err == SSL_ERROR_WANT_X509_LOOKUP) {

						;	/* proceed */

				} else if (err == SSL_ERROR_SYSCALL) {
					if (n < 0) {
						if(errno != EINTR) {
							/* connection finished */
							goto done;
						}
						/* proceed */
					} else {
						/* S finished */
						s_rd = 0;
						s_wr = 0;
					}
				} else if (err == SSL_ERROR_ZERO_RETURN) {
					/* S is EOF */
					s_rd = 0;
					if (cptr == 0 && s_wr) {
						/* and no more in cbuf to send */
						SSL_shutdown(ssl);
						s_wr = 0;
					}
					if (sptr == 0 && c_wr) {
						/* and no more in sbuf to send */
						shutdown(csock, SHUT_WR);
						c_wr = 0;
					}
				} else if (err == SSL_ERROR_SSL) {
					/* connection finished */
					goto done;
				}
			}
		}
	}

	done:
	rfbLog("SSL: ssl_xfer[%d]: closing sockets %d, %d, %d\n",
			    getpid(), csock, s_in, s_out);
	close(csock);
	close(s_in);
	close(s_out);
	return;
}

#define MSZ 4096
static void init_prng(void) {
	int db = 0, bytes, ubytes, fd;
	char file[MSZ], dtmp[100];
	unsigned int sr;

	RAND_file_name(file, MSZ);

	rfbLog("RAND_file_name: %s\n", file);

	bytes = RAND_load_file(file, -1);
	if (db) fprintf(stderr, "bytes read: %d\n", bytes);
	
	ubytes = RAND_load_file("/dev/urandom", 64);
	bytes += ubytes;
	if (db) fprintf(stderr, "bytes read: %d / %d\n", bytes, ubytes);

	/* mix in more predictable stuff as well for fallback */
	sprintf(dtmp, "/tmp/p%.8f.XXXXXX", dnow());
	fd = mkstemp(dtmp);
	RAND_add(dtmp, strlen(dtmp), 0);
	if (fd >= 0) {
		close(fd);
		unlink(dtmp);
	}
	sprintf(dtmp, "%d-%.8f", (int) getpid(), dnow());
	RAND_add(dtmp, strlen(dtmp), 0);

	if (!RAND_status()) {
		ubytes = -1;
		rfbLog("calling RAND_poll()\n");
		RAND_poll();
	}
	
	RAND_bytes((unsigned char *)&sr, 4);
	srand(sr);

	if (bytes > 0) {
		if (! quiet) {
			rfbLog("initialized PRNG with %d random bytes.\n",
			    bytes);
		}
		if (ubytes > 32 && rnow() < 0.25) {
			RAND_write_file(file);
		}
		return;
	}

	bytes += RAND_load_file("/dev/random", 8);
	if (db) fprintf(stderr, "bytes read: %d\n", bytes);
	RAND_poll();

	if (! quiet) {
		rfbLog("initialized PRNG with %d random bytes.\n", bytes);
	}
}
#endif	/* FORK_OK */
#endif	/* LIBVNCSERVER_HAVE_LIBSSL */

void check_openssl(void) {
	fd_set fds;
	struct timeval tv;
	int nfds, nmax = openssl_sock;
	static time_t last_waitall = 0;
	static double last_check = 0.0;
	double now;

	if (! use_openssl) {
		return;
	}

	if (time(NULL) > last_waitall + 120) {
		last_waitall = time(NULL);
		ssl_helper_pid(0, -2);	/* waitall */
	}

	if (openssl_sock < 0 && openssl_sock6 < 0) {
		return;
	}

	now = dnow();
	if (now < last_check + 0.5) {
		return;
	}
	last_check = now;

	FD_ZERO(&fds);
	if (openssl_sock >= 0) {
		FD_SET(openssl_sock, &fds);
	}
	if (openssl_sock6 >= 0) {
		FD_SET(openssl_sock6, &fds);
		if (openssl_sock6 > openssl_sock) {
			nmax = openssl_sock6;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(nmax+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}

	if (openssl_sock >= 0 && FD_ISSET(openssl_sock, &fds)) {
		rfbLog("SSL: accept_openssl(OPENSSL_VNC)\n");
		accept_openssl(OPENSSL_VNC, -1);
	}
	if (openssl_sock6 >= 0 && FD_ISSET(openssl_sock6, &fds)) {
		rfbLog("SSL: accept_openssl(OPENSSL_VNC6)\n");
		accept_openssl(OPENSSL_VNC6, -1);
	}
}

void check_https(void) {
	fd_set fds;
	struct timeval tv;
	int nfds, nmax = https_sock;
	static double last_check = 0.0;
	double now;

	if (! use_openssl || (https_sock < 0 && https_sock6 < 0)) {
		return;
	}

	now = dnow();
	if (now < last_check + 0.5) {
		return;
	}
	last_check = now;

	FD_ZERO(&fds);
	if (https_sock >= 0) {
		FD_SET(https_sock, &fds);
	}
	if (https_sock6 >= 0) {
		FD_SET(https_sock6, &fds);
		if (https_sock6 > https_sock) {
			nmax = https_sock6;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(nmax+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}

	if (https_sock >= 0 && FD_ISSET(https_sock, &fds)) {
		rfbLog("SSL: accept_openssl(OPENSSL_HTTPS)\n");
		accept_openssl(OPENSSL_HTTPS, -1);
	}
	if (https_sock6 >= 0 && FD_ISSET(https_sock6, &fds)) {
		rfbLog("SSL: accept_openssl(OPENSSL_HTTPS6)\n");
		accept_openssl(OPENSSL_HTTPS6, -1);
	}
}

void openssl_port(int restart) {
	int sock = -1, shutdown = 0;
	static int port = -1;
	static in_addr_t iface = INADDR_ANY;
	int db = 0, fd6 = -1;

	if (! screen) {
		rfbLog("openssl_port: no screen!\n");
		clean_up_exit(1);
	}
	if (inetd) {
		ssl_initialized = 1;
		return;
	}

	if (ipv6_listen && screen->port <= 0) {
		if (got_rfbport) {
			screen->port = got_rfbport_val;
		} else {
			int ap = 5900;
			if (auto_port > 0) {
				ap = auto_port;
			}
			screen->port = find_free_port6(ap, ap+200);
		}
		rfbLog("openssl_port: reset port from 0 => %d\n", screen->port);
	}

	if (restart) {
		port = screen->port;
	} else if (screen->listenSock > -1 && screen->port > 0) {
		port = screen->port;
		shutdown = 1;
	} else if (ipv6_listen && screen->port > 0) {
		port = screen->port;
	} else if (screen->port == 0) {
		port = screen->port;
	}

	iface = screen->listenInterface;

	if (shutdown) {
		if (db) fprintf(stderr, "shutting down %d/%d\n",
		    port, screen->listenSock);
#if LIBVNCSERVER_HAS_SHUTDOWNSOCKETS
		rfbShutdownSockets(screen);
#endif
	}

	if (openssl_sock >= 0) {
		close(openssl_sock);
		openssl_sock = -1;
	}
	if (openssl_sock6 >= 0) {
		close(openssl_sock6);
		openssl_sock6 = -1;
	}

	if (port < 0) {
		rfbLog("openssl_port: could not obtain listening port %d\n", port);
		if (!got_rfbport && !got_ipv6_listen) {
			rfbLog("openssl_port: if this system is IPv6-only, use the -6 option\n");
		}
		clean_up_exit(1);
	} else if (port == 0) {
		/* no listen case, i.e. -connect */
		sock = -1;
	} else {
		sock = listen_tcp(port, iface, 0);
		if (ipv6_listen) {
			fd6 = listen6(port);
		} else if (!got_rfbport && !got_ipv6_listen) {
			if (sock < 0) {
				rfbLog("openssl_port: if this system is IPv6-only, use the -6 option\n");
			}
		}
		if (sock < 0) {
			if (fd6 < 0) {
				rfbLog("openssl_port: could not reopen port %d\n", port);
				if (!restart) {
					clean_up_exit(1);
				}
			} else {
				rfbLog("openssl_port: Info: listening on IPv6 only.\n");
			}
		}
	}
	rfbLog("openssl_port: listen on port/sock %d/%d\n", port, sock);
	if (ipv6_listen && port > 0) {
		if (fd6 < 0) {
			fd6 = listen6(port);
		}
		if (fd6 < 0) {
			ipv6_listen = 0;
		} else {
			rfbLog("openssl_port: listen on port/sock %d/%d (ipv6)\n",
			    port, fd6);
			openssl_sock6 = fd6;
		}
	}
	if (!quiet && sock >=0) {
		announce(port, 1, NULL);
	}
	openssl_sock = sock;
	openssl_port_num = port;

	ssl_initialized = 1;
}

void https_port(int restart) {
	int sock, fd6 = -1;
	static int port = 0;
	static in_addr_t iface = INADDR_ANY;

	/* as openssl_port above: open a listening socket for pure https: */
	if (https_port_num < 0) {
		return;
	}
	if (! screen) {
		rfbLog("https_port: no screen!\n");
		clean_up_exit(1);
	}
	if (! screen->httpDir) {
		return;
	}
	if (screen->listenInterface) {
		iface = screen->listenInterface;
	}

	if (https_port_num == 0) {
		https_port_num = find_free_port(5801, 5851);
	}
	if (ipv6_listen && https_port_num <= 0) {
		https_port_num = find_free_port6(5801, 5851);
	}
	if (https_port_num <= 0) {
		rfbLog("https_port: could not find port %d\n", https_port_num);
		clean_up_exit(1);
	}
	port = https_port_num;

	if (port <= 0) {
		rfbLog("https_port: could not obtain listening port %d\n", port);
		if (!restart) {
			clean_up_exit(1);
		} else {
			return;
		}
	}
	if (https_sock >= 0) {
		close(https_sock);
		https_sock = -1;
	}
	if (https_sock6 >= 0) {
		close(https_sock6);
		https_sock6 = -1;
	}
	sock = listen_tcp(port, iface, 0);
	if (sock < 0) {
		rfbLog("https_port: could not open port %d\n", port);
		if (ipv6_listen) {
			fd6 = listen6(port);
		} 
		if (fd6 < 0) {
			if (!restart) {
				clean_up_exit(1);
			}
		}
		rfbLog("https_port: trying IPv6 only mode.\n");
	}
	rfbLog("https_port: listen on port/sock %d/%d\n", port, sock);
	https_sock = sock;

	if (ipv6_listen) {
		if (fd6 < 0) {
			fd6 = listen6(port);
		}
		if (fd6 < 0) {
			;
		} else {
			rfbLog("https_port: listen on port/sock %d/%d (ipv6)\n",
			    port, fd6);
			https_sock6 = fd6;
		}
		if (fd6 < 0 && https_sock < 0) {
			rfbLog("https_port: could not listen on either IPv4 or IPv6.\n");
			if (!restart) {
				clean_up_exit(1);
			}
		}
	}
}

static void lose_ram(void) {
	/*
	 * for a forked child that will be around for a long time
	 * without doing exec().  we really should re-exec, but a pain
	 * to redo all SSL ctx.
	 */
	free_old_fb();

	free_tiles();
}

/* utility to keep track of existing helper processes: */

void ssl_helper_pid(pid_t pid, int sock) {
#	define HPSIZE 256
	static pid_t helpers[HPSIZE];
	static int   sockets[HPSIZE], first = 1;
	int i, empty, set, status;
	static int db = 0;

	if (first) {
		for (i=0; i < HPSIZE; i++)  {
			helpers[i] = 0;
			sockets[i] = 0;
		}
		if (getenv("SSL_HELPER_PID_DB")) {
			db = 1;
		}
		first = 0;
	}


	if (pid == 0) {
		/* killall or waitall */
		for (i=0; i < HPSIZE; i++) {
			if (helpers[i] == 0) {
				sockets[i] = -1;
				continue;
			}
			if (kill(helpers[i], 0) == 0) {
				int kret = -2;
				pid_t wret;
				if (sock != -2) {
					if (sockets[i] >= 0) {
						close(sockets[i]);
					}
					kret = kill(helpers[i], SIGTERM);
					if (kret == 0) {
						usleep(20 * 1000);
					}
				}

#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID 
				wret = waitpid(helpers[i], &status, WNOHANG); 

if (db) fprintf(stderr, "waitpid(%d)\n", helpers[i]);
if (db) fprintf(stderr, "  waitret1=%d\n", wret);

				if (kret == 0 && wret != helpers[i]) {
					int k;
					for (k=0; k < 10; k++) {
						usleep(100 * 1000);
						wret = waitpid(helpers[i], &status, WNOHANG); 
if (db) fprintf(stderr, "  waitret2=%d\n", wret);
						if (wret == helpers[i]) {
							break;
						}
					}
				}
#endif
				if (sock == -2) {
					continue;
				}
			}
			helpers[i] = 0;
			sockets[i] = -1;
		}
		return;
	}

if (db) fprintf(stderr, "ssl_helper_pid(%d, %d)\n", pid, sock);

	/* add (or delete for sock == -1) */
	set = 0;
	empty = -1;
	for (i=0; i < HPSIZE; i++) {
		if (helpers[i] == pid) {
			if (sock == -1) {
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID 
				pid_t wret;
				wret = waitpid(helpers[i], &status, WNOHANG); 

if (db) fprintf(stderr, "waitpid(%d) 2\n", helpers[i]);
if (db) fprintf(stderr, "  waitret1=%d\n", wret);
#endif
				helpers[i] = 0;
			}
			sockets[i] = sock;
			set = 1;
		} else if (empty == -1 && helpers[i] == 0) {
			empty = i;
		}
	}
	if (set || sock == -1) {
		return;	/* done */
	}

	/* now try to store */
	if (empty >= 0) {
		helpers[empty] = pid;
		sockets[empty] = sock;
		return;
	}
	for (i=0; i < HPSIZE; i++) {
		if (helpers[i] == 0) {
			continue;
		}
		/* clear out stale pids: */
		if (kill(helpers[i], 0) != 0) {
			helpers[i] = 0;
			sockets[i] = -1;
			
			if (empty == -1) {
				empty = i;
			}
		}
	}
	if (empty >= 0) {
		helpers[empty] = pid;
		sockets[empty] = sock;
	}
}

static int is_ssl_readable(int s_in, double last_https, char *last_get,
    int mode) {
	int nfd, db = 0;
	struct timeval tv;
	fd_set rd;

	if (getenv("ACCEPT_OPENSSL_DEBUG")) {
		db = atoi(getenv("ACCEPT_OPENSSL_DEBUG"));
	}

	/*
	 * we'll do a select() on s_in for reading.  this is not an
	 * absolute proof that SSL_read is ready (XXX use SSL utility).
	 */
	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	if (mode == OPENSSL_INETD) {
		/*
		 * https via inetd is icky because x11vnc is restarted
		 * for each socket (and some clients send requests
		 * rapid fire).
		 */
		tv.tv_sec = 4;
	}

	/*
	 * increase the timeout if we know HTTP traffic has occurred
	 * recently:
	 */
	if (dnow() < last_https + 30.0) {
		tv.tv_sec = 10;
		if (last_get && strstr(last_get, "VncViewer")) {
			tv.tv_sec = 5;
		}
	}
	if (getenv("X11VNC_HTTPS_VS_VNC_TIMEOUT")) {
		tv.tv_sec  = atoi(getenv("X11VNC_HTTPS_VS_VNC_TIMEOUT"));
	}
if (db) fprintf(stderr, "tv_sec: %d - '%s'\n", (int) tv.tv_sec, last_get);

	FD_ZERO(&rd);
	FD_SET(s_in, &rd);

	if (db) fprintf(stderr, "is_ssl_readable: begin  select(%d secs) %.6f\n", (int) tv.tv_sec, dnow());
	do {
		nfd = select(s_in+1, &rd, NULL, NULL, &tv);
	} while (nfd < 0 && errno == EINTR);
	if (db) fprintf(stderr, "is_ssl_readable: finish select(%d secs) %.6f\n", (int) tv.tv_sec, dnow());

	if (db) fprintf(stderr, "https nfd: %d\n", nfd);

	if (nfd <= 0 || ! FD_ISSET(s_in, &rd)) {
		return 0;
	}
	return 1;
}

static int watch_for_http_traffic(char *buf_a, int *n_a, int raw_sock) {
	int is_http, err, n, n2;
	char *buf;
	int db = 0;
	/*
	 * sniff the first couple bytes of the stream and try to see
	 * if it is http or not.  if we read them OK, we must read the
	 * rest of the available data otherwise we may deadlock.
	 * what has been read is returned in buf_a and n_a.
	 * *buf_a is ABSIZE+1 long and zeroed.
	 */
	if (getenv("ACCEPT_OPENSSL_DEBUG")) {
		db = atoi(getenv("ACCEPT_OPENSSL_DEBUG"));
	}
	if (! buf_a || ! n_a) {
		return 0;
	}

	buf = (char *) calloc((ABSIZE+1), 1);
	*n_a = 0;

	if (enc_str && !strcmp(enc_str, "none")) {
		n = read(raw_sock, buf, 2);
		err = SSL_ERROR_NONE;
	} else {
#if LIBVNCSERVER_HAVE_LIBSSL 
		n = SSL_read(ssl, buf, 2);
		err = SSL_get_error(ssl, n);
#else
		err = n = 0;
		badnews("1 in watch_for_http_traffic");
#endif
	}

	if (err != SSL_ERROR_NONE || n < 2) {
		if (n > 0) {
			strncpy(buf_a, buf, n);
			*n_a = n;
		}
		if (db) fprintf(stderr, "watch_for_http_traffic ssl err: %d/%d\n", err, n);
		return -1;
	}

	/* look for GET, HEAD, POST, CONNECT */
	is_http = 0;
	if (!strncmp("GE", buf, 2)) {
		is_http = 1;
	} else if (!strncmp("HE", buf, 2)) {
		is_http = 1;
	} else if (!strncmp("PO", buf, 2)) {
		is_http = 1;
	} else if (!strncmp("CO", buf, 2)) {
		is_http = 1;
	}
	if (db) fprintf(stderr, "watch_for_http_traffic read: '%s' %d\n", buf, n);

	/*
	 * better read all we can and fwd it along to avoid blocking
	 * in ssl_xfer().
	 */

	if (enc_str && !strcmp(enc_str, "none")) {
		n2 = read(raw_sock, buf + n, ABSIZE - n);
	} else {
#if LIBVNCSERVER_HAVE_LIBSSL 
		n2 = SSL_read(ssl, buf + n, ABSIZE - n);
#else
		n2 = 0;
		badnews("2 in watch_for_http_traffic");
#endif
	}
	if (n2 >= 0) {
		n += n2;
	}

	*n_a = n;

	if (db) fprintf(stderr, "watch_for_http_traffic readmore: %d\n", n2);

	if (n > 0) {
		memcpy(buf_a, buf, n);
	}
	if (db > 1) {
		fprintf(stderr, "watch_for_http_traffic readmore: ");
		write(2, buf_a, *n_a);
		fprintf(stderr, "\n");
	}
	if (db) fprintf(stderr, "watch_for_http_traffic return: %d\n", is_http);
	return is_http;
}

static int csock_timeout_sock = -1;

static void csock_timeout (int sig) {
	rfbLog("sig: %d, csock_timeout.\n", sig);
	if (csock_timeout_sock >= 0) {
		close(csock_timeout_sock);
		csock_timeout_sock = -1;
	}
}

static int check_ssl_access(char *addr) {
	static char *save_allow_once = NULL;
	static time_t time_allow_once = 0;

	/* due to "Fetch Cert" activities for SSL really need to "allow twice" */
	if (allow_once != NULL) {
		save_allow_once = strdup(allow_once);
		time_allow_once = time(NULL);
	} else if (save_allow_once != NULL) {
		if (getenv("X11VNC_NO_SSL_ALLOW_TWICE")) {
			;
		} else if (time(NULL) < time_allow_once + 30) {
			/* give them 30 secs to check and save the fetched cert. */
			allow_once = save_allow_once; 
			rfbLog("SSL: Permitting 30 sec grace period for allowonce.\n");
			rfbLog("SSL: Set X11VNC_NO_SSL_ALLOW_TWICE=1 to disable.\n");
		}
		save_allow_once = NULL;
		time_allow_once = 0;
	}

	return check_access(addr);
}

void accept_openssl(int mode, int presock) {
	int sock = -1, listen = -1, cport, csock, vsock;	
	int peerport = 0;
	int status, n, i, db = 0;
	struct sockaddr_in addr;
#ifdef __hpux
	int addrlen = sizeof(addr);
#else
	socklen_t addrlen = sizeof(addr);
#endif
	rfbClientPtr client;
	pid_t pid;
	char uniq[] = "_evilrats_";
	char cookie[256], rcookie[256], *name = NULL;
	int vencrypt_sel = 0;
	int anontls_sel = 0;
	char *ipv6_name = NULL;
	static double last_https = 0.0;
	static char last_get[256];
	static int first = 1;
	unsigned char *rb;

#if !LIBVNCSERVER_HAVE_LIBSSL 
	if (enc_str == NULL || strcmp(enc_str, "none")) {
		badnews("0 accept_openssl");
	}
#endif

	openssl_last_helper_pid = 0;

	/* zero buffers for use below. */
	for (i=0; i<256; i++) {
		if (first) {
			last_get[i] = '\0';
		}
		cookie[i]  = '\0';
		rcookie[i] = '\0';
	}
	first = 0;

	if (getenv("ACCEPT_OPENSSL_DEBUG")) {
		db = atoi(getenv("ACCEPT_OPENSSL_DEBUG"));
	}

	/* do INETD, VNC, or HTTPS cases (result is client socket or pipe) */
	if (mode == OPENSSL_INETD) {
		ssl_initialized = 1;

	} else if (mode == OPENSSL_VNC) {
		sock = accept(openssl_sock, (struct sockaddr *)&addr, &addrlen);
		if (sock < 0)  {
			rfbLog("SSL: accept_openssl: accept connection failed\n");
			rfbLogPerror("accept");
			if (ssl_no_fail) {
				clean_up_exit(1);
			}
			return;
		}
		listen = openssl_sock;

	} else if (mode == OPENSSL_VNC6 || mode == OPENSSL_HTTPS6) {
#if X11VNC_IPV6
		struct sockaddr_in6 a6;
		socklen_t a6len = sizeof(a6);
		int fd = (mode == OPENSSL_VNC6 ? openssl_sock6 : https_sock6); 

		sock = accept(fd, (struct sockaddr *)&a6, &a6len);
		if (sock < 0)  {
			rfbLog("SSL: accept_openssl: accept connection failed\n");
			rfbLogPerror("accept");
			if (ssl_no_fail) {
				clean_up_exit(1);
			}
			return;
		}
		ipv6_name = ipv6_getipaddr((struct sockaddr *)&a6, a6len);
		if (!ipv6_name) ipv6_name = strdup("unknown");
		listen = fd;
#endif
	} else if (mode == OPENSSL_REVERSE) {
		sock = presock;
		if (sock < 0)  {
			rfbLog("SSL: accept_openssl: connection failed\n");
			if (ssl_no_fail) {
				clean_up_exit(1);
			}
			return;
		}
		if (getenv("OPENSSL_REVERSE_DEBUG")) fprintf(stderr, "OPENSSL_REVERSE: ipv6_client_ip_str: %s\n", ipv6_client_ip_str);
		if (ipv6_client_ip_str != NULL) {
			ipv6_name = strdup(ipv6_client_ip_str);
		}
		listen = -1;

	} else if (mode == OPENSSL_HTTPS) {
		sock = accept(https_sock, (struct sockaddr *)&addr, &addrlen);
		if (sock < 0)  {
			rfbLog("SSL: accept_openssl: accept connection failed\n");
			rfbLogPerror("accept");
			if (ssl_no_fail) {
				clean_up_exit(1);
			}
			return;
		}
		listen = https_sock;
	}
	if (db) fprintf(stderr, "SSL: accept_openssl: sock: %d\n", sock);

	if (openssl_last_ip) {
		free(openssl_last_ip);
		openssl_last_ip = NULL;
	}
	if (mode == OPENSSL_INETD) {
		openssl_last_ip = get_remote_host(fileno(stdin));
	} else if (mode == OPENSSL_VNC6 || mode == OPENSSL_HTTPS6) {
		openssl_last_ip = ipv6_name;
	} else if (mode == OPENSSL_REVERSE && ipv6_name != NULL) {
		openssl_last_ip = ipv6_name;
	} else {
		openssl_last_ip = get_remote_host(sock);
	}

	if (!check_ssl_access(openssl_last_ip)) {
		rfbLog("SSL: accept_openssl: denying client %s\n", openssl_last_ip);
		rfbLog("SSL: accept_openssl: does not match -allow or other reason.\n");
		close(sock);
		sock = -1;
		if (ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}

	/* now make a listening socket for child to connect back to us by: */

	cport = find_free_port(20000, 22000);
	if (! cport && ipv6_listen) {
		rfbLog("SSL: accept_openssl: seeking IPv6 port.\n");
		cport = find_free_port6(20000, 22000);
		rfbLog("SSL: accept_openssl: IPv6 port: %d\n", cport);
	}
	if (! cport) {
		rfbLog("SSL: accept_openssl: could not find open port.\n");
		close(sock);
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: cport: %d\n", cport);

	csock = listen_tcp(cport, htonl(INADDR_LOOPBACK), 1);

	if (csock < 0) {
		rfbLog("SSL: accept_openssl: could not listen on port %d.\n",
		    cport);
		close(sock);
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: csock: %d\n", csock);

	fflush(stderr);

	/*
	 * make a simple cookie to id the child socket, not foolproof
	 * but hard to guess exactly (just worrying about local lusers
	 * here, since we use INADDR_LOOPBACK).
	 */
	rb = (unsigned char *) calloc(6, 1);
#if LIBVNCSERVER_HAVE_LIBSSL 
	RAND_bytes(rb, 6);
#endif
	sprintf(cookie, "RB=%d%d%d%d%d%d/%f%f/%p",
	    rb[0], rb[1], rb[2], rb[3], rb[4], rb[5],
            dnow() - x11vnc_start, x11vnc_start, (void *)rb);

	if (mode == OPENSSL_VNC6) {
		name = strdup(ipv6_name);
		peerport = get_remote_port(sock);
	} else if (mode == OPENSSL_REVERSE && ipv6_name != NULL) {
		name = strdup(ipv6_name);
		peerport = get_remote_port(sock);
	} else if (mode != OPENSSL_INETD) {
		name = get_remote_host(sock);
		peerport = get_remote_port(sock);
	} else {
		openssl_last_ip = get_remote_host(fileno(stdin));
		peerport = get_remote_port(fileno(stdin));
		if (openssl_last_ip) {
			name = strdup(openssl_last_ip);
		} else {
			name = strdup("unknown");
		}
	}
	if (name) {
		if (mode == OPENSSL_INETD) {
			rfbLog("SSL: (inetd) spawning helper process "
			    "to handle: %s:%d\n", name, peerport);
		} else {
			rfbLog("SSL: spawning helper process to handle: "
			    "%s:%d\n", name, peerport);
		}
		free(name);
		name = NULL;
	}

	if (certret) {
		free(certret);
	}
	if (certret_str) {
		free(certret_str);
		certret_str = NULL;
	}
	certret = strdup("/tmp/x11vnc-certret.XXXXXX");
	omode = umask(077);
	certret_fd = mkstemp(certret);
	umask(omode);
	if (certret_fd < 0) {
		free(certret);
		certret = NULL;
		certret_fd = -1;
	}

	if (dhret) {
		free(dhret);
	}
	if (dhret_str) {
		free(dhret_str);
		dhret_str = NULL;
	}
	dhret = strdup("/tmp/x11vnc-dhret.XXXXXX");
	omode = umask(077);
	dhret_fd = mkstemp(dhret);
	umask(omode);
	if (dhret_fd < 0) {
		free(dhret);
		dhret = NULL;
		dhret_fd = -1;
	}

	/* now fork the child to handle the SSL: */
	pid = fork();

	if (pid > 0) {
		rfbLog("SSL: helper for peerport %d is pid %d: \n",
		    peerport, (int) pid);
	}

	if (pid < 0) {
		rfbLog("SSL: accept_openssl: could not fork.\n");
		rfbLogPerror("fork");
		close(sock);
		close(csock);
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;

	} else if (pid == 0) {
		int s_in, s_out, httpsock = -1;
		int vncsock;
		int i, have_httpd = 0;
		int f_in  = fileno(stdin);
		int f_out = fileno(stdout);
		int skip_vnc_tls = mode == OPENSSL_HTTPS ? 1 : 0;

		if (db) fprintf(stderr, "helper pid in: %d %d %d %d\n", f_in, f_out, sock, listen);

		/* reset all handlers to default (no interrupted() calls) */
		unset_signals();

		/* close all non-essential fd's */
		for (i=0; i<256; i++) {
			if (mode == OPENSSL_INETD) {
				if (i == f_in || i == f_out) {
					continue;
				}
			}
			if (i == sock) {
				continue;
			}
			if (i == 2) {
				continue;
			}
			close(i);
		}

		/*
		 * sadly, we are a long lived child and so the large
		 * framebuffer memory areas will soon differ from parent.
		 * try to free as much as possible.
		 */
		lose_ram();

		/* now connect back to parent socket: */
		vncsock = connect_tcp("127.0.0.1", cport);
		if (vncsock < 0) {
			rfbLog("SSL: ssl_helper[%d]: could not connect"
			    " back to: %d\n", getpid(), cport);
			rfbLog("SSL: ssl_helper[%d]: exit case 1 (no local vncsock)\n", getpid());
			exit(1);
		}
		if (db) fprintf(stderr, "vncsock %d\n", vncsock);

		/* try to initialize SSL with the remote client */

		if (mode == OPENSSL_INETD) {
			s_in  = fileno(stdin);
			s_out = fileno(stdout);
		} else {
			s_in = s_out = sock;
		}

		if (! ssl_init(s_in, s_out, skip_vnc_tls, last_https)) {
			close(vncsock);
			rfbLog("SSL: ssl_helper[%d]: exit case 2 (ssl_init failed)\n", getpid());
			exit(1);
		}

		if (vencrypt_selected != 0) {
			char *tbuf;
			tbuf = (char *) malloc(strlen(cookie) + 100);
			sprintf(tbuf, "%s,VENCRYPT=%d,%s", uniq, vencrypt_selected, cookie);
			write(vncsock, tbuf, strlen(cookie));
			goto wrote_cookie;
		} else if (anontls_selected != 0) {
			char *tbuf;
			tbuf = (char *) malloc(strlen(cookie) + 100);
			sprintf(tbuf, "%s,ANONTLS=%d,%s", uniq, anontls_selected, cookie);
			write(vncsock, tbuf, strlen(cookie));
			goto wrote_cookie;
		}

		/*
		 * things get messy below since we are trying to do
		 * *both* VNC and Java applet httpd through the same
		 * SSL socket.
		 */

		if (! screen) {
			close(vncsock);
			exit(1);
		}
		if (screen->httpListenSock >= 0 && screen->httpPort > 0) {
			have_httpd = 1;
		} else if (ipv6_http_fd >= 0) {
			have_httpd = 1;
		} else if (screen->httpListenSock == -2) {
			have_httpd = 1;
		}
		if (mode == OPENSSL_HTTPS && ! have_httpd) {
			rfbLog("SSL: accept_openssl[%d]: no httpd socket for "
			    "-https mode\n", getpid());
			close(vncsock);
			rfbLog("SSL: ssl_helper[%d]: exit case 3 (no httpd sock)\n", getpid());
			exit(1);
		}

		if (have_httpd) {
			int n = 0, is_http = 0;
			int hport = screen->httpPort; 
			char *iface = NULL;
			char *buf, *tbuf;

			buf  = (char *) calloc((ABSIZE+1), 1);
			tbuf = (char *) calloc((2*ABSIZE+1), 1);

			if (mode == OPENSSL_HTTPS) {
				/*
				 * for this mode we know it is HTTP traffic
				 * so we skip trying to guess.
				 */
				is_http = 1;
				n = 0;
				goto connect_to_httpd;
			}

			/*
			 * Check if there is stuff to read from remote end
			 * if so it is likely a GET or HEAD.
			 */
			if (! is_ssl_readable(s_in, last_https, last_get,
			    mode)) {
				goto write_cookie;
			}
	
			/* 
			 * read first 2 bytes to try to guess.  sadly,
			 * the user is often pondering a "non-verified
			 * cert" dialog for a long time before the GET
			 * is ever sent.  So often we timeout here.
			 */

			if (db) fprintf(stderr, "watch_for_http_traffic\n");

			is_http = watch_for_http_traffic(buf, &n, s_in);

			if (is_http < 0 || is_http == 0) {
				/*
				 * error or http not detected, fall back
				 * to normal VNC socket.
				 */
				if (db) fprintf(stderr, "is_http err: %d n: %d\n", is_http, n);
				write(vncsock, cookie, strlen(cookie));
				if (n > 0) {
					write(vncsock, buf, n);
				}
				goto wrote_cookie;
			}

			if (db) fprintf(stderr, "is_http: %d n: %d\n",
			    is_http, n);
			if (db) fprintf(stderr, "buf: '%s'\n", buf);

			if (strstr(buf, "/request.https.vnc.connection")) {
				char reply[] = "HTTP/1.0 200 OK\r\n"
				    "Content-Type: octet-stream\r\n"
				    "Connection: Keep-Alive\r\n"
				    "VNC-Server: x11vnc\r\n"
				    "Pragma: no-cache\r\n\r\n";
				/*
				 * special case proxy coming thru https
				 * instead of a direct SSL connection.
				 */
				rfbLog("Handling VNC request via https GET. [%d]\n", getpid());
				rfbLog("-- %s\n", buf);

				if (strstr(buf, "/reverse.proxy")) {
					char *buf2;
					int n, ptr;
#if !LIBVNCSERVER_HAVE_LIBSSL
					write(s_out, reply, strlen(reply));
#else
					SSL_write(ssl, reply, strlen(reply));
#endif
				
					buf2  = (char *) calloc((8192+1), 1);
					n = 0;
					ptr = 0;
					while (ptr < 8192) {
#if !LIBVNCSERVER_HAVE_LIBSSL
						n = read(s_in, buf2 + ptr, 1);
#else
						n = SSL_read(ssl, buf2 + ptr, 1);
#endif
						if (n > 0) {
							ptr += n;
						}
						if (db) fprintf(stderr, "buf2: '%s'\n", buf2);

						if (strstr(buf2, "\r\n\r\n")) {
							break;
						}
					}
					free(buf2);
				}
				goto write_cookie;

			} else if (strstr(buf, "/check.https.proxy.connection")) {
				char reply[] = "HTTP/1.0 200 OK\r\n"
				    "Connection: close\r\n"
				    "Content-Type: octet-stream\r\n"
				    "VNC-Server: x11vnc\r\n"
				    "Pragma: no-cache\r\n\r\n";

				rfbLog("Handling Check HTTPS request via https GET. [%d]\n", getpid());
				rfbLog("-- %s\n", buf);

#if !LIBVNCSERVER_HAVE_LIBSSL
				write(s_out, reply, strlen(reply));
#else
				SSL_write(ssl, reply, strlen(reply));
				SSL_shutdown(ssl);
#endif

				strcpy(tbuf, uniq);
				strcat(tbuf, cookie);
				write(vncsock, tbuf, strlen(tbuf));
				close(vncsock);

				rfbLog("SSL: ssl_helper[%d]: exit case 4 (check.https.proxy.connection)\n", getpid());
				exit(0);
			}
			connect_to_httpd:

			/*
			 * Here we go... no turning back.  we have to
			 * send failure to parent and close socket to have
			 * http processed at all in a timely fashion...
			 */

			/* send the failure tag: */
			strcpy(tbuf, uniq);

			if (https_port_redir < 0 || (strstr(buf, "PORT=") || strstr(buf, "port="))) {
				char *q = strstr(buf, "Host:");
				int fport = 443, match = 0;
				char num[16];

				if (q && strstr(q, "\n")) {
				    q += strlen("Host:") + 1;
				    while (*q != '\n') {
					int p;
					if (*q == ':' && sscanf(q, ":%d", &p) == 1) {
						if (p > 0 && p < 65536) {
							fport = p;
							match = 1;
							break;
						}
					}
					q++;
				    }
				}
				if (!match || !https_port_redir) {
					int p;
					if (sscanf(buf, "PORT=%d,", &p) == 1) {
						if (p > 0 && p < 65536) {
							fport = p;
						}
					} else if (sscanf(buf, "port=%d,", &p) == 1) {
						if (p > 0 && p < 65536) {
							fport = p;
						}
					}
				}
				sprintf(num, "HP=%d,", fport);
				strcat(tbuf, num);
			}

			if (strstr(buf, "HTTP/") != NULL)  {
				char *q, *str;
				/*
				 * Also send back the GET line for heuristics.
				 * (last_https, get file).
				 */
				str = strdup(buf);
				q = strstr(str, "HTTP/");
				if (q != NULL) {
					*q = '\0';	
					strcat(tbuf, str);
				}
				free(str);
			}

			/*
			 * Also send the cookie to pad out the number of
			 * bytes to more than the parent wants to read.
			 * Since this is the failure case, it does not
			 * matter that we send more than strlen(cookie).
			 */
			strcat(tbuf, cookie);
			write(vncsock, tbuf, strlen(tbuf));

			usleep(150*1000);
			if (db) fprintf(stderr, "close vncsock: %d\n", vncsock);
			close(vncsock);

			/* now, finally, connect to the libvncserver httpd: */
			if (screen->listenInterface == htonl(INADDR_ANY) ||
			    screen->listenInterface == htonl(INADDR_NONE)) {
				iface = "127.0.0.1";
			} else {
				struct in_addr in;
				in.s_addr = screen->listenInterface;
				iface = inet_ntoa(in);
			}
			if (iface == NULL || !strcmp(iface, "")) {
				iface = "127.0.0.1";
			}
			if (db) fprintf(stderr, "iface: %s:%d\n", iface, hport);
			usleep(150*1000);

			httpsock = connect_tcp(iface, hport);

			if (httpsock < 0) {
				/* UGH, after all of that! */
				rfbLog("Could not connect to httpd socket!\n");
				rfbLog("SSL: ssl_helper[%d]: exit case 5.\n", getpid());
				exit(1);
			}
			if (db) fprintf(stderr, "ssl_helper[%d]: httpsock: %d %d\n",
			    getpid(), httpsock, n);

			/*
			 * send what we read to httpd, and then connect
			 * the rest of the SSL session to it:
			 */
			if (n > 0) {
				char *s = getenv("X11VNC_EXTRA_HTTPS_PARAMS");
				int did_extra = 0;

				if (db) fprintf(stderr, "sending http buffer httpsock: %d n=%d\n'%s'\n", httpsock, n, buf);
				if (s != NULL) {
					char *q = strstr(buf, " HTTP/");
					if (q) {
						int m;
						*q = '\0';
						m = strlen(buf);
						write(httpsock, buf, m);
						write(httpsock, s, strlen(s));
						*q = ' ';
						write(httpsock, q, n-m);
						did_extra = 1;
					}
				}
				if (!did_extra) {
					write(httpsock, buf, n);
				}
			}
			ssl_xfer(httpsock, s_in, s_out, is_http);
			rfbLog("SSL: ssl_helper[%d]: exit case 6 (https ssl_xfer done)\n", getpid());
			exit(0);
		}

		/*
		 * ok, back from the above https mess, simply send the
		 * cookie back to the parent (who will attach us to
		 * libvncserver), and connect the rest of the SSL session
		 * to it.
		 */
		write_cookie:
		write(vncsock, cookie, strlen(cookie));

		wrote_cookie:
		ssl_xfer(vncsock, s_in, s_out, 0);
		rfbLog("SSL: ssl_helper[%d]: exit case 7 (ssl_xfer done)\n", getpid());
		if (0) usleep(50 * 1000);
		exit(0);
	}
	/* parent here */

	if (mode != OPENSSL_INETD) {
		close(sock);
	}
	if (db) fprintf(stderr, "helper process is: %d\n", pid);

	/* accept connection from our child.  */
	signal(SIGALRM, csock_timeout);
	csock_timeout_sock = csock;
	alarm(20);

	vsock = accept(csock, (struct sockaddr *)&addr, &addrlen);

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	close(csock);


	if (vsock < 0) {
		rfbLog("SSL: accept_openssl: connection from ssl_helper[%d] FAILED.\n", pid);
		rfbLogPerror("accept");

		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		if (certret_fd >= 0) {
			close(certret_fd);
			certret_fd = -1;
		}
		if (certret) {
			unlink(certret);
		}
		if (dhret_fd >= 0) {
			close(dhret_fd);
			dhret_fd = -1;
		}
		if (dhret) {
			unlink(dhret);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: vsock: %d\n", vsock);

	n = read(vsock, rcookie, strlen(cookie));
	if (n < 0 && errno != 0) {
		rfbLogPerror("read");
	}

	if (certret) {
		struct stat sbuf;
		sbuf.st_size = 0;
		if (certret_fd >= 0 && stat(certret, &sbuf) == 0 && sbuf.st_size > 0) {
			certret_str = (char *) calloc(sbuf.st_size+1, 1);
			read(certret_fd, certret_str, sbuf.st_size);
			close(certret_fd);
			certret_fd = -1;
		}
		if (certret_fd >= 0) {
			close(certret_fd);
			certret_fd = -1;
		}
		unlink(certret);
		if (certret_str && strstr(certret_str, "NOCERT") == certret_str) {
			free(certret_str);
			certret_str = NULL;
		}
		if (0 && certret_str) {
			fprintf(stderr, "certret_str[%d]:\n%s\n", (int) sbuf.st_size, certret_str);
		}
	}

	if (dhret) {
		struct stat sbuf;
		sbuf.st_size = 0;
		if (dhret_fd >= 0 && stat(dhret, &sbuf) == 0 && sbuf.st_size > 0) {
			dhret_str = (char *) calloc(sbuf.st_size+1, 1);
			read(dhret_fd, dhret_str, sbuf.st_size);
			close(dhret_fd);
			dhret_fd = -1;
		}
		if (dhret_fd >= 0) {
			close(dhret_fd);
			dhret_fd = -1;
		}
		unlink(dhret);
		if (dhret_str && strstr(dhret_str, "NOCERT") == dhret_str) {
			free(dhret_str);
			dhret_str = NULL;
		}
		if (dhret_str) {
			if (new_dh_params == NULL) {
				fprintf(stderr, "dhret_str[%d]:\n%s\n", (int) sbuf.st_size, dhret_str);
				new_dh_params = strdup(dhret_str);
			}
		}
	}

	if (0) {
		fprintf(stderr, "rcookie: %s\n", rcookie);
		fprintf(stderr, "cookie:  %s\n", cookie);
	}

	if (strstr(rcookie, uniq) == rcookie) {
		char *q = strstr(rcookie, "RB=");
		if (q && strstr(cookie, q) == cookie) {
			vencrypt_sel = 0;
			anontls_sel = 0;
			q = strstr(rcookie, "VENCRYPT=");
			if (q && sscanf(q, "VENCRYPT=%d,", &vencrypt_sel) == 1) {
				if (vencrypt_sel != 0) {
					rfbLog("SSL: VENCRYPT mode=%d accepted. helper[%d]\n", vencrypt_sel, pid);
					goto accept_client;
				}
			}
			q = strstr(rcookie, "ANONTLS=");
			if (q && sscanf(q, "ANONTLS=%d,", &anontls_sel) == 1) {
				if (anontls_sel != 0) {
					rfbLog("SSL: ANONTLS mode=%d accepted. helper[%d]\n", anontls_sel, pid);
					goto accept_client;
				}
			}
		}
	}

	if (n != (int) strlen(cookie) || strncmp(cookie, rcookie, n)) {
		rfbLog("SSL: accept_openssl: cookie from ssl_helper[%d] FAILED. %d\n", pid, n);
		if (db) fprintf(stderr, "'%s'\n'%s'\n", cookie, rcookie);
		close(vsock);

		if (strstr(rcookie, uniq) == rcookie) {
			int i;
			double https_download_wait_time = 15.0;

			if (getenv("X11VNC_HTTPS_DOWNLOAD_WAIT_TIME")) {
				https_download_wait_time = atof(getenv("X11VNC_HTTPS_DOWNLOAD_WAIT_TIME"));
			}

			rfbLog("SSL: BUT WAIT! HTTPS for helper process[%d] succeeded. Good.\n", pid);
			if (mode != OPENSSL_HTTPS) {
				last_https = dnow();
				for (i=0; i<256; i++) {
					last_get[i] = '\0';
				}
				strncpy(last_get, rcookie, 100);
				if (db) fprintf(stderr, "last_get: '%s'\n", last_get);
			}
			if (rcookie && strstr(rcookie, "VncViewer.class")) {
				rfbLog("\n");
				rfbLog("helper[%d]:\n", pid);
				rfbLog("***********************************************************\n");
				rfbLog("SSL: WARNING CLIENT ASKED FOR NONEXISTENT 'VncViewer.class'\n");
				rfbLog("SSL: USER NEEDS TO MAKE SURE THE JAVA PLUGIN IS INSTALLED\n");
				rfbLog("SSL: AND WORKING PROPERLY (e.g. a test-java-plugin page.)\n");
				rfbLog("SSL: AND/OR USER NEEDS TO **RESTART** HIS WEB BROWSER.\n");
				rfbLog("SSL: SOMETIMES THE BROWSER 'REMEMBERS' FAILED APPLET DOWN-\n");
				rfbLog("SSL: LOADS AND RESTARTING IT IS THE ONLY WAY TO FIX THINGS.\n");
				rfbLog("***********************************************************\n");
				rfbLog("\n");
			}
			ssl_helper_pid(pid, -2);

			if (https_port_redir) {
				double start;
				int origport = screen->port;
				int useport = screen->port;
				int saw_httpsock = 0;
				/* to expand $PORT correctly in index.vnc */
				if (https_port_redir < 0) {
					char *q = strstr(rcookie, "HP=");
					if (q) {
						int p;
						if (sscanf(q, "HP=%d,", &p) == 1) {
							useport = p;
						}
					}
				} else {
					useport = https_port_redir;
				}
				screen->port = useport;
				if (origport != useport) {
					rfbLog("SSL: -httpsredir guess port: %d  helper[%d]\n", screen->port, pid);
				}

				start = dnow();
				while (dnow() < start + https_download_wait_time) {
					if (screen->httpSock >= 0) saw_httpsock = 1;
					rfbPE(10000);
					usleep(10000);
					if (screen->httpSock >= 0) saw_httpsock = 1;
					waitpid(pid, &status, WNOHANG); 
					if (kill(pid, 0) != 0) {
						rfbLog("SSL: helper[%d] pid finished\n", pid);
						break;
					}
					if (0 && saw_httpsock && screen->httpSock < 0) {
						/* this check can kill the helper too soon. */
						rfbLog("SSL: httpSock for helper[%d] went away\n", pid);
						break;
					}
				}
				rfbLog("SSL: guessing child helper[%d] https finished. dt=%.6f\n",
				    pid, dnow() - start);

				rfbPE(10000);
				rfbPE(10000);
				rfbPE(10000);

				screen->port = origport;
				ssl_helper_pid(0, -2);
				if (mode == OPENSSL_INETD) {
					clean_up_exit(1);
				}
			} else if (mode == OPENSSL_INETD) {
				double start;
				int saw_httpsock = 0;

				/* to expand $PORT correctly in index.vnc */
				if (screen->port == 0) {
					int fd = fileno(stdin);
					if (getenv("X11VNC_INETD_PORT")) {
						/* mutex */
						screen->port = atoi(getenv(
						    "X11VNC_INETD_PORT"));
					} else {
						int tport = get_local_port(fd);
						if (tport > 0) {
							screen->port = tport;
						}
					}
				}
				rfbLog("SSL: screen->port %d for helper[%d]\n", screen->port, pid);

				/* kludge for https fetch via inetd */
				start = dnow();
				while (dnow() < start + https_download_wait_time) {
					if (screen->httpSock >= 0) saw_httpsock = 1;
					rfbPE(10000);
					usleep(10000);
					if (screen->httpSock >= 0) saw_httpsock = 1;
					waitpid(pid, &status, WNOHANG); 
					if (kill(pid, 0) != 0) {
						rfbLog("SSL: helper[%d] pid finished\n", pid);
						break;
					}
					if (0 && saw_httpsock && screen->httpSock < 0) {
						/* this check can kill the helper too soon. */
						rfbLog("SSL: httpSock for helper[%d] went away\n", pid);
						break;
					}
				}
				rfbLog("SSL: OPENSSL_INETD guessing "
				    "child helper[%d] https finished. dt=%.6f\n",
				    pid, dnow() - start);

				rfbPE(10000);
				rfbPE(10000);
				rfbPE(10000);

				ssl_helper_pid(0, -2);
				clean_up_exit(1);
			}
			/* this will actually only get earlier https */
			ssl_helper_pid(0, -2);
			return;
		}
		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}

	accept_client:

	if (db) fprintf(stderr, "accept_openssl: cookie good: %s\n", cookie);

	rfbLog("SSL: handshake with helper process[%d] succeeded.\n", pid);

	openssl_last_helper_pid = pid;
	ssl_helper_pid(pid, vsock);

	if (vnc_redirect) {
		vnc_redirect_sock = vsock;
		openssl_last_helper_pid = 0;
		return;
	}

	client = create_new_client(vsock, 0);
	openssl_last_helper_pid = 0;

	if (client) {
		int swt = 0;
		if (mode == OPENSSL_VNC6 && openssl_last_ip != NULL) {
			swt = 1;
		} else if (mode == OPENSSL_REVERSE && ipv6_name != NULL && openssl_last_ip != NULL) {
			swt = 1;
		}
		if (swt) {
			if (client->host) {
				free(client->host);
			}
			client->host = strdup(openssl_last_ip);
		}
		if (db) fprintf(stderr, "accept_openssl: client %p\n", (void *) client);
		if (db) fprintf(stderr, "accept_openssl: new_client %p\n", (void *) screen->newClientHook);
		if (db) fprintf(stderr, "accept_openssl: new_client %p\n", (void *) new_client);
		if (mode == OPENSSL_INETD) {
			inetd_client = client;
			client->clientGoneHook = client_gone;
		}
		if (openssl_last_ip &&
		    strpbrk(openssl_last_ip, "0123456789") == openssl_last_ip) {
			client->host = strdup(openssl_last_ip);
		}
		if (vencrypt_sel != 0) {
			client->protocolMajorVersion = 3;
			client->protocolMinorVersion = 8;
#if LIBVNCSERVER_HAVE_LIBSSL
			if (!finish_vencrypt_auth(client, vencrypt_sel)) {
				rfbCloseClient(client);
				client = NULL;
			}
#else
			badnews("3 accept_openssl");
#endif
		} else if (anontls_sel != 0) {
			client->protocolMajorVersion = 3;
			client->protocolMinorVersion = 8;
			rfbAuthNewClient(client);
		}
		if (use_threads && client != NULL) {
			rfbStartOnHoldClient(client);
		}
		/* try to get RFB proto done now. */
		progress_client();
	} else {
		rfbLog("SSL: accept_openssl: rfbNewClient failed.\n");
		close(vsock);

		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
}

void raw_xfer(int csock, int s_in, int s_out) {
	char buf0[8192];
	int sz = 8192, n, m, status, db = 1;
	char *buf;
#ifdef FORK_OK
	pid_t par = getpid();
	pid_t pid = fork();

	buf = buf0;
	if (vnc_redirect) {
		/* change buf size some direction. */
	}

	if (getenv("X11VNC_DEBUG_RAW_XFER")) {
		db = atoi(getenv("X11VNC_DEBUG_RAW_XFER"));
	}
	if (pid < 0) {
		exit(1);
	}
	/* this is for testing or special helper usage, no SSL just socket redir */
	if (pid) {
		if (db) rfbLog("raw_xfer start: %d -> %d/%d\n", csock, s_in, s_out);

		while (1) {
			n = read(csock, buf, sz);
			if (n == 0 || (n < 0 && errno != EINTR) ) {
				break;
			} else if (n > 0) {
				int len = n;
				char *src = buf;
				if (db > 1) write(2, buf, n);
				while (len > 0) {
					m = write(s_out, src, len);
					if (m > 0) {
						src += m;
						len -= m;
						continue;
					}
					if (m < 0 && (errno == EINTR || errno == EAGAIN)) {
						continue;
					}
if (db) rfbLog("raw_xfer bad write:  %d -> %d | %d/%d  errno=%d\n", csock, s_out, m, n, errno);
					break;
				}
			}
		}
		usleep(250*1000);
		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (db) rfbLog("raw_xfer done:  %d -> %d\n", csock, s_out);

	} else {
		if (db) usleep(50*1000);
		if (db) rfbLog("raw_xfer start: %d <- %d\n", csock, s_in);

		while (1) {
			n = read(s_in, buf, sz);
			if (n == 0 || (n < 0 && errno != EINTR) ) {
				break;
			} else if (n > 0) {
				int len = n;
				char *src = buf;
				if (db > 1) write(2, buf, n);
				while (len > 0) {
					m = write(csock, src, len);
					if (m > 0) {
						src += m;
						len -= m;
						continue;
					}
					if (m < 0 && (errno == EINTR || errno == EAGAIN)) {
						continue;
					}
		if (db) rfbLog("raw_xfer bad write:  %d <- %d | %d/%d errno=%d\n", csock, s_in, m, n, errno);
					break;
				}
			}
		}
		usleep(250*1000);
		kill(par, SIGTERM);
		waitpid(par, &status, WNOHANG); 
		if (db) rfbLog("raw_xfer done:  %d <- %d\n", csock, s_in);
	}
	close(csock);
	close(s_in);
	close(s_out);
#endif	/* FORK_OK */
}

/* compile with -DENC_HAVE_OPENSSL=0 to disable enc stuff but still have ssl */

#define ENC_MODULE

#if LIBVNCSERVER_HAVE_LIBSSL
#ifndef ENC_HAVE_OPENSSL
#define ENC_HAVE_OPENSSL 1
#endif
#else
#define ENC_HAVE_OPENSSL 0
#endif

#define ENC_DISABLE_SHOW_CERT 

#include "enc.h"

static void symmetric_encryption_xfer(int csock, int s_in, int s_out) {
	char tmp[100];
	char *cipher, *keyfile, *q;

	if (! enc_str) {
		return;
	}
	cipher = (char *) malloc(strlen(enc_str) + 100);
	q = strchr(enc_str, ':');
	if (!q) return;
	*q = '\0';
	if (getenv("X11VNC_USE_ULTRADSM_IV")) {
		sprintf(cipher, "rev:%s", enc_str);
	} else {
		sprintf(cipher, "noultra:rev:%s", enc_str);
	}
	keyfile = strdup(q+1);
	*q = ':';


	/* TBD: s_in != s_out */
	if (s_out) {}

	sprintf(tmp, "fd=%d,%d", s_in, csock);

	enc_do(cipher, keyfile, "-1", tmp);
}

