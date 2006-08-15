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
#include "scan.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "xwrappers.h"
#include "xevents.h"

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
void reverse_connect(char *str);
void set_vnc_connect_prop(char *str);
void read_vnc_connect_prop(int);
void set_x11vnc_remote_prop(char *str);
void read_x11vnc_remote_prop(int);
void check_connect_inputs(void);
void check_gui_inputs(void);
enum rfbNewClientAction new_client(rfbClientPtr client);
void start_client_info_sock(char *host_port_cookie);
void send_client_info(char *str);
void adjust_grabs(int grab, int quiet);
void check_new_clients(void);
int accept_client(rfbClientPtr client);


static rfbClientPtr *client_match(char *str);
static int run_user_command(char *cmd, rfbClientPtr client, char *mode);
static void free_client_data(rfbClientPtr client);
static int check_access(char *addr);
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
			break;
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
		count++;
	}
	rfbReleaseClientIterator(iter);

	/*
	 * each client:
         * <id>:<ip>:<port>:<user>:<unix>:<hostname>:<input>:<loginview>:<time>,
	 * 8+1+64+1+5+1+24+1+24+1+256+1+5+1+1+1+10+1
	 * 123.123.123.123:60000/0x11111111-rw,
	 * so count+1 * 500 must cover it.
	 */
	list = (char *) malloc((count+1)*500);
	
	list[0] = '\0';

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd = (ClientData *) cl->clientData;
		if (! cd) {
			continue;
		}
		if (*list != '\0') {
			strcat(list, ",");
		}
		sprintf(tmp, "0x%x:", cd->uid);
		strcat(list, tmp);
		strcat(list, cl->host);
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
		strcat(list, cd->hostname);
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
		if (strstr(str, "0x") == str) {
			unsigned int in;
			int id;
			ClientData *cd = (ClientData *) cl->clientData;
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
			char *rstr = str;
			if (! dotted_ip(str))  {
				rstr = host2ip(str);
				if (rstr == NULL || *rstr == '\0') {
					if (host_warn++) {
						continue;
					}
					rfbLog("skipping bad lookup: \"%s\"\n",
					    str);
					continue;
				}
				rfbLog("lookup: %s -> %s\n", str, rstr);
			}
			if (!strcmp(rstr, cl->host)) {
				cl_list[i++] = cl;
			}
			if (rstr != str) {
				free(rstr);
			}
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

	p = strchr(str, ':');
	if (! p) {
		return;
	}
	*p = '\0';
	p++;
	val = short_kmbc(p);
	
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
static int run_user_command(char *cmd, rfbClientPtr client, char *mode) {
	char *old_display = NULL;
	char *addr = client->host;
	char str[100];
	int rc, ok;
	ClientData *cd = (ClientData *) client->clientData;

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

	if (client->state == RFB_PROTOCOL_VERSION) {
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

	/* set RFB_CLIENT_PORT to peer port for command to use */
	if (cd && cd->client_port > 0) {
		sprintf(str, "%d", cd->client_port);
	} else {
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
	} else {
		char *sip = get_local_host(client->sock);
		set_env("RFB_SERVER_IP", sip);
		if (sip) free(sip);
	}

	if (cd && cd->server_port > 0) {
		sprintf(str, "%d", cd->server_port);
	} else {
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
	if (!strcmp(mode, "accept") && cmd_ok("accept")) {
		ok = 1;
	}
	if (!strcmp(mode, "afteraccept") && cmd_ok("afteraccept")) {
		ok = 1;
	}
	if (!strcmp(mode, "gone") && cmd_ok("gone")) {
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
	rfbLog("  %s\n", cmd);

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
			execlp("/bin/sh", "/bin/sh", "-c", cmd, (char *) NULL);
			exit(1);
		}
	}
#else
	/* this will still have port 5900 open */
	rc = system(cmd);
#endif

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

	client_count--;
	if (client_count < 0) client_count = 0;

	speeds_net_rate_measured = 0;
	speeds_net_latency_measured = 0;

	rfbLog("client_count: %d\n", client_count);

	if (unixpw_in_progress && unixpw_client) {
		if (client == unixpw_client) {
			unixpw_in_progress = 0;
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
			run_user_command(gone_cmd, client, "gone");
		}
	}

	free_client_data(client);

	if (inetd && client == inetd_client) {
		rfbLog("inetd viewer exited.\n");
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
		     client->state == RFB_AUTHENTICATION) && accepted_client) {
			rfbLog("connect_once: invalid password or early "
			   "disconnect.\n");
			rfbLog("connect_once: waiting for next connection.\n"); 
			accepted_client = 0;
			return;
		}
		if (shared && client_count > 0)  {
			rfbLog("connect_once: other shared clients still "
			    "connected, not exiting.\n");
			return;
		}

		rfbLog("viewer exited.\n");
		clean_up_exit(0);
	}
}

