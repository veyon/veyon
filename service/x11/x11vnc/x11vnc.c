/*
 * x11vnc: a VNC server for X displays.
 *
 * Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 *
 *  This file is part of x11vnc.
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
 *  In addition, as a special exception, Karl J. Runge
 *  gives permission to link the code of its release of x11vnc with the
 *  OpenSSL project's "OpenSSL" library (or with modified versions of it
 *  that use the same license as the "OpenSSL" library), and distribute
 *  the linked executables.  You must obey the GNU General Public License
 *  in all respects for all of the code used other than "OpenSSL".  If you
 *  modify this file, you may extend this exception to your version of the
 *  file, but you are not obligated to do so.  If you do not wish to do
 *  so, delete this exception statement from your version.
 */

/*
 * This program is based on some ideas from the following programs:
 *
 *       the initial x11vnc.c in libvncserver (Johannes E. Schindelin)
 *	 x0rfbserver, the original native X vnc server (Jens Wagner)
 *       krfb, the KDE desktopsharing project (Tim Jansen)
 *
 * Please see http://www.karlrunge.com/x11vnc for the most up-to-date
 * information about x11vnc.  Some of the following text may be out
 * of date.
 *
 * The primary goal of this program is to create a portable and simple
 * command-line server utility that allows a VNC viewer to connect
 * to an actual X display (as the above do).  The only non-standard
 * dependency of this program is the static library libvncserver.a.
 * Although in some environments libjpeg.so or libz.so may not be
 * readily available and needs to be installed, they may be found
 * at ftp://ftp.uu.net/graphics/jpeg/ and http://www.gzip.org/zlib/,
 * respectively.  To increase portability it is written in plain C.
 *
 * Another goal is to improve performance and interactive response.
 * The algorithm of x0rfbserver was used as a base.  Many additional
 * heuristics are also applied.
 *
 * Another goal is to add many features that enable and incourage creative
 * usage and application of the tool.  Apologies for the large number
 * of options!
 *
 * To build:
 *
 * Obtain the libvncserver package (http://libvncserver.sourceforge.net).
 * As of 12/2002 this version of x11vnc.c is contained in the libvncserver
 * CVS tree and released in version 0.5.
 *
 * gcc should be used on all platforms.  To build a threaded version put
 * "-D_REENTRANT -DX11VNC_THREADED" in the environment variable CFLAGS
 * or CPPFLAGS (e.g. before running the libvncserver configure).  The
 * threaded mode is a bit more responsive, but can be unstable (e.g.
 * if more than one client the same tight or zrle encoding).
 *
 * Known shortcomings:
 *
 * The screen updates are good, but of course not perfect since the X
 * display must be continuously polled and read for changes and this is
 * slow for most hardware. This can be contrasted with receiving a change
 * callback from the X server, if that were generally possible... (UPDATE:
 * this is handled now with the X DAMAGE extension, but unfortunately
 * that doesn't seem to address the slow read from the video h/w).  So,
 * e.g., opaque moves and similar window activity can be very painful;
 * one has to modify one's behavior a bit.
 *
 * General audio at the remote display is lost unless one separately
 * sets up some audio side-channel such as esd.
 *
 * It does not appear possible to query the X server for the current
 * cursor shape.  We can use XTest to compare cursor to current window's
 * cursor, but we cannot extract what the cursor is... (UPDATE: we now
 * use XFIXES extension for this.  Also on Solaris and IRIX Overlay
 * extensions exists that allow drawing the mouse into the framebuffer)
 * 
 * The current *position* of the remote X mouse pointer is shown with
 * the -cursor option.  Further, if -cursor X is used, a trick
 * is done to at least show the root window cursor vs non-root cursor.
 * (perhaps some heuristic can be done to further distinguish cases...,
 * currently "-cursor some" is a first hack at this)
 *
 * Under XFIXES mode for showing the cursor shape, the cursor may be
 * poorly approximated if it has transparency (alpha channel).
 *
 * Windows using visuals other than the default X visual may have
 * their colors messed up.  When using 8bpp indexed color, the colormap
 * is attempted to be followed, but may become out of date.  Use the
 * -flashcmap option to have colormap flashing as the pointer moves
 * windows with private colormaps (slow).  Displays with mixed depth 8 and
 * 24 visuals will incorrectly display windows using the non-default one.
 * On Sun and Sgi hardware we can to work around this with -overlay.
 *
 * Feature -id <windowid> can be picky: it can crash for things like
 * the window not sufficiently mapped into server memory, etc (UPDATE:
 * we now use the -xrandr mechanisms to trap errors more robustly for
 * this mode).  SaveUnders menus, popups, etc will not be seen.
 *
 * Under some situations the keysym unmapping is not correct, especially
 * if the two keyboards correspond to different languages.  The -modtweak
 * option is the default and corrects most problems. One can use the
 * -xkb option to try to use the XKEYBOARD extension to clear up any
 * remaining problems.
 *
 * Occasionally, a few tile updates can be missed leaving a patch of
 * color that needs to be refreshed.  This may only be when threaded,
 * which is no longer the default.
 *
 * There seems to be a serious bug with simultaneous clients when
 * threaded, currently the only workaround in this case is -nothreads
 * (which is now the default).
 */


/* -- x11vnc.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xdamage.h"
#include "xrecord.h"
#include "xevents.h"
#include "xinerama.h"
#include "xrandr.h"
#include "xkb_bell.h"
#include "win_utils.h"
#include "remote.h"
#include "scan.h"
#include "gui.h"
#include "help.h"
#include "user.h"
#include "cleanup.h"
#include "keyboard.h"
#include "pointer.h"
#include "cursor.h"
#include "userinput.h"
#include "screen.h"
#include "connections.h"
#include "rates.h"
#include "unixpw.h"
#include "inet.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "selection.h"
#include "pm.h"
#include "solid.h"

/*
 * main routine for the x11vnc program
 */
void watch_loop(void);

static int limit_shm(void);
static void check_rcfile(int argc, char **argv);
static void immediate_switch_user(int argc, char* argv[]);
static void print_settings(int try_http, int bg, char *gui_str);
static void check_loop_mode(int argc, char* argv[], int force);
static void check_appshare_mode(int argc, char* argv[]);

static int tsdo_timeout_flag;

static void tsdo_timeout (int sig) {
	tsdo_timeout_flag = 1;
	if (sig) {};
}

#define TASKMAX 32
static pid_t ts_tasks[TASKMAX];
static int ts_taskn = -1;

int tsdo(int port, int lsock, int *conn) {
	int csock, rsock, i, db = 1;
	pid_t pid;
	struct sockaddr_in addr;
#ifdef __hpux
	int addrlen = sizeof(addr);
#else
	socklen_t addrlen = sizeof(addr);
#endif

	if (*conn < 0) {
		signal(SIGALRM, tsdo_timeout);
		tsdo_timeout_flag = 0;

		alarm(10);
		csock = accept(lsock, (struct sockaddr *)&addr, &addrlen);
		alarm(0);

		if (db) rfbLog("tsdo: accept: lsock: %d, csock: %d, port: %d\n", lsock, csock, port);

		if (tsdo_timeout_flag > 0 || csock < 0) {
			close(csock);
			*conn = -1;
			return 1;
		}
		*conn = csock;
	} else {
		csock = *conn;
		if (db) rfbLog("tsdo: using existing csock: %d, port: %d\n", csock, port);
	}

	rsock = connect_tcp("127.0.0.1", port);
	if (rsock < 0) {
		if (db) rfbLog("tsdo: connect_tcp(port=%d) failed.\n", port);
		close(csock);
		return 2;
	}

	pid = fork();
	if (pid < 0) {
		close(csock);
		close(rsock);
		return 3;
	}
	if (pid > 0) {
		ts_taskn = (ts_taskn+1) % TASKMAX;
		ts_tasks[ts_taskn] = pid;
		close(csock);
		close(rsock);
		*conn = -1;
		return 0;
	}
	if (pid == 0) {
		for (i=0; i<255; i++) {
			if (i != csock && i != rsock && i != 2) {
				close(i);
			}
		}
#if LIBVNCSERVER_HAVE_SETSID
		if (setsid() == -1) {
			perror("setsid");
			close(csock);
			close(rsock);
			exit(1);
		}
#else
		if (setpgrp() == -1) {
			perror("setpgrp");
			close(csock);
			close(rsock);
			exit(1);
		}
#endif	/* SETSID */
		raw_xfer(rsock, csock, csock);
		close(csock);
		close(rsock);
		exit(0);
	}
	return 0;
}

void set_redir_properties(void);

#define TSMAX 32
#define TSSTK 16

void terminal_services(char *list) {
	int i, j, n, db = 1;
	char *p, *q, *r, *str;
#if !NO_X11
	char *tag[TSMAX];
	int listen[TSMAX], redir[TSMAX][TSSTK], socks[TSMAX], tstk[TSSTK];
	double rate_start;
	int rate_count;
	Atom at, atom[TSMAX];
	fd_set rd;
	Window rwin;
	XErrorHandler   old_handler1;
	XIOErrorHandler old_handler2;
	char num[32];
	time_t last_clean = time(NULL);

	if (getenv("TS_REDIR_DEBUG")) {
		db = 2;
	}

	if (! dpy) {
		return;
	}

	rwin = RootWindow(dpy, DefaultScreen(dpy));

	at = XInternAtom(dpy, "TS_REDIR_LIST", False);
	if (at != None) {
		XChangeProperty(dpy, rwin, at, XA_STRING, 8,
		    PropModeReplace, (unsigned char *)list, strlen(list));
		XSync(dpy, False);
	}
	if (db) fprintf(stderr, "TS_REDIR_LIST Atom: %d.\n", (int) at);

	oh_restart_it_all:

	for (i=0; i<TASKMAX; i++) {
		ts_tasks[i] = 0;
	}
	for (i=0; i<TSMAX; i++) {
		socks[i] = -1;
		listen[i] = -1;
		for (j=0; j<TSSTK; j++) {
			redir[i][j] = 0;
		}
	}

	rate_start = 0.0;
	rate_count = 0;

	n = 0;
	str = strdup(list);
	p = strtok(str, ",");
	while (p) {
		int m1, m2;
		if (db) fprintf(stderr, "item: %s\n", p);
		q = strrchr(p, ':');
		if (!q) {
			p = strtok(NULL, ",");
			continue;
		}
		r = strchr(p, ':');
		if (!r || r == q) {
			p = strtok(NULL, ",");
			continue;
		}

		m1 = atoi(q+1);
		*q = '\0';
		m2 = atoi(r+1);
		*r = '\0';

		if (m1 <= 0 || m2 <= 0 || m1 >= 0xffff || m2 >= 0xffff) {
			p = strtok(NULL, ",");
			continue;
		}

		redir[n][0] = m1;
		listen[n] = m2;
		tag[n] = strdup(p);

		if (db) fprintf(stderr, "     %d %d %s\n", redir[n][0], listen[n], tag[n]);

		*r = ':';
		*q = ':';

		n++;
		if (n >= TSMAX) {
			break;
		}
		p = strtok(NULL, ",");
	}
	free(str);

	if (n==0) {
		return;
	}

	at = XInternAtom(dpy, "TS_REDIR_PID", False);
	if (at != None) {
		sprintf(num, "%d", getpid());
		XChangeProperty(dpy, rwin, at, XA_STRING, 8,
		    PropModeReplace, (unsigned char *)num, strlen(num));
		XSync(dpy, False);
	}

	for (i=0; i<n; i++) {
		int k;
		atom[i] = XInternAtom(dpy, tag[i], False);
		if (db) fprintf(stderr, "tag: %s atom: %d\n", tag[i], (int) atom[i]);
		if (atom[i] == None) {
			continue;
		}
		sprintf(num, "%d", redir[i][0]);
		if (db) fprintf(stderr, "     listen: %d  redir: %s\n", listen[i], num);
		XChangeProperty(dpy, rwin, atom[i], XA_STRING, 8,
		    PropModeReplace, (unsigned char *)num, strlen(num));
		XSync(dpy, False);

		for (k=1; k <= 5; k++) {
			/* XXX ::1 fallback? */
			socks[i] = listen_tcp(listen[i], htonl(INADDR_LOOPBACK), 1);
			if (socks[i] >= 0) {
				if (db) fprintf(stderr, "     listen succeeded: %d\n", listen[i]);
				break;
			}
			if (db) fprintf(stderr, "     listen failed***: %d\n", listen[i]);
			usleep(k * 2000*1000);
		}
	}

	if (getenv("TSD_RESTART")) {
		if (!strcmp(getenv("TSD_RESTART"), "1")) {
			set_redir_properties();
		}
	}

	while (1) {
		struct timeval tv;
		int nfd;
		int fmax = -1;

		tv.tv_sec  = 3;
		tv.tv_usec = 0;

		FD_ZERO(&rd);
		for (i=0; i<n; i++) {
			if (socks[i] >= 0) {
				FD_SET(socks[i], &rd);
				if (socks[i] > fmax) {
					fmax = socks[i];
				}
			}
		}

		nfd = select(fmax+1, &rd, NULL, NULL, &tv);

		if (db && 0) fprintf(stderr, "nfd=%d\n", nfd);
		if (nfd < 0 && errno == EINTR) {
			XSync(dpy, True);
			continue;
		}
		if (nfd > 0) {
			int did_ts = 0;
			for(i=0; i<n; i++) {
				int k = 0;
				for (j = 0; j < TSSTK; j++) {
					tstk[j] = 0;
				}
				for (j = 0; j < TSSTK; j++) {
					if (redir[i][j] != 0) {
						tstk[k++] = redir[i][j];
					}
				}
				for (j = 0; j < TSSTK; j++) {
					redir[i][j] = tstk[j];
if (tstk[j] != 0) fprintf(stderr, "B redir[%d][%d] = %d  %s\n", i, j, tstk[j], tag[i]);
				}
			}
			for(i=0; i<n; i++) {
				int s = socks[i];
				if (s < 0) {
					continue;
				}
				if (FD_ISSET(s, &rd)) {
					int p0, p, found = -1, jzero = -1;
					int conn = -1;

					get_prop(num, 32, atom[i], None);
					p0 = atoi(num);

					for (j = TSSTK-1; j >= 0; j--) {
						if (redir[i][j] == 0) {
							jzero = j;
							continue;
						}
						if (p0 > 0 && p0 < 0xffff) {
							if (redir[i][j] == p0) {
								found = j;
								break;
							}
						}
					}
					if (jzero < 0) {
						jzero = TSSTK-1;
					}
					if (found < 0) {
						if (p0 > 0 && p0 < 0xffff) {
							redir[i][jzero] = p0;
						}
					}
					for (j = TSSTK-1; j >= 0; j--) {
						int rc;
						p = redir[i][j];
						if (p <= 0 || p >= 0xffff) {
							redir[i][j] = 0;
							continue;
						}

						if (dnow() > rate_start + 10.0) {
							rate_start = dnow();
							rate_count = 0;
						}
						rate_count++;

						rc = tsdo(p, s, &conn);
						did_ts++;

						if (rc == 0) {
							/* AOK */
							if (db) fprintf(stderr, "tsdo[%d] OK: %d\n", i, p);
							if (p != p0) {
								sprintf(num, "%d", p);
								XChangeProperty(dpy, rwin, atom[i], XA_STRING, 8,
								    PropModeReplace, (unsigned char *)num, strlen(num));
								XSync(dpy, False);
							}
							break;
						} else if (rc == 1) {
							/* accept failed */
							if (db) fprintf(stderr, "tsdo[%d] accept failed: %d, sleep 50ms\n", i, p);
							usleep(50*1000);
							break;
						} else if (rc == 2) {
							/* connect failed */
							if (db) fprintf(stderr, "tsdo[%d] connect failed: %d, sleep 50ms  rate: %d/10s\n", i, p, rate_count);
							redir[i][j] = 0;
							usleep(50*1000);
							continue;
						} else if (rc == 3) {
							/* fork failed */
							usleep(500*1000);
							break;
						}
					}
					for (j = 0; j < TSSTK; j++) {
						if (redir[i][j] != 0) fprintf(stderr, "A redir[%d][%d] = %d  %s\n", i, j, redir[i][j], tag[i]);
					}
				}
			}
			if (did_ts && rate_count > 100) {
				int db_netstat = 1;
				char dcmd[100];

				if (no_external_cmds) {
					db_netstat = 0;
				}

				rfbLog("terminal_services: throttling high connect rate %d/10s\n", rate_count);
				usleep(2*1000*1000);
				rfbLog("terminal_services: stopping ts services.\n");
				for(i=0; i<n; i++) {
					int s = socks[i];
					if (s < 0) {
						continue;
					}
					rfbLog("terminal_services: closing listen=%d sock=%d.\n", listen[i], socks[i]);
					if (listen[i] >= 0 && db_netstat) {
						sprintf(dcmd, "netstat -an | grep -w '%d'", listen[i]);
						fprintf(stderr, "#1 %s\n", dcmd);
						system(dcmd);
					}
					close(s);
					socks[i] = -1;
					usleep(2*1000*1000);
					if (listen[i] >= 0 && db_netstat) {
						fprintf(stderr, "#2 %s\n", dcmd);
						system(dcmd);
					}
				}
				usleep(10*1000*1000);

				rfbLog("terminal_services: restarting ts services\n");
				goto oh_restart_it_all;
			}
		}
		for (i=0; i<TASKMAX; i++) {
			pid_t p = ts_tasks[i];
			if (p > 0) {
				int status;
				pid_t p2 = waitpid(p, &status, WNOHANG); 
				if (p2 == p) {
					ts_tasks[i] = 0;
				}
			}
		}
		/* this is to drop events and exit when X server is gone. */
		old_handler1 = XSetErrorHandler(trap_xerror);
		old_handler2 = XSetIOErrorHandler(trap_xioerror);
		trapped_xerror = 0;
		trapped_xioerror = 0;

		XSync(dpy, True);

		sprintf(num, "%d", (int) time(NULL));
		at = XInternAtom(dpy, "TS_REDIR", False);
		if (at != None) {
			XChangeProperty(dpy, rwin, at, XA_STRING, 8,
			    PropModeReplace, (unsigned char *)num, strlen(num));
			XSync(dpy, False);
		}
		if (time(NULL) > last_clean + 20 * 60) {
			int i, j;
			for(i=0; i<n; i++) {
				int first = 1;
				for (j = TSSTK-1; j >= 0; j--) {
					int s, p = redir[i][j];
					if (p <= 0 || p >= 0xffff) {
						redir[i][j] = 0;
						continue;
					}
					s = connect_tcp("127.0.0.1", p);
					if (s < 0) {
						redir[i][j] = 0;
						if (db) fprintf(stderr, "tsdo[%d][%d] clean: connect failed: %d\n", i, j, p);
					} else {
						close(s);
						if (first) {
							sprintf(num, "%d", p);
							XChangeProperty(dpy, rwin, atom[i], XA_STRING, 8,
							    PropModeReplace, (unsigned char *)num, strlen(num));
							XSync(dpy, False);
						}
						first = 0;
					}
					usleep(500*1000);
				}
			}
			last_clean = time(NULL);
		}
		if (trapped_xerror || trapped_xioerror) {
			if (db) fprintf(stderr, "Xerror: %d/%d\n", trapped_xerror, trapped_xioerror);
			exit(0);
		}
		XSetErrorHandler(old_handler1);
		XSetIOErrorHandler(old_handler2);
	}
#endif
}

char *ts_services[][2] = {
	{"FD_CUPS", "TS_CUPS_REDIR"},
	{"FD_SMB",  "TS_SMB_REDIR"},
	{"FD_ESD",  "TS_ESD_REDIR"},
	{"FD_NAS",  "TS_NAS_REDIR"},
	{NULL, NULL}
};

void do_tsd(void) {
#if !NO_X11
	Atom a;
	char prop[513];
	pid_t pid;
	char *cmd;
	int n, sz = 0;
	char *disp = DisplayString(dpy);
	char *logfile = getenv("TS_REDIR_LOGFILE");
	int db = 0;

	if (getenv("TS_REDIR_DEBUG")) {
		db = 1;
	}
	if (db) fprintf(stderr, "do_tsd() in.\n");

	prop[0] = '\0';
	a = XInternAtom(dpy, "TS_REDIR_LIST", False);
	if (a != None) {
		get_prop(prop, 512, a, None);
	}
	if (db) fprintf(stderr, "TS_REDIR_LIST Atom: %d = '%s'\n", (int) a, prop);

	if (prop[0] == '\0') {
		return;
	}

	if (! program_name) {
		program_name = "x11vnc";
	}
	sz += strlen(program_name) + 1;
	sz += strlen("-display") + 1;
	sz += strlen(disp) + 1;
	sz += strlen("-tsd") + 1;
	sz += 1 + strlen(prop) + 1 + 1;
	sz += strlen("-env TSD_RESTART=1") + 1;
	sz += strlen("</dev/null 1>/dev/null 2>&1") + 1;
	sz += strlen(" &") + 1;
	if (logfile) {
		sz += strlen(logfile);
	}
	if (ipv6_listen) {
		sz += strlen("-6") + 1;
	}

	cmd = (char *) malloc(sz);

	if (getenv("XAUTHORITY")) {
		char *xauth = getenv("XAUTHORITY");
		if (!strcmp(xauth, "") || access(xauth, R_OK) != 0) {
			*(xauth-2) = '_';	/* yow */
		}
	}
	sprintf(cmd, "%s -display %s -tsd '%s' -env TSD_RESTART=1 %s </dev/null 1>%s 2>&1 &",
	    program_name, disp, prop, ipv6_listen ? "-6" : "",
	    logfile ? logfile : "/dev/null" ); 
	rfbLog("running: %s\n", cmd);

#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SETSID
	/* fork into the background now */
	if ((pid = fork()) > 0)  {
		pid_t pidw;
		int status;
		double s = dnow();

		while (dnow() < s + 1.5) {
			pidw = waitpid(pid, &status, WNOHANG);
			if (pidw == pid) {
				break;
			}
			usleep(100*1000);
		}
		return;

	} else if (pid == -1) {
		system(cmd);
	} else {
		setsid();
		/* adjust our stdio */
		n = open("/dev/null", O_RDONLY);
		dup2(n, 0);
		dup2(n, 1);
		dup2(n, 2);
		if (n > 2) {
			close(n);
		}
		system(cmd);
		exit(0);
	}
#else
	system(cmd);
#endif

#endif
}

