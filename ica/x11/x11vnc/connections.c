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

/* -- connections.c -- */

#include "x11vnc.h"
#include "inet.h"
#include "remote.h"
#include "keyboard.h"
#include "cleanup.h"
#include "gui.h"
#include "solid.h"
#include "rates.h"
#include "screen.h"
#include "unixpw.h"
#include "user.h"
#include "scan.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "xwrappers.h"
#include "xevents.h"
#include "win_utils.h"
#include "macosx.h"
#include "macosxCG.h"
#include "userinput.h"
#include "pointer.h"
#include "xrandr.h"

/*
 * routines for handling incoming, outgoing, etc connections
 */

/* string for the VNC_CONNECT property */
char vnc_connect_str[VNC_CONNECT_MAX+1];
Atom vnc_connect_prop = None;
char x11vnc_remote_str[X11VNC_REMOTE_MAX+1];
Atom x11vnc_remote_prop = None;
rfbClientPtr inetd_client = NULL;

int all_clients_initialized(void);
char *list_clients(void);
int new_fb_size_clients(rfbScreenInfoPtr s);
void close_all_clients(void);
void close_clients(char *str);
void set_client_input(char *str);
void set_child_info(void);
int cmd_ok(char *cmd);
void client_gone(rfbClientPtr client);
void client_gone_chat_helper(rfbClientPtr client);
void reverse_connect(char *str);
void set_vnc_connect_prop(char *str);
void read_vnc_connect_prop(int);
void set_x11vnc_remote_prop(char *str);
void read_x11vnc_remote_prop(int);
void check_connect_inputs(void);
void check_gui_inputs(void);
rfbClientPtr create_new_client(int sock, int start_thread);
enum rfbNewClientAction new_client(rfbClientPtr client);
enum rfbNewClientAction new_client_chat_helper(rfbClientPtr client);
rfbBool password_check_chat_helper(rfbClientPtr cl, const char* response, int len);
void start_client_info_sock(char *host_port_cookie);
void send_client_info(char *str);
void adjust_grabs(int grab, int quiet);
void check_new_clients(void);
int accept_client(rfbClientPtr client);
void check_ipv6_listen(long usec);
void check_unix_sock(long usec);
int run_user_command(char *cmd, rfbClientPtr client, char *mode, char *input,
    int len, FILE *output);
int check_access(char *addr);
void client_set_net(rfbClientPtr client);
char *get_xprop(char *prop, Window win);
int set_xprop(char *prop, Window win, char *value);
char *bcx_xattach(char *str, int *pg_init, int *kg_init);
void grab_state(int *ptr_grabbed, int *kbd_grabbed);
char *wininfo(Window win, int show_children);

static rfbClientPtr *client_match(char *str);
static void free_client_data(rfbClientPtr client);
static void ugly_geom(char *p, int *x, int *y);
static int ugly_window(char *addr, char *userhost, int X, int Y,
    int timeout, char *mode, int accept);
static int action_match(char *action, int rc);
static void check_connect_file(char *file);
static void send_client_connect(void);


/*
 * check that all clients are in RFB_NORMAL state
 */
int all_clients_initialized(void) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int ok = 1;

	if (! screen) {
		return ok;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (cl->state != RFB_NORMAL) {
			ok = 0;
		} else {
			client_normal_count++;
		}
	}
	rfbReleaseClientIterator(iter);

	return ok;
}

char *list_clients(void) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	char *list, tmp[256];
	int count = 0;

	if (!screen) {
		return strdup("");
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		client_set_net(cl);
		count++;
	}
	rfbReleaseClientIterator(iter);

	/*
	 * each client:
         * <id>:<ip>:<port>:<user>:<unix>:<hostname>:<input>:<loginview>:<time>,
	 * 8+1+64+1+5+1+24+1+24+1+256+1+5+1+1+1+10+1
	 * 123.123.123.123:60000/0x11111111-rw,
	 * so count+1 * 1000 must cover it.
	 */
	list = (char *) malloc((count+1)*1000);
	
	list[0] = '\0';

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;
		char *tmp_host, *p;

		if (! cd) {
			continue;
		}
		if (*list != '\0') {
			strcat(list, ",");
		}
		sprintf(tmp, "0x%x:", cd->uid);
		strcat(list, tmp);
		p = tmp_host = strdup(cl->host);
		while (*p) {
			if (*p == ':') *p = '#';
			p++;
		}
		strcat(list, tmp_host);
		free(tmp_host);
		strcat(list, ":");
		sprintf(tmp, "%d:", cd->client_port);
		strcat(list, tmp);
		if (cd->username[0] == '\0') {
			char *s = ident_username(cl);
			if (s) free(s);
		}
		if (strstr(cd->username, "UNIX:") == cd->username) {
			strcat(list, cd->username + strlen("UNIX:"));
		} else {
			strcat(list, cd->username);
		}
		strcat(list, ":");
		if (cd->unixname[0] == '\0') {
			strcat(list, "none");
		} else {
			strcat(list, cd->unixname);
		}
		strcat(list, ":");
		p = tmp_host = strdup(cd->hostname);
		while (*p) {
			if (*p == ':') *p = '#';
			p++;
		}
		strcat(list, tmp_host);
		free(tmp_host);
		strcat(list, ":");
		strcat(list, cd->input);
		strcat(list, ":");
		sprintf(tmp, "%d", cd->login_viewonly);
		strcat(list, tmp);
		strcat(list, ":");
		sprintf(tmp, "%d", (int) cd->login_time);
		strcat(list, tmp);
	}
	rfbReleaseClientIterator(iter);
	return list;
}

/* count number of clients supporting NewFBSize */
int new_fb_size_clients(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int count = 0;

	if (! s) {
		return 0;
	}

	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (cl->useNewFBSize) {
			count++;
		}
	}
	rfbReleaseClientIterator(iter);
	return count;
}

void close_all_clients(void) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (! screen) {
		return;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		rfbCloseClient(cl);
		rfbClientConnectionGone(cl);
	}
	rfbReleaseClientIterator(iter);
}

static rfbClientPtr *client_match(char *str) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl, *cl_list;
	int i, n, host_warn = 0, hex_warn = 0;

	n = client_count + 10;
	cl_list = (rfbClientPtr *) malloc(n * sizeof(rfbClientPtr));
	
	i = 0;
	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;
		if (strstr(str, "0x") == str) {
			unsigned int in;
			int id;
			if (! cd) {
				continue;
			}
			if (sscanf(str, "0x%x", &in) != 1) {
				if (hex_warn++) {
					continue;
				}
				rfbLog("skipping invalid client hex id: %s\n",
				    str);
				continue;
			}
			id = (unsigned int) in;
			if (cd->uid == id) {
				cl_list[i++] = cl;
			}
		} else {
			int port = -1;
			char *rstr = strdup(str);
			char *q = strrchr(rstr, ':');
			if (q) {
				port = atoi(q+1);
				*q = '\0';
				if (port == 0 && q[1] != '0') {
					port = -1;
				} else if (port < 0) {
					port = -port;
				} else if (port < 200) {
					port = 5500 + port;
				}
			}
			if (ipv6_ip(str)) {
				;
			} else if (! dotted_ip(str, 0)) {
				char *orig = rstr;
				rstr = host2ip(rstr);
				free(orig);
				if (rstr == NULL || *rstr == '\0') {
					if (host_warn++) {
						continue;
					}
					rfbLog("skipping bad lookup: \"%s\"\n", str);
					continue;
				}
				rfbLog("lookup: %s -> %s port=%d\n", str, rstr, port);
			}
			if (!strcmp(rstr, cl->host)) {
				int ok = 1;
				if (port > 0) {
					if (cd != NULL && cd->client_port > 0) {
						if (cd->client_port != port) {
							ok = 0;
						}
					} else {
						int cport = get_remote_port(cl->sock);
						if (cport != port) {
							ok = 0;
						}
					}
				}
				if (ok) {
					cl_list[i++] = cl;
				}
			}
			free(rstr);
		}
		if (i >= n - 1) {
			break;
		}
	}
	rfbReleaseClientIterator(iter);

	cl_list[i] = NULL;

	return cl_list;
}

void close_clients(char *str) {
	rfbClientPtr *cl_list, *cp;

	if (!strcmp(str, "all") || !strcmp(str, "*")) {
		close_all_clients();
		return;
	}

	if (! screen) {
		return;
	}
	
	cl_list = client_match(str);

	cp = cl_list;
	while (*cp) {
		rfbCloseClient(*cp);
		rfbClientConnectionGone(*cp);
		cp++;
	}
	free(cl_list);
}

void set_client_input(char *str) {
	rfbClientPtr *cl_list, *cp;
	char *p, *val;

	/* str is "match:value" */

	if (! screen) {
		return;
	}

	p = strrchr(str, ':');
	if (! p) {
		return;
	}
	*p = '\0';
	p++;
	val = short_kmbcf(p);
	
	cl_list = client_match(str);

	cp = cl_list;
	while (*cp) {
		ClientData *cd = (ClientData *) (*cp)->clientData;
		if (! cd) {
			continue;
		}
		cd->input[0] = '\0';
		strcat(cd->input, "_");
		strcat(cd->input, val);
		cp++;
	}

	free(val);
	free(cl_list);
}

void set_child_info(void) {
	char pid[16];
	/* set up useful environment for child process */
	sprintf(pid, "%d", (int) getpid());
	set_env("X11VNC_PID", pid);
	if (program_name) {
		/* e.g. for remote control -R */
		set_env("X11VNC_PROG", program_name);
	}
	if (program_cmdline) {
		set_env("X11VNC_CMDLINE", program_cmdline);
	}
	if (raw_fb_str) {
		set_env("X11VNC_RAWFB_STR", raw_fb_str);
	} else {
		set_env("X11VNC_RAWFB_STR", "");
	}
}

int cmd_ok(char *cmd) {
	char *p, *str;
	if (no_external_cmds) {
		return 0;
	}
	if (! cmd || cmd[0] == '\0') {
		return 0;
	}
	if (! allowed_external_cmds) {
		/* default, allow any (overridden by -nocmds) */
		return 1;
	}

	str = strdup(allowed_external_cmds);
	p = strtok(str, ",");
	while (p) {
		if (!strcmp(p, cmd)) {
			free(str);
			return 1;
		}
		p = strtok(NULL, ",");
	}
	free(str);
	return 0;
}

/*
 * utility to run a user supplied command setting some RFB_ env vars.
 * used by, e.g., accept_client() and client_gone()
 */
int run_user_command(char *cmd, rfbClientPtr client, char *mode, char *input,
   int len, FILE *output) {
	char *old_display = NULL;
	char *addr = NULL;
	char str[100];
	int rc, ok;
	ClientData *cd = NULL;
	client_set_net(client);
	if (client != NULL) {
		cd = (ClientData *) client->clientData;
		addr = client->host;
	}

	if (addr == NULL || addr[0] == '\0') {
		addr = "unknown-host";
	}

	/* set RFB_CLIENT_ID to semi unique id for command to use */
	if (cd && cd->uid) {
		sprintf(str, "0x%x", cd->uid);
	} else {
		/* not accepted yet: */
		sprintf(str, "0x%x", clients_served);
	}
	set_env("RFB_CLIENT_ID", str);

	/* set RFB_CLIENT_IP to IP addr for command to use */
	set_env("RFB_CLIENT_IP", addr);

	/* set RFB_X11VNC_PID to our pid for command to use */
	sprintf(str, "%d", (int) getpid());
	set_env("RFB_X11VNC_PID", str);

	if (client == NULL) {
		;
	} else if (client->state == RFB_PROTOCOL_VERSION) {
		set_env("RFB_STATE", "PROTOCOL_VERSION");
	} else if (client->state == RFB_SECURITY_TYPE) {
		set_env("RFB_STATE", "SECURITY_TYPE");
	} else if (client->state == RFB_AUTHENTICATION) {
		set_env("RFB_STATE", "AUTHENTICATION");
	} else if (client->state == RFB_INITIALISATION) {
		set_env("RFB_STATE", "INITIALISATION");
	} else if (client->state == RFB_NORMAL) {
		set_env("RFB_STATE", "NORMAL");
	} else {
		set_env("RFB_STATE", "UNKNOWN");
	}
	if (certret_str) {
		set_env("RFB_SSL_CLIENT_CERT", certret_str);
	} else {
		set_env("RFB_SSL_CLIENT_CERT", "");
	}

	/* set RFB_CLIENT_PORT to peer port for command to use */
	if (cd && cd->client_port > 0) {
		sprintf(str, "%d", cd->client_port);
	} else if (client) {
		sprintf(str, "%d", get_remote_port(client->sock));
	}
	set_env("RFB_CLIENT_PORT", str);

	set_env("RFB_MODE", mode);

	/* 
	 * now do RFB_SERVER_IP and RFB_SERVER_PORT (i.e. us!)
	 * This will establish a 5-tuple (including tcp) the external
	 * program can potentially use to work out the virtual circuit
	 * for this connection.
	 */
	if (cd && cd->server_ip) {
		set_env("RFB_SERVER_IP", cd->server_ip);
	} else if (client) {
		char *sip = get_local_host(client->sock);
		set_env("RFB_SERVER_IP", sip);
		if (sip) free(sip);
	}

	if (cd && cd->server_port > 0) {
		sprintf(str, "%d", cd->server_port);
	} else if (client) {
		sprintf(str, "%d", get_local_port(client->sock));
	}
	set_env("RFB_SERVER_PORT", str);

	if (cd) {
		sprintf(str, "%d", cd->login_viewonly);
	} else {
		sprintf(str, "%d", -1);
	}
	set_env("RFB_LOGIN_VIEWONLY", str);

	if (cd) {
		sprintf(str, "%d", (int) cd->login_time);
	} else {
		sprintf(str, ">%d", (int) time(NULL));
	}
	set_env("RFB_LOGIN_TIME", str);

	sprintf(str, "%d", (int) time(NULL));
	set_env("RFB_CURRENT_TIME", str);

	if (!cd || !cd->username || cd->username[0] == '\0') {
		set_env("RFB_USERNAME", "unknown-user");
	} else {
		set_env("RFB_USERNAME", cd->username);
	}
	/* 
	 * Better set DISPLAY to the one we are polling, if they
	 * want something trickier, they can handle on their own
	 * via environment, etc. 
	 */
	if (getenv("DISPLAY")) {
		old_display = strdup(getenv("DISPLAY"));
	}

	if (raw_fb && ! dpy) {	/* raw_fb hack */
		set_env("DISPLAY", "rawfb");
	} else {
		set_env("DISPLAY", DisplayString(dpy));
	}

	/*
	 * work out the number of clients (have to use client_count
	 * since there is deadlock in rfbGetClientIterator) 
	 */
	sprintf(str, "%d", client_count);
	set_env("RFB_CLIENT_COUNT", str);

	/* gone, accept, afteraccept */
	ok = 0;
	if (!strcmp(mode, "env")) {
		return 1;
	}
	if (!strcmp(mode, "accept") && cmd_ok("accept")) {
		ok = 1;
	}
	if (!strcmp(mode, "afteraccept") && cmd_ok("afteraccept")) {
		ok = 1;
	}
	if (!strcmp(mode, "gone") && cmd_ok("gone")) {
		ok = 1;
	}
	if (!strcmp(mode, "cmd_verify") && cmd_ok("unixpw")) {
		ok = 1;
	}
	if (!strcmp(mode, "read_passwds") && cmd_ok("passwdfile")) {
		ok = 1;
	}
	if (!strcmp(mode, "custom_passwd") && cmd_ok("custom_passwd")) {
		ok = 1;
	}
	if (no_external_cmds || !ok) {
		rfbLogEnable(1);
		rfbLog("cannot run external commands in -nocmds mode:\n");
		rfbLog("   \"%s\"\n", cmd);
		rfbLog("   exiting.\n");
		clean_up_exit(1);
	}
	rfbLog("running command:\n");
	if (!quiet) {
		fprintf(stderr, "\n  %s\n\n", cmd);
	}
	close_exec_fds();

	if (output != NULL) {
		FILE *ph;
		char line[1024];
		char *cmd2 = NULL;
		char tmp[] = "/tmp/x11vnc-tmp.XXXXXX";
		int deltmp = 0;

		if (input != NULL) {
			int tmp_fd = mkstemp(tmp);
			if (tmp_fd < 0) {
				rfbLog("mkstemp failed on: %s\n", tmp);
				clean_up_exit(1);
			}
			write(tmp_fd, input, len);
			close(tmp_fd);
			deltmp = 1;
			cmd2 = (char *) malloc(100 + strlen(tmp) + strlen(cmd));
			sprintf(cmd2, "/bin/cat %s | %s", tmp, cmd);
			
			ph = popen(cmd2, "r");
		} else {
			ph = popen(cmd, "r");
		}
		if (ph == NULL) {
			rfbLog("popen(%s) failed", cmd);
			rfbLogPerror("popen");
			clean_up_exit(1);
		}
		memset(line, 0, sizeof(line));
		while (fgets(line, sizeof(line), ph) != NULL) {
			int j, k = -1;
			if (0) fprintf(stderr, "line: %s", line);
			/* take care to handle embedded nulls */
			for (j=0; j < (int) sizeof(line); j++) {
				if (line[j] != '\0') {
					k = j;
				}
			}
			if (k >= 0) {
				write(fileno(output), line, k+1);
			}
			memset(line, 0, sizeof(line));
		}

		rc = pclose(ph);

		if (cmd2 != NULL) {
			free(cmd2);
		}
		if (deltmp) {
			unlink(tmp);
		}
		goto got_rc;
	} else if (input != NULL) {
		FILE *ph = popen(cmd, "w");
		if (ph == NULL) {
			rfbLog("popen(%s) failed", cmd);
			rfbLogPerror("popen");
			clean_up_exit(1);
		}
		write(fileno(ph), input, len);
		rc = pclose(ph);
		goto got_rc;
	}

#if LIBVNCSERVER_HAVE_FORK
	{
		pid_t pid, pidw;
		struct sigaction sa, intr, quit;
		sigset_t omask;

		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		sigaction(SIGINT,  &sa, &intr);
		sigaction(SIGQUIT, &sa, &quit);

		sigaddset(&sa.sa_mask, SIGCHLD);
		sigprocmask(SIG_BLOCK, &sa.sa_mask, &omask);

		if ((pid = fork()) > 0 || pid == -1) {

			if (pid != -1) {
				pidw = waitpid(pid, &rc, 0);
			}

			sigaction(SIGINT,  &intr, (struct sigaction *) NULL);
			sigaction(SIGQUIT, &quit, (struct sigaction *) NULL);
			sigprocmask(SIG_SETMASK, &omask, (sigset_t *) NULL);

			if (pid == -1) {
				fprintf(stderr, "could not fork\n");
				rfbLogPerror("fork");
				rc = system(cmd);
			}
		} else {
			/* this should close port 5900, etc.. */
			int fd;
			sigaction(SIGINT,  &intr, (struct sigaction *) NULL);
			sigaction(SIGQUIT, &quit, (struct sigaction *) NULL);
			sigprocmask(SIG_SETMASK, &omask, (sigset_t *) NULL);
			for (fd=3; fd<256; fd++) {
				close(fd);
			}
/* XXX test more */
			if (!strcmp(mode, "gone")) {
#if LIBVNCSERVER_HAVE_SETSID
				setsid();
#else
				setpgrp();
#endif
			}
			execlp("/bin/sh", "/bin/sh", "-c", cmd, (char *) NULL);
			exit(1);
		}
	}
#else
	rc = system(cmd);
#endif
	got_rc:

	if (rc >= 256) {
		rc = rc/256;
	}
	rfbLog("command returned: %d\n", rc);

	if (old_display) {
		set_env("DISPLAY", old_display);
		free(old_display);
	}

	return rc;
}

