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

/* -- gui.c -- */

#include "x11vnc.h"
#include "xevents.h"
#include "win_utils.h"
#include "remote.h"
#include "cleanup.h"
#include "xwrappers.h"
#include "connections.h"

#include "tkx11vnc.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
#define XEMBED_VERSION 0
#define XEMBED_MAPPED  (1 << 0)

int icon_mode = 0;		/* hack for -gui tray/icon */
char *icon_mode_file = NULL;
FILE *icon_mode_fh = NULL;
int icon_mode_socks[ICON_MODE_SOCKS];
int tray_manager_ok = 0;
Window tray_request = None;
Window tray_window = None;
int tray_unembed = 0;
pid_t run_gui_pid = 0;
pid_t gui_pid = 0;


char *get_gui_code(void);
int tray_embed(Window iconwin, int remove);
void do_gui(char *opts, int sleep);


static Window tweak_tk_window_id(Window win);
static int tray_manager_running(Display *d, Window *manager);
static void run_gui(char *gui_xdisplay, int connect_to_x11vnc, int start_x11vnc,
    int simple_gui, pid_t parent, char *gui_opts);


char *get_gui_code(void) {
	return gui_code;
}

static Window tweak_tk_window_id(Window win) {
#if NO_X11
	if (!win) {}
	return None;
#else
	char *name = NULL;
	Window parent, new_win;

	if (getenv("NO_TWEAK_TK_WINDOW_ID")) {
		return win;
	}

	/* hack for tk, does not report outermost window */
	new_win = win;
	parent = parent_window(win, &name);
	if (parent && name != NULL) {
		lowercase(name);
		if (strstr(name, "wish") || strstr(name, "x11vnc")) {
			new_win = parent;
			rfbLog("tray_embed: using parent: %s\n", name);
		}
	}
	if (name != NULL) {
		XFree_wr(name);
	}
	return new_win;
#endif	/* NO_X11 */
}

int tray_embed(Window iconwin, int remove) {
#if NO_X11
	RAWFB_RET(0)
	if (!iconwin || !remove) {}
	return 0;
#else
	XEvent ev;
	XErrorHandler old_handler;
	Window manager;
	Atom xembed_info;
	Atom tatom;
	XWindowAttributes attr;
	long info[2] = {XEMBED_VERSION, XEMBED_MAPPED};
	long data = 0;

	RAWFB_RET(0)

	if (remove) {
		if (!valid_window(iconwin, &attr, 1)) {
			return 0;
		}
		iconwin = tweak_tk_window_id(iconwin);
		trapped_xerror = 0;
		old_handler = XSetErrorHandler(trap_xerror);

		/*
		 * unfortunately no desktops seem to obey this
		 * part of the XEMBED spec yet...
		 */
		XReparentWindow(dpy, iconwin, rootwin, 0, 0);

		XSetErrorHandler(old_handler);
		if (trapped_xerror) {
			trapped_xerror = 0;
			return 0;
		}
		trapped_xerror = 0;
		return 1;
	}

	xembed_info = XInternAtom(dpy, "_XEMBED_INFO", False);
	if (xembed_info == None) {
		return 0;
	}

	if (!tray_manager_running(dpy, &manager)) {
		return 0;
	}

	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = manager;
	ev.xclient.message_type = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE",
	    False);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
	ev.xclient.data.l[2] = iconwin;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	if (!valid_window(iconwin, &attr, 1)) {
		return 0;
	}

	iconwin = tweak_tk_window_id(iconwin);
	ev.xclient.data.l[2] = iconwin;

	XUnmapWindow(dpy, iconwin);

	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);

	XSendEvent(dpy, manager, False, NoEventMask, &ev);
	XSync(dpy, False);

	if (trapped_xerror) {
		XSetErrorHandler(old_handler);
		trapped_xerror = 0;
		return 0;
	}

	XChangeProperty(dpy, iconwin, xembed_info, xembed_info, 32,
	    PropModeReplace, (unsigned char *)&info, 2);