void set_redir_properties(void) {
#if !NO_X11
	char *e, *f, *t;
	Atom a;
	char num[32];
	int i, p;

	if (! dpy) {
		return;
	}

	i = 0;
	while (ts_services[i][0] != NULL) {
		f = ts_services[i][0]; 
		t = ts_services[i][1]; 
		e = getenv(f);
		if (!e || strstr(e, "DAEMON-") != e) {
			i++;
			continue;
		}
		p = atoi(e + strlen("DAEMON-"));
		if (p <= 0) {
			i++;
			continue;
		}
		sprintf(num, "%d", p);
		a = XInternAtom(dpy, t, False);
		if (a != None) {
			Window rwin = RootWindow(dpy, DefaultScreen(dpy));
fprintf(stderr, "Set: %s %s %s -> %s\n", f, t, e, num);
			XChangeProperty(dpy, rwin, a, XA_STRING, 8,
			    PropModeReplace, (unsigned char *) num, strlen(num));
			XSync(dpy, False);
		}
		i++;
	}
#endif
}

static void check_redir_services(void) {
#if !NO_X11
	Atom a;
	char prop[513];
	time_t tsd_last;
	int restart = 0;
	pid_t pid = 0;
	int db = 0;
	db = 0;

	if (getenv("TS_REDIR_DEBUG")) {
		db = 1;
	}
	if (db) fprintf(stderr, "check_redir_services in.\n");

	if (! dpy) {
		return;
	}

	a = XInternAtom(dpy, "TS_REDIR_PID", False);
	if (a != None) {
		prop[0] = '\0';
		get_prop(prop, 512, a, None);
		if (prop[0] != '\0') {
			pid = (pid_t) atoi(prop);
		}
	}
	if (db) fprintf(stderr, "TS_REDIR_PID Atom: %d = '%s'\n", (int) a, prop);

	if (getenv("FD_TAG") && strcmp(getenv("FD_TAG"), "")) {
		a = XInternAtom(dpy, "FD_TAG", False);
		if (a != None) {
			Window rwin = RootWindow(dpy, DefaultScreen(dpy));
			char *tag = getenv("FD_TAG");
			XChangeProperty(dpy, rwin, a, XA_STRING, 8,
			    PropModeReplace, (unsigned char *)tag, strlen(tag));
			XSync(dpy, False);
		}
		if (db) fprintf(stderr, "FD_TAG Atom: %d = '%s'\n", (int) a, prop);
	}

	prop[0] = '\0';
	a = XInternAtom(dpy, "TS_REDIR", False);
	if (a != None) {
		get_prop(prop, 512, a, None);
	}
	if (db) fprintf(stderr, "TS_REDIR Atom: %d = '%s'\n", (int) a, prop);
	if (prop[0] == '\0') {
		rfbLog("TS_REDIR is empty, restarting...\n");
		restart = 1;
	} else {
		tsd_last = (time_t) atoi(prop);
		if (time(NULL) > tsd_last + 30) {
			rfbLog("TS_REDIR seems dead for: %d sec, restarting...\n",
			    time(NULL) - tsd_last);
			restart = 1;
		} else if (pid > 0 && time(NULL) > tsd_last + 6) {
			if (kill(pid, 0) != 0) {
				rfbLog("TS_REDIR seems dead via kill(%d, 0), restarting...\n",
				    pid);
				restart = 1;
			}
		}
	}
	if (restart) {

		if (pid > 1) {
			rfbLog("killing TS_REDIR_PID: %d\n", pid);
			kill(pid, SIGTERM);
			usleep(500*1000);
			kill(pid, SIGKILL);
		}
		do_tsd();
		if (db) fprintf(stderr, "check_redir_services restarted.\n");
		return;
	}

	if (db) fprintf(stderr, "check_redir_services, no restart, calling set_redir_properties.\n");
	set_redir_properties();
#endif
}