static void free_client_data(rfbClientPtr client) {
	if (! client) {
		return;
	}
	if (client->clientData) {
		ClientData *cd = (ClientData *) client->clientData;
		if (cd) {
			if (cd->server_ip) {
				free(cd->server_ip);
				cd->server_ip = NULL;
			}
			if (cd->hostname) {
				free(cd->hostname);
				cd->hostname = NULL;
			}
			if (cd->username) {
				free(cd->username);
				cd->username = NULL;
			}
			if (cd->unixname) {
				free(cd->unixname);
				cd->unixname = NULL;
			}
		}
		free(client->clientData);
		client->clientData = NULL;
	}
}

static int accepted_client = 0;

/*
 * callback for when a client disconnects
 */
void client_gone(rfbClientPtr client) {
	ClientData *cd = NULL;

	CLIENT_LOCK;

	client_count--;
	if (client_count < 0) client_count = 0;

	speeds_net_rate_measured = 0;
	speeds_net_latency_measured = 0;

	rfbLog("client_count: %d\n", client_count);
	last_client_gone = dnow();

	if (unixpw_in_progress && unixpw_client) {
		if (client == unixpw_client) {
			unixpw_in_progress = 0;
			/* mutex */
			screen->permitFileTransfer = unixpw_file_xfer_save;
			if ((tightfilexfer = unixpw_tightvnc_xfer_save)) {
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
				rfbLog("rfbRegisterTightVNCFileTransferExtension: 3\n");
				rfbRegisterTightVNCFileTransferExtension();
#endif
			}
			unixpw_client = NULL;
			copy_screen();
		}
	}


	if (no_autorepeat && client_count == 0) {
		autorepeat(1, 0);
	}
	if (use_solid_bg && client_count == 0) {
		solid_bg(1);
	}
	if ((ncache || ncache0) && client_count == 0) {
		kde_no_animate(1);
	}
	if (client->clientData) {
		cd = (ClientData *) client->clientData;
		if (cd->ssl_helper_pid > 0) {
			int status;
			rfbLog("sending SIGTERM to ssl_helper_pid: %d\n",
			    cd->ssl_helper_pid);
			kill(cd->ssl_helper_pid, SIGTERM);
			usleep(200*1000);
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID 
			waitpid(cd->ssl_helper_pid, &status, WNOHANG); 
#endif
			ssl_helper_pid(cd->ssl_helper_pid, -1);	/* delete */
		}
	}
	if (gone_cmd && *gone_cmd != '\0') {
		if (strstr(gone_cmd, "popup") == gone_cmd) {
			int x = -64000, y = -64000, timeout = 120;
			char *userhost = ident_username(client);
			char *addr, *p, *mode;

			/* extract timeout */
			if ((p = strchr(gone_cmd, ':')) != NULL) {
				int in;
				if (sscanf(p+1, "%d", &in) == 1) {
					timeout = in;
				}
			}
			/* extract geometry */
			if ((p = strpbrk(gone_cmd, "+-")) != NULL) {
				ugly_geom(p, &x, &y);
			}

			/* find mode: mouse, key, or both */
			if (strstr(gone_cmd, "popupmouse") == gone_cmd) {
				mode = "mouse_only";
			} else if (strstr(gone_cmd, "popupkey") == gone_cmd) {
				mode = "key_only";
			} else {
				mode = "both";
			}

			addr = client->host;

			ugly_window(addr, userhost, x, y, timeout, mode, 0);

			free(userhost);
		} else {
			rfbLog("client_gone: using cmd: %s\n", client->host);
			run_user_command(gone_cmd, client, "gone", NULL, 0, NULL);
		}
	}

	free_client_data(client);

	if (inetd && client == inetd_client) {
		rfbLog("inetd viewer exited.\n");
		if (gui_pid > 0) {
			rfbLog("killing gui_pid %d\n", gui_pid);
			kill(gui_pid, SIGTERM);
		}
		clean_up_exit(0);
	}

	if (connect_once) {
		/*
		 * This non-exit is done for a bad passwd to be consistent
		 * with our RFB_CLIENT_REFUSE behavior in new_client()  (i.e.
		 * we disconnect after 1 successful connection).
		 */
		if ((client->state == RFB_PROTOCOL_VERSION ||
		     client->state == RFB_SECURITY_TYPE ||
		     client->state == RFB_AUTHENTICATION ||
		     client->state == RFB_INITIALISATION) && accepted_client) {
			rfbLog("connect_once: invalid password or early "
			   "disconnect.  %d\n", client->state);
			rfbLog("connect_once: waiting for next connection.\n"); 
			accepted_client--;
			if (accepted_client < 0) {
				accepted_client = 0;
			}
			CLIENT_UNLOCK;
			if (connect_or_exit) {
				clean_up_exit(1);
			}
			return;
		}
		if (shared && client_count > 0)  {
			rfbLog("connect_once: other shared clients still "
			    "connected, not exiting.\n");
			CLIENT_UNLOCK;
			return;
		}

		rfbLog("viewer exited.\n");
		if ((client_connect || connect_or_exit) && gui_pid > 0) {
			rfbLog("killing gui_pid %d\n", gui_pid);
			kill(gui_pid, SIGTERM);
		}
		CLIENT_UNLOCK;
		clean_up_exit(0);
	}
#ifdef MACOSX
	if (macosx_console && client_count == 0) {
		macosxCG_refresh_callback_off();
	}
#endif
	CLIENT_UNLOCK;
}

/*
 * Simple routine to limit access via string compare.  A power user will
 * want to compile libvncserver with libwrap support and use /etc/hosts.allow.
 */
int check_access(char *addr) {
	int allowed = 0;
	int ssl = 0;
	char *p, *list;

	if (use_openssl || use_stunnel) {
		ssl = 1;
	}
	if (deny_all) {
		rfbLog("check_access: new connections are currently "
		    "blocked.\n");
		return 0;
	}
	if (addr == NULL || *addr == '\0') {
		rfbLog("check_access: denying empty host IP address string.\n");
		return 0;
	}

	if (allow_list == NULL) {
		/* set to "" to possibly append allow_once */
		allow_list = strdup("");
	}
	if (*allow_list == '\0' && allow_once == NULL) {
		/* no constraints, accept it */
		return 1;
	}

	if (strchr(allow_list, '/')) {
		/* a file of IP addresess or prefixes */
		int len, len2 = 0;
		struct stat sbuf;
		FILE *in;
		char line[1024], *q;

		if (stat(allow_list, &sbuf) != 0) {
			rfbLogEnable(1);
			rfbLog("check_access: failure stating file: %s\n",
			    allow_list);
			rfbLogPerror("stat");
			clean_up_exit(1);
		}
		len = sbuf.st_size + 1;	/* 1 more for '\0' at end */
		if (allow_once) {
			len2 = strlen(allow_once) + 2;
			len += len2;
		}
		if (ssl) {
			len2 = strlen("127.0.0.1") + 2;
			len += len2;
		}
		list = (char *) malloc(len);
		list[0] = '\0';
		
		in = fopen(allow_list, "r");
		if (in == NULL) {
			rfbLogEnable(1);
			rfbLog("check_access: cannot open: %s\n", allow_list);
			rfbLogPerror("fopen");
			clean_up_exit(1);
		}
		while (fgets(line, 1024, in) != NULL) {
			if ( (q = strchr(line, '#')) != NULL) {
				*q = '\0';
			}
			if (strlen(list) + strlen(line) >=
			    (size_t) (len - len2)) {
				/* file grew since our stat() */
				break;
			}
			strcat(list, line);
		}
		fclose(in);
		if (allow_once) {
			strcat(list, "\n");
			strcat(list, allow_once);
			strcat(list, "\n");
		}
		if (ssl) {
			strcat(list, "\n");
			strcat(list, "127.0.0.1");
			strcat(list, "\n");
		}
	} else {
		int len = strlen(allow_list) + 1;
		if (allow_once) {
			len += strlen(allow_once) + 1;
		}
		if (ssl) {
			len += strlen("127.0.0.1") + 1;
		}
		list = (char *) malloc(len);
		list[0] = '\0';
		strcat(list, allow_list);
		if (allow_once) {
			strcat(list, ",");
			strcat(list, allow_once);
		}
		if (ssl) {
			strcat(list, ",");
			strcat(list, "127.0.0.1");
		}
	}

	if (allow_once) {
		free(allow_once);
		allow_once = NULL;
	}
	
	p = strtok(list, ", \t\n\r");
	while (p) {
		char *chk, *q, *r = NULL;
		if (*p == '\0') {
			p = strtok(NULL, ", \t\n\r");
			continue;	
		}
		if (ipv6_ip(p)) {
			chk = p;
		} else if (! dotted_ip(p, 1)) {
			r = host2ip(p);
			if (r == NULL || *r == '\0') {
				rfbLog("check_access: bad lookup \"%s\"\n", p);
				p = strtok(NULL, ", \t\n\r");
				continue;
			}
			rfbLog("check_access: lookup %s -> %s\n", p, r);
			chk = r;
		} else {
			chk = p;
		}
		if (getenv("X11VNC_DEBUG_ACCESS")) fprintf(stderr, "chk: %s  part: %s  addr: %s\n", chk, p, addr);

		q = strstr(addr, chk);
		if (ipv6_ip(addr)) {
			if (!strcmp(chk, "localhost") && !strcmp(addr, "::1")) {
				rfbLog("check_access: client addr %s is local.\n", addr);
				allowed = 1;
			} else if (!strcmp(chk, "::1") && !strcmp(addr, "::1")) {
				rfbLog("check_access: client addr %s is local.\n", addr);
				allowed = 1;
			} else if (!strcmp(chk, "127.0.0.1") && !strcmp(addr, "::1")) {
				/* this if for host2ip("localhost") */
				rfbLog("check_access: client addr %s is local.\n", addr);
				allowed = 1;
			} else if (q == addr) {
				rfbLog("check_access: client %s matches pattern %s\n", addr, chk);
				allowed = 1;
			}
		} else if (chk[strlen(chk)-1] != '.') {
			if (!strcmp(addr, chk)) {
				if (chk != p) {
					rfbLog("check_access: client %s " "matches host %s=%s\n", addr, chk, p);
				} else {
					rfbLog("check_access: client %s " "matches host %s\n", addr, chk);
				}
				allowed = 1;
			} else if(!strcmp(chk, "localhost") && !strcmp(addr, "127.0.0.1")) {
				allowed = 1;
			}
		} else if (q == addr) {
			rfbLog("check_access: client %s matches pattern %s\n", addr, chk);
			allowed = 1;
		}
		p = strtok(NULL, ", \t\n\r");
		if (r) {
			free(r);
		}
		if (allowed) {
			break;
		}
	}
	free(list);
	return allowed;
}

/*
 * x11vnc's first (and only) visible widget: accept/reject dialog window.
 * We go through this pain to avoid dependency on libXt...
 */
