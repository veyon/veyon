/* -- remote.c -- */

#include "x11vnc.h"
#include "inet.h"
#include "xwrappers.h"
#include "xevents.h"
#include "xinerama.h"
#include "xrandr.h"
#include "xdamage.h"
#include "xrecord.h"
#include "xkb_bell.h"
#include "win_utils.h"
#include "screen.h"
#include "cleanup.h"
#include "gui.h"
#include "solid.h"
#include "user.h"
#include "rates.h"
#include "scan.h"
#include "connections.h"
#include "pointer.h"
#include "cursor.h"
#include "userinput.h"
#include "keyboard.h"
#include "selection.h"
#include "unixpw.h"
#include "uinput.h"

int send_remote_cmd(char *cmd, int query, int wait);
int do_remote_query(char *remote_cmd, char *query_cmd, int remote_sync,
    int qdefault);
void check_black_fb(void);
int check_httpdir(void);
void http_connections(int on);
int remote_control_access_ok(void);
char *process_remote_cmd(char *cmd, int stringonly);


static char *add_item(char *instr, char *item);
static char *delete_item(char *instr, char *item);
static void if_8bpp_do_new_fb(void);
static void reset_httpport(int old, int new);
static void reset_rfbport(int old, int new) ;


/*
 * for the wild-n-crazy -remote/-R interface.
 */
int send_remote_cmd(char *cmd, int query, int wait) {
	FILE *in = NULL;

	if (client_connect_file) {
		in = fopen(client_connect_file, "w");
		if (in == NULL) {
			fprintf(stderr, "send_remote_cmd: could not open "
			    "connect file \"%s\" for writing\n",
			    client_connect_file);
			perror("fopen");
			return 1;
		}
	} else if (x11vnc_remote_prop == None) {
		initialize_x11vnc_remote_prop();
		if (x11vnc_remote_prop == None) {
			fprintf(stderr, "send_remote_cmd: could not obtain "
			    "X11VNC_REMOTE X property\n");
			return 1;
		}
	}

	if (in != NULL) {
		fprintf(stderr, ">>> sending remote command: \"%s\"\n  via"
		    " connect file: %s\n", cmd, client_connect_file);
		fprintf(in, "%s\n", cmd);
		fclose(in);
	} else {
		fprintf(stderr, ">>> sending remote command: \"%s\" via"
		    " X11VNC_REMOTE X property.\n", cmd);
		set_x11vnc_remote_prop(cmd);
		if (dpy) {
			XFlush_wr(dpy);
		}
	}

	if (query || wait) {
		char line[X11VNC_REMOTE_MAX];	
		int rc=1, i=0, max=70, ms_sl=50;

		if (!strcmp(cmd, "cmd=stop")) {
			max = 20;
		}
		for (i=0; i<max; i++) {
			usleep(ms_sl * 1000);
			if (client_connect_file) {
				char *q;
				in = fopen(client_connect_file, "r");
				if (in == NULL) {
					fprintf(stderr, "send_remote_cmd: could"
					    " not open connect file \"%s\" for"
					    " writing\n", client_connect_file);
					perror("fopen");
					return 1;
				}
				fgets(line, X11VNC_REMOTE_MAX, in);
				fclose(in);
				q = line;
				while (*q != '\0') {
					if (*q == '\n') *q = '\0';
					q++;
				}
			} else {
				read_x11vnc_remote_prop(1);
				strncpy(line, x11vnc_remote_str,
				    X11VNC_REMOTE_MAX);
			}
			if (strcmp(cmd, line)){
				if (query) {
					fprintf(stdout, "%s\n", line);
					fflush(stdout);
				}
				rc = 0;
				break;
			}
		}
		if (rc) {
			fprintf(stderr, "error: could not connect to "
			    "an x11vnc server at %s  (rc=%d)\n",
			    client_connect_file ? client_connect_file
			    : DisplayString(dpy), rc);
		}
		return rc;
	}
	return 0;
}

int do_remote_query(char *remote_cmd, char *query_cmd, int remote_sync,
    int qdefault) {
	char *rcmd = NULL, *qcmd = NULL;
	int rc = 1;

	if (qdefault && !query_cmd) {
		query_cmd = remote_cmd;
		remote_cmd = NULL;
	}

	if (remote_cmd) {
		rcmd = (char *) malloc(strlen(remote_cmd) + 5);
		strcpy(rcmd, "cmd=");
		strcat(rcmd, remote_cmd);
	}
	if (query_cmd) {
		qcmd = (char *) malloc(strlen(query_cmd) + 5);
		strcpy(qcmd, "qry=");
		strcat(qcmd, query_cmd);
	}
	if (qdefault) {
		char *res;
		if (!qcmd) {
			return 1;
		}
		res = process_remote_cmd(qcmd, 1);
		fprintf(stdout, "%s\n", res);
		fflush(stdout);
		return 0;
	}
	
	if (rcmd && qcmd) {
		rc = send_remote_cmd(rcmd, 0, 1);
		if (rc) {
			free(rcmd);
			free(qcmd);
			return(rc);
		}
		rc = send_remote_cmd(qcmd, 1, 1);
	} else if (rcmd) {
		rc = send_remote_cmd(rcmd, 0, remote_sync);
		free(rcmd);
	} else if (qcmd) {
		rc = send_remote_cmd(qcmd, 1, 1);
		free(qcmd);
	}
	return rc;
}

static char *add_item(char *instr, char *item) {
	char *p, *str;
	int len, saw_item = 0;

	if (! instr || *instr == '\0') {
		str = strdup(item);
		return str;
	}
	len = strlen(instr) + 1 + strlen(item) + 1;
	str = (char *) malloc(len);
	str[0] = '\0';

	/* n.b. instr will be modified; caller replaces with returned string */
	p = strtok(instr, ",");
	while (p) {
		if (!strcmp(p, item)) {
			if (saw_item) {
				p = strtok(NULL, ",");
				continue;
			}
			saw_item = 1;
		} else if (*p == '\0') {
			p = strtok(NULL, ",");
			continue;
		}
		if (str[0]) {
			strcat(str, ",");
		}
		strcat(str, p);
		p = strtok(NULL, ",");
	}
	if (! saw_item) {
		if (str[0]) {
			strcat(str, ",");
		}
		strcat(str, item);
	}
	return str;
}

static char *delete_item(char *instr, char *item) {
	char *p, *str;
	int len;

	if (! instr || *instr == '\0') {
		str = strdup("");
		return str;
	}
	len = strlen(instr) + 1;
	str = (char *) malloc(len);
	str[0] = '\0';

	/* n.b. instr will be modified; caller replaces with returned string */
	p = strtok(instr, ",");
	while (p) {
		if (!strcmp(p, item) || *p == '\0') {
			p = strtok(NULL, ",");
			continue;
		}
		if (str[0]) {
			strcat(str, ",");
		}
		strcat(str, p);
		p = strtok(NULL, ",");
	}
	return str;
}

static void if_8bpp_do_new_fb(void) {
	if (bpp == 8) {
		do_new_fb(0);
	} else {
		rfbLog("  bpp(%d) is not 8bpp, not resetting fb\n", bpp);
	}
}

void check_black_fb(void) {
	if (!screen) {
		return;
	}
	if (new_fb_size_clients(screen) != client_count) {
		rfbLog("trying to send a black fb for non-newfbsize"
		    " clients %d != %d\n", client_count,
		    new_fb_size_clients(screen));
		push_black_screen(4);
	}
}

int check_httpdir(void) {
	if (http_dir && http_dir[0] != '\0') {
		return 1;
	} else {
		char *prog = NULL, *httpdir, *q;
		struct stat sbuf;
		int len;

		rfbLog("check_httpdir: trying to guess httpdir...\n");
		if (program_name[0] == '/') {
			prog = strdup(program_name);
		} else {
			char cwd[1024];
			getcwd(cwd, 1024);
			len = strlen(cwd) + 1 + strlen(program_name) + 1;
			prog = (char *) malloc(len);
			snprintf(prog, len, "%s/%s", cwd, program_name);
			if (stat(prog, &sbuf) != 0) {
				char *path = strdup(getenv("PATH"));
				char *p, *base;
				base = strrchr(program_name, '/');
				if (base) {
					base++;
				} else {
					base = program_name;
				}
				
				p = strtok(path, ":");
				while(p) {
					if (prog) {
						free(prog);
						prog = NULL;
					}
					len = strlen(p) + 1 + strlen(base) + 1;
					prog = (char *) malloc(len);
					snprintf(prog, len, "%s/%s", p, base);
					if (stat(prog, &sbuf) == 0) {
						break;
					}
					p = strtok(NULL, ":");
				}
				free(path);
			}
		}
		/*
		 * /path/to/bin/x11vnc
		 * /path/to/bin/../share/x11vnc/classes
		 *                    12345678901234567
		 * /path/to/bin/../share/x11vnc/classes/ssl
		 *                    123456789012345678901
		 *                                        21
		 */
		if ((q = strrchr(prog, '/')) == NULL) {
			rfbLog("check_httpdir: bad program path: %s\n", prog);
			free(prog);
			return 0;
		}

		len = strlen(prog) + 21 + 1;
		*q = '\0';
		httpdir = (char *) malloc(len);
		if (use_openssl || use_stunnel || http_ssl) {
			snprintf(httpdir, len, "%s/../share/x11vnc/classes/ssl", prog);
		} else {
			snprintf(httpdir, len, "%s/../share/x11vnc/classes", prog);
		}
		free(prog);

		if (stat(httpdir, &sbuf) == 0) {
			/* good enough for me */
			rfbLog("check_httpdir: guessed directory:\n");
			rfbLog("   %s\n", httpdir);
			http_dir = httpdir;
			return 1;
		} else {
			/* try some hardwires: */
			int i;
			char **use;
			char *list[] = {
				"/usr/local/share/x11vnc/classes",
				"/usr/share/x11vnc/classes",
				NULL
			};
			char *ssllist[] = {
				"/usr/local/share/x11vnc/classes/ssl",
				"/usr/share/x11vnc/classes/ssl",
				NULL
			};
			if (use_openssl || use_stunnel || http_ssl) {
				use = ssllist;
			} else {
				use = list;
			}
			i = 0;
			while (use[i] != NULL) {
				if (stat(use[i], &sbuf) == 0) {
					http_dir = strdup(use[i]);	
					return 1;
				}
				i++;
			}

			rfbLog("check_httpdir: bad guess:\n");
			rfbLog("   %s\n", httpdir);
			return 0;
		}
	}
}

void http_connections(int on) {
	if (!screen) {
		return;
	}
	if (on) {
		rfbLog("http_connections: turning on http service.\n");

		if (inetd && use_openssl) {
			/*
			 * try to work around rapid fire https requests
			 * in inetd mode... ugh.
			 */
			if (screen->httpPort == 0) {
				int port = find_free_port(5800, 5850);
				if (port) {
					screen->httpPort = port;
				}
			}
		}
		screen->httpInitDone = FALSE;
		if (check_httpdir()) {
			screen->httpDir = http_dir;
			rfbHttpInitSockets(screen);
		}
	} else {
		rfbLog("http_connections: turning off http service.\n");
		if (screen->httpListenSock > -1) {
			close(screen->httpListenSock);
		}
		screen->httpListenSock = -1;
		screen->httpDir = NULL;
	}
}

static void reset_httpport(int old, int new) {
	int hp = new;
	if (hp < 0) {
		rfbLog("reset_httpport: invalid httpport: %d\n", hp);
	} else if (hp == old) {
		rfbLog("reset_httpport: unchanged httpport: %d\n", hp);
	} else if (inetd) {
		rfbLog("reset_httpport: cannot set httpport: %d"
		    " in inetd.\n", hp);
	} else if (screen) {
		screen->httpPort = hp;
		screen->httpInitDone = FALSE;
		if (screen->httpListenSock > -1) {
			close(screen->httpListenSock);
		}
		rfbLog("reset_httpport: setting httpport %d -> %d.\n",
		    old == -1 ? hp : old, hp);
		rfbHttpInitSockets(screen);
	}
}

static void reset_rfbport(int old, int new)  {
	int rp = new;
	if (rp < 0) {
		rfbLog("reset_rfbport: invalid rfbport: %d\n", rp);
	} else if (rp == old) {
		rfbLog("reset_rfbport: unchanged rfbport: %d\n", rp);
	} else if (inetd) {
		rfbLog("reset_rfbport: cannot set rfbport: %d"
		    " in inetd.\n", rp);
	} else if (screen) {
		rfbClientIteratorPtr iter;
		rfbClientPtr cl;
		int maxfd;
		if (rp == 0) {
			screen->autoPort = TRUE;
		} else {
			screen->autoPort = FALSE;
		}
		screen->port = rp;
		screen->socketState = RFB_SOCKET_INIT;

		if (screen->listenSock > -1) {
			close(screen->listenSock);
		}

		rfbLog("reset_rfbport: setting rfbport %d -> %d.\n",
		    old == -1 ? rp : old, rp);
		rfbInitSockets(screen);

		maxfd = screen->maxFd;
		if (screen->udpSock > 0 && screen->udpSock > maxfd) {
			maxfd = screen->udpSock;
		}
		iter = rfbGetClientIterator(screen);
		while( (cl = rfbClientIteratorNext(iter)) ) {
			if (cl->sock > -1) {
				FD_SET(cl->sock, &(screen->allFds));
				if (cl->sock > maxfd) {
					maxfd = cl->sock;
				}
			}
		}
		rfbReleaseClientIterator(iter);

		screen->maxFd = maxfd;

		set_vnc_desktop_name();
	}
}

/*
 * Do some sanity checking of the permissions on the XAUTHORITY and the
 * -connect file.  This is -privremote.  What should be done is check
 * for an empty host access list, currently we lazily do not bring in
 * libXau yet.
 */
