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

/* -- sslcmds.c -- */

#include "x11vnc.h"
#include "inet.h"
#include "cleanup.h"
#include "sslhelper.h"
#include "ssltools.h"
#include "connections.h"

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H
#if LIBVNCSERVER_HAVE_WAITPID
#define SSLCMDS
#endif
#endif
#endif


void check_stunnel(void);
int start_stunnel(int stunnel_port, int x11vnc_port, int hport, int x11vnc_hport);
void stop_stunnel(void);
void setup_stunnel(int rport, int *argc, char **argv);
char *get_Cert_dir(char *cdir_in, char **tmp_in);
void sslScripts(void);
void sslGenCA(char *cdir);
void sslGenCert(char *ty, char *nm);
void sslEncKey(char *path, int info_only);

static pid_t stunnel_pid = 0;

void check_stunnel(void) {
	static time_t last_check = 0;
	time_t now = time(NULL);

	if (last_check + 3 >= now) {
		return;
	}
	last_check = now;

	/* double check that stunnel is still running: */

	if (stunnel_pid > 0) {
		int status;
#ifdef SSLCMDS
		waitpid(stunnel_pid, &status, WNOHANG); 
#endif
		if (kill(stunnel_pid, 0) != 0) {
#ifdef SSLCMDS
			waitpid(stunnel_pid, &status, WNOHANG); 
#endif
			rfbLog("stunnel subprocess %d died.\n", stunnel_pid); 
			stunnel_pid = 0;
			clean_up_exit(1);
		}
	}
}