/*
 * Simple routine to limit access via string compare.  A power user will
 * want to compile libvncserver with libwrap support and use /etc/hosts.allow.
 */
static int check_access(char *addr) {
	int allowed = 0;
	char *p, *list;

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
	} else {
		int len = strlen(allow_list) + 1;
		if (allow_once) {
			len += strlen(allow_once) + 1;
		}
		list = (char *) malloc(len);
		list[0] = '\0';
		strcat(list, allow_list);
		if (allow_once) {
			strcat(list, ",");
			strcat(list, allow_once);
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
		if (! dotted_ip(p)) {
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

		q = strstr(addr, chk);
		if (chk[strlen(chk)-1] != '.') {
			if (!strcmp(addr, chk)) {
				if (chk != p) {
					rfbLog("check_access: client %s "
					    "matches host %s=%s\n", addr,
					    chk, p);
				} else {
					rfbLog("check_access: client %s "
					    "matches host %s\n", addr, chk);
				}
				allowed = 1;
			} else if(!strcmp(chk, "localhost") &&
			    !strcmp(addr, "127.0.0.1")) {
				allowed = 1;
			}
		} else if (q == addr) {
			rfbLog("check_access: client %s matches pattern %s\n",
			    addr, chk);
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
#if NO_X11
	nox11_exit(1);
	return 0;
#else

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

	XSelectInput(dpy, awin, evmask);

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
			type = "unixpw";
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
			XSelectInput(dpy, awin, 0);
			XUnmapWindow(dpy, awin);
			XFreeGC(dpy, gc);
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
		rc = run_user_command(cmd, client, "accept");

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

/*
 * For the -connect <file> option: periodically read the file looking for
 * a connect string.  If one is found set client_connect to it.
 */
static void check_connect_file(char *file) {
	FILE *in;
	char line[VNC_CONNECT_MAX], host[VNC_CONNECT_MAX];
	static int first_warn = 1, truncate_ok = 1;
	static time_t last_time = 0; 
	time_t now = time(NULL);

	if (last_time == 0) {
		last_time = now;
	}
	if (now - last_time < 1) {
		/* check only once a second */
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
				    dnow() - x11vnc_start < 3.0) {
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

/*
 * Do a reverse connect for a single "host" or "host:port"
 */
static int do_reverse_connect(char *str) {
	rfbClientPtr cl;
	char *host, *p;
	int rport = 5500, len = strlen(str);

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
	if (use_openssl && !getenv("X11VNC_SSL_ALLOW_REVERSE")) {
		rfbLog("reverse connections disabled in -ssl mode.\n");
		return 0;
	}
	if (unixpw_in_progress) return 0;

	/* copy in to host */
	host = (char *) malloc(len+1);
	if (! host) {
		rfbLog("reverse_connect: could not malloc string %d\n", len);
		return 0;
	}
	strncpy(host, str, len);
	host[len] = '\0';

	/* extract port, if any */
	if ((p = strchr(host, ':')) != NULL) {
		rport = atoi(p+1);
		*p = '\0';
	}

	if (inetd && unixpw) {
		if(strcmp(host, "localhost") && strcmp(host, "127.0.0.1")) {
			if (! getenv("UNIXPW_DISABLE_LOCALHOST")) {
				rfbLog("reverse_connect: in -inetd only localhost\n");
				rfbLog("connections allowed under -unixpw\n");
				return 0;
			}
		}
		if (! getenv("UNIXPW_DISABLE_SSL") && ! have_ssh_env()) {
			rfbLog("reverse_connect: in -inetd stunnel/ssh\n");
			rfbLog("required under -unixpw\n");
			return 0;
		}
	}

	cl = rfbReverseConnection(screen, host, rport);
	free(host);

	if (cl == NULL) {
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

	if (unixpw_in_progress) return;

	tmp = strdup(str);

	p = strtok(tmp, ", \t\r\n");
	while (p) {
		if ((n = do_reverse_connect(p)) != 0) {
			rfbPE(-1);
		}
		cnt += n;

		p = strtok(NULL, ", \t\r\n");
		if (p) {
			t = 0;
			while (t < sleep_between_host) {
				usleep(dt * 1000);
				rfbPE(-1);
				t += dt;
			}
		}
	}
	free(tmp);

	if (cnt == 0) {
		return;
	}

	/*
	 * XXX: we need to process some of the initial handshaking
	 * events, otherwise the client can get messed up (why??) 
	 * so we send rfbProcessEvents() all over the place.
	 */

	n = cnt;
	if (n >= n_max) {
		n = n_max; 
	} 
	t = sleep_max - sleep_min;
	tot = sleep_min + ((n-1) * t) / (n_max-1);

	t = 0;
	while (t < tot) {
		rfbPE(-1);
		usleep(dt * 1000);
		t += dt;
	}
}

/*
 * Routines for monitoring the VNC_CONNECT and X11VNC_REMOTE properties
 * for changes.  The vncconnect(1) will set it on our X display.
 */
void set_vnc_connect_prop(char *str) {
	RAWFB_RET_VOID
#if !NO_X11
	XChangeProperty(dpy, rootwin, vnc_connect_prop, XA_STRING, 8,
	    PropModeReplace, (unsigned char *)str, strlen(str));
#endif	/* NO_X11 */
}

void set_x11vnc_remote_prop(char *str) {
	RAWFB_RET_VOID
#if !NO_X11
	XChangeProperty(dpy, rootwin, x11vnc_remote_prop, XA_STRING, 8,
	    PropModeReplace, (unsigned char *)str, strlen(str));
#endif	/* NO_X11 */
}

void read_vnc_connect_prop(int nomsg) {
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
#if NO_X11
	return;
#else

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
				XFree(data);
				break;
			}
			memcpy(vnc_connect_str+slen, data, dlen);
			slen += dlen;
			vnc_connect_str[slen] = '\0';
			XFree(data);
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
#if NO_X11
	return;
#else

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
				XFree(data);
				break;
			}
			memcpy(x11vnc_remote_str+slen, data, dlen);
			slen += dlen;
			x11vnc_remote_str[slen] = '\0';
			XFree(data);
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
	} else if (strlen(x11vnc_remote_str) > 38) {
		char trim[100]; 
		trim[0] = '\0';
		strncat(trim, x11vnc_remote_str, 38);
		rfbLog("read X11VNC_REMOTE: %s ...\n", trim);
		
	} else {
		rfbLog("read X11VNC_REMOTE: %s\n", x11vnc_remote_str);
	}
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

		p = strtok(buf, "\r\n");
		while (p) {
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
			p = strtok(NULL, "\r\n");
		}
	}
}

/*
 * libvncserver callback for when a new client connects
 */
enum rfbNewClientAction new_client(rfbClientPtr client) {
	ClientData *cd; 

	last_event = last_input = time(NULL);

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
if (getenv("NEW_CLIENT")) fprintf(stderr, "new_client: %s %d\n", client->host, clients_served);

	if (use_openssl || use_stunnel) {
		if (! ssl_initialized) {
			rfbLog("denying additional client: %s ssl not setup"
			    " yet.\n", client->host);
			return(RFB_CLIENT_REFUSE);
		}
	}
	if (unixpw && unixpw_in_progress) {
		rfbLog("denying additional client: %s during -unixpw login.\n",
		     client->host);
		return(RFB_CLIENT_REFUSE);
	}
	if (connect_once) {
		if (screen->dontDisconnect && screen->neverShared) {
			if (! shared && accepted_client) {
				rfbLog("denying additional client: %s\n",
				     client->host);
				return(RFB_CLIENT_REFUSE);
			}
		}
	}
	if (! check_access(client->host)) {
		rfbLog("denying client: %s does not match %s\n", client->host,
		    allow_list ? allow_list : "(null)" );
		return(RFB_CLIENT_REFUSE);
	}

	client->clientData = (void *) calloc(sizeof(ClientData), 1);
	cd = (ClientData *) client->clientData;

	cd->client_port = get_remote_port(client->sock);
	cd->server_port = get_local_port(client->sock);
	cd->server_ip   = get_local_host(client->sock);
	cd->hostname = ip2host(client->host);
	cd->username = strdup("");
	cd->unixname = strdup("");

	cd->input[0] = '-';
	cd->login_viewonly = -1;
	cd->login_time = time(NULL);
	cd->ssl_helper_pid = 0;

	if (use_openssl && openssl_last_helper_pid) {
if (0) fprintf(stderr, "SET ssl_helper_pid: %d\n", openssl_last_helper_pid);
		cd->ssl_helper_pid = openssl_last_helper_pid;
		openssl_last_helper_pid = 0;
	}

	if (! accept_client(client)) {
		rfbLog("denying client: %s local user rejected connection.\n",
		    client->host);
		rfbLog("denying client: accept_cmd=\"%s\"\n",
		    accept_cmd ? accept_cmd : "(null)" );

		free_client_data(client);

		return(RFB_CLIENT_REFUSE);
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
	if (use_solid_bg && client_count == 1) {
		solid_bg(0);
	}

	if (pad_geometry) {
		install_padded_fb(pad_geometry);
	}

	cd->timer = dnow();
	cd->send_cmp_rate = 0.0;
	cd->send_raw_rate = 0.0;
	cd->latency = 0.0;
	cd->cmp_bytes_sent = 0;
	cd->raw_bytes_sent = 0;

	accepted_client = 1;
	last_client = time(NULL);

	if (unixpw) {
		unixpw_in_progress = 1;
		unixpw_client = client;
		unixpw_login_viewonly = 0;
		if (client->viewOnly) {
			unixpw_login_viewonly = 1;
			client->viewOnly = FALSE;
		}
		unixpw_last_try_time = time(NULL);
		unixpw_screen(1);
		unixpw_keystroke(0, 0, 1);
		if (!unixpw_in_rfbPE) {
			rfbLog("new client: %s in non-unixpw_in_rfbPE.\n",
			     client->host);
		}
		/* always put client on hold even if unixpw_in_rfbPE is true */
		return(RFB_CLIENT_ON_HOLD);
	}

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
		sock = rfbConnectToTcpAddr(host, port);
		if (sock < 0) {
			usleep(200 * 1000);
			sock = rfbConnectToTcpAddr(host, port);
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
	static int last_count = 0;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int i, send_info = 0;
	int run_after_accept = 0;

	if (unixpw_in_progress) {
		if (unixpw_client && unixpw_client->viewOnly) {
			unixpw_login_viewonly = 1;
			unixpw_client->viewOnly = FALSE;
		}
		if (time(NULL) > unixpw_last_try_time + 25) {
			rfbLog("unixpw_deny: timed out waiting for reply.\n");
			unixpw_deny();
		}
		return;
	}

	if (grab_kbd || grab_ptr) {
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
	
	if (client_count == last_count) {
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
				    "afteraccept");
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