int remote_control_access_ok(void) {
	struct stat sbuf;

#if NO_X11
	return 0;
#else
	if (client_connect_file) {
		if (stat(client_connect_file, &sbuf) == 0) {
			if (sbuf.st_mode & S_IWOTH) {
				rfbLog("connect file is writable by others.\n");
				rfbLog("   %s\n", client_connect_file);
				return 0;
			}
			if (sbuf.st_mode & S_IWGRP) {
				rfbLog("connect file is writable by group.\n");
				rfbLog("   %s\n", client_connect_file);
				return 0;
			}
		}
	}

	if (dpy) {
		char tmp[1000];
		char *home, *xauth;
		char *dpy_str = DisplayString(dpy);
		Display *dpy2;
		XHostAddress *xha;
		Bool enabled;
		int n;

		home = get_home_dir();
		if (getenv("XAUTHORITY") != NULL) {
			xauth = getenv("XAUTHORITY");
		} else if (home) {
			int len = 1000 - strlen("/.Xauthority") - 1;
			strncpy(tmp, home, len); 
			strcat(tmp, "/.Xauthority");
			xauth = tmp;
		} else {
			rfbLog("cannot determine default XAUTHORITY.\n");
			return 0;
		}
		if (home) {
			free(home);
		}
		if (stat(xauth, &sbuf) == 0) {
			if (sbuf.st_mode & S_IWOTH) {
				rfbLog("XAUTHORITY is writable by others!!\n");
				rfbLog("   %s\n", xauth);
				return 0;
			}
			if (sbuf.st_mode & S_IWGRP) {
				rfbLog("XAUTHORITY is writable by group!!\n");
				rfbLog("   %s\n", xauth);
				return 0;
			}
			if (sbuf.st_mode & S_IROTH) {
				rfbLog("XAUTHORITY is readable by others.\n");
				rfbLog("   %s\n", xauth);
				return 0;
			}
			if (sbuf.st_mode & S_IRGRP) {
				rfbLog("XAUTHORITY is readable by group.\n");
				rfbLog("   %s\n", xauth);
				return 0;
			}
		}

		xha = XListHosts(dpy, &n, &enabled);
		if (! enabled) {
			rfbLog("X access control is disabled, X clients can\n");
			rfbLog("   connect from any host.  Run 'xhost -'\n");
			return 0;
		}
		if (xha) {
			int i;
			rfbLog("The following hosts can connect w/o X11 "
			    "auth:\n");
			for (i=0; i<n; i++) {
				if (xha[i].family == FamilyInternet) {
					char *str = raw2host(xha[i].address,
					    xha[i].length);
					char *ip = raw2ip(xha[i].address);
					rfbLog("  %s/%s\n", str, ip);
					free(str);
					free(ip);
				} else {
					rfbLog("  unknown-%d\n", i+1);
				}
			}
			XFree(xha);
			return 0;
		}

		if (getenv("XAUTHORITY")) {
			xauth = strdup(getenv("XAUTHORITY"));
		} else {
			xauth = NULL;
		}
		set_env("XAUTHORITY", "/impossible/xauthfile");

		fprintf(stderr, "\nChecking if display %s requires "
		    "XAUTHORITY\n", dpy_str);
		fprintf(stderr, "   -- (ignore any Xlib: errors that"
		    " follow) --\n");
		dpy2 = XOpenDisplay_wr(dpy_str); 
		fflush(stderr);
		fprintf(stderr, "   -- (done checking) --\n\n");

		if (xauth) {
			set_env("XAUTHORITY", xauth);
			free(xauth);
		} else {
			xauth = getenv("XAUTHORITY");
			if (xauth) {
				*(xauth-2) = '_';	/* yow */
			}
		}
		if (dpy2) {
			rfbLog("XAUTHORITY is not required on display.\n");
			rfbLog("   %s\n", DisplayString(dpy));
			XCloseDisplay_wr(dpy2);
			dpy2 = NULL;
			return 0;
		}

	}
	return 1;
#endif	/* NO_X11 */
}

static int hack_val = 0;

/*
 * Huge, ugly switch to handle all remote commands and queries
 * -remote/-R and -query/-Q.
 */
char *process_remote_cmd(char *cmd, int stringonly) {
#if REMOTE_CONTROL
	char *p = cmd;
	char *co = "";
	char buf[X11VNC_REMOTE_MAX]; 
	int bufn = X11VNC_REMOTE_MAX;
	int query = 0;
	static char *prev_cursors_mode = NULL;

	if (!query_default && !accept_remote_cmds) {
		rfbLog("remote commands disabled: %s\n", cmd);
		return NULL;
	}
	if (unixpw_in_progress) {
		rfbLog("skip remote command: %s unixpw_in_progress.\n", cmd);
		return NULL;
	}

	if (!query_default && priv_remote) {
		if (! remote_control_access_ok()) {
			rfbLog("** Disabling remote commands in -privremote "
			    "mode.\n");
			accept_remote_cmds = 0;
			return NULL;
		}
	}

	strcpy(buf, "");
	if (strstr(cmd, "cmd=") == cmd) {
		p += strlen("cmd=");
	} else if (strstr(cmd, "qry=") == cmd) {
		query = 1;
		if (strchr(cmd, ',')) {
			/* comma separated batch mode */
			char *s, *q, *res;
			char tmp[512];
			char **pieces;
			int k = 0, n = 0;

			pieces = (char **) malloc(strlen(cmd) * sizeof(char *));
			s = strdup(cmd + strlen("qry="));
			q = strtok(s, ","); 

			while (q) {
				strcpy(tmp, "qry=");
				strncat(tmp, q, 500);
				pieces[n] = strdup(tmp);
				n++;
				q = strtok(NULL, ",");
			}
			free(s);

			strcpy(buf, "");
			for (k=0; k<n; k++) {
				res = process_remote_cmd(pieces[k], 1);
				if (res && strlen(buf)+strlen(res)
				    >= X11VNC_REMOTE_MAX - 1) {
					rfbLog("overflow in process_remote_cmd:"
					    " %s -- %s\n", buf, res);
					free(res);
					break;
				}
				if (res) {
					strcat(buf, res);
					free(res);
				}
				if (k < n - 1) {
					strcat(buf, ",");
				}
			}
			for (k=0; k<n; k++) {
				free(pieces[k]);
			}
			free(pieces);
			goto qry;
		}
		p += strlen("qry=");
	} else {
		rfbLog("ignoring malformed command: %s\n", cmd);
		goto done;
	}

	/* allow var=val usage */
	if (!strchr(p, ':')) {
		char *q = strchr(p, '=');
		if (q) *q = ':';
	}

	/* always call like: COLON_CHECK("foobar:") */
#define COLON_CHECK(str) \
	if (strstr(p, str) != p) { \
		co = ":"; \
		if (! query) { \
			goto done; \
		} \
	} else { \
		char *q = strchr(p, ':'); \
		if (query && q != NULL) { \
			*(q+1) = '\0'; \
		} \
	}

#define NOTAPP \
	if (query) { \
		if (strchr(p, ':')) { \
			snprintf(buf, bufn, "ans=%sN/A", p); \
		} else { \
			snprintf(buf, bufn, "ans=%s:N/A", p); \
		} \
		goto qry; \
	}

#define NOTAPPRO \
	if (query) { \
		if (strchr(p, ':')) { \
			snprintf(buf, bufn, "aro=%sN/A", p); \
		} else { \
			snprintf(buf, bufn, "aro=%s:N/A", p); \
		} \
		goto qry; \
	}

/*
 * Maybe add: passwdfile logfile bg rfbauth passwd...
 */
	if (!strcmp(p, "stop") || !strcmp(p, "quit") ||
	    !strcmp(p, "exit") || !strcmp(p, "shutdown")) {
		NOTAPP
		if (client_connect_file) {
			FILE *in = fopen(client_connect_file, "w");
			if (in) {
				fprintf(in, "cmd=noop\n");
				fclose(in);
			}
		}
		rfbLog("remote_cmd: setting shut_down flag\n");
		shut_down = 1;
		close_all_clients();

	} else if (!strcmp(p, "ping")) {
		query = 1;
		if (rfb_desktop_name) {
			snprintf(buf, bufn, "ans=%s:%s", p, rfb_desktop_name);
		} else {
			snprintf(buf, bufn, "ans=%s:%s", p, "unknown");
		}
		goto qry;

	} else if (!strcmp(p, "blacken") || !strcmp(p, "zero")) {
		NOTAPP
		push_black_screen(4);
	} else if (!strcmp(p, "refresh")) {
		NOTAPP
		refresh_screen(1);
	} else if (!strcmp(p, "reset")) {
		NOTAPP
		do_new_fb(1);
	} else if (strstr(p, "zero:") == p) { /* skip-cmd-list */
		int x1, y1, x2, y2;
		NOTAPP
		p += strlen("zero:");
		if (sscanf(p, "%d,%d,%d,%d", &x1, &y1, &x2, &y2) == 4)  {
			int mark = 1;
			rfbLog("zeroing rect: %s\n", p);
			if (x1 < 0 || x2 < 0) {
				x1 = nabs(x1);
				x2 = nabs(x2);
				mark = 0;	/* hack for testing */
			}

			zero_fb(x1, y1, x2, y2);
			if (mark) {
				mark_rect_as_modified(x1, y1, x2, y2, 0);
			}
			push_sleep(4);
		}
	} else if (strstr(p, "damagefb:") == p) { /* skip-cmd-list */
		int delay;
		NOTAPP
		p += strlen("damagefb:");
		if (sscanf(p, "%d", &delay) == 1)  {
			rfbLog("damaging client fb's for %d secs "
			    "(by not marking rects.)\n", delay);
			damage_time = time(NULL);
			damage_delay = delay;
		}

	} else if (strstr(p, "close") == p) {
		NOTAPP
		COLON_CHECK("close:")
		p += strlen("close:");
		close_clients(p);
	} else if (strstr(p, "disconnect") == p) {
		NOTAPP
		COLON_CHECK("disconnect:")
		p += strlen("disconnect:");
		close_clients(p);

	} else if (strstr(p, "id") == p) {
		int ok = 0;
		Window twin;
		COLON_CHECK("id:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s0x%lx", p, co,
			    rootshift ? 0 : subwin);
			goto qry;
		}
		p += strlen("id:");
		if (*p == '\0' || !strcmp("root", p)) {
			/* back to root win */
			twin = 0x0;
			ok = 1;
		} else if (!strcmp("pick", p)) {
			twin = 0x0;
			if (safe_remote_only) {
				rfbLog("unsafe: '-id pick'\n");
			} else if (pick_windowid(&twin)) {
				ok = 1;
			}
		} else if (! scan_hexdec(p, &twin)) {
			rfbLog("-id: skipping incorrect hex/dec number:"
			    " %s\n", p);
		} else {
			ok = 1;
		}
		if (ok) {
			if (twin && ! valid_window(twin, NULL, 0)) {
				rfbLog("skipping invalid sub-window: 0x%lx\n",
				    twin);
			} else {
				subwin = twin;
				rootshift = 0;
				check_black_fb();
				do_new_fb(1);
			}
		}
	} else if (strstr(p, "sid") == p) {
		int ok = 0;
		Window twin;
		COLON_CHECK("sid:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s0x%lx", p, co,
			    !rootshift ? 0 : subwin);
			goto qry;
		}
		p += strlen("sid:");
		if (*p == '\0' || !strcmp("root", p)) {
			/* back to root win */
			twin = 0x0;
			ok = 1;
		} else if (!strcmp("pick", p)) {
			twin = 0x0;
			if (safe_remote_only) {
				rfbLog("unsafe: '-sid pick'\n");
			} else if (pick_windowid(&twin)) {
				ok = 1;
			}
		} else if (! scan_hexdec(p, &twin)) {
			rfbLog("-sid: skipping incorrect hex/dec number: %s\n", p);
		} else {
			ok = 1;
		}
		if (ok) {
			if (twin && ! valid_window(twin, NULL, 0)) {
				rfbLog("skipping invalid sub-window: 0x%lx\n",
				    twin);
			} else {
				subwin = twin;
				rootshift = 1;
				check_black_fb();
				do_new_fb(1);
			}
		}
	} else if (strstr(p, "waitmapped") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    subwin_wait_mapped);
			goto qry;
		}
		subwin_wait_mapped = 1;
	} else if (strstr(p, "nowaitmapped") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !subwin_wait_mapped);
			goto qry;
		}
		subwin_wait_mapped = 0;

	} else if (!strcmp(p, "clip") ||
	    strstr(p, "clip:") == p) {	/* skip-cmd-list */
		COLON_CHECK("clip:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(clip_str));
			goto qry;
		}
		p += strlen("clip:");
		if (clip_str) free(clip_str);
		clip_str = strdup(p);

		/* OK, this requires a new fb... */
		do_new_fb(1);

	} else if (!strcmp(p, "flashcmap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, flash_cmap);
			goto qry;
		}
		rfbLog("remote_cmd: turning on flashcmap mode.\n");
		flash_cmap = 1;
	} else if (!strcmp(p, "noflashcmap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !flash_cmap);
			goto qry;
		}
		rfbLog("remote_cmd: turning off flashcmap mode.\n");
		flash_cmap = 0;
		
	} else if (strstr(p, "shiftcmap") == p) {
		COLON_CHECK("shiftcmap:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, shift_cmap);
			goto qry;
		}
		p += strlen("shiftcmap:");
		shift_cmap = atoi(p);
		rfbLog("remote_cmd: set -shiftcmap %d\n", shift_cmap);
		do_new_fb(1);

	} else if (!strcmp(p, "truecolor")) {
		int orig = force_indexed_color;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !force_indexed_color);
			goto qry;
		}
		rfbLog("remote_cmd: turning off notruecolor mode.\n");
		force_indexed_color = 0;
		if (orig != force_indexed_color) {
			if_8bpp_do_new_fb();
		}
	} else if (!strcmp(p, "notruecolor")) {
		int orig = force_indexed_color;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    force_indexed_color);
			goto qry;
		}
		rfbLog("remote_cmd: turning on notruecolor mode.\n");
		force_indexed_color = 1;
		if (orig != force_indexed_color) {
			if_8bpp_do_new_fb();
		}
		
	} else if (!strcmp(p, "overlay")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, overlay);
			goto qry;
		}
		rfbLog("remote_cmd: turning on -overlay mode.\n");
		if (!overlay_present) {
			rfbLog("skipping: overlay extension not present.\n");
		} else if (overlay) {
			rfbLog("skipping: already in -overlay mode.\n");
		} else {
			int reset_mem = 0;
			/* here we go... */
			if (using_shm) {
				rfbLog("setting -noshm mode.\n");
				using_shm = 0;
				reset_mem = 1;
			}
			overlay = 1;
			do_new_fb(reset_mem);
		}
	} else if (!strcmp(p, "nooverlay")) {
		int orig = overlay;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !overlay);
			goto qry;
		}
		rfbLog("remote_cmd: turning off overlay mode\n");
		overlay = 0;
		if (!overlay_present) {
			rfbLog("warning: overlay extension not present.\n");
		} else if (!orig) {
			rfbLog("skipping: already not in -overlay mode.\n");
		} else {
			/* here we go... */
			do_new_fb(0);
		}
		
	} else if (!strcmp(p, "overlay_cursor") ||
	    !strcmp(p, "overlay_yescursor") ||
	    !strcmp(p, "nooverlay_nocursor")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, overlay_cursor);
			goto qry;
		}
		rfbLog("remote_cmd: turning on overlay_cursor mode.\n");
		overlay_cursor = 1;
		if (!overlay_present) {
			rfbLog("warning: overlay extension not present.\n");
		} else if (!overlay) {
			rfbLog("warning: not in -overlay mode.\n");
		} else {
			rfbLog("You may want to run -R noshow_cursor or\n");
			rfbLog(" -R cursor:none to disable any extra "
			    "cursors.\n");
		}
	} else if (!strcmp(p, "nooverlay_cursor") ||
	    !strcmp(p, "nooverlay_yescursor") ||
	    !strcmp(p, "overlay_nocursor")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !overlay_cursor);
			goto qry;
		}
		rfbLog("remote_cmd: turning off overlay_cursor mode\n");
		overlay_cursor = 0;
		if (!overlay_present) {
			rfbLog("warning: overlay extension not present.\n");
		} else if (!overlay) {
			rfbLog("warning: not in -overlay mode.\n");
		} else {
			rfbLog("You may want to run -R show_cursor or\n");
			rfbLog(" -R cursor:... to re-enable any cursors.\n");
		}
		
	} else if (!strcmp(p, "8to24")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, cmap8to24);
			goto qry;
		}
		if (overlay) {
			rfbLog("disabling -overlay in -8to24 mode.\n");
			overlay = 0;
		}
		rfbLog("remote_cmd: turning on -8to24 mode.\n");
		cmap8to24 = 1;
		if (overlay) {
			rfbLog("disabling -overlay in -8to24 mode.\n");
			overlay = 0;
		}
		do_new_fb(0);

	} else if (!strcmp(p, "no8to24")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !cmap8to24);
			goto qry;
		}
		rfbLog("remote_cmd: turning off -8to24 mode.\n");
		cmap8to24 = 0;
		do_new_fb(0);

	} else if (strstr(p, "8to24_opts") == p) {
		COLON_CHECK("8to24_opts:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(cmap8to24_str));
			goto qry;
		}
		p += strlen("8to24_opts:");
		if (cmap8to24_str) {
			free(cmap8to24_str);
		}
		cmap8to24_str = strdup(p);
		if (*p == '\0') {
			cmap8to24 = 0;
		} else {
			cmap8to24 = 1;
		}
		rfbLog("remote_cmd: set cmap8to24_str to: %s\n", cmap8to24_str);
		do_new_fb(0);

	} else if (!strcmp(p, "24to32")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, xform24to32);
			goto qry;
		}
		rfbLog("remote_cmd: turning on -24to32 mode.\n");
		xform24to32 = 1;
		do_new_fb(1);

	} else if (!strcmp(p, "no24to32")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !xform24to32);
			goto qry;
		}
		rfbLog("remote_cmd: turning off -24to32 mode.\n");
		if (set_visual_str_to_something) {
			if (visual_str) {
				rfbLog("unsetting: %d %d/%d\n", visual_str,
				    (int) visual_id, visual_depth);
				free(visual_str);
			}
			visual_str = NULL;
			visual_id = (VisualID) 0;
			visual_depth = 0;
		}
		xform24to32 = 0;
		do_new_fb(1);

	} else if (strstr(p, "visual") == p) {
		COLON_CHECK("visual:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(visual_str));
			goto qry;
		}
		p += strlen("visual:");
		if (visual_str) free(visual_str);
		visual_str = strdup(p);

		/* OK, this requires a new fb... */
		do_new_fb(0);

	} else if (!strcmp(p, "scale") ||
		    strstr(p, "scale:") == p) {	/* skip-cmd-list */
		COLON_CHECK("scale:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(scale_str));
			goto qry;
		}
		p += strlen("scale:");
		if (scale_str) free(scale_str);
		scale_str = strdup(p);

		/* OK, this requires a new fb... */
		check_black_fb();
		do_new_fb(0);

	} else if (!strcmp(p, "scale_cursor") ||
		    strstr(p, "scale_cursor:") == p) {	/* skip-cmd-list */
		COLON_CHECK("scale_cursor:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(scale_cursor_str));
			goto qry;
		}
		p += strlen("scale_cursor:");
		if (scale_cursor_str) free(scale_cursor_str);
		if (*p == '\0') {
			scale_cursor_str = NULL;
		} else {
			scale_cursor_str = strdup(p);
		}
		setup_cursors_and_push();

	} else if (!strcmp(p, "viewonly")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, view_only);
			goto qry;
		}
		rfbLog("remote_cmd: enable viewonly mode.\n");
		view_only = 1;
	} else if (!strcmp(p, "noviewonly")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !view_only);
			goto qry;
		}
		rfbLog("remote_cmd: disable viewonly mode.\n");
		view_only = 0;
		if (raw_fb) set_raw_fb_params(0);

	} else if (!strcmp(p, "shared")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, shared); goto qry;
		}
		rfbLog("remote_cmd: enable sharing.\n");
		shared = 1;
		if (screen) {
			screen->alwaysShared = TRUE;
			screen->neverShared = FALSE;
		}
	} else if (!strcmp(p, "noshared")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !shared); goto qry;
		}
		rfbLog("remote_cmd: disable sharing.\n");
		shared = 0;
		if (screen) {
			screen->alwaysShared = FALSE;
			screen->neverShared = TRUE;
		}

	} else if (!strcmp(p, "forever")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, 1-connect_once);
			goto qry;
		}
		rfbLog("remote_cmd: enable -forever mode.\n");
		connect_once = 0;
	} else if (!strcmp(p, "noforever") || !strcmp(p, "once")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, connect_once);
			goto qry;
		}
		rfbLog("remote_cmd: disable -forever mode.\n");
		connect_once = 1;

	} else if (strstr(p, "timeout") == p) {
		int to;
		COLON_CHECK("timeout:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    first_conn_timeout);
			goto qry;
		}
		p += strlen("timeout:");
		to = atoi(p);
		if (to > 0 ) {
			to = -to;
		}
		first_conn_timeout = to;
		rfbLog("remote_cmd: set -timeout to %d\n", -to);

	} else if (!strcmp(p, "filexfer")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, filexfer);
			goto qry;
		}
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
		if (! filexfer) {
			rfbLog("remote_cmd: enabling -filexfer for new clients.\n");
			filexfer = 1;
			rfbRegisterTightVNCFileTransferExtension();
		}
