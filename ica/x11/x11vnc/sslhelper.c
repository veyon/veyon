/* -- sslhelper.c -- */

#include "x11vnc.h"
#include "inet.h"
#include "cleanup.h"
#include "screen.h"
#include "scan.h"
#include "connections.h"
#include "sslcmds.h"

#define OPENSSL_INETD 1
#define OPENSSL_VNC   2
#define OPENSSL_HTTPS 3

#define DO_DH 0

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID
#define FORK_OK
#endif
#endif

#ifdef NO_SSL_OR_UNIXPW
#undef FORK_OK
#undef LIBVNCSERVER_HAVE_LIBSSL
#define LIBVNCSERVER_HAVE_LIBSSL 0
#endif


int openssl_sock = -1;
int openssl_port_num = 0;
int https_sock = -1;
pid_t openssl_last_helper_pid = 0;
char *openssl_last_ip = NULL;

void raw_xfer(int csock, int s_in, int s_out);

#if !LIBVNCSERVER_HAVE_LIBSSL
int openssl_present(void) {return 0;}
static void badnews(void) {
	use_openssl = 0;
	use_stunnel = 0;
	rfbLog("** not compiled with libssl OpenSSL support **\n");
	clean_up_exit(1);
}
void openssl_init(void) {badnews();}
void openssl_port(void) {badnews();}
void https_port(void) {badnews();}
void check_openssl(void) {if (use_openssl) badnews();}
void check_https(void) {if (use_openssl) badnews();}
void ssl_helper_pid(pid_t pid, int sock) {badnews(); sock = pid;}
void accept_openssl(int mode) {mode = 0; badnews();}
char *find_openssl_bin(void) {badnews(); return NULL;}
char *get_saved_pem(char *string, int create) {badnews(); return NULL;}
#else

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

int openssl_present(void);
void openssl_init(void);
void openssl_port(void);
void check_openssl(void);
void check_https(void);
void ssl_helper_pid(pid_t pid, int sock);
void accept_openssl(int mode);
char *find_openssl_bin(void);
char *get_saved_pem(char *string, int create);

static SSL_CTX *ctx = NULL;
static RSA *rsa_512 = NULL;
static RSA *rsa_1024 = NULL;
static SSL *ssl = NULL;


static void init_prng(void);
static void sslerrexit(void);
static char *get_input(char *tag, char **in);
static char *create_tmp_pem(char *path, int prompt);
static int  ssl_init(int s_in, int s_out);
static void ssl_xfer(int csock, int s_in, int s_out, int is_https);

#ifndef FORK_OK
void openssl_init(void) {
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
		char *new = NULL;
		if (create) {
			new = create_tmp_pem(path, prompt);
			if (! getenv("X11VNC_SSL_NO_PASSPHRASE") && ! inetd) {
				sslEncKey(new, 0);
			}
		}
		return new;
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
		fprintf(stderr, "could not find openssl(1) program in PATH.\n");
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
		fprintf(stderr, "(also checked: %s)\n", extra);
		return NULL;
	}
	return exe;
}

/* uses /usr/bin/openssl to create a tmp cert */

static char *create_tmp_pem(char *pathin, int prompt) {
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
	L = strdup(UT.sysname ? UT.sysname : "unknown-os");
	snprintf(line, 1024, "%s-%f", UT.nodename ? UT.nodename :
	    "unknown-node", dnow());
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
	rfbLog("This will NOT prevent man-in-the-middle attacks UNLESS you\n");	
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
	pem_fd = mkstemp(pem);

	if (cnf_fd < 0 || pem_fd < 0) {
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
		char cmd[100];
		if (inetd) {
			sprintf(cmd, "openssl x509 -text -in '%s' 1>&2", pem);
		} else {
			sprintf(cmd, "openssl x509 -text -in '%s'", pem);
		}
		fprintf(stderr, "\n");
		system(cmd);
		fprintf(stderr, "\n");
	}

	if (pathin) {
		unlink(pem);
		return strdup(pathin);
	} else {
		return strdup(pem);
	}
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
	
static char *get_ssl_verify_file(char *str_in) {
	char *p, *str, *cdir, *tmp;
	char *tfile, *tfile2;
	FILE *file;
	struct stat sbuf;
	int count = 0;

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

	sprintf(tfile, "%s/sslverify-load-%d.crts", tmp, getpid());

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
			fprintf(stderr, "sslverify: loaded %s\n", tfile2);
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
				fprintf(stderr, "sslverify: loaded %s\n",
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
			fprintf(stderr, "sslverify: loaded %s\n", tfile2);
			count++;
		}
		p = strtok(NULL, ",");
	}
	fclose(file);
	free(tfile2);
	free(str);

	fprintf(stderr, "sslverify: using %d client certs in %s\n", count,
	    tfile);

	return tfile;
}