void ssh_remote_tunnel(char *instr, int lport) {
#ifndef WIN32
	char *q, *cmd, *ssh;
	char *s = strdup(instr);
	int sleep = 300, disp = 0, sport = 0;
	int rc, len, rport;

	/* user@host:port:disp+secs */

	/* +sleep */
	q = strrchr(s, '+');
	if (q) {
		sleep = atoi(q+1);
		if (sleep <= 0) {
			sleep = 1;
		}
		*q = '\0';
	}
	/* :disp */
	q = strrchr(s, ':');
	if (q) {
		disp = atoi(q+1);
		*q = '\0';
	}
	
	/* :sshport */
	q = strrchr(s, ':');
	if (q) {
		sport = atoi(q+1);
		*q = '\0';
	}

	if (getenv("SSH")) {
		ssh = getenv("SSH");
	} else {
		ssh = "ssh";
	}

	len = 0;
	len += strlen(ssh) + strlen(s) + 500;
	cmd = (char *) malloc(len);

	if (disp >= 0 && disp <= 200) {
		rport = disp + 5900;
	} else if (disp < 0) {
		rport = -disp;
	} else {
		rport = disp;
	}

	if (sport > 0) {
		sprintf(cmd, "%s -f -p %d -R '%d:localhost:%d' '%s' 'sleep %d'", ssh, sport, rport, lport, s, sleep);
	} else {
		sprintf(cmd, "%s -f       -R '%d:localhost:%d' '%s' 'sleep %d'", ssh,        rport, lport, s, sleep);
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

	if (rc != 0) {
		free(cmd);
		free(s);
		rfbLog("ssh remote listen failed.\n");
		clean_up_exit(1);
	}

	if (1) {
		FILE *pipe;
		int mypid = (int) getpid();
		int bestpid = -1;
		int best = -1;
		char line[1024];
		char *psef = "ps -ef";
		char *psww = "ps wwwwwwaux";
		char *ps = psef;
		/* not portable... but it is really good to terminate the ssh when done. */
		/* ps -ef | egrep 'ssh2.*-R.*5907:localhost:5900.*runge@celias.lbl.gov.*sleep 300' | grep -v grep | awk '{print $2}' */
		if (strstr(UT.sysname, "Linux")) {
			ps = psww;
		} else if (strstr(UT.sysname, "BSD")) {
			ps = psww;
		} else if (strstr(UT.sysname, "Darwin")) {
			ps = psww;
		}
		sprintf(cmd, "env COLUMNS=256 %s | egrep '%s.*-R *%d:localhost:%d.*%s.*sleep *%d' | grep -v grep | awk '{print $2}'", ps, ssh, rport, lport, s, sleep);
		pipe = popen(cmd, "r");
		if (pipe) {
			while (fgets(line, 1024, pipe) != NULL) {
				int p = atoi(line);
				if (p > 0) {
					int score;
					if (p > mypid) 	{
						score = p - mypid;
					} else {
						score = p - mypid + 32768;
						if (score < 0) {
							score = 32768;
						}
					}
					if (best < 0 || score < best) {
						best = score;
						bestpid = p;
					}
				}
			}
			pclose(pipe);
		}

		if (bestpid != -1) {
			ssh_pid = (pid_t) bestpid;
			rfbLog("guessed ssh pid=%d, will terminate it on exit.\n", bestpid);
		}
	}

	free(cmd);
	free(s);
#endif
}

/* 
 * check blacklist for OSs with tight shm limits.
 */
static int limit_shm(void) {
	int limit = 0;

#ifndef WIN32
	if (UT.sysname == NULL) {
		return 0;
	}
	if (getenv("X11VNC_NO_LIMIT_SHM")) {
		return 0;
	}
	if (!strcmp(UT.sysname, "SunOS")) {
		char *r = UT.release;
		if (*r == '5' && *(r+1) == '.') {
			if (strchr("2345678", *(r+2)) != NULL) {
				limit = 1;
			}
		}
	} else if (!strcmp(UT.sysname, "Darwin")) {
		limit = 1;
	}
	if (limit && ! quiet) {
		fprintf(stderr, "reducing shm usage on %s %s (adding "
		    "-onetile)\n", UT.sysname, UT.release);
	}
#endif
	return limit;
}


/*
 * quick-n-dirty ~/.x11vncrc: each line (except # comments) is a cmdline option.
 */
static int argc2 = 0;
static char **argv2;

static void check_rcfile(int argc, char **argv) {
	int i, j, pwlast, enclast, norc = 0, argmax = 1024;
	char *infile = NULL;
	char rcfile[1024];
	FILE *rc = NULL; 

	for (i=1; i < argc; i++) {
		if (!strcmp(argv[i], "-printgui")) {
			fprintf(stdout, "%s", get_gui_code());
			fflush(stdout);
			exit(0);
		}
		if (!strcmp(argv[i], "-norc")) {
			norc = 1;
			got_norc = 1;
		}
		if (!strcmp(argv[i], "-QD")) {
			norc = 1;
		}
		if (!strcmp(argv[i], "-rc")) {
			if (i+1 >= argc) {
				fprintf(stderr, "-rc option requires a "
				    "filename\n");
				exit(1);
			} else {
				infile = argv[i+1];
			}
		}
	}
	rc_norc = norc;
	rc_rcfile = strdup("");
	if (norc) {
		;
	} else if (infile != NULL) {
		rc = fopen(infile, "r");
		rc_rcfile = strdup(infile);
		if (rc == NULL) {
			fprintf(stderr, "could not open rcfile: %s\n", infile);
			perror("fopen");
			exit(1);
		}
	} else {
		char *home = get_home_dir();
		if (! home) {
			norc = 1;
		} else {
			memset(rcfile, 0, sizeof(rcfile));
			strncpy(rcfile, home, 500);
			free(home);

			strcat(rcfile, "/.x11vncrc");
			infile = rcfile;
			rc = fopen(rcfile, "r");
			if (rc == NULL) {
				norc = 1;
			} else {
				rc_rcfile = strdup(rcfile);
				rc_rcfile_default = 1;
			}
		}
	}

	argv2 = (char **) malloc(argmax * sizeof(char *));
	argv2[argc2++] = strdup(argv[0]);

	if (! norc) {
		char line[4096], parm[400], tmp[401];
		char *buf, *tbuf;
		struct stat sbuf;
		int sz;

		if (fstat(fileno(rc), &sbuf) != 0) {
			fprintf(stderr, "problem with %s\n", infile);
			perror("fstat");
			exit(1);
		}
		sz = sbuf.st_size+1;	/* allocate whole file size */
		if (sz < 1024) {
			sz = 1024;
		}

		buf = (char *) malloc(sz);
		buf[0] = '\0';

		while (fgets(line, 4096, rc) != NULL) {
			char *q, *p = line;
			char c;
			int cont = 0;

			q = p;
			c = '\0';
			while (*q) {
				if (*q == '#') {
					if (c != '\\') {
						*q = '\0';
						break;
					}
				}
				c = *q;
				q++;
			}

			q = p;
			c = '\0';
			while (*q) {
				if (*q == '\n') {
					if (c == '\\') {
						cont = 1;
						*q = '\0';
						*(q-1) = ' ';
						break;
					}
					while (isspace((unsigned char) (*q))) {
						*q = '\0';
						if (q == p) {
							break;
						}
						q--;
					}
					break;
				}
				c = *q;
				q++;
			}
			if (q != p && !cont) {
				if (*q == '\0') {
					q--;
				}
				while (isspace((unsigned char) (*q))) {
					*q = '\0';
					if (q == p) {
						break;
					}
					q--;
				}
			}

			p = lblanks(p);

			strncat(buf, p, sz - strlen(buf) - 1);
			if (cont) {
				continue;
			}
			if (buf[0] == '\0') {
				continue;
			}

			i = 0;
			q = buf;
			while (*q) {
				i++;
				if (*q == '\n' || isspace((unsigned char) (*q))) {
					break;
				}
				q++;
			}

			if (i >= 400) {
				fprintf(stderr, "invalid rcfile line: %s/%s\n",
				    p, buf);
				exit(1);
			}

			if (sscanf(buf, "%s", parm) != 1) {
				fprintf(stderr, "invalid rcfile line: %s\n", p);
				exit(1);
			}
			if (parm[0] == '-') {
				strncpy(tmp, parm, 400); 
			} else {
				tmp[0] = '-';
				strncpy(tmp+1, parm, 400); 
			}

			if (strstr(tmp, "-loop") == tmp) {
				if (! getenv("X11VNC_LOOP_MODE")) {
					check_loop_mode(argc, argv, 1);
					exit(0);
				}
			}

			argv2[argc2++] = strdup(tmp);
			if (argc2 >= argmax) {
				fprintf(stderr, "too many rcfile options\n");
				exit(1);
			}
			
			p = buf;
			p += strlen(parm);
			p = lblanks(p);

			if (*p == '\0') {
				buf[0] = '\0';
				continue;
			}

			tbuf = (char *) calloc(strlen(p) + 1, 1);

			j = 0;
			while (*p) {
				if (*p == '\\' && *(p+1) == '#') {
					;
				} else {
					tbuf[j++] = *p;
				}
				p++;
			}

			argv2[argc2++] = strdup(tbuf);
			free(tbuf);
			if (argc2 >= argmax) {
				fprintf(stderr, "too many rcfile options\n");
				exit(1);
			}
			buf[0] = '\0';
		}
		fclose(rc);
		free(buf);
	}
	pwlast = 0;
	enclast = 0;
	for (i=1; i < argc; i++) {
		argv2[argc2++] = strdup(argv[i]);

		if (pwlast || !strcmp("-passwd", argv[i])
		    || !strcmp("-viewpasswd", argv[i])) {
			char *p = argv[i];		
			if (pwlast) {
				pwlast = 0;
			} else {
				pwlast = 1;
			}
			strzero(p);
		}
		if (enclast || !strcmp("-enc", argv[i])) {
			char *q, *p = argv[i];		
			if (enclast) {
				enclast = 0;
			} else {
				enclast = 1;
			}
			q = strstr(p, "pw=");
			if (q) {
				strzero(q);
			}
		}
		if (argc2 >= argmax) {
			fprintf(stderr, "too many rcfile options\n");
			exit(1);
		}
	}
}

static void immediate_switch_user(int argc, char* argv[]) {
	int i, bequiet = 0;
	for (i=1; i < argc; i++) {
		if (strcmp(argv[i], "-inetd")) {
			bequiet = 1;
		}
		if (strcmp(argv[i], "-quiet")) {
			bequiet = 1;
		}
		if (strcmp(argv[i], "-q")) {
			bequiet = 1;
		}
	}
	for (i=1; i < argc; i++) {
		char *u, *q;

		if (strcmp(argv[i], "-users")) {
			continue;
		}
		if (i == argc - 1) {
			fprintf(stderr, "not enough arguments for: -users\n");
			exit(1);
		}
		if (*(argv[i+1]) != '=') {
			break;
		}

		/* wants an immediate switch: =bob */
		u = strdup(argv[i+1]);
		*u = '+';
		q = strchr(u, '.');
		if (q) {
			user2group = (char **) malloc(2*sizeof(char *));
			user2group[0] = strdup(u+1);
			user2group[1] = NULL;
			*q = '\0';
		}
		if (strstr(u, "+guess") == u) {
			fprintf(stderr, "invalid user: %s\n", u+1);
			exit(1);
		}
		if (!switch_user(u, 0)) {
			fprintf(stderr, "Could not switch to user: %s\n", u+1);
			exit(1);
		} else {
			if (!bequiet) {
				fprintf(stderr, "Switched to user: %s\n", u+1);
			}
			started_as_root = 2;
		}
		free(u);
		break;
	}
}

static void quick_pw(char *str) {
	char *p, *q;
	char tmp[1024];
	int db = 0;

	if (db) fprintf(stderr, "quick_pw: %s\n", str);

	if (! str || str[0] == '\0') {
		exit(2);
	}
	if (str[0] != '%') {
		exit(2);
	}
	/*
	 * "%-" or "%stdin" means read one line from stdin.
	 *
	 * "%env" means it is in $UNIXPW env var.
	 *
	 * starting "%/" or "%." means read the first line from that file.
	 *
	 * "%%" or "%" means prompt user.
	 *
	 * otherwise: %user:pass
	 */
	if (!strcmp(str, "%-") || !strcmp(str, "%stdin")) {
		if(fgets(tmp, 1024, stdin) == NULL) {
			exit(2);
		}
		q = strdup(tmp);
	} else if (!strcmp(str, "%env")) {
		if (getenv("UNIXPW") == NULL) {
			exit(2);
		}
		q = strdup(getenv("UNIXPW"));
	} else if (!strcmp(str, "%%") || !strcmp(str, "%")) {
		char *t, inp[1024];
		fprintf(stdout, "username: ");
		if(fgets(tmp, 128, stdin) == NULL) {
			exit(2);
		}
		strcpy(inp, tmp);
		t = strchr(inp, '\n');
		if (t) {
			*t = ':'; 
		} else {
			strcat(inp, ":");
			
		}
		fprintf(stdout, "password: ");
		/* test mode: no_external_cmds does not apply */
		system("stty -echo");
		if(fgets(tmp, 128, stdin) == NULL) {
			fprintf(stdout, "\n");
			system("stty echo");
			exit(2);
		}
		system("stty echo");
		fprintf(stdout, "\n");
		strcat(inp, tmp);
		q = strdup(inp);
	} else if (str[1] == '/' || str[1] == '.') {
		FILE *in = fopen(str+1, "r");
		if (in == NULL) {
			exit(2);
		}
		if(fgets(tmp, 1024, in) == NULL) {
			exit(2);
		}
		fclose(in);
		q = strdup(tmp);
	} else {
		q = strdup(str+1);
	}
	p = (char *) malloc(strlen(q) + 10);
	strcpy(p, q);
	if (strchr(p, '\n') == NULL) {
		strcat(p, "\n");
	}

	if ((q = strchr(p, ':')) == NULL) {
		exit(2);
	}
	*q = '\0';
	if (db) fprintf(stderr, "'%s' '%s'\n", p, q+1);
	if (unixpw_cmd) {
		if (cmd_verify(p, q+1)) {
			fprintf(stdout, "Y %s\n", p);
			exit(0);
		} else {
			fprintf(stdout, "N %s\n", p);
			exit(1);
		}
	} else if (unixpw_nis) {
		if (crypt_verify(p, q+1)) {
			fprintf(stdout, "Y %s\n", p);
			exit(0);
		} else {
			fprintf(stdout, "N %s\n", p);
			exit(1);
		}
	} else {
		char *ucmd = getenv("UNIXPW_CMD");
		if (su_verify(p, q+1, ucmd, NULL, NULL, 1)) {
			fprintf(stdout, "Y %s\n", p);
			exit(0);
		} else {
			fprintf(stdout, "N %s\n", p);
			exit(1);
		}
	}
	/* NOTREACHED */
	exit(2);
}

static void print_settings(int try_http, int bg, char *gui_str) {

	fprintf(stderr, "\n");
	fprintf(stderr, "Settings:\n");
	fprintf(stderr, " display:    %s\n", use_dpy ? use_dpy
	    : "null");
#if SMALL_FOOTPRINT < 2
	fprintf(stderr, " authfile:   %s\n", auth_file ? auth_file
	    : "null");
	fprintf(stderr, " subwin:     0x%lx\n", subwin);
	fprintf(stderr, " -sid mode:  %d\n", rootshift);
	fprintf(stderr, " clip:       %s\n", clip_str ? clip_str
	    : "null");
	fprintf(stderr, " flashcmap:  %d\n", flash_cmap);
	fprintf(stderr, " shiftcmap:  %d\n", shift_cmap);
	fprintf(stderr, " force_idx:  %d\n", force_indexed_color);
	fprintf(stderr, " cmap8to24:  %d\n", cmap8to24);
	fprintf(stderr, " 8to24_opts: %s\n", cmap8to24_str ? cmap8to24_str
	    : "null");
	fprintf(stderr, " 24to32:     %d\n", xform24to32);
	fprintf(stderr, " visual:     %s\n", visual_str ? visual_str
	    : "null");
	fprintf(stderr, " overlay:    %d\n", overlay);
	fprintf(stderr, " ovl_cursor: %d\n", overlay_cursor);
	fprintf(stderr, " scaling:    %d %.4f %.4f\n", scaling, scale_fac_x, scale_fac_y);
	fprintf(stderr, " viewonly:   %d\n", view_only);
	fprintf(stderr, " shared:     %d\n", shared);
	fprintf(stderr, " conn_once:  %d\n", connect_once);
	fprintf(stderr, " timeout:    %d\n", first_conn_timeout);
	fprintf(stderr, " ping:       %d\n", ping_interval);
	fprintf(stderr, " inetd:      %d\n", inetd);
	fprintf(stderr, " tightfilexfer:   %d\n", tightfilexfer);
	fprintf(stderr, " http:       %d\n", try_http);
	fprintf(stderr, " connect:    %s\n", client_connect
	    ? client_connect : "null");
	fprintf(stderr, " connectfile %s\n", client_connect_file
	    ? client_connect_file : "null");
	fprintf(stderr, " vnc_conn:   %d\n", vnc_connect);
	fprintf(stderr, " allow:      %s\n", allow_list ? allow_list
	    : "null");
	fprintf(stderr, " input:      %s\n", allowed_input_str
	    ? allowed_input_str : "null");
	fprintf(stderr, " passfile:   %s\n", passwdfile ? passwdfile
	    : "null");
	fprintf(stderr, " unixpw:     %d\n", unixpw);
	fprintf(stderr, " unixpw_lst: %s\n", unixpw_list ? unixpw_list:"null");
	fprintf(stderr, " ssl:        %s\n", openssl_pem ? openssl_pem:"null");
	fprintf(stderr, " ssldir:     %s\n", ssl_certs_dir ? ssl_certs_dir:"null");
	fprintf(stderr, " ssltimeout  %d\n", ssl_timeout_secs);
	fprintf(stderr, " sslverify:  %s\n", ssl_verify ? ssl_verify:"null");
	fprintf(stderr, " stunnel:    %d\n", use_stunnel);
	fprintf(stderr, " accept:     %s\n", accept_cmd ? accept_cmd
	    : "null");
	fprintf(stderr, " accept:     %s\n", afteraccept_cmd ? afteraccept_cmd
	    : "null");
	fprintf(stderr, " gone:       %s\n", gone_cmd ? gone_cmd
	    : "null");
	fprintf(stderr, " users:      %s\n", users_list ? users_list
	    : "null");
	fprintf(stderr, " using_shm:  %d\n", using_shm);
	fprintf(stderr, " flipbytes:  %d\n", flip_byte_order);
	fprintf(stderr, " onetile:    %d\n", single_copytile);
	fprintf(stderr, " solid:      %s\n", solid_str
	    ? solid_str : "null");
	fprintf(stderr, " blackout:   %s\n", blackout_str
	    ? blackout_str : "null");
	fprintf(stderr, " xinerama:   %d\n", xinerama);
	fprintf(stderr, " xtrap:      %d\n", xtrap_input);
	fprintf(stderr, " xrandr:     %d\n", xrandr);
	fprintf(stderr, " xrandrmode: %s\n", xrandr_mode ? xrandr_mode
	    : "null");
	fprintf(stderr, " padgeom:    %s\n", pad_geometry
	    ? pad_geometry : "null");
	fprintf(stderr, " logfile:    %s\n", logfile ? logfile
	    : "null");
	fprintf(stderr, " logappend:  %d\n", logfile_append);
	fprintf(stderr, " flag:       %s\n", flagfile ? flagfile
	    : "null");
	fprintf(stderr, " rm_flag:    %s\n", rm_flagfile ? rm_flagfile
	    : "null");
	fprintf(stderr, " rc_file:    \"%s\"\n", rc_rcfile ? rc_rcfile
	    : "null");
	fprintf(stderr, " norc:       %d\n", rc_norc);
	fprintf(stderr, " dbg:        %d\n", crash_debug);
	fprintf(stderr, " bg:         %d\n", bg);
	fprintf(stderr, " mod_tweak:  %d\n", use_modifier_tweak);
	fprintf(stderr, " isolevel3:  %d\n", use_iso_level3);
	fprintf(stderr, " xkb:        %d\n", use_xkb_modtweak);
	fprintf(stderr, " skipkeys:   %s\n",
	    skip_keycodes ? skip_keycodes : "null");
	fprintf(stderr, " sloppykeys: %d\n", sloppy_keys);
	fprintf(stderr, " skip_dups:  %d\n", skip_duplicate_key_events);
	fprintf(stderr, " addkeysyms: %d\n", add_keysyms);
	fprintf(stderr, " xkbcompat:  %d\n", xkbcompat);
	fprintf(stderr, " clearmods:  %d\n", clear_mods);
	fprintf(stderr, " remap:      %s\n", remap_file ? remap_file
	    : "null");
	fprintf(stderr, " norepeat:   %d\n", no_autorepeat);
	fprintf(stderr, " norepeatcnt:%d\n", no_repeat_countdown);
	fprintf(stderr, " nofb:       %d\n", nofb);
	fprintf(stderr, " watchbell:  %d\n", watch_bell);
	fprintf(stderr, " watchsel:   %d\n", watch_selection);
	fprintf(stderr, " watchprim:  %d\n", watch_primary);
	fprintf(stderr, " seldir:     %s\n", sel_direction ?
	    sel_direction : "null");
	fprintf(stderr, " cursor:     %d\n", show_cursor);
	fprintf(stderr, " multicurs:  %d\n", show_multiple_cursors);
	fprintf(stderr, " curs_mode:  %s\n", multiple_cursors_mode
	    ? multiple_cursors_mode : "null");
	fprintf(stderr, " arrow:      %d\n", alt_arrow);
	fprintf(stderr, " xfixes:     %d\n", use_xfixes);
	fprintf(stderr, " alphacut:   %d\n", alpha_threshold);
	fprintf(stderr, " alphafrac:  %.2f\n", alpha_frac);
	fprintf(stderr, " alpharemove:%d\n", alpha_remove);
	fprintf(stderr, " alphablend: %d\n", alpha_blend);
	fprintf(stderr, " cursorshape:%d\n", cursor_shape_updates);
	fprintf(stderr, " cursorpos:  %d\n", cursor_pos_updates);
	fprintf(stderr, " xwarpptr:   %d\n", use_xwarppointer);
	fprintf(stderr, " alwaysinj:  %d\n", always_inject);
	fprintf(stderr, " buttonmap:  %s\n", pointer_remap
	    ? pointer_remap : "null");
	fprintf(stderr, " dragging:   %d\n", show_dragging);
	fprintf(stderr, " ncache:     %d\n", ncache);
	fprintf(stderr, " wireframe:  %s\n", wireframe_str ?
	    wireframe_str : WIREFRAME_PARMS);
	fprintf(stderr, " wirecopy:   %s\n", wireframe_copyrect ?
	    wireframe_copyrect : wireframe_copyrect_default);
	fprintf(stderr, " scrollcopy: %s\n", scroll_copyrect ?
	    scroll_copyrect : scroll_copyrect_default);
	fprintf(stderr, "  scr_area:  %d\n", scrollcopyrect_min_area);
	fprintf(stderr, "  scr_skip:  %s\n", scroll_skip_str ?
	    scroll_skip_str : scroll_skip_str0);
	fprintf(stderr, "  scr_inc:   %s\n", scroll_good_str ?
	    scroll_good_str : scroll_good_str0);
	fprintf(stderr, "  scr_keys:  %s\n", scroll_key_list_str ?
	    scroll_key_list_str : "null");
	fprintf(stderr, "  scr_term:  %s\n", scroll_term_str ?
	    scroll_term_str : "null");
	fprintf(stderr, "  scr_keyrep: %s\n", max_keyrepeat_str ?
	    max_keyrepeat_str : "null");
	fprintf(stderr, "  scr_parms: %s\n", scroll_copyrect_str ?
	    scroll_copyrect_str : SCROLL_COPYRECT_PARMS);
	fprintf(stderr, " fixscreen:  %s\n", screen_fixup_str ?
	    screen_fixup_str : "null");
	fprintf(stderr, " noxrecord:  %d\n", noxrecord);
	fprintf(stderr, " grabbuster: %d\n", grab_buster);
	fprintf(stderr, " ptr_mode:   %d\n", pointer_mode);
	fprintf(stderr, " inputskip:  %d\n", ui_skip);
	fprintf(stderr, " speeds:     %s\n", speeds_str
	    ? speeds_str : "null");
	fprintf(stderr, " wmdt:       %s\n", wmdt_str
	    ? wmdt_str : "null");
	fprintf(stderr, " debug_ptr:  %d\n", debug_pointer);
	fprintf(stderr, " debug_key:  %d\n", debug_keyboard);
	fprintf(stderr, " defer:      %d\n", defer_update);
	fprintf(stderr, " waitms:     %d\n", waitms);
	fprintf(stderr, " wait_ui:    %.2f\n", wait_ui);
	fprintf(stderr, " nowait_bog: %d\n", !wait_bog);
	fprintf(stderr, " slow_fb:    %.2f\n", slow_fb);
	fprintf(stderr, " xrefresh:   %.2f\n", xrefresh);
	fprintf(stderr, " readtimeout: %d\n", rfbMaxClientWait/1000);
	fprintf(stderr, " take_naps:  %d\n", take_naps);
	fprintf(stderr, " sb:         %d\n", screen_blank);
	fprintf(stderr, " fbpm:       %d\n", !watch_fbpm);
	fprintf(stderr, " dpms:       %d\n", !watch_dpms);
	fprintf(stderr, " xdamage:    %d\n", use_xdamage);
	fprintf(stderr, "  xd_area:   %d\n", xdamage_max_area);
	fprintf(stderr, "  xd_mem:    %.3f\n", xdamage_memory);
	fprintf(stderr, " sigpipe:    %s\n", sigpipe
	    ? sigpipe : "null");
	fprintf(stderr, " threads:    %d\n", use_threads);
	fprintf(stderr, " fs_frac:    %.2f\n", fs_frac);
	fprintf(stderr, " gaps_fill:  %d\n", gaps_fill);
	fprintf(stderr, " grow_fill:  %d\n", grow_fill);
	fprintf(stderr, " tile_fuzz:  %d\n", tile_fuzz);
	fprintf(stderr, " snapfb:     %d\n", use_snapfb);
	fprintf(stderr, " rawfb:      %s\n", raw_fb_str
	    ? raw_fb_str : "null");
	fprintf(stderr, " pipeinput:  %s\n", pipeinput_str
	    ? pipeinput_str : "null");
	fprintf(stderr, " gui:        %d\n", launch_gui);
	fprintf(stderr, " gui_mode:   %s\n", gui_str
	    ? gui_str : "null");
	fprintf(stderr, " noremote:   %d\n", !accept_remote_cmds);
	fprintf(stderr, " unsafe:     %d\n", !safe_remote_only);
	fprintf(stderr, " privremote: %d\n", priv_remote);
	fprintf(stderr, " safer:      %d\n", more_safe);
	fprintf(stderr, " nocmds:     %d\n", no_external_cmds);
	fprintf(stderr, " deny_all:   %d\n", deny_all);
	fprintf(stderr, " pid:        %d\n", getpid());
	fprintf(stderr, "\n");
#endif
}


static void check_loop_mode(int argc, char* argv[], int force) {
	int i;
	int loop_mode = 0, loop_sleep = 2000, loop_max = 0;

	if (force) {
		loop_mode = 1;
	}
	for (i=1; i < argc; i++) {
		char *p = argv[i];
		if (strstr(p, "--") == p) {
			p++;
		}
		if (strstr(p, "-loop") == p) {
			char *q;
			loop_mode = 1;
			if ((q = strchr(p, ',')) != NULL) {
				loop_max = atoi(q+1);
				*q = '\0';
			}
			if (strstr(p, "-loopbg") == p) {
				set_env("X11VNC_LOOP_MODE_BG", "1");
				loop_sleep = 500;
			}
			
			q = strpbrk(p, "0123456789");
			if (q) {
				loop_sleep = atoi(q);
				if (loop_sleep <= 0) {
					loop_sleep = 20;
				}
			}
		}
	}
	if (loop_mode && getenv("X11VNC_LOOP_MODE") == NULL) {
#if LIBVNCSERVER_HAVE_FORK
		char **argv2;
		int k, i = 1;

		set_env("X11VNC_LOOP_MODE", "1");
		argv2 = (char **) malloc((argc+1)*sizeof(char *));

		for (k=0; k < argc+1; k++) {
			argv2[k] = NULL;
			if (k < argc) {
				argv2[k] = argv[k];
			}
		}
		while (1) {
			int status;
			pid_t p;
			fprintf(stderr, "\n --- x11vnc loop: %d ---\n\n", i++);
			fflush(stderr);
			usleep(500 * 1000);
			if ((p = fork()) > 0)  {
				fprintf(stderr, " --- x11vnc loop: waiting "
				    "for: %d\n\n", p);
				wait(&status);
			} else if (p == -1) {
				fprintf(stderr, "could not fork\n");
				perror("fork");
				exit(1);
			} else {
				/* loop mode: no_external_cmds does not apply */
				execvp(argv[0], argv2); 
				exit(1);
			}

			if (loop_max > 0 && i > loop_max) {
				fprintf(stderr, "\n --- x11vnc loop: did %d"
				    " done. ---\n\n", loop_max);
				break;
			}
			
			fprintf(stderr, "\n --- x11vnc loop: sleeping %d ms "
			    "---\n\n", loop_sleep);
			usleep(loop_sleep * 1000);
		}
		exit(0);
#else
		fprintf(stderr, "fork unavailable, cannot do -loop mode\n");
		exit(1);
#endif
	}
}

extern int appshare_main(int argc, char* argv[]);

static void check_appshare_mode(int argc, char* argv[]) {
#ifndef WIN32
	int i;

	for (i=1; i < argc; i++) {
		char *p = argv[i];
		if (strstr(p, "--") == p) {
			p++;
		}
		if (strstr(p, "-appshare") == p) {
			appshare_main(argc, argv);
			exit(0);
		}
	}
#endif
}

static void store_homedir_passwd(char *file) {
#ifndef WIN32
	char str1[32], str2[32], *p, *h, *f;
	struct stat sbuf;

	str1[0] = '\0';
	str2[0] = '\0';

	/* storepasswd */
	if (no_external_cmds || !cmd_ok("storepasswd")) {
		fprintf(stderr, "-nocmds cannot be used with -storepasswd\n");
		exit(1);
	}

	fprintf(stderr, "Enter VNC password: ");
	system("stty -echo");
	if (fgets(str1, 32, stdin) == NULL) {
		perror("fgets");
		system("stty echo");
		exit(1);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Verify password:    ");
	if (fgets(str2, 32, stdin) == NULL) {
		perror("fgets");
		system("stty echo");
		exit(1);
	}
	fprintf(stderr, "\n");
	system("stty echo");

	if ((p = strchr(str1, '\n')) != NULL) {
		*p = '\0';
	}
	if ((p = strchr(str2, '\n')) != NULL) {
		*p = '\0';
	}
	if (strcmp(str1, str2)) {
		fprintf(stderr, "** passwords differ.\n");
		exit(1);
	}
	if (str1[0] == '\0') {
		fprintf(stderr, "** no password supplied.\n");
		exit(1);
	}

	if (file != NULL) {
		f = file;
	} else {
		
		h = getenv("HOME");
		if (! h) {
			fprintf(stderr, "** $HOME not set.\n");
			exit(1);
		}

		f = (char *) malloc(strlen(h) + strlen("/.vnc/passwd") + 1);
		sprintf(f, "%s/.vnc", h);

		if (stat(f, &sbuf) != 0) {
			if (mkdir(f, 0755) != 0) {
				fprintf(stderr, "** could not create directory %s\n", f);
				perror("mkdir");
				exit(1);
			}
		} else if (! S_ISDIR(sbuf.st_mode)) {
			fprintf(stderr, "** not a directory %s\n", f);
			exit(1);
		}

		sprintf(f, "%s/.vnc/passwd", h);
	}
	fprintf(stderr, "Write password to %s?  [y]/n ", f);

	if (fgets(str2, 32, stdin) == NULL) {
		perror("fgets");
		exit(1);
	}
	if (str2[0] == 'n' || str2[0] == 'N') {
		fprintf(stderr, "not creating password.\n");
		exit(1);
	}

	if (rfbEncryptAndStorePasswd(str1, f) != 0) {
		fprintf(stderr, "** error creating password: %s\n", f);
		perror("storepasswd");
		exit(1);
	}
	if (stat(f, &sbuf) != 0) {
		fprintf(stderr, "** error creating password: %s\n", f);
		perror("stat");
		exit(1);
	}
	fprintf(stdout, "Password written to: %s\n", f);
	exit(0);
#endif
}

void ncache_beta_tester_message(void) {

char msg[] = 
"\n"
"******************************************************************************\n"
"\n"
"Hello!  Exciting News!!\n"
"\n"
"You have been selected at random to beta-test the x11vnc '-ncache' VNC\n"
"client-side pixel caching feature!\n"
"\n"
"This scheme stores pixel data offscreen on the VNC viewer side for faster\n"
"retrieval.  It should work with any VNC viewer.\n"
"\n"
"This method requires much testing and so we hope you will try it out and\n"
"perhaps even report back your observations.  However, if you do not want\n"
"to test or use the feature, run x11vnc like this:\n"
"\n"
"    x11vnc -noncache ...\n"
"\n"
"Your current setting is: -ncache %d\n"
"\n"
"The feature needs additional testing because we want to have x11vnc\n"
"performance enhancements on by default.  Otherwise, only a relative few\n"
"would notice and use the -ncache option (e.g. the wireframe and scroll\n"
"detection features are on by default).  A couple things to note:\n"
"\n"
"    1) It uses a large amount of RAM (on both viewer and server sides)\n"
"\n"
"    2) You can actually see the cached pixel data if you scroll down\n"
"       to it in your viewer; adjust your viewer's size to hide it.\n"
"\n"
"More info: http://www.karlrunge.com/x11vnc/faq.html#faq-client-caching\n"
"\n"
"waiting for connections:\n"
;

char msg2[] = 
"\n"
"******************************************************************************\n"
"Have you tried the x11vnc '-ncache' VNC client-side pixel caching feature yet?\n"
"\n"
"The scheme stores pixel data offscreen on the VNC viewer side for faster\n"
"retrieval.  It should work with any VNC viewer.  Try it by running:\n"
"\n"
"    x11vnc -ncache 10 ...\n"
"\n"
"One can also add -ncache_cr for smooth 'copyrect' window motion.\n"
"More info: http://www.karlrunge.com/x11vnc/faq.html#faq-client-caching\n"
"\n"
;

	if (raw_fb_str && !macosx_console) {
		return;
	}
	if (quiet) {
		return;
	}
	if (remote_direct) {
		return;
	}
	if (nofb) {
		return;
	}
#ifdef NO_NCACHE
	return;
#endif

	if (ncache == 0) {
		fprintf(stderr, "%s", msg2);
		ncache0 = ncache = 0;
	} else {
		fprintf(stderr, msg, ncache);
	}
}

#define	SHOW_NO_PASSWORD_WARNING \
	(!got_passwd && !got_rfbauth && (!got_passwdfile || !passwd_list) \
	    && !query_cmd && !remote_cmd && !unixpw && !got_gui_pw \
	    && ! ssl_verify && !inetd && !terminal_services_daemon)

static void do_sleepin(char *sleep) {
	int n1, n2, nt;
	double f1, f2, ft;

	if (strchr(sleep, '-')) {
		double s = atof(strchr(sleep, '-')+1); 
		if (sscanf(sleep, "%d-%d", &n1, &n2) == 2) {
			if (n1 > n2) {
				nt = n1;
				n1 = n2;
				n2 = nt;
			}
			s = n1 + rfac() * (n2 - n1);
		} else if (sscanf(sleep, "%lf-%lf", &f1, &f2) == 2) {
			if (f1 > f2) {
				ft = f1;
				f1 = f2;
				f2 = ft;
			}
			s = f1 + rfac() * (f2 - f1);
		}
		if (getenv("DEBUG_SLEEPIN")) fprintf(stderr, "sleepin: %f secs\n", s);
		usleep( (int) (1000*1000*s) );
	} else {
		n1 = atoi(sleep);
		if (getenv("DEBUG_SLEEPIN")) fprintf(stderr, "sleepin: %d secs\n", n1);
		if (n1 > 0) {
			usleep(1000*1000*n1);
		}
	}
}

static void check_guess_auth_file(void)  {
	if (!strcasecmp(auth_file, "guess")) {
		char line[4096], *cmd, *q, *disp = use_dpy ? use_dpy: "";
		FILE *p;
		int n;
		if (!program_name) {
			rfbLog("-auth guess: no program_name found.\n");
			clean_up_exit(1);
		}
		if (strpbrk(program_name, " \t\r\n")) {
			rfbLog("-auth guess: whitespace in program_name '%s'\n", program_name);
			clean_up_exit(1);
		}
		if (no_external_cmds || !cmd_ok("findauth")) {
			rfbLog("-auth guess: cannot run external commands in -nocmds mode:\n");
			clean_up_exit(1);
		}

		cmd = (char *)malloc(100 + strlen(program_name) + strlen(disp));
		sprintf(cmd, "%s -findauth %s -env _D_XDM=1", program_name, disp);
		p = popen(cmd, "r");
		if (!p) {
			rfbLog("-auth guess: could not run cmd '%s'\n", cmd);
			clean_up_exit(1);
		}
		memset(line, 0, sizeof(line));
		n = fread(line, 1, sizeof(line), p);
		pclose(p);
		q = strrchr(line, '\n');
		if (q) *q = '\0';
		if (!strcmp(disp, "")) {
			disp = getenv("DISPLAY");
			if (!disp) {
				disp = "unset";
			}
		}
		if (strstr(line, "XAUTHORITY=") != line && !getenv("FD_XDM")) {
			if (use_dpy == NULL || strstr(use_dpy, "cmd=FIND") == NULL) {
				if (getuid() == 0 || geteuid() == 0) {
					char *q = strstr(cmd, "_D_XDM=1");
					if (q) {
						*q = 'F';
						rfbLog("-auth guess: failed for display='%s'\n", disp);
						rfbLog("-auth guess: since we are root, retrying with FD_XDM=1\n");
						p = popen(cmd, "r");
						if (!p) {
							rfbLog("-auth guess: could not run cmd '%s'\n", cmd);
							clean_up_exit(1);
						}
						memset(line, 0, sizeof(line));
						n = fread(line, 1, sizeof(line), p);
						pclose(p);
						q = strrchr(line, '\n');
						if (q) *q = '\0';
					}
				}
			}
		}
		if (!strcmp(line, "")) {
			rfbLog("-auth guess: failed for display='%s'\n", disp);
			clean_up_exit(1);
		} else if (strstr(line, "XAUTHORITY=") != line) {
			rfbLog("-auth guess: failed. '%s' for display='%s'\n", line, disp);
			clean_up_exit(1);
		} else if (!strcmp(line, "XAUTHORITY=")) {
			rfbLog("-auth guess: using default XAUTHORITY for display='%s'\n", disp);
			q = getenv("XAUTHORITY");
			if (q) {
				*(q-2) = '_';	/* yow */
			}
			auth_file = NULL;
		} else {
			rfbLog("-auth guess: using '%s' for disp='%s'\n", line, disp);
			auth_file = strdup(line + strlen("XAUTHORITY="));
		}
	}
}

extern int is_decimal(char *);

int main(int argc, char* argv[]) {

	int i, len, tmpi;
	int ev, er, maj, min;
	char *arg;
	int remote_sync = 0;
	char *remote_cmd = NULL;
	char *query_cmd  = NULL;
	int query_retries = 0;
	double query_delay = 0.5;
	char *query_match  = NULL;
	char *gui_str = NULL;
	int got_gui_pw = 0;
	int pw_loc = -1, got_passwd = 0, got_rfbauth = 0, nopw = NOPW;
	int got_viewpasswd = 0, got_localhost = 0, got_passwdfile = 0;
	int vpw_loc = -1;
	int dt = 0, bg = 0;
	int got_rfbwait = 0;
	int got_httpdir = 0, try_http = 0;
	int orig_use_xdamage = use_xdamage;
	int http_oneport_msg = 0;
	XImage *fb0 = NULL;
	int ncache_msg = 0;
	char *got_rfbport_str = NULL;
	int got_rfbport_pos = -1;
	int got_tls = 0;
	int got_inetd = 0;
	int got_noxrandr = 0;

	/* used to pass args we do not know about to rfbGetScreen(): */
	int argc_vnc_max = 1024;
	int argc_vnc = 1; char *argv_vnc[2048];

	/* check for -loop mode: */
	check_loop_mode(argc, argv, 0);

	/* check for -appshare mode: */
	check_appshare_mode(argc, argv);

	dtime0(&x11vnc_start);

	for (i=1; i < argc; i++) {
		if (!strcmp(argv[i], "-inetd")) {
			got_inetd = 1;
		}
	}

	if (!getuid() || !geteuid()) {
		started_as_root = 1;
		if (0 && !got_inetd) {
			rfbLog("getuid: %d  geteuid: %d\n", getuid(), geteuid());
		}

		/* check for '-users =bob' */
		immediate_switch_user(argc, argv);
	}

	for (i=0; i < 2048; i++) {
		argv_vnc[i] = NULL;
	}

	argv_vnc[0] = strdup(argv[0]);
	program_name = strdup(argv[0]);
	program_pid = (int) getpid();

	solid_default = strdup(solid_default);	/* for freeing with -R */

	len = 0;
	for (i=1; i < argc; i++) {
		len += strlen(argv[i]) + 4 + 1;
	}
	program_cmdline = (char *) malloc(len+1);
	program_cmdline[0] = '\0';
	for (i=1; i < argc; i++) {
		char *s = argv[i];
		if (program_cmdline[0]) {
			strcat(program_cmdline, " ");
		}
		if (*s == '-') {
			strcat(program_cmdline, s);
		} else {
			strcat(program_cmdline, "{{");
			strcat(program_cmdline, s);
			strcat(program_cmdline, "}}");
		}
	}

	for (i=0; i<ICON_MODE_SOCKS; i++) {
		icon_mode_socks[i] = -1;
	}

	check_rcfile(argc, argv);
	/* kludge for the new array argv2 set in check_rcfile() */
#	define argc argc2
#	define argv argv2

#	define CHECK_ARGC if (i >= argc-1) { \
		fprintf(stderr, "not enough arguments for: %s\n", arg); \
		exit(1); \
	}

	/*
	 * do a quick check for parameters that apply to "utility"
	 * commands, i.e. ones that do not run the server.
	 */
	for (i=1; i < argc; i++) {
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}
		if (!strcmp(arg, "-ssldir")) {
			CHECK_ARGC
			ssl_certs_dir = strdup(argv[++i]);
		}
	}

	/*
	 * do a quick check for -env parameters
	 */
	for (i=1; i < argc; i++) {
		char *p, *q;
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}
		if (!strcmp(arg, "-env")) {
			CHECK_ARGC
			p = strdup(argv[++i]);
			q = strchr(p, '=');
			if (! q) {
				fprintf(stderr, "no -env '=' found: %s\n", p);
				exit(1);
			} else {
				*q = '\0';
			}
			set_env(p, q+1);
			free(p);
		}
	}

	for (i=1; i < argc; i++) {
		/* quick-n-dirty --option handling. */
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}

		if (!strcmp(arg, "-display")) {
			CHECK_ARGC
			use_dpy = strdup(argv[++i]);
			if (strstr(use_dpy, "WAIT")) {
				extern char find_display[];
				extern char create_display[];
				if (strstr(use_dpy, "cmd=FINDDISPLAY-print")) {
					fprintf(stdout, "%s", find_display);
					exit(0);
				}
				if (strstr(use_dpy, "cmd=FINDCREATEDISPLAY-print")) {
					fprintf(stdout, "%s", create_display);
					exit(0);
				}
			}
			continue;
		}
		if (!strcmp(arg, "-reopen")) {
			char *str = getenv("X11VNC_REOPEN_DISPLAY");
			if (str) {
				int rmax = atoi(str);
				if (rmax > 0) {
					set_env("X11VNC_REOPEN_DISPLAY", str);
				}
			} else {
				set_env("X11VNC_REOPEN_DISPLAY", "1");
			}
			continue;
		}
		if (!strcmp(arg, "-find")) {
			use_dpy = strdup("WAIT:cmd=FINDDISPLAY");
			continue;
		}
		if (!strcmp(arg, "-finddpy") || strstr(arg, "-listdpy") == arg) {
			int ic = 0;
			use_dpy = strdup("WAIT:cmd=FINDDISPLAY-run");
			if (argc > i+1) {
				set_env("X11VNC_USER", argv[i+1]);
				fprintf(stdout, "X11VNC_USER=%s\n", getenv("X11VNC_USER"));
			}
			if (strstr(arg, "-listdpy") == arg) {
				set_env("FIND_DISPLAY_ALL", "1");
			}
			wait_for_client(&ic, NULL, 0);
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-findauth")) {
			got_findauth = 1;
			if (argc > i+1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					set_env("FINDAUTH_DISPLAY", argv[i+1]);
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-create")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvfb");
			continue;
		}
		if (!strcmp(arg, "-create_xsrv")) {
			CHECK_ARGC
			use_dpy = (char *) malloc(strlen(argv[i+1])+100); 
			sprintf(use_dpy, "WAIT:cmd=FINDCREATEDISPLAY-%s", argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-xdummy")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xdummy");
			continue;
		}
		if (!strcmp(arg, "-xdummy_xvfb")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xdummy,Xvfb");
			continue;
		}
		if (!strcmp(arg, "-xvnc")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvnc");
			continue;
		}
		if (!strcmp(arg, "-xvnc_redirect")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvnc.redirect");
			continue;
		}
		if (!strcmp(arg, "-redirect")) {
			char *q, *t, *t0 = "WAIT:cmd=FINDDISPLAY-vnc_redirect";
			CHECK_ARGC
			t = (char *) malloc(strlen(t0) + strlen(argv[++i]) + 2);
			q = strrchr(argv[i], ':');
			if (q) *q = ' ';
			sprintf(t, "%s=%s", t0, argv[i]);
			use_dpy = t;
			continue;
		}
		if (!strcmp(arg, "-auth") || !strcmp(arg, "-xauth")) {
			CHECK_ARGC
			auth_file = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-N")) {
			display_N = 1;
			continue;
		}
		if (!strcmp(arg, "-autoport")) {
			CHECK_ARGC
			auto_port = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-reflect")) {
			CHECK_ARGC
			raw_fb_str = (char *) malloc(4 + strlen(argv[i+1]) + 1);
			sprintf(raw_fb_str, "vnc:%s", argv[++i]);
			shared = 1;
			continue;
		}
		if (!strcmp(arg, "-tsd")) {
			CHECK_ARGC
			terminal_services_daemon = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-id") || !strcmp(arg, "-sid")) {
			CHECK_ARGC
			if (!strcmp(arg, "-sid")) {
				rootshift = 1;
			} else {
				rootshift = 0;
			}
			i++;
			if (!strcmp("pick", argv[i])) {
				if (started_as_root) {
					fprintf(stderr, "unsafe: %s pick\n",
					    arg);
					exit(1);
				} else if (! pick_windowid(&subwin)) {
					fprintf(stderr, "invalid %s pick\n",
					    arg);
					exit(1);
				}
			} else if (! scan_hexdec(argv[i], &subwin)) {
				fprintf(stderr, "invalid %s arg: %s\n", arg,
				    argv[i]);
				exit(1);
			}
			continue;
		}
		if (!strcmp(arg, "-waitmapped")) {
			subwin_wait_mapped = 1;
			continue;
		}
		if (!strcmp(arg, "-clip")) {
			CHECK_ARGC
			clip_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-flashcmap")) {
			flash_cmap = 1;
			continue;
		}
		if (!strcmp(arg, "-shiftcmap")) {
			CHECK_ARGC
			shift_cmap = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-notruecolor")) {
			force_indexed_color = 1;
			continue;
		}
		if (!strcmp(arg, "-advertise_truecolor")) {
			advertise_truecolor = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					if (strstr(s, "reset")) {
						advertise_truecolor_reset = 1;
					}
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-overlay")) {
			overlay = 1;
			continue;
		}
		if (!strcmp(arg, "-overlay_nocursor")) {
			overlay = 1;
			overlay_cursor = 0;
			continue;
		}
		if (!strcmp(arg, "-overlay_yescursor")) {
			overlay = 1;
			overlay_cursor = 2;
			continue;
		}
		if (!strcmp(arg, "-8to24")) {
#if !SKIP_8TO24
			cmap8to24 = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					cmap8to24_str = strdup(s);
					i++;
				}
			}