#if 0
{
XSizeHints *xszh = XAllocSizeHints();
xszh->flags = PMinSize;
xszh->min_width = 24;
xszh->min_height = 24;
XSetWMNormalHints(dpy, iconwin, xszh);
}
#endif

	/* kludge for KDE evidently needed... */
	tatom = XInternAtom(dpy, "KWM_DOCKWINDOW", False);
	XChangeProperty(dpy, iconwin, tatom, tatom, 32, PropModeReplace,
	    (unsigned char *)&data, 1);
	tatom = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
	XChangeProperty(dpy, iconwin, tatom, XA_WINDOW, 32, PropModeReplace,
	    (unsigned char *)&data, 1);

	XSetErrorHandler(old_handler);
	trapped_xerror = 0;
	return 1;
#endif	/* NO_X11 */
}

static int tray_manager_running(Display *d, Window *manager) {
#if NO_X11
	RAWFB_RET(0)
	if (!d || !manager) {}
	return 0;
#else
	char tray_string[100];
	Atom tray_manager;
	Window tray_win;

	RAWFB_RET(0)

	if (manager) {
		*manager = None;
	}
	sprintf(tray_string, "_NET_SYSTEM_TRAY_S%d", scr);

	tray_manager = XInternAtom(d, tray_string, True);
	if (tray_manager == None) {
		return 0;
	}

	tray_win = XGetSelectionOwner(d, tray_manager);
	if (manager) {
		*manager = tray_win;
	}

	if (tray_win == None) {
		return 0;
	} else {
		return 1;
	}
#endif	/* NO_X11 */
}

static char *gui_geometry = NULL;
static int icon_in_tray = 0;
static char *icon_mode_embed_id = NULL;
static char *icon_mode_font = NULL;
static char *icon_mode_params = NULL;

static int got_sigusr1 = 0;

static void sigusr1 (int sig) {
	got_sigusr1 = 1;
	if (0) sig = 0;
}

/* Most of the following mess is for wish on Solaris: */

static char *extra_path = ":/usr/local/bin:/usr/bin/X11:/usr/sfw/bin"
	    ":/usr/X11R6/bin:/usr/openwin/bin:/usr/dt/bin:/opt/sfw/bin";
static char *wishes[] = {"wish8.4", "wish", "wish8.3", "wish8.5", "wish8.6", "wish8.7", "wishx", "wish8.0", NULL};