void openssl_init(void) {
	int db = 0, tmp_pem = 0, do_dh;
	FILE *in;
	double ds;
	long mode;

	do_dh = DO_DH;

	if (! quiet) {
		rfbLog("\n");
		rfbLog("Initializing SSL.\n");
	}
	if (db) fprintf(stderr, "\nSSL_load_error_strings()\n");

	SSL_load_error_strings();

	if (db) fprintf(stderr, "SSL_library_init()\n");

	SSL_library_init();

	if (db) fprintf(stderr, "init_prng()\n");

	init_prng();

	ctx = SSL_CTX_new( SSLv23_server_method() );

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

	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout(ctx, 300);

	ds = dnow();
	if (! openssl_pem) {
		openssl_pem = create_tmp_pem(NULL, 0);
		if (! openssl_pem) {
			rfbLog("openssl_init: could not create temporary,"
			    " self-signed PEM.\n");	
			clean_up_exit(1);
		}
		tmp_pem = 1;

	} else if (strstr(openssl_pem, "SAVE") == openssl_pem) {
		openssl_pem = get_saved_pem(openssl_pem, 1);
		if (! openssl_pem) {
			rfbLog("openssl_init: could not create or open"
			    " saved PEM:\n", openssl_pem);	
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

	if (ssl_verify) {
		struct stat sbuf;
		char *file;
		int lvl;

		file = get_ssl_verify_file(ssl_verify);

		if (!file || stat(file, &sbuf) != 0) {
			rfbLog("openssl_init: -sslverify does not exists %s.\n",
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
		SSL_CTX_set_verify(ctx, lvl, NULL);
		if (strstr(file, "tmp/sslverify-load-")) {
			/* temporary file */
			unlink(file);
		}
	} else {
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	}

	rfbLog("\n");
}

void openssl_port(void) {
	int sock, shutdown = 0;
	static int port = 0;
	static in_addr_t iface = INADDR_ANY;
	int db = 0;

	if (! screen) {
		rfbLog("openssl_port: no screen!\n");
		clean_up_exit(1);
	}
	if (inetd) {
		ssl_initialized = 1;
		return;
	}

	if (screen->listenSock > -1 && screen->port > 0) {
		port = screen->port;
		shutdown = 1;
	}
	if (screen->listenInterface) {
		iface = screen->listenInterface;
	}

	if (shutdown) {
		if (db) fprintf(stderr, "shutting down %d/%d\n",
		    port, screen->listenSock);
		rfbShutdownSockets(screen);
	}

	sock = rfbListenOnTCPPort(port, iface);
	if (sock < 0) {
		rfbLog("openssl_port: could not reopen port %d\n", port);
		clean_up_exit(1);
	}
	rfbLog("openssl_port: listen on port/sock %d/%d\n", port, sock);
	openssl_sock = sock;
	openssl_port_num = port;

	ssl_initialized = 1;
}


void https_port(void) {
	int sock;
	static int port = 0;
	static in_addr_t iface = INADDR_ANY;
	int db = 0;

	/* as openssl_port above: open a listening socket for pure https: */
	if (https_port_num < 0) {
		return;
	}
	if (! screen) {
		rfbLog("https_port: no screen!\n");
		clean_up_exit(1);
	}
	if (screen->listenInterface) {
		iface = screen->listenInterface;
	}

	if (https_port_num == 0) {
		https_port_num = find_free_port(5801, 5851);
	}
	if (https_port_num <= 0) {
		rfbLog("https_port: could not find port %d\n", https_port_num);
		clean_up_exit(1);
	}
	port = https_port_num;

	sock = rfbListenOnTCPPort(port, iface);
	if (sock < 0) {
		rfbLog("https_port: could not open port %d\n", port);
		clean_up_exit(1);
	}
	if (db) fprintf(stderr, "https_port: listen on port/sock %d/%d\n",
	    port, sock);

	https_sock = sock;
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
	int i, empty, set, status, db = 0;

	if (first) {
		for (i=0; i < HPSIZE; i++)  {
			helpers[i] = 0;
			sockets[i] = 0;
		}
		first = 0;
	}

	if (pid == 0) {
		/* killall */
		for (i=0; i < HPSIZE; i++) {
			if (helpers[i] == 0) {
				sockets[i] = -1;
				continue;
			}
			if (kill(helpers[i], 0) == 0) {
				if (sock != -2) {
					if (sockets[i] >= 0) {
						close(sockets[i]);
					}
					kill(helpers[i], SIGTERM);
				}

#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID 
if (db) fprintf(stderr, "waitpid(%d)\n", helpers[i]);
				waitpid(helpers[i], &status, WNOHANG); 
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
if (db) fprintf(stderr, "waitpid(%d) 2\n", helpers[i]);
				waitpid(helpers[i], &status, WNOHANG); 
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

static int is_ssl_readable(int s_in, time_t last_https, char *last_get,
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
		tv.tv_sec  = 6;
	}

	/*
	 * increase the timeout if we know HTTP traffic has occurred
	 * recently:
	 */
	if (time(NULL) < last_https + 30) {
		tv.tv_sec  = 8;
		if (last_get && strstr(last_get, "VncViewer")) {
			tv.tv_sec  = 4;
		}
	}
if (db) fprintf(stderr, "tv_sec: %d - %s\n", (int) tv.tv_sec, last_get);

	FD_ZERO(&rd);
	FD_SET(s_in, &rd);

	do {
		nfd = select(s_in+1, &rd, NULL, NULL, &tv);
	} while (nfd < 0 && errno == EINTR);

	if (db) fprintf(stderr, "https nfd: %d\n", nfd);

	if (nfd <= 0 || ! FD_ISSET(s_in, &rd)) {
		return 0;
	}
	return 1;
}

#define ABSIZE 16384
static int watch_for_http_traffic(char *buf_a, int *n_a) {
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

	n = SSL_read(ssl, buf, 2);
	err = SSL_get_error(ssl, n);

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

	n2 = SSL_read(ssl, buf + n, ABSIZE - n);
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

static int wait_conn(int sock) {
	int conn;
	struct sockaddr_in addr;
#ifdef __hpux
	int addrlen = sizeof(addr);
#else
	socklen_t addrlen = sizeof(addr);
#endif

	signal(SIGALRM, csock_timeout);
	csock_timeout_sock = sock;
	
	alarm(15);
	conn = accept(sock, (struct sockaddr *)&addr, &addrlen);
	alarm(0);

	signal(SIGALRM, SIG_DFL);
	return conn;
}

int proxy_hack(int vncsock, int listen, int s_in, int s_out, char *cookie,
    int mode) {
	int sock1, db = 0;
	char reply[] = "HTTP/1.1 200 OK\r\n"
	    "Content-Type: octet-stream\r\n"
	    "Pragma: no-cache\r\n\r\n";
	char reply0[] = "HTTP/1.0 200 OK\r\n"
	    "Content-Type: octet-stream\r\n"
	    "Content-Length: 9\r\n"
	    "Pragma: no-cache\r\n\r\nGO_AHEAD\n";

	rfbLog("SSL: accept_openssl: detected https proxied connection"
	    " request.\n");

	if (getenv("ACCEPT_OPENSSL_DEBUG")) {
		db = atoi(getenv("ACCEPT_OPENSSL_DEBUG"));
	}

	SSL_write(ssl, reply0, strlen(reply0));
	SSL_shutdown(ssl);
	SSL_shutdown(ssl);
	close(s_in);
	close(s_out);
	SSL_free(ssl);

	if (mode == OPENSSL_VNC) {
		listen = openssl_sock;
	} else if (mode == OPENSSL_HTTPS) {
		listen = https_sock;
	} else {
		/* inetd */
		return 0;
	}

	sock1 = wait_conn(listen);

	if (csock_timeout_sock < 0 || sock1 < 0) {
		close(sock1);
		return 0;
	}

if (db) fprintf(stderr, "got applet input sock1: %d\n", sock1);

	if (! ssl_init(sock1, sock1)) {
if (db) fprintf(stderr, "ssl_init FAILED\n");
		exit(1);
	}

	SSL_write(ssl, reply, strlen(reply));

	{
		char *buf;
		int n = 0, ptr = 0;
	
		buf  = (char *) calloc((8192+1), 1);
		while (ptr < 8192) {
			n = SSL_read(ssl, buf + ptr, 8192 - ptr);
			if (n > 0) {
				ptr += n;
			}
if (db) fprintf(stderr, "buf: '%s'\n", buf);
			if (strstr(buf, "\r\n\r\n")) {
				break;
			}
		}
	}

	if (cookie) {
		write(vncsock, cookie, strlen(cookie));
	}
	ssl_xfer(vncsock, sock1, sock1, 0);

	return 1;
}

void accept_openssl(int mode) {
	int sock = -1, listen = -1, cport, csock, vsock;	
	int status, n, i, db = 0;
	struct sockaddr_in addr;
#ifdef __hpux
	int addrlen = sizeof(addr);
#else
	socklen_t addrlen = sizeof(addr);
#endif
	rfbClientPtr client;
	pid_t pid;
	char uniq[] = "__evilrats__";
	char cookie[128], rcookie[128], *name = NULL;
	static time_t last_https = 0;
	static char last_get[128];
	static int first = 1;

	openssl_last_helper_pid = 0;

	/* zero buffers for use below. */
	for (i=0; i<128; i++) {
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
	} else {
		openssl_last_ip = get_remote_host(sock);
	}

	/* now make a listening socket for child to connect back to us by: */

	cport = find_free_port(20000, 0);
	if (! cport) {
		rfbLog("SSL: accept_openssl: could not find open port.\n");
		close(sock);
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: cport: %d\n", cport);

	csock = rfbListenOnTCPPort(cport, htonl(INADDR_LOOPBACK));

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
	sprintf(cookie, "%f/%f", dnow(), x11vnc_start);

	if (mode != OPENSSL_INETD) {
		name = get_remote_host(sock);
	} else {
		openssl_last_ip = get_remote_host(fileno(stdin));
		if (openssl_last_ip) {
			name = strdup(openssl_last_ip);
		} else {
			name = strdup("unknown");
		}
	}
	if (name) {
		if (mode == OPENSSL_INETD) {
			rfbLog("SSL: (inetd) spawning helper process "
			    "to handle: %s\n", name);
		} else {
			rfbLog("SSL: spawning helper process to handle: "
			    "%s\n", name);
		}
		free(name);
		name = NULL;
	}

	/* now fork the child to handle the SSL: */
	pid = fork();

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
			if (i == listen) {
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
		vncsock = rfbConnectToTcpAddr("127.0.0.1", cport);
		if (vncsock < 0) {
			rfbLog("SSL: ssl_helper[%d]: could not connect"
			    " back to: %d\n", getpid(), cport);
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

		if (! ssl_init(s_in, s_out)) {
			close(vncsock);
			exit(1);
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
		}
		if (mode == OPENSSL_HTTPS && ! have_httpd) {
			rfbLog("SSL: accept_openssl[%d]: no httpd socket for "
			    "-https mode\n", getpid());
			close(vncsock);
			exit(1);
		}

		if (have_httpd) {
			int n, is_http;
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

			is_http = watch_for_http_traffic(buf, &n);

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
					SSL_write(ssl, reply, strlen(reply));
				
					buf2  = (char *) calloc((8192+1), 1);
					n = 0;
					ptr = 0;
					while (ptr < 8192) {
						n = SSL_read(ssl, buf2 + ptr, 1);
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
				    "Pragma: no-cache\r\n\r\n";

				rfbLog("Handling Check HTTPS request via https GET. [%d]\n", getpid());
				rfbLog("-- %s\n", buf);

				SSL_write(ssl, reply, strlen(reply));
				SSL_shutdown(ssl);

				strcpy(tbuf, uniq);
				strcat(tbuf, cookie);
				write(vncsock, tbuf, strlen(tbuf));
				close(vncsock);

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
if (db) fprintf(stderr, "iface: %s\n", iface);
			usleep(150*1000);

			httpsock = rfbConnectToTcpAddr(iface, hport);

			if (httpsock < 0) {
				/* UGH, after all of that! */
				rfbLog("Could not connect to httpd socket!\n");
				exit(1);
			}
			if (db) fprintf(stderr, "ssl_helper[%d]: httpsock: %d %d\n",
			    getpid(), httpsock, n);

			/*
			 * send what we read to httpd, and then connect
			 * the rest of the SSL session to it:
			 */
			if (n > 0) {
				write(httpsock, buf, n);
			}
			ssl_xfer(httpsock, s_in, s_out, is_http);
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
		rfbLog("SSL: accept_openssl: connection from ssl_helper failed.\n");
		rfbLogPerror("accept");

		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: vsock: %d\n", vsock);

	n = read(vsock, rcookie, strlen(cookie));
	if (n != (int) strlen(cookie) || strncmp(cookie, rcookie, n)) {
		rfbLog("SSL: accept_openssl: cookie from ssl_helper failed. %d\n", n);
		if (errno != 0) {
			rfbLogPerror("read");
		}
		if (db) fprintf(stderr, "'%s'\n'%s'\n", cookie, rcookie);
		close(vsock);

		if (strstr(rcookie, uniq) == rcookie) {
			int i;
			rfbLog("SSL: but https for helper process succeeded.\n");
			if (mode != OPENSSL_HTTPS) {
				last_https = time(NULL);
				for (i=0; i<128; i++) {
					last_get[i] = '\0';
				}
				strncpy(last_get, rcookie, 100);
				if (db) fprintf(stderr, "last_get: '%s'\n", last_get);
			}
			ssl_helper_pid(pid, -2);

			if (mode == OPENSSL_INETD) {
				double start;
				/* to expand $PORT correctly in index.vnc */
				if (screen->port == 0) {
					int fd = fileno(stdin);
					if (getenv("X11VNC_INETD_PORT")) {
						screen->port = atoi(getenv(
						    "X11VNC_INETD_PORT"));
					} else {
						int tport = get_local_port(fd);
						if (tport > 0) {
							screen->port = tport;
						}
					}
				}
				rfbLog("SSL: screen->port %d\n", screen->port);

				/* kludge for https fetch via inetd */
				start = dnow();
				while (dnow() < start + 10.0) {
					rfbPE(10000);
					usleep(10000);
					waitpid(pid, &status, WNOHANG); 
					if (kill(pid, 0) != 0) {
						rfbPE(10000);
						rfbPE(10000);
						break;
					}
				}
				rfbLog("SSL: OPENSSL_INETD guessing "
				    "child https finished.\n");
				clean_up_exit(1);
			}
			return;
		}
		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (mode == OPENSSL_INETD || ssl_no_fail) {
			clean_up_exit(1);
		}
		return;
	}
	if (db) fprintf(stderr, "accept_openssl: cookie good: %s\n", cookie);

	rfbLog("SSL: handshake with helper process succeeded.\n");

	openssl_last_helper_pid = pid;
	ssl_helper_pid(pid, vsock);

	client = rfbNewClient(screen, vsock);
	openssl_last_helper_pid = 0;

	if (client) {
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

static void ssl_timeout (int sig) {
	rfbLog("sig: %d, ssl_init timed out.\n", sig);
	exit(1);
}

static int ssl_init(int s_in, int s_out) {
	unsigned char *sid = (unsigned char *) "x11vnc SID";
	char *name;
	int db = 0, rc, err;
	int ssock = s_in;
	double start = dnow();
	int timeout = 20;

	if (getenv("SSL_DEBUG")) {
		db = atoi(getenv("SSL_DEBUG"));
	}
	if (db) fprintf(stderr, "ssl_init: %d/%d\n", s_in, s_out);

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

	SSL_set_accept_state(ssl);

if (db > 1) fprintf(stderr, "ssl_init: 3\n");

	name = get_remote_host(ssock);

if (db > 1) fprintf(stderr, "ssl_init: 4\n");

	while (1) {
		if (db) fprintf(stderr, "calling SSL_accept...\n");

		signal(SIGALRM, ssl_timeout);
		alarm(timeout);

		rc = SSL_accept(ssl);
		err = SSL_get_error(ssl, rc);

		alarm(0);
		signal(SIGALRM, SIG_DFL);

		if (db) fprintf(stderr, "SSL_accept %d/%d\n", rc, err);
		if (err == SSL_ERROR_NONE) {
			break;
		} else if (err == SSL_ERROR_WANT_READ) {

			if (db) fprintf(stderr, "got SSL_ERROR_WANT_READ\n");
			rfbLog("SSL: ssl_helper: SSL_accept() failed for: %s\n",
			    name);
			return 0;
			
		} else if (err == SSL_ERROR_WANT_WRITE) {

			if (db) fprintf(stderr, "got SSL_ERROR_WANT_WRITE\n");
			rfbLog("SSL: ssl_helper: SSL_accept() failed for: %s\n",
			    name);
			return 0;

		} else if (err == SSL_ERROR_SYSCALL) {

			if (db) fprintf(stderr, "got SSL_ERROR_SYSCALL\n");
			rfbLog("SSL: ssl_helper: SSL_accept() failed for: %s\n",
			    name);
			return 0;

		} else if (err == SSL_ERROR_ZERO_RETURN) {

			if (db) fprintf(stderr, "got SSL_ERROR_ZERO_RETURN\n");
			rfbLog("SSL: ssl_helper: SSL_accept() failed for: %s\n",
			    name);
			return 0;

		} else if (rc < 0) {

			rfbLog("SSL: ssl_helper: SSL_accept() fatal: %d\n", rc);
			return 0;

		} else if (dnow() > start + 3.0) {

			rfbLog("SSL: ssl_helper: timeout looping SSL_accept() "
			    "fatal.\n");
			return 0;

		} else {
			BIO *bio = SSL_get_rbio(ssl);
			if (bio == NULL) {
				rfbLog("SSL: ssl_helper: ssl BIO is null. "
				    "fatal.\n");
				return 0;
			}
			if (BIO_eof(bio)) {
				rfbLog("SSL: ssl_helper: ssl BIO is EOF. "
				    "fatal.\n");
				return 0;
			}
		}
		usleep(10 * 1000);
	}

	rfbLog("SSL: ssl_helper[%d]: SSL_accept() succeeded for: %s\n", getpid(), name);
	free(name);

	return 1;
}

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
	time_t start;
	int tv_https_early = 60;
	int tv_https_later = 20;
	int tv_vnc_early = 25;
	int tv_vnc_later = 43200;	/* was 300, stunnel: 43200 */
	int tv_cutover = 70;
	int tv_closing = 60;
	int tv_use;

	if (dbxfer) {
		raw_xfer(csock, s_in, s_out);
		return;
	}
	if (getenv("SSL_DEBUG")) {
		db = atoi(getenv("SSL_DEBUG"));
	}

	if (db) fprintf(stderr, "ssl_xfer begin\n");

	start = time(NULL);
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

		if (tv_cutover && time(NULL) > start + tv_cutover) {
			rfbLog("SSL: ssl_xfer[%d]: tv_cutover: %d\n", getpid(),
			    tv_cutover);
			tv_cutover = 0;
			if (is_https) {
				tv_use = tv_https_later;
			} else {
				tv_use = tv_vnc_later;
			}
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

		if (nfd < 0) {
			rfbLog("SSL: ssl_xfer[%d]: select error: %d\n", getpid(), nfd);
			perror("select");
			/* connection finished */
			return;	
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
			rfbLog("SSL: ssl_xfer[%d]: connection timedout. %d\n",
			    getpid(), ndata);
			/* connection finished */
			return;
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
					return;
				}
				/* proceed */
			} else if (n == 0) {
				/* connection finished XXX double check */
				return;
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
						return;
					}
					/* proceed */
				} else if (err == SSL_ERROR_ZERO_RETURN) {
					/* S finished */
					s_rd = 0;
					s_wr = 0;
				} else if (err == SSL_ERROR_SSL) {
					/* connection finished */
					return;
				}
			}
		}

		if (c_rd && FD_ISSET(csock, &rd)) {


			/* try to read some data from C into our cbuf */

			n = read(csock, cbuf + cptr, ABSIZE - cptr);

			if (n < 0) {
				if (errno != EINTR) {
					/* connection finished */
					return;
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
							return;
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
					return;
				}
			}
		}
	}
}

void check_openssl(void) {
	fd_set fds;
	struct timeval tv;
	int nfds;
	static time_t last_waitall = 0;
	static double last_check = 0.0;
	double now;

	if (! use_openssl || openssl_sock < 0) {
		return;
	}

	now = dnow();
	if (now < last_check + 0.5) {
		return;
	}
	last_check = now;

	if (time(NULL) > last_waitall + 150) {
		last_waitall = time(NULL);
		ssl_helper_pid(0, -2);	/* waitall */
	}

	FD_ZERO(&fds);
	FD_SET(openssl_sock, &fds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(openssl_sock+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}
	
	rfbLog("SSL: accept_openssl(OPENSSL_VNC)\n");
	accept_openssl(OPENSSL_VNC);
}

void check_https(void) {
	fd_set fds;
	struct timeval tv;
	int nfds;
	static double last_check = 0.0;
	double now;

	if (! use_openssl || https_sock < 0) {
		return;
	}

	now = dnow();
	if (now < last_check + 0.5) {
		return;
	}
	last_check = now;

	FD_ZERO(&fds);
	FD_SET(https_sock, &fds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(https_sock+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}
	rfbLog("SSL: accept_openssl(OPENSSL_HTTPS)\n");
	accept_openssl(OPENSSL_HTTPS);
}

#define MSZ 4096
static void init_prng(void) {
	int db = 0, bytes;
	char file[MSZ];

	RAND_file_name(file, MSZ);

	rfbLog("RAND_file_name: %s\n", file);

	bytes = RAND_load_file(file, -1);
	if (db) fprintf(stderr, "bytes read: %d\n", bytes);
	
	bytes += RAND_load_file("/dev/urandom", 64);
	if (db) fprintf(stderr, "bytes read: %d\n", bytes);
	
	if (bytes > 0) {
		if (! quiet) {
			rfbLog("initialized PRNG with %d random bytes.\n",
			    bytes);
		}
		return;
	}

	bytes += RAND_load_file("/dev/random", 8);
	if (db) fprintf(stderr, "bytes read: %d\n", bytes);
	if (! quiet) {
		rfbLog("initialized PRNG with %d random bytes.\n", bytes);
	}
}
#endif	/* FORK_OK */
#endif	/* LIBVNCSERVER_HAVE_LIBSSL */

void raw_xfer(int csock, int s_in, int s_out) {
	char buf[8192];
	int sz = 8192, n, m, status, db = 1;
#ifdef FORK_OK
	pid_t pid = fork();

	/* this is for testing, no SSL just socket redir */
	if (pid < 0) {
		exit(1);
	}
	if (pid) {
		if (db) fprintf(stderr, "raw_xfer start: %d -> %d/%d\n", csock, s_in, s_out);

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
if (db) fprintf(stderr, "raw_xfer bad write:  %d -> %d | %d/%d  errno=%d\n", csock, s_out, m, n, errno);
					break;
				}
			}
		}
		kill(pid, SIGTERM);
		waitpid(pid, &status, WNOHANG); 
		if (db) fprintf(stderr, "raw_xfer done:  %d -> %d\n", csock, s_out);

	} else {
		if (db) fprintf(stderr, "raw_xfer start: %d <- %d\n", csock, s_in);

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
		if (db) fprintf(stderr, "raw_xfer bad write:  %d <- %d | %d/%d errno=%d\n", csock, s_in, m, n, errno);
					break;
				}
			}
		}
		if (db) fprintf(stderr, "raw_xfer done:  %d <- %d\n", csock, s_in);

	}
	close(csock);
	close(s_in);
	close(s_out);
#endif	/* FORK_OK */
}