#endif
			continue;
		}
		if (!strcmp(arg, "-24to32")) {
			xform24to32 = 1;
			continue;
		}
		if (!strcmp(arg, "-visual")) {
			CHECK_ARGC
			visual_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scale")) {
			CHECK_ARGC
			scale_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-geometry")) {
			CHECK_ARGC
			scale_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scale_cursor")) {
			CHECK_ARGC
			scale_cursor_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-viewonly")) {
			view_only = 1;
			continue;
		}
		if (!strcmp(arg, "-noviewonly")) {
			view_only = 0;
			got_noviewonly = 1;
			continue;
		}
		if (!strcmp(arg, "-shared")) {
			shared = 1;
			continue;
		}
		if (!strcmp(arg, "-noshared")) {
			shared = 0;
			continue;
		}
		if (!strcmp(arg, "-once")) {
			connect_once = 1;
			got_connect_once = 1;
			continue;
		}
		if (!strcmp(arg, "-many") || !strcmp(arg, "-forever")) {
			connect_once = 0;
			continue;
		}
		if (strstr(arg, "-loop") == arg) {
			;	/* handled above */
			continue;
		}
		if (strstr(arg, "-appshare") == arg) {
			;	/* handled above */
			continue;
		}
		if (strstr(arg, "-freeze_when_obscured") == arg) {
			freeze_when_obscured = 1;
			continue;
		}
		if (!strcmp(arg, "-timeout")) {
			CHECK_ARGC
			first_conn_timeout = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-sleepin")) {
			CHECK_ARGC
			do_sleepin(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-users")) {
			CHECK_ARGC
			users_list = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-inetd")) {
			inetd = 1;
			continue;
		}
		if (!strcmp(arg, "-notightfilexfer")) {
			tightfilexfer = 0;
			continue;
		}
		if (!strcmp(arg, "-tightfilexfer")) {
			tightfilexfer = 1;
			continue;
		}
		if (!strcmp(arg, "-http")) {
			try_http = 1;
			continue;
		}
		if (!strcmp(arg, "-http_ssl")) {
			try_http = 1;
			http_ssl = 1;
			got_tls++;
			continue;
		}
		if (!strcmp(arg, "-avahi") || !strcmp(arg, "-mdns") || !strcmp(arg, "-zeroconf")) {
			avahi = 1;
			continue;
		}
		if (!strcmp(arg, "-connect") ||
		    !strcmp(arg, "-connect_or_exit") ||
		    !strcmp(arg, "-coe")) {
			CHECK_ARGC
			if (!strcmp(arg, "-connect_or_exit")) {
				connect_or_exit = 1;
			} else if (!strcmp(arg, "-coe")) {
				connect_or_exit = 1;
			}
			if (strchr(argv[++i], '/') && !strstr(argv[i], "repeater://")) {
				struct stat sb;
				client_connect_file = strdup(argv[i]);
				if (stat(client_connect_file, &sb) != 0) {
					FILE* f = fopen(client_connect_file, "w");
					if (f != NULL) fclose(f);
				}
			} else {
				client_connect = strdup(argv[i]);
			}
			continue;
		}
		if (!strcmp(arg, "-proxy")) {
			CHECK_ARGC
			connect_proxy = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-vncconnect")) {
			vnc_connect = 1;
			continue;
		}
		if (!strcmp(arg, "-novncconnect")) {
			vnc_connect = 0;
			continue;
		}
		if (!strcmp(arg, "-allow")) {
			CHECK_ARGC
			allow_list = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-localhost")) {
			allow_list = strdup("127.0.0.1");
			got_localhost = 1;
			continue;
		}
		if (!strcmp(arg, "-unixsock")) {
			CHECK_ARGC
			unix_sock = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-listen6")) {
			CHECK_ARGC
#if X11VNC_IPV6
			listen_str6 = strdup(argv[++i]);
#else
			++i;
#endif
			continue;
		}
		if (!strcmp(arg, "-nolookup")) {
			host_lookup = 0;
			continue;
		}
		if (!strcmp(arg, "-6")) {
#if X11VNC_IPV6
			ipv6_listen = 1;
			got_ipv6_listen = 1;
#endif
			continue;
		}
		if (!strcmp(arg, "-no6")) {
#if X11VNC_IPV6
			ipv6_listen = 0;
			got_ipv6_listen = 0;
#endif
			continue;
		}
		if (!strcmp(arg, "-noipv6")) {
			noipv6 = 1;
			continue;
		}
		if (!strcmp(arg, "-noipv4")) {
			noipv4 = 1;
			continue;
		}

		if (!strcmp(arg, "-input")) {
			CHECK_ARGC
			allowed_input_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-grabkbd")) {
			grab_kbd = 1;
			continue;
		}
		if (!strcmp(arg, "-grabptr")) {
			grab_ptr = 1;
			continue;
		}
		if (!strcmp(arg, "-ungrabboth")) {
			ungrab_both = 1;
			continue;
		}
		if (!strcmp(arg, "-grabalways")) {
			grab_kbd = 1;
			grab_ptr = 1;
			grab_always = 1;
			continue;
		}
#ifdef ENABLE_GRABLOCAL
		if (!strcmp(arg, "-grablocal")) {
			CHECK_ARGC
			grab_local = atoi(argv[++i]);
			continue;
		}
#endif
		if (!strcmp(arg, "-viewpasswd")) {
			vpw_loc = i;
			CHECK_ARGC
			viewonly_passwd = strdup(argv[++i]);
			got_viewpasswd = 1;
			continue;
		}
		if (!strcmp(arg, "-passwdfile")) {
			CHECK_ARGC
			passwdfile = strdup(argv[++i]);
			got_passwdfile = 1;
			continue;
		}
		if (!strcmp(arg, "-svc") || !strcmp(arg, "-service")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvfb");
			unixpw = 1;
			users_list = strdup("unixpw=");
			use_openssl = 1;
			openssl_pem = strdup("SAVE");
			continue;
		}
		if (!strcmp(arg, "-svc_xdummy")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xdummy");
			unixpw = 1;
			users_list = strdup("unixpw=");
			use_openssl = 1;
			openssl_pem = strdup("SAVE");
			continue;
		}
		if (!strcmp(arg, "-svc_xdummy_xvfb")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xdummy,Xvfb");
			unixpw = 1;
			users_list = strdup("unixpw=");
			use_openssl = 1;
			openssl_pem = strdup("SAVE");
			continue;
		}
		if (!strcmp(arg, "-svc_xvnc")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvnc");
			unixpw = 1;
			users_list = strdup("unixpw=");
			use_openssl = 1;
			openssl_pem = strdup("SAVE");
			continue;
		}
		if (!strcmp(arg, "-xdmsvc") || !strcmp(arg, "-xdm_service")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvfb.xdmcp");
			unixpw = 1;
			users_list = strdup("unixpw=");
			use_openssl = 1;
			openssl_pem = strdup("SAVE");
			continue;
		}
		if (!strcmp(arg, "-sshxdmsvc")) {
			use_dpy = strdup("WAIT:cmd=FINDCREATEDISPLAY-Xvfb.xdmcp");
			allow_list = strdup("127.0.0.1");
			got_localhost = 1;
			continue;
		}
		if (!strcmp(arg, "-unixpw_system_greeter")) {
			unixpw_system_greeter = 1;
			continue;
		}
		if (!strcmp(arg, "-unixpw_cmd")
		    || !strcmp(arg, "-unixpw_cmd_unsafe")) {
			CHECK_ARGC
			unixpw_cmd = strdup(argv[++i]);
			unixpw = 1;
			if (strstr(arg, "_unsafe")) {
				/* hidden option for testing. */
				set_env("UNIXPW_DISABLE_SSL", "1");
				set_env("UNIXPW_DISABLE_LOCALHOST", "1");
			}
			continue;
		}
		if (strstr(arg, "-unixpw") == arg) {
			unixpw = 1;
			if (strstr(arg, "-unixpw_nis")) {
				unixpw_nis = 1;
			}
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					unixpw_list = strdup(s);
					i++;
				}
				if (s[0] == '%') {
					unixpw_list = NULL;
					quick_pw(s);
					exit(2);
				}
			}
			if (strstr(arg, "_unsafe")) {
				/* hidden option for testing. */
				set_env("UNIXPW_DISABLE_SSL", "1");
				set_env("UNIXPW_DISABLE_LOCALHOST", "1");
			}
			continue;
		}
		if (strstr(arg, "-nounixpw") == arg) {
			unixpw = 0;
			unixpw_nis = 0;
			if (unixpw_list) {
				unixpw_list = NULL;
			}
			if (unixpw_cmd) {
				unixpw_cmd = NULL;
			}
			continue;
		}
		if (!strcmp(arg, "-vencrypt")) {
			char *s;
			CHECK_ARGC
			s = strdup(argv[++i]);
			got_tls++;
			if (strstr(s, "never")) {
				vencrypt_mode = VENCRYPT_NONE;
			} else if (strstr(s, "support")) {
				vencrypt_mode = VENCRYPT_SUPPORT;
			} else if (strstr(s, "only")) {
				vencrypt_mode = VENCRYPT_SOLE;
			} else if (strstr(s, "force")) {
				vencrypt_mode = VENCRYPT_FORCE;
			} else {
				fprintf(stderr, "invalid %s arg: %s\n", arg, s);
				exit(1);
			}
			if (strstr(s, "nodh")) {
				vencrypt_kx = VENCRYPT_NODH;
			} else if (strstr(s, "nox509")) {
				vencrypt_kx = VENCRYPT_NOX509;
			}
			if (strstr(s, "newdh")) {
				create_fresh_dhparams = 1;
			}
			if (strstr(s, "noplain")) {
				vencrypt_enable_plain_login = 0;
			} else if (strstr(s, "plain")) {
				vencrypt_enable_plain_login = 1;
			}
			free(s);
			continue;
		}
		if (!strcmp(arg, "-anontls")) {
			char *s;
			CHECK_ARGC
			s = strdup(argv[++i]);
			got_tls++;
			if (strstr(s, "never")) {
				anontls_mode = ANONTLS_NONE;
			} else if (strstr(s, "support")) {
				anontls_mode = ANONTLS_SUPPORT;
			} else if (strstr(s, "only")) {
				anontls_mode = ANONTLS_SOLE;
			} else if (strstr(s, "force")) {
				anontls_mode = ANONTLS_FORCE;
			} else {
				fprintf(stderr, "invalid %s arg: %s\n", arg, s);
				exit(1);
			}
			if (strstr(s, "newdh")) {
				create_fresh_dhparams = 1;
			}
			free(s);
			continue;
		}
		if (!strcmp(arg, "-sslonly")) {
			vencrypt_mode = VENCRYPT_NONE;
			anontls_mode = ANONTLS_NONE;
			got_tls++;
			continue;
		}
		if (!strcmp(arg, "-dhparams")) {
			CHECK_ARGC
			dhparams_file = strdup(argv[++i]);
			got_tls++;
			continue;
		}
		if (!strcmp(arg, "-nossl")) {
			use_openssl = 0;
			openssl_pem = NULL;
			got_tls = -1000;
			continue;
		}
		if (!strcmp(arg, "-ssl")) {
			use_openssl = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					if (!strcmp(s, "ADH")) {
						openssl_pem = strdup("ANON");
					} else if (!strcmp(s, "ANONDH")) {
						openssl_pem = strdup("ANON");
					} else if (!strcmp(s, "TMP")) {
						openssl_pem = NULL;
					} else {
						openssl_pem = strdup(s);
					}
					i++;
				} else {
					openssl_pem = strdup("SAVE");
				}
			} else {
				openssl_pem = strdup("SAVE");
			}
			continue;
		}
		if (!strcmp(arg, "-enc")) {
			use_openssl = 1;
			CHECK_ARGC
			enc_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-http_oneport")) {
			http_oneport_msg = 1;
			use_openssl = 1;
			enc_str = strdup("none");
			continue;
		}
		if (!strcmp(arg, "-ssltimeout")) {
			CHECK_ARGC
			ssl_timeout_secs = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-sslnofail")) {
			ssl_no_fail = 1;
			continue;
		}
		if (!strcmp(arg, "-ssldir")) {
			CHECK_ARGC
			ssl_certs_dir = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-sslverify")) {
			CHECK_ARGC
			ssl_verify = strdup(argv[++i]);
			got_tls++;
			continue;
		}
		if (!strcmp(arg, "-sslCRL")) {
			CHECK_ARGC
			ssl_crl = strdup(argv[++i]);
			got_tls++;
			continue;
		}
		if (!strcmp(arg, "-sslGenCA")) {
			char *cdir = NULL;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					cdir = strdup(s);
					i++;
				}
			}
			sslGenCA(cdir);
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-sslGenCert")) {
			char *ty, *nm = NULL;
			if (i >= argc-1) {
				fprintf(stderr, "Must be:\n");
				fprintf(stderr, "          x11vnc -sslGenCert server ...\n");
				fprintf(stderr, "or        x11vnc -sslGenCert client ...\n");
				exit(1);
			}
			ty = argv[i+1];
			if (strcmp(ty, "server") && strcmp(ty, "client")) {
				fprintf(stderr, "Must be:\n");
				fprintf(stderr, "          x11vnc -sslGenCert server ...\n");
				fprintf(stderr, "or        x11vnc -sslGenCert client ...\n");
				exit(1);
			}
			if (i < argc-2) {
				nm = argv[i+2];
			}
			sslGenCert(ty, nm);
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-sslEncKey")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 0);
			}
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-sslCertInfo")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 1);
			}
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-sslDelCert")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 2);
			}
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-sslScripts")) {
			sslScripts();
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-stunnel")) {
			use_stunnel = 1;
			got_tls = -1000;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					if (!strcmp(s, "TMP")) {
						stunnel_pem = NULL;
					} else {
						stunnel_pem = strdup(s);
					}
					i++;
				} else {
					stunnel_pem = strdup("SAVE");
				}
			} else {
				stunnel_pem = strdup("SAVE");
			}
			continue;
		}
		if (!strcmp(arg, "-stunnel3")) {
			use_stunnel = 3;
			got_tls = -1000;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					if (!strcmp(s, "TMP")) {
						stunnel_pem = NULL;
					} else {
						stunnel_pem = strdup(s);
					}
					i++;
				} else {
					stunnel_pem = strdup("SAVE");
				}
			} else {
				stunnel_pem = strdup("SAVE");
			}
			continue;
		}
		if (!strcmp(arg, "-https")) {
			https_port_num = 0;
			try_http = 1;
			got_tls++;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					https_port_num = atoi(s);
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-httpsredir")) {
			https_port_redir = -1;
			got_tls++;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					https_port_redir = atoi(s);
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-nopw")) {
			nopw = 1;
			continue;
		}
		if (!strcmp(arg, "-ssh")) {
			CHECK_ARGC
			ssh_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-usepw")) {
			usepw = 1;
			continue;
		}
		if (!strcmp(arg, "-storepasswd")) {
			if (argc == i+1) {
				store_homedir_passwd(NULL);
				exit(0);
			}
			if (argc == i+2) {
				store_homedir_passwd(argv[i+1]);
				exit(0);
			}
			if (argc >= i+4 || rfbEncryptAndStorePasswd(argv[i+1],
			    argv[i+2]) != 0) {
				perror("storepasswd");
				fprintf(stderr, "-storepasswd failed for file: %s\n",
				    argv[i+2]);
				exit(1);
			} else {
				fprintf(stderr, "stored passwd in file: %s\n",
				    argv[i+2]);
				exit(0);
			}
			continue;
		}
		if (!strcmp(arg, "-showrfbauth")) {
			if (argc >= i+2) {
				char *f = argv[i+1];
				char *s = rfbDecryptPasswdFromFile(f);
				if (!s) {
					perror("showrfbauth");
					fprintf(stderr, "rfbDecryptPasswdFromFile failed: %s\n", f);
					exit(1);
				}
				fprintf(stdout, "rfbDecryptPasswdFromFile file: %s\n", f);
				fprintf(stdout, "rfbDecryptPasswdFromFile pass: %s\n", s);
			}
			exit(0);
		}
		if (!strcmp(arg, "-accept")) {
			CHECK_ARGC
			accept_cmd = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-afteraccept")) {
			CHECK_ARGC
			afteraccept_cmd = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-gone")) {
			CHECK_ARGC
			gone_cmd = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-noshm")) {
			using_shm = 0;
			continue;
		}
		if (!strcmp(arg, "-flipbyteorder")) {
			flip_byte_order = 1;
			continue;
		}
		if (!strcmp(arg, "-onetile")) {
			single_copytile = 1;
			continue;
		}
		if (!strcmp(arg, "-solid")) {
			use_solid_bg = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					solid_str = strdup(s);
					i++;
				}
			}
			if (! solid_str) {
				solid_str = strdup(solid_default);
			}
			continue;
		}
		if (!strcmp(arg, "-blackout")) {
			CHECK_ARGC
			blackout_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-xinerama")) {
			xinerama = 1;
			continue;
		}
		if (!strcmp(arg, "-noxinerama")) {
			xinerama = 0;
			continue;
		}
		if (!strcmp(arg, "-xtrap")) {
			xtrap_input = 1;
			continue;
		}
		if (!strcmp(arg, "-xrandr")) {
			xrandr = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (known_xrandr_mode(s)) {
					xrandr_mode = strdup(s);
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-noxrandr")) {
			xrandr = 0;
			xrandr_maybe = 0;
			got_noxrandr = 1;
			continue;
		}
		if (!strcmp(arg, "-rotate")) {
			CHECK_ARGC
			rotating_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-padgeom")
		    || !strcmp(arg, "-padgeometry")) {
			CHECK_ARGC
			pad_geometry = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-o") || !strcmp(arg, "-logfile")) {
			CHECK_ARGC
			logfile_append = 0;
			logfile = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-oa") || !strcmp(arg, "-logappend")) {
			CHECK_ARGC
			logfile_append = 1;
			logfile = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-flag")) {
			CHECK_ARGC
			flagfile = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-rmflag")) {
			CHECK_ARGC
			rm_flagfile = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-rc")) {
			i++;	/* done above */
			continue;
		}
		if (!strcmp(arg, "-norc")) {
			;	/* done above */
			continue;
		}
		if (!strcmp(arg, "-env")) {
			i++;	/* done above */
			continue;
		}
		if (!strcmp(arg, "-prog")) {
			CHECK_ARGC
			if (program_name) {
				free(program_name);
			}
			program_name = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-h") || !strcmp(arg, "-help")) {
			print_help(0);
			continue;
		}
		if (!strcmp(arg, "-?") || !strcmp(arg, "-opts")) {
			print_help(1);
			continue;
		}
		if (!strcmp(arg, "-V") || !strcmp(arg, "-version")) {
			fprintf(stdout, "x11vnc: %s\n", lastmod);
			exit(0);
			continue;
		}
		if (!strcmp(arg, "-license") ||
		    !strcmp(arg, "-copying") || !strcmp(arg, "-warranty")) {
			print_license();
			continue;
		}
		if (!strcmp(arg, "-dbg")) {
			crash_debug = 1;
			continue;
		}
		if (!strcmp(arg, "-nodbg")) {
			crash_debug = 0;
			continue;
		}
		if (!strcmp(arg, "-q") || !strcmp(arg, "-quiet")) {
			quiet = 1;
			continue;
		}
		if (!strcmp(arg, "-noquiet")) {
			quiet = 0;
			continue;
		}
		if (!strcmp(arg, "-v") || !strcmp(arg, "-verbose")) {
			verbose = 1;
			continue;
		}
		if (!strcmp(arg, "-bg") || !strcmp(arg, "-background")) {
#if LIBVNCSERVER_HAVE_SETSID
			bg = 1;
			opts_bg = bg;
#else
			if (!got_inetd) {
				fprintf(stderr, "warning: -bg mode not supported.\n");
			}
#endif
			continue;
		}
		if (!strcmp(arg, "-modtweak")) {
			use_modifier_tweak = 1;
			continue;
		}
		if (!strcmp(arg, "-nomodtweak")) {
			use_modifier_tweak = 0;
			got_nomodtweak = 1;
			continue;
		}
		if (!strcmp(arg, "-isolevel3")) {
			use_iso_level3 = 1;
			continue;
		}
		if (!strcmp(arg, "-xkb")) {
			use_modifier_tweak = 1;
			use_xkb_modtweak = 1;
			continue;
		}
		if (!strcmp(arg, "-noxkb")) {
			use_xkb_modtweak = 0;
			got_noxkb = 1;
			continue;
		}
		if (!strcmp(arg, "-capslock")) {
			watch_capslock = 1;
			continue;
		}
		if (!strcmp(arg, "-skip_lockkeys")) {
			skip_lockkeys = 1;
			continue;
		}
		if (!strcmp(arg, "-noskip_lockkeys")) {
			skip_lockkeys = 0;
			continue;
		}
		if (!strcmp(arg, "-xkbcompat")) {
			xkbcompat = 1;
			continue;
		}
		if (!strcmp(arg, "-skip_keycodes")) {
			CHECK_ARGC
			skip_keycodes = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-sloppy_keys")) {
			sloppy_keys++;
			continue;
		}
		if (!strcmp(arg, "-skip_dups")) {
			skip_duplicate_key_events = 1;
			continue;
		}
		if (!strcmp(arg, "-noskip_dups")) {
			skip_duplicate_key_events = 0;
			continue;
		}
		if (!strcmp(arg, "-add_keysyms")) {
			add_keysyms++;
			continue;
		}
		if (!strcmp(arg, "-noadd_keysyms")) {
			add_keysyms = 0;
			continue;
		}
		if (!strcmp(arg, "-clear_mods")) {
			clear_mods = 1;
			continue;
		}
		if (!strcmp(arg, "-clear_keys")) {
			clear_mods = 2;
			continue;
		}
		if (!strcmp(arg, "-clear_all")) {
			clear_mods = 3;
			continue;
		}
		if (!strcmp(arg, "-remap")) {
			CHECK_ARGC
			remap_file = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-norepeat")) {
			no_autorepeat = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s == '-') {
					s++;
				}
				if (isdigit((unsigned char) (*s))) {
					no_repeat_countdown = atoi(argv[++i]);
				}
			}
			continue;
		}
		if (!strcmp(arg, "-repeat")) {
			no_autorepeat = 0;
			continue;
		}
		if (!strcmp(arg, "-nofb")) {
			nofb = 1;
			continue;
		}
		if (!strcmp(arg, "-nobell")) {
			watch_bell = 0;
			sound_bell = 0;
			continue;
		}
		if (!strcmp(arg, "-nosel")) {
			watch_selection = 0;
			watch_primary = 0;
			watch_clipboard = 0;
			continue;
		}
		if (!strcmp(arg, "-noprimary")) {
			watch_primary = 0;
			continue;
		}
		if (!strcmp(arg, "-nosetprimary")) {
			set_primary = 0;
			continue;
		}
		if (!strcmp(arg, "-noclipboard")) {
			watch_clipboard = 0;
			continue;
		}
		if (!strcmp(arg, "-nosetclipboard")) {
			set_clipboard = 0;
			continue;
		}
		if (!strcmp(arg, "-seldir")) {
			CHECK_ARGC
			sel_direction = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-cursor")) {
			show_cursor = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (known_cursors_mode(s)) {
					multiple_cursors_mode = strdup(s);
					i++;
					if (!strcmp(s, "none")) {
						show_cursor = 0;
					}
				}
			}
			continue;
		}
		if (!strcmp(arg, "-nocursor")) { 
			multiple_cursors_mode = strdup("none");
			show_cursor = 0;
			continue;
		}
		if (!strcmp(arg, "-cursor_drag")) { 
			cursor_drag_changes = 1;
			continue;
		}
		if (!strcmp(arg, "-nocursor_drag")) { 
			cursor_drag_changes = 0;
			continue;
		}
		if (!strcmp(arg, "-arrow")) {
			CHECK_ARGC
			alt_arrow = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-xfixes")) { 
			use_xfixes = 1;
			continue;
		}
		if (!strcmp(arg, "-noxfixes")) { 
			use_xfixes = 0;
			continue;
		}
		if (!strcmp(arg, "-alphacut")) {
			CHECK_ARGC
			alpha_threshold = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-alphafrac")) {
			CHECK_ARGC
			alpha_frac = atof(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-alpharemove")) {
			alpha_remove = 1;
			continue;
		}
		if (!strcmp(arg, "-noalphablend")) {
			alpha_blend = 0;
			continue;
		}
		if (!strcmp(arg, "-nocursorshape")) {
			cursor_shape_updates = 0;
			continue;
		}
		if (!strcmp(arg, "-cursorpos")) {
			cursor_pos_updates = 1;
			got_cursorpos = 1;
			continue;
		}
		if (!strcmp(arg, "-nocursorpos")) {
			cursor_pos_updates = 0;
			continue;
		}
		if (!strcmp(arg, "-xwarppointer")) {
			use_xwarppointer = 1;
			continue;
		}
		if (!strcmp(arg, "-noxwarppointer")) {
			use_xwarppointer = 0;
			got_noxwarppointer = 1;
			continue;
		}
		if (!strcmp(arg, "-always_inject")) {
			always_inject = 1;
			continue;
		}
		if (!strcmp(arg, "-buttonmap")) {
			CHECK_ARGC
			pointer_remap = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-nodragging")) {
			show_dragging = 0;
			continue;
		}
#ifndef NO_NCACHE
		if (!strcmp(arg, "-ncache") || !strcmp(arg, "-nc")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					ncache = atoi(s);
					i++;
				} else {
					ncache = ncache_default;
				}
			} else {
				ncache = ncache_default;
			}
			if (ncache % 2 != 0) {
				ncache++;
			}
			continue;
		}
		if (!strcmp(arg, "-noncache") || !strcmp(arg, "-nonc")) {
			ncache = 0;
			continue;
		}
		if (!strcmp(arg, "-ncache_cr") || !strcmp(arg, "-nc_cr")) {
			ncache_copyrect = 1;
			continue;
		}
		if (!strcmp(arg, "-ncache_no_moveraise") || !strcmp(arg, "-nc_no_moveraise")) {
			ncache_wf_raises = 1;
			continue;
		}
		if (!strcmp(arg, "-ncache_no_dtchange") || !strcmp(arg, "-nc_no_dtchange")) {
			ncache_dt_change = 0;
			continue;
		}
		if (!strcmp(arg, "-ncache_no_rootpixmap") || !strcmp(arg, "-nc_no_rootpixmap")) {
			ncache_xrootpmap = 0;
			continue;
		}
		if (!strcmp(arg, "-ncache_keep_anims") || !strcmp(arg, "-nc_keep_anims")) {
			ncache_keep_anims = 1;
			continue;
		}
		if (!strcmp(arg, "-ncache_old_wm") || !strcmp(arg, "-nc_old_wm")) {
			ncache_old_wm = 1;
			continue;
		}
		if (!strcmp(arg, "-ncache_pad") || !strcmp(arg, "-nc_pad")) {
			CHECK_ARGC
			ncache_pad = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-debug_ncache")) {
			ncdb++;
			continue;
		}