static int ugly_window(char *addr, char *userhost, int X, int Y,
    int timeout, char *mode, int accept) {
#if NO_X11
	if (!addr || !userhost || !X || !Y || !timeout || !mode || !accept) {}
	RAWFB_RET(0)
	nox11_exit(1);
	return 0;
#else

#define t2x2_width 16
#define t2x2_height 16
static unsigned char t2x2_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0x33, 0x33, 0x33, 0x33, 0xff, 0xff, 0xff, 0xff,
   0x33, 0x33, 0x33, 0x33, 0xff, 0xff, 0xff, 0xff, 0x33, 0x33, 0x33, 0x33,
   0xff, 0xff, 0xff, 0xff, 0x33, 0x33, 0x33, 0x33};

	Window awin;
	GC gc;
	XSizeHints hints;
	XGCValues values;
	static XFontStruct *font_info = NULL;
	static Pixmap ico = 0;
	unsigned long valuemask = 0;
	static char dash_list[] = {20, 40};
	int list_length = sizeof(dash_list);

	Atom wm_protocols;
	Atom wm_delete_window;

	XEvent ev;
	long evmask = ExposureMask | KeyPressMask | ButtonPressMask
	    | StructureNotifyMask;
	double waited = 0.0;

	/* strings and geometries y/n */
	KeyCode key_y, key_n, key_v;
	char strh[100];
	char stri[100];
	char str1_b[] = "To accept: press \"y\" or click the \"Yes\" button";
	char str2_b[] = "To reject: press \"n\" or click the \"No\" button";
	char str3_b[] = "View only: press \"v\" or click the \"View\" button";
	char str1_m[] = "To accept: click the \"Yes\" button";
	char str2_m[] = "To reject: click the \"No\" button";
	char str3_m[] = "View only: click the \"View\" button";
	char str1_k[] = "To accept: press \"y\"";
	char str2_k[] = "To reject: press \"n\"";
	char str3_k[] = "View only: press \"v\"";
	char *str1, *str2, *str3;
	char str_y[] = "Yes";
	char str_n[] = "No";
	char str_v[] = "View";
	int x, y, w = 345, h = 175, ret = 0;
	int X_sh = 20, Y_sh = 30, dY = 20;
	int Ye_x = 20,  Ye_y = 0, Ye_w = 45, Ye_h = 20;
	int No_x = 75,  No_y = 0, No_w = 45, No_h = 20; 
	int Vi_x = 130, Vi_y = 0, Vi_w = 45, Vi_h = 20; 
	char *sprop = "new x11vnc client";

	KeyCode key_o;

	RAWFB_RET(0)

	if (! accept) {
		sprintf(str_y, "OK");
		sprop = "x11vnc client disconnected";
		h = 110;
		str1 = "";
		str2 = "";
		str3 = "";
	} else if (!strcmp(mode, "mouse_only")) {
		str1 = str1_m;
		str2 = str2_m;
		str3 = str3_m;
	} else if (!strcmp(mode, "key_only")) {
		str1 = str1_k;
		str2 = str2_k;
		str3 = str3_k;
		h -= dY;
	} else {
		str1 = str1_b;
		str2 = str2_b;
		str3 = str3_b;
	}
	if (view_only) {
		h -= dY;
	}

	/* XXX handle coff_x/coff_y? */
	if (X < -dpy_x) {
		x = (dpy_x - w)/2;	/* large negative: center */
		if (x < 0) x = 0;
	} else if (X < 0) {
		x = dpy_x + X - w;	/* from lower right */
	} else {
		x = X;			/* from upper left */
	}
	
	if (Y < -dpy_y) {
		y = (dpy_y - h)/2;
		if (y < 0) y = 0;
	} else if (Y < 0) {
		y = dpy_y + Y - h;
	} else {
		y = Y;
	}

	X_LOCK;

	awin = XCreateSimpleWindow(dpy, window, x, y, w, h, 4,
	    BlackPixel(dpy, scr), WhitePixel(dpy, scr));

	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, awin, &wm_delete_window, 1);

	if (! ico) {
		ico = XCreateBitmapFromData(dpy, awin, (char *) t2x2_bits,
		    t2x2_width, t2x2_height);
	}

	hints.flags = PPosition | PSize | PMinSize;
	hints.x = x;
	hints.y = y;
	hints.width = w;
	hints.height = h;
	hints.min_width = w;
	hints.min_height = h;

	XSetStandardProperties(dpy, awin, sprop, "x11vnc query", ico, NULL,
	    0, &hints);

	XSelectInput_wr(dpy, awin, evmask);

	if (! font_info && (font_info = XLoadQueryFont(dpy, "fixed")) == NULL) {
		rfbLogEnable(1);
		rfbLog("ugly_window: cannot locate font fixed.\n");
		X_UNLOCK;
		clean_up_exit(1);
	}

	gc = XCreateGC(dpy, awin, valuemask, &values);
	XSetFont(dpy, gc, font_info->fid);
	XSetForeground(dpy, gc, BlackPixel(dpy, scr));
	XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
	XSetDashes(dpy, gc, 0, dash_list, list_length);

	XMapWindow(dpy, awin);
	XFlush_wr(dpy);

	if (accept) {
		char *ip = addr;
		char *type = "accept";
		if (unixpw && strstr(userhost, "UNIX:") != userhost) {
			type = "UNIXPW";
			if (openssl_last_ip) {
				ip = openssl_last_ip;
			}
		}
		snprintf(strh, 100, "x11vnc: %s connection from %s?", type, ip);
	} else {
		snprintf(strh, 100, "x11vnc: client disconnected from %s", addr);
	}
	snprintf(stri, 100, "        (%s)", userhost);

	key_o = XKeysymToKeycode(dpy, XStringToKeysym("o"));
	key_y = XKeysymToKeycode(dpy, XStringToKeysym("y"));
	key_n = XKeysymToKeycode(dpy, XStringToKeysym("n"));
	key_v = XKeysymToKeycode(dpy, XStringToKeysym("v"));

	while (1) {
		int out = -1, x, y, tw, k;

		if (XCheckWindowEvent(dpy, awin, evmask, &ev)) {
			;	/* proceed to handling */
		} else if (XCheckTypedEvent(dpy, ClientMessage, &ev)) {
			;	/* proceed to handling */
		} else {
			int ms = 100;	/* sleep a bit */
			usleep(ms * 1000);
			waited += ((double) ms)/1000.;
			if (timeout && (int) waited >= timeout) {
				rfbLog("ugly_window: popup timed out after "
				    "%d seconds.\n", timeout);
				out = 0;
				ev.type = 0;
			} else {
				continue;
			}
		}

		switch(ev.type) {
		case Expose:
			while (XCheckTypedEvent(dpy, Expose, &ev)) {
				;
			}
			k=0;

			/* instructions */
			XDrawString(dpy, awin, gc, X_sh, Y_sh+(k++)*dY,
			    strh, strlen(strh));
			XDrawString(dpy, awin, gc, X_sh, Y_sh+(k++)*dY,
			    stri, strlen(stri));
			if (accept) {
			  XDrawString(dpy, awin, gc, X_sh, Y_sh+(k++)*dY,
			    str1, strlen(str1));
			  XDrawString(dpy, awin, gc, X_sh, Y_sh+(k++)*dY,
			    str2, strlen(str2));
			  if (! view_only) {
				XDrawString(dpy, awin, gc, X_sh, Y_sh+(k++)*dY,
				    str3, strlen(str3));
			  }
			}

			if (!strcmp(mode, "key_only")) {
				break;
			}

			/* buttons */
			Ye_y = Y_sh+k*dY;
			No_y = Y_sh+k*dY;
			Vi_y = Y_sh+k*dY;
			XDrawRectangle(dpy, awin, gc, Ye_x, Ye_y, Ye_w, Ye_h);

			if (accept) {
			  XDrawRectangle(dpy, awin, gc, No_x, No_y, No_w, No_h);
			  if (! view_only) {
				XDrawRectangle(dpy, awin, gc, Vi_x, Vi_y,
				    Vi_w, Vi_h);
			  }
			}

			tw = XTextWidth(font_info, str_y, strlen(str_y));
			tw = (Ye_w - tw)/2;
			if (tw < 0) tw = 1;
			XDrawString(dpy, awin, gc, Ye_x+tw, Ye_y+Ye_h-5,
			    str_y, strlen(str_y));

			if (!accept) {
				break;
			}
			tw = XTextWidth(font_info, str_n, strlen(str_n));
			tw = (No_w - tw)/2;
			if (tw < 0) tw = 1;
			XDrawString(dpy, awin, gc, No_x+tw, No_y+No_h-5,
			    str_n, strlen(str_n));

			if (! view_only) {
				tw = XTextWidth(font_info, str_v,
				    strlen(str_v));
				tw = (Vi_w - tw)/2;
				if (tw < 0) tw = 1;
				XDrawString(dpy, awin, gc, Vi_x+tw,
				    Vi_y+Vi_h-5, str_v, strlen(str_v));
			}

			break;

		case ClientMessage:
			if (ev.xclient.message_type == wm_protocols &&
			    (Atom) ev.xclient.data.l[0] == wm_delete_window) {
				out = 0;
			}
			break;

		case ButtonPress:
			x = ev.xbutton.x;
			y = ev.xbutton.y;
			if (!strcmp(mode, "key_only")) {
				;
			} else if (x > Ye_x && x < Ye_x+Ye_w && y > Ye_y
			    && y < Ye_y+Ye_h) {
				out = 1;
			} else if (! accept) {
				;
			} else if (x > No_x && x < No_x+No_w && y > No_y
			    && y < No_y+No_h) {
				out = 0;
			} else if (! view_only && x > Vi_x && x < Vi_x+Vi_w
			    && y > Vi_y && y < Vi_y+Ye_h) {
				out = 2;
			}
			break;

		case KeyPress:
			if (!strcmp(mode, "mouse_only")) {
				;
			} else if (! accept) {
				if (ev.xkey.keycode == key_o) {
					out = 1;
				}
				if (ev.xkey.keycode == key_y) {
					out = 1;
				}
			} else if (ev.xkey.keycode == key_y) {
				out = 1;
				;
			} else if (ev.xkey.keycode == key_n) {
				out = 0;
			} else if (! view_only && ev.xkey.keycode == key_v) {
				out = 2;
			}
			break;
		default:
			break;
		}
		if (out != -1) {
			ret = out;
			XSelectInput_wr(dpy, awin, 0);
			XUnmapWindow(dpy, awin);
			XFree_wr(gc);
			XDestroyWindow(dpy, awin);
			XFlush_wr(dpy);
			break;
		}
	}
	X_UNLOCK;

	return ret;
#endif	/* NO_X11 */
}

/*
 * process a "yes:0,no:*,view:3" type action list comparing to command
 * return code rc.  * means the default action with no other match.
 */
static int action_match(char *action, int rc) {
	char *p, *q, *s = strdup(action);
	int cases[4], i, result;
	char *labels[4];

	labels[1] = "yes";
	labels[2] = "no";
	labels[3] = "view";

	rfbLog("accept_client: process action line: %s\n",
	    action);

	for (i=1; i <= 3; i++) {
		cases[i] = -2;
	}

	p = strtok(s, ",");
	while (p) {
		if ((q = strchr(p, ':')) != NULL) {
			int in, k = 1;
			*q = '\0';
			q++;
			if (strstr(p, "yes") == p) {
				k = 1;
			} else if (strstr(p, "no") == p) {
				k = 2;
			} else if (strstr(p, "view") == p) {
				k = 3;
			} else {
				rfbLogEnable(1);
				rfbLog("invalid action line: %s\n", action);
				clean_up_exit(1);
			}
			if (*q == '*') {
				cases[k] = -1;
			} else if (sscanf(q, "%d", &in) == 1) {
				if (in < 0) {
					rfbLogEnable(1);
					rfbLog("invalid action line: %s\n",
					    action);
					clean_up_exit(1);
				}
				cases[k] = in;
			} else {
				rfbLogEnable(1);
				rfbLog("invalid action line: %s\n", action);
				clean_up_exit(1);
			}
		} else {
			rfbLogEnable(1);
			rfbLog("invalid action line: %s\n", action);
			clean_up_exit(1);
		}
		p = strtok(NULL, ",");
	}
	free(s);

	result = -1;
	for (i=1; i <= 3; i++) {
		if (cases[i] == -1) {
			rfbLog("accept_client: default action is case=%d %s\n",
			    i, labels[i]);
			result = i;
			break;
		}
	}
	if (result == -1) {
		rfbLog("accept_client: no default action\n");
	}
	for (i=1; i <= 3; i++) {
		if (cases[i] >= 0 && cases[i] == rc) {
			rfbLog("accept_client: matched action is case=%d %s\n",
			    i, labels[i]);
			result = i;
			break;
		}
	}
	if (result < 0) {
		rfbLog("no action match: %s rc=%d set to no\n", action, rc);
		result = 2;
	}
	return result;
}

static void ugly_geom(char *p, int *x, int *y) {
	int x1, y1;

	if (sscanf(p, "+%d+%d", &x1, &y1) == 2) {
		*x = x1;
		*y = y1;
	} else if (sscanf(p, "+%d-%d", &x1, &y1) == 2) {
		*x = x1;
		*y = -y1;
	} else if (sscanf(p, "-%d+%d", &x1, &y1) == 2) {
		*x = -x1;
		*y = y1;
	} else if (sscanf(p, "-%d-%d", &x1, &y1) == 2) {
		*x = -x1;
		*y = -y1;
	}
}

/*
 * Simple routine to prompt the user on the X display whether an incoming
 * client should be allowed to connect or not.  If a gui is involved it
 * will be running in the environment/context of the X11 DISPLAY.
 *
 * The command supplied via -accept is run as is (i.e. no string
 * substitution) with the RFB_CLIENT_IP environment variable set to the
 * incoming client's numerical IP address.
 * 
 * If the external command exits with 0 the client is accepted, otherwise
 * the client is rejected.
 * 
 * Some builtins are provided:
 *
 *	xmessage:  use homebrew xmessage(1) for the external command.  
 *	popup:     use internal X widgets for prompting.
 * 
 */
int accept_client(rfbClientPtr client) {

	char xmessage[200], *cmd = NULL;
	char *addr = client->host;
	char *action = NULL;

	if (accept_cmd == NULL || *accept_cmd == '\0') {
		return 1;	/* no command specified, so we accept */
	}

	if (addr == NULL || addr[0] == '\0') {
		addr = "unknown-host";
	}

	if (strstr(accept_cmd, "popup") == accept_cmd) {
		/* use our builtin popup button */

		/* (popup|popupkey|popupmouse)[+-X+-Y][:timeout] */

		int ret, timeout = 120;
		int x = -64000, y = -64000;
		char *p, *mode;
		char *userhost = ident_username(client);

		/* extract timeout */
		if ((p = strchr(accept_cmd, ':')) != NULL) {
			int in;
			if (sscanf(p+1, "%d", &in) == 1) {
				timeout = in;
			}
		}
		/* extract geometry */
		if ((p = strpbrk(accept_cmd, "+-")) != NULL) {
			ugly_geom(p, &x, &y);
		}

		/* find mode: mouse, key, or both */
		if (strstr(accept_cmd, "popupmouse") == accept_cmd) {
			mode = "mouse_only";
		} else if (strstr(accept_cmd, "popupkey") == accept_cmd) {
			mode = "key_only";
		} else {
			mode = "both";
		}

		if (dpy == NULL && use_dpy && strstr(use_dpy, "WAIT:") ==
		    use_dpy) {
			rfbLog("accept_client: warning allowing client under conditions:\n");
			rfbLog("  -display WAIT:, dpy == NULL, -accept popup.\n");
			rfbLog("   There will be another popup.\n");
			return 1;
		}

		rfbLog("accept_client: using builtin popup for: %s\n", addr);
		if ((ret = ugly_window(addr, userhost, x, y, timeout,
		    mode, 1))) {
			free(userhost);
			if (ret == 2) {
				rfbLog("accept_client: viewonly: %s\n", addr);
				client->viewOnly = TRUE;
			}
			rfbLog("accept_client: popup accepted: %s\n", addr);
			return 1;
		} else {
			free(userhost);
			rfbLog("accept_client: popup rejected: %s\n", addr);
			return 0;
		}

	} else if (!strcmp(accept_cmd, "xmessage")) {
		/* make our own command using xmessage(1) */

		if (view_only) {
			sprintf(xmessage, "xmessage -buttons yes:0,no:2 -center"
			    " 'x11vnc: accept connection from %s?'", addr);
		} else {
			sprintf(xmessage, "xmessage -buttons yes:0,no:2,"
			    "view-only:3 -center" " 'x11vnc: accept connection"
			    " from %s?'", addr);
			action = "yes:0,no:*,view:3";
		}
		cmd = xmessage;
		
	} else {
		/* use the user supplied command: */

		cmd = accept_cmd;

		/* extract any action prefix:  yes:N,no:M,view:K */
		if (strstr(accept_cmd, "yes:") == accept_cmd) {
			char *p;
			if ((p = strpbrk(accept_cmd, " \t")) != NULL) {
				int i;
				cmd = p;
				p = accept_cmd;
				for (i=0; i<200; i++) {
					if (*p == ' ' || *p == '\t') {
						xmessage[i] = '\0';
						break;
					}
					xmessage[i] = *p;
					p++;
				}
				xmessage[200-1] = '\0';
				action = xmessage;
			}
		}
	}

	if (cmd) {
		int rc;

		rfbLog("accept_client: using cmd for: %s\n", addr);
		rc = run_user_command(cmd, client, "accept", NULL, 0, NULL);

		if (action) {
			int result;

			if (rc < 0) {
				rfbLog("accept_client: cannot use negative "
				    "rc: %d, action %s\n", rc, action);
				result = 2;
			} else {
				result = action_match(action, rc);
			}

			if (result == 1) {
				rc = 0;
			} else if (result == 2) {
				rc = 1;
			} else if (result == 3) {
				rc = 0;
				rfbLog("accept_client: viewonly: %s\n", addr);
				client->viewOnly = TRUE;
			} else {
				rc = 1;	/* NOTREACHED */
			}
		}

		if (rc == 0) {
			rfbLog("accept_client: accepted: %s\n", addr);
			return 1;
		} else {
			rfbLog("accept_client: rejected: %s\n", addr);
			return 0;
		}
	} else {
		rfbLog("accept_client: no command, rejecting %s\n", addr);
		return 0;
	}

	/* return 0; NOTREACHED */
}