static void run_gui(char *gui_xdisplay, int connect_to_x11vnc, int start_x11vnc,
    int simple_gui, pid_t parent, char *gui_opts) {
	char *x11vnc_xdisplay = NULL;
	char cmd[100];
	char *wish = NULL, *orig_path, *full_path, *tpath, *p;
	char *old_xauth = NULL;
	int try_max = 4, sleep = 300, totms, rc = 0;
	pid_t mypid = getpid();
	FILE *pipe, *tmpf;

if (0) fprintf(stderr, "run_gui: %s -- %d %d\n", gui_xdisplay, connect_to_x11vnc, (int) parent);
	if (*gui_code == '\0') {
		rfbLog("gui: gui not compiled into this program.\n");
		exit(0);
	}
	if (getenv("DISPLAY") != NULL) {
		/* worst case */
		x11vnc_xdisplay = strdup(getenv("DISPLAY"));
	}
	if (use_dpy) {
		/* better */
		x11vnc_xdisplay = strdup(use_dpy);
	}
	if (connect_to_x11vnc) {
		int i;
		rfbLogEnable(1);
		if (! client_connect_file) {
			if (getenv("XAUTHORITY") != NULL) {
				old_xauth = strdup(getenv("XAUTHORITY"));
			} else {
				old_xauth = strdup("");
			}
			dpy = XOpenDisplay_wr(x11vnc_xdisplay); 
			if (! dpy && auth_file) {
				set_env("XAUTHORITY", auth_file);
				dpy = XOpenDisplay_wr(x11vnc_xdisplay);
			}
			if (! dpy && ! x11vnc_xdisplay) {
				/* worstest case */
				x11vnc_xdisplay = strdup(":0");
				dpy = XOpenDisplay_wr(x11vnc_xdisplay); 
			}
			if (! dpy) {
				rfbLog("gui: could not open x11vnc "
				    "display: %s\n", NONUL(x11vnc_xdisplay));
#ifdef MACOSX
				goto macjump;
#endif
				exit(1);
			}
			scr = DefaultScreen(dpy);
			rootwin = RootWindow(dpy, scr);
			initialize_vnc_connect_prop();
			initialize_x11vnc_remote_prop();
		}

#ifdef MACOSX
		macjump:
#endif
		
		signal(SIGUSR1, sigusr1);
		got_sigusr1 = 0;
		totms = 0;
		while (totms < 3500) {
			usleep(50*1000);
			totms += 50;
			if (got_sigusr1) {
				fprintf(stderr, "\n");
				if (! quiet) rfbLog("gui: got SIGUSR1\n");
				break;
			}
			if (! start_x11vnc && totms >= 150) {
				break;
			}
		}
		signal(SIGUSR1, SIG_DFL);
		if (! got_sigusr1) fprintf(stderr, "\n");

		if (!quiet && ! got_sigusr1) {
			rfbLog("gui: trying to contact a x11vnc server at X"
			    " display %s ...\n", NONUL(x11vnc_xdisplay));
		}

		for (i=0; i<try_max; i++) {
			if (! got_sigusr1) {
				if (!quiet) {
					rfbLog("gui: pinging %s try=%d ...\n",
					    NONUL(x11vnc_xdisplay), i+1);
				}
				rc = send_remote_cmd("qry=ping", 1, 1);
				if (rc == 0) {
					break;
				}
			} else {
				rc = 0;
				break;
			}
			if (parent && mypid != parent && kill(parent, 0) != 0) {
				rfbLog("gui: parent process %d has gone"
				    " away: bailing out.\n", parent);
				rc = 1;
				break;
			}
			usleep(sleep*1000);
		}
		set_env("X11VNC_XDISPLAY", x11vnc_xdisplay);
		if (getenv("XAUTHORITY") != NULL) {
			set_env("X11VNC_AUTH_FILE", getenv("XAUTHORITY"));
		}
		if (rc == 0) {
			rfbLog("gui: ping succeeded.\n");
			set_env("X11VNC_CONNECT", "1");
		} else {
			rfbLog("gui: could not connect to: '%s', try"
			    " again manually.\n", x11vnc_xdisplay);
		}
		if (client_connect_file) {
			set_env("X11VNC_CONNECT_FILE", client_connect_file);
		}
		if (dpy)  {
			XCloseDisplay_wr(dpy);
			dpy = NULL;
		}
		if (old_xauth) {
			if (*old_xauth == '\0') {
				/* wasn't set, hack it out if it is now */
				char *xauth = getenv("XAUTHORITY");
				if (xauth) {
					*(xauth-2) = '_';	/* yow */
				}
			} else {
				set_env("XAUTHORITY", old_xauth);
			}
			free(old_xauth);
		}
		rfbLogEnable(0);
	}

	orig_path = getenv("PATH");
	if (! orig_path) {
		orig_path = strdup("/bin:/usr/bin:/usr/bin/X11");
	}
	full_path = (char *) malloc(strlen(orig_path)+strlen(extra_path)+1);
	strcpy(full_path, orig_path);
	strcat(full_path, extra_path);

	tpath = strdup(full_path);
	p = strtok(tpath, ":");

	while (p) {
		char *try;
		struct stat sbuf;
		int i;

		try = (char *) malloc(strlen(p) + 1 + strlen("wish8.4") + 1);
		i = 0;
		while (wishes[i] != NULL) {
			sprintf(try, "%s/%s", p, wishes[i]);
			if (stat(try, &sbuf) == 0) {
				/* assume executable, should check mode */
				wish = wishes[i];
				break;
			}
			i++;
		}
		free(try);
		if (wish) {
			break;
		}
		p = strtok(NULL, ":");
	}
	free(tpath);
	if (!wish) {
		wish = strdup("wish");
	}
	if (getenv("WISH")) {
		char *w = getenv("WISH");
		if (strcmp(w, "")) {
			wish = strdup(w);
		}
	}
	if (getenv("DEBUG_WISH")) {
		fprintf(stderr, "wish: %s\n", wish);
	}
	set_env("PATH", full_path);
	set_env("DISPLAY", gui_xdisplay);
	set_env("X11VNC_PROG", program_name);
	set_env("X11VNC_CMDLINE", program_cmdline);
	set_env("X11VNC_WISHCMD", wish);
	if (simple_gui) {
		set_env("X11VNC_SIMPLE_GUI", "1");
	}
	if (gui_opts) {
		set_env("X11VNC_GUI_PARAMS", gui_opts);
	}
	if (gui_geometry) {
		set_env("X11VNC_GUI_GEOM", gui_geometry);
	}
	if (start_x11vnc) {
		set_env("X11VNC_STARTED", "1");
	}
	if (icon_mode) {
		set_env("X11VNC_ICON_MODE", "1");
		if (icon_mode_file) {
			set_env("X11VNC_CLIENT_FILE", icon_mode_file);
		}
		if (icon_in_tray) {
			if (tray_manager_ok) {
				set_env("X11VNC_ICON_MODE", "TRAY:RUNNING");
			} else {
				set_env("X11VNC_ICON_MODE", "TRAY");
			}
		} else {
			set_env("X11VNC_ICON_MODE", "ICON");
		}
		if (icon_mode_params) {
			char *p, *str = strdup(icon_mode_params);
			p = strtok(str, ":-/,.+");
			while (p) {
				if(strstr(p, "setp") == p) {
					set_env("X11VNC_ICON_SETPASS", "1");
					if (rc != 0) {
						set_env("X11VNC_SETPASS_FAIL", "1");
					}
				} else if(strstr(p, "noadvanced") == p) {
					set_env("X11VNC_ICON_NOADVANCED", "1");
				} else if(strstr(p, "minimal") == p) {
					set_env("X11VNC_ICON_MINIMAL", "1");
				} else if (strstr(p, "0x") == p) {
					set_env("X11VNC_ICON_EMBED_ID", p);
					icon_mode_embed_id = strdup(p);
				}
				p = strtok(NULL, ":-/,.+");
			}
			free(str);
		}
	}
	if (icon_mode_font) {
		set_env("X11VNC_ICON_FONT", icon_mode_font);
	}

	/* gui */
	if (no_external_cmds || !cmd_ok("gui")) {
		fprintf(stderr, "cannot run external commands in -nocmds "
		    "mode:\n");
		fprintf(stderr, "   \"%s\"\n", "gui + wish");
		fprintf(stderr, "   exiting.\n");
		fflush(stderr);
		exit(1);
	}

	tmpf = tmpfile();
	if (tmpf == NULL) {
		/* if no tmpfile, use a pipe */
		if (icon_mode_embed_id) {
			if (strlen(icon_mode_embed_id) < 20) {
				strcat(cmd, " -use ");
				strcat(cmd, icon_mode_embed_id);
			}
		}
		close_exec_fds();
		pipe = popen(cmd, "w");
		if (! pipe) {
			fprintf(stderr, "could not run: %s\n", cmd);
			perror("popen");
		}
		fprintf(pipe, "%s", gui_code);
		pclose(pipe);
	} else {
		/*
		 * we prefer a tmpfile since then this x11vnc process
		 * will then be gone, otherwise the x11vnc program text
		 * will still be in use.
		 */
		int n = fileno(tmpf);
		fprintf(tmpf, "%s", gui_code);
		fflush(tmpf);
		rewind(tmpf);
		dup2(n, 0);
		close(n);
		if (icon_mode_embed_id) {
			execlp(wish, wish, "-", "-use", icon_mode_embed_id,
			    (char *) NULL); 
		} else {
			execlp(wish, wish, "-", (char *) NULL); 
		}
		fprintf(stderr, "could not exec wish: %s -\n", wish);
		perror("execlp");
	}
	exit(0);
}