int start_stunnel(int stunnel_port, int x11vnc_port, int hport, int x11vnc_hport) {
#ifdef SSLCMDS
	char extra[] = ":/usr/sbin:/usr/local/sbin:/dist/sbin";
	char *path, *p, *exe;
	char *stunnel_path = NULL;
	struct stat verify_buf;
	struct stat crl_buf;
	int status, tmp_pem = 0;

	if (stunnel_pid) {
		stop_stunnel();
	}
	stunnel_pid = 0;

	path = getenv("PATH");
	if (! path) {
		path = strdup(extra+1);
	} else {
		char *pt = path;
		path = (char *) malloc(strlen(path)+strlen(extra)+1);
		if (! path) {
			return 0;
		}
		strcpy(path, pt);
		strcat(path, extra);
	}

	exe = (char *) malloc(strlen(path) + 1 + strlen("stunnel4") + 1);

	p = strtok(path, ":");

	exe[0] = '\0';

	while (p) {
		struct stat sbuf;

		sprintf(exe, "%s/%s", p, "stunnel4");
		if (! stunnel_path && stat(exe, &sbuf) == 0) {
			if (! S_ISDIR(sbuf.st_mode)) {
				stunnel_path = exe;
				break;
			}
		}

		sprintf(exe, "%s/%s", p, "stunnel");
		if (! stunnel_path && stat(exe, &sbuf) == 0) {
			if (! S_ISDIR(sbuf.st_mode)) {
				stunnel_path = exe;
				break;
			}
		}

		p = strtok(NULL, ":");
	}
	if (path) {
		free(path);
	}

	if (getenv("STUNNEL_PROG")) {
		free(exe);
		exe = strdup(getenv("STUNNEL_PROG"));
		stunnel_path = exe;
	}

	if (! stunnel_path) {
		free(exe);
		return 0;
	}
	if (stunnel_path[0] == '\0') {
		free(exe);
		return 0;
	}

	/* stunnel */
	if (no_external_cmds || !cmd_ok("stunnel")) {
		rfbLogEnable(1);
		rfbLog("start_stunnel: cannot run external commands in -nocmds mode:\n");
		rfbLog("   \"%s\"\n", stunnel_path);
		rfbLog("   exiting.\n");
		clean_up_exit(1);
	}

	if (! quiet) {
		rfbLog("\n");
		rfbLog("starting ssl tunnel: %s  %d -> %d\n", stunnel_path,
		    stunnel_port, x11vnc_port);
	}

	if (stunnel_pem && strstr(stunnel_pem, "SAVE") == stunnel_pem) {
		stunnel_pem = get_saved_pem(stunnel_pem, 1);
		if (! stunnel_pem) {
			rfbLog("start_stunnel: could not create or open"
			    " saved PEM.\n");	
			clean_up_exit(1);
		}
	} else if (!stunnel_pem) {
		stunnel_pem = create_tmp_pem(NULL, 0);
		if (! stunnel_pem) {
			rfbLog("start_stunnel: could not create temporary,"
			    " self-signed PEM.\n");	
			clean_up_exit(1);
		}
		tmp_pem = 1;
		if (getenv("X11VNC_SHOW_TMP_PEM")) {
			FILE *in = fopen(stunnel_pem, "r");
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
	}

	if (ssl_verify) {
		char *file = get_ssl_verify_file(ssl_verify);
		if (file) {
			ssl_verify = file;
		}
		if (stat(ssl_verify, &verify_buf) != 0) {
			rfbLog("stunnel: %s does not exist.\n", ssl_verify);
			clean_up_exit(1);
		}
	}
	if (ssl_crl) {
		if (stat(ssl_crl, &crl_buf) != 0) {
			rfbLog("stunnel: %s does not exist.\n", ssl_crl);
			clean_up_exit(1);
		}
	}

	stunnel_pid = fork();

	if (stunnel_pid < 0) {
		stunnel_pid = 0;
		free(exe);
		return 0;
	}

	if (stunnel_pid == 0) {
		FILE *in;
		char fd[20];
		int i;
		char *st_if = getenv("STUNNEL_LISTEN");

		if (st_if == NULL) {
			st_if = "";
		} else {
			st_if = (char *) malloc(strlen(st_if) + 2);
			sprintf(st_if, "%s:", getenv("STUNNEL_LISTEN"));
		}


		for (i=3; i<256; i++) {
			close(i);
		}

		if (use_stunnel == 3) {
			char sp[30], xp[30], *a = NULL;
			char *st = stunnel_path;
			char *pm = stunnel_pem;
			char *sv = ssl_verify;

			sprintf(sp, "%d", stunnel_port);
			sprintf(xp, "%d", x11vnc_port);

			if (ssl_verify) {
				if(S_ISDIR(verify_buf.st_mode)) {
					a = "-a";
				} else {
					a = "-A";
				}
			}

			if (ssl_crl) {
				rfbLog("stunnel: stunnel3 does not support CRL. %s\n", ssl_crl);
				clean_up_exit(1);
			}
			
			if (stunnel_pem && ssl_verify) {
				/* XXX double check -v 2 */
				execlp(st, st, "-f", "-d", sp, "-r", xp, "-P",
				    "none", "-p", pm, a, sv, "-v", "2",
				    (char *) NULL);
			} else if (stunnel_pem && !ssl_verify) {
				execlp(st, st, "-f", "-d", sp, "-r", xp, "-P",
				    "none", "-p", pm,
				    (char *) NULL);
			} else if (!stunnel_pem && ssl_verify) {
				execlp(st, st, "-f", "-d", sp, "-r", xp, "-P",
				    "none", a, sv, "-v", "2",
				    (char *) NULL);
			} else {
				execlp(st, st, "-f", "-d", sp, "-r", xp, "-P",
				    "none", (char *) NULL);
			}
			exit(1);
		}

		in = tmpfile();
		if (! in) {
			exit(1);
		}

		fprintf(in, "foreground = yes\n");
		fprintf(in, "pid =\n");
		if (stunnel_pem) {
			fprintf(in, "cert = %s\n", stunnel_pem);
		}
		if (ssl_crl) {
			if(S_ISDIR(crl_buf.st_mode)) {
				fprintf(in, "CRLpath = %s\n", ssl_crl);
			} else {
				fprintf(in, "CRLfile = %s\n", ssl_crl);
			}
		}
		if (ssl_verify) {
			if(S_ISDIR(verify_buf.st_mode)) {
				fprintf(in, "CApath = %s\n", ssl_verify);
			} else {
				fprintf(in, "CAfile = %s\n", ssl_verify);
			}
			fprintf(in, "verify = 2\n");
		}
		fprintf(in, ";debug = 7\n\n");
		fprintf(in, "[x11vnc_stunnel]\n");
		fprintf(in, "accept = %s%d\n", st_if, stunnel_port);
		fprintf(in, "connect = %d\n", x11vnc_port);

		if (hport > 0 && x11vnc_hport > 0) {
			fprintf(in, "\n[x11vnc_http]\n");
			fprintf(in, "accept = %s%d\n", st_if, hport);
			fprintf(in, "connect = %d\n", x11vnc_hport);
		}

		fflush(in);
		rewind(in);

		if (getenv("STUNNEL_DEBUG")) {
			char line[1000];
			fprintf(stderr, "\nstunnel config contents:\n\n");
			while (fgets(line, sizeof(line), in) != NULL) {
				fprintf(stderr, "%s", line);
			}
			fprintf(stderr, "\n");
			rewind(in);
		}
		
		sprintf(fd, "%d", fileno(in));
		execlp(stunnel_path, stunnel_path, "-fd", fd, (char *) NULL);
		exit(1);
	}

	free(exe);
	usleep(750 * 1000);

	waitpid(stunnel_pid, &status, WNOHANG); 

	if (ssl_verify && strstr(ssl_verify, "/sslverify-tmp-load-")) {
		/* temporary file */
		usleep(1000 * 1000);
		unlink(ssl_verify);
	}
	if (tmp_pem) {
		/* temporary cert */
		usleep(1500 * 1000);
		unlink(stunnel_pem);
	}

	if (kill(stunnel_pid, 0) != 0) {
		waitpid(stunnel_pid, &status, WNOHANG); 
		stunnel_pid = 0;
		return 0;
	}

	if (! quiet) {
		rfbLog("stunnel pid is: %d\n", (int) stunnel_pid);
	}

	return 1;
#else
	return 0;
#endif
}

void stop_stunnel(void) {
	int status;
	if (! stunnel_pid) {
		return;
	}
#ifdef SSLCMDS
	kill(stunnel_pid, SIGTERM);
	usleep (150 * 1000);
	kill(stunnel_pid, SIGKILL);
	usleep (50 * 1000);
	waitpid(stunnel_pid, &status, WNOHANG); 
#endif
	stunnel_pid = 0;
}

void setup_stunnel(int rport, int *argc, char **argv) {
	int i, xport = 0, hport = 0, xhport = 0;

	if (! rport && argc && argv) {
		for (i=0; i< *argc; i++) {
			if (argv[i] && !strcmp(argv[i], "-rfbport")) {
				if (i < *argc - 1) {
					rport = atoi(argv[i+1]);
				}
			}
		}
	}

	if (! rport) {
		/* we do our own autoprobing then... */
		rport = find_free_port(5900, 5999);
		if (! rport) {
			goto stunnel_fail;
		}
	}

	xport = find_free_port(5950, 5999);
	if (! xport) {
		goto stunnel_fail; 
	}

	if (https_port_num > 0) {
		hport = https_port_num;
	}

	if (! hport && argc && argv) {
		for (i=0; i< *argc; i++) {
			if (argv[i] && !strcmp(argv[i], "-httpport")) {
				if (i < *argc - 1) {
					hport = atoi(argv[i+1]);
				}
			}
		}
	}

	if (! hport && http_try_it) {
		hport = find_free_port(rport-100, rport-1);
		if (! hport) {
			goto stunnel_fail;
		}
	}
	if (hport) {
		xhport = find_free_port(5850, 5899);
		if (! xhport) {
			goto stunnel_fail; 
		}
		stunnel_http_port = hport;
	}
	

	if (start_stunnel(rport, xport, hport, xhport)) {
		int tweaked = 0;
		char tmp[30];
		sprintf(tmp, "%d", xport);
		if (argc && argv) {
			for (i=0; i < *argc; i++) {
				if (argv[i] && !strcmp(argv[i], "-rfbport")) {
					if (i < *argc - 1) {
						/* replace orig value */
						argv[i+i] = strdup(tmp); 
						tweaked = 1;
						break;
					}
				}
			}
			if (! tweaked) {
				i = *argc;
				argv[i] = strdup("-rfbport");
				argv[i+1] = strdup(tmp);
				*argc += 2;
				got_rfbport = 1;
				got_rfbport_val = atoi(tmp);
			}
		}
		stunnel_port = rport;
		ssl_initialized = 1;
		return;
	}

	stunnel_fail:
	rfbLog("failed to start stunnel.\n");
	clean_up_exit(1);
}

char *get_Cert_dir(char *cdir_in, char **tmp_in) {
	char *cdir, *home, *tmp;
	struct stat sbuf;
	int i;
	char *cases1[] = {"/.vnc", "/.vnc/certs", "/.vnc/certs/CA"};
	char *cases2[] = {"", "/CA", "/tmp"};

	if (cdir_in != NULL) {
		cdir = cdir_in;
	} else {
		cdir = ssl_certs_dir;
	}

	if (cdir == NULL) {
		home = get_home_dir();
		if (! home) {
			return NULL;
		}
		cdir = (char *) malloc(strlen(home) + strlen("/.vnc/certs/CA") + 1);
		for (i=0; i<3; i++) {
			sprintf(cdir, "%s%s", home, cases1[i]);
			if (stat(cdir, &sbuf) != 0) {
				rfbLog("creating dir: %s\n", cdir);
				if (mkdir(cdir, 0755) != 0) {
					rfbLog("could not create directory %s\n", cdir);
					rfbLogPerror("mkdir");
					return NULL;
				}
			} else if (! S_ISDIR(sbuf.st_mode)) {
				rfbLog("not a directory: %s\n", cdir);
				return NULL;
			}
		}
		sprintf(cdir, "%s%s", home, cases1[1]);
	}

	tmp = (char *) malloc(strlen(cdir) + strlen("/tmp") + 1);
	for (i=0; i<3; i++) {
		int ret;
		sprintf(tmp, "%s%s", cdir, cases2[i]);
		if (stat(tmp, &sbuf) != 0) {
			rfbLog("creating dir: %s\n", tmp);
			if (! strcmp(cases2[i], "/tmp")) {
				ret = mkdir(tmp, 0700);
			} else {
				ret = mkdir(tmp, 0755);
			}
				
			if (ret != 0) {
				rfbLog("could not create directory %s\n", tmp);
				rfbLogPerror("mkdir");
				return NULL;
			}
		} else if (! S_ISDIR(sbuf.st_mode)) {
			rfbLog("not a directory: %s\n", tmp);
			return NULL;
		}
	}
	sprintf(tmp, "%s/tmp", cdir);
	*tmp_in = tmp;
	return cdir;
}

static char *getsslscript(char *cdir, char *name, char *script) {
	char *openssl = find_openssl_bin();
	char *tmp, *scr, *cdir_use;
	FILE *out;

	if (! openssl || openssl[0] == '\0') {
		exit(1);
	}

	if (!name || !script) {
		exit(1);
	}

	cdir_use = get_Cert_dir(cdir, &tmp);
	if (!cdir_use || !tmp) {
		exit(1);
	}

	scr = (char *) malloc(strlen(tmp) + 1 + strlen(name) + 30);

	sprintf(scr, "%s/%s.%d.sh", tmp, name, getpid());
	out = fopen(scr, "w");
	if (! out) {
		rfbLog("could not open: %s\n", scr);
		rfbLogPerror("fopen");
		exit(1);
	}
	fprintf(out, "%s", script);
	fclose(out);

	rfbLog("Using openssl:   %s\n", openssl);
	rfbLog("Using certs dir: %s\n", cdir_use);
	fprintf(stderr, "\n");

	set_env("BASE_DIR", cdir_use);
	set_env("OPENSSL", openssl);

	return scr;
}

void sslScripts(void) {
	fprintf(stdout, "======================================================\n");
	fprintf(stdout, "genCA script for '-sslGenCA':\n\n");
	fprintf(stdout, "%s\n", genCA);
	fprintf(stdout, "======================================================\n");
	fprintf(stdout, "genCert script for '-sslGenCert', etc.:\n\n");
	fprintf(stdout, "%s\n", genCert);
}

void sslGenCA(char *cdir) {
	char *cmd, *scr = getsslscript(cdir, "genca", genCA);

	if (! scr) {
		exit(1);
	}

	cmd = (char *)malloc(strlen("/bin/sh ") + strlen(scr) + 1);
	sprintf(cmd, "/bin/sh %s", scr);

	system(cmd);
	unlink(scr);

	free(cmd);
	free(scr);
}

void sslGenCert(char *ty, char *nm) {
	char *cmd, *scr = getsslscript(NULL, "gencert", genCert);

	if (! scr) {
		exit(1);
	}

	cmd = (char *)malloc(strlen("/bin/sh ") + strlen(scr) + 1);
	sprintf(cmd, "/bin/sh %s", scr);

	if (! ty) {
		set_env("TYPE", "");
	} else {
		set_env("TYPE", ty);
	}
	if (! nm) {
		set_env("NAME", "");
	} else {
		char *q = strstr(nm, "SAVE-");
		if (!strcmp(nm, "SAVE")) {
			set_env("NAME", "");
		} else if (q == nm) {
			q += strlen("SAVE-");
			set_env("NAME", q);
		} else {
			set_env("NAME", nm);
		}
	}

	system(cmd);
	unlink(scr);

	free(cmd);
	free(scr);
}

void sslEncKey(char *path, int mode) {
	char *openssl = find_openssl_bin();
	char *scr, *cert = NULL, *tca, *cdir = NULL;
	char line[1024], tmp[] = "/tmp/x11vnc-tmp.XXXXXX";
	int tmp_fd, incert, info_only = 0, delete_only = 0, listlong = 0;
	struct stat sbuf;
	FILE *file;
	static int depth = 0;

	if (depth > 0) {
		/* get_saved_pem may call us back. */
		return;
	}

	if (! path) {
		return;
	}

	depth++;

	if (mode == 1) {
		info_only = 1;
	} else if (mode == 2) {
		delete_only = 1;
	}

	if (! openssl) {
		exit(1);
	}

	cdir = get_Cert_dir(NULL, &tca);
	if (! cdir || ! tca) {
		fprintf(stderr, "could not find Cert dir\n");
		exit(1);
	}

	if (!strcasecmp(path, "LL") || !strcasecmp(path, "LISTL")) {
		listlong = 1;
		path = "LIST";
	}

	if (strstr(path, "SAVE") == path) {
		char *p = get_saved_pem(path, 0);
		if (p == NULL) {
			fprintf(stderr, "could not find saved pem "
			    "matching: %s\n", path);
			exit(1);
		}
		path = p;

	} else if (!strcmp(path, "CA")) {
		tca = (char *) malloc(strlen(cdir)+strlen("/CA/cacert.pem")+1);
		sprintf(tca, "%s/CA/cacert.pem", cdir);
		path = tca;

	} else if (info_only && (!strcasecmp(path, "LIST") ||
	    !strcasecmp(path, "LS") || !strcasecmp(path, "ALL"))) {

		if (! program_name || strchr(program_name, ' ')) {
			fprintf(stderr, "bad program name.\n");
			exit(1);
		}
		if (strchr(cdir, '\'')) {
			fprintf(stderr, "bad certdir char: %s\n", cdir);
			exit(1);
		}

		tca = (char *) malloc(2*strlen(cdir)+strlen(program_name)+1000);

		sprintf(tca, "find '%s' | egrep '/(CA|tmp|clients)$|"
		    "\\.(crt|pem|key|req)$' | grep -v CA/newcerts", cdir);

		if (!strcasecmp(path, "ALL")) {
			/* ugh.. */
			strcat(tca, " | egrep -v 'private/cakey.pem|"
			    "(CA|tmp|clients)$' | xargs -n1 ");
			strcat(tca, program_name);
			strcat(tca, " -ssldir '");
			strcat(tca, cdir);
			strcat(tca, "' -sslCertInfo 2>&1 ");
		} else if (listlong) {
			strcat(tca, " | xargs ls -ld ");
		}
		system(tca);
		free(tca);

		depth--;
		return;

	} else if (info_only && (!strcasecmp(path, "HASHON")
	    || !strcasecmp(path, "HASHOFF"))) {

		tmp_fd = mkstemp(tmp);
		if (tmp_fd < 0) {
			exit(1);
		}

		write(tmp_fd, genCert, strlen(genCert));
		close(tmp_fd);

		scr = (char *) malloc(strlen("/bin/sh ") + strlen(tmp) + 1);
		sprintf(scr, "/bin/sh %s", tmp);

		set_env("BASE_DIR", cdir);
		set_env("OPENSSL", openssl);
		set_env("TYPE", "server");
		if (!strcasecmp(path, "HASHON")) {
			set_env("HASHON", "1");
		} else {
			set_env("HASHOFF", "1");
		}
		system(scr);
		unlink(tmp);
		free(scr);

		depth--;
		return;
	}


	if (stat(path, &sbuf) != 0) {
	    if (strstr(path, "client") || strchr(path, '/') == NULL) {
		int i;
		tca = (char *) malloc(strlen(cdir) + strlen(path) + 100);
		for (i = 1; i <= 15; i++)  {
			tca[0] = '\0';
			if (       i == 1) {
			    sprintf(tca, "%s/%s", cdir, path); 
			} else if (i == 2 && mode > 0) {
			    sprintf(tca, "%s/%s.crt", cdir, path); 
			} else if (i == 3) {
			    sprintf(tca, "%s/%s.pem", cdir, path); 
			} else if (i == 4 && mode > 1) {
			    sprintf(tca, "%s/%s.req", cdir, path); 
			} else if (i == 5 && mode > 1) {
			    sprintf(tca, "%s/%s.key", cdir, path); 
			} else if (i == 6) {
			    sprintf(tca, "%s/clients/%s", cdir, path); 
			} else if (i == 7 && mode > 0) {
			    sprintf(tca, "%s/clients/%s.crt", cdir, path); 
			} else if (i == 8) {
			    sprintf(tca, "%s/clients/%s.pem", cdir, path); 
			} else if (i == 9 && mode > 1) {
			    sprintf(tca, "%s/clients/%s.req", cdir, path); 
			} else if (i == 10 && mode > 1) {
			    sprintf(tca, "%s/clients/%s.key", cdir, path); 
			} else if (i == 11) {
			    sprintf(tca, "%s/server-%s", cdir, path); 
			} else if (i == 12 && mode > 0) {
			    sprintf(tca, "%s/server-%s.crt", cdir, path); 
			} else if (i == 13) {
			    sprintf(tca, "%s/server-%s.pem", cdir, path); 
			} else if (i == 14 && mode > 1) {
			    sprintf(tca, "%s/server-%s.req", cdir, path); 
			} else if (i == 15 && mode > 1) {
			    sprintf(tca, "%s/server-%s.key", cdir, path); 
			}
			if (tca[0] == '\0') {
				continue;
			}
			if (stat(tca, &sbuf) == 0) {
				path = tca;
				break;
			}
		}
	    }
	}

	if (stat(path, &sbuf) != 0) {
		rfbLog("sslEncKey: %s\n", path);
		rfbLogPerror("stat");
		exit(1);
	}

	if (! info_only) {
		cert = (char *) malloc(2*(sbuf.st_size + 1024));
		file = fopen(path, "r");
		if (file == NULL) {
			rfbLog("sslEncKey: %s\n", path);
			rfbLogPerror("fopen");
			exit(1);
		}
		incert = 0;
		cert[0] = '\0';
		while (fgets(line, 1024, file) != NULL) {
			if (strstr(line, "-----BEGIN CERTIFICATE-----")
			    == line) {
				incert = 1;
			}
			if (incert) {
				if (strlen(cert)+strlen(line) <
				    2 * (size_t) sbuf.st_size) {
					strcat(cert, line);
				}
			}
			if (strstr(line, "-----END CERTIFICATE-----")
			    == line) {
				incert = 0;
			}
		}
		fclose(file);
	}

	tmp_fd = mkstemp(tmp);
	if (tmp_fd < 0) {
		exit(1);
	}

	write(tmp_fd, genCert, strlen(genCert));
	close(tmp_fd);

        scr = (char *) malloc(strlen("/bin/sh ") + strlen(tmp) + 1);
	sprintf(scr, "/bin/sh %s", tmp);

	set_env("BASE_DIR", "/no/such/dir");
	set_env("OPENSSL", openssl);
	set_env("TYPE", "server");
	if (info_only) {
		set_env("INFO_ONLY", path);
	} else if (delete_only) {
		set_env("DELETE_ONLY", path);
	} else {
		set_env("ENCRYPT_ONLY", path);
	}
	system(scr);
	unlink(tmp);

	if (! mode && cert && cert[0] != '\0') {
		int got_cert = 0;
		file = fopen(path, "r");
		if (file == NULL) {
			rfbLog("sslEncKey: %s\n", path);
			rfbLogPerror("fopen");
			exit(1);
		}
		while (fgets(line, 1024, file) != NULL) {
			if (strstr(line, "-----BEGIN CERTIFICATE-----")
			    == line) {
				got_cert++;
			}
			if (strstr(line, "-----END CERTIFICATE-----")
			    == line) {
				got_cert++;
			}
		}
		fclose(file);
		if (got_cert < 2) {
			file = fopen(path, "a");
			if (file == NULL) {
				rfbLog("sslEncKey: %s\n", path);
				rfbLogPerror("fopen");
				exit(1);
			}
			fprintf(file, "%s", cert);
			fclose(file);
		}
		free(cert);
	}

	depth--;
}