void check_ipv6_listen(long usec) {
#if X11VNC_IPV6
	fd_set fds;
	struct timeval tv;
	int nfds, csock = -1, one = 1;
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof(addr);
	rfbClientPtr cl;
	int nmax = 0;
	char *name;

	if (!ipv6_listen || noipv6) {
		return;
	}
	if (ipv6_listen_fd < 0 && ipv6_http_fd < 0) {
		return;
	}

	FD_ZERO(&fds);
	if (ipv6_listen_fd >= 0) {
		FD_SET(ipv6_listen_fd, &fds);
		nmax = ipv6_listen_fd;
	}
	if (ipv6_http_fd >= 0 && screen->httpSock < 0) {
		FD_SET(ipv6_http_fd, &fds);
		if (ipv6_http_fd > nmax) {
			nmax = ipv6_http_fd;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(nmax+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}

	if (ipv6_listen_fd >= 0 && FD_ISSET(ipv6_listen_fd, &fds)) {

		csock = accept(ipv6_listen_fd, (struct sockaddr *)&addr, &addrlen);
		if (csock < 0) {
			rfbLogPerror("check_ipv6_listen: accept");
			goto err1;
		}
		if (fcntl(csock, F_SETFL, O_NONBLOCK) < 0) {
			rfbLogPerror("check_ipv6_listen: fcntl");
			close(csock);
			goto err1;
		}
		if (setsockopt(csock, IPPROTO_TCP, TCP_NODELAY,
		    (char *)&one, sizeof(one)) < 0) {
			rfbLogPerror("check_ipv6_listen: setsockopt");
			close(csock);
			goto err1;
		}

		name = ipv6_getipaddr((struct sockaddr *) &addr, addrlen);

		ipv6_client_ip_str = name;
		cl = rfbNewClient(screen, csock);
		ipv6_client_ip_str = NULL;
		if (cl == NULL) {
			close(csock);
			goto err1;
		}

		if (name) {
			if (cl->host) {
				free(cl->host);
			}
			cl->host = name;
			rfbLog("ipv6 client: %s\n", name);
		}
	}

	err1:

	if (ipv6_http_fd >= 0 && FD_ISSET(ipv6_http_fd, &fds)) {

		csock = accept(ipv6_http_fd, (struct sockaddr *)&addr, &addrlen);
		if (csock < 0) {
			rfbLogPerror("check_ipv6_listen: accept");
			return;
		}
		if (fcntl(csock, F_SETFL, O_NONBLOCK) < 0) {
			rfbLogPerror("check_ipv6_listen: fcntl");
			close(csock);
			return;
		}
		if (setsockopt(csock, IPPROTO_TCP, TCP_NODELAY,
		    (char *)&one, sizeof(one)) < 0) {
			rfbLogPerror("check_ipv6_listen: setsockopt");
			close(csock);
			return;
		}

		rfbLog("check_ipv6_listen: setting httpSock to %d\n", csock);
		screen->httpSock = csock;

		if (screen->httpListenSock < 0) {
			/* this may not always work... */
			int save = screen->httpListenSock;
			screen->httpListenSock = ipv6_http_fd;	
			rfbLog("check_ipv6_listen: no httpListenSock, calling rfbHttpCheckFds()\n");
			rfbHttpCheckFds(screen);
			screen->httpListenSock = save;	
		}
	}
#endif
	if (usec) {}
}

void check_unix_sock(long usec) {
	fd_set fds;
	struct timeval tv;
	int nfds, csock = -1;
	rfbClientPtr cl;
	int nmax = 0;
	char *name;

	if (!unix_sock || unix_sock_fd < 0) {
		return;
	}

	FD_ZERO(&fds);
	if (unix_sock_fd >= 0) {
		FD_SET(unix_sock_fd, &fds);
		nmax = unix_sock_fd;
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(nmax+1, &fds, NULL, NULL, &tv);

	if (nfds <= 0) {
		return;
	}

	if (unix_sock_fd >= 0 && FD_ISSET(unix_sock_fd, &fds)) {
		csock = accept_unix(unix_sock_fd);
		if (csock < 0) {
			return;
		}
		if (fcntl(csock, F_SETFL, O_NONBLOCK) < 0) {
			rfbLogPerror("check_unix_sock: fcntl");
			close(csock);
			return;
		}

		/* rfbNewClient() will screw us with setsockopt TCP_NODELAY...
		   you need to comment out in libvncserver/rfbserver.c:
			rfbLogPerror("setsockopt failed");
			close(sock);
			return NULL;
		 */
		cl = rfbNewClient(screen, csock);

		if (cl == NULL) {
			close(csock);
			return;
		}

		name = strdup(unix_sock);

		if (name) {
			if (cl->host) {
				free(cl->host);
			}
			cl->host = name;
			rfbLog("unix sock client: %s\n", name);
		}
	}
}

/*
 * For the -connect <file> option: periodically read the file looking for
 * a connect string.  If one is found set client_connect to it.
 */
static void check_connect_file(char *file) {
	FILE *in;
	char line[VNC_CONNECT_MAX], host[VNC_CONNECT_MAX];
	static int first_warn = 1, truncate_ok = 1;
	static double last_time = 0.0, delay = 0.5; 
	double now = dnow();
	struct stat sbuf;

	if (last_time == 0.0) {
		if (!getenv("X11VNC_APPSHARE_ACTIVE")) {
			/* skip first */
			last_time = now;
		} else {
			delay = 0.25;
		}
	}
	if (now - last_time < delay) {
		/* check only about once a second */
		return;
	}
	last_time = now;

	if (! truncate_ok) {
		/* check if permissions changed */
		if (access(file, W_OK) == 0) {
			truncate_ok = 1;
		} else {
			return;
		}
	}

	if (stat(file, &sbuf) == 0) {
		/* skip empty file directly */
		if (sbuf.st_size == 0) {
			return;
		}
	}

	in = fopen(file, "r");
	if (in == NULL) {
		if (first_warn) {
			rfbLog("check_connect_file: fopen failure: %s\n", file);
			rfbLogPerror("fopen");
			first_warn = 0;
		}
		return;
	}

	if (fgets(line, VNC_CONNECT_MAX, in) != NULL) {
		if (sscanf(line, "%s", host) == 1) {
			if (strlen(host) > 0) {
				char *str = strdup(host);
				if (strlen(str) > 38) {
					char trim[100]; 
					trim[0] = '\0';
					strncat(trim, str, 38);
					rfbLog("read connect file: %s ...\n",
					    trim);
				} else {
					rfbLog("read connect file: %s\n", str);
				}
				if (!strcmp(str, "cmd=stop") &&
				    dnowx() < 3.0) {
					rfbLog("ignoring stale cmd=stop\n");
				} else {
					client_connect = str;
				}
			}
		}
	}
	fclose(in);

	/* truncate file */
	in = fopen(file, "w");
	if (in != NULL) {
		fclose(in);
	} else {
		/* disable if we cannot truncate */
		rfbLog("check_connect_file: could not truncate %s, "
		   "disabling checking.\n", file);
		truncate_ok = 0;
	}
}

static int socks5_proxy(char *host, int port, int sock) {
	unsigned char buf[512], tmp[2];
	char reply[512];
	int len, n, i, j = 0;

	memset(buf, 0, 512);
	memset(reply, 0, 512);

	buf[0] = 0x5;
	buf[1] = 0x1;
	buf[2] = 0x0;

	write(sock, buf, 3);

	n = read(sock, buf, 2);

	if (n != 2) {
		rfbLog("socks5_proxy: read error: %d\n", n);
		close(sock);
		return 0;
	}
	if (buf[0] != 0x5 || buf[1] != 0x0) {
		rfbLog("socks5_proxy: handshake error: %d %d\n", (int) buf[0], (int) buf[1]);
		close(sock);
		return 0;
	}

	buf[0] = 0x5;
	buf[1] = 0x1;
	buf[2] = 0x0;
	buf[3] = 0x3;

	buf[4] = (unsigned char) strlen(host);
	strcat((char *) buf+5, host); 

	len = 5 + strlen(host);

	buf[len]   = (unsigned char) (port >> 8);
	buf[len+1] = (unsigned char) (port & 0xff);

	write(sock, buf, len+2);

	for (i=0; i<4; i++) {
		int n;
		n = read(sock, tmp, 1);
		j++;
		if (n < 0) {
			if (errno != EINTR) {
				break;
			} else {
				i--;
				if (j > 100) {
					break;
				}
				continue;
			}
		}
		if (n == 0) {
			break;
		}
		reply[i] = tmp[0];
	}
	if (reply[3] == 0x1) {
		read(sock, reply+4, 4 + 2);
	} else if (reply[3] == 0x3) {
		n = read(sock, tmp, 1);
		reply[4] = tmp[0];
		read(sock, reply+5, (int) reply[4] + 2);
	} else if (reply[3] == 0x4) {
		read(sock, reply+4, 16 + 2);
	}

	if (0) {
		int i;
		for (i=0; i<len+2; i++) {
			fprintf(stderr, "b[%d]: %d\n", i, (int) buf[i]);
		}
		for (i=0; i<len+2; i++) {
			fprintf(stderr, "r[%d]: %d\n", i, (int) reply[i]);
		}
	}
	if (reply[0] == 0x5 && reply[1] == 0x0 && reply[2] == 0x0) {
		rfbLog("SOCKS5 connect OK to %s:%d sock=%d\n", host, port, sock);
		return 1;
	} else {
		rfbLog("SOCKS5 error to %s:%d sock=%d\n", host, port, sock);
		close(sock);
		return 0;
	}
}

static int socks_proxy(char *host, int port, int sock) {
	unsigned char buf[512], tmp[2];
	char reply[16];
	int socks4a = 0, len, i, j = 0, d1, d2, d3, d4;

	memset(buf, 0, 512);

	buf[0] = 0x4;
	buf[1] = 0x1;
	buf[2] = (unsigned char) (port >> 8);
	buf[3] = (unsigned char) (port & 0xff);


	if (strlen(host) > 256)  {
		rfbLog("socks_proxy: hostname too long: %s\n", host);
		close(sock);
		return 0;
	}

	if (!strcmp(host, "localhost") || !strcmp(host, "127.0.0.1")) {
		buf[4] = 127;
		buf[5] = 0;
		buf[6] = 0;
		buf[7] = 1;
	} else if (sscanf(host, "%d.%d.%d.%d", &d1, &d2, &d3, &d4) == 4) {
		buf[4] = (unsigned char) d1;
		buf[5] = (unsigned char) d2;
		buf[6] = (unsigned char) d3;
		buf[7] = (unsigned char) d4;
	} else {
		buf[4] = 0x0;
		buf[5] = 0x0;
		buf[6] = 0x0;
		buf[7] = 0x3;
		socks4a = 1;
	}
	len = 8;

	strcat((char *)buf+8, "nobody"); 
	len += strlen("nobody") + 1;

	if (socks4a) {
		strcat((char *) buf+8+strlen("nobody") + 1, host);
		len += strlen(host) + 1;
	}

	write(sock, buf, len);

	for (i=0; i<8; i++) {
		int n;
		n = read(sock, tmp, 1);
		j++;
		if (n < 0) {
			if (errno != EINTR) {
				break;
			} else {
				i--;
				if (j > 100) {
					break;
				}
				continue;
			}
		}
		if (n == 0) {
			break;
		}
		reply[i] = tmp[0];
	}
	if (0) {
		int i;
		for (i=0; i<len; i++) {
			fprintf(stderr, "b[%d]: %d\n", i, (int) buf[i]);
		}
		for (i=0; i<8; i++) {
			fprintf(stderr, "r[%d]: %d\n", i, (int) reply[i]);
		}
	}
	if (reply[0] == 0x0 && reply[1] == 0x5a) {
		if (socks4a) {
			rfbLog("SOCKS4a connect OK to %s:%d sock=%d\n", host, port, sock);
		} else {
			rfbLog("SOCKS4  connect OK to %s:%d sock=%d\n", host, port, sock);
		}
		return 1;
	} else {
		if (socks4a) {
			rfbLog("SOCKS4a error to %s:%d sock=%d\n", host, port, sock);
		} else {
			rfbLog("SOCKS4  error to %s:%d sock=%d\n", host, port, sock);
		}
		close(sock);
		return 0;
	}
}

#define PXY_HTTP	1
#define PXY_GET		2
#define PXY_SOCKS	3
#define PXY_SOCKS5	4
#define PXY_SSH		5
#define PXY 3

static int pxy_get_sock;

static int pconnect(int psock, char *host, int port, int type, char *http_path, char *gethost, int getport) {
	char reply[4096];
	int i, ok, len;
	char *req;

	pxy_get_sock = -1;

	if (type == PXY_SOCKS) {
		return socks_proxy(host, port, psock);
	}
	if (type == PXY_SOCKS5) {
		return socks5_proxy(host, port, psock);
	}
	if (type == PXY_SSH) {
		return 1;
	}

	len = strlen("CONNECT ") + strlen(host);
	if (type == PXY_GET) {
		len += strlen(http_path) + strlen(gethost);
		len += strlen("host=") + 1 + strlen("port=") + 1 + 1;
	}
	len += 1 + 20 + strlen("HTTP/1.1\r\n") + 1;

	req = (char *)malloc(len);

	if (type == PXY_GET) {
		int noquery = 0;
		char *t = strstr(http_path, "__END__");
		if (t) {
			noquery = 1;
			*t = '\0';
		}

		if (noquery) {
			sprintf(req, "GET %s HTTP/1.1\r\n", http_path);
		} else {
			sprintf(req, "GET %shost=%s&port=%d HTTP/1.1\r\n", http_path, host, port);
		}
	} else {
		sprintf(req, "CONNECT %s:%d HTTP/1.1\r\n", host, port);
	}
	rfbLog("http proxy: %s", req);
	write(psock, req, strlen(req));

	if (type == PXY_GET) {
		char *t = "Connection: close\r\n";
		write(psock, t, strlen(t));
	}

	if (type == PXY_GET) {
		sprintf(req, "Host: %s:%d\r\n", gethost, getport);
		rfbLog("http proxy: %s", req);
		sprintf(req, "Host: %s:%d\r\n\r\n", gethost, getport);
	} else {
		sprintf(req, "Host: %s:%d\r\n", host, port);
		rfbLog("http proxy: %s", req);
		sprintf(req, "Host: %s:%d\r\n\r\n", host, port);
	}

	write(psock, req, strlen(req));

	ok = 0;
	reply[0] = '\0';

	for (i=0; i<4096; i++) {
		int n;
		req[0] = req[1] = '\0';
		n = read(psock, req, 1);
		if (n < 0) {
			if (errno != EINTR) {
				break;
			} else {
				continue;
			}
		}
		if (n == 0) {
			break;
		}
		strcat(reply, req);
		if (strstr(reply, "\r\n\r\n")) {
			if (strstr(reply, "HTTP/") == reply) {
				char *q = strchr(reply, ' ');
				if (q) {
					q++;
					if (q[0] == '2' && q[1] == '0' && q[2] == '0' && q[3] == ' ') {
						ok = 1;
					}
				}
			}
			break;
		}
	}

	if (type == PXY_GET) {
		char *t1 = strstr(reply, "VNC-IP-Port: ");
		char *t2 = strstr(reply, "VNC-Host-Port: ");
		char *s, *newhost = NULL;
		int newport = 0;
		fprintf(stderr, "%s\n", reply);
		if (t1) {
			t1 += strlen("VNC-IP-Port: ");
			s = strstr(t1, ":");
			if (s) {
				*s = '\0';
				newhost = strdup(t1);
				newport = atoi(s+1);
			}
		} else if (t2) {
			t2 += strlen("VNC-Host-Port: ");
			s = strstr(t2, ":");
			if (s) {
				*s = '\0';
				newhost = strdup(t2);
				newport = atoi(s+1);
			}
		}
		if (newhost && newport > 0) {
			rfbLog("proxy GET reconnect to: %s:%d\n", newhost, newport);
			pxy_get_sock = connect_tcp(newhost, newport);
		}
	}
	free(req);

	return ok;
}

static int proxy_connect(char *host, int port) {
	char *p, *q, *str;
	int i, n, pxy[PXY],pxy_p[PXY];
	int psock = -1;
	char *pxy_h[PXY], *pxy_g[PXY];

	if (! connect_proxy) {
		return -1;
	}
	str = strdup(connect_proxy);

	for (i=0; i<PXY; i++) {
		pxy[i] = 0;
		pxy_p[i] = 0;
		pxy_h[i] = NULL;
		pxy_g[i] = NULL;
	}

	n = 0;
	p = str;
	while (p) {
		char *hp, *c, *s = NULL;

		q = strchr(p, ',');
		if (q) {
			*q = '\0';
		}

		if (n==0) fprintf(stderr, "\n");
		rfbLog("proxy_connect[%d]: %s\n", n+1, p);

		pxy[n] = 0;
		pxy_p[n] = 0;
		pxy_h[n] = NULL;
		pxy_g[n] = NULL;

		if (strstr(p, "socks://") == p)	{
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_SOCKS;
		} else if (strstr(p, "socks4://") == p) {
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_SOCKS;
		} else if (strstr(p, "socks5://") == p) {
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_SOCKS5;
		} else if (strstr(p, "ssh://") == p) {
			if (n != 0) {
				rfbLog("ssh:// proxy must be the first one\n");
				clean_up_exit(1);
			}
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_SSH;
		} else if (strstr(p, "http://") == p) {
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_HTTP;
		} else if (strstr(p, "https://") == p) {
			hp = strstr(p, "://") + 3;
			pxy[n] = PXY_HTTP;
		} else {
			hp = p;
			pxy[n] = PXY_HTTP;
		}
		c = strstr(hp, ":");
		if (!c && pxy[n] == PXY_SSH) {
			char *hp2 = (char *) malloc(strlen(hp) + 5);
			sprintf(hp2, "%s:1", hp);
			hp = hp2;
			c = strstr(hp, ":");
		}
		if (!c) {
			pxy[n] = 0;
			if (q) {
				*q = ',';
				p = q + 1;
			} else {
				p = NULL;
			}
			continue;
		}
	
		if (pxy[n] == PXY_HTTP) {
			s = strstr(c, "/");
			if (s) {
				pxy[n] = PXY_GET;
				pxy_g[n] = strdup(s);
				*s = '\0';
			}
		}
		pxy_p[n] = atoi(c+1);

		if (pxy_p[n] <= 0) {
			pxy[n] = 0;
			pxy_p[n] = 0;
			if (q) {
				*q = ',';
				p = q + 1;
			} else {
				p = NULL;
			}
			continue;
		}
		*c = '\0';
		pxy_h[n] = strdup(hp);

		if (++n >= PXY) {
			break;
		}

		if (q) {
			*q = ',';
			p = q + 1;
		} else {
			p = NULL;
		}
	}
	free(str);

	if (!n) {
		psock = -1;
		goto pxy_clean;
	}

	if (pxy[0] == PXY_SSH) {
		int rc, len = 0;
		char *cmd, *ssh;
		int sport = find_free_port(7300, 8000);
		if (getenv("SSH")) {
			ssh = getenv("SSH");
		} else {
			ssh = "ssh";
		}
		len = 200 + strlen(ssh) + strlen(pxy_h[0]) + strlen(host);
		cmd = (char *) malloc(len);
		if (n == 1) {
			if (pxy_p[0] <= 1) {
				sprintf(cmd, "%s -f       -L '%d:%s:%d' '%s' 'sleep 20'", ssh,           sport, host, port, pxy_h[0]);
			} else {
				sprintf(cmd, "%s -f -p %d -L '%d:%s:%d' '%s' 'sleep 20'", ssh, pxy_p[0], sport, host, port, pxy_h[0]);
			}
		} else {
			if (pxy_p[0] <= 1) {
				sprintf(cmd, "%s -f       -L '%d:%s:%d' '%s' 'sleep 20'", ssh,           sport, pxy_h[1], pxy_p[1], pxy_h[0]);
			} else {
				sprintf(cmd, "%s -f -p %d -L '%d:%s:%d' '%s' 'sleep 20'", ssh, pxy_p[0], sport, pxy_h[1], pxy_p[1], pxy_h[0]);
			}
		}
		if (no_external_cmds || !cmd_ok("ssh")) {
			rfbLogEnable(1);
			rfbLog("cannot run external commands in -nocmds mode:\n");
			rfbLog("   \"%s\"\n", cmd);
			rfbLog("   exiting.\n");
			clean_up_exit(1);
		}
		close_exec_fds();
		fprintf(stderr, "\n");
		rfbLog("running: %s\n", cmd);
		rc = system(cmd);
		free(cmd);
		if (rc != 0) {
			psock = -1;
			goto pxy_clean;
		}
		psock = connect_tcp("localhost", sport);

	} else {
		psock = connect_tcp(pxy_h[0], pxy_p[0]);
	}

	if (psock < 0) {
		psock = -1;
		goto pxy_clean;
	}
	rfbLog("opened socket to proxy: %s:%d\n", pxy_h[0], pxy_p[0]);

	if (n >= 2) {
		if (! pconnect(psock, pxy_h[1], pxy_p[1], pxy[0], pxy_g[0], pxy_h[0], pxy_p[0])) {
			close(psock); psock = -1; goto pxy_clean;
		}
		if (pxy_get_sock >= 0) {close(psock); psock = pxy_get_sock;}
		
		if (n >= 3) {
			if (! pconnect(psock, pxy_h[2], pxy_p[2], pxy[1], pxy_g[1], pxy_h[1], pxy_p[1])) {
				close(psock); psock = -1; goto pxy_clean;
			}
			if (pxy_get_sock >= 0) {close(psock); psock = pxy_get_sock;}
			if (! pconnect(psock, host, port, pxy[2], pxy_g[2], pxy_h[2], pxy_p[2])) {
				close(psock); psock = -1; goto pxy_clean;
			}
			if (pxy_get_sock >= 0) {close(psock); psock = pxy_get_sock;}
			
		} else {
			if (! pconnect(psock, host, port, pxy[1], pxy_g[1], pxy_h[1], pxy_p[1])) {
				close(psock); psock = -1; goto pxy_clean;
			}
			if (pxy_get_sock >= 0) {close(psock); psock = pxy_get_sock;}
		}
	} else {
		if (! pconnect(psock, host, port, pxy[0], pxy_g[0], pxy_h[0], pxy_p[0])) {
			close(psock); psock = -1; goto pxy_clean;
		}
		if (pxy_get_sock >= 0) {close(psock); psock = pxy_get_sock;}
	}

	pxy_clean:
	for (i=0; i < PXY; i++) {
		if (pxy_h[i] != NULL) {
			free(pxy_h[i]);
		}
		if (pxy_g[i] != NULL) {
			free(pxy_g[i]);
		}
	}

	return psock;
}

char *get_repeater_string(char *str, int *len) {
	int pren, which = 0;
	int prestring_len = 0;	
	char *prestring = NULL, *ptmp = NULL;
	char *equals = strchr(str, '=');
	char *plus   = strrchr(str, '+');

	*len = 0;
	if (!plus || !equals) {
		return NULL;
	}

	*plus = '\0';
	if (strstr(str, "repeater=") == str) {
		/* ultravnc repeater http://www.uvnc.com/addons/repeater.html */
		prestring_len = 250;
		ptmp = (char *) calloc(prestring_len+1, 1);
		snprintf(ptmp, 250, "%s", str + strlen("repeater="));
		which = 1;
	} else if (strstr(str, "pre=") == str) {
		prestring_len = strlen(str + strlen("pre="));
		ptmp = (char *) calloc(prestring_len+1, 1);
		snprintf(ptmp, prestring_len+1, "%s", str + strlen("pre="));
		which = 2;
	} else if (sscanf(str, "pre%d=", &pren) == 1) {
		if (pren > 0 && pren <= 16384) {
			prestring_len = pren;
			ptmp = (char *) calloc(prestring_len+1, 1);
			snprintf(prestring, prestring_len, "%s", equals+1);
			which = 3;
		}
	}
	if (ptmp != NULL) {
		int i, k = 0;
		char *p = ptmp;
		prestring = (char *)calloc(prestring_len+1, 1);
		/* translate \n to newline, etc. */
		for (i=0; i < prestring_len; i++) {
			if (i < prestring_len-1 && *(p+i) == '\\') {
				if (*(p+i+1) == 'r') {
					prestring[k++] = '\r'; i++;
				} else if (*(p+i+1) == 'n') {
					prestring[k++] = '\n'; i++;
				} else if (*(p+i+1) == 't') {
					prestring[k++] = '\t'; i++;
				} else if (*(p+i+1) == 'a') {
					prestring[k++] = '\a'; i++;
				} else if (*(p+i+1) == 'b') {
					prestring[k++] = '\b'; i++;
				} else if (*(p+i+1) == 'v') {
					prestring[k++] = '\v'; i++;
				} else if (*(p+i+1) == 'f') {
					prestring[k++] = '\f'; i++;
				} else if (*(p+i+1) == '\\') {
					prestring[k++] = '\\'; i++;
				} else if (*(p+i+1) == 'c') {
					prestring[k++] = ','; i++;
				} else {
					prestring[k++] = *(p+i);
				}
			} else {
				prestring[k++] = *(p+i);
			}
		}
		if (which == 2) {
			prestring_len = k;
		}
		if (!quiet) {
			rfbLog("-connect prestring: '%s'\n", prestring);
		}
		free(ptmp);
	}
	*plus = '+';

	*len = prestring_len;
	return prestring;
}

#ifndef USE_TIMEOUT_INTERRUPT
#define USE_TIMEOUT_INTERRUPT 0
#endif

static void reverse_connect_timeout (int sig) {
	rfbLog("sig: %d, reverse_connect_timeout.\n", sig);
#if USE_TIMEOUT_INTERRUPT
	rfbLog("reverse_connect_timeout proceeding assuming connect(2) interrupt.\n");
#else
	clean_up_exit(0);
#endif
}


/*
 * Do a reverse connect for a single "host" or "host:port"
 */

static int do_reverse_connect(char *str_in) {
	rfbClientPtr cl;
	char *host, *p, *str = str_in, *s = NULL;
	char *prestring = NULL;
	int prestring_len = 0;
	int rport = 5500, len = strlen(str);
	int set_alarm = 0;

	if (len < 1) {
		return 0;
	}
	if (len > 1024) {
		rfbLog("reverse_connect: string too long: %d bytes\n", len);
		return 0;
	}
	if (!screen) {
		rfbLog("reverse_connect: screen not setup yet.\n");
		return 0;
	}
	if (unixpw_in_progress) return 0;

	/* look for repeater pre-string */
	if (strchr(str, '=') && strrchr(str, '+')
	    && (strstr(str, "pre") == str || strstr(str, "repeater=") == str)) {
		prestring = get_repeater_string(str, &prestring_len);
		str = strrchr(str, '+') + 1;
	} else if (strrchr(str, '+') && strstr(str, "repeater://") == str) {
		/* repeater://host:port+string */
		/*   repeater=string+host:port */
		char *plus = strrchr(str, '+');
		str = (char *) malloc(strlen(str_in)+1);
		s = str;
		*plus = '\0';
		sprintf(str, "repeater=%s+%s", plus+1, str_in + strlen("repeater://"));
		prestring = get_repeater_string(str, &prestring_len);
		str = strrchr(str, '+') + 1;
		*plus = '+';
	}

	/* copy in to host */
	host = (char *) malloc(len+1);
	if (! host) {
		rfbLog("reverse_connect: could not malloc string %d\n", len);
		return 0;
	}
	strncpy(host, str, len);
	host[len] = '\0';

	/* extract port, if any */
	if ((p = strrchr(host, ':')) != NULL) {
		rport = atoi(p+1);
		if (rport < 0) {
			rport = -rport;
		} else if (rport < 20) {
			rport = 5500 + rport;
		}
		*p = '\0';
	}

	if (ipv6_client_ip_str) {
		free(ipv6_client_ip_str);
		ipv6_client_ip_str = NULL;
	}

	if (use_openssl) {
		int vncsock;
		if (connect_proxy) {
			vncsock = proxy_connect(host, rport);
		} else {
			vncsock = connect_tcp(host, rport);
		}
		if (vncsock < 0) {
			rfbLog("reverse_connect: failed to connect to: %s\n", str);
			return 0;
		}
		if (prestring != NULL) {
			write(vncsock, prestring, prestring_len);
			free(prestring);
		}
/* XXX use header */
#define OPENSSL_REVERSE 6
		if (!getenv("X11VNC_DISABLE_SSL_CLIENT_MODE")) {
			openssl_init(1);
		}

		if (first_conn_timeout > 0) {
			set_alarm = 1;
			signal(SIGALRM, reverse_connect_timeout);
#if USE_TIMEOUT_INTERRUPT
			siginterrupt(SIGALRM, 1);
#endif
			rfbLog("reverse_connect: using alarm() timeout of %d seconds.\n", first_conn_timeout);
			alarm(first_conn_timeout);
		}
		accept_openssl(OPENSSL_REVERSE, vncsock);
		if (set_alarm) {alarm(0); signal(SIGALRM, SIG_DFL);}

		openssl_init(0);
		free(host);
		return 1;
	}

	if (use_stunnel) {
		if(strcmp(host, "localhost") && strcmp(host, "127.0.0.1")) {
			if (!getenv("STUNNEL_DISABLE_LOCALHOST")) {
				rfbLog("reverse_connect: error host not localhost in -stunnel mode.\n");
				return 0;
			}
		}
	}

	if (unixpw) {
		int is_localhost = 0, user_disabled_it = 0;

		if(!strcmp(host, "localhost") || !strcmp(host, "127.0.0.1")) {
			is_localhost = 1;
		}
		if (getenv("UNIXPW_DISABLE_LOCALHOST")) {
			user_disabled_it = 1;
		}

		if (! is_localhost) {
			if (user_disabled_it) {
				rfbLog("reverse_connect: warning disabling localhost constraint in -unixpw\n");
			} else {
				rfbLog("reverse_connect: error not localhost in -unixpw\n");
				return 0;
			}
		}
	}

	if (first_conn_timeout > 0) {
		set_alarm = 1;
		signal(SIGALRM, reverse_connect_timeout);
#if USE_TIMEOUT_INTERRUPT
		siginterrupt(SIGALRM, 1);
#endif
		rfbLog("reverse_connect: using alarm() timeout of %d seconds.\n", first_conn_timeout);
		alarm(first_conn_timeout);
	}

	if (connect_proxy != NULL) {
		int sock = proxy_connect(host, rport);
		if (set_alarm) {alarm(0); signal(SIGALRM, SIG_DFL);}
		if (sock >= 0) {
			if (prestring != NULL) {
				write(sock, prestring, prestring_len);
				free(prestring);
			}
			cl = create_new_client(sock, 1);
		} else {
			return 0;
		}
	} else if (prestring != NULL) {
		int sock = connect_tcp(host, rport);
		if (set_alarm) {alarm(0); signal(SIGALRM, SIG_DFL);}
		if (sock >= 0) {
			write(sock, prestring, prestring_len);
			free(prestring);
			cl = create_new_client(sock, 1);
		} else {
			return 0;
		}
	} else {
		cl = rfbReverseConnection(screen, host, rport);
		if (cl == NULL) {
			int sock = connect_tcp(host, rport);
			if (sock >= 0) {
				cl = create_new_client(sock, 1);
			}
		}
		if (set_alarm) {alarm(0); signal(SIGALRM, SIG_DFL);}
		if (cl != NULL && use_threads) {
			cl->onHold = FALSE;
			rfbStartOnHoldClient(cl);
		}
	}

	free(host);

	if (ipv6_client_ip_str) {
		free(ipv6_client_ip_str);
		ipv6_client_ip_str = NULL;
	}


	if (cl == NULL) {
		if (quiet && connect_or_exit) {
			rfbLogEnable(1);
		}
		rfbLog("reverse_connect: %s failed\n", str);
		return 0;
	} else {
		rfbLog("reverse_connect: %s/%s OK\n", str, cl->host);
		/* let's see if anyone complains: */
		if (! getenv("X11VNC_REVERSE_CONNECTION_NO_AUTH")) {
			rfbLog("reverse_connect: turning on auth for %s\n",
			    cl->host);
			cl->reverseConnection = FALSE;
		}
		return 1;
	}
}

/*
 * Break up comma separated list of hosts and call do_reverse_connect()
 */
void reverse_connect(char *str) {
	char *p, *tmp;
	int sleep_between_host = 300;
	int sleep_min = 1500, sleep_max = 4500, n_max = 5;
	int n, tot, t, dt = 100, cnt = 0;
	int nclients0 = client_count;
	int lcnt, j;
	char **list;
	int do_appshare = 0;

	if (!getenv("X11VNC_REVERSE_USE_OLD_SLEEP")) {
		sleep_min = 500;
		sleep_max = 2500;
	}

	if (unixpw_in_progress) return;

	tmp = strdup(str);

	list = (char **) calloc( (strlen(tmp)+2) * sizeof (char *), 1);
	lcnt = 0;

	p = strtok(tmp, ", \t\r\n");
	while (p) {
		list[lcnt++] = strdup(p);
		p = strtok(NULL, ", \t\r\n");
	}
	free(tmp);

	if (subwin && getenv("X11VNC_APPSHARE_ACTIVE")) {
		do_appshare = 1;
		sleep_between_host = 0;	/* too agressive??? */
	}
	if (getenv("X11VNC_REVERSE_SLEEP_BETWEEN_HOST")) {
		sleep_between_host = atoi(getenv("X11VNC_REVERSE_SLEEP_BETWEEN_HOST"));
	}

	if (do_appshare) {
		if (screen && dpy) {
			char *s = choose_title(DisplayString(dpy));

			/* mutex */
			screen->desktopName = s;
			if (rfb_desktop_name) {
				free(rfb_desktop_name);
			}
			rfb_desktop_name = strdup(s);
		}
	}

	for (j = 0; j < lcnt; j++) {
		p = list[j];
		
		if ((n = do_reverse_connect(p)) != 0) {
			int i;
			progress_client();
			for (i=0; i < 3; i++) {
				rfbPE(-1);
			}
		}
		cnt += n;
		if (list[j+1] != NULL) {
			t = 0;
			while (t < sleep_between_host) {
				double t1, t2;
				int i;
				t1 = dnow();
				for (i=0; i < 8; i++) {
					rfbPE(-1);
					if (do_appshare && t == 0) {
						rfbPE(-1);
					}
				}
				t2 = dnow();
				t += (int) (1000 * (t2 - t1));
				if (t >= sleep_between_host) {
					break;
				}
				usleep(dt * 1000);
				t += dt;
			}
		}
	}

	for (j = 0; j < lcnt; j++) {
		p = list[j];
		if (p) free(p);
	}
	free(list);

	if (cnt == 0) {
		if (connect_or_exit) {
			rfbLogEnable(1);
			rfbLog("exiting under -connect_or_exit\n");
			if (gui_pid > 0) {
				rfbLog("killing gui_pid %d\n", gui_pid);
				kill(gui_pid, SIGTERM);
			}
			clean_up_exit(1);
		}
		if (xrandr || xrandr_maybe) {
			check_xrandr_event("reverse_connect1");
		}
		return;
	}

	/*
	 * XXX: we need to process some of the initial handshaking
	 * events, otherwise the client can get messed up (why??) 
	 * so we send rfbProcessEvents() all over the place.
	 *
	 * How much is this still needed?
	 */

	n = cnt;
	if (n >= n_max) {
		n = n_max; 
	}
	t = sleep_max - sleep_min;
	tot = sleep_min + ((n-1) * t) / (n_max-1);

	if (do_appshare) {
		tot /= 3;
		if (tot < dt) {
			tot = dt;
		}
		tot = 0;	/* too agressive??? */
	}

	if (getenv("X11VNC_REVERSE_SLEEP_MAX")) {
		tot = atoi(getenv("X11VNC_REVERSE_SLEEP_MAX"));
	}

	t = 0;
	while (t < tot) {
		int i;
		double t1, t2;
		t1 = dnow();
		for (i=0; i < 8; i++) {
			rfbPE(-1);
			if (t == 0) rfbPE(-1);
		}
		t2 = dnow();
		t += (int) (1000 * (t2 - t1));
		if (t >= tot) {
			break;
		}
		usleep(dt * 1000);
		t += dt;
	}
	if (connect_or_exit) {
		if (client_count <= nclients0)  {
			for (t = 0; t < 10; t++) {
				int i;
				for (i=0; i < 3; i++) {
					rfbPE(-1);
				}
				usleep(100 * 1000);
			}
		}
		if (client_count <= nclients0)  {
			rfbLogEnable(1);
			rfbLog("exiting under -connect_or_exit\n");
			if (gui_pid > 0) {
				rfbLog("killing gui_pid %d\n", gui_pid);
				kill(gui_pid, SIGTERM);
			}
			clean_up_exit(1);
		}
	}
	if (xrandr || xrandr_maybe) {
		check_xrandr_event("reverse_connect2");
	}
}

/*
 * Routines for monitoring the VNC_CONNECT and X11VNC_REMOTE properties
 * for changes.  The vncconnect(1) will set it on our X display.
 */
void set_vnc_connect_prop(char *str) {
	RAWFB_RET_VOID
#if !NO_X11
	if (vnc_connect_prop == None) return;
	XChangeProperty(dpy, rootwin, vnc_connect_prop, XA_STRING, 8,
	    PropModeReplace, (unsigned char *)str, strlen(str));
#else
	if (!str) {}
#endif	/* NO_X11 */
}

void set_x11vnc_remote_prop(char *str) {
	RAWFB_RET_VOID
#if !NO_X11
	if (x11vnc_remote_prop == None) return;
	XChangeProperty(dpy, rootwin, x11vnc_remote_prop, XA_STRING, 8,
	    PropModeReplace, (unsigned char *)str, strlen(str));
#else
	if (!str) {}
#endif	/* NO_X11 */
}

void read_vnc_connect_prop(int nomsg) {
#if NO_X11
	RAWFB_RET_VOID
	if (!nomsg) {}
	return;
#else
	Atom type;
	int format, slen, dlen;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;
	int db = 1;

	vnc_connect_str[0] = '\0';
	slen = 0;

	if (! vnc_connect || vnc_connect_prop == None) {
		/* not active or problem with VNC_CONNECT atom */
		return;
	}
	RAWFB_RET_VOID

	/* read the property value into vnc_connect_str: */
	do {
		if (XGetWindowProperty(dpy, DefaultRootWindow(dpy),
		    vnc_connect_prop, nitems/4, VNC_CONNECT_MAX/16, False,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > VNC_CONNECT_MAX) {
				/* too big */
				rfbLog("warning: truncating large VNC_CONNECT"
				   " string > %d bytes.\n", VNC_CONNECT_MAX);
				XFree_wr(data);
				break;
			}
			memcpy(vnc_connect_str+slen, data, dlen);
			slen += dlen;
			vnc_connect_str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);

	vnc_connect_str[VNC_CONNECT_MAX] = '\0';
	if (! db || nomsg) {
		;
	} else {
		rfbLog("read VNC_CONNECT: %s\n", vnc_connect_str);
	}
#endif	/* NO_X11 */
}

void read_x11vnc_remote_prop(int nomsg) {
#if NO_X11
	RAWFB_RET_VOID
	if (!nomsg) {}
	return;
#else
	Atom type;
	int format, slen, dlen;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;
	int db = 1;

	x11vnc_remote_str[0] = '\0';
	slen = 0;

	if (! vnc_connect || x11vnc_remote_prop == None) {
		/* not active or problem with X11VNC_REMOTE atom */
		return;
	}
	RAWFB_RET_VOID

	/* read the property value into x11vnc_remote_str: */
	do {
		if (XGetWindowProperty(dpy, DefaultRootWindow(dpy),
		    x11vnc_remote_prop, nitems/4, X11VNC_REMOTE_MAX/16, False,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > X11VNC_REMOTE_MAX) {
				/* too big */
				rfbLog("warning: truncating large X11VNC_REMOTE"
				   " string > %d bytes.\n", X11VNC_REMOTE_MAX);
				XFree_wr(data);
				break;
			}
			memcpy(x11vnc_remote_str+slen, data, dlen);
			slen += dlen;
			x11vnc_remote_str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);

	x11vnc_remote_str[X11VNC_REMOTE_MAX] = '\0';
	if (! db || nomsg) {
		;
	} else if (strstr(x11vnc_remote_str, "ans=stop:N/A,ans=quit:N/A,ans=")) {
		;
	} else if (strstr(x11vnc_remote_str, "qry=stop,quit,exit")) {
		;
	} else if (strstr(x11vnc_remote_str, "ack=") == x11vnc_remote_str) {
		;
	} else if (quiet && strstr(x11vnc_remote_str, "qry=ping") ==
	    x11vnc_remote_str) {
		;
	} else if (strstr(x11vnc_remote_str, "cmd=") &&
	    strstr(x11vnc_remote_str, "passwd")) {
		rfbLog("read X11VNC_REMOTE: *\n");
	} else if (strlen(x11vnc_remote_str) > 36) {
		char trim[100]; 
		trim[0] = '\0';
		strncat(trim, x11vnc_remote_str, 36);
		rfbLog("read X11VNC_REMOTE: %s ...\n", trim);
		
	} else {
		rfbLog("read X11VNC_REMOTE: %s\n", x11vnc_remote_str);
	}
#endif	/* NO_X11 */
}

extern int rc_npieces;

void grab_state(int *ptr_grabbed, int *kbd_grabbed) {
	int rcp, rck;
	double t0, t1;
	double ta, tb, tc;
	*ptr_grabbed = -1;
	*kbd_grabbed = -1;

	if (!dpy) {
		return;
	}
	*ptr_grabbed = 0;
	*kbd_grabbed = 0;

#if !NO_X11
	X_LOCK;

	XSync(dpy, False);

	ta = t0 = dnow();

	rcp = XGrabPointer(dpy, window, False, 0, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XUngrabPointer(dpy, CurrentTime);

	tb = dnow();
	
	rck = XGrabKeyboard(dpy, window, False, GrabModeAsync, GrabModeAsync, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	tc = dnow();

	XSync(dpy, False);

	t1 = dnow();

	X_UNLOCK;
	if (rcp == AlreadyGrabbed || rcp == GrabFrozen) {
		*ptr_grabbed = 1;
	}
	if (rck == AlreadyGrabbed || rck == GrabFrozen) {
		*kbd_grabbed = 1;
	}
	if (rc_npieces < 10) {
		rfbLog("grab_state: checked %d,%d in %.6f sec (%.6f %.6f)\n",
		    *ptr_grabbed, *kbd_grabbed, t1-t0, tb-ta, tc-tb);
	}
#endif
}

static void pmove(int x, int y) {
	if (x < 0 || y < 0) {
		rfbLog("pmove: skipping negative x or y: %d %d\n", x, y);
		return;
	}
	rfbLog("pmove: x y: %d %d\n", x, y);
	pointer_event(0, x, y, NULL);
	X_LOCK;
	XFlush_wr(dpy);
	X_UNLOCK;
}


char *bcx_xattach(char *str, int *pg_init, int *kg_init) {
	int grab_check = 1;
	int shift = 20;
	int final_x = 30, final_y = 30;
	int extra_x = -1, extra_y = -1;
	int t1, t2, dt = 40 * 1000;
	int ifneeded = 0;
	char *dir = "none", *flip = "none", *q;
	int pg1, kg1, pg2, kg2;
	char _bcx_res[128];
	
	/* str:[up,down,left,right]+nograbcheck+shift=n+final=x+y+extra_move=x+y+[master_to_slave,slave_to_master,M2S,S2M]+dt=n+retry=n+ifneeded */

	if (strstr(str, "up")) {
		dir = "up";
	} else if (strstr(str, "down")) {
		dir = "down";
	} else if (strstr(str, "left")) {
		dir = "left";
	} else if (strstr(str, "right")) {
		dir = "right";
	} else {
		return strdup("FAIL,NO_DIRECTION_SPECIFIED");
	}

	if (strstr(str, "master_to_slave") || strstr(str, "M2S")) {
		flip = "M2S";
	} else if (strstr(str, "slave_to_master") || strstr(str, "S2M")) {
		flip = "S2M";
	} else {
		return strdup("FAIL,NO_MODE_CHANGE_SPECIFIED");
	}

	if (strstr(str, "nograbcheck")) {
		grab_check = 0;
	}
	if (strstr(str, "ifneeded")) {
		ifneeded = 1;
	}
	q = strstr(str, "shift=");
	if (q && sscanf(q, "shift=%d", &t1) == 1) {
		shift = t1;
	}
	q = strstr(str, "final=");
	if (q && sscanf(q, "final=%d+%d", &t1, &t2) == 2) {
		final_x = t1;
		final_y = t2;
	}
	q = strstr(str, "extra_move=");
	if (q && sscanf(q, "extra_move=%d+%d", &t1, &t2) == 2) {
		extra_x = t1;
		extra_y = t2;
	}
	q = strstr(str, "dt=");
	if (q && sscanf(q, "dt=%d", &t1) == 1) {
		dt = t1 * 1000;
	}

	if (grab_check) {
		int read_init = 0;

		if (*pg_init >=0 && *kg_init >=0)  {
			pg1 = *pg_init;
			kg1 = *kg_init;
			read_init = 1;
		} else {
			grab_state(&pg1, &kg1);
			read_init = 0;
		}

		if (!strcmp(flip, "M2S")) {
			if (ifneeded && pg1 == 1 && kg1 == 1) {
				rfbLog("bcx_xattach: M2S grab state is already what we want, skipping moves:  %d,%d\n", pg1, kg1);
				return strdup("DONE,GRAB_OK");
			}
		} else if (!strcmp(flip, "S2M")) {
			if (ifneeded && pg1 == 0 && kg1 == 0) {
				rfbLog("bcx_xattach: S2M grab state is already what we want, skipping moves:  %d,%d\n", pg1, kg1);
				return strdup("DONE,GRAB_OK");
			}
		}

		if (read_init) {
			;
		} else if (!strcmp(flip, "M2S")) {
			if (pg1 != 0 || kg1 != 0) {
				rfbLog("bcx_xattach: M2S init grab state incorrect:  %d,%d\n", pg1, kg1);
				usleep(2*dt);
				grab_state(&pg1, &kg1);
				rfbLog("bcx_xattach: slept and retried, grab is now: %d,%d\n", pg1, kg1);
			}
		} else if (!strcmp(flip, "S2M")) {
			if (pg1 != 1 || kg1 != 1) {
				rfbLog("bcx_xattach: S2M init grab state incorrect:  %d,%d\n", pg1, kg1);
				usleep(2*dt);
				grab_state(&pg1, &kg1);
				rfbLog("bcx_xattach: slept and retried, grab is now: %d,%d\n", pg1, kg1);
			}
		}
		if (!read_init) {
			*pg_init = pg1;
			*kg_init = kg1;
		}
	}

	/*
	 * A guide for BARCO xattach:
	 *
	 *   For -cursor_rule 'b(0):%:t(1),t(1):%:b(0)'
	 *	down+M2S  up+S2M
	 *   For -cursor_rule 'r(0):%:l(1),l(1):%:r(0)'
	 *	right+M2S  left+S2M
	 *
	 *   For -cursor_rule 't(0):%:b(1),b(1):%:t(0)'
	 *	up+M2S  down+S2M
	 *   For -cursor_rule 'l(0):%:r(1),r(1):%:l(0)'
	 *	left+M2S  right+S2M
	 *   For -cursor_rule 'l(0):%:r(1),r(1):%:l(0),r(0):%:l(1),l(1):%:r(0)'
	 *	left+M2S  right+S2M  (we used to do both 'right')
	 */

	if (!strcmp(flip, "M2S")) {
		if (!strcmp(dir, "up")) {
			pmove(shift, 0);		/* go to top edge */
			usleep(dt);
			pmove(shift+1, 0);		/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "down")) {
			pmove(shift,   dpy_y-1);	/* go to bottom edge */
			usleep(dt);
			pmove(shift+1, dpy_y-1);	/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "left")) {
			pmove(0, shift);		/* go to left edge */
			usleep(dt);
			pmove(0, shift+1);		/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "right")) {
			pmove(dpy_x-1, shift);		/* go to right edge */
			usleep(dt);
			pmove(dpy_x-1, shift+1);	/* move 1 for Motion Notify  */
		}
	} else if (!strcmp(flip, "S2M")) {
		int dts = dt/2;
		if (!strcmp(dir, "up")) {
			pmove(shift, 2);		/* Approach top edge in 3 moves.  1st move */
			usleep(dts);
			pmove(shift, 1);		/* 2nd move */
			usleep(dts);
			pmove(shift, 0);		/* 3rd move */
			usleep(dts);
			pmove(shift+1, 0);		/* move 1 for MotionNotify */
			usleep(dts);
			pmove(shift+1, dpy_y-2);	/* go to height-2 for extra pixel (slave y now == 0?) */
			usleep(dts);
			pmove(shift,   dpy_y-2);	/* move 1 for MotionNotify */
			usleep(dts);
			pmove(shift, 1);		/* go to 1 to be sure slave y == 0 */
			usleep(dts);
			pmove(shift+1, 1);		/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "down")) {
			pmove(shift,   dpy_y-3);	/* Approach bottom edge in 3 moves.  1st move */
			usleep(dts);
			pmove(shift,   dpy_y-2);	/* 2nd move */
			usleep(dts);
			pmove(shift,   dpy_y-1);	/* 3rd move */
			usleep(dts);
			pmove(shift+1, dpy_y-1);	/* move 1 for MotionNotify */
			usleep(dts);
			pmove(shift+1, 1);		/* go to 1 for extra pixel (slave y now == dpy_y-1?) */
			usleep(dts);
			pmove(shift, 1);		/* move 1 for MotionNotify */
			usleep(dts);
			pmove(shift,   dpy_y-2);	/* go to dpy_y-2 to be sure slave y == dpy_y-1 */
			usleep(dts);
			pmove(shift+1, dpy_y-2);	/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "left")) {
			pmove(2, shift);		/* Approach left edge in 3 moves.  1st move */
			usleep(dts);
			pmove(1, shift);		/* 2nd move */
			usleep(dts);
			pmove(0, shift);		/* 3rd move */
			usleep(dts);
			pmove(0, shift+1);		/* move 1 for MotionNotify */
			usleep(dts);
			pmove(dpy_x-2, shift+1);	/* go to width-2 for extra pixel (slave x now == 0?) */
			usleep(dts);
			pmove(dpy_x-2, shift);		/* move 1 for MotionNotify */
			usleep(dts);
			pmove(1, shift);		/* go to 1 to be sure slave x == 0 */
			usleep(dts);
			pmove(1, shift+1);		/* move 1 for MotionNotify */
		} else if (!strcmp(dir, "right")) {
			pmove(dpy_x-3, shift);		/* Approach right edge in 3 moves.  1st move */
			usleep(dts);
			pmove(dpy_x-2, shift);		/* 2nd move */
			usleep(dts);
			pmove(dpy_x-1, shift);		/* 3rd move */
			usleep(dts);
			pmove(dpy_x-1, shift+1);	/* move 1 for MotionNotify */
			usleep(dts);
			pmove(1, shift+1);		/* go to 1 to extra pixel (slave x now == dpy_x-1?) */
			usleep(dts);
			pmove(1, shift);		/* move 1 for MotionNotify */
			usleep(dts);
			pmove(dpy_x-2, shift);		/* go to dpy_x-2 to be sure slave x == dpy_x-1 */
			usleep(dts);
			pmove(dpy_x-2, shift+1);	/* move 1 for MotionNotify */
		}
	}

	usleep(dt);
	pmove(final_x, final_y);
	usleep(dt);

	if (extra_x >= 0 && extra_y >= 0) {
		pmove(extra_x, extra_y);
		usleep(dt);
	}

	strcpy(_bcx_res, "DONE");

	if (grab_check) {
		char st[64];

		usleep(3*dt);
		grab_state(&pg2, &kg2);

		if (!strcmp(flip, "M2S")) {
			if (pg2 != 1 || kg2 != 1) {
				rfbLog("bcx_xattach: M2S fini grab state incorrect:  %d,%d\n", pg2, kg2);
				usleep(2*dt);
				grab_state(&pg2, &kg2);
				rfbLog("bcx_xattach: slept and retried, grab is now: %d,%d\n", pg2, kg2);
			}
		} else if (!strcmp(flip, "S2M")) {
			if (pg2 != 0 || kg2 != 0) {
				rfbLog("bcx_xattach: S2M fini grab state incorrect:  %d,%d\n", pg2, kg2);
				usleep(2*dt);
				grab_state(&pg2, &kg2);
				rfbLog("bcx_xattach: slept and retried, grab is now: %d,%d\n", pg2, kg2);
			}
		}

		sprintf(st, ":%d,%d-%d,%d", pg1, kg1, pg2, kg2);

		if (getenv("GRAB_CHECK_LOOP")) {
			int i, n = atoi(getenv("GRAB_CHECK_LOOP"));
			rfbLog("grab st: %s\n", st);
			for (i=0; i < n; i++) {
				usleep(dt);
				grab_state(&pg2, &kg2);
				sprintf(st, ":%d,%d-%d,%d", pg1, kg1, pg2, kg2);
				rfbLog("grab st: %s\n", st);
			}
		}

		if (!strcmp(flip, "M2S")) {
			if (pg1 == 0 && kg1 == 0 && pg2 == 1 && kg2 == 1) {
				strcat(_bcx_res, ",GRAB_OK");
			} else {
				rfbLog("bcx_xattach: M2S grab state incorrect: %d,%d -> %d,%d\n", pg1, kg1, pg2, kg2);
				strcat(_bcx_res, ",GRAB_FAIL");
				if (pg2 == 1 && kg2 == 1) {
					strcat(_bcx_res, "_INIT");
				} else if (pg1 == 0 && kg1 == 0) {
					strcat(_bcx_res, "_FINAL");
				}
				strcat(_bcx_res, st);
			}
		} else if (!strcmp(flip, "S2M")) {
			if (pg1 == 1 && kg1 == 1 && pg2 == 0 && kg2 == 0) {
				strcat(_bcx_res, ",GRAB_OK");
			} else {
				rfbLog("bcx_xattach: S2M grab state incorrect: %d,%d -> %d,%d\n", pg1, kg1, pg2, kg2);
				strcat(_bcx_res, ",GRAB_FAIL");
				if (pg2 == 0 && kg2 == 0) {
					strcat(_bcx_res, "_INIT");
				} else if (pg1 == 1 && kg1 == 1) {
					strcat(_bcx_res, "_FINAL");
				}
				strcat(_bcx_res, st);
			}
		}
	}
	return strdup(_bcx_res);
}

int set_xprop(char *prop, Window win, char *value) {
	int rc = -1;
#if !NO_X11
	Atom aprop;

	RAWFB_RET(rc)

	if (!prop || !value) {
		return rc;
	}
	if (win == None) {
		win = rootwin;
	}
	aprop = XInternAtom(dpy, prop, False);
	if (aprop == None) {
		return rc;
	}
	rc = XChangeProperty(dpy, win, aprop, XA_STRING, 8,
	    PropModeReplace, (unsigned char *)value, strlen(value));
	return rc;
#else
	RAWFB_RET(rc)
	if (!prop || !win || !value) {}
	return rc;
#endif	/* NO_X11 */
}

char *get_xprop(char *prop, Window win) {
#if NO_X11
	RAWFB_RET(NULL)
	if (!prop || !win) {}
	return NULL;
#else
	Atom type, aprop;
	int format, slen, dlen;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;
	char get_str[VNC_CONNECT_MAX+1];

	RAWFB_RET(NULL)

	if (prop == NULL || !strcmp(prop, "")) {
		return NULL;
	}
	if (win == None) {
		win = rootwin;
	}
	aprop = XInternAtom(dpy, prop, True);
	if (aprop == None) {
		return NULL;
	}

	get_str[0] = '\0';
	slen = 0;

	/* read the property value into get_str: */
	do {
		if (XGetWindowProperty(dpy, win, aprop, nitems/4,
		    VNC_CONNECT_MAX/16, False, AnyPropertyType, &type,
		    &format, &nitems, &bytes_after, &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > VNC_CONNECT_MAX) {
				/* too big */
				rfbLog("get_xprop: warning: truncating large '%s'"
				   " string > %d bytes.\n", prop, VNC_CONNECT_MAX);
				XFree_wr(data);
				break;
			}
			memcpy(get_str+slen, data, dlen);
			slen += dlen;
			get_str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);

	get_str[VNC_CONNECT_MAX] = '\0';
	rfbLog("get_prop: read: '%s' = '%s'\n", prop, get_str);

	return strdup(get_str);
#endif	/* NO_X11 */
}

static char _win_fmt[1000];

static char *win_fmt(Window win, XWindowAttributes a) {
	memset(_win_fmt, 0, sizeof(_win_fmt));
	sprintf(_win_fmt, "0x%lx:%dx%dx%d+%d+%d-map:%d-bw:%d-cl:%d-vis:%d-bs:%d/%d",
	    win, a.width, a.height, a.depth, a.x, a.y, a.map_state, a.border_width, a.class,
	    (int) ((a.visual)->visualid), a.backing_store, a.save_under);
	return _win_fmt;
}

char *wininfo(Window win, int show_children) {
#if NO_X11
	RAWFB_RET(NULL)
	if (!win || !show_children) {}
	return NULL;
#else
	XWindowAttributes attr;
	int n, size = X11VNC_REMOTE_MAX;
	char get_str[X11VNC_REMOTE_MAX+1];
	unsigned int nchildren;
	Window rr, pr, *children; 

	RAWFB_RET(NULL)

	if (win == None) {
		return strdup("None");
	}

	X_LOCK;
	if (!valid_window(win, &attr, 1)) {
		X_UNLOCK;
		return strdup("Invalid");
	}
	get_str[0] = '\0';

	if (show_children) {
		XQueryTree_wr(dpy, win, &rr, &pr, &children, &nchildren);
	} else {
		nchildren = 1;
		children = (Window *) calloc(2 * sizeof(Window), 1);
		children[0] = win;
	}
	for (n=0; n < (int) nchildren; n++) {
		char tmp[32];
		char *str = "Invalid";
		Window w = children[n];
		if (valid_window(w, &attr, 1)) {
			if (!show_children) {
				str = win_fmt(w, attr);
			} else {
				sprintf(tmp, "0x%lx", w);
				str = tmp;
			}
		}
		if ((int) (strlen(get_str) + 1 + strlen(str)) >= size) {
			break;
		}
		if (n > 0) {
			strcat(get_str, ",");
		}
		strcat(get_str, str);
	}
	get_str[size] = '\0';
	if (!show_children) {
		free(children);
	} else if (nchildren) {
		XFree_wr(children);
	}
	rfbLog("wininfo computed: %s\n", get_str);
	X_UNLOCK;

	return strdup(get_str);
#endif	/* NO_X11 */
}

/*
 * check if client_connect has been set, if so make the reverse connections.
 */
static void send_client_connect(void) {
	if (client_connect != NULL) {
		char *str = client_connect;
		if (strstr(str, "cmd=") == str || strstr(str, "qry=") == str) {
			process_remote_cmd(client_connect, 0);
		} else if (strstr(str, "ans=") == str
		    || strstr(str, "aro=") == str) {
			;
		} else if (strstr(str, "ack=") == str) {
			;
		} else {
			reverse_connect(client_connect);
		}
		free(client_connect);
		client_connect = NULL;
	}
}

/*
 * monitor the various input methods
 */
void check_connect_inputs(void) {

	if (unixpw_in_progress) return;

	/* flush any already set: */
	send_client_connect();

	/* connect file: */
	if (client_connect_file != NULL) {
		check_connect_file(client_connect_file);		
	}
	send_client_connect();

	/* VNC_CONNECT property (vncconnect program) */
	if (vnc_connect && *vnc_connect_str != '\0') {
		client_connect = strdup(vnc_connect_str);
		vnc_connect_str[0] = '\0';
	}
	send_client_connect();

	/* X11VNC_REMOTE property */
	if (vnc_connect && *x11vnc_remote_str != '\0') {
		client_connect = strdup(x11vnc_remote_str);
		x11vnc_remote_str[0] = '\0';
	}
	send_client_connect();
}

void check_gui_inputs(void) {
	int i, gnmax = 0, n = 0, nfds;
	int socks[ICON_MODE_SOCKS];
	fd_set fds;
	struct timeval tv;
	char buf[X11VNC_REMOTE_MAX+1];
	ssize_t nbytes;

	if (unixpw_in_progress) return;

	for (i=0; i<ICON_MODE_SOCKS; i++) {
		if (icon_mode_socks[i] >= 0) {
			socks[n++] = i;
			if (icon_mode_socks[i] > gnmax) {
				gnmax = icon_mode_socks[i];
			}
		}
	}

	if (! n) {
		return;
	}

	FD_ZERO(&fds);
	for (i=0; i<n; i++) {
		FD_SET(icon_mode_socks[socks[i]], &fds);
	}
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	nfds = select(gnmax+1, &fds, NULL, NULL, &tv);
	if (nfds <= 0) {
		return;
	}
	
	for (i=0; i<n; i++) {
		int k, fd = icon_mode_socks[socks[i]];
		char *p;
		char **list;
		int lind;

		if (! FD_ISSET(fd, &fds)) {
			continue;
		}
		for (k=0; k<=X11VNC_REMOTE_MAX; k++) {
			buf[k] = '\0';
		}
		nbytes = read(fd, buf, X11VNC_REMOTE_MAX);
		if (nbytes <= 0) {
			close(fd);
			icon_mode_socks[socks[i]] = -1;
			continue;
		}

		list = (char **) calloc((strlen(buf)+2) * sizeof(char *), 1);

		lind = 0;
		p = strtok(buf, "\r\n");
		while (p) {
			list[lind++] = strdup(p);
			p = strtok(NULL, "\r\n");
		}

		lind = 0;
		while (list[lind] != NULL) {
			p = list[lind++];
			if (strstr(p, "cmd=") == p ||
			    strstr(p, "qry=") == p) {
				char *str = process_remote_cmd(p, 1);
				if (! str) {
					str = strdup("");
				}
				nbytes = write(fd, str, strlen(str));
				write(fd, "\n", 1);
				free(str);
				if (nbytes < 0) {
					close(fd);
					icon_mode_socks[socks[i]] = -1;
					break;
				}
			}
		}

		lind = 0;
		while (list[lind] != NULL) {
			p = list[lind++];
			if (p) free(p);
		}
		free(list);
	}
}

rfbClientPtr create_new_client(int sock, int start_thread) {
	rfbClientPtr cl;

	if (!screen) {
		return NULL;
	}

	cl = rfbNewClient(screen, sock);

	if (cl == NULL) {
		return NULL;	
	}
	if (use_threads) {
		cl->onHold = FALSE;
		if (start_thread) {
			rfbStartOnHoldClient(cl);
		}
	}
	return cl;
}

static int turn_off_truecolor = 0;

static void turn_off_truecolor_ad(rfbClientPtr client) {
	if (client) {}
	if (turn_off_truecolor) {
		rfbLog("turning off truecolor advertising.\n");
		/* mutex */
		screen->serverFormat.trueColour = FALSE;
		screen->displayHook = NULL;
		screen->serverFormat.redShift   = 0;
		screen->serverFormat.greenShift = 0;
		screen->serverFormat.blueShift  = 0;
		screen->serverFormat.redMax     = 0;
		screen->serverFormat.greenMax   = 0;
		screen->serverFormat.blueMax    = 0;
		turn_off_truecolor = 0;
	}
}

/*
 * some overrides for the local console text chat.
 * could be useful in general for local helpers.
 */

rfbBool password_check_chat_helper(rfbClientPtr cl, const char* response, int len) {
	if (response || len) {}
	if (cl != chat_window_client) {
		rfbLog("invalid client during chat_helper login\n");
		return FALSE;
	} else {
		if (!cl->host) {
			rfbLog("empty cl->host during chat_helper login\n");
			return FALSE;
		}
		if (strcmp(cl->host, "127.0.0.1")) {
			rfbLog("invalid cl->host during chat_helper login: %s\n", cl->host);
			return FALSE;
		}
		rfbLog("chat_helper login accepted\n");
		return TRUE;
	}
}

enum rfbNewClientAction new_client_chat_helper(rfbClientPtr client) {
	if (client) {}
	client->clientGoneHook = client_gone_chat_helper;
	rfbLog("new chat helper\n");
	return(RFB_CLIENT_ACCEPT);
}

void client_gone_chat_helper(rfbClientPtr client) {
	if (client) {}
	rfbLog("finished chat helper\n");
	chat_window_client = NULL;
}

void client_set_net(rfbClientPtr client) {
	ClientData *cd; 
	if (client == NULL) {
		return;
	}
	cd = (ClientData *) client->clientData;
	if (cd == NULL) {
		return;
	}
	if (cd->client_port < 0) {
		double dt = dnow();
		cd->client_port = get_remote_port(client->sock);
		cd->server_port = get_local_port(client->sock);
		cd->server_ip   = get_local_host(client->sock);
		cd->hostname = ip2host(client->host);
		rfbLog("client_set_net: %s  %.4f\n", client->host, dnow() - dt);
	}
}
/*
 * libvncserver callback for when a new client connects
 */
enum rfbNewClientAction new_client(rfbClientPtr client) {
	ClientData *cd; 

	CLIENT_LOCK;

	last_event = last_input = time(NULL);

	latest_client = client;

	if (inetd) {
		/* 
		 * Set this so we exit as soon as connection closes,
		 * otherwise client_gone is only called after RFB_CLIENT_ACCEPT
		 */
		if (inetd_client == NULL) {
			inetd_client = client;
			client->clientGoneHook = client_gone;
		}
	}

	clients_served++;

	if (use_openssl || use_stunnel) {
		if (! ssl_initialized) {
			rfbLog("denying additional client: %s ssl not setup"
			    " yet.\n", client->host);
			CLIENT_UNLOCK;
			return(RFB_CLIENT_REFUSE);
		}
	}
	if (unixpw_in_progress) {
		rfbLog("denying additional client: %s during -unixpw login.\n",
		     client->host);
		CLIENT_UNLOCK;
		return(RFB_CLIENT_REFUSE);
	}
	if (connect_once) {
		if (screen->dontDisconnect && screen->neverShared) {
			if (! shared && accepted_client) {
				rfbLog("denying additional client: %s:%d\n",
				     client->host, get_remote_port(client->sock));
				CLIENT_UNLOCK;
				return(RFB_CLIENT_REFUSE);
			}
		}
	}

	if (ipv6_client_ip_str != NULL) {
		rfbLog("renaming client->host from '%s' to '%s'\n",
		    client->host ? client->host : "", ipv6_client_ip_str);
		if (client->host) {
			free(client->host);
		}
		client->host = strdup(ipv6_client_ip_str);
	}

	if (! check_access(client->host)) {
		rfbLog("denying client: %s does not match %s\n", client->host,
		    allow_list ? allow_list : "(null)" );
		CLIENT_UNLOCK;
		return(RFB_CLIENT_REFUSE);
	}

	client->clientData = (void *) calloc(sizeof(ClientData), 1);
	cd = (ClientData *) client->clientData;

	/* see client_set_net() we delay the DNS lookups during handshake */
	cd->client_port = -1;
	cd->username = strdup("");
	cd->unixname = strdup("");

	cd->input[0] = '-';
	cd->login_viewonly = -1;
	cd->login_time = time(NULL);
	cd->ssl_helper_pid = 0;

	if (use_openssl && openssl_last_helper_pid) {
		cd->ssl_helper_pid = openssl_last_helper_pid;
		openssl_last_helper_pid = 0;
	}

	if (! accept_client(client)) {
		rfbLog("denying client: %s local user rejected connection.\n",
		    client->host);
		rfbLog("denying client: accept_cmd=\"%s\"\n",
		    accept_cmd ? accept_cmd : "(null)" );

		free_client_data(client);

		CLIENT_UNLOCK;
		return(RFB_CLIENT_REFUSE);
	}

	/* We will RFB_CLIENT_ACCEPT or RFB_CLIENT_ON_HOLD from here on. */

	if (passwdfile) {
		if (strstr(passwdfile, "read:") == passwdfile ||
		    strstr(passwdfile, "cmd:") == passwdfile) {
			if (read_passwds(passwdfile)) {
				install_passwds();
			} else {
				rfbLog("problem reading: %s\n", passwdfile);
				clean_up_exit(1);
			}
		} else if (strstr(passwdfile, "custom:") == passwdfile) {
			if (screen) {
				/* mutex */
				screen->passwordCheck = custom_passwd_check;
			}
		}
	}

	cd->uid = clients_served;

	client->clientGoneHook = client_gone;

	if (client_count) {
		speeds_net_rate_measured = 0;
		speeds_net_latency_measured = 0;
	}
	client_count++;

	last_keyboard_input = last_pointer_input = time(NULL);

	if (no_autorepeat && client_count == 1 && ! view_only) {
		/*
		 * first client, turn off X server autorepeat
		 * XXX handle dynamic change of view_only and per-client.
		 */
		autorepeat(0, 0);
	}
#ifdef MACOSX
	if (macosx_console && client_count == 1) {
		macosxCG_refresh_callback_on();
	}
#endif
	if (use_solid_bg && client_count == 1) {
		solid_bg(0);
	}

	if (pad_geometry) {
		install_padded_fb(pad_geometry);
	}

	cd->timer = last_new_client = dnow();
	cd->send_cmp_rate = 0.0;
	cd->send_raw_rate = 0.0;
	cd->latency = 0.0;
	cd->cmp_bytes_sent = 0;
	cd->raw_bytes_sent = 0;

	accepted_client++;
	rfbLog("incr accepted_client=%d for %s:%d  sock=%d\n", accepted_client,
	    client->host, get_remote_port(client->sock), client->sock);
	last_client = time(NULL);

	if (ncache) {
		check_ncache(1, 0);
	}

	if (advertise_truecolor && indexed_color) {
		int rs = 0, gs = 2, bs = 4;
		int rm = 3, gm = 3, bm = 3;
		if (bpp >= 24) {
			rs = 0, gs = 8, bs = 16;
			rm = 255, gm = 255, bm = 255;
		} else if (bpp >= 16) {
			rs = 0, gs = 5, bs = 10;
			rm = 31, gm = 31, bm = 31;
		}
		rfbLog("advertising truecolor.\n");
		if (getenv("ADVERT_BMSHIFT")) {
			bm--;
		}

		if (use_threads) LOCK(client->updateMutex);

		client->format.trueColour = TRUE;
		client->format.redShift   = rs;
		client->format.greenShift = gs;
		client->format.blueShift  = bs;
		client->format.redMax     = rm;
		client->format.greenMax   = gm;
		client->format.blueMax    = bm;

		if (use_threads) UNLOCK(client->updateMutex);

		rfbSetTranslateFunction(client);

		/* mutex */
		screen->serverFormat.trueColour = TRUE;
		screen->serverFormat.redShift   = rs;
		screen->serverFormat.greenShift = gs;
		screen->serverFormat.blueShift  = bs;
		screen->serverFormat.redMax     = rm;
		screen->serverFormat.greenMax   = gm;
		screen->serverFormat.blueMax    = bm;
		screen->displayHook = turn_off_truecolor_ad;

		turn_off_truecolor = 1;
	}

	if (unixpw) {
		unixpw_in_progress = 1;
		unixpw_client = client;
		unixpw_login_viewonly = 0;

		unixpw_file_xfer_save = screen->permitFileTransfer;
		screen->permitFileTransfer = FALSE;
		unixpw_tightvnc_xfer_save = tightfilexfer;
		tightfilexfer = 0;
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
		rfbLog("rfbUnregisterTightVNCFileTransferExtension: 1\n");
		rfbUnregisterTightVNCFileTransferExtension();
#endif

		if (client->viewOnly) {
			unixpw_login_viewonly = 1;
			client->viewOnly = FALSE;
		}
		unixpw_last_try_time = time(NULL) + 10;

		unixpw_screen(1);
		unixpw_keystroke(0, 0, 1);

		if (!unixpw_in_rfbPE) {
			rfbLog("new client: %s in non-unixpw_in_rfbPE.\n",
			     client->host);
		}
		CLIENT_UNLOCK;
		if (!use_threads) {
			/* always put client on hold even if unixpw_in_rfbPE is true */
			return(RFB_CLIENT_ON_HOLD);
		} else {
			/* unixpw threads is still in testing mode, disabled by default. See UNIXPW_THREADS */
			return(RFB_CLIENT_ACCEPT);
		}
	}

	CLIENT_UNLOCK;
	return(RFB_CLIENT_ACCEPT);
}

void start_client_info_sock(char *host_port_cookie) {
	char *host = NULL, *cookie = NULL, *p;
	char *str = strdup(host_port_cookie);
	int i, port, sock, next = -1;
	static time_t start_time[ICON_MODE_SOCKS];
	time_t oldest = 0;
	int db = 0;

	port = -1;

	for (i = 0; i < ICON_MODE_SOCKS; i++) {
		if (icon_mode_socks[i] < 0) {
			next = i;
			break;
		}
		if (oldest == 0 || start_time[i] < oldest) {
			next = i;
			oldest = start_time[i];
		}
	}

	p = strtok(str, ":");
	i = 0;
	while (p) {
		if (i == 0) {
			host = strdup(p);
		} else if (i == 1) {
			port = atoi(p);
		} else if (i == 2) {
			cookie = strdup(p);
		}
		i++;
		p = strtok(NULL, ":");
	}
	free(str);

	if (db) fprintf(stderr, "%s/%d/%s next=%d\n", host, port, cookie, next);

	if (host && port && cookie) {
		if (*host == '\0') {
			free(host);
			host = strdup("localhost");
		}
		sock = connect_tcp(host, port);
		if (sock < 0) {
			usleep(200 * 1000);
			sock = connect_tcp(host, port);
		}
		if (sock >= 0) {
			char *lst = list_clients();
			icon_mode_socks[next] = sock;
			start_time[next] = time(NULL);
			write(sock, "COOKIE:", strlen("COOKIE:"));
			write(sock, cookie, strlen(cookie));
			write(sock, "\n", strlen("\n"));
			write(sock, "none\n", strlen("none\n"));
			write(sock, "none\n", strlen("none\n"));
			write(sock, lst, strlen(lst));
			write(sock, "\n", strlen("\n"));
			if (db) {
				fprintf(stderr, "list: %s\n", lst);
			}
			free(lst);
			rfbLog("client_info_sock to: %s:%d\n", host, port);
		} else {
			rfbLog("failed client_info_sock: %s:%d\n", host, port);
		}
	} else {
		rfbLog("malformed client_info_sock: %s\n", host_port_cookie);	
	}

	if (host) free(host);
	if (cookie) free(cookie);
}

void send_client_info(char *str) {
	int i;
	static char *pstr = NULL;
	static int len = 128; 

	if (!str || strlen(str) == 0) {
		return;
	}

	if (!pstr)  {
		pstr = (char *)malloc(len);
	}
	if (strlen(str) + 2 > (size_t) len) {
		free(pstr);
		len *= 2;
		pstr = (char *)malloc(len);
	}
	strcpy(pstr, str);
	strcat(pstr, "\n");

	if (icon_mode_fh) {
		if (0) fprintf(icon_mode_fh, "\n");
		fprintf(icon_mode_fh, "%s", pstr);
		fflush(icon_mode_fh);
	}

	for (i=0; i<ICON_MODE_SOCKS; i++) {
		int len, n, sock = icon_mode_socks[i];
		char *buf = pstr;

		if (sock < 0) {
			continue;
		}

		len = strlen(pstr);
		while (len > 0) {
			if (0) write(sock, "\n", 1);
			n = write(sock, buf, len);
			if (n > 0) {
				buf += n;
				len -= n;
				continue;
			}

			if (n < 0 && errno == EINTR) {
				continue;
			}
			close(sock);
			icon_mode_socks[i] = -1;
			break;
		}
	}
}

void adjust_grabs(int grab, int quiet) {
	RAWFB_RET_VOID
#if NO_X11
	if (!grab || !quiet) {}
	return;
#else
	/* n.b. caller decides to X_LOCK or not. */
	if (grab) {
		if (grab_kbd) {
			if (! quiet) {
				rfbLog("grabbing keyboard with XGrabKeyboard\n");
			}
			XGrabKeyboard(dpy, window, False, GrabModeAsync,
			    GrabModeAsync, CurrentTime);
		}
		if (grab_ptr) {
			if (! quiet) {
				rfbLog("grabbing pointer with XGrabPointer\n");
			}
			XGrabPointer(dpy, window, False, 0, GrabModeAsync,
			    GrabModeAsync, None, None, CurrentTime);
		}
	} else {
		if (grab_kbd) {
			if (! quiet) {
				rfbLog("ungrabbing keyboard with XUngrabKeyboard\n");
			}
			XUngrabKeyboard(dpy, CurrentTime);
		}
		if (grab_ptr) {
			if (! quiet) {
				rfbLog("ungrabbing pointer with XUngrabPointer\n");
			}
			XUngrabPointer(dpy, CurrentTime);
		}
	}
#endif	/* NO_X11 */
}

void check_new_clients(void) {
	static int last_count = -1;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int i, send_info = 0;
	int run_after_accept = 0;

	if (unixpw_in_progress) {
		static double lping = 0.0;
		if (lping < dnow() + 5) {
			mark_rect_as_modified(0, 0, 1, 1, 1);
			lping = dnow();
		}
		if (unixpw_client && unixpw_client->viewOnly) {
			unixpw_login_viewonly = 1;
			unixpw_client->viewOnly = FALSE;
		}
		if (time(NULL) > unixpw_last_try_time + 45) {
			rfbLog("unixpw_deny: timed out waiting for reply.\n");
			unixpw_deny();
		}
		return;
	}

	if (grab_always) {
		;
	} else if (grab_kbd || grab_ptr) {
		static double last_force = 0.0;
		if (client_count != last_count || dnow() > last_force + 0.25) {
			int q = (client_count == last_count);
			last_force = dnow();
			X_LOCK;
			if (client_count) {
				adjust_grabs(1, q);
			} else {
				adjust_grabs(0, q);
			}
			X_UNLOCK;
		}
	}
	
	if (last_count == -1) {
		last_count = 0;
	} else if (client_count == last_count) {
		return;
	}

	if (! all_clients_initialized()) {
		return;
	}

	if (client_count > last_count) {
		if (afteraccept_cmd != NULL && afteraccept_cmd[0] != '\0') {
			run_after_accept = 1;
		}
	}

	last_count = client_count;

	if (! screen) {
		return;
	}

	if (! client_count) {
		send_client_info("clients:none");
		return;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;
		char *s;

		client_set_net(cl);
		if (! cd) {
			continue;
		}

		if (cd->login_viewonly < 0) {
			/* this is a general trigger to initialize things */
			if (cl->viewOnly) {
				cd->login_viewonly = 1;
				s = allowed_input_view_only;
				if (s && cd->input[0] == '-') {
					cl->viewOnly = FALSE;
					cd->input[0] = '\0';
					strncpy(cd->input, s, CILEN);
				}
			} else {
				cd->login_viewonly = 0;
				s = allowed_input_normal;
				if (s && cd->input[0] == '-') {
					cd->input[0] = '\0';
					strncpy(cd->input, s, CILEN);
				}
			}
			if (run_after_accept) {
				run_user_command(afteraccept_cmd, cl,
				    "afteraccept", NULL, 0, NULL);
			}
		}
	}
	rfbReleaseClientIterator(iter);

	if (icon_mode_fh) {
		send_info++;
	}
	for (i = 0; i < ICON_MODE_SOCKS; i++) {
		if (send_info || icon_mode_socks[i] >= 0) {
			send_info++;
			break;
		}
	}
	if (send_info) {
		char *str, *s = list_clients();
		str = (char *) malloc(strlen("clients:") + strlen(s) + 1);
		sprintf(str, "clients:%s", s);
		send_client_info(str);
		free(str);
		free(s);
	}
}