#else
		rfbLog("remote_cmd: -filexfer not supported in this binary.\n");
#endif

	} else if (!strcmp(p, "nofilexfer")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !filexfer);
			goto qry;
		}
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
		if (filexfer) {
			rfbLog("remote_cmd: disabling -filexfer for new clients.\n");
			filexfer = 0;
			rfbUnregisterTightVNCFileTransferExtension();
		}
#else
		rfbLog("remote_cmd: -filexfer not supported in this binary.\n");
#endif

	} else if (!strcmp(p, "deny") || !strcmp(p, "lock")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, deny_all);
			goto qry;
		}
		rfbLog("remote_cmd: denying new connections.\n");
		deny_all = 1;
	} else if (!strcmp(p, "nodeny") || !strcmp(p, "unlock")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !deny_all);
			goto qry;
		}
		rfbLog("remote_cmd: allowing new connections.\n");
		deny_all = 0;

	} else if (strstr(p, "connect") == p) {
		NOTAPP
		COLON_CHECK("connect:")
		p += strlen("connect:");
		/* this is a reverse connection */
		reverse_connect(p);

	} else if (strstr(p, "allowonce") == p) {
		COLON_CHECK("allowonce:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(allow_once));
			goto qry;
		}
		p += strlen("allowonce:");
		allow_once = strdup(p);
		rfbLog("remote_cmd: set allow_once %s\n", allow_once);

	} else if (strstr(p, "allow") == p) {
		char *before, *old;
		COLON_CHECK("allow:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(allow_list));
			goto qry;
		}

		if (unixpw) {
			rfbLog("remote_cmd: cannot change allow in -unixpw\n");
			goto done;
		}
		p += strlen("allow:");
		if (allow_list && strchr(allow_list, '/')) {
			rfbLog("remote_cmd: cannot use allow:host\n");
			rfbLog("in '-allow %s' mode.\n", allow_list);
			goto done;
		}
		if (allow_list) {
			before = strdup(allow_list);
		} else {
			before = strdup("");
		}

		old = allow_list;
		if (*p == '+') {
			p++;
			allow_list = add_item(allow_list, p);
		} else if (*p == '-') {
			p++;
			allow_list = delete_item(allow_list, p);
		} else {
			allow_list = strdup(p);
		}

		if (strcmp(before, allow_list)) {
			rfbLog("remote_cmd: modified allow_list:\n");
			rfbLog(" from: \"%s\"\n", before);
			rfbLog(" to:   \"%s\"\n", allow_list);
		}
		if (old) free(old);
		free(before);

	} else if (!strcmp(p, "localhost")) {
		char *before, *old;
		if (query) {
			int state = 0;
			char *s = allow_list;
			if (s && (!strcmp(s, "127.0.0.1") ||
			    !strcmp(s, "localhost"))) {
				state = 1;
			}
			snprintf(buf, bufn, "ans=%s:%d", p, state);
			goto qry;
		}
		if (allow_list) {
			before = strdup(allow_list);
		} else {
			before = strdup("");
		}
		old = allow_list;

		allow_list = strdup("127.0.0.1");

		if (strcmp(before, allow_list)) {
			rfbLog("remote_cmd: modified allow_list:\n");
			rfbLog(" from: \"%s\"\n", before);
			rfbLog(" to:   \"%s\"\n", allow_list);
		}
		if (old) free(old);
		free(before);

		if (listen_str) {
			free(listen_str);
		}
		listen_str = strdup("localhost");

		screen->listenInterface = htonl(INADDR_LOOPBACK);
		rfbLog("listening on loopback network only.\n");
		rfbLog("allow list is: '%s'\n", NONUL(allow_list));
		reset_rfbport(-1, screen->port);
		if (screen->httpListenSock > -1) {
			reset_httpport(-1, screen->httpPort);
		}
	} else if (!strcmp(p, "nolocalhost")) {
		char *before, *old;
		if (query) {
			int state = 0;
			char *s = allow_list;
			if (s && (!strcmp(s, "127.0.0.1") ||
			    !strcmp(s, "localhost"))) {
				state = 1;
			}
			snprintf(buf, bufn, "ans=%s:%d", p, !state);
			goto qry;
		}
		if (unixpw) {
			rfbLog("remote_cmd: cannot change localhost in -unixpw\n");
			goto done;
		}
		if (allow_list) {
			before = strdup(allow_list);
		} else {
			before = strdup("");
		}
		old = allow_list;

		allow_list = strdup("");

		if (strcmp(before, allow_list)) {
			rfbLog("remote_cmd: modified allow_list:\n");
			rfbLog(" from: \"%s\"\n", before);
			rfbLog(" to:   \"%s\"\n", allow_list);
		}
		if (old) free(old);
		free(before);

		if (listen_str) {
			free(listen_str);
		}
		listen_str = NULL;

		screen->listenInterface = htonl(INADDR_ANY);
		rfbLog("listening on ALL network interfaces.\n");
		rfbLog("allow list is: '%s'\n", NONUL(allow_list));
		reset_rfbport(-1, screen->port);
		if (screen->httpListenSock > -1) {
			reset_httpport(-1, screen->httpPort);
		}

	} else if (strstr(p, "listen") == p) {
		char *before;
		int ok, mod = 0;

		COLON_CHECK("listen:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(listen_str));
			goto qry;
		}
		if (unixpw) {
			rfbLog("remote_cmd: cannot change listen in -unixpw\n");
			goto done;
		}
		if (listen_str) {
			before = strdup(listen_str);
		} else {
			before = strdup("");
		}
		p += strlen("listen:");

		listen_str = strdup(p);

		if (strcmp(before, listen_str)) {
			rfbLog("remote_cmd: modified listen_str:\n");
			rfbLog(" from: \"%s\"\n", before);
			rfbLog(" to:   \"%s\"\n", listen_str);
			mod = 1;
		}

		ok = 1;
		if (listen_str == NULL || *listen_str == '\0' ||
		    !strcmp(listen_str, "any")) {
			screen->listenInterface = htonl(INADDR_ANY);
		} else if (!strcmp(listen_str, "localhost")) {
			screen->listenInterface = htonl(INADDR_LOOPBACK);
		} else {
			struct hostent *hp;
			in_addr_t iface = inet_addr(listen_str);
			if (iface == htonl(INADDR_NONE)) {
				if (!host_lookup) {
					ok = 0;
				} else if (!(hp = gethostbyname(listen_str))) {
					ok = 0;
				} else {
					iface = *(unsigned long *)hp->h_addr;
				}
			}
			if (ok) {
				screen->listenInterface = iface;
			}
		}

		if (ok && mod) {
			int is_loopback = 0;
			in_addr_t iface = screen->listenInterface;

			if (allow_list) {
				if (!strcmp(allow_list, "127.0.0.1") ||
				    !strcmp(allow_list, "localhost")) {
					is_loopback = 1;
				}
			}
			if (iface != htonl(INADDR_LOOPBACK)) {
			    if (is_loopback) {
				rfbLog("re-setting -allow list to all "
				   "hosts for non-loopback listening.\n");
				if (allow_list) {
					free(allow_list);
				}
				allow_list = NULL;
			    }
			} else {
			    if (!is_loopback) {
				if (allow_list) {
					free(allow_list);
				}
				rfbLog("setting -allow list to 127.0.0.1\n");
				allow_list = strdup("127.0.0.1");
			    }
			}
		}
		if (ok) {
			rfbLog("allow list is: '%s'\n", NONUL(allow_list));
			reset_rfbport(-1, screen->port);
			if (screen->httpListenSock > -1) {
				reset_httpport(-1, screen->httpPort);
			}
			free(before);
		} else {
			rfbLog("invalid listen string: %s\n", listen_str);
			free(listen_str);
			listen_str = before;
		}
	} else if (!strcmp(p, "lookup")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, host_lookup);
			goto qry;
		}
		rfbLog("remote_cmd: enabling hostname lookup.\n");
		host_lookup = 1;
	} else if (!strcmp(p, "nolookup")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !host_lookup);
			goto qry;
		}
		rfbLog("remote_cmd: disabling hostname lookup.\n");
		host_lookup = 0;

	} else if (strstr(p, "accept") == p) {
		int doit = 1, safe = 0;
		COLON_CHECK("accept:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(accept_cmd));
			goto qry;
		}
		p += strlen("accept:");
		if (!strcmp(p, "") || strstr(p, "popup") == p) { /* skip-cmd-list */
			safe = 1;
		}
		if (safe_remote_only && ! safe) {
			rfbLog("unsafe: %s\n", p);
			doit = 0;
		}

		if (doit) {
			if (accept_cmd) free(accept_cmd);
			accept_cmd = strdup(p);
		}

	} else if (strstr(p, "afteraccept") == p) {
		int safe = 0;
		COLON_CHECK("afteraccept:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(afteraccept_cmd));
			goto qry;
		}
		p += strlen("afteraccept:");
		if (!strcmp(p, "")) { /* skip-cmd-list */
			safe = 1;
		}
		if (safe_remote_only && ! safe) {
			rfbLog("unsafe: %s\n", p);
		} else {
			if (afteraccept_cmd) free(afteraccept_cmd);
			afteraccept_cmd = strdup(p);
		}

	} else if (strstr(p, "gone") == p) {
		int safe = 0;
		COLON_CHECK("gone:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(gone_cmd));
			goto qry;
		}
		p += strlen("gone:");
		if (!strcmp(p, "") || strstr(p, "popup") == p) { /* skip-cmd-list */
			safe = 1;
		}
		if (safe_remote_only && ! safe) {
			rfbLog("unsafe: %s\n", p);
		} else {
			if (gone_cmd) free(gone_cmd);
			gone_cmd = strdup(p);
		}

	} else if (!strcmp(p, "shm")) {
		int orig = using_shm;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, using_shm);
			goto qry;
		}
		rfbLog("remote_cmd: turning off noshm mode.\n");
		using_shm = 1;
		if (raw_fb) set_raw_fb_params(0);

		if (orig != using_shm) {
			do_new_fb(1);
		} else {
			rfbLog(" already in shm mode.\n");
		}
	} else if (!strcmp(p, "noshm")) {
		int orig = using_shm;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !using_shm);
			goto qry;
		}
		rfbLog("remote_cmd: turning on noshm mode.\n");
		using_shm = 0;
		if (orig != using_shm) {
			do_new_fb(1);
		} else {
			rfbLog(" already in noshm mode.\n");
		}
		
	} else if (!strcmp(p, "flipbyteorder")) {
		int orig = flip_byte_order;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, flip_byte_order);
			goto qry;
		}
		rfbLog("remote_cmd: turning on flipbyteorder mode.\n");
		flip_byte_order = 1;
		if (orig != flip_byte_order) {
			if (! using_shm || xform24to32) {
				do_new_fb(1);
			} else {
				rfbLog("  using shm, not resetting fb\n");
			}
		}
	} else if (!strcmp(p, "noflipbyteorder")) {
		int orig = flip_byte_order;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !flip_byte_order);
			goto qry;
		}
		rfbLog("remote_cmd: turning off flipbyteorder mode.\n");
		flip_byte_order = 0;
		if (orig != flip_byte_order) {
			if (! using_shm || xform24to32) {
				do_new_fb(1);
			} else {
				rfbLog("  using shm, not resetting fb\n");
			}
		}
		
	} else if (!strcmp(p, "onetile")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, single_copytile);
			goto qry;
		}
		rfbLog("remote_cmd: enable -onetile mode.\n");
		single_copytile = 1;
	} else if (!strcmp(p, "noonetile")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !single_copytile);
			goto qry;
		}
		rfbLog("remote_cmd: disable -onetile mode.\n");
		if (tile_shm_count < ntiles_x) {
			rfbLog(" this has no effect: tile_shm_count=%d"
			    " ntiles_x=%d\n", tile_shm_count, ntiles_x);
			
		}
		single_copytile = 0;

	} else if (strstr(p, "solid_color") == p) {
		/*
		 * n.b. this solid stuff perhaps should reflect
		 * safe_remote_only but at least the command names
		 * are fixed.
		 */
		char *new;
		int doit = 1;
		COLON_CHECK("solid_color:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(solid_str));
			goto qry;
		}
		p += strlen("solid_color:");
		if (*p != '\0') {
			new = strdup(p);
		} else {
			new = strdup(solid_default);
		}
		rfbLog("remote_cmd: solid %s -> %s\n", NONUL(solid_str), new);

		if (solid_str) {
			if (!strcmp(solid_str, new)) {
				doit = 0;
			}
			free(solid_str);
		}
		solid_str = new;
		use_solid_bg = 1;
		if (raw_fb) set_raw_fb_params(0);

		if (doit && client_count) {
			solid_bg(0);
		}
	} else if (!strcmp(p, "solid")) {
		int orig = use_solid_bg;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_solid_bg);
			goto qry;
		}
		rfbLog("remote_cmd: enable -solid mode\n");
		if (! solid_str) {
			solid_str = strdup(solid_default);
		}
		use_solid_bg = 1;
		if (raw_fb) set_raw_fb_params(0);
		if (client_count && !orig) {
			solid_bg(0);
		}
	} else if (!strcmp(p, "nosolid")) {
		int orig = use_solid_bg;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_solid_bg);
			goto qry;
		}
		rfbLog("remote_cmd: disable -solid mode\n");
		use_solid_bg = 0;
		if (client_count && orig) {
			solid_bg(1);
		}

	} else if (strstr(p, "blackout") == p) {
		char *before, *old;
		COLON_CHECK("blackout:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(blackout_str));
			goto qry;
		}
		p += strlen("blackout:");
		if (blackout_str) {
			before = strdup(blackout_str);
		} else {
			before = strdup("");
		}
		old = blackout_str;
		if (*p == '+') {
			p++;
			blackout_str = add_item(blackout_str, p);
		} else if (*p == '-') {
			p++;
			blackout_str = delete_item(blackout_str, p);
		} else {
			blackout_str = strdup(p);
		}
		if (strcmp(before, blackout_str)) {
			rfbLog("remote_cmd: changing -blackout\n");
			rfbLog(" from: %s\n", before);
			rfbLog(" to:   %s\n", blackout_str);
			if (0 && !strcmp(blackout_str, "") &&
			    single_copytile_orig != single_copytile) {
				rfbLog("resetting single_copytile to: %d\n",
				    single_copytile_orig);
				single_copytile = single_copytile_orig;
			}
			initialize_blackouts_and_xinerama();
		}
		if (old) free(old);
		free(before);

	} else if (!strcmp(p, "xinerama")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, xinerama);
			goto qry;
		}
		rfbLog("remote_cmd: enable xinerama mode. (if applicable).\n");
		xinerama = 1;
		initialize_blackouts_and_xinerama();
	} else if (!strcmp(p, "noxinerama")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !xinerama);
			goto qry;
		}
		rfbLog("remote_cmd: disable xinerama mode. (if applicable).\n");
		xinerama = 0;
		initialize_blackouts_and_xinerama();

	} else if (!strcmp(p, "xtrap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, xtrap_input);
			goto qry;
		}
		rfbLog("remote_cmd: enable xtrap input mode."
		    "(if applicable).\n");
		if (! xtrap_input) {
			xtrap_input = 1;
			disable_grabserver(dpy, 1);
		}

	} else if (!strcmp(p, "noxtrap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !xtrap_input);
			goto qry;
		}
		rfbLog("remote_cmd: disable xtrap input mode."
		    "(if applicable).\n");
		if (xtrap_input) {
			xtrap_input = 0;
			disable_grabserver(dpy, 1);
		}

	} else if (!strcmp(p, "xrandr")) {
		int orig = xrandr;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, xrandr); goto qry;
		}
		if (xrandr_present) {
			rfbLog("remote_cmd: enable xrandr mode.\n");
			xrandr = 1;
			if (raw_fb) set_raw_fb_params(0);
			if (! xrandr_mode) {
				xrandr_mode = strdup("default");
			}
			if (orig != xrandr) {
				initialize_xrandr();
			}
		} else {
			rfbLog("remote_cmd: XRANDR ext. not present.\n");
		}
	} else if (!strcmp(p, "noxrandr")) {
		int orig = xrandr;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !xrandr); goto qry;
		}
		xrandr = 0;
		if (xrandr_present) {
			rfbLog("remote_cmd: disable xrandr mode.\n");
			if (orig != xrandr) {
				initialize_xrandr();
			}
		} else {
			rfbLog("remote_cmd: XRANDR ext. not present.\n");
		}
	} else if (strstr(p, "xrandr_mode") == p) {
		int orig = xrandr;
		COLON_CHECK("xrandr_mode:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(xrandr_mode));
			goto qry;
		}
		p += strlen("xrandr_mode:");
		if (!strcmp("none", p)) {
			xrandr = 0;
		} else {
			if (known_xrandr_mode(p)) {
				if (xrandr_mode) free(xrandr_mode);
				xrandr_mode = strdup(p);
			} else {
				rfbLog("skipping unknown xrandr mode: %s\n", p);
				goto done;
			}
			xrandr = 1;
		}
		if (xrandr_present) {
			if (xrandr) {
				rfbLog("remote_cmd: enable xrandr mode.\n");
			} else {
				rfbLog("remote_cmd: disable xrandr mode.\n");
			}
			if (! xrandr_mode) {
				xrandr_mode = strdup("default");
			}
			if (orig != xrandr) {
				initialize_xrandr();
			}
		} else {
			rfbLog("remote_cmd: XRANDR ext. not present.\n");
		}

	} else if (strstr(p, "rotate") == p) {
		COLON_CHECK("rotate:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(rotating_str));
			goto qry;
		}
		p += strlen("rotate:");
		if (rotating_str) free(rotating_str);
		rotating_str = strdup(p);
		rfbLog("remote_cmd: set rotate to \"%s\"\n", rotating_str);

		do_new_fb(0);

	} else if (strstr(p, "padgeom") == p) {
		COLON_CHECK("padgeom:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(pad_geometry));
			goto qry;
		}
		p += strlen("padgeom:");
		if (!strcmp("force", p) || !strcmp("do",p) || !strcmp("go",p)) {
			rfbLog("remote_cmd: invoking install_padded_fb()\n");
			install_padded_fb(pad_geometry);
		} else {
			if (pad_geometry) free(pad_geometry);
			pad_geometry = strdup(p);
			rfbLog("remote_cmd: set padgeom to: %s\n",
			    pad_geometry);
		}

	} else if (!strcmp(p, "quiet") || !strcmp(p, "q")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, quiet); goto qry;
		}
		rfbLog("remote_cmd: turning on quiet mode.\n");
		quiet = 1;
	} else if (!strcmp(p, "noquiet")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !quiet); goto qry;
		}
		rfbLog("remote_cmd: turning off quiet mode.\n");
		quiet = 0;
		
	} else if (!strcmp(p, "modtweak")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_modifier_tweak);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -modtweak mode.\n");
		if (! use_modifier_tweak) {
			use_modifier_tweak = 1;
			initialize_modtweak();
		}
		use_modifier_tweak = 1;

	} else if (!strcmp(p, "nomodtweak")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !use_modifier_tweak);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -nomodtweak mode.\n");
		got_nomodtweak = 1;
		use_modifier_tweak = 0;

	} else if (!strcmp(p, "xkb")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_xkb_modtweak);
			goto qry;
		}
		if (! xkb_present) {
			rfbLog("remote_cmd: cannot enable -xkb "
			    "modtweak mode (not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: enabling -xkb modtweak mode"
		    " (if supported).\n");
		if (! use_modifier_tweak || ! use_xkb_modtweak) {
			use_modifier_tweak = 1;
			use_xkb_modtweak = 1;
			initialize_modtweak();
		}
		use_modifier_tweak = 1;
		use_xkb_modtweak = 1;

	} else if (!strcmp(p, "noxkb")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_xkb_modtweak);
			goto qry;
		}
		if (! xkb_present) {
			rfbLog("remote_cmd: cannot disable -xkb "
			    "modtweak mode (not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: disabling -xkb modtweak mode.\n");
		use_xkb_modtweak = 0;
		got_noxkb = 1;
		initialize_modtweak();

	} else if (!strcmp(p, "capslock")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, watch_capslock);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -capslock mode\n");
		watch_capslock = 1;

	} else if (!strcmp(p, "nocapslock")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !watch_capslock);
			goto qry;
		}
		rfbLog("remote_cmd: disabling -capslock mode\n");
		watch_capslock = 0;

	} else if (!strcmp(p, "skip_lockkeys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, skip_lockkeys);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -skip_lockkeys mode\n");
		skip_lockkeys = 1;

	} else if (!strcmp(p, "noskip_lockkeys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !skip_lockkeys);
			goto qry;
		}
		rfbLog("remote_cmd: disabling -skip_lockkeys mode\n");
		skip_lockkeys = 0;

	} else if (strstr(p, "skip_keycodes") == p) {
		COLON_CHECK("skip_keycodes:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(skip_keycodes));
			goto qry;
		}
		p += strlen("skip_keycodes:");
		rfbLog("remote_cmd: setting xkb -skip_keycodes"
		    " to:\n\t'%s'\n", p);
		if (! xkb_present) {
			rfbLog("remote_cmd: warning xkb not present\n");
		} else if (! use_xkb_modtweak) {
			rfbLog("remote_cmd: turning on xkb.\n");
			use_xkb_modtweak = 1;
			if (! use_modifier_tweak) {
				rfbLog("remote_cmd: turning on modtweak.\n");
				use_modifier_tweak = 1;
			}
		}
		if (skip_keycodes) free(skip_keycodes);
		skip_keycodes = strdup(p);
		initialize_modtweak();

	} else if (!strcmp(p, "sloppy_keys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, sloppy_keys);
			goto qry;
		}
		sloppy_keys += 1;
		rfbLog("remote_cmd: set sloppy_keys to: %d\n", sloppy_keys);
	} else if (!strcmp(p, "nosloppy_keys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !sloppy_keys);
			goto qry;
		}
		sloppy_keys = 0;
		rfbLog("remote_cmd: set sloppy_keys to: %d\n", sloppy_keys);

	} else if (!strcmp(p, "skip_dups")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    skip_duplicate_key_events);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -skip_dups mode\n");
		skip_duplicate_key_events = 1;
	} else if (!strcmp(p, "noskip_dups")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !skip_duplicate_key_events);
			goto qry;
		}
		rfbLog("remote_cmd: disabling -skip_dups mode\n");
		skip_duplicate_key_events = 0;

	} else if (!strcmp(p, "add_keysyms")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, add_keysyms);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -add_keysyms mode.\n");
		add_keysyms = 1;

	} else if (!strcmp(p, "noadd_keysyms")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !add_keysyms);
			goto qry;
		}
		rfbLog("remote_cmd: disabling -add_keysyms mode.\n");
		add_keysyms = 0;

	} else if (!strcmp(p, "clear_mods")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, clear_mods == 1);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -clear_mods mode.\n");
		clear_mods = 1;
		clear_modifiers(0);

	} else if (!strcmp(p, "noclear_mods")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !(clear_mods == 1));
			goto qry;
		}
		rfbLog("remote_cmd: disabling -clear_mods mode.\n");
		clear_mods = 0;

	} else if (!strcmp(p, "clear_keys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    clear_mods == 2);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -clear_keys mode.\n");
		clear_mods = 2;
		clear_keys();

	} else if (!strcmp(p, "noclear_keys")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !(clear_mods == 2));
			goto qry;
		}
		rfbLog("remote_cmd: disabling -clear_keys mode.\n");
		clear_mods = 0;

	} else if (strstr(p, "remap") == p) {
		char *before, *old;
		COLON_CHECK("remap:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(remap_file));
			goto qry;
		}
		p += strlen("remap:");
		if ((*p == '+' || *p == '-') && remap_file &&
		    strchr(remap_file, '/')) {
			rfbLog("remote_cmd: cannot use remap:+/-\n");
			rfbLog("in '-remap %s' mode.\n", remap_file);
			goto done;
		}
		if (remap_file) {
			before = strdup(remap_file);
		} else {
			before = strdup("");
		}
		old = remap_file;
		if (*p == '+') {
			p++;
			remap_file = add_item(remap_file, p);
		} else if (*p == '-') {
			p++;
			remap_file = delete_item(remap_file, p);
			if (! strchr(remap_file, '-')) {
				*remap_file = '\0';
			}
		} else {
			remap_file = strdup(p);
		}
		if (strcmp(before, remap_file)) {
			rfbLog("remote_cmd: changed -remap\n");
			rfbLog(" from: %s\n", before);
			rfbLog(" to:   %s\n", remap_file);
			initialize_remap(remap_file);
		}
		if (old) free(old);
		free(before);

	} else if (!strcmp(p, "repeat")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !no_autorepeat);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -repeat mode.\n");
		autorepeat(1, 0);	/* restore initial setting */
		no_autorepeat = 0;

	} else if (!strcmp(p, "norepeat")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, no_autorepeat);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -norepeat mode.\n");
		no_autorepeat = 1;
		if (no_repeat_countdown >= 0) {
			no_repeat_countdown = 2;
		}
		if (client_count && ! view_only) {
			autorepeat(0, 0);	/* disable if any clients */
		}

	} else if (!strcmp(p, "fb")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !nofb);
			goto qry;
		}
		if (nofb) {
			rfbLog("remote_cmd: disabling nofb mode.\n");
			rfbLog("  you may need to these turn back on:\n");
			rfbLog("     xfixes, xdamage, solid, flashcmap\n");
			rfbLog("     overlay, shm, noonetile, nap, cursor\n");
			rfbLog("     cursorpos, cursorshape, bell.\n");
			nofb = 0;
			set_nofb_params(1);
			do_new_fb(1);
		}
	} else if (!strcmp(p, "nofb")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, nofb);
			goto qry;
		}
		if (!nofb) {
			rfbLog("remote_cmd: enabling nofb mode.\n");
			if (main_fb) {
				push_black_screen(4);
			}
			nofb = 1;
			sound_bell = 0;
			initialize_watch_bell();
			set_nofb_params(0);
			do_new_fb(1);
		}

	} else if (!strcmp(p, "bell")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, sound_bell);
			goto qry;
		}
		rfbLog("remote_cmd: enabling bell (if supported).\n");
		initialize_watch_bell();
		sound_bell = 1;

	} else if (!strcmp(p, "nobell")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !sound_bell);
			goto qry;
		}
		rfbLog("remote_cmd: disabling bell.\n");
		initialize_watch_bell();
		sound_bell = 0;

	} else if (!strcmp(p, "sel")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, watch_selection);
			goto qry;
		}
		rfbLog("remote_cmd: enabling watch selection+primary.\n");
		watch_selection = 1;
		watch_primary = 1;
		watch_clipboard = 1;

	} else if (!strcmp(p, "nosel")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !watch_selection);
			goto qry;
		}
		rfbLog("remote_cmd: disabling watch selection+primary.\n");
		watch_selection = 0;
		watch_primary = 0;
		watch_clipboard = 0;

	} else if (!strcmp(p, "primary")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, watch_primary);
			goto qry;
		}
		rfbLog("remote_cmd: enabling watch_primary.\n");
		watch_primary = 1;

	} else if (!strcmp(p, "noprimary")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !watch_primary);
			goto qry;
		}
		rfbLog("remote_cmd: disabling watch_primary.\n");
		watch_primary = 0;

	} else if (!strcmp(p, "setprimary")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, set_primary);
			goto qry;
		}
		rfbLog("remote_cmd: enabling set_primary.\n");
		set_primary = 1;

	} else if (!strcmp(p, "nosetprimary")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !set_primary);
			goto qry;
		}
		rfbLog("remote_cmd: disabling set_primary.\n");
		set_primary = 0;

	} else if (!strcmp(p, "clipboard")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, watch_clipboard);
			goto qry;
		}
		rfbLog("remote_cmd: enabling watch_clipboard.\n");
		watch_clipboard = 1;

	} else if (!strcmp(p, "noclipboard")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !watch_clipboard);
			goto qry;
		}
		rfbLog("remote_cmd: disabling watch_clipboard.\n");
		watch_clipboard = 0;

	} else if (!strcmp(p, "setclipboard")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, set_clipboard);
			goto qry;
		}
		rfbLog("remote_cmd: enabling set_clipboard.\n");
		set_clipboard = 1;

	} else if (!strcmp(p, "nosetclipboard")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !set_clipboard);
			goto qry;
		}
		rfbLog("remote_cmd: disabling set_clipboard.\n");
		set_clipboard = 0;

	} else if (strstr(p, "seldir") == p) {
		COLON_CHECK("seldir:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(sel_direction));
			goto qry;
		}
		p += strlen("seldir:");
		rfbLog("remote_cmd: setting -seldir to %s\n", p);
		if (sel_direction) free(sel_direction);
		sel_direction = strdup(p);

	} else if (!strcmp(p, "set_no_cursor")) { /* skip-cmd-list */
		rfbLog("remote_cmd: calling set_no_cursor()\n");
		set_no_cursor();

	} else if (!strcmp(p, "cursorshape")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    cursor_shape_updates);
			goto qry;
		}
		rfbLog("remote_cmd: turning on cursorshape mode.\n");

		set_no_cursor();
		cursor_shape_updates = 1;
		restore_cursor_shape_updates(screen);
		first_cursor();
	} else if (!strcmp(p, "nocursorshape")) {
		int i, max = 5;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !cursor_shape_updates);
			goto qry;
		}
		rfbLog("remote_cmd: turning off cursorshape mode.\n");
		
		set_no_cursor();
		for (i=0; i<max; i++) {
			/* XXX: try to force empty cursor back to client */
			rfbPE(-1);
		}
		cursor_shape_updates = 0;
		disable_cursor_shape_updates(screen);
		first_cursor();

	} else if (!strcmp(p, "cursorpos")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    cursor_pos_updates);
			goto qry;
		}
		rfbLog("remote_cmd: turning on cursorpos mode.\n");
		cursor_pos_updates = 1;
	} else if (!strcmp(p, "nocursorpos")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !cursor_pos_updates);
			goto qry;
		}
		rfbLog("remote_cmd: turning off cursorpos mode.\n");
		cursor_pos_updates = 0;

	} else if (strstr(p, "cursor") == p) {
		COLON_CHECK("cursor:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(multiple_cursors_mode));
			goto qry;
		}
		p += strlen("cursor:");
		if (multiple_cursors_mode) {
			if (prev_cursors_mode) free(prev_cursors_mode);
			prev_cursors_mode = strdup(multiple_cursors_mode);
			free(multiple_cursors_mode);
		}
		multiple_cursors_mode = strdup(p);

		rfbLog("remote_cmd: changed -cursor mode "
		    "to: %s\n", multiple_cursors_mode);

		if (strcmp(multiple_cursors_mode, "none") && !show_cursor) {
			show_cursor = 1;
			rfbLog("remote_cmd: changed show_cursor "
			    "to: %d\n", show_cursor);
		}
		initialize_cursors_mode();
		first_cursor();

	} else if (!strcmp(p, "show_cursor")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, show_cursor);
			goto qry;
		}
		rfbLog("remote_cmd: enabling show_cursor.\n");
		show_cursor = 1;
		if (multiple_cursors_mode && !strcmp(multiple_cursors_mode,
		    "none")) {
			free(multiple_cursors_mode);
			if (prev_cursors_mode) {
				multiple_cursors_mode =
				    strdup(prev_cursors_mode);
			} else {
				multiple_cursors_mode = strdup("default");
			}
			rfbLog("remote_cmd: changed -cursor mode "
			    "to: %s\n", multiple_cursors_mode);
		}
		initialize_cursors_mode();
		first_cursor();
	} else if (!strcmp(p, "noshow_cursor") || !strcmp(p, "nocursor")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !show_cursor);
			goto qry;
		}
		if (prev_cursors_mode) free(prev_cursors_mode);
		prev_cursors_mode = strdup(multiple_cursors_mode);

		rfbLog("remote_cmd: disabling show_cursor.\n");
		show_cursor = 0;
		initialize_cursors_mode();
		first_cursor();

	} else if (strstr(p, "arrow") == p) {
		COLON_CHECK("arrow:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, alt_arrow);
			goto qry;
		}
		p += strlen("arrow:");
		alt_arrow = atoi(p);
		rfbLog("remote_cmd: setting alt_arrow: %d.\n", alt_arrow);
		setup_cursors_and_push();

	} else if (!strcmp(p, "xfixes")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_xfixes);
			goto qry;
		}
		if (! xfixes_present) {
			rfbLog("remote_cmd: cannot enable xfixes "
			    "(not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: enabling -xfixes"
		    " (if supported).\n");
		use_xfixes = 1;
		initialize_xfixes();
		first_cursor();
	} else if (!strcmp(p, "noxfixes")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_xfixes);
			goto qry;
		}
		if (! xfixes_present) {
			rfbLog("remote_cmd: disabling xfixes  "
			    "(but not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: disabling -xfixes.\n");
		use_xfixes = 0;
		initialize_xfixes();
		first_cursor();

	} else if (!strcmp(p, "xdamage")) {
		int orig = use_xdamage;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_xdamage);
			goto qry;
		}
		if (! xdamage_present) {
			rfbLog("remote_cmd: cannot enable xdamage hints "
			    "(not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: enabling xdamage hints"
		    " (if supported).\n");
		use_xdamage = 1;
		if (use_xdamage != orig) {
			initialize_xdamage();
			create_xdamage_if_needed();
		}
	} else if (!strcmp(p, "noxdamage")) {
		int orig = use_xdamage;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_xdamage);
			goto qry;
		}
		if (! xdamage_present) {
			rfbLog("remote_cmd: disabling xdamage hints "
			    "(but not supported on X display)\n");
			goto done;
		}
		rfbLog("remote_cmd: disabling xdamage hints.\n");
		use_xdamage = 0;
		if (use_xdamage != orig) {
			initialize_xdamage();
			destroy_xdamage_if_needed();
		}

	} else if (strstr(p, "xd_area") == p) {
		int a;
		COLON_CHECK("xd_area:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    xdamage_max_area);
			goto qry;
		}
		p += strlen("xd_area:");
		a = atoi(p);
		if (a >= 0) {
			rfbLog("remote_cmd: setting xdamage_max_area "
			    "%d -> %d.\n", xdamage_max_area, a);
			xdamage_max_area = a;
		}
	} else if (strstr(p, "xd_mem") == p) {
		double a;
		COLON_CHECK("xd_mem:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%.3f", p, co,
			    xdamage_memory);
			goto qry;
		}
		p += strlen("xd_mem:");
		a = atof(p);
		if (a >= 0.0) {
			rfbLog("remote_cmd: setting xdamage_memory "
			    "%.3f -> %.3f.\n", xdamage_memory, a);
			xdamage_memory = a;
		}

	} else if (strstr(p, "alphacut") == p) {
		int a;
		COLON_CHECK("alphacut:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    alpha_threshold);
			goto qry;
		}
		p += strlen("alphacut:");
		a = atoi(p);
		if (a < 0) a = 0;
		if (a > 256) a = 256;	/* allow 256 for testing. */
		if (alpha_threshold != a) {
			rfbLog("remote_cmd: setting alphacut "
			    "%d -> %d.\n", alpha_threshold, a);
			if (a == 256) {
				rfbLog("note: alphacut=256 leads to completely"
				    " transparent cursors.\n");
			}
			alpha_threshold = a;
			setup_cursors_and_push();
		}
	} else if (strstr(p, "alphafrac") == p) {
		double a;
		COLON_CHECK("alphafrac:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%f", p, co,
			    alpha_frac);
			goto qry;
		}
		p += strlen("alphafrac:");
		a = atof(p);
		if (a < 0.0) a = 0.0;
		if (a > 1.0) a = 1.0;
		if (alpha_frac != a) {
			rfbLog("remote_cmd: setting alphafrac "
			    "%f -> %f.\n", alpha_frac, a);
			alpha_frac = a;
			setup_cursors_and_push();
		}
	} else if (strstr(p, "alpharemove") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, alpha_remove);
			goto qry;
		}
		if (!alpha_remove) {
			rfbLog("remote_cmd: enable alpharemove\n");
			alpha_remove = 1;
			setup_cursors_and_push();
		}
	} else if (strstr(p, "noalpharemove") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !alpha_remove);
			goto qry;
		}
		if (alpha_remove) {
			rfbLog("remote_cmd: disable alpharemove\n");
			alpha_remove = 0;
			setup_cursors_and_push();
		}
	} else if (strstr(p, "alphablend") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, alpha_blend);
			goto qry;
		}
		if (!alpha_blend) {
			rfbLog("remote_cmd: enable alphablend\n");
			alpha_remove = 0;
			alpha_blend = 1;
			setup_cursors_and_push();
		}
	} else if (strstr(p, "noalphablend") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !alpha_blend);
			goto qry;
		}
		if (alpha_blend) {
			rfbLog("remote_cmd: disable alphablend\n");
			alpha_blend = 0;
			setup_cursors_and_push();
		}

	} else if (strstr(p, "xwarppointer") == p || strstr(p, "xwarp") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_xwarppointer);
			goto qry;
		}
		rfbLog("remote_cmd: turning on xwarppointer mode.\n");
		use_xwarppointer = 1;
	} else if (strstr(p, "noxwarppointer") == p ||
		    strstr(p, "noxwarp") == p) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_xwarppointer);
			goto qry;
		}
		rfbLog("remote_cmd: turning off xwarppointer mode.\n");
		use_xwarppointer = 0;

	} else if (strstr(p, "buttonmap") == p) {
		COLON_CHECK("buttonmap:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(pointer_remap));
			goto qry;
		}
		p += strlen("buttonmap:");
		if (pointer_remap) free(pointer_remap);
		pointer_remap = strdup(p);

		rfbLog("remote_cmd: setting -buttonmap to:\n\t'%s'\n", p);
		initialize_pointer_map(p);

	} else if (!strcmp(p, "dragging")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, show_dragging);
			goto qry;
		}
		rfbLog("remote_cmd: enabling mouse dragging mode.\n");
		show_dragging = 1;
	} else if (!strcmp(p, "nodragging")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !show_dragging);
			goto qry;
		}
		rfbLog("remote_cmd: enabling mouse nodragging mode.\n");
		show_dragging = 0;

	} else if (strstr(p, "wireframe_mode") == p) {
		COLON_CHECK("wireframe_mode:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    wireframe_str ? wireframe_str : WIREFRAME_PARMS);
			goto qry;
		}
		p += strlen("wireframe_mode:");
		if (*p) {
			if (wireframe_str) {
				free(wireframe_str);
			}
			wireframe_str = strdup(p);
			parse_wireframe();
		}
		rfbLog("remote_cmd: enabling -wireframe mode.\n");
		wireframe = 1;
	} else if (strstr(p, "wireframe:") == p) {	/* skip-cmd-list */
		COLON_CHECK("wireframe:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, wireframe);
			goto qry;
		}
		p += strlen("wireframe:");
		if (*p) {
			if (wireframe_str) {
				free(wireframe_str);
			}
			wireframe_str = strdup(p);
			parse_wireframe();
		}
		rfbLog("remote_cmd: enabling -wireframe mode.\n");
		wireframe = 1;
	} else if (strstr(p, "wf:") == p) {	/* skip-cmd-list */
		COLON_CHECK("wf:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, wireframe);
			goto qry;
		}
		p += strlen("wf:");
		if (*p) {
			if (wireframe_str) {
				free(wireframe_str);
			}
			wireframe_str = strdup(p);
			parse_wireframe();
		}
		rfbLog("remote_cmd: enabling -wireframe mode.\n");
		wireframe = 1;
	} else if (!strcmp(p, "wireframe") || !strcmp(p, "wf")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, wireframe);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -wireframe mode.\n");
		wireframe = 1;
	} else if (!strcmp(p, "nowireframe") || !strcmp(p, "nowf")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !wireframe);
			goto qry;
		}
		rfbLog("remote_cmd: enabling -nowireframe mode.\n");
		wireframe = 0;

	} else if (strstr(p, "wirecopyrect") == p) {
		COLON_CHECK("wirecopyrect:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(wireframe_copyrect));
			goto qry;
		}
		p += strlen("wirecopyrect:");

		set_wirecopyrect_mode(p);
		rfbLog("remote_cmd: changed -wirecopyrect mode "
		    "to: %s\n", NONUL(wireframe_copyrect));
		got_wirecopyrect = 1;
	} else if (strstr(p, "wcr") == p) {
		COLON_CHECK("wcr:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(wireframe_copyrect));
			goto qry;
		}
		p += strlen("wcr:");

		set_wirecopyrect_mode(p);
		rfbLog("remote_cmd: changed -wirecopyrect mode "
		    "to: %s\n", NONUL(wireframe_copyrect));
		got_wirecopyrect = 1;
	} else if (!strcmp(p, "nowirecopyrect") || !strcmp(p, "nowcr")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%s", p,
			    NONUL(wireframe_copyrect));
			goto qry;
		}

		set_wirecopyrect_mode("never");
		rfbLog("remote_cmd: changed -wirecopyrect mode "
		    "to: %s\n", NONUL(wireframe_copyrect));

	} else if (strstr(p, "scr_area") == p) {
		COLON_CHECK("scr_area:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    scrollcopyrect_min_area);
			goto qry;
		}
		p += strlen("scr_area:");

		scrollcopyrect_min_area = atoi(p);
		rfbLog("remote_cmd: changed -scr_area to: %d\n",
		    scrollcopyrect_min_area);

	} else if (strstr(p, "scr_skip") == p) {
		char *s = scroll_skip_str;
		COLON_CHECK("scr_skip:")
		if (!s || *s == '\0') s = scroll_skip_str0;
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co, NONUL(s));
			goto qry;
		}
		p += strlen("scr_skip:");
		if (scroll_skip_str) {
			free(scroll_skip_str);
		}

		scroll_skip_str = strdup(p);
		rfbLog("remote_cmd: changed -scr_skip to: %s\n",
		    scroll_skip_str);
		initialize_scroll_matches();
	} else if (strstr(p, "scr_inc") == p) {
		char *s = scroll_good_str;
		if (!s || *s == '\0') s = scroll_good_str0;
		COLON_CHECK("scr_inc:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co, NONUL(s));
			goto qry;
		}
		p += strlen("scr_inc:");
		if (scroll_good_str) {
			free(scroll_good_str);
		}

		scroll_good_str = strdup(p);
		rfbLog("remote_cmd: changed -scr_inc to: %s\n",
		    scroll_good_str);
		initialize_scroll_matches();
	} else if (strstr(p, "scr_keys") == p) {
		COLON_CHECK("scr_keys:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(scroll_key_list_str));
			goto qry;
		}
		p += strlen("scr_keys:");
		if (scroll_key_list_str) {
			free(scroll_key_list_str);
		}

		scroll_key_list_str = strdup(p);
		rfbLog("remote_cmd: changed -scr_keys to: %s\n",
		    scroll_key_list_str);
		initialize_scroll_keys();
	} else if (strstr(p, "scr_term") == p) {
		char *s = scroll_term_str;
		if (!s || *s == '\0') s = scroll_term_str0;
		COLON_CHECK("scr_term:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co, NONUL(s));
			goto qry;
		}
		p += strlen("scr_term:");
		if (scroll_term_str) {
			free(scroll_term_str);
		}

		scroll_term_str = strdup(p);
		rfbLog("remote_cmd: changed -scr_term to: %s\n",
		    scroll_term_str);
		initialize_scroll_term();

	} else if (strstr(p, "scr_keyrepeat") == p) {
		char *s = max_keyrepeat_str;
		if (!s || *s == '\0') s = max_keyrepeat_str0;
		COLON_CHECK("scr_keyrepeat:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co, NONUL(s));
			goto qry;
		}
		p += strlen("scr_keyrepeat:");
		if (max_keyrepeat_str) {
			free(max_keyrepeat_str);
		}

		max_keyrepeat_str = strdup(p);
		rfbLog("remote_cmd: changed -scr_keyrepeat to: %s\n",
		    max_keyrepeat_str);
		initialize_max_keyrepeat();

	} else if (strstr(p, "scr_parms") == p) {
		COLON_CHECK("scr_parms:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    scroll_copyrect_str ? scroll_copyrect_str
			    : SCROLL_COPYRECT_PARMS);
			goto qry;
		}
		p += strlen("scr_parms:");
		if (*p) {
			if (scroll_copyrect_str) {
				free(scroll_copyrect_str);
			}
			set_scrollcopyrect_mode("always");
			scroll_copyrect_str = strdup(p);
			parse_scroll_copyrect();
		}
		rfbLog("remote_cmd: set -scr_parms %s.\n",
		    NONUL(scroll_copyrect_str));
		got_scrollcopyrect = 1;

	} else if (strstr(p, "scrollcopyrect") == p) {
		COLON_CHECK("scrollcopyrect:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(scroll_copyrect));
			goto qry;
		}
		p += strlen("scrollcopyrect:");

		set_scrollcopyrect_mode(p);
		rfbLog("remote_cmd: changed -scrollcopyrect mode "
		    "to: %s\n", NONUL(scroll_copyrect));
		got_scrollcopyrect = 1;
	} else if (!strcmp(p, "scr") ||
	    strstr(p, "scr:") == p) {	/* skip-cmd-list */
		COLON_CHECK("scr:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(scroll_copyrect));
			goto qry;
		}
		p += strlen("scr:");

		set_scrollcopyrect_mode(p);
		rfbLog("remote_cmd: changed -scrollcopyrect mode "
		    "to: %s\n", NONUL(scroll_copyrect));
		got_scrollcopyrect = 1;
	} else if (!strcmp(p, "noscrollcopyrect") || !strcmp(p, "noscr")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%s", p,
			    NONUL(scroll_copyrect));
			goto qry;
		}

		set_scrollcopyrect_mode("never");
		rfbLog("remote_cmd: changed -scrollcopyrect mode "
		    "to: %s\n", NONUL(scroll_copyrect));

	} else if (strstr(p, "fixscreen") == p) {
		COLON_CHECK("fixscreen:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(screen_fixup_str));
			goto qry;
		}
		p += strlen("fixscreen:");
		if (screen_fixup_str) {
			free(screen_fixup_str);
		}
		screen_fixup_str = strdup(p);
		parse_fixscreen();
		rfbLog("remote_cmd: set -fixscreen %s.\n",
		    NONUL(screen_fixup_str));

	} else if (!strcmp(p, "noxrecord")) {
		int orig = noxrecord;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, noxrecord);
			goto qry;
		}
		noxrecord = 1;
		rfbLog("set noxrecord to: %d\n", noxrecord);
		if (orig != noxrecord) {
			shutdown_xrecord();
		}
	} else if (!strcmp(p, "xrecord")) {
		int orig = noxrecord;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !noxrecord);
			goto qry;
		}
		noxrecord = 0;
		rfbLog("set noxrecord to: %d\n", noxrecord);
		if (orig != noxrecord) {
			initialize_xrecord();
		}
	} else if (!strcmp(p, "reset_record")) {
		NOTAPP
		if (use_xrecord) {
			rfbLog("resetting RECORD\n");
			check_xrecord_reset(1);
		} else {
			rfbLog("RECORD is disabled, not resetting.\n");
		}

	} else if (strstr(p, "pointer_mode") == p) {
		int pm;
		COLON_CHECK("pointer_mode:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, pointer_mode);
			goto qry;
		}
		p += strlen("pointer_mode:");
		pm = atoi(p);
		if (pm < 0 || pm > pointer_mode_max) {
			rfbLog("remote_cmd: pointer_mode out of range:"
			   " 1-%d: %d\n", pointer_mode_max, pm);
		} else {
			rfbLog("remote_cmd: setting pointer_mode %d\n", pm);
			pointer_mode = pm;
		}
	} else if (strstr(p, "pm") == p) {
		int pm;
		COLON_CHECK("pm:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, pointer_mode);
			goto qry;
		}
		p += strlen("pm:");
		pm = atoi(p);
		if (pm < 0 || pm > pointer_mode_max) {
			rfbLog("remote_cmd: pointer_mode out of range:"
			   " 1-%d: %d\n", pointer_mode_max, pm);
		} else {
			rfbLog("remote_cmd: setting pointer_mode %d\n", pm);
			pointer_mode = pm;
		}

	} else if (strstr(p, "input_skip") == p) {
		int is;
		COLON_CHECK("input_skip:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, ui_skip);
			goto qry;
		}
		p += strlen("input_skip:");
		is = atoi(p);
		rfbLog("remote_cmd: setting input_skip %d\n", is);
		ui_skip = is;

	} else if (!strcmp(p, "allinput")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, all_input);
			goto qry;
		}
		all_input = 1;
		rfbLog("enabled allinput\n");
	} else if (!strcmp(p, "noallinput")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !all_input);
			goto qry;
		}
		all_input = 0;
		rfbLog("disabled allinput\n");

	} else if (strstr(p, "input") == p) {
		int doit = 1;
		COLON_CHECK("input:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(allowed_input_str));
			goto qry;
		}
		p += strlen("input:");
		if (allowed_input_str && !strcmp(p, allowed_input_str)) {
			doit = 0;
		}
		rfbLog("remote_cmd: setting input %s\n", p);
		if (allowed_input_str) free(allowed_input_str);
		if (*p == '\0') {
			allowed_input_str = NULL;
		} else {
			allowed_input_str = strdup(p);
		}
		if (doit) {
			initialize_allowed_input();
		}
	} else if (!strcmp(p, "grabkbd")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, grab_kbd);
			goto qry;
		}
		grab_kbd = 1;
		rfbLog("enabled grab_kbd\n");
	} else if (!strcmp(p, "nograbkbd")) {
		int orig = grab_kbd;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !grab_kbd);
			goto qry;
		}
		grab_kbd = 0;
		if (orig && dpy) {
#if !NO_X11
			X_LOCK;
			XUngrabKeyboard(dpy, CurrentTime);
			X_UNLOCK;
#endif
		}
		rfbLog("disabled grab_kbd\n");
	} else if (!strcmp(p, "grabptr")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, grab_ptr);
			goto qry;
		}
		grab_ptr = 1;
		rfbLog("enabled grab_ptr\n");
	} else if (!strcmp(p, "nograbptr")) {
		int orig = grab_ptr;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !grab_ptr);
			goto qry;
		}
		grab_ptr = 0;
		if (orig && dpy) {
#if !NO_X11
			X_LOCK;
			XUngrabPointer(dpy, CurrentTime);
			X_UNLOCK;
#endif
		}
		rfbLog("disabled grab_ptr\n");

	} else if (strstr(p, "client_input") == p) {
		NOTAPP
		COLON_CHECK("client_input:")
		p += strlen("client_input:");
		set_client_input(p);

	} else if (strstr(p, "ssltimeout") == p) {
		int is;
		COLON_CHECK("ssltimeout:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    ssl_timeout_secs);
			goto qry;
		}
		p += strlen("ssltimeout:");
		is = atoi(p);
		rfbLog("remote_cmd: setting ssltimeout: %d\n", is);
		ssl_timeout_secs = is;

	} else if (strstr(p, "speeds") == p) {
		COLON_CHECK("speeds:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(speeds_str));
			goto qry;
		}
		p += strlen("speeds:");
		if (speeds_str) free(speeds_str);
		speeds_str = strdup(p);

		rfbLog("remote_cmd: setting -speeds to:\n\t'%s'\n", p);
		initialize_speeds();

	} else if (strstr(p, "wmdt") == p) {
		COLON_CHECK("wmdt:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(wmdt_str));
			goto qry;
		}
		p += strlen("wmdt:");
		if (wmdt_str) free(wmdt_str);
		wmdt_str = strdup(p);

		rfbLog("remote_cmd: setting -wmdt to: %s\n", p);

	} else if (!strcmp(p, "debug_pointer") || !strcmp(p, "dp")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_pointer);
			goto qry;
		}
		rfbLog("remote_cmd: turning on debug_pointer.\n");
		debug_pointer = 1;
	} else if (!strcmp(p, "nodebug_pointer") || !strcmp(p, "nodp")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_pointer);
			goto qry;
		}
		rfbLog("remote_cmd: turning off debug_pointer.\n");
		debug_pointer = 0;

	} else if (!strcmp(p, "debug_keyboard") || !strcmp(p, "dk")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_keyboard);
			goto qry;
		}
		rfbLog("remote_cmd: turning on debug_keyboard.\n");
		debug_keyboard = 1;
	} else if (!strcmp(p, "nodebug_keyboard") || !strcmp(p, "nodk")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_keyboard);
			goto qry;
		}
		rfbLog("remote_cmd: turning off debug_keyboard.\n");
		debug_keyboard = 0;

	} else if (strstr(p, "deferupdate") == p) {
		int d;
		COLON_CHECK("deferupdate:")
		if (query) {
			if (!screen) {
				d = defer_update;
			} else {
				d = screen->deferUpdateTime;
			}
			snprintf(buf, bufn, "ans=%s%s%d", p, co, d);
			goto qry;
		}
		p += strlen("deferupdate:");
		d = atoi(p);
		if (d < 0) d = 0;
		rfbLog("remote_cmd: setting defer to %d ms.\n", d);
		screen->deferUpdateTime = d;
		got_defer = 1;
		
	} else if (strstr(p, "defer") == p) {
		int d;
		COLON_CHECK("defer:")
		if (query) {
			if (!screen) {
				d = defer_update;
			} else {
				d = screen->deferUpdateTime;
			}
			snprintf(buf, bufn, "ans=%s%s%d", p, co, d);
			goto qry;
		}
		p += strlen("defer:");
		d = atoi(p);
		if (d < 0) d = 0;
		rfbLog("remote_cmd: setting defer to %d ms.\n", d);
		screen->deferUpdateTime = d;
		got_defer = 1;
		
	} else if (strstr(p, "wait_ui") == p) {
		double w;
		COLON_CHECK("wait_ui:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%.2f", p, co, wait_ui);
			goto qry;
		}
		p += strlen("wait_ui:");
		w = atof(p);
		if (w <= 0) w = 1.0;
		rfbLog("remote_cmd: setting wait_ui factor %.2f -> %.2f\n",
		    wait_ui, w);
		wait_ui = w;

	} else if (!strcmp(p, "wait_bog")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, wait_bog);
			goto qry;
		}
		wait_bog = 1;
		rfbLog("remote_cmd: setting wait_bog to %d\n", wait_bog);
	} else if (!strcmp(p, "nowait_bog")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !wait_bog);
			goto qry;
		}
		wait_bog = 0;
		rfbLog("remote_cmd: setting wait_bog to %d\n", wait_bog);

	} else if (strstr(p, "slow_fb") == p) {
		double w;
		COLON_CHECK("slow_fb:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%.2f", p, co, slow_fb);
			goto qry;
		}
		p += strlen("slow_fb:");
		w = atof(p);
		if (w <= 0) w = 0.0;
		rfbLog("remote_cmd: setting slow_fb factor %.2f -> %.2f\n",
		    slow_fb, w);
		slow_fb = w;

	} else if (strstr(p, "wait") == p) {
		int w;
		COLON_CHECK("wait:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, waitms);
			goto qry;
		}
		p += strlen("wait:");
		w = atoi(p);
		if (w < 0) w = 0;
		rfbLog("remote_cmd: setting wait %d -> %d ms.\n", waitms, w);
		waitms = w;

	} else if (strstr(p, "readtimeout") == p) {
		int w, orig = rfbMaxClientWait;
		COLON_CHECK("readtimeout:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    rfbMaxClientWait/1000);
			goto qry;
		}
		p += strlen("readtimeout:");
		w = atoi(p) * 1000;
		if (w <= 0) w = 0;
		rfbLog("remote_cmd: setting rfbMaxClientWait %d -> "
		    "%d msec.\n", orig, w);
		rfbMaxClientWait = w;

	} else if (!strcmp(p, "nap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, take_naps);
			    goto qry;
		}
		rfbLog("remote_cmd: turning on nap mode.\n");
		take_naps = 1;
	} else if (!strcmp(p, "nonap")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !take_naps);
			    goto qry;
		}
		rfbLog("remote_cmd: turning off nap mode.\n");
		take_naps = 0;

	} else if (strstr(p, "sb") == p) {
		int w;
		COLON_CHECK("sb:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, screen_blank);
			goto qry;
		}
		p += strlen("sb:");
		w = atoi(p);
		if (w < 0) w = 0;
		rfbLog("remote_cmd: setting screen_blank %d -> %d sec.\n",
		    screen_blank, w);
		screen_blank = w;
	} else if (strstr(p, "screen_blank") == p) {
		int w;
		COLON_CHECK("screen_blank:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, screen_blank);
			goto qry;
		}
		p += strlen("screen_blank:");
		w = atoi(p);
		if (w < 0) w = 0;
		rfbLog("remote_cmd: setting screen_blank %d -> %d sec.\n",
		    screen_blank, w);
		screen_blank = w;

	} else if (!strcmp(p, "fbpm")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !watch_fbpm);
			    goto qry;
		}
		rfbLog("remote_cmd: turning off -nofbpm mode.\n");
		watch_fbpm = 0;
	} else if (!strcmp(p, "nofbpm")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, watch_fbpm);
			    goto qry;
		}
		rfbLog("remote_cmd: turning on -nofbpm mode.\n");
		watch_fbpm = 1;

	} else if (strstr(p, "fs") == p) {
		COLON_CHECK("fs:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%f", p, co, fs_frac);
			goto qry;
		}
		p += strlen("fs:");
		fs_frac = atof(p);
		rfbLog("remote_cmd: setting -fs frac to %f\n", fs_frac);

	} else if (strstr(p, "gaps") == p) {
		int g;
		COLON_CHECK("gaps:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, gaps_fill);
			goto qry;
		}
		p += strlen("gaps:");
		g = atoi(p);
		if (g < 0) g = 0;
		rfbLog("remote_cmd: setting gaps_fill %d -> %d.\n",
		    gaps_fill, g);
		gaps_fill = g;
	} else if (strstr(p, "grow") == p) {
		int g;
		COLON_CHECK("grow:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, grow_fill);
			goto qry;
		}
		p += strlen("grow:");
		g = atoi(p);
		if (g < 0) g = 0;
		rfbLog("remote_cmd: setting grow_fill %d -> %d.\n",
		    grow_fill, g);
		grow_fill = g;
	} else if (strstr(p, "fuzz") == p) {
		int f;
		COLON_CHECK("fuzz:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, tile_fuzz);
			goto qry;
		}
		p += strlen("fuzz:");
		f = atoi(p);
		if (f < 0) f = 0;
		rfbLog("remote_cmd: setting tile_fuzz %d -> %d.\n",
		    tile_fuzz, f);
		grow_fill = f;

	} else if (!strcmp(p, "snapfb")) {
		int orig = use_snapfb;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, use_snapfb);
			    goto qry;
		}
		rfbLog("remote_cmd: turning on snapfb mode.\n");
		use_snapfb = 1;
		if (orig != use_snapfb) {
			do_new_fb(1);
		}
	} else if (!strcmp(p, "nosnapfb")) {
		int orig = use_snapfb;
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !use_snapfb);
			    goto qry;
		}
		rfbLog("remote_cmd: turning off snapfb mode.\n");
		use_snapfb = 0;
		if (orig != use_snapfb) {
			do_new_fb(1);
		}

	} else if (strstr(p, "rawfb") == p) {
		COLON_CHECK("rawfb:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(raw_fb_str));
			goto qry;
		}
		p += strlen("rawfb:");
		if (raw_fb_str) free(raw_fb_str);
		raw_fb_str = strdup(p);
		if (safe_remote_only && strstr(p, "setup:") == p) { /* skip-cmd-list */
			/* n.b. we still allow filename, shm, of rawfb */
			fprintf(stderr, "unsafe rawfb setup: %s\n", p);
			exit(1);
		}

		rfbLog("remote_cmd: setting -rawfb to:\n\t'%s'\n", p);

		if (*raw_fb_str == '\0') {
			free(raw_fb_str);
			raw_fb_str = NULL;
			if (raw_fb_mmap) {
				munmap(raw_fb_addr, raw_fb_mmap);
			}
			if (raw_fb_fd >= 0) {
				close(raw_fb_fd);
			}
			raw_fb_fd = -1;
			raw_fb = NULL;
			raw_fb_addr = NULL;
			raw_fb_offset = 0;
			raw_fb_shm = 0;
			raw_fb_mmap = 0;
			raw_fb_seek = 0;
			rfbLog("restoring per-rawfb settings...\n");
			set_raw_fb_params(1);
		}
		rfbLog("hang on tight, here we go...\n");
		raw_fb_back_to_X = 1;
		do_new_fb(1);
		raw_fb_back_to_X = 0;

	} else if (strstr(p, "uinput_accel") == p) {
		COLON_CHECK("uinput_accel:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(get_uinput_accel()));
			goto qry;
		}
		p += strlen("uinput_accel:");
		rfbLog("set_uinput_accel: %s\n", p);
		set_uinput_accel(p);

	} else if (strstr(p, "uinput_thresh") == p) {
		COLON_CHECK("uinput_thresh:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(get_uinput_thresh()));
			goto qry;
		}
		p += strlen("uinput_thresh:");
		rfbLog("set_uinput_thresh: %s\n", p);
		set_uinput_thresh(p);

	} else if (strstr(p, "uinput_reset") == p) {
		COLON_CHECK("uinput_reset:")
		p += strlen("uinput_reset:");
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    get_uinput_reset());
			goto qry;
		}
		rfbLog("set_uinput_reset: %s\n", p);
		set_uinput_reset(atoi(p));

	} else if (strstr(p, "uinput_always") == p) {
		COLON_CHECK("uinput_always:")
		p += strlen("uinput_always:");
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    get_uinput_always());
			goto qry;
		}
		rfbLog("set_uinput_always: %s\n", p);
		set_uinput_always(atoi(p));

	} else if (strstr(p, "progressive") == p) {
		int f;
		COLON_CHECK("progressive:")
		if (query) {
			if (!screen) {
				f = 0;
			} else {
				f = screen->progressiveSliceHeight;
			}
			snprintf(buf, bufn, "ans=%s%s%d", p, co, f);
			goto qry;
		}
		p += strlen("progressive:");
		f = atoi(p);
		if (f < 0) f = 0;
		rfbLog("remote_cmd: setting progressive %d -> %d.\n",
		    screen->progressiveSliceHeight, f);
		screen->progressiveSliceHeight = f;

	} else if (strstr(p, "rfbport") == p) {
		int rp, orig = screen ? screen->port : 5900;
		COLON_CHECK("rfbport:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, orig);
			goto qry;
		}
		p += strlen("rfbport:");
		rp = atoi(p);
		reset_rfbport(orig, rp);

	} else if (!strcmp(p, "http")) {
		if (query) {
			int ls = screen ? screen->httpListenSock : -1;
			snprintf(buf, bufn, "ans=%s:%d", p, (ls > -1));
			goto qry;
		}
		if (screen->httpListenSock > -1) {
			rfbLog("already listening for http connections.\n");
		} else {
			rfbLog("turning on listening for http connections.\n");
			if (check_httpdir()) {
				http_connections(1);
			}
		}
	} else if (!strcmp(p, "nohttp")) {
		if (query) {
			int ls = screen ? screen->httpListenSock : -1;
			snprintf(buf, bufn, "ans=%s:%d", p, !(ls > -1));
			goto qry;
		}
		if (screen->httpListenSock < 0) {
			rfbLog("already not listening for http connections.\n");
		} else {
			rfbLog("turning off listening for http connections.\n");
			if (check_httpdir()) {
				http_connections(0);
			}
		}

	} else if (strstr(p, "httpport") == p) {
		int hp, orig = screen ? screen->httpPort : 0;
		COLON_CHECK("httpport:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, orig);
			goto qry;
		}
		p += strlen("httpport:");
		hp = atoi(p);
		reset_httpport(orig, hp);

	} else if (strstr(p, "httpdir") == p) {
		COLON_CHECK("httpdir:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(http_dir));
			goto qry;
		}
		p += strlen("httpdir:");
		if (http_dir && !strcmp(http_dir, p)) {
			rfbLog("no change in httpdir: %s\n", http_dir);
		} else {
			if (http_dir) {
				free(http_dir);
			}
			http_dir = strdup(p);
			http_connections(0);
			if (*p != '\0') {
				http_connections(1);
			}
		}

	} else if (!strcmp(p, "enablehttpproxy")) {
		if (query) {
			int ht = screen ? screen->httpEnableProxyConnect : 0;
			snprintf(buf, bufn, "ans=%s:%d", p, ht != 0);
			goto qry;
		}
		rfbLog("turning on enablehttpproxy.\n");
		screen->httpEnableProxyConnect = 1;
	} else if (!strcmp(p, "noenablehttpproxy")) {
		if (query) {
			int ht = screen ? screen->httpEnableProxyConnect : 0;
			snprintf(buf, bufn, "ans=%s:%d", p, ht == 0);
			goto qry;
		}
		rfbLog("turning off enablehttpproxy.\n");
		screen->httpEnableProxyConnect = 0;

	} else if (!strcmp(p, "alwaysshared")) {
		if (query) {
			int t = screen ? screen->alwaysShared : 0;
			snprintf(buf, bufn, "ans=%s:%d", p, t != 0);
			goto qry;
		}
		rfbLog("turning on alwaysshared.\n");
		screen->alwaysShared = 1;
	} else if (!strcmp(p, "noalwaysshared")) {
		if (query) {
			int t = screen ? screen->alwaysShared : 0;
			snprintf(buf, bufn, "ans=%s:%d", p, t == 0);
			goto qry;
		}
		rfbLog("turning off alwaysshared.\n");
		screen->alwaysShared = 0;

	} else if (!strcmp(p, "nevershared")) {
		if (query) {
			int t = screen ? screen->neverShared : 1;
			snprintf(buf, bufn, "ans=%s:%d", p, t != 0);
			goto qry;
		}
		rfbLog("turning on nevershared.\n");
		screen->neverShared = 1;
	} else if (!strcmp(p, "noalwaysshared")) {
		if (query) {
			int t = screen ? screen->neverShared : 1;
			snprintf(buf, bufn, "ans=%s:%d", p, t == 0);
			goto qry;
		}
		rfbLog("turning off nevershared.\n");
		screen->neverShared = 0;

	} else if (!strcmp(p, "dontdisconnect")) {
		if (query) {
			int t = screen ? screen->dontDisconnect : 1;
			snprintf(buf, bufn, "ans=%s:%d", p, t != 0);
			goto qry;
		}
		rfbLog("turning on dontdisconnect.\n");
		screen->dontDisconnect = 1;
	} else if (!strcmp(p, "nodontdisconnect")) {
		if (query) {
			int t = screen ? screen->dontDisconnect : 1;
			snprintf(buf, bufn, "ans=%s:%d", p, t == 0);
			goto qry;
		}
		rfbLog("turning off dontdisconnect.\n");
		screen->dontDisconnect = 0;

	} else if (!strcmp(p, "desktop") ||
	    strstr(p, "desktop:") == p) {	/* skip-cmd-list */
		COLON_CHECK("desktop:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%s", p, co,
			    NONUL(rfb_desktop_name));
			goto qry;
		}
		p += strlen("desktop:");
		if (rfb_desktop_name) {
			free(rfb_desktop_name);
		}
		rfb_desktop_name = strdup(p);
		screen->desktopName = rfb_desktop_name;
		rfbLog("remote_cmd: setting desktop name to %s\n",
		    rfb_desktop_name);

	} else if (!strcmp(p, "debug_xevents")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_xevents);
			goto qry;
		}
		debug_xevents = 1;
		rfbLog("set debug_xevents to: %d\n", debug_xevents);
	} else if (!strcmp(p, "nodebug_xevents")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_xevents);
			goto qry;
		}
		debug_xevents = 0;
		rfbLog("set debug_xevents to: %d\n", debug_xevents);
	} else if (strstr(p, "debug_xevents") == p) {
		COLON_CHECK("debug_xevents:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, debug_xevents);
			goto qry;
		}
		p += strlen("debug_xevents:");
		debug_xevents = atoi(p);
		rfbLog("set debug_xevents to: %d\n", debug_xevents);

	} else if (!strcmp(p, "debug_xdamage")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_xdamage);
			goto qry;
		}
		debug_xdamage = 1;
		rfbLog("set debug_xdamage to: %d\n", debug_xdamage);
	} else if (!strcmp(p, "nodebug_xdamage")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_xdamage);
			goto qry;
		}
		debug_xdamage = 0;
		rfbLog("set debug_xdamage to: %d\n", debug_xdamage);
	} else if (strstr(p, "debug_xdamage") == p) {
		COLON_CHECK("debug_xdamage:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, debug_xdamage);
			goto qry;
		}
		p += strlen("debug_xdamage:");
		debug_xdamage = atoi(p);
		rfbLog("set debug_xdamage to: %d\n", debug_xdamage);

	} else if (!strcmp(p, "debug_wireframe")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_wireframe);
			goto qry;
		}
		debug_wireframe = 1;
		rfbLog("set debug_wireframe to: %d\n", debug_wireframe);
	} else if (!strcmp(p, "nodebug_wireframe")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_wireframe);
			goto qry;
		}
		debug_wireframe = 0;
		rfbLog("set debug_wireframe to: %d\n", debug_wireframe);
	} else if (strstr(p, "debug_wireframe") == p) {
		COLON_CHECK("debug_wireframe:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    debug_wireframe);
			goto qry;
		}
		p += strlen("debug_wireframe:");
		debug_wireframe = atoi(p);
		rfbLog("set debug_wireframe to: %d\n", debug_wireframe);

	} else if (!strcmp(p, "debug_scroll")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_scroll);
			goto qry;
		}
		debug_scroll = 1;
		rfbLog("set debug_scroll to: %d\n", debug_scroll);
	} else if (!strcmp(p, "nodebug_scroll")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_scroll);
			goto qry;
		}
		debug_scroll = 0;
		rfbLog("set debug_scroll to: %d\n", debug_scroll);
	} else if (strstr(p, "debug_scroll") == p) {
		COLON_CHECK("debug_scroll:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    debug_scroll);
			goto qry;
		}
		p += strlen("debug_scroll:");
		debug_scroll = atoi(p);
		rfbLog("set debug_scroll to: %d\n", debug_scroll);

	} else if (!strcmp(p, "debug_tiles") || !strcmp(p, "dbt")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_tiles);
			goto qry;
		}
		debug_tiles = 1;
		rfbLog("set debug_tiles to: %d\n", debug_tiles);
	} else if (!strcmp(p, "nodebug_tiles") || !strcmp(p, "nodbt")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_tiles);
			goto qry;
		}
		debug_tiles = 0;
		rfbLog("set debug_tiles to: %d\n", debug_tiles);
	} else if (strstr(p, "debug_tiles") == p) {
		COLON_CHECK("debug_tiles:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co,
			    debug_tiles);
			goto qry;
		}
		p += strlen("debug_tiles:");
		debug_tiles = atoi(p);
		rfbLog("set debug_tiles to: %d\n", debug_tiles);

	} else if (!strcmp(p, "debug_grabs")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_grabs);
			goto qry;
		}
		debug_grabs = 1;
		rfbLog("set debug_grabs to: %d\n", debug_grabs);
	} else if (!strcmp(p, "nodebug_grabs")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_grabs);
			goto qry;
		}
		debug_grabs = 0;
		rfbLog("set debug_grabs to: %d\n", debug_grabs);

	} else if (!strcmp(p, "debug_sel")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, debug_sel);
			goto qry;
		}
		debug_sel = 1;
		rfbLog("set debug_sel to: %d\n", debug_sel);
	} else if (!strcmp(p, "nodebug_sel")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !debug_sel);
			goto qry;
		}
		debug_sel = 0;
		rfbLog("set debug_sel to: %d\n", debug_sel);

	} else if (!strcmp(p, "dbg")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, crash_debug);
			goto qry;
		}
		crash_debug = 1;
		rfbLog("set crash_debug to: %d\n", crash_debug);
	} else if (!strcmp(p, "nodbg")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p, !crash_debug);
			goto qry;
		}
		crash_debug = 0;
		rfbLog("set crash_debug to: %d\n", crash_debug);

	} else if (strstr(p, "hack") == p) { /* skip-cmd-list */
		COLON_CHECK("hack:")
		if (query) {
			snprintf(buf, bufn, "ans=%s%s%d", p, co, hack_val);
			goto qry;
		}
		p += strlen("hack:");
		hack_val = atoi(p);
		rfbLog("set hack_val to: %d\n", hack_val);

	} else if (!strcmp(p, "noremote")) {
		if (query) {
			snprintf(buf, bufn, "ans=%s:%d", p,
			    !accept_remote_cmds);
			goto qry;
		}
		rfbLog("remote_cmd: disabling remote commands.\n");
		accept_remote_cmds = 0; /* cannot be turned back on. */

	} else if (strstr(p, "client_info_sock") == p) { /* skip-cmd-list */
		NOTAPP
		p += strlen("client_info_sock:");
		if (*p != '\0') {
			start_client_info_sock(p);
		}

	} else if (strstr(p, "noop") == p) {
		NOTAPP
		rfbLog("remote_cmd: noop\n");

	} else if (icon_mode && !query && strstr(p, "passwd") == p) { /* skip-cmd-list */
		char **passwds_new = (char **) malloc(3*sizeof(char *));
		char **passwds_old = (char **) screen->authPasswdData;

		COLON_CHECK("passwd:")
		p += strlen("passwd:");

		passwds_new[0] = strdup(p);

		if (screen->authPasswdData &&
		    screen->passwordCheck == rfbCheckPasswordByList) {
				passwds_new[1] = passwds_old[1];
		} else {
			passwds_new[1] = NULL;
			screen->passwordCheck = rfbCheckPasswordByList;
		}
		passwds_new[2] = NULL;

		screen->authPasswdData = (void*) passwds_new;
		if (*p == '\0') {
			screen->authPasswdData = (void*) NULL;
		}
		rfbLog("remote_cmd: changed full access passwd.\n");

	} else if (icon_mode && !query && strstr(p, "viewpasswd") == p) { /* skip-cmd-list */
		char **passwds_new = (char **) malloc(3*sizeof(char *));
		char **passwds_old = (char **) screen->authPasswdData;
		
		COLON_CHECK("viewpasswd:")
		p += strlen("viewpasswd:");

		passwds_new[1] = strdup(p);

		if (screen->authPasswdData &&
		    screen->passwordCheck == rfbCheckPasswordByList) {
				passwds_new[0] = passwds_old[0];
		} else {
			char *tmp = (char *) malloc(4 + CHALLENGESIZE);
			rfbRandomBytes((unsigned char*)tmp);
			passwds_new[0] = tmp;
			screen->passwordCheck = rfbCheckPasswordByList;
		}
		passwds_new[2] = NULL;

		if (*p == '\0') {
			passwds_new[1] = NULL;
		}

		screen->authPasswdData = (void*) passwds_new;
		rfbLog("remote_cmd: changed view only passwd.\n");

	} else if (strstr(p, "trayembed") == p) { /* skip-cmd-list */
		unsigned long id;
		NOTAPP

		COLON_CHECK("trayembed:")
		p += strlen("trayembed:");
		if (scan_hexdec(p, &id)) {
			tray_request = (Window) id;
			tray_unembed = 0;
			rfbLog("remote_cmd: will try to embed 0x%x in"
			    " the system tray.\n", id);
		}
	} else if (strstr(p, "trayunembed") == p) { /* skip-cmd-list */
		unsigned long id;
		NOTAPP

		COLON_CHECK("trayunembed:")
		p += strlen("trayunembed:");
		if (scan_hexdec(p, &id)) {
			tray_request = (Window) id;
			tray_unembed = 1;
			rfbLog("remote_cmd: will try to unembed 0x%x out"
			    " of the system tray.\n", id);
		}


	} else if (query) {
		/* read-only variables that can only be queried: */

		if (!strcmp(p, "display")) {
			if (raw_fb) {
				snprintf(buf, bufn, "aro=%s:rawfb:%p",
				    p, raw_fb_addr);
			} else if (! dpy) {
				snprintf(buf, bufn, "aro=%s:", p);
			} else {
				char *d;
				d = DisplayString(dpy);
				if (! d) d = "unknown";
				if (*d == ':') {
					snprintf(buf, bufn, "aro=%s:%s%s", p,
					    this_host(), d);
				} else {
					snprintf(buf, bufn, "aro=%s:%s", p, d);
				}
			}
		} else if (!strcmp(p, "vncdisplay")) {
			snprintf(buf, bufn, "aro=%s:%s", p,
			    NONUL(vnc_desktop_name));
		} else if (!strcmp(p, "desktopname")) {
			snprintf(buf, bufn, "aro=%s:%s", p,
			    NONUL(rfb_desktop_name));
		} else if (!strcmp(p, "guess_desktop")) {
			snprintf(buf, bufn, "aro=%s:%s", p,
			    NONUL(guess_desktop()));
		} else if (!strcmp(p, "http_url")) {
			if (!screen) {
				snprintf(buf, bufn, "aro=%s:", p);
			} else if (screen->httpListenSock > -1) {
				snprintf(buf, bufn, "aro=%s:http://%s:%d", p,
				    NONUL(screen->thisHost), screen->httpPort);
			} else {
				snprintf(buf, bufn, "aro=%s:%s", p,
				    "http_not_active");
			}
		} else if (!strcmp(p, "auth") || !strcmp(p, "xauth")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(auth_file));
		} else if (!strcmp(p, "users")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(users_list));
		} else if (!strcmp(p, "rootshift")) {
			snprintf(buf, bufn, "aro=%s:%d", p, rootshift);
		} else if (!strcmp(p, "clipshift")) {
			snprintf(buf, bufn, "aro=%s:%d", p, clipshift);
		} else if (!strcmp(p, "scale_str")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(scale_str));
		} else if (!strcmp(p, "scaled_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scaled_x);
		} else if (!strcmp(p, "scaled_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scaled_y);
		} else if (!strcmp(p, "scale_numer")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scale_numer);
		} else if (!strcmp(p, "scale_denom")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scale_denom);
		} else if (!strcmp(p, "scale_fac")) {
			snprintf(buf, bufn, "aro=%s:%f", p, scale_fac);
		} else if (!strcmp(p, "scaling_blend")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scaling_blend);
		} else if (!strcmp(p, "scaling_nomult4")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scaling_nomult4);
		} else if (!strcmp(p, "scaling_pad")) {
			snprintf(buf, bufn, "aro=%s:%d", p, scaling_pad);
		} else if (!strcmp(p, "scaling_interpolate")) {
			snprintf(buf, bufn, "aro=%s:%d", p,
			    scaling_interpolate);
		} else if (!strcmp(p, "inetd")) {
			snprintf(buf, bufn, "aro=%s:%d", p, inetd);
		} else if (!strcmp(p, "privremote")) {
			snprintf(buf, bufn, "aro=%s:%d", p, priv_remote);
		} else if (!strcmp(p, "unsafe")) {
			snprintf(buf, bufn, "aro=%s:%d", p, !safe_remote_only);
		} else if (!strcmp(p, "safer")) {
			snprintf(buf, bufn, "aro=%s:%d", p, more_safe);
		} else if (!strcmp(p, "nocmds")) {
			snprintf(buf, bufn, "aro=%s:%d", p, no_external_cmds);
		} else if (!strcmp(p, "passwdfile")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(passwdfile));