#endif
		if (!strcmp(arg, "-wireframe")
		    || !strcmp(arg, "-wf")) {
			wireframe = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s != '-') {
					wireframe_str = strdup(argv[++i]);
				}
			}
			continue;
		}
		if (!strcmp(arg, "-nowireframe")
		    || !strcmp(arg, "-nowf")) {
			wireframe = 0;
			continue;
		}
		if (!strcmp(arg, "-nowireframelocal")
		    || !strcmp(arg, "-nowfl")) {
			wireframe_local = 0;
			continue;
		}
		if (!strcmp(arg, "-wirecopyrect")
		    || !strcmp(arg, "-wcr")) {
			CHECK_ARGC
			set_wirecopyrect_mode(argv[++i]);
			got_wirecopyrect = 1;
			continue;
		}
		if (!strcmp(arg, "-nowirecopyrect")
		    || !strcmp(arg, "-nowcr")) {
			set_wirecopyrect_mode("never");
			continue;
		}
		if (!strcmp(arg, "-debug_wireframe")
		    || !strcmp(arg, "-dwf")) {
			debug_wireframe++;
			continue;
		}
		if (!strcmp(arg, "-scrollcopyrect")
		    || !strcmp(arg, "-scr")) {
			CHECK_ARGC
			set_scrollcopyrect_mode(argv[++i]);
			got_scrollcopyrect = 1;
			continue;
		}
		if (!strcmp(arg, "-noscrollcopyrect")
		    || !strcmp(arg, "-noscr")) {
			set_scrollcopyrect_mode("never");
			continue;
		}
		if (!strcmp(arg, "-scr_area")) {
			int tn;
			CHECK_ARGC
			tn = atoi(argv[++i]);
			if (tn >= 0) {
				scrollcopyrect_min_area = tn;
			}
			continue;
		}
		if (!strcmp(arg, "-scr_skip")) {
			CHECK_ARGC
			scroll_skip_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scr_inc")) {
			CHECK_ARGC
			scroll_good_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scr_keys")) {
			CHECK_ARGC
			scroll_key_list_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scr_term")) {
			CHECK_ARGC
			scroll_term_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scr_keyrepeat")) {
			CHECK_ARGC
			max_keyrepeat_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-scr_parms")) {
			CHECK_ARGC
			scroll_copyrect_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-fixscreen")) {
			CHECK_ARGC
			screen_fixup_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-debug_scroll")
		    || !strcmp(arg, "-ds")) {
			debug_scroll++;
			continue;
		}
		if (!strcmp(arg, "-noxrecord")) {
			noxrecord = 1;
			continue;
		}
		if (!strcmp(arg, "-pointer_mode")
		    || !strcmp(arg, "-pm")) {
			char *p, *s;
			CHECK_ARGC
			s = argv[++i];
			if ((p = strchr(s, ':')) != NULL) {
				ui_skip = atoi(p+1);
				if (! ui_skip) ui_skip = 1;
				*p = '\0';
			}
			if (atoi(s) < 1 || atoi(s) > pointer_mode_max) {
				if (!got_inetd) {
					rfbLog("pointer_mode out of range 1-%d: %d\n",
					    pointer_mode_max, atoi(s));
				}
			} else {
				pointer_mode = atoi(s);
				got_pointer_mode = pointer_mode;
			}
			continue;
		}
		if (!strcmp(arg, "-input_skip")) {
			CHECK_ARGC
			ui_skip = atoi(argv[++i]);
			if (! ui_skip) ui_skip = 1;
			continue;
		}
		if (!strcmp(arg, "-allinput")) {
			all_input = 1;
			continue;
		}
		if (!strcmp(arg, "-noallinput")) {
			all_input = 0;
			continue;
		}
		if (!strcmp(arg, "-input_eagerly")) {
			handle_events_eagerly = 1;
			continue;
		}
		if (!strcmp(arg, "-noinput_eagerly")) {
			handle_events_eagerly = 0;
			continue;
		}
		if (!strcmp(arg, "-speeds")) {
			CHECK_ARGC
			speeds_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-wmdt")) {
			CHECK_ARGC
			wmdt_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-debug_pointer")
		    || !strcmp(arg, "-dp")) {
			debug_pointer++;
			continue;
		}
		if (!strcmp(arg, "-debug_keyboard")
		    || !strcmp(arg, "-dk")) {
			debug_keyboard++;
			continue;
		}
		if (!strcmp(arg, "-debug_xdamage")) {
			debug_xdamage++;
			continue;
		}
		if (!strcmp(arg, "-defer")) {
			CHECK_ARGC
			defer_update = atoi(argv[++i]);
			got_defer = 1;
			continue;
		}
		if (!strcmp(arg, "-setdefer")) {
			CHECK_ARGC
			set_defer = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-wait")) {
			CHECK_ARGC
			waitms = atoi(argv[++i]);
			got_waitms = 1;
			continue;
		}
		if (!strcmp(arg, "-extra_fbur")) {
			CHECK_ARGC
			extra_fbur = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-wait_ui")) {
			CHECK_ARGC
			wait_ui = atof(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-nowait_bog")) {
			wait_bog = 0;
			continue;
		}
		if (!strcmp(arg, "-slow_fb")) {
			CHECK_ARGC
			slow_fb = atof(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-xrefresh")) {
			CHECK_ARGC
			xrefresh = atof(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-readtimeout")) {
			CHECK_ARGC
			rfbMaxClientWait = atoi(argv[++i]) * 1000;
			continue;
		}
		if (!strcmp(arg, "-ping")) {
			CHECK_ARGC
			ping_interval = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-nap")) {
			take_naps = 1;
			continue;
		}
		if (!strcmp(arg, "-nonap")) {
			take_naps = 0;
			continue;
		}
		if (!strcmp(arg, "-sb")) {
			CHECK_ARGC
			screen_blank = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-nofbpm")) {
			watch_fbpm = 1;
			continue;
		}
		if (!strcmp(arg, "-fbpm")) {
			watch_fbpm = 0;
			continue;
		}
		if (!strcmp(arg, "-nodpms")) {
			watch_dpms = 1;
			continue;
		}
		if (!strcmp(arg, "-dpms")) {
			watch_dpms = 0;
			continue;
		}
		if (!strcmp(arg, "-forcedpms")) {
			force_dpms = 1;
			continue;
		}
		if (!strcmp(arg, "-clientdpms")) {
			client_dpms = 1;
			continue;
		}
		if (!strcmp(arg, "-noserverdpms")) {
			no_ultra_dpms = 1;
			continue;
		}
		if (!strcmp(arg, "-noultraext")) {
			no_ultra_ext = 1;
			continue;
		}
		if (!strcmp(arg, "-chatwindow")) {
			chat_window = 1;
			if (argc_vnc + 1 < argc_vnc_max) {
				if (!got_inetd) {
					rfbLog("setting '-rfbversion 3.6' for -chatwindow.\n");
				}
				argv_vnc[argc_vnc++] = strdup("-rfbversion");
				argv_vnc[argc_vnc++] = strdup("3.6");
			}
			continue;
		}
		if (!strcmp(arg, "-xdamage")) {
			use_xdamage++;
			continue;
		}
		if (!strcmp(arg, "-noxdamage")) {
			use_xdamage = 0;
			continue;
		}
		if (!strcmp(arg, "-xd_area")) {
			int tn;
			CHECK_ARGC
			tn = atoi(argv[++i]);
			if (tn >= 0) {
				xdamage_max_area = tn;
			}
			continue;
		}
		if (!strcmp(arg, "-xd_mem")) {
			double f;
			CHECK_ARGC
			f = atof(argv[++i]);
			if (f >= 0.0) {
				xdamage_memory = f;
			}
			continue;
		}
		if (!strcmp(arg, "-sigpipe") || !strcmp(arg, "-sig")) {
			CHECK_ARGC
			if (known_sigpipe_mode(argv[++i])) {
				sigpipe = strdup(argv[i]);
			} else {
				fprintf(stderr, "invalid -sigpipe arg: %s, must"
				    " be \"ignore\" or \"exit\"\n", argv[i]);
				exit(1);
			}
			continue;
		}
#if LIBVNCSERVER_HAVE_LIBPTHREAD
		if (!strcmp(arg, "-threads")) {
#if defined(X11VNC_THREADED)
			use_threads = 1;
#else
			if (getenv("X11VNC_THREADED")) {
				use_threads = 1;
			} else if (1) {
				/* we re-enable it due to threaded mode bugfixes. */
				use_threads = 1;
			} else {
				if (!got_inetd) {
					rfbLog("\n");
					rfbLog("The -threads mode is unstable and not tested or maintained.\n");
					rfbLog("It is disabled in the source code.  If you really need\n");
					rfbLog("the feature you can reenable it at build time by setting\n");
					rfbLog("-DX11VNC_THREADED in CPPFLAGS. Or set X11VNC_THREADED=1\n");
					rfbLog("in your runtime environment.\n");
					rfbLog("\n");
					usleep(500*1000);
				}
			}
#endif
			continue;
		}
		if (!strcmp(arg, "-nothreads")) {
			use_threads = 0;
			continue;
		}
#endif
		if (!strcmp(arg, "-fs")) {
			CHECK_ARGC
			fs_frac = atof(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-gaps")) {
			CHECK_ARGC
			gaps_fill = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-grow")) {
			CHECK_ARGC
			grow_fill = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-fuzz")) {
			CHECK_ARGC
			tile_fuzz = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-debug_tiles")
		    || !strcmp(arg, "-dbt")) {
			debug_tiles++;
			continue;
		}
		if (!strcmp(arg, "-debug_grabs")) {
			debug_grabs++;
			continue;
		}
		if (!strcmp(arg, "-debug_sel")) {
			debug_sel++;
			continue;
		}
		if (!strcmp(arg, "-grab_buster")) {
			grab_buster++;
			continue;
		}
		if (!strcmp(arg, "-nograb_buster")) {
			grab_buster = 0;
			continue;
		}
		if (!strcmp(arg, "-snapfb")) {
			use_snapfb = 1;
			continue;
		}
		if (!strcmp(arg, "-rand")) {
			/* equiv. to -nopw -rawfb rand for quick tests */
			raw_fb_str = strdup("rand");
			nopw = 1;
			continue;
		}
		if (!strcmp(arg, "-rawfb")) {
			CHECK_ARGC
			raw_fb_str = strdup(argv[++i]);
			if (strstr(raw_fb_str, "vnc:") == raw_fb_str) {
				shared = 1;
			}
			continue;
		}
		if (!strcmp(arg, "-freqtab")) {
			CHECK_ARGC
			freqtab = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-pipeinput")) {
			CHECK_ARGC
			pipeinput_str = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-macnodim")) {
			macosx_nodimming = 1;
			continue;
		}
		if (!strcmp(arg, "-macnosleep")) {
			macosx_nosleep = 1;
			continue;
		}
		if (!strcmp(arg, "-macnosaver")) {
			macosx_noscreensaver = 1;
			continue;
		}
		if (!strcmp(arg, "-macnowait")) {
			macosx_wait_for_switch = 0;
			continue;
		}
		if (!strcmp(arg, "-macwheel")) {
			CHECK_ARGC
			macosx_mouse_wheel_speed = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-macnoswap")) {
			macosx_swap23 = 0;
			continue;
		}
		if (!strcmp(arg, "-macnoresize")) {
			macosx_resize = 0;
			continue;
		}
		if (!strcmp(arg, "-maciconanim")) {
			CHECK_ARGC
			macosx_icon_anim_time = atoi(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-macmenu")) {
			macosx_ncache_macmenu = 1;
			continue;
		}
		if (!strcmp(arg, "-macuskbd")) {
			macosx_us_kbd = 1;
			continue;
		}
		if (!strcmp(arg, "-macnoopengl")) {
			macosx_no_opengl = 1;
			continue;
		}
		if (!strcmp(arg, "-macnorawfb")) {
			macosx_no_rawfb = 1;
			continue;
		}
		if (!strcmp(arg, "-gui")) {
			launch_gui = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s != '-') {
					gui_str = strdup(s);
					if (strstr(gui_str, "setp")) {
						got_gui_pw = 1;
					}
					i++;
				}
			}
			continue;
		}
		if (!strcmp(arg, "-remote") || !strcmp(arg, "-R")
		    || !strcmp(arg, "-r") || !strcmp(arg, "-remote-control")) {
			char *str;
			CHECK_ARGC
			i++;
			str = argv[i];
			if (*str == '-') {
				/* accidental leading '-' */
				str++;
			}
			if (!strcmp(str, "ping")) {
				query_cmd = strdup(str);
			} else {
				remote_cmd = strdup(str);
			}
			if (remote_cmd && strchr(remote_cmd, ':') == NULL) {
			    /* interpret -R -scale 3/4 at least */
		 	    if (i < argc-1 && *(argv[i+1]) != '-') {
				int n;

				/* it must be the parameter value */
				i++;
				n = strlen(remote_cmd) + strlen(argv[i]) + 2;

				str = (char *) malloc(n);
				sprintf(str, "%s:%s", remote_cmd, argv[i]);
				free(remote_cmd);
				remote_cmd = str;
			    }
			}
			if (!getenv("QUERY_VERBOSE")) {
				quiet = 1;
			}
			xkbcompat = 0;
			continue;
		}
		if (!strcmp(arg, "-query") || !strcmp(arg, "-Q")) {
			CHECK_ARGC
			query_cmd = strdup(argv[++i]);
			if (!getenv("QUERY_VERBOSE")) {
				quiet = 1;
			}
			xkbcompat = 0;
			continue;
		}
		if (!strcmp(arg, "-query_retries")) {
			char *s;
			CHECK_ARGC
			s = strdup(argv[++i]);
			/* n[:t][/match] */
			if (strchr(s, '/')) {
				char *q = strchr(s, '/');
				query_match = strdup(q+1);
				*q = '\0';
			}
			if (strchr(s, ':')) {
				char *q = strchr(s, ':');
				query_delay = atof(q+1);
			}
			query_retries = atoi(s);
			free(s);
			continue;
		}
		if (!strcmp(arg, "-QD")) {
			CHECK_ARGC
			query_cmd = strdup(argv[++i]);
			query_default = 1;
			continue;
		}
		if (!strcmp(arg, "-sync")) {
			remote_sync = 1;
			continue;
		}
		if (!strcmp(arg, "-nosync")) {
			remote_sync = 0;
			continue;
		}
		if (!strcmp(arg, "-remote_prefix")) {
			CHECK_ARGC
			remote_prefix = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-noremote")) {
			accept_remote_cmds = 0;
			continue;
		}
		if (!strcmp(arg, "-yesremote")) {
			accept_remote_cmds = 1;
			continue;
		}
		if (!strcmp(arg, "-unsafe")) {
			safe_remote_only = 0;
			continue;
		}
		if (!strcmp(arg, "-privremote")) {
			priv_remote = 1;
			continue;
		}
		if (!strcmp(arg, "-safer")) {
			more_safe = 1;
			continue;
		}
		if (!strcmp(arg, "-nocmds")) {
			no_external_cmds = 1;
			continue;
		}
		if (!strcmp(arg, "-allowedcmds")) {
			CHECK_ARGC
			allowed_external_cmds = strdup(argv[++i]);
			continue;
		}
		if (!strcmp(arg, "-deny_all")) {
			deny_all = 1;
			continue;
		}
		if (!strcmp(arg, "-httpdir")) {
			CHECK_ARGC
			http_dir = strdup(argv[++i]);
			got_httpdir = 1;
			continue;
		}
		if (1) {
			if (!strcmp(arg, "-desktop") && i < argc-1) {
				dt = 1;
				rfb_desktop_name = strdup(argv[i+1]);
			}
			if (!strcmp(arg, "-passwd")) {
				pw_loc = i;
				got_passwd = 1;
			}
			if (!strcmp(arg, "-rfbauth")) {
				got_rfbauth = 1;
			}
			if (!strcmp(arg, "-rfbwait")) {
				got_rfbwait = 1;
			}
			if (!strcmp(arg, "-deferupdate")) {
				got_deferupdate = 1;
			}
			if (!strcmp(arg, "-rfbport") && i < argc-1) {
				got_rfbport = 1;
				if (!strcasecmp(argv[i+1], "prompt")) {
					;
				} else if (!is_decimal(argv[i+1])) {
					if (!got_inetd) {
						rfbLog("Invalid -rfbport value: '%s'\n", argv[i+1]);
						rfbLog("setting it to '-1' to induce failure.\n");
						argv[i+1] = strdup("-1");
					}
				}
				got_rfbport_str = strdup(argv[i+1]);
				got_rfbport_pos = argc_vnc+1;
				got_rfbport_val = atoi(argv[i+1]);
			}
			if (!strcmp(arg, "-httpport") && i < argc-1) {
				if (!is_decimal(argv[i+1])) {
					rfbLog("Invalid -httpport value: '%s'\n", argv[i+1]);
					clean_up_exit(1);
				}
			}
			if (!strcmp(arg, "-alwaysshared ")) {
				got_alwaysshared = 1;
			}
			if (!strcmp(arg, "-nevershared")) {
				got_nevershared = 1;
			}
			if (!strcmp(arg, "-listen") && i < argc-1) {
				listen_str = strdup(argv[i+1]);
			}
			/* otherwise copy it for libvncserver use below. */
			if (!strcmp(arg, "-ultrafilexfer")) {
				got_ultrafilexfer = 1;
			} else if (argc_vnc < argc_vnc_max) {
				argv_vnc[argc_vnc++] = strdup(arg);
			} else {
				rfbLog("too many arguments.\n");
				exit(1);
			}
			continue;
		}
	}

	if (! getenv("NO_LIBXCB_ALLOW_SLOPPY_LOCK")) {
		/* libxcb is a bit too strict for us sometimes... */
		set_env("LIBXCB_ALLOW_SLOPPY_LOCK", "1");
	}

	if (getenv("PATH") == NULL || !strcmp(getenv("PATH"), "")) {
		/* set a minimal PATH, usually only null in inetd. */
		set_env("PATH", "/bin:/usr/bin");
	}

	/* handle -findauth case now that cmdline has been read */
	if (got_findauth) {
		char *s;
		int ic = 0;
		if (use_dpy != NULL) {
			set_env("DISPLAY", use_dpy);
		}
		use_dpy = strdup("WAIT:cmd=FINDDISPLAY-run");

		s = getenv("FINDAUTH_DISPLAY");
		if (s && strcmp("", s)) {
			set_env("DISPLAY", s);
		}
		s = getenv("DISPLAY");
		if (s && strcmp("", s)) {
			set_env("X11VNC_SKIP_DISPLAY", s);
		} else {
			set_env("X11VNC_SKIP_DISPLAY", ":0");
		}
		set_env("X11VNC_SKIP_DISPLAY_NEGATE", "1");
		set_env("FIND_DISPLAY_XAUTHORITY_PATH", "1");
		set_env("FIND_DISPLAY_NO_SHOW_XAUTH", "1");
		set_env("FIND_DISPLAY_NO_SHOW_DISPLAY", "1");
		wait_for_client(&ic, NULL, 0);
		exit(0);
	}

#ifndef WIN32
	/* set OS struct UT */
	uname(&UT);
#endif

	orig_use_xdamage = use_xdamage;

	if (!auto_port && getenv("AUTO_PORT")) {
		auto_port = atoi(getenv("AUTO_PORT"));
	}

	if (getenv("X11VNC_LOOP_MODE")) {
		if (bg && !getenv("X11VNC_LOOP_MODE_BG")) {
			if (! quiet) {
				fprintf(stderr, "disabling -bg in -loop "
				    "mode\n");
			}
			bg = 0;
		} else if (!bg && getenv("X11VNC_LOOP_MODE_BG")) {
			if (! quiet) {
				fprintf(stderr, "enabling -bg in -loopbg "
				    "mode\n");
			}
			bg = 1;
		}
		if (inetd) {
			if (! quiet) {
				fprintf(stderr, "disabling -inetd in -loop "
				    "mode\n");
			}
			inetd = 0;
		}
	}

	if (launch_gui && (query_cmd || remote_cmd)) {
		launch_gui = 0;
		gui_str = NULL;
	}
	if (more_safe) {
		launch_gui = 0;
	}

#ifdef MACOSX
	if (! use_dpy) {
		/* we need this for gui since no X properties */
		if (!client_connect_file && !client_connect) {
			char *user = get_user_name();
			char *str = (char *) malloc(strlen(user) + strlen("/tmp/x11vnc-macosx-remote.") + 1);
			struct stat sb;
			sprintf(str, "/tmp/x11vnc-macosx-remote.%s", user);
			if (!remote_cmd && !query_cmd) {
				unlink(str);
				if (stat(str, &sb) != 0) {
					int fd = open(str, O_WRONLY|O_EXCL|O_CREAT, 0600);
					if (fd >= 0) {
						close(fd);
						client_connect_file = str;
					}
				}
			} else {
				client_connect_file = str;
			}
			if (client_connect_file) {
				if (!got_inetd) {
					rfbLog("MacOS X: set -connect file to %s\n", client_connect_file);
				}
			}
		}
	}
#endif
	if (got_rfbport_str != NULL && !strcasecmp(got_rfbport_str, "prompt")) {
		char *opts, tport[32];

		if (gui_str) {
			opts = (char *) malloc(strlen(gui_str) + 32);
			sprintf(opts, "%s,portprompt", gui_str);
		} else {
			opts = strdup("portprompt");
		}
		got_rfbport_val = -1;

		do_gui(opts, 0);
		if (got_rfbport_val == -1) {
			rfbLog("Port prompt indicated cancel.\n");
			clean_up_exit(1);
		}
		if (!got_inetd) {
			rfbLog("Port prompt selected: %d\n", got_rfbport_val);
		}
		sprintf(tport, "%d", got_rfbport_val);
		argv_vnc[got_rfbport_pos] = strdup(tport);
		free(opts);
	}

	{
		char num[32];
		sprintf(num, "%d", got_rfbport_val);
		set_env("X11VNC_GOT_RFBPORT_VAL", num);
	}

	if (got_ultrafilexfer && argc_vnc + 2 < argc_vnc_max) {
		argv_vnc[argc_vnc++] = strdup("-rfbversion");
		argv_vnc[argc_vnc++] = strdup("3.6");
		argv_vnc[argc_vnc++] = strdup("-permitfiletransfer");
	}
	
	if (launch_gui) {
		int sleep = 0;
		if (SHOW_NO_PASSWORD_WARNING && !nopw) {
			sleep = 1;
		}
		do_gui(gui_str, sleep);
	}
	if (logfile) {
		int n;
		char *pstr = "%VNCDISPLAY";
		if (strstr(logfile, pstr)) {
			char *h = this_host();
			char *s, *q, *newlog;
			int n, p = got_rfbport_val;
			/* we don't really know the port yet... so guess */
			if (p < 0) {
				p = auto_port;
			}
			if (p <= 0) {
				p = 5900;
			}
			s = (char *) malloc(strlen(h) + 32);
			sprintf(s, "%s:%d", h, p);
			n = 1;
			q = logfile;
			while (1) {
				char *t = strstr(q, pstr);
				if (!t) break;
				n++;
				q = t+1; 
			}
			newlog = (char *) malloc(strlen(logfile) + n * strlen(pstr));
			newlog[0] = '\0';

			q = logfile;
			while (1) {
				char *t = strstr(q, pstr);
				if (!t) {
					strcat(newlog, q);
					break;
				}
				strncat(newlog, q, t - q);
				strcat(newlog, s);
				q = t + strlen(pstr); 
			}
			logfile = newlog;
			if (!quiet && !got_inetd) {
				rfbLog("Expanded logfile to '%s'\n", newlog);
				
			}
			free(s);
		}
		pstr = "%HOME";
		if (strstr(logfile, pstr)) {
			char *h = get_home_dir();
			char *s, *q, *newlog;

			s = (char *) malloc(strlen(h) + 32);
			sprintf(s, "%s", h);
			n = 1;
			q = logfile;
			while (1) {
				char *t = strstr(q, pstr);
				if (!t) break;
				n++;
				q = t+1; 
			}
			newlog = (char *) malloc(strlen(logfile) + n * strlen(pstr));
			newlog[0] = '\0';

			q = logfile;
			while (1) {
				char *t = strstr(q, pstr);
				if (!t) {
					strcat(newlog, q);
					break;
				}
				strncat(newlog, q, t - q);
				strcat(newlog, s);
				q = t + strlen(pstr); 
			}
			logfile = newlog;
			if (!quiet && !got_inetd) {
				rfbLog("Expanded logfile to '%s'\n", newlog);
			}
			free(s);
		}

		if (logfile_append) {
			n = open(logfile, O_WRONLY|O_CREAT|O_APPEND, 0666);
		} else {
			n = open(logfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		}
		if (n < 0) {
			fprintf(stderr, "error opening logfile: %s\n", logfile);
			perror("open");
			exit(1);
		}
		if (dup2(n, 2) < 0) {
			fprintf(stderr, "dup2 failed\n");
			perror("dup2");
			exit(1);
		}
		if (n > 2) {
			close(n);
		}
	}
	if (ipv6_listen) {
		if (inetd) {
			ipv6_listen = 0;
		}
	}
	if (inetd && quiet && !logfile) {
		int n;
		/*
		 * Redir stderr to /dev/null under -inetd and -quiet
		 * but no -o logfile.  Typical problem:
		 *   Xlib:  extension "RECORD" missing on display ":1.0".
		 * If they want this info, they should use -o logfile,
		 * or no -q and 2>logfile.
		 */
		n = open("/dev/null", O_WRONLY);
		if (n >= 0) {
			if (dup2(n, 2) >= 0) {
				if (n > 2) {
					close(n);
				}
			}
		}
	}
	if (! quiet && ! inetd) {
		int i;
		if (http_oneport_msg) {
			rfbLog("setting '-enc none' for -http_oneport mode.\n");
		}
		for (i=1; i < argc_vnc; i++) {
			rfbLog("passing arg to libvncserver: %s\n", argv_vnc[i]);
			if (!strcmp(argv_vnc[i], "-passwd")) {
				i++;
			}
		}
	}

	if (remote_cmd || query_cmd) {
		/*
		 * no need to open DISPLAY, just write it to the file now
		 * similar for query_default.
		 */
		if (client_connect_file || query_default) {
			int i, rc = 1;
			for (i=0; i <= query_retries; i++) {
				rc = do_remote_query(remote_cmd, query_cmd,
				    remote_sync, query_default);
				if (rc == 0) {
					if (query_match) {
						if (query_result && strstr(query_result, query_match)) {
							break;
						}
						rc = 1;
					} else {
						break;
					}
				}
				if (i < query_retries) {
					fprintf(stderr, "sleep: %.3f\n", query_delay);
					usleep( (int) (query_delay * 1000 * 1000) );
				}
			}
			fflush(stderr);
			fflush(stdout);
			exit(rc);
		}
	}

	if (usepw && ! got_rfbauth && ! got_passwd && ! got_passwdfile && !unixpw) {
		char *f, *h = getenv("HOME");
		struct stat sbuf;
		int found = 0, set_rfbauth = 0;

		if (!h) {
			rfbLog("HOME unset in -usepw mode.\n");
			exit(1);
		}
		f = (char *) malloc(strlen(h)+strlen("/.vnc/passwdfile") + 1);

		sprintf(f, "%s/.vnc/passwd", h);
		if (stat(f, &sbuf) == 0) {
			found = 1;
			if (! quiet) {
				rfbLog("-usepw: found %s\n", f);
			}
			got_rfbauth = 1;
			set_rfbauth = 1;
		}

		sprintf(f, "%s/.vnc/passwdfile", h);
		if (! found && stat(f, &sbuf) == 0) {
			found = 1;
			if (! quiet) {
				rfbLog("-usepw: found %s\n", f);
			}
			got_passwdfile = 1;
			passwdfile = strdup(f);
		}

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H
#if LIBVNCSERVER_HAVE_WAITPID
		if (! found) {
			pid_t pid = fork();
			if (pid < 0) {
				;
			} else if (pid == 0) {
				execlp(argv[0], argv[0], "-storepasswd",
				    (char *) NULL);
				exit(1);
			} else {
				int s;
				waitpid(pid, &s, 0); 
				if (WIFEXITED(s) && WEXITSTATUS(s) == 0) {
					got_rfbauth = 1;
					set_rfbauth = 1;
					found = 1;
				}
			}
		}
#endif
#endif
#endif

		if (set_rfbauth) {
			sprintf(f, "%s/.vnc/passwd", h);
			if (argc_vnc < 100) {
				argv_vnc[argc_vnc++] = strdup("-rfbauth");
			} else {
				exit(1);
			}
			if (argc_vnc < 100) {
				argv_vnc[argc_vnc++] = strdup(f);
			} else {
				exit(1);
			}
		}
		if (! found) {
			fprintf(stderr, "x11vnc -usepw: could not find"
			    " a password to use.\n");
			exit(1);
		}
		free(f);
	}

	if (got_rfbauth && (got_passwd || got_viewpasswd || got_passwdfile)) {
		fprintf(stderr, "option -rfbauth is incompatible with:\n");
		fprintf(stderr, "            -passwd, -viewpasswd, and -passwdfile\n");
		exit(1);
	}
	if (got_passwdfile && (got_passwd || got_viewpasswd)) {
		fprintf(stderr, "option -passwdfile is incompatible with:\n");
		fprintf(stderr, "            -passwd and -viewpasswd\n");
		exit(1);
	}

	/*
	 * If -passwd was used, clear it out of argv.  This does not
	 * work on all UNIX, have to use execvp() in general...
	 */
	if (pw_loc > 0) {
		int i;
		for (i=pw_loc; i <= pw_loc+1; i++) {
			if (i < argc) {
				char *p = argv[i];		
				strzero(p);
			}
		}
	} else if (passwdfile) {
		/* read passwd(s) from file */
		if (strstr(passwdfile, "cmd:") == passwdfile ||
		    strstr(passwdfile, "custom:") == passwdfile) {
			char tstr[100], *q;
			sprintf(tstr, "%f", dnow());
			if ((q = strrchr(tstr, '.')) == NULL) {
				q = tstr;
			} else {
				q++;
			}
			/* never used under cmd:, used to force auth */
			argv_vnc[argc_vnc++] = strdup("-passwd");
			argv_vnc[argc_vnc++] = strdup(q);
		} else if (read_passwds(passwdfile)) {
			argv_vnc[argc_vnc++] = strdup("-passwd");
			argv_vnc[argc_vnc++] = strdup(passwd_list[0]);
		}
		got_passwd = 1;
		pw_loc = 100;	/* just for pw_loc check below */
	}
	if (vpw_loc > 0) {
		int i;
		for (i=vpw_loc; i <= vpw_loc+1; i++) {
			if (i < argc) {
				char *p = argv[i];		
				strzero(p);
			}
		}
	}
#ifdef HARDWIRE_PASSWD
	if (!got_rfbauth && !got_passwd) {
		argv_vnc[argc_vnc++] = strdup("-passwd");
		argv_vnc[argc_vnc++] = strdup(HARDWIRE_PASSWD);
		got_passwd = 1;
		pw_loc = 100;
	}
#endif
#ifdef HARDWIRE_VIEWPASSWD
	if (!got_rfbauth && got_passwd && !viewonly_passwd && !passwd_list) {
		viewonly_passwd = strdup(HARDWIRE_VIEWPASSWD);
	}
#endif
	if (viewonly_passwd && pw_loc < 0) {
		rfbLog("-passwd must be supplied when using -viewpasswd\n");
		exit(1);
	}
	if (1) {
		/* mix things up a little bit */
		unsigned char buf[CHALLENGESIZE];
		int k, kmax = (int) (50 * rfac()) + 10;
		for (k=0; k < kmax; k++) {
			rfbRandomBytes(buf);
		}
	}

	if (SHOW_NO_PASSWORD_WARNING) {
		char message[] = "-rfbauth, -passwdfile, -passwd password, "
		    "or -unixpw required.";
		if (! nopw) {
			nopassword_warning_msg(got_localhost);
		}
#if PASSWD_REQUIRED
		rfbLog("%s\n", message);
		exit(1);
#endif
#if PASSWD_UNLESS_NOPW
		if (! nopw) {
			rfbLog("%s\n", message);
			exit(1);
		}
#endif
		message[0] = '\0';	/* avoid compiler warning */
	}

	if (more_safe) {
		if (! quiet) {
			rfbLog("-safer mode:\n");
			rfbLog("   vnc_connect=0\n");
			rfbLog("   accept_remote_cmds=0\n");
			rfbLog("   safe_remote_only=1\n");
			rfbLog("   launch_gui=0\n");
		}
		vnc_connect = 0;
		accept_remote_cmds = 0;
		safe_remote_only = 1;
		launch_gui = 0;
	}

	if (users_list && strchr(users_list, '.')) {
		char *p, *q, *tmp = (char *) malloc(strlen(users_list)+1);
		char *str = strdup(users_list);
		int i, n, db = 1;

		tmp[0] = '\0';

		n = strlen(users_list) + 1;
		user2group = (char **) malloc(n * sizeof(char *));
		for (i=0; i<n; i++) {
			user2group[i] = NULL;
		}

		i = 0;
		p = strtok(str, ",");
		if (db) fprintf(stderr, "users_list: %s\n", users_list);
		while (p) {
			if (tmp[0] != '\0') {
				strcat(tmp, ",");
			}
			q = strchr(p, '.');
			if (q) {
				char *s = strchr(p, '=');
				if (! s) {
					s = p;
				} else {
					s++;
				}
				if (s[0] == '+') s++;
				user2group[i++] = strdup(s);
				if (db) fprintf(stderr, "u2g: %s\n", s);
				*q = '\0';
			}
			strcat(tmp, p);
			p = strtok(NULL, ",");
		}
		free(str);
		users_list = tmp;
		if (db) fprintf(stderr, "users_list: %s\n", users_list);
	}

	if (got_tls > 0 && !use_openssl) {
		rfbLog("SSL: Error: you did not supply the '-ssl ...' option even\n");
		rfbLog("SSL: though you supplied one of these related options:\n");
		rfbLog("SSL:   -sslonly, -sslverify, -sslCRL, -vencrypt, -anontls,\n");
		rfbLog("SSL:   -dhparams, -https, -http_ssl, or -httpsredir.\n");
		rfbLog("SSL: Restart with, for example, '-ssl SAVE' on the cmd line.\n");
		rfbLog("SSL: See the '-ssl' x11vnc -help description for more info.\n");
		if (!getenv("X11VNC_FORCE_NO_OPENSSL")) {
			exit(1);
		}
	}

	if (unixpw) {
		if (inetd) {
			use_stunnel = 0;
		}
		if (! use_stunnel && ! use_openssl) {
			if (getenv("UNIXPW_DISABLE_SSL")) {
				rfbLog("Skipping -ssl/-stunnel requirement"
				    " due to\n");
				rfbLog("UNIXPW_DISABLE_SSL setting.\n");

				if (!getenv("UNIXPW_DISABLE_LOCALHOST")) {
					if (!got_localhost) {
						rfbLog("Forcing -localhost mode.\n");
					}
					allow_list = strdup("127.0.0.1");
					got_localhost = 1;
				}
			} else if (have_ssh_env()) {
				char *s = getenv("SSH_CONNECTION");
				if (! s) s = getenv("SSH_CLIENT");
				if (! s) s = "SSH_CONNECTION";
				fprintf(stderr, "\n");
				rfbLog("Skipping -ssl/-stunnel constraint in"
				    " -unixpw mode,\n");
				rfbLog("assuming your SSH encryption"
				    " is:\n");
				rfbLog("   %s\n", s);

				if (!getenv("UNIXPW_DISABLE_LOCALHOST")) {
					if (!got_localhost) {
						rfbLog("Setting -localhost in SSH + -unixpw mode.\n");
					}
					allow_list = strdup("127.0.0.1");
					got_localhost = 1;
				}

				rfbLog("If you *actually* want SSL, restart"
				    " with -ssl on the cmdline\n");
				if (! nopw) {
					usleep(2000*1000);
				}
			} else {
				if (openssl_present()) {
					rfbLog("set -ssl in -unixpw mode.\n");
					use_openssl = 1;
				} else if (inetd) {
					rfbLog("could not set -ssl in -inetd"
					    " + -unixpw mode.\n");
					exit(1);
				} else {
					rfbLog("set -stunnel in -unixpw mode.\n");
					use_stunnel = 1;
				}
			}
			rfbLog("\n");
		}
		if (use_threads && !getenv("UNIXPW_THREADS")) {
			if (! quiet) {
				rfbLog("disabling -threads under -unixpw\n");
				rfbLog("\n");
			}
			use_threads = 0;
		}
	}
	if (use_stunnel && ! got_localhost) {
		if (! getenv("STUNNEL_DISABLE_LOCALHOST") &&
		    ! getenv("UNIXPW_DISABLE_LOCALHOST")) {
			if (! quiet) {
				rfbLog("Setting -localhost in -stunnel mode.\n");
			}
			allow_list = strdup("127.0.0.1");
			got_localhost = 1;
		}
	}
	if (ssl_verify && ! use_stunnel && ! use_openssl) {
		rfbLog("-sslverify must be used with -ssl or -stunnel\n");
		exit(1);
	}
	if (https_port_num >= 0 && ! use_openssl) {
		rfbLog("-https must be used with -ssl\n");
		exit(1);
	}

	if (use_threads && !got_noxrandr) {
		xrandr = 1;
		if (! quiet) {
			rfbLog("enabling -xrandr in -threads mode.\n");
		}
	}

	/* fixup settings that do not make sense */
		
	if (use_threads && nofb && cursor_pos_updates) {
		if (! quiet) {
			rfbLog("disabling -threads under -nofb -cursorpos\n");
		}
		use_threads = 0;
	}
	if (tile_fuzz < 1) {
		tile_fuzz = 1;
	}
	if (waitms < 0) {
		waitms = 0;
	}

	if (alpha_threshold < 0) {
		alpha_threshold = 0;
	}
	if (alpha_threshold > 256) {
		alpha_threshold = 256;
	}
	if (alpha_frac < 0.0) {
		alpha_frac = 0.0;
	}
	if (alpha_frac > 1.0) {
		alpha_frac = 1.0;
	}
	if (alpha_blend) {
		alpha_remove = 0;
	}

	if (cmap8to24 && overlay) {
		if (! quiet) {
			rfbLog("disabling -overlay in -8to24 mode.\n");
		}
		overlay = 0;
	}

	if (tightfilexfer && view_only) {
		if (! quiet) {
			rfbLog("setting -notightfilexfer in -viewonly mode.\n");
		}
		/* how to undo via -R? */
		tightfilexfer = 0;
	}

	if (inetd) {
		shared = 0;
		connect_once = 1;
		bg = 0;
		if (use_stunnel) {
			exit(1);
		}
	}

	http_try_it = try_http;

	if (flip_byte_order && using_shm && ! quiet) {
		rfbLog("warning: -flipbyte order only works with -noshm\n");
	}

	if (! wireframe_copyrect) {
		set_wirecopyrect_mode(NULL);
	}
	if (! scroll_copyrect) {
		set_scrollcopyrect_mode(NULL);
	}
	if (screen_fixup_str) {
		parse_fixscreen();
	}
	initialize_scroll_matches();
	initialize_scroll_term();
	initialize_max_keyrepeat();

	/* increase rfbwait if threaded */
	if (use_threads && ! got_rfbwait) {
		/* ??? lower this ??? */
		rfbMaxClientWait = 604800000;
	}

	/* no framebuffer (Win2VNC) mode */

	if (nofb) {
		/* disable things that do not make sense with no fb */
		set_nofb_params(0);

		if (! got_deferupdate && ! got_defer) {
			/* reduce defer time under -nofb */
			defer_update = defer_update_nofb;
		}
		if (got_pointer_mode < 0) {
			pointer_mode = POINTER_MODE_NOFB;
		}
	}

	if (ncache < 0) {
		ncache_beta_tester = 1;
		ncache_msg = 1;
		if (ncache == -1) {
			ncache = 0;
		}
		ncache = -ncache;
		if (try_http || got_httpdir) {
			/* JVM usually not set to handle all the memory */
			ncache = 0;
			ncache_msg = 0;
		}
		if (subwin) {
			ncache = 0;
			ncache_msg = 0;
		}
	}

	if (raw_fb_str) {
		set_raw_fb_params(0);
	}
	if (! got_deferupdate) {
		char tmp[40];
		sprintf(tmp, "%d", defer_update);
		argv_vnc[argc_vnc++] = strdup("-deferupdate");
		argv_vnc[argc_vnc++] = strdup(tmp);
	}

	if (debug_pointer || debug_keyboard) {
		if (!logfile) {
			if (bg || quiet) {
				rfbLog("disabling -bg/-q under -debug_pointer"
				    "/-debug_keyboard\n");
				bg = 0;
				quiet = 0;
			}
		}
	}

	/* initialize added_keysyms[] array to zeros */
	add_keysym(NoSymbol);

	/* tie together cases of -localhost vs. -listen localhost */
	if (! listen_str) {
		if (allow_list && !strcmp(allow_list, "127.0.0.1")) {
			listen_str = strdup("localhost");
			argv_vnc[argc_vnc++] = strdup("-listen");
			argv_vnc[argc_vnc++] = strdup(listen_str);
		}
	} else if (!strcmp(listen_str, "localhost") ||
	    !strcmp(listen_str, "127.0.0.1")) {
		allow_list = strdup("127.0.0.1");
	}

	initialize_crash_handler();

	if (! quiet) {
		if (verbose) {
			print_settings(try_http, bg, gui_str);
		}
		rfbLog("x11vnc version: %s  pid: %d\n", lastmod, getpid());
	} else {
		rfbLogEnable(0);
	}

	X_INIT;
	SCR_INIT;
	CLIENT_INIT;
	INPUT_INIT;
	POINTER_INIT;
	
	/* open the X display: */

#if LIBVNCSERVER_HAVE_XKEYBOARD
	/*
	 * Disable XKEYBOARD before calling XOpenDisplay()
	 * this should be used if there is ambiguity in the keymapping. 
	 */
	if (xkbcompat) {
		Bool rc = XkbIgnoreExtension(True);
		if (! quiet) {
			rfbLog("Disabling xkb XKEYBOARD extension. rc=%d\n",
			    rc);
		}
		if (watch_bell) {
			watch_bell = 0;
			if (! quiet) rfbLog("Disabling bell.\n");
		}
	}
#else
	watch_bell = 0;
	use_xkb_modtweak = 0;
#endif

#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
	if (tightfilexfer) {
		rfbLog("rfbRegisterTightVNCFileTransferExtension: 6\n");
		rfbRegisterTightVNCFileTransferExtension();
	} else {
		if (0) rfbLog("rfbUnregisterTightVNCFileTransferExtension: 3\n");
		rfbUnregisterTightVNCFileTransferExtension();
	}
#endif

	initialize_allowed_input();

	if (display_N && !got_rfbport) {
		char *ud = use_dpy;
		if (ud == NULL) {
			ud = getenv("DISPLAY");
		}
		if (ud && strstr(ud, "cmd=") == NULL) {
			char *p;
			ud = strdup(ud);
			p = strrchr(ud, ':');
			if (p) {
				int N;	
				char *q = strchr(p, '.');
				if (q) {
					*q = '\0';
				}
				N = atoi(p+1);	
				if (argc_vnc+1 < argc_vnc_max) {
					char Nstr[16];
					sprintf(Nstr, "%d", (5900 + N) % 65536); 
					argv_vnc[argc_vnc++] = strdup("-rfbport");
					argv_vnc[argc_vnc++] = strdup(Nstr);
					got_rfbport = 1;
				}
			}
			free(ud);
		}
	}

	if (users_list && strstr(users_list, "lurk=")) {
		if (use_dpy) {
			rfbLog("warning: -display does not make sense in "
			    "\"lurk=\" mode...\n");
		}
		if (auth_file != NULL && strcmp(auth_file, "guess")) {
			set_env("XAUTHORITY", auth_file);
		}
		lurk_loop(users_list);

	} else if (use_dpy && strstr(use_dpy, "WAIT:") == use_dpy) {
		char *mcm = multiple_cursors_mode;

		if (strstr(use_dpy, "Xdummy")) {
			if (!xrandr && !got_noxrandr) {
				if (! quiet) {
					rfbLog("Enabling -xrandr for possible use of Xdummy server.\n");
				}
				xrandr = 1;
			}
		}

		waited_for_client = wait_for_client(&argc_vnc, argv_vnc,
		    try_http && ! got_httpdir);

		if (!mcm && multiple_cursors_mode) {
			free(multiple_cursors_mode);
			multiple_cursors_mode = NULL;
		}
	}


	if (auth_file) {
		check_guess_auth_file();
		if (auth_file != NULL) {
			set_env("XAUTHORITY", auth_file);
		}
	}

#ifdef MACOSX
	if (use_dpy && !strcmp(use_dpy, "console")) {
		;
	} else
#endif
	if (use_dpy && strcmp(use_dpy, "")) {
		dpy = XOpenDisplay_wr(use_dpy);
#ifdef MACOSX
	} else if (!subwin && getenv("DISPLAY")
	    && strstr(getenv("DISPLAY"), "/tmp/") ) {
		/* e.g. /tmp/launch-XlspvM/:0 on leopard */
		rfbLog("MacOSX: Ignoring $DISPLAY '%s'\n", getenv("DISPLAY"));
		rfbLog("MacOSX: Use -display $DISPLAY to force it.\n");
#endif
	} else if (raw_fb_str != NULL && raw_fb_str[0] != '+' && !got_noviewonly) {
		rfbLog("Not opening DISPLAY in -rawfb mode (force via -rawfb +str)\n");
		dpy = NULL; /* don't open it. */
	} else if ( (use_dpy = getenv("DISPLAY")) ) {
		if (strstr(use_dpy, "localhost") == use_dpy) {
			rfbLog("\n");
			rfbLog("WARNING: DISPLAY starts with localhost: '%s'\n", use_dpy);
			rfbLog("WARNING: Is this an SSH X11 port forwarding?  You most\n");
			rfbLog("WARNING: likely don't want x11vnc to use that DISPLAY.\n");
			rfbLog("WARNING: You probably should supply something\n");
			rfbLog("WARNING: like: -display :0  to access the physical\n");
			rfbLog("WARNING: X display on the machine where x11vnc is running.\n");
			rfbLog("\n");
			usleep(500 * 1000);
		} else if (using_shm && use_dpy[0] != ':') {
			rfbLog("\n");
			rfbLog("WARNING: DISPLAY might not be local: '%s'\n", use_dpy);
			rfbLog("WARNING: Is this the DISPLAY of another machine?  Usually,\n");
			rfbLog("WARNING: x11vnc is run on the same machine with the\n");
			rfbLog("WARNING: physical X display to be exported by VNC.  If\n");
			rfbLog("WARNING: that is what you really meant, supply something\n");
			rfbLog("WARNING: like: -display :0  on the x11vnc command line.\n");
			rfbLog("\n");
			usleep(250 * 1000);
		}
		dpy = XOpenDisplay_wr(use_dpy);
	} else {
		dpy = XOpenDisplay_wr("");
	}
	last_open_xdisplay = time(NULL);

	if (terminal_services_daemon != NULL) {
		terminal_services(terminal_services_daemon);
		exit(0);
	}

	if (dpy && !xrandr && !got_noxrandr) {
#if !NO_X11
		Atom trap_xrandr = XInternAtom(dpy, "X11VNC_TRAP_XRANDR", True);
		if (trap_xrandr != None) {
			if (! quiet) {
				rfbLog("Enabling -xrandr due to X11VNC_TRAP_XRANDR atom.\n");
			}
			xrandr = 1;
		}
#endif
	}

#ifdef MACOSX
	if (! dpy && ! raw_fb_str) {
		raw_fb_str = strdup("console");
	}
#endif

	if (! dpy && raw_fb_str) {
		rfbLog("Continuing without X display in -rawfb mode.\n");
		goto raw_fb_pass_go_and_collect_200_dollars;
	}

	if (! dpy && ! use_dpy && ! getenv("DISPLAY")) {
		int i, s = 4;
		rfbLogEnable(1);
		rfbLog("\a\n");
		rfbLog("*** XOpenDisplay failed. No -display or DISPLAY.\n");
		rfbLog("*** Trying \":0\" in %d seconds.  Press Ctrl-C to"
		    " abort.\n", s);
		rfbLog("*** ");
		for (i=1; i<=s; i++)  {
			fprintf(stderr, "%d ", i);
			sleep(1);
		}
		fprintf(stderr, "\n");
		use_dpy = ":0";
		dpy = XOpenDisplay_wr(use_dpy);
		last_open_xdisplay = time(NULL);
		if (dpy) {
			rfbLog("*** XOpenDisplay of \":0\" successful.\n");
		}
		rfbLog("\n");
		if (quiet) rfbLogEnable(0);
	}

	if (! dpy) {
		char *d = use_dpy;
		if (!d) d = getenv("DISPLAY");
		if (!d) d = "null";
		rfbLogEnable(1);
		fprintf(stderr, "\n");
		rfbLog("***************************************\n", d);
		rfbLog("*** XOpenDisplay failed (%s)\n", d);
		xopen_display_fail_message(d);
		exit(1);
	} else if (use_dpy) {
		if (! quiet) rfbLog("Using X display %s\n", use_dpy);
	} else {
		if (! quiet) rfbLog("Using default X display.\n");
	}

	if (clip_str != NULL && dpy != NULL) {
		check_xinerama_clip();
	}

	scr = DefaultScreen(dpy);
	rootwin = RootWindow(dpy, scr);

#if !NO_X11
	if (dpy) {
		Window w = XCreateSimpleWindow(dpy, rootwin, 0, 0, 1, 1, 0, 0, 0);
		if (! quiet) rfbLog("rootwin: 0x%lx reswin: 0x%lx dpy: 0x%x\n", rootwin, w, dpy);
		if (w != None) {
			XDestroyWindow(dpy, w);
		}
		XSync(dpy, False);
	}
#endif

	if (ncache_beta_tester) {
		int h = DisplayHeight(dpy, scr);
		int w = DisplayWidth(dpy, scr);
		int mem = (w * h * 4) / (1000 * 1000), MEM = 96;
		if (mem < 1) mem = 1;

		/* limit poor, unsuspecting beta tester's viewer to 96 MB */
		if ( (ncache+2) * mem > MEM ) {
			int n = (MEM/mem) - 2;
			if (n < 0) n = 0;
			n = 2 * (n / 2);
			if (n < ncache) {
				ncache = n;
			}
		}
	}

	if (grab_always) {
		Window save = window;
		window = rootwin;
		adjust_grabs(1, 0);
		window = save;
	}

	if (   (remote_cmd && strstr(remote_cmd, "DIRECT:") == remote_cmd)
	    || (query_cmd  && strstr(query_cmd,  "DIRECT:") == query_cmd )) {
		/* handled below after most everything is setup. */
		if (getenv("QUERY_VERBOSE")) {
			quiet = 0;
		} else {
			quiet = 1;
			remote_direct = 1;
		}
		if (!auto_port) {
			auto_port = 5970;
		}
	} else if (remote_cmd || query_cmd) {
		int i, rc = 1;
		for (i=0; i <= query_retries; i++) {
			rc = do_remote_query(remote_cmd, query_cmd, remote_sync,
			    query_default);
			if (rc == 0) {
				if (query_match) {
					if (query_result && strstr(query_result, query_match)) {
						break;
					}
					rc = 1;
				} else {
					break;
				}
			}
			if (i < query_retries) {
				fprintf(stderr, "sleep: %.3f\n", query_delay);
				usleep( (int) (query_delay * 1000 * 1000) );
			}
		}
		XFlush_wr(dpy);
		fflush(stderr);
		fflush(stdout);
		usleep(30 * 1000);	/* still needed? */
		XCloseDisplay_wr(dpy);
		exit(rc);
	}

	if (! quiet && ! raw_fb_str) {
		rfbLog("\n");
		rfbLog("------------------ USEFUL INFORMATION ------------------\n");
	}

	if (priv_remote) {
		if (! remote_control_access_ok()) {
			rfbLog("** Disabling remote commands in -privremote mode.\n");
			accept_remote_cmds = 0;
		}
	}

	sync_tod_with_servertime();
	if (grab_buster) {
		spawn_grab_buster();
	}

#if LIBVNCSERVER_HAVE_LIBXFIXES
	if (! XFixesQueryExtension(dpy, &xfixes_base_event_type, &er)) {
		if (! quiet && ! raw_fb_str) {
			rfbLog("Disabling XFIXES mode: display does not support it.\n");
		}
		xfixes_base_event_type = 0;
		xfixes_present = 0;
	} else {
		xfixes_present = 1;
	}
#endif
	if (! xfixes_present) {
		use_xfixes = 0;
	}

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	if (! XDamageQueryExtension(dpy, &xdamage_base_event_type, &er)) {
		if (! quiet && ! raw_fb_str) {
			rfbLog("Disabling X DAMAGE mode: display does not support it.\n");
		}
		xdamage_base_event_type = 0;
		xdamage_present = 0;
	} else {
		xdamage_present = 1;
	}
#endif
	if (! xdamage_present) {
		use_xdamage = 0;
	}
	if (! quiet && xdamage_present && use_xdamage && ! raw_fb_str) {
		rfbLog("X DAMAGE available on display, using it for polling hints.\n");
		rfbLog("  To disable this behavior use: '-noxdamage'\n");
		rfbLog("\n");
		rfbLog("  Most compositing window managers like 'compiz' or 'beryl'\n");
		rfbLog("  cause X DAMAGE to fail, and so you may not see any screen\n");
		rfbLog("  updates via VNC.  Either disable 'compiz' (recommended) or\n");
		rfbLog("  supply the x11vnc '-noxdamage' command line option.\n");
	}

	if (! quiet && wireframe && ! raw_fb_str) {
		rfbLog("\n");
		rfbLog("Wireframing: -wireframe mode is in effect for window moves.\n");
		rfbLog("  If this yields undesired behavior (poor response, painting\n");
		rfbLog("  errors, etc) it may be disabled:\n");
		rfbLog("   - use '-nowf' to disable wireframing completely.\n");
		rfbLog("   - use '-nowcr' to disable the Copy Rectangle after the\n");
		rfbLog("     moved window is released in the new position.\n");
		rfbLog("  Also see the -help entry for tuning parameters.\n");
		rfbLog("  You can press 3 Alt_L's (Left \"Alt\" key) in a row to \n");
		rfbLog("  repaint the screen, also see the -fixscreen option for\n");
		rfbLog("  periodic repaints.\n");
		if (scale_str && !strstr(scale_str, "nocr")) {
			rfbLog("  Note: '-scale' is on and this can cause more problems.\n");
		}
	}

	overlay_present = 0;
#if defined(SOLARIS_OVERLAY) && !NO_X11
	if (! XQueryExtension(dpy, "SUN_OVL", &maj, &ev, &er)) {
		if (! quiet && overlay && ! raw_fb_str) {
			rfbLog("Disabling -overlay: SUN_OVL extension not available.\n");
		}
	} else {
		overlay_present = 1;
	}
#endif
#if defined(IRIX_OVERLAY) && !NO_X11
	if (! XReadDisplayQueryExtension(dpy, &ev, &er)) {
		if (! quiet && overlay && ! raw_fb_str) {
			rfbLog("Disabling -overlay: IRIX ReadDisplay extension not available.\n");
		}
	} else {
		overlay_present = 1;
	}
#endif
	if (overlay && !overlay_present) {
		overlay = 0;
		overlay_cursor = 0;
	}

	/* cursor shapes setup */
	if (! multiple_cursors_mode) {
		multiple_cursors_mode = strdup("default");
	}
	if (show_cursor) {
		if(!strcmp(multiple_cursors_mode, "default")
		    && xfixes_present && use_xfixes) {
			free(multiple_cursors_mode);
			multiple_cursors_mode = strdup("most");

			if (! quiet && ! raw_fb_str) {
				rfbLog("\n");
				rfbLog("XFIXES available on display, resetting cursor mode\n");
				rfbLog("  to: '-cursor most'.\n");
				rfbLog("  to disable this behavior use: '-cursor arrow'\n");
				rfbLog("  or '-noxfixes'.\n");
			}
		}
		if(!strcmp(multiple_cursors_mode, "most")) {
			if (xfixes_present && use_xfixes &&
			    overlay_cursor == 1) {
				if (! quiet && ! raw_fb_str) {
					rfbLog("using XFIXES for cursor drawing.\n");
				}
				overlay_cursor = 0;
			}
		}
	}

	if (overlay) {
		using_shm = 0;
		if (flash_cmap && ! quiet && ! raw_fb_str) {
			rfbLog("warning: -flashcmap may be incompatible with -overlay\n");
		}
		if (show_cursor && overlay_cursor) {
			char *s = multiple_cursors_mode;
			if (*s == 'X' || !strcmp(s, "some") ||
			    !strcmp(s, "arrow")) {
				/*
				 * user wants these modes, so disable fb cursor
				 */
				overlay_cursor = 0;
			} else {
				/*
				 * "default" and "most", we turn off
				 * show_cursor since it will automatically
				 * be in the framebuffer.
				 */
				show_cursor = 0;
			}
		}
	}

	initialize_cursors_mode();

	/* check for XTEST */
	if (! XTestQueryExtension_wr(dpy, &ev, &er, &maj, &min)) {
		if (! quiet && ! raw_fb_str) {
			rfbLog("\n");
			rfbLog("WARNING: XTEST extension not available (either missing from\n");
			rfbLog("  display or client library libXtst missing at build time).\n");
			rfbLog("  MOST user input (pointer and keyboard) will be DISCARDED.\n");
			rfbLog("  If display does have XTEST, be sure to build x11vnc with\n");
			rfbLog("  a working libXtst build environment (e.g. libxtst-dev,\n");
			rfbLog("  or other packages).\n");
			rfbLog("No XTEST extension, switching to -xwarppointer mode for\n");
			rfbLog("  pointer motion input.\n");
		}
		xtest_present = 0;
		use_xwarppointer = 1;
	} else {
		xtest_present = 1;
		xtest_base_event_type = ev;
		if (maj <= 1 || (maj == 2 && min <= 2)) {
			/* no events defined as of 2.2 */
			xtest_base_event_type = 0;
		}
	}

	if (! XETrapQueryExtension_wr(dpy, &ev, &er, &maj)) {
		xtrap_present = 0;
	} else {
		xtrap_present = 1;
		xtrap_base_event_type = ev;
	}

	/*
	 * Window managers will often grab the display during resize,
	 * etc, using XGrabServer().  To avoid deadlock (our user resize
	 * input is not processed) we tell the server to process our
	 * requests during all grabs:
	 */
	disable_grabserver(dpy, 0);

	/* check for RECORD */
	if (! XRecordQueryVersion_wr(dpy, &maj, &min)) {
		xrecord_present = 0;
		if (! quiet) {
			rfbLog("\n");
			rfbLog("The RECORD X extension was not found on the display.\n");
			rfbLog("If your system has disabled it by default, you can\n");
			rfbLog("enable it to get a nice x11vnc performance speedup\n");
			rfbLog("for scrolling by putting this into the \"Module\" section\n");
			rfbLog("of /etc/X11/xorg.conf or /etc/X11/XF86Config:\n");
			rfbLog("\n");
			rfbLog("  Section \"Module\"\n");
			rfbLog("  ...\n");
			rfbLog("      Load    \"record\"\n");
			rfbLog("  ...\n");
			rfbLog("  EndSection\n");
			rfbLog("\n");
		}
	} else {
		xrecord_present = 1;
	}

	initialize_xrecord();

	tmpi = 1;
	if (scroll_copyrect) {
		if (strstr(scroll_copyrect, "never")) {
			tmpi = 0;
		}
	} else if (scroll_copyrect_default) {
		if (strstr(scroll_copyrect_default, "never")) {
			tmpi = 0;
		}
	}
	if (! xrecord_present) {
		tmpi = 0;
	}
#if !LIBVNCSERVER_HAVE_RECORD
	tmpi = 0;
#endif
	if (! quiet && tmpi && ! raw_fb_str) {
		rfbLog("\n");
		rfbLog("Scroll Detection: -scrollcopyrect mode is in effect to\n");
		rfbLog("  use RECORD extension to try to detect scrolling windows\n");
		rfbLog("  (induced by either user keystroke or mouse input).\n");
		rfbLog("  If this yields undesired behavior (poor response, painting\n");
		rfbLog("  errors, etc) it may be disabled via: '-noscr'\n");
		rfbLog("  Also see the -help entry for tuning parameters.\n");
		rfbLog("  You can press 3 Alt_L's (Left \"Alt\" key) in a row to \n");
		rfbLog("  repaint the screen, also see the -fixscreen option for\n");
		rfbLog("  periodic repaints.\n");
		if (scale_str && !strstr(scale_str, "nocr")) {
			rfbLog("  Note: '-scale' is on and this can cause more problems.\n");
		}
	}

	if (! quiet && ncache && ! raw_fb_str) {
		rfbLog("\n");
		rfbLog("Client Side Caching: -ncache mode is in effect to provide\n");
		rfbLog("  client-side pixel data caching.  This speeds up\n");
		rfbLog("  iconifying/deiconifying windows, moving and raising\n");
		rfbLog("  windows, and reposting menus.  In the simple CopyRect\n");
		rfbLog("  encoding scheme used (no compression) a huge amount\n");
		rfbLog("  of extra memory (20-100MB) is used on both the server and\n");
		rfbLog("  client sides.  This mode works with any VNC viewer.\n");
		rfbLog("  However, in most you can actually see the cached pixel\n");
		rfbLog("  data by scrolling down, so you need to re-adjust its size.\n");
		rfbLog("  See http://www.karlrunge.com/x11vnc/faq.html#faq-client-caching.\n");
		rfbLog("  If this mode yields undesired behavior (poor response,\n");
		rfbLog("  painting errors, etc) it may be disabled via: '-ncache 0'\n");
		rfbLog("  You can press 3 Alt_L's (Left \"Alt\" key) in a row to \n");
		rfbLog("  repaint the screen, also see the -fixscreen option for\n");
		rfbLog("  periodic repaints.\n");
		if (scale_str) {
			rfbLog("  Note: '-scale' is on and this can cause more problems.\n");
		}
	}
	if (ncache && getenv("NCACHE_DEBUG")) {
		ncdb = 1;
	}

	/* check for OS with small shm limits */
	if (using_shm && ! single_copytile) {
		if (limit_shm()) {
			single_copytile = 1;
		}
	}

	single_copytile_orig = single_copytile;

	/* check for MIT-SHM */
	if (! XShmQueryExtension_wr(dpy)) {
		xshm_present = 0;
		if (! using_shm) {
			if (! quiet && ! raw_fb_str) {
				rfbLog("info: display does not support XShm.\n");
			}
		} else {
		    if (! quiet && ! raw_fb_str) {
			rfbLog("\n");
			rfbLog("warning: XShm extension is not available.\n");
			rfbLog("For best performance the X Display should be local. (i.e.\n");
			rfbLog("the x11vnc and X server processes should be running on\n");
			rfbLog("the same machine.)\n");
#if LIBVNCSERVER_HAVE_XSHM
			rfbLog("Restart with -noshm to override this.\n");
		    }
		    exit(1);
#else
			rfbLog("Switching to -noshm mode.\n");
		    }
		    using_shm = 0;
#endif
		}
	} else {
#if !NO_X11
		int op, ev, er;
		if (XQueryExtension(dpy, "MIT-SHM", &op, &ev, &er)) {
			xshm_opcode = op;
			if (0) fprintf(stderr, "xshm_opcode: %d %d %d\n", op, ev, er);
		}
#endif
	}

#if LIBVNCSERVER_HAVE_XKEYBOARD
	/* check for XKEYBOARD */
	initialize_xkb();
	initialize_watch_bell();
	if (!xkb_present && use_xkb_modtweak) {
		if (! quiet && ! raw_fb_str) {
			rfbLog("warning: disabling xkb modtweak. XKEYBOARD ext. not present.\n");
		}
		use_xkb_modtweak = 0;
	}
#endif

	if (xkb_present && !use_xkb_modtweak && !got_noxkb) {
		if (use_modifier_tweak) {
			switch_to_xkb_if_better();
		}
	}

#if LIBVNCSERVER_HAVE_LIBXRANDR
	if (! XRRQueryExtension(dpy, &xrandr_base_event_type, &er)) {
		if (xrandr && ! quiet && ! raw_fb_str) {
			rfbLog("Disabling -xrandr mode: display does not support X RANDR.\n");
		}
		xrandr_base_event_type = 0;
		xrandr = 0;
		xrandr_maybe = 0;
		xrandr_present = 0;
	} else {
		xrandr_present = 1;
	}
#endif

	check_pm();

	if (! quiet && ! raw_fb_str) {
		rfbLog("--------------------------------------------------------\n");
		rfbLog("\n");
	}

	raw_fb_pass_go_and_collect_200_dollars:

	if (! dpy || raw_fb_str) {
		int doit = 0;
		/* XXX this needs improvement (esp. for remote control) */
		if (! raw_fb_str || strstr(raw_fb_str, "console") == raw_fb_str) {
#ifdef MACOSX
			doit = 1;
#endif
		}
		if (raw_fb_str && strstr(raw_fb_str, "vnc") == raw_fb_str) {
			doit = 1;
		}
		if (doit) {
			if (! multiple_cursors_mode) {
				multiple_cursors_mode = strdup("most");
			}
			initialize_cursors_mode();
			use_xdamage = orig_use_xdamage;
			if (use_xdamage) {
				xdamage_present = 1;
				initialize_xdamage();
			}
		}
	}

	if (! dt) {
		static char str[] = "-desktop";
		argv_vnc[argc_vnc++] = str;
		argv_vnc[argc_vnc++] = choose_title(use_dpy);
		rfb_desktop_name = strdup(argv_vnc[argc_vnc-1]);
	}
	
	/*
	 * Create the XImage corresponding to the display framebuffer.
	 */

	fb0 = initialize_xdisplay_fb();

	/*
	 * In some cases (UINPUT touchscreens) we need the dpy_x dpy_y
	 * to initialize pipeinput. So we do it after fb is created.
	 */
	initialize_pipeinput();

	/*
	 * n.b. we do not have to X_LOCK any X11 calls until watch_loop()
	 * is called since we are single-threaded until then.
	 */

	initialize_screen(&argc_vnc, argv_vnc, fb0);

	if (waited_for_client) {
		if (fake_fb) {
			free(fake_fb);
			fake_fb = NULL;
		}
		if (use_solid_bg && client_count) {
			solid_bg(0);
		}
		if (accept_cmd && strstr(accept_cmd, "popup") == accept_cmd) {
			rfbClientIteratorPtr iter;
			rfbClientPtr cl, cl0 = NULL;
			int i = 0;
			iter = rfbGetClientIterator(screen);
			while( (cl = rfbClientIteratorNext(iter)) ) {
				i++;	
				if (i != 1) {
					rfbLog("WAIT popup: too many clients\n");
					clean_up_exit(1);
				}
				cl0 = cl;
			}
			rfbReleaseClientIterator(iter);
			if (i != 1 || cl0 == NULL) {
				rfbLog("WAIT popup: no clients.\n");
				clean_up_exit(1);
			}
			if (! accept_client(cl0)) {
				rfbLog("WAIT popup: denied.\n");
				clean_up_exit(1);
			}
			rfbLog("waited_for_client: popup accepted.\n");
			cl0->onHold = FALSE;
		}
		if (macosx_console) {
			refresh_screen(1);
		}
		if (dpy && xdmcp_insert != NULL) {
#if !NO_X11
			char c;
			int n = strlen(xdmcp_insert);
			KeyCode k, k2;
			KeySym sym;
			int i, ok = 1;
			for (i = 0; i < n; i++) {
				c = xdmcp_insert[i];
				sym = (KeySym) c;
				if (sym < ' ' || sym > 0x7f) {
					ok = 0;
					break;
				}
				k = XKeysymToKeycode(dpy, sym);
				if (k == NoSymbol) {
					ok = 0;
					break;
				}
			}
			if (ok) {
				XFlush_wr(dpy);
				usleep(2*1000*1000);
				if (!quiet) {
					rfbLog("sending XDM '%s'\n", xdmcp_insert);
				}
				for (i = 0; i < n; i++) {
					c = xdmcp_insert[i];
					sym = (KeySym) c;
					k = XKeysymToKeycode(dpy, sym);
					if (isupper(c)) {
						k2 = XKeysymToKeycode(dpy, XK_Shift_L);
						XTestFakeKeyEvent_wr(dpy, k2, True, CurrentTime);
						XFlush_wr(dpy);
						usleep(100*1000);
					}
					if (0) fprintf(stderr, "C/k %c/%x\n", c, k);
					XTestFakeKeyEvent_wr(dpy, k, True, CurrentTime);
					XFlush_wr(dpy);
					usleep(100*1000);
					XTestFakeKeyEvent_wr(dpy, k, False, CurrentTime);
					XFlush_wr(dpy);
					usleep(100*1000);
					if (isupper(c)) {
						k2 = XKeysymToKeycode(dpy, XK_Shift_L);
						XTestFakeKeyEvent_wr(dpy, k2, False, CurrentTime);
						XFlush_wr(dpy);
						usleep(100*1000);
					}
				}
				k2 = XKeysymToKeycode(dpy, XK_Tab);
				XTestFakeKeyEvent_wr(dpy, k2, True, CurrentTime);
				XFlush_wr(dpy);
				usleep(100*1000);
				XTestFakeKeyEvent_wr(dpy, k2, False, CurrentTime);
				XFlush_wr(dpy);
				usleep(100*1000);
			}
			free(xdmcp_insert);
#endif
		}
		check_redir_services();
	}

	if (! waited_for_client) {
		if (try_http && ! got_httpdir && check_httpdir()) {
			http_connections(1);
		}
		if (ssh_str != NULL) {
			ssh_remote_tunnel(ssh_str, screen->port);
		}
	}

	initialize_tiles();

	/* rectangular blackout regions */
	initialize_blackouts_and_xinerama();

	/* created shm or XImages when using_shm = 0 */
	initialize_polling_images();

	initialize_signals();

	initialize_speeds();

	if (speeds_read_rate_measured > 80) {
		/* framebuffer read is fast at > 80 MB/sec */
		int same = 0;
		if (waitms == defer_update) {
			same = 1;
		}
		if (! got_waitms) {
			waitms /= 2;
			if (waitms < 5) {
				waitms = 5;
			}
			if (!quiet) {
				rfbLog("fast read: reset -wait  ms to: %d\n", waitms);
			}
		}
		if (! got_deferupdate && ! got_defer) {
			if (defer_update > 10) {
				if (same) {
					defer_update = waitms;
				} else {
					defer_update = 10;
				}
				if (screen) {
					screen->deferUpdateTime = defer_update;
				}
				rfbLog("fast read: reset -defer ms to: %d\n", defer_update);
			}
		}
	}

	initialize_keyboard_and_pointer();

	if (inetd && use_openssl) {
		if (! waited_for_client) {
			accept_openssl(OPENSSL_INETD, -1);
		}
	}
	if (! inetd && ! use_openssl) {
		if (! screen->port || screen->listenSock < 0) {
			if (got_rfbport && got_rfbport_val == 0) {
				;
			} else if (ipv6_listen && ipv6_listen_fd >= 0) {
				rfbLog("Info: listening only on IPv6 interface.\n");
			} else {
				rfbLogEnable(1);
				rfbLog("Error: could not obtain listening port.\n");
				if (!got_rfbport && !got_ipv6_listen) {
					rfbLog("If this system is IPv6-only, use the -6 option.\n");
				}
				clean_up_exit(1);
			}
		}
	}

#ifdef MACOSX
	if (remote_cmd || query_cmd) {
		;
	} else if (macosx_console) {
		double dt = dnow();
		copy_screen();
		dt = dnow() - dt;
		rfbLog("macosx_console: copied screen in %.3f sec %.1f MB/sec\n",
		    dt, dpy_x * dpy_y * bpp / (1e+6 * 8 * dt));

	}
#endif

	if (! quiet) {
		rfbLog("screen setup finished.\n");
		if (SHOW_NO_PASSWORD_WARNING && !nopw) {
			rfbLog("\n");
			rfbLog("WARNING: You are running x11vnc WITHOUT"
			    " a password.  See\n");
			rfbLog("WARNING: the warning message printed above"
			    " for more info.\n");
		}
	}
	set_vnc_desktop_name();

	if (ncache_beta_tester && (ncache != 0 || ncache_msg)) {
		ncache_beta_tester_message();
	}

	if (remote_cmd || query_cmd) {
		/* This is DIRECT: case */
		do_remote_query(remote_cmd, query_cmd, remote_sync, query_default);
		if (getenv("SLEEP")) sleep(atoi(getenv("SLEEP")));
		clean_up_exit(0);
	}

#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SETSID
	if (bg) {
		int p, n;
		if (getenv("X11VNC_LOOP_MODE_BG")) {
			if (screen && screen->listenSock >= 0) {
				close(screen->listenSock);
				FD_CLR(screen->listenSock,&screen->allFds);
				screen->listenSock = -1;
			}
			if (screen && screen->httpListenSock >= 0) {
				close(screen->httpListenSock);
				screen->httpListenSock = -1;
			}
			if (openssl_sock >= 0) {
				close(openssl_sock);
				openssl_sock = -1;
			}
			if (https_sock >= 0) {
				close(https_sock);
				https_sock = -1;
			}
			if (openssl_sock6 >= 0) {
				close(openssl_sock6);
				openssl_sock6 = -1;
			}
			if (https_sock6 >= 0) {
				close(https_sock6);
				https_sock6 = -1;
			}
			if (ipv6_listen_fd >= 0) {
				close(ipv6_listen_fd);
				ipv6_listen_fd = -1;
			}
			if (ipv6_http_fd >= 0) {
				close(ipv6_http_fd);
				ipv6_http_fd = -1;
			}
		}
		/* fork into the background now */
		if ((p = fork()) > 0)  {
			exit(0);
		} else if (p == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "could not fork\n");
			perror("fork");
			clean_up_exit(1);
		}
		if (setsid() == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "setsid failed\n");
			perror("setsid");
			clean_up_exit(1);
		}
		/* adjust our stdio */
		n = open("/dev/null", O_RDONLY);
		dup2(n, 0);
		dup2(n, 1);
		if (! logfile) {
			dup2(n, 2);
		}
		if (n > 2) {
			close(n);
		}
	}
#endif

	watch_loop();

	return(0);

#undef argc
#undef argv

}