void do_gui(char *opts, int sleep) {
	char *s, *p;
	char *old_xauth = NULL;
	char *gui_xdisplay = NULL;
	int got_gui_xdisplay = 0;
	int start_x11vnc = 1;
	int connect_to_x11vnc = 0;
	int simple_gui = 0, none_gui = 0;
	int portprompt = 0;
	Display *test_dpy;

	if (opts) {
		s = strdup(opts);
	} else {
		s = strdup("");
	}

	if (use_dpy) {
		/* worst case */
		gui_xdisplay = strdup(use_dpy);
		
	}
	if (getenv("DISPLAY") != NULL) {
		/* better */
		gui_xdisplay = strdup(getenv("DISPLAY"));
	}

	p = strtok(s, ",");

	while(p) {
		if (*p == '\0') {
			;
		} else if (strchr(p, ':') != NULL) {
			/* best */
			if (gui_xdisplay) {
				free(gui_xdisplay);
			}
			gui_xdisplay = strdup(p);
			got_gui_xdisplay = 1;
		} else if (!strcmp(p, "wait")) {
			start_x11vnc = 0;
			connect_to_x11vnc = 0;
		} else if (!strcmp(p, "none")) {
			none_gui = 1;
		} else if (!strcmp(p, "portprompt")) {
			start_x11vnc = 0;
			connect_to_x11vnc = 0;
			portprompt = 1;
		} else if (!strcmp(p, "conn") || !strcmp(p, "connect")) {
			start_x11vnc = 0;
			connect_to_x11vnc = 1;
		} else if (!strcmp(p, "ez") || !strcmp(p, "simple")) {
			simple_gui = 1;
		} else if (strstr(p, "iconfont") == p) {
			char *q;
			if ((q = strchr(p, '=')) != NULL) {
				icon_mode_font = strdup(q+1);
			}
		} else if (strstr(p, "full") == p) {
			if (strstr(p, "setp") && 0) {
				set_env("X11VNC_ICON_MODE", "2");
				set_env("X11VNC_ICON_SETPASS", "2");
			}
		} else if (strstr(p, "tray") == p || strstr(p, "icon") == p) {
			char *q;
			icon_mode = 1;
			if ((q = strchr(p, '=')) != NULL) {
				icon_mode_params = strdup(q+1);
				if (strstr(icon_mode_params, "setp")) {
					deny_all = 1;
				}
			}
			if (strstr(p, "tray") == p) {
				icon_in_tray = 1;
			}
		} else if (strstr(p, "geom") == p) {
			char *q;
			if ((q = strchr(p, '=')) != NULL) {
				gui_geometry = strdup(q+1);
			}
		} else {
			fprintf(stderr, "unrecognized gui opt: %s\n", p);
		}
		
		p = strtok(NULL, ",");
	}
	free(s);

	if (none_gui) {
		if (!start_x11vnc) {
			exit(0);
		}
		return;
	}
	if (start_x11vnc) {
		connect_to_x11vnc = 1;
	}


#ifdef MACOSX
	goto startit;
#endif

	if (icon_mode && !got_gui_xdisplay) {
		/* for tray mode, prefer the polled DISPLAY */
		if (use_dpy) {
			if (gui_xdisplay) {
				free(gui_xdisplay);
			}
			gui_xdisplay = strdup(use_dpy);
		}
	}

	if (! gui_xdisplay) {
		fprintf(stderr, "error: cannot determine X DISPLAY for gui"
		    " to display on.\n");
		exit(1);
	}
	if (!quiet && !portprompt) {
		fprintf(stderr, "starting gui, trying display: %s\n",
		    gui_xdisplay);
	}
	test_dpy = XOpenDisplay_wr(gui_xdisplay);
	if (! test_dpy && auth_file) {
		if (getenv("XAUTHORITY") != NULL) {
			old_xauth = strdup(getenv("XAUTHORITY"));
		}
		set_env("XAUTHORITY", auth_file);
		test_dpy = XOpenDisplay_wr(gui_xdisplay);
	}
	if (! test_dpy) {
		if (! old_xauth && getenv("XAUTHORITY") != NULL) {
			old_xauth = strdup(getenv("XAUTHORITY"));
		}
		set_env("XAUTHORITY", "");
		test_dpy = XOpenDisplay_wr(gui_xdisplay);
	}
	if (! test_dpy) {
		fprintf(stderr, "error: cannot connect to gui X DISPLAY: %s\n",
		    gui_xdisplay);
		exit(1);
	}
	if (icon_mode && icon_in_tray) {
		if (tray_manager_running(test_dpy, NULL)) {
			tray_manager_ok = 1;
		} else {
			tray_manager_ok = 0;
		}
	}
	XCloseDisplay_wr(test_dpy);

#ifdef MACOSX
	startit:
#endif
	if (portprompt) {
		char *cmd, *p, *p2, *p1, *p0 = getenv("PATH");
		char tf1[] = "/tmp/x11vnc_port_prompt.2XXXXXX";
		char tf2[] = "/tmp/x11vnc_port_prompt.1XXXXXX";
		int fd;
		char *dstr = "", *wish = NULL;
		char line[128];
		FILE *fp;

		if (no_external_cmds || !cmd_ok("gui")) {
			return;
		}

		if (gui_xdisplay) {
			dstr = gui_xdisplay;
			if (strchr(gui_xdisplay, '\'')) {
				return;
			}
		}
		if (!p0) {
			p0 = "";
		}
		if (strchr(p0, '\'')) {
			return;
		}

		fd = mkstemp(tf2);
		if (fd < 0) {
			return;
		}
		close(fd);

		fd = mkstemp(tf1);
		if (fd < 0) {
			unlink(tf2);
			return;
		}

		write(fd, gui_code, strlen(gui_code));
		close(fd);

		p1 = (char *) malloc(10 + strlen(p0) + strlen(extra_path));
		sprintf(p1, "%s:%s", p0, extra_path);
		p2 = strdup(p1);
		p = strtok(p2, ":");

		while (p) {
			char *try;
			struct stat sbuf;
			int i;

			try = (char *) malloc(strlen(p) + 1 + strlen("wish8.4") + 1);
			i = 0;
			while (wishes[i] != NULL) {
				sprintf(try, "%s/%s", p, wishes[i]);
				if (stat(try, &sbuf) == 0) {
					/* assume executable, should check mode */
					wish = wishes[i];
					break;
				}
				i++;
			}
			free(try);
			if (wish) {
				break;
			}
			p = strtok(NULL, ":");
		}
		free(p2);

		if (!wish) {
			wish = "wish";
		}

		cmd = (char *) malloc(200 + strlen(dstr) + strlen(p1));

		if (!strcmp(dstr, "")) {
			sprintf(cmd, "env PATH='%s' %s %s -name x11vnc_port_prompt -portprompt > %s", p1, wish, tf1, tf2);
		} else {
			sprintf(cmd, "env PATH='%s' DISPLAY='%s' %s %s -name x11vnc_port_prompt -portprompt > %s", p1, dstr, wish, tf1, tf2);
		}
		if (getenv("X11VNC_DEBUG_PORTPROMPT")) {
			fprintf(stderr, "cmd=%s\n", cmd);
		}
		if (use_openssl) {
			set_env("X11VNC_SSL_ENABLED", "1");
		}
		if (allow_list && !strcmp(allow_list, "127.0.0.1")) {
			set_env("X11VNC_LOCALHOST_ENABLED", "1");
		}
		if (got_ultrafilexfer) {
			set_env("X11VNC_FILETRANSFER_ENABLED", "ultra");
		} else if (tightfilexfer) {
			set_env("X11VNC_FILETRANSFER_ENABLED", "tight");
		}
		system(cmd);
		free(cmd);
		free(p1);

		fp = fopen(tf2, "r");
		memset(line, 0, sizeof(line));
		if (fp) {
			fgets(line, 128, fp);
			fclose(fp);
			if (line[0] != '\0') {
				int readport = atoi(line);
				if (readport > 0) {
					got_rfbport_val = readport;
				}
			}
		}

		if (strstr(line, "ssl0")) {
			if (use_openssl) use_openssl = 0;
		} else if (strstr(line, "ssl1")) {
			if (!use_openssl) {
				use_openssl = 1;
				openssl_pem = strdup("SAVE_NOPROMPT");
				set_env("X11VNC_GOT_SSL", "1");
			}
		}

		if (strstr(line, "localhost0")) {
			if (allow_list && !strcmp(allow_list, "127.0.0.1")) {
				allow_list = NULL;
			}
		} else if (strstr(line, "localhost1")) {
			allow_list = strdup("127.0.0.1");
		}

		if (strstr(line, "ft_ultra")) {
			got_ultrafilexfer = 1;
			tightfilexfer = 0;
		} else if (strstr(line, "ft_tight")) {
			got_ultrafilexfer = 0;
			tightfilexfer = 1;
		} else if (strstr(line, "ft_none")) {
			got_ultrafilexfer = 0;
			tightfilexfer = 0;
		}

		unlink(tf1);
		unlink(tf2);

		if (old_xauth) {
			set_env("XAUTHORITY", old_xauth);
		}

		return;
	}

	if (start_x11vnc) {

#if LIBVNCSERVER_HAVE_FORK
		/* fork into the background now */
		int p;
		pid_t parent = getpid();

		if (icon_mode) {
			char tf[] = "/tmp/x11vnc.tray.XXXXXX"; 
			int fd;

			fd = mkstemp(tf);
			if (fd < 0) {
				icon_mode = 0;
			} else {
				close(fd);
				icon_mode_fh = fopen(tf, "w");
				if (! icon_mode_fh) {
					icon_mode = 0;
				} else {
					chmod(tf, 0400);
					icon_mode_file = strdup(tf);
					rfbLog("icon_mode_file=%s\n", icon_mode_file);
					fprintf(icon_mode_fh, "none\n");
					fprintf(icon_mode_fh, "none\n");
					fflush(icon_mode_fh);
					if (! got_connect_once) {
						if (!client_connect && !connect_or_exit) {
							/* want -forever for tray? */
							connect_once = 0;
						}
					}
				}
			}
		}

		if ((p = fork()) > 0)  {
			;	/* parent */
		} else if (p == -1) {
			fprintf(stderr, "could not fork\n");
			perror("fork");
			clean_up_exit(1);
		} else {
			if (sleep > 0) {
				usleep(sleep * 1000 * 1000);
			}
			run_gui(gui_xdisplay, connect_to_x11vnc, start_x11vnc,
			    simple_gui, parent, opts);
			exit(1);
		}
		if (connect_to_x11vnc) {
			run_gui_pid = p;
			gui_pid = p;
		}
#else
		fprintf(stderr, "system does not support fork: start "
		    "x11vnc in the gui.\n");
		start_x11vnc = 0;
#endif
	}
	if (!start_x11vnc) {
		run_gui(gui_xdisplay, connect_to_x11vnc, start_x11vnc,
		    simple_gui, 0, opts);
		exit(1);
	}
	if (old_xauth) {
		set_env("XAUTHORITY", old_xauth);
	}
}