#ifndef NO_SSL_OR_UNIXPW
		} else if (!strcmp(p, "unixpw")) {
			snprintf(buf, bufn, "aro=%s:%d", p, unixpw);
		} else if (!strcmp(p, "unixpw_nis")) {
			snprintf(buf, bufn, "aro=%s:%d", p, unixpw_nis);
		} else if (!strcmp(p, "unixpw_list")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(unixpw_list));
		} else if (!strcmp(p, "ssl")) {
			snprintf(buf, bufn, "aro=%s:%d", p, use_openssl);
		} else if (!strcmp(p, "ssl_pem")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(openssl_pem));
		} else if (!strcmp(p, "sslverify")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(ssl_verify));
		} else if (!strcmp(p, "stunnel")) {
			snprintf(buf, bufn, "aro=%s:%d", p, use_stunnel);
		} else if (!strcmp(p, "stunnel_pem")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(stunnel_pem));
		} else if (!strcmp(p, "https")) {
			snprintf(buf, bufn, "aro=%s:%d", p, https_port_num);
#endif
		} else if (!strcmp(p, "usepw")) {
			snprintf(buf, bufn, "aro=%s:%d", p, usepw);
		} else if (!strcmp(p, "using_shm")) {
			snprintf(buf, bufn, "aro=%s:%d", p, !using_shm);
		} else if (!strcmp(p, "logfile") || !strcmp(p, "o")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(logfile));
		} else if (!strcmp(p, "flag")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(flagfile));
		} else if (!strcmp(p, "rc")) {
			char *s = rc_rcfile;
			if (rc_rcfile_default) {
				s = NULL;
			}
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(s));
		} else if (!strcmp(p, "norc")) {
			snprintf(buf, bufn, "aro=%s:%d", p, got_norc);
		} else if (!strcmp(p, "h") || !strcmp(p, "help") ||
		    !strcmp(p, "V") || !strcmp(p, "version") ||
		    !strcmp(p, "lastmod")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(lastmod));
		} else if (!strcmp(p, "bg")) {
			snprintf(buf, bufn, "aro=%s:%d", p, opts_bg);
		} else if (!strcmp(p, "sigpipe")) {
			snprintf(buf, bufn, "aro=%s:%s", p, NONUL(sigpipe));
		} else if (!strcmp(p, "threads")) {
			snprintf(buf, bufn, "aro=%s:%d", p, use_threads);
		} else if (!strcmp(p, "readrate")) {
			snprintf(buf, bufn, "aro=%s:%d", p, get_read_rate());
		} else if (!strcmp(p, "netrate")) {
			snprintf(buf, bufn, "aro=%s:%d", p, get_net_rate());
		} else if (!strcmp(p, "netlatency")) {
			snprintf(buf, bufn, "aro=%s:%d", p, get_net_latency());
		} else if (!strcmp(p, "pipeinput")) {
			snprintf(buf, bufn, "aro=%s:%s", p,
			    NONUL(pipeinput_str));
		} else if (!strcmp(p, "clients")) {
			char *str = list_clients();
			snprintf(buf, bufn, "aro=%s:%s", p, str);
			free(str);
		} else if (!strcmp(p, "client_count")) {
			snprintf(buf, bufn, "aro=%s:%d", p, client_count);
		} else if (!strcmp(p, "pid")) {
			snprintf(buf, bufn, "aro=%s:%d", p, (int) getpid());
		} else if (!strcmp(p, "ext_xtest")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xtest_present);
		} else if (!strcmp(p, "ext_xtrap")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xtrap_present);
		} else if (!strcmp(p, "ext_xrecord")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xrecord_present);
		} else if (!strcmp(p, "ext_xkb")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xkb_present);
		} else if (!strcmp(p, "ext_xshm")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xshm_present);
		} else if (!strcmp(p, "ext_xinerama")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xinerama_present);
		} else if (!strcmp(p, "ext_overlay")) {
			snprintf(buf, bufn, "aro=%s:%d", p, overlay_present);
		} else if (!strcmp(p, "ext_xfixes")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xfixes_present);
		} else if (!strcmp(p, "ext_xdamage")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xdamage_present);
		} else if (!strcmp(p, "ext_xrandr")) {
			snprintf(buf, bufn, "aro=%s:%d", p, xrandr_present);
		} else if (!strcmp(p, "rootwin")) {
			snprintf(buf, bufn, "aro=%s:0x%x", p,
			    (unsigned int) rootwin);
		} else if (!strcmp(p, "num_buttons")) {
			snprintf(buf, bufn, "aro=%s:%d", p, num_buttons);
		} else if (!strcmp(p, "button_mask")) {
			snprintf(buf, bufn, "aro=%s:%d", p, button_mask);
		} else if (!strcmp(p, "mouse_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, cursor_x);
		} else if (!strcmp(p, "mouse_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, cursor_y);
		} else if (!strcmp(p, "bpp")) {
			snprintf(buf, bufn, "aro=%s:%d", p, bpp);
		} else if (!strcmp(p, "depth")) {
			snprintf(buf, bufn, "aro=%s:%d", p, depth);
		} else if (!strcmp(p, "indexed_color")) {
			snprintf(buf, bufn, "aro=%s:%d", p, indexed_color);
		} else if (!strcmp(p, "dpy_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, dpy_x);
		} else if (!strcmp(p, "dpy_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, dpy_y);
		} else if (!strcmp(p, "wdpy_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, wdpy_x);
		} else if (!strcmp(p, "wdpy_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, wdpy_y);
		} else if (!strcmp(p, "off_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, off_x);
		} else if (!strcmp(p, "off_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, off_y);
		} else if (!strcmp(p, "cdpy_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, cdpy_x);
		} else if (!strcmp(p, "cdpy_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, cdpy_y);
		} else if (!strcmp(p, "coff_x")) {
			snprintf(buf, bufn, "aro=%s:%d", p, coff_x);
		} else if (!strcmp(p, "coff_y")) {
			snprintf(buf, bufn, "aro=%s:%d", p, coff_y);
		} else if (!strcmp(p, "rfbauth")) {
			NOTAPPRO
		} else if (!strcmp(p, "passwd")) {
			NOTAPPRO
		} else if (!strcmp(p, "viewpasswd")) {
			NOTAPPRO
		} else {
			NOTAPP
		}
		goto qry;
	} else {
		char tmp[100];
		NOTAPP
		rfbLog("remote_cmd: warning unknown\n");
		strncpy(tmp, p, 90);
		rfbLog("command \"%s\"\n", tmp);
		goto done;
	}

	done:

	if (*buf == '\0') {
		sprintf(buf, "ack=1");
	}

	qry:

	if (stringonly) {
		return strdup(buf);
	} else if (client_connect_file) {
		FILE *out = fopen(client_connect_file, "w");
		if (out != NULL) {
			fprintf(out, "%s\n", buf);
			fclose(out);
			usleep(20*1000);
		}
	} else {
		if (dpy) {	/* raw_fb hack */
			set_x11vnc_remote_prop(buf);
			XFlush_wr(dpy);
		}
	}
#endif
	return NULL;
}


