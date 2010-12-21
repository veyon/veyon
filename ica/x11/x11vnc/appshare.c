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

/* -- appshare.c -- */

#include "x11vnc.h"

extern int pick_windowid(unsigned long *num);
extern char *get_xprop(char *prop, Window win);
extern int set_xprop(char *prop, Window win, char *value);
extern void set_env(char *name, char *value);
extern double dnow(void);

static char *usage =
"\n"
"  x11vnc -appshare: an experiment in application sharing via x11vnc.\n"
"\n"
#if !SMALL_FOOTPRINT
"  Usage:   x11vnc -appshare -id windowid -connect viewer_host:0\n"
"           x11vnc -appshare -id pick     -connect viewer_host:0\n"
"\n"
"  Both the -connect option and the -id (or -sid) option are required.\n"
"  (However see the -control option below that can replace -connect.)\n"
"\n"
"  The VNC viewer at viewer_host MUST be in 'listen' mode.  This is because\n"
"  a new VNC connection (and viewer window) is established for each new\n"
"  toplevel window that the application creates.  For example:\n"
"\n"
"       vncviewer -listen 0\n"
"\n"
"  The '-connect viewer_host:0' indicates the listening viewer to connect to.\n"
"\n"
"  No password should be used, otherwise it will need to be typed for each\n"
"  new window (or one could use vncviewer -passwd file if the viewer supports\n"
"  that.)  For security an SSH tunnel can be used:\n"
"\n"
"       ssh -R 5500:localhost:5500 user@server_host\n"
"\n"
"  (then use -connect localhost:0)\n"
"\n"
"  The -id/-sid option is as in x11vnc(1).  It is either a numerical window\n"
"  id or the string 'pick' which will ask the user to click on an app window.\n"
"  To track more than one application at the same time, list their window ids\n"
"  separated by commas (see also the 'add_app' command below.)\n"
"\n"
"  Additional options:\n"
"\n"
"      -h, -help      Print this help.\n"
"      -debug         Print debugging output (same as X11VNC_APPSHARE_DEBUG=1)\n"
"      -showmenus     Create a new viewer window even if a new window is\n"
"                     completely inside of an existing one.  Default is to\n"
"                     try to not show them in a new viewer window.\n"
"      -noexit        Do not exit if the main app (windowid/pick) window\n"
"                     goes away.  Default is to exit.\n"
"      -display dpy   X DISPLAY to use.\n"
"      -trackdir dir  Set tracking directory to 'dir'. x11vnc -appshare does\n"
"                     better if it can communicate with the x11vnc's via a\n"
"                     file channel. By default a dir in /tmp is used, -trackdir\n"
"                     specifies another directory, or use 'none' to disable.\n"
"      -args 'string' Pass options 'string' to x11vnc (e.g. -scale 3/4,\n"
"                     -viewonly, -wait, -once, etc.)\n"
"      -env VAR=VAL   Set environment variables on cmdline as in x11vnc.\n"
"\n"
"      -control file  This is a file that one edits to manage the appshare\n"
"                     mode.  It replaces -connect.  Lines beginning with '#'\n"
"                     are ignored.  Initially start off with all of the\n"
"                     desired clients in the file, one per line.  If you add\n"
"                     a new client-line, that client is connected to. If you\n"
"                     delete (or comment out) a client-line, that client is\n"
"                     disconnected (for this to work, do not disable trackdir.)\n"
"\n"
"                     You can also put cmd= lines in the control file to perform\n"
"                     different actions.  These are supported:\n"
"\n"
"                         cmd=quit            Disconnect all clients and exit.\n"
"                         cmd=restart         Restart all of the x11vnc's.\n"
"                         cmd=noop            Do nothing (e.g. ping)\n"
"                         cmd=x11vnc          Run ps(1) looking for x11vnc's\n"
"                         cmd=help            Print out help text.\n"
"                         cmd=add_window:win  Add a window to be watched.\n"
"                         cmd=del_window:win  Delete a window.\n"
"                         cmd=add_app:win     Add an application to be watched.\n"
"                         cmd=del_app:win     Delete an application.\n"
"                         cmd=add_client:host Add client ('internal' mode only)\n"
"                         cmd=del_client:host Del client ('internal' mode only)\n"
"                         cmd=list_windows    List all tracked windows.\n"
"                         cmd=list_apps       List all tracked applications.\n"
"                         cmd=list_clients    List all connected clients.\n"
"                         cmd=list_all        List all three.\n"
"                         cmd=print_logs      Print out the x11vnc logfiles.\n"
"                         cmd=debug:n         Set -debug to n (0 or 1).\n"
"                         cmd=showmenus:n     Set -showmenus to n (0 or 1).\n"
"                         cmd=noexit:n        Set -noexit to n (0 or 1).\n"
"\n"
"                     See the '-command internal' mode described below for a way\n"
"                     that tracks connected clients internally (not in a file.)\n"
"\n"
"                     In '-shell' mode (see below) you can type in the above\n"
"                     without the leading 'cmd='.\n"
"\n"
"                     For 'add_window' and 'del_window' the 'win' can be a\n"
"                     numerical window id or 'pick'.  Same for 'add_app'.  Be\n"
"                     sure to remove or comment out the add/del line quickly\n"
"                     (e.g. before picking) or it will be re-run the next time\n"
"                     the file is processed.\n"
"\n"
"                     If a file with the same name as the control file but\n"
"                     ending with suffix '.cmd' is found, then commands in it\n"
"                     (cmd=...) are processed and then the file is truncated.\n"
"                     This allows 'one time' command actions to be run.  Any\n"
"                     client hostnames in the '.cmd' file are ignored.  Also\n"
"                     see below for the X11VNC_APPSHARE_COMMAND X property\n"
"                     which is similar to '.cmd'\n"
"\n"
"      -control internal   Manage connected clients internally, see below.\n"
"      -control shell      Same as: -shell -control internal\n"
"\n"
"      -delay secs    Maximum timeout delay before re-checking the control file.\n"
"                     It can be a fraction, e.g. -delay 0.25  Default 0.5\n"
"\n"
"      -shell         Simple command line for '-control internal' mode (see the\n"
"                     details of this mode below.)  Enter '?' for command list.\n"
"\n"
"  To stop x11vnc -appshare press Ctrl-C, or (if -noexit not supplied) delete\n"
"  the initial app window or exit the application. Or cmd=quit in -control mode.\n"
"\n"
#if 0
"  If you want your setup to survive periods of time where there are no clients\n"
"  connected you will need to supply -args '-forever' otherwise the x11vnc's\n"
"  will exit when the last client disconnects.  Howerver, _starting_ with no\n"
"  clients (e.g. empty control file) will work without -args '-forever'.\n"
"\n"
#endif
"  In addition to the '.cmd' file channel, for faster response you can set\n"
"  X11VNC_APPSHARE_COMMAND X property on the root window to the string that\n"
"  would go into the '.cmd' file.  For example:\n"
"\n"
" xprop -root -f X11VNC_APPSHARE_COMMAND 8s -set X11VNC_APPSHARE_COMMAND cmd=quit\n"
"\n"
"  The property value will be set to 'DONE' after the command(s) is processed.\n"
"\n"
"  If -control file is specified as 'internal' then no control file is used\n"
"  and client tracking is done internally.  You must add and delete clients\n"
"  with the cmd=add_client:<client> and cmd=del_client:<client> commands.\n"
"  Note that '-control internal' is required for '-shell' mode.  Using\n"
"  '-control shell' implies internal mode and -shell.\n"
"\n"
"  Limitations:\n"
"\n"
"     This is a quick lash-up, many things will not work properly.\n"
"\n"
"     The main idea is to provide simple application sharing for two or more\n"
"     parties to collaborate without needing to share the entire desktop.  It\n"
"     provides an improvement over -id/-sid that only shows a single window.\n"
"\n"
"     Only reverse connections can be done.  (Note: one can specify multiple\n"
"     viewing hosts via: -connect host1,host2,host3 or add/remove them\n"
"     dynamically as described above.)\n"
"\n"
"     If a new window obscures an old one, you will see some or all of the\n"
"     new window in the old one.  The hope is this is a popup dialog or menu\n"
"     that will go away soon.  Otherwise a user at the physical display will\n"
"     need to move it. (See also the SSVNC viewer features described below.) \n"
"\n"
"     The viewer side cannot resize or make windows move on the physical\n"
"     display.  Again, a user at the physical display may need to help, or\n"
"     use the SSVNC viewer (see Tip below.)\n"
"\n"
"     Tip: If the application has its own 'resize corner', then dragging\n"
"          it may successfully resize the application window.\n"
"     Tip: Some desktop environments enable moving a window via, say,\n"
"          Alt+Left-Button-Drag.  One may be able to move a window this way.\n"
"          Also, e.g., Alt+Right-Button-Drag may resize a window.\n"
"     Tip: Clicking on part of an obscured window may raise it to the top.\n"
"          Also, e.g., Alt+Middle-Button may toggle Raise/Lower.\n"
"\n"
"     Tip: The SSVNC 1.0.25 unix and macosx vncviewer has 'EscapeKeys' hot\n"
"          keys that will move, resize, raise, and lower the window via the\n"
"          x11vnc -remote_prefix X11VNC_APPSHARE_CMD: feature.  So in the\n"
"          viewer while holding down Shift_L+Super_L+Alt_L the arrow keys\n"
"          move the window, PageUp/PageDn/Home/End resize it, and - and +\n"
"          raise and lower it.  Key 'M' or Button1 moves the remote window\n"
"          to the +X+Y of the viewer window.  Key 'D' or Button3 deletes\n"
"          the remote window.\n"
"\n"
"          You can run the SSVNC vncviewer with options '-escape default',\n"
"          '-multilisten' and '-env VNCVIEWER_MIN_TITLE=1'; or just run\n"
"          with option '-appshare' to enable these and automatic placement.\n"
"\n"
"     If any part of a window goes off of the display screen, then x11vnc\n"
"     may be unable to poll it (without crashing), and so the window will\n"
"     stop updating until the window is completely on-screen again.\n"
"\n"
"     The (stock) vnc viewer does not know where to best position each new\n"
"     viewer window; it likely centers each one (including when resized.)\n"
"     Note: The SSVNC viewer in '-appshare' mode places them correctly.\n"
"\n"
"     Deleting a viewer window does not delete the real window.\n"
"     Note: The SSVNC viewer Shift+EscapeKeys+Button3 deletes it.\n"
"\n"
"     Sometimes new window detection fails.\n"
"\n"
"     Sometimes menu/popup detection fails.\n"
"\n"
"     Sometimes the contents of a menu/popup window have blacked-out regions.\n"
"     Try -sid or -showmenus as a workaround.\n"
"\n"
"     If the application starts up a new application (a different process)\n"
"     that new application will not be tracked (but, unfortunately, it may\n"
"     cover up existing windows that are being tracked.) See cmd=add_window\n"
"     and cmd=add_app described above.\n"
"\n"
#endif
;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WMAX 192
#define CMAX 128
#define AMAX 32

static Window root = None;
static Window watch[WMAX];
static Window apps[WMAX];
static int state[WMAX];
static char *clients[CMAX];
static XWindowAttributes attr;
static char *ticker_atom_str = "X11VNC_APPSHARE_TICKER";
static Atom ticker_atom = None;
static char *cmd_atom_str = "X11VNC_APPSHARE_COMMAND";
static Atom cmd_atom = None;
static char *connect_to = NULL;
static char *x11vnc_args = "";
static char *id_opt = "-id";
static int skip_menus = 1;
static int exit_no_app_win = 1;
static int shell = 0;
static int tree_depth = 3;
static char *prompt = "appshare> ";
static char *x11vnc = "x11vnc";
static char *control = NULL;
static char *trackdir = "unset";
static char *trackpre = "/tmp/x11vnc-appshare-trackdir-tmp";
static char *tracktmp = NULL;
static char unique_tag[100];
static int use_forever = 1;
static int last_event_type = 0;
static pid_t helper_pid = 0;
static pid_t parent_pid = 0;
static double helper_delay = 0.5;
static int appshare_debug = 0;
static double start_time = 0.0;

static void get_wm_name(Window win, char **name);
static int win_attr(Window win);
static int get_xy(Window win, int *x, int *y);
static Window check_inside(Window win);
static int ours(Window win);
static void destroy_win(Window win);
static int same_app(Window win, Window app);

static void ff(void) {
	fflush(stdout);
	fflush(stderr);
}

static int find_win(Window win) {
	int i;
	for (i=0; i < WMAX; i++) {
		if (watch[i] == win) {
			return i;
		}
	}
	return -1;
}

static int find_app(Window app) {
	int i;
	for (i=0; i < AMAX; i++) {
		if (apps[i] == app) {
			return i;
		}
	}
	return -1;
}

static int find_client(char *cl) {
	int i;
	for (i=0; i < CMAX; i++) {
		if (cl == NULL) {
			if (clients[i] == NULL) {
				return i;
			}
			continue;
		}
		if (clients[i] == NULL) {
			continue;
		}
		if (!strcmp(clients[i], cl)) {
			return i;
		}
	}
	return -1;
}

static int trackdir_pid(Window win) {
	FILE *f;
	int ln = 0, pid = 0;
	char line[1024];

	if (!trackdir) {
		return 0;
	}
	sprintf(tracktmp, "%s/0x%lx.log", trackdir, win);
	f = fopen(tracktmp, "r");
	if (!f) {
		return 0;
	}
	while (fgets(line, sizeof(line), f) != NULL) {
		if (ln++ > 30) {
			break;
		}
		if (strstr(line, "x11vnc version:")) {
			char *q = strstr(line, "pid:");
			if (q) {
				int p;
				if (sscanf(q, "pid: %d", &p) == 1) {
					if (p > 0) {
						pid = p;
						break;
					}
				}
			}
		}
	}
	fclose(f);
	return pid;
}

static void trackdir_cleanup(Window win) {
	char *suffix[] = {"log", "connect", NULL};
	int i=0;
	if (!trackdir) {
		return;
	}
	while (suffix[i] != NULL) {
		sprintf(tracktmp, "%s/0x%lx.%s", trackdir, win, suffix[i]);
		if (appshare_debug && !strcmp(suffix[i], "log")) {
			fprintf(stderr, "keeping:  %s\n", tracktmp);
			ff();
		} else {
			if (appshare_debug) {
				fprintf(stderr, "removing: %s\n", tracktmp);
				ff();
			}
			unlink(tracktmp);
		}
		i++;
	}
}

static void launch(Window win) {
	char *cmd, *tmp, *connto, *name;
	int len, timeo = 30, uf = use_forever;
	int w = 0, h = 0, x = 0, y = 0;

	if (win_attr(win)) {
		/* maybe switch to debug only. */
		w = attr.width;
		h = attr.height;
		get_xy(win, &x, &y);
	}

	get_wm_name(win, &name);

	if (strstr(x11vnc_args, "-once")) {
		uf = 0;
	}

	if (control) {
		int i = 0;
		len = 0;
		for (i=0; i < CMAX; i++) {
			if (clients[i] != NULL) {
				len += strlen(clients[i]) + 2;
			}
		}
		connto = (char *) calloc(len, 1);
		for (i=0; i < CMAX; i++) {
			if (clients[i] != NULL) {
				if (connto[0] != '\0') {
					strcat(connto, ",");
				}
				strcat(connto, clients[i]);
			}
		}
	} else {
		connto = strdup(connect_to);
	}
	if (!strcmp(connto, "")) {
		timeo = 0;
	}
	if (uf) {
		timeo = 0;
	}
	
	len = 1000 + strlen(x11vnc) + strlen(connto) + strlen(x11vnc_args)
	    + 3 * (trackdir ? strlen(trackdir) : 100);

	cmd = (char *) calloc(len, 1);
	tmp = (char *) calloc(len, 1);

	sprintf(cmd, "%s %s 0x%lx -bg -quiet %s -nopw -rfbport 0 "
	    "-timeout %d -noxdamage -noxinerama -norc -repeat -speeds dsl "
	    "-env X11VNC_AVOID_WINDOWS=never -env X11VNC_APPSHARE_ACTIVE=1 "
	    "-env X11VNC_NO_CHECK_PM=1 -env %s -novncconnect -shared -nonap "
	    "-remote_prefix X11VNC_APPSHARE_CMD:",
	    x11vnc, id_opt, win, use_forever ? "-forever" : "-once", timeo, unique_tag);

	if (trackdir) {
		FILE *f;
		sprintf(tracktmp, " -noquiet -o %s/0x%lx.log", trackdir, win);
		strcat(cmd, tracktmp);
		sprintf(tracktmp, "%s/0x%lx.connect", trackdir, win);
		f = fopen(tracktmp, "w");
		if (f) {
			fprintf(f, "%s", connto);
			fclose(f);
			sprintf(tmp, " -connect_or_exit '%s'", tracktmp);
			strcat(cmd, tmp);
		} else {
			sprintf(tmp, " -connect_or_exit '%s'", connto);
			strcat(cmd, tmp);
		}
	} else {
		if (!strcmp(connto, "")) {
			sprintf(tmp, " -connect '%s'", connto);
		} else {
			sprintf(tmp, " -connect_or_exit '%s'", connto);
		}
		strcat(cmd, tmp);
	}
	if (uf) {
		char *q = strstr(cmd, "-connect_or_exit");
		if (q) q = strstr(q, "_or_exit");
		if (q) {
			unsigned int i;
			for (i=0; i < strlen("_or_exit"); i++) {
				*q = ' ';
				q++;
			}
		}
	}

	strcat(cmd, " ");
	strcat(cmd, x11vnc_args);

	fprintf(stdout, "launching: x11vnc for window 0x%08lx %dx%d+%d+%d \"%s\"\n",
	    win, w, h, x, y, name);

	if (appshare_debug) {
		fprintf(stderr, "\nrunning:   %s\n\n", cmd);
	}
	ff();

	system(cmd);

	free(cmd);
	free(tmp);
	free(connto);
	free(name);
}

static void stop(Window win) {
	char *cmd;
	int pid = -1;
	int f = find_win(win);
	if (f < 0 || win == None) {
		return;
	}
	if (state[f] == 0) {
		return;
	}
	if (trackdir) {
		pid = trackdir_pid(win);
		if (pid > 0) {
			if (appshare_debug) {fprintf(stderr,
			    "sending SIGTERM to: %d\n", pid); ff();}
			kill((pid_t) pid, SIGTERM);
		}
	}

	cmd = (char *) malloc(1000 + strlen(x11vnc));
	sprintf(cmd, "pkill -TERM -f '%s %s 0x%lx -bg'", x11vnc, id_opt, win);
	if (appshare_debug) {
		fprintf(stdout, "stopping:  0x%08lx - %s\n", win, cmd);
	} else {
		fprintf(stdout, "stopping:  x11vnc for window 0x%08lx  "
		    "(pid: %d)\n", win, pid);
	}
	ff();
	system(cmd);

	sprintf(cmd, "(sleep 0.25 2>/dev/null || sleep 1; pkill -KILL -f '%s "
	    "%s 0x%lx -bg') &", x11vnc, id_opt, win);
	system(cmd);

	if (trackdir) {
		trackdir_cleanup(win);
	}

	free(cmd);
}

static void kill_helper_pid(void) {
	int status;
	if (helper_pid <= 0) {
		return;
	}
	fprintf(stderr, "stopping: helper_pid: %d\n", (int) helper_pid);
	kill(helper_pid, SIGTERM);
	usleep(50 * 1000);
	kill(helper_pid, SIGKILL);
	usleep(25 * 1000);
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID 
	waitpid(helper_pid, &status, WNOHANG); 
#endif
}

static void be_helper_pid(char *dpy_str) {
	int cnt = 0;
	int ms = (int) (1000 * helper_delay);
	double last_check = 0.0;

	if (ms < 50) ms = 50;

#if NO_X11
	fprintf(stderr, "be_helper_pid: not compiled with X11.\n");
#else
	dpy = XOpenDisplay(dpy_str);
	ticker_atom = XInternAtom(dpy, ticker_atom_str, False);

	while (1) {
		char tmp[32];
		sprintf(tmp, "HELPER_CNT_%08d", cnt++);
		XChangeProperty(dpy, DefaultRootWindow(dpy), ticker_atom, XA_STRING, 8,
		    PropModeReplace, (unsigned char *) tmp, strlen(tmp));
		XFlush(dpy);
		usleep(ms*1000);
		if (parent_pid > 0) {
			if(dnow() > last_check + 1.0) {
				last_check = dnow();
				if (kill(parent_pid, 0) != 0) {
					fprintf(stderr, "be_helper_pid: parent %d is gone.\n", (int) parent_pid);
					break;
				}
			}
		}
	}
#endif
	exit(0);
}

static void print_logs(void) {
	if (trackdir) {
		DIR *dir = opendir(trackdir);
		if (dir) {
			struct dirent *dp;
			while ( (dp = readdir(dir)) != NULL) {
				FILE *f;
				char *name = dp->d_name;
				if (!strcmp(name, ".") || !strcmp(name, "..")) {
					continue;
				}
				if (strstr(name, "0x") != name) {
					continue;
				}
				if (strstr(name, ".log") == NULL) {
					continue;
				}
				sprintf(tracktmp, "%s/%s", trackdir, name);
				f = fopen(tracktmp, "r");
				if (f) {
					char line[1024];
					fprintf(stderr, "===== x11vnc log %s =====\n", tracktmp);
					while (fgets(line, sizeof(line), f) != NULL) {
						fprintf(stderr, "%s", line);
					}
					fprintf(stderr, "\n");
					ff();
					fclose(f);
				}
			}
			closedir(dir);
		}
	}
}

static void appshare_cleanup(int s) {
	int i;
	if (s) {}

	if (use_forever) {
		/* launch this backup in case they kill -9 us before we terminate everything */
		char cmd[1000];
		sprintf(cmd, "(sleep 3; pkill -TERM -f '%s') &", unique_tag);
		if (appshare_debug) fprintf(stderr, "%s\n", cmd);
		system(cmd);
	}

	for (i=0; i < WMAX; i++) {
		if (watch[i] != None) {
			stop(watch[i]);
		}
	}

	if (trackdir) {
		DIR *dir = opendir(trackdir);
		if (dir) {
			struct dirent *dp;
			while ( (dp = readdir(dir)) != NULL) {
				char *name = dp->d_name;
				if (!strcmp(name, ".") || !strcmp(name, "..")) {
					continue;
				}
				if (strstr(name, "0x") != name) {
					fprintf(stderr, "skipping: %s\n", name);
					continue;
				}
				if (!appshare_debug) {
					fprintf(stderr, "removing: %s\n", name);
					sprintf(tracktmp, "%s/%s", trackdir, name);
					unlink(tracktmp);
				} else {
					if (appshare_debug) fprintf(stderr, "keeping:  %s\n", name);
				}
			}
			closedir(dir);
		}
		if (!appshare_debug) {
			if (strstr(trackdir, trackpre) == trackdir) {
				if (appshare_debug) fprintf(stderr, "removing: %s\n", trackdir);
				rmdir(trackdir);
			}
		}
		ff();
	}

	kill_helper_pid();
			
#if !NO_X11
	XCloseDisplay(dpy);
#endif
	fprintf(stdout, "done.\n");
	ff();
	exit(0);
}

static int trap_xerror(Display *d, XErrorEvent *error) {
	if (d || error) {}
	return 0;
}

#if 0
typedef struct {
    int x, y;                   /* location of window */
    int width, height;          /* width and height of window */
    int border_width;           /* border width of window */
    int depth;                  /* depth of window */
    Visual *visual;             /* the associated visual structure */
    Window root;                /* root of screen containing window */
    int class;                  /* InputOutput, InputOnly*/
    int bit_gravity;            /* one of bit gravity values */
    int win_gravity;            /* one of the window gravity values */
    int backing_store;          /* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preserved if possible */
    unsigned long backing_pixel;/* value to be used when restoring planes */
    Bool save_under;            /* boolean, should bits under be saved? */
    Colormap colormap;          /* color map to be associated with window */
    Bool map_installed;         /* boolean, is color map currently installed*/
    int map_state;              /* IsUnmapped, IsUnviewable, IsViewable */
    long all_event_masks;       /* set of events all people have interest in*/
    long your_event_mask;       /* my event mask */
    long do_not_propagate_mask; /* set of events that should not propagate */
    Bool override_redirect;     /* boolean value for override-redirect */
    Screen *screen;             /* back pointer to correct screen */
} XWindowAttributes;
#endif

static void get_wm_name(Window win, char **name) {
	int ok;

#if !NO_X11
        XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
	ok = XFetchName(dpy, win, name);
       	XSetErrorHandler(old_handler);
#endif

	if (!ok || *name == NULL) {
		*name = strdup("unknown");
	}
}

static int win_attr(Window win) {
	int ok = 0;
#if !NO_X11
        XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
	ok = XGetWindowAttributes(dpy, win, &attr);
       	XSetErrorHandler(old_handler);
#endif

	if (ok) {
		return 1;
	} else {
		return 0;
	}
}

static void win_select(Window win, int ignore) {
#if !NO_X11
        XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
	if (ignore) {
		XSelectInput(dpy, win, 0);
	} else {
		XSelectInput(dpy, win, SubstructureNotifyMask);
	}
	XSync(dpy, False);
       	XSetErrorHandler(old_handler);
#endif
}

static Window get_parent(Window win) {
	int ok;
	Window r, parent = None, *list = NULL;
	unsigned int nchild;

#if !NO_X11
        XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
	ok = XQueryTree(dpy, win, &r, &parent, &list, &nchild);
       	XSetErrorHandler(old_handler);

	if (!ok) {
		return None;
	}
	if (list) {
		XFree(list);
	}
#endif
	return parent;
}

static int get_xy(Window win, int *x, int *y) {
	Window cr;
	Bool rc = False; 
#if !NO_X11
	XErrorHandler old_handler = XSetErrorHandler(trap_xerror);

	rc = XTranslateCoordinates(dpy, win, root, 0, 0, x, y, &cr);
       	XSetErrorHandler(old_handler);
#endif

	if (!rc) {
		return 0;
	} else {
		return 1;
	}
}

static Window check_inside(Window win) {
	int i, nwin = 0;
	int w, h, x, y;
	int Ws[WMAX], Hs[WMAX], Xs[WMAX], Ys[WMAX];
	Window wins[WMAX];

	if (!win_attr(win)) {
		return None; 
	}

	/* store them first to give the win app more time to settle.  */
	for (i=0; i < WMAX; i++) {
		int X, Y;
		Window wchk = watch[i];
		if (wchk == None) {
			continue;
		}
		if (state[i] == 0) {
			continue;
		}
		if (!win_attr(wchk)) {
			continue;
		}
		if (!get_xy(wchk, &X, &Y)) {
			continue;
		}

		Xs[nwin] = X;
		Ys[nwin] = Y;
		Ws[nwin] = attr.width;
		Hs[nwin] = attr.height;
		wins[nwin] = wchk;
		nwin++;
	}

	if (nwin == 0) {
		return None;
	}

	if (!win_attr(win)) {
		return None; 
	}
	w = attr.width;
	h = attr.height;

	get_xy(win, &x, &y);
	if (!get_xy(win, &x, &y)) {
		return None;
	}

	for (i=0; i < nwin; i++) {
		int X, Y, W, H;
		Window wchk = wins[i];
		X = Xs[i];
		Y = Ys[i];
		W = Ws[i];
		H = Hs[i];

		if (appshare_debug) fprintf(stderr, "check inside: 0x%lx  %dx%d+%d+%d %dx%d+%d+%d\n", wchk, w, h, x, y, W, H, X, Y);

		if (X <= x && Y <= y) {
			if (x + w  <= X + W && y + h < Y + H) {
				return wchk;
			}
		}
	}

	return None;
}

static void add_win(Window win) {
	int idx  = find_win(win);
	int free = find_win(None);
	if (idx >= 0) {
		if (appshare_debug) {fprintf(stderr, "already watching window: 0x%lx\n", win); ff();}
		return;
	}
	if (free < 0) {
		fprintf(stderr, "ran out of slots for window: 0x%lx\n", win); ff();
		return;
	}

	if (appshare_debug) {fprintf(stderr, "watching: 0x%lx at %d\n", win, free); ff();}

	watch[free] = win;
	state[free] = 0;

	win_select(win, 0);
}

static void delete_win(Window win) {
	int i;
	for (i=0; i < WMAX; i++) {
		if (watch[i] == win) {
			watch[i] = None;
			state[i] = 0;
			if (appshare_debug) {fprintf(stderr, "deleting: 0x%lx at %d\n", win, i); ff();}
		}
	}
}

static void recurse_search(int level, int level_max, Window top, Window app, int *nw) {
	Window w, r, parent, *list = NULL;
	unsigned int nchild;
	int ok = 0;

	if (appshare_debug > 1) {
		fprintf(stderr, "level: %d level_max: %d  top: 0x%lx  app: 0x%lx\n", level, level_max, top, app);
	}
	if (level >= level_max) {
		return;
	}
	
#if !NO_X11
	ok = XQueryTree(dpy, top, &r, &parent, &list, &nchild);
	if (ok) {
		int i;
		for (i=0; i < (int) nchild; i++) {
			w = list[i];
			if (w == None || find_win(w) >= 0) {
				continue;
			}
			if (ours(w) && w != app) {
				if (appshare_debug) fprintf(stderr, "add level %d 0x%lx %d/%d\n",
				    level, w, i, nchild);
				add_win(w);
				(*nw)++;
			}
		}
		for (i=0; i < (int) nchild; i++) {
			w = list[i];
			if (w == None || ours(w)) {
				continue;
			} 
			recurse_search(level+1, level_max, w, app, nw);
		}
	}
	if (list) {
		XFree(list);
	}
#endif
}
		
static void add_app(Window app) {
	int i, nw = 0, free = -1;
        XErrorHandler old_handler;

#if !NO_X11
	i = find_app(app);
	if (i >= 0) {
		fprintf(stderr, "already tracking app: 0x%lx\n", app);
		return;
	}
	for (i=0; i < AMAX; i++) {
		if (same_app(apps[i], app)) {
			fprintf(stderr, "already tracking app: 0x%lx via 0x%lx\n", app, apps[i]);
			return;
		}
	}
	free = find_app(None);
	if (free < 0) {
		fprintf(stderr, "ran out of app slots.\n");
		return;
	}
	apps[free] = app;

	add_win(app);

        old_handler = XSetErrorHandler(trap_xerror);
	recurse_search(0, tree_depth, root, app, &nw);
       	XSetErrorHandler(old_handler);
#endif
	fprintf(stderr, "tracking %d windows related to app window 0x%lx\n", nw, app);
}

static void del_app(Window app) {
	int i;
	for (i=0; i < WMAX; i++) {
		Window win = watch[i];
		if (win != None) {
			if (same_app(app, win)) {
				destroy_win(win);
			}
		}
	}
	for (i=0; i < AMAX; i++) {
		Window app2 = apps[i];
		if (app2 != None) {
			if (same_app(app, app2)) {
				apps[i] = None;
			}
		}
	}
}

static void wait_until_empty(char *file) {
	double t = 0.0, dt = 0.05;
	while (t < 1.0) {
		struct stat sb;
		if (stat(file, &sb) != 0) {
			return;
		}
		if (sb.st_size == 0) {
			return;
		}
		t += dt;
		usleep( (int) (dt * 1000 * 1000) );
	}
}

static void client(char *client, int add) {
	DIR *dir;
	struct dirent *dp;

	if (!client) {
		return;
	}
	if (!trackdir) {
		fprintf(stderr, "no trackdir, cannot %s client: %s\n",
		    add ? "add" : "disconnect", client);
		ff();
		return;
	}
	fprintf(stdout, "%s client: %s\n", add ? "adding  " : "deleting", client);

	dir = opendir(trackdir);
	if (!dir) {
		fprintf(stderr, "could not opendir trackdir: %s\n", trackdir);
		return;
	}
	while ( (dp = readdir(dir)) != NULL) {
		char *name = dp->d_name;
		if (!strcmp(name, ".") || !strcmp(name, "..")) {
			continue;
		}
		if (strstr(name, "0x") != name) {
			continue;
		}
		if (strstr(name, ".connect")) {
			FILE *f;
			char *tmp;
			Window twin;

			if (scan_hexdec(name, &twin)) {
				int f = find_win(twin);
				if (appshare_debug) {
					fprintf(stderr, "twin: 0x%lx name=%s f=%d\n", twin, name, f);
					ff();
				}
				if (f < 0) {
					continue;
				}
			}

			tmp = (char *) calloc(100 + strlen(client), 1);
			sprintf(tracktmp, "%s/%s", trackdir, name);
			if (add) {
				sprintf(tmp, "%s\n", client);
			} else {
				sprintf(tmp, "cmd=close:%s\n", client);
			}
			wait_until_empty(tracktmp);
			f = fopen(tracktmp, "w");
			if (f) {
				if (appshare_debug) {
					fprintf(stderr, "%s client: %s + %s",
					add ? "add" : "disconnect", tracktmp, tmp);
					ff();
				}
				fprintf(f, "%s", tmp);
				fclose(f);
			}
			free(tmp);
		}
	}
	closedir(dir);
}

static void mapped(Window win) {
	int f;
	if (win == None) {
		return;
	}
	f = find_win(win);
	if (f < 0) {
		if (win_attr(win)) {
			if (get_parent(win) == root) {
				/* XXX more cases? */
				add_win(win);
			}
		}
	}
}

static void unmapped(Window win) {
	int f = find_win(win);
	if (f < 0 || win == None) {
		return;
	}
	stop(win);	
	state[f] = 0;
}

static void destroy_win(Window win) {
	stop(win);
	delete_win(win);
}

static Window parse_win(char *str) {
	Window win = None;
	if (!str) {
		return None;
	}
	if (!strcmp(str, "pick") || !strcmp(str, "p")) {
		static double last_pick = 0.0;
		if (dnow() < start_time + 15) {
			;
		} else if (dnow() < last_pick + 2) {
			return None;
		} else {
			last_pick = dnow();
		}
		if (!pick_windowid(&win)) {
			fprintf(stderr, "parse_win: bad window pick.\n");
			win = None;
		}
		if (win == root) {
			fprintf(stderr, "parse_win: ignoring pick of rootwin 0x%lx.\n", win);
			win = None;
		}
		ff();
	} else if (!scan_hexdec(str, &win)) {
		win = None;
	}
	return win;
}

static void add_or_del_app(char *str, int add) {
	Window win = parse_win(str);

	if (win != None) {
		if (add) {
			add_app(win);
		} else {
			del_app(win);
		}
	} else if (!strcmp(str, "all")) {
		if (!add) {
			int i;
			for (i=0; i < AMAX; i++) {
				if (apps[i] != None) {
					del_app(apps[i]);
				}
			}
		}
	}
}

static void add_or_del_win(char *str, int add) {
	Window win = parse_win(str);

	if (win != None) {
		int f = find_win(win);
		if (add) {
			if (f < 0 && win_attr(win)) {
				add_win(win);
			}
		} else {
			if (f >= 0) {
				destroy_win(win);
			}
		}
	} else if (!strcmp(str, "all")) {
		if (!add) {
			int i;
			for (i=0; i < WMAX; i++) {
				if (watch[i] != None) {
					destroy_win(watch[i]);
				}
			}
		}
	} 
}

static void add_or_del_client(char *str, int add) {
	int i;

	if (!str) {
		return;
	}
	if (strcmp(control, "internal")) {
		return;
	}
	if (add) {
		int idx  = find_client(str);
		int free = find_client(NULL);

		if (idx >=0) {
			fprintf(stderr, "already tracking client: %s in slot %d\n", str, idx);
			ff();
			return;
		}
		if (free < 0) {
			static int cnt = 0;
			if (cnt++ < 10) {
				fprintf(stderr, "ran out of client slots.\n");
				ff();
			}
			return;
		}
		clients[free] = strdup(str);
		client(str, 1);
	} else {
		if (str[0] == '#' || str[0] == '%') {
			if (sscanf(str+1, "%d", &i) == 1) {
				i--;
				if (0 <= i && i < CMAX) {
					if (clients[i] != NULL) {
						client(clients[i], 0);
						free(clients[i]);
						clients[i] = NULL;
						return;
					}
				}
			}
		} else if (!strcmp(str, "all")) {
			for (i=0; i < CMAX; i++) {
				if (clients[i] == NULL) {
					continue;
				}
				client(clients[i], 0);
				free(clients[i]);
				clients[i] = NULL;
			}
			return;
		}

		i = find_client(str);
		if (i >= 0) {
			free(clients[i]);
			clients[i] = NULL;
			client(str, 0);
		}
	}
}

static void restart_x11vnc(void) {
	int i, n = 0;
	Window win, active[WMAX];
	for (i=0; i < WMAX; i++) {
		win = watch[i];
		if (win == None) {
			continue;
		}
		if (state[i]) {
			active[n++] = win;
			stop(win);
		}
	}
	if (n) {
		usleep(1500 * 1000);
	}
	for (i=0; i < n; i++) {
		win = active[i];
		launch(win);
	}
}

static unsigned long cmask = 0x3fc00000; /* 00111111110000000000000000000000 */

static void init_cmask(void) {
	/* dependent on the X server implementation; XmuClientWindow better? */
	/* xc/programs/Xserver/include/resource.h */
	int didit = 0, res_cnt = 29, client_bits = 8;

	if (getenv("X11VNC_APPSHARE_CLIENT_MASK")) {
		unsigned long cr;
		if (sscanf(getenv("X11VNC_APPSHARE_CLIENT_MASK"), "0x%lx", &cr) == 1) {
			cmask = cr;
			didit = 1;
		}
	} else if (getenv("X11VNC_APPSHARE_CLIENT_BITS")) {
		int cr = atoi(getenv("X11VNC_APPSHARE_CLIENT_BITS"));
		if (cr > 0) {
			client_bits = cr;
		}
	}
	if (!didit) {
		cmask = (((1 << client_bits) - 1) << (res_cnt-client_bits));
	}
	fprintf(stderr, "client_mask: 0x%08lx\n", cmask);
}

static int same_app(Window win, Window app) {
	if ( (win & cmask) == (app & cmask) ) {
		return 1;
	} else {
		return 0;
	}
}

static int ours(Window win) {
	int i;
	for (i=0; i < AMAX; i++) {
		if (apps[i] != None) {
			if (same_app(win, apps[i])) {
				return 1;
			}
		}
	}
	return 0;
}

static void list_clients(void) {
	int i, n = 0;
	for (i=0; i < CMAX; i++) {
		if (clients[i] == NULL) {
			continue;
		}
		fprintf(stdout, "client[%02d] %s\n", ++n, clients[i]);
	}
	fprintf(stdout, "total clients: %d\n", n);
	ff();
}

static void list_windows(void) {
	int i, n = 0;
	for (i=0; i < WMAX; i++) {
		char *name;
		Window win = watch[i];
		if (win == None) {
			continue;
		}
		get_wm_name(win, &name);
		fprintf(stdout, "window[%02d] 0x%08lx state: %d slot: %03d \"%s\"\n",
		    ++n, win, state[i], i, name);
		free(name);
	}
	fprintf(stdout, "total windows: %d\n", n);
	ff();
}

static void list_apps(void) {
	int i, n = 0;
	for (i=0; i < AMAX; i++) {
		char *name;
		Window win = apps[i];
		if (win == None) {
			continue;
		}
		get_wm_name(win, &name);
		fprintf(stdout, "app[%02d] 0x%08lx state: %d slot: %03d \"%s\"\n",
		    ++n, win, state[i], i, name);
		free(name);
	}
	fprintf(stdout, "total apps: %d\n", n);
	ff();
}

static int process_control(char *file, int check_clients) {
	int i, nnew = 0, seen[CMAX];
	char line[1024], *newctl[CMAX];
	FILE *f;

	f = fopen(file, "r");
	if (!f) {
		return 1;
	}
	if (check_clients) {
		for (i=0; i < CMAX; i++) {
			seen[i] = 0;
		}
	}
	while (fgets(line, sizeof(line), f) != NULL) {
		char *q = strchr(line, '\n');
		if (q) *q = '\0';

		if (appshare_debug) {
			fprintf(stderr, "check_control: %s\n", line);
			ff();
		}

		q = lblanks(line);
		if (q[0] == '#') {
			continue;
		}
		if (!strcmp(q, "")) {
			continue;
		}
		if (strstr(q, "cmd=") == q) {
			char *cmd = q + strlen("cmd=");
			if (!strcmp(cmd, "quit")) {
				if (strcmp(control, file) && strstr(file, ".cmd")) {
					FILE *f2 = fopen(file, "w");
					if (f2) fclose(f2);
				}
				appshare_cleanup(0);
			} else if (!strcmp(cmd, "wait")) {
				return 0;
			} else if (strstr(cmd, "bcast:") == cmd) {
				;
			} else if (strstr(cmd, "del_window:") == cmd) {
				add_or_del_win(cmd + strlen("del_window:"), 0);
			} else if (strstr(cmd, "add_window:") == cmd) {
				add_or_del_win(cmd + strlen("add_window:"), 1);
			} else if (strstr(cmd, "del:") == cmd) {
				add_or_del_win(cmd + strlen("del:"), 0);
			} else if (strstr(cmd, "add:") == cmd) {
				add_or_del_win(cmd + strlen("add:"), 1);
			} else if (strstr(cmd, "del_client:") == cmd) {
				add_or_del_client(cmd + strlen("del_client:"), 0);
			} else if (strstr(cmd, "add_client:") == cmd) {
				add_or_del_client(cmd + strlen("add_client:"), 1);
			} else if (strstr(cmd, "-") == cmd) {
				add_or_del_client(cmd + strlen("-"), 0);
			} else if (strstr(cmd, "+") == cmd) {
				add_or_del_client(cmd + strlen("+"), 1);
			} else if (strstr(cmd, "del_app:") == cmd) {
				add_or_del_app(cmd + strlen("del_app:"), 0);
			} else if (strstr(cmd, "add_app:") == cmd) {
				add_or_del_app(cmd + strlen("add_app:"), 1);
			} else if (strstr(cmd, "debug:") == cmd) {
				appshare_debug = atoi(cmd + strlen("debug:"));
			} else if (strstr(cmd, "showmenus:") == cmd) {
				skip_menus = atoi(cmd + strlen("showmenus:"));
				skip_menus = !(skip_menus);
			} else if (strstr(cmd, "noexit:") == cmd) {
				exit_no_app_win = atoi(cmd + strlen("noexit:"));
				exit_no_app_win = !(exit_no_app_win);
			} else if (strstr(cmd, "use_forever:") == cmd) {
				use_forever = atoi(cmd + strlen("use_forever:"));
			} else if (strstr(cmd, "tree_depth:") == cmd) {
				tree_depth = atoi(cmd + strlen("tree_depth:"));
			} else if (strstr(cmd, "x11vnc_args:") == cmd) {
				x11vnc_args = strdup(cmd + strlen("x11vnc_args:"));
			} else if (strstr(cmd, "env:") == cmd) {
				putenv(cmd + strlen("env:"));
			} else if (strstr(cmd, "noop") == cmd) {
				;
			} else if (!strcmp(cmd, "restart")) {
				restart_x11vnc();
			} else if (!strcmp(cmd, "list_clients") || !strcmp(cmd, "lc")) {
				list_clients();
			} else if (!strcmp(cmd, "list_windows") || !strcmp(cmd, "lw")) {
				list_windows();
			} else if (!strcmp(cmd, "list_apps") || !strcmp(cmd, "la")) {
				list_apps();
			} else if (!strcmp(cmd, "list_all") || !strcmp(cmd, "ls")) {
				list_windows();
				fprintf(stderr, "\n");
				list_apps();
				fprintf(stderr, "\n");
				list_clients();
			} else if (!strcmp(cmd, "print_logs") || !strcmp(cmd, "pl")) {
				print_logs();
			} else if (!strcmp(cmd, "?") || !strcmp(cmd, "h") || !strcmp(cmd, "help")) {
				fprintf(stderr, "available commands:\n");
				fprintf(stderr, "\n");
				fprintf(stderr, "   quit restart noop x11vnc help ? ! !!\n");
				fprintf(stderr, "\n");
				fprintf(stderr, "   add_window:win  (add:win, add:pick)\n");
				fprintf(stderr, "   del_window:win  (del:win, del:pick, del:all)\n");
				fprintf(stderr, "   add_app:win     (add_app:pick)\n");
				fprintf(stderr, "   del_app:win     (del_app:pick, del_app:all)\n");
				fprintf(stderr, "   add_client:host (+host)\n");
				fprintf(stderr, "   del_client:host (-host, -all)\n");
				fprintf(stderr, "\n");
				fprintf(stderr, "   list_windows    (lw)\n");
				fprintf(stderr, "   list_apps       (la)\n");
				fprintf(stderr, "   list_clients    (lc)\n");
				fprintf(stderr, "   list_all        (ls)\n");
				fprintf(stderr, "   print_logs      (pl)\n");
				fprintf(stderr, "\n");
				fprintf(stderr, "   debug:n   showmenus:n   noexit:n\n");
			} else {
				fprintf(stderr, "unrecognized %s\n", q);
			}
			continue;
		}
		if (check_clients) {
			int idx = find_client(q);
			if (idx >= 0) {
				seen[idx] = 1;
			} else {
				newctl[nnew++] = strdup(q);
			}
		}
	}
	fclose(f);

	if (check_clients) {
		for (i=0; i < CMAX; i++) {
			if (clients[i] == NULL) {
				continue;
			}
			if (!seen[i]) {
				client(clients[i], 0);
				free(clients[i]);
				clients[i] = NULL;
			}
		}
		for (i=0; i < nnew; i++) {
			int free = find_client(NULL);
			if (free < 0) {
				static int cnt = 0;
				if (cnt++ < 10) {
					fprintf(stderr, "ran out of client slots.\n");
					ff();
					break;
				}
				continue;
			}
			clients[free] = newctl[i];
			client(newctl[i], 1);
		}
	}
	return 1;
}

static int check_control(void) {
	static int last_size = -1;
	static time_t last_mtime = 0;
	struct stat sb;
	char *control_cmd;

	if (!control) {
		return 1;
	}

	if (!strcmp(control, "internal")) {
		return 1;
	}
		
	control_cmd = (char *)malloc(strlen(control) + strlen(".cmd") + 1);
	sprintf(control_cmd, "%s.cmd", control);
	if (stat(control_cmd, &sb) == 0) {
		FILE *f;
		if (sb.st_size > 0) {
			process_control(control_cmd, 0);
		}
		f = fopen(control_cmd, "w");
		if (f) {
			fclose(f);
		}
	}
	free(control_cmd);

	if (stat(control, &sb) != 0) {
		return 1;
	}
	if (last_size == (int) sb.st_size && last_mtime == sb.st_mtime) {
		return 1;
	}
	last_size = (int) sb.st_size;
	last_mtime = sb.st_mtime;

	return process_control(control, 1);
}

static void update(void) {
	int i, app_ok = 0;
	if (last_event_type != PropertyNotify) {
		if (appshare_debug) fprintf(stderr, "\nupdate ...\n");
	} else if (appshare_debug > 1) {
		fprintf(stderr, "update ... propertynotify\n");
	}
	if (!check_control()) {
		return;
	}
	for (i=0; i < WMAX; i++) {
		Window win = watch[i];
		if (win == None) {
			continue;
		}
		if (!win_attr(win)) {
			destroy_win(win);
			continue;
		}
		if (find_app(win) >= 0) {
			app_ok++;
		}
		if (state[i] == 0) {
			if (attr.map_state == IsViewable) {
				if (skip_menus) {
					Window inside = check_inside(win);
					if (inside != None) {
						if (appshare_debug) {fprintf(stderr, "skip_menus: window 0x%lx is inside of 0x%lx, not tracking it.\n", win, inside); ff();}
						delete_win(win);
						continue;
					}
				}
				launch(win);
				state[i] = 1;
			}
		} else if (state[i] == 1) {
			if (attr.map_state != IsViewable) {
				stop(win);
				state[i] = 0;
			}
		}
	}
	if (exit_no_app_win && !app_ok) {
		for (i=0; i < AMAX; i++) {
			if (apps[i] != None) {
				fprintf(stdout, "main application window is gone: 0x%lx\n", apps[i]);
			}
		}
		ff();
		appshare_cleanup(0);
	}
	if (last_event_type != PropertyNotify) {
		if (appshare_debug) {fprintf(stderr, "update done.\n"); ff();}
	}
}

static void exiter(char *msg, int rc) {
	fprintf(stderr, "%s", msg);
	ff();
	kill_helper_pid();
	exit(rc);
}

static void set_trackdir(void) {
	char tmp[256];
	struct stat sb;
	if (!strcmp(trackdir, "none")) {
		trackdir = NULL;
		return;
	}
	if (!strcmp(trackdir, "unset")) {
		int fd;
		sprintf(tmp, "%s.XXXXXX", trackpre);
		fd = mkstemp(tmp);
		if (fd < 0) {
			strcat(tmp, ": failed to create file.\n");
			exiter(tmp, 1);
		}
		/* XXX race */
		close(fd);
		unlink(tmp);
		if (mkdir(tmp, 0700) != 0) {
			strcat(tmp, ": failed to create dir.\n");
			exiter(tmp, 1);
		}
		trackdir = strdup(tmp);
	}
	if (stat(trackdir, &sb) != 0) {
		if (mkdir(trackdir, 0700) != 0) {
			exiter("could not make trackdir.\n", 1);
		}
	} else if (! S_ISDIR(sb.st_mode)) {
		exiter("trackdir not a directory.\n", 1);
	}
	tracktmp = (char *) calloc(1000 + strlen(trackdir), 1);
}

static void process_string(char *str) {
	FILE *f;
	char *file;
	if (trackdir) {
		sprintf(tracktmp, "%s/0xprop.cmd", trackdir);
		file = strdup(tracktmp);
	} else {
		char tmp[] = "/tmp/x11vnc-appshare.cmd.XXXXXX";
		int fd = mkstemp(tmp);
		if (fd < 0) {
			return;
		}
		file = strdup(tmp);
		close(fd);
	}
	f = fopen(file, "w");
	if (f) {
		fprintf(f, "%s", str);
		fclose(f);
		process_control(file, 0);
	}
	unlink(file);
	free(file);
}

static void handle_shell(void) {
	struct timeval tv;
	static char lastline[1000];
	static int first = 1;
	fd_set rfds;
	int fd0 = fileno(stdin);

	if (first) {
		memset(lastline, 0, sizeof(lastline));
		first = 0;
	}

	FD_ZERO(&rfds);
	FD_SET(fd0, &rfds);
	tv.tv_sec = 0; 
	tv.tv_usec = 0; 
	select(fd0+1, &rfds, NULL, NULL, &tv);
	if (FD_ISSET(fd0, &rfds)) {
		char line[1000], line2[1010];
		if (fgets(line, sizeof(line), stdin) != NULL) {
			char *str = lblanks(line);
			char *q = strrchr(str, '\n');
			if (q) *q = '\0';
			if (strcmp(str, "")) {
				if (!strcmp(str, "!!")) {
					sprintf(line, "%s", lastline);
					fprintf(stderr, "%s\n", line);
					str = line;
				}
				if (strstr(str, "!") == str) {
					system(str+1);
				} else if (!strcmp(str, "x11vnc") || !strcmp(str, "ps")) {
					char *cmd = "ps -elf | egrep 'PID|x11vnc' | grep -v egrep";
					fprintf(stderr, "%s\n", cmd);
					system(cmd);
				} else {
					sprintf(line2, "cmd=%s", str);
					process_string(line2);
				}
				sprintf(lastline, "%s", str);
			}
		}
		fprintf(stderr, "\n%s", prompt); ff();
	}
}

static void handle_prop_cmd(void) {
	char *value, *str, *done = "DONE";

	if (cmd_atom == None) {
		return;
	}

	value = get_xprop(cmd_atom_str, root);
	if (value == NULL) {
		return;
	}

	str = lblanks(value);
	if (!strcmp(str, done)) {
		free(value);
		return;
	}
	if (strstr(str, "cmd=quit") == str || strstr(str, "\ncmd=quit")) {
		set_xprop(cmd_atom_str, root, done);
		appshare_cleanup(0);
	}

	process_string(str);

	free(value);
	set_xprop(cmd_atom_str, root, done);
}

#define PREFIX if(appshare_debug) fprintf(stderr, "  %8.2f  0x%08lx : ", dnow() - start, ev.xany.window);

static void monitor(void) {
#if !NO_X11
	XEvent ev;
	double start = dnow();
	int got_prop_cmd = 0;

	if (shell) {
		update();
		fprintf(stderr, "\n\n");
		process_string("cmd=help");
		fprintf(stderr, "\n%s", prompt); ff();
	}

	while (1) {
		int t;

		if (XEventsQueued(dpy, QueuedAlready) == 0) {
			update();
			if (got_prop_cmd) {
				handle_prop_cmd();
			}
			got_prop_cmd = 0;
			if (shell) {
				handle_shell();
			}
		}

		XNextEvent(dpy, &ev);

		last_event_type = ev.type;

		switch (ev.type) {
		case Expose:
			PREFIX
			if(appshare_debug) fprintf(stderr, "Expose %04dx%04d+%04d+%04d\n", ev.xexpose.width, ev.xexpose.height, ev.xexpose.x, ev.xexpose.y);
			break;
		case ConfigureNotify:
#if 0
			PREFIX
			if(appshare_debug) fprintf(stderr, "ConfigureNotify %04dx%04d+%04d+%04d  above: 0x%lx\n", ev.xconfigure.width, ev.xconfigure.height, ev.xconfigure.x, ev.xconfigure.y, ev.xconfigure.above);
#endif
			break;
		case VisibilityNotify:
			PREFIX
			if (appshare_debug) {
			fprintf(stderr, "VisibilityNotify: ");
			t = ev.xvisibility.state;
			if (t == VisibilityFullyObscured)     fprintf(stderr, "VisibilityFullyObscured\n");
			if (t == VisibilityPartiallyObscured) fprintf(stderr, "VisibilityPartiallyObscured\n");
			if (t == VisibilityUnobscured)        fprintf(stderr, "VisibilityUnobscured\n");
			}
			break;
		case MapNotify:
			PREFIX
			if(appshare_debug) fprintf(stderr, "MapNotify      win: 0x%lx\n", ev.xmap.window);
			if (ours(ev.xmap.window)) {
				mapped(ev.xmap.window);
			}
			break;
		case UnmapNotify:
			PREFIX
			if(appshare_debug) fprintf(stderr, "UnmapNotify    win: 0x%lx\n", ev.xmap.window);
			if (ours(ev.xmap.window)) {
				unmapped(ev.xmap.window);
			}
			break;
		case MapRequest:
			PREFIX
			if(appshare_debug) fprintf(stderr, "MapRequest\n");
			break;
		case CreateNotify:
			PREFIX
			if(appshare_debug) fprintf(stderr, "CreateNotify parent: 0x%lx  win: 0x%lx\n", ev.xcreatewindow.parent, ev.xcreatewindow.window);
			if (ev.xcreatewindow.parent == root && ours(ev.xcreatewindow.window)) {
				if (find_win(ev.xcreatewindow.window) >= 0) {
					destroy_win(ev.xcreatewindow.window);
				}
				add_win(ev.xcreatewindow.window);
			}
			break;
		case DestroyNotify:
			PREFIX
			if(appshare_debug) fprintf(stderr, "DestroyNotify  win: 0x%lx\n", ev.xdestroywindow.window);
			if (ours(ev.xdestroywindow.window)) {
				destroy_win(ev.xdestroywindow.window);
			}
			break;
		case ConfigureRequest:
			PREFIX
			if(appshare_debug) fprintf(stderr, "ConfigureRequest\n");
			break;
		case CirculateRequest:
#if 0
			PREFIX
			if(appshare_debug) fprintf(stderr, "CirculateRequest parent: 0x%lx  win: 0x%lx\n", ev.xcirculaterequest.parent, ev.xcirculaterequest.window);
#endif
			break;
		case CirculateNotify:
#if 0
			PREFIX
			if(appshare_debug) fprintf(stderr, "CirculateNotify\n");
#endif
			break;
		case PropertyNotify:
#if 0
			PREFIX
			if(appshare_debug) fprintf(stderr, "PropertyNotify\n");
#endif
			if (cmd_atom != None && ev.xproperty.atom == cmd_atom) {
				got_prop_cmd++;
			}
			break;
		case ReparentNotify:
			PREFIX
			if(appshare_debug) fprintf(stderr, "ReparentNotify parent: 0x%lx  win: 0x%lx\n", ev.xreparent.parent, ev.xreparent.window);
			if (ours(ev.xreparent.window)) {
				if (ours(ev.xreparent.parent)) {
					destroy_win(ev.xreparent.window);
				} else if (ev.xreparent.parent == root) {
					/* ??? */
				}
			}
			break;
		default:
			PREFIX
			if(appshare_debug) fprintf(stderr, "Unknown: %d\n", ev.type);
			break;
		}
	}
#endif
}

int appshare_main(int argc, char *argv[]) {
	int i;
	char *app_str = NULL;
	char *dpy_str = NULL;
	long xselectinput = 0;
#if NO_X11
	exiter("not compiled with X11\n", 1);
#else
	for (i=0; i < WMAX; i++) {
		watch[i] = None;
		state[i] = 0;
	}
	for (i=0; i < AMAX; i++) {
		apps[i]  = None;
	}
	for (i=0; i < CMAX; i++) {
		clients[i] = NULL;
	}

	x11vnc = strdup(argv[0]);

	for (i=1; i < argc; i++) {
		int end = (i == argc-1) ? 1 : 0;
		char *s = argv[i];
		if (strstr(s, "--") == s) {
			s++;
		}

		if (!strcmp(s, "-h") || !strcmp(s, "-help")) {
			fprintf(stdout, "%s", usage);
			exit(0);
		} else if (!strcmp(s, "-id")) {
			id_opt = "-id";
			if (end) exiter("no -id value supplied\n", 1);
			app_str = strdup(argv[++i]);
		} else if (!strcmp(s, "-sid")) {
			id_opt = "-sid";
			if (end) exiter("no -sid value supplied\n", 1);
			app_str = strdup(argv[++i]);
		} else if (!strcmp(s, "-connect") || !strcmp(s, "-connect_or_exit") || !strcmp(s, "-coe")) {
			if (end) exiter("no -connect value supplied\n", 1);
			connect_to = strdup(argv[++i]);
		} else if (!strcmp(s, "-control")) {
			if (end) exiter("no -control value supplied\n", 1);
			control = strdup(argv[++i]);
			if (!strcmp(control, "shell")) {
				free(control);
				control = strdup("internal");
				shell = 1;
			}
		} else if (!strcmp(s, "-trackdir")) {
			if (end) exiter("no -trackdir value supplied\n", 1);
			trackdir = strdup(argv[++i]);
		} else if (!strcmp(s, "-display")) {
			if (end) exiter("no -display value supplied\n", 1);
			dpy_str = strdup(argv[++i]);
			set_env("DISPLAY", dpy_str);
		} else if (!strcmp(s, "-delay")) {
			if (end) exiter("no -delay value supplied\n", 1);
			helper_delay = atof(argv[++i]);
		} else if (!strcmp(s, "-args")) {
			if (end) exiter("no -args value supplied\n", 1);
			x11vnc_args = strdup(argv[++i]);
		} else if (!strcmp(s, "-env")) {
			if (end) exiter("no -env value supplied\n", 1);
			putenv(argv[++i]);
		} else if (!strcmp(s, "-debug")) {
			appshare_debug++;
		} else if (!strcmp(s, "-showmenus")) {
			skip_menus = 0;
		} else if (!strcmp(s, "-noexit")) {
			exit_no_app_win = 0;
		} else if (!strcmp(s, "-shell")) {
			shell = 1;
		} else if (!strcmp(s, "-nocmds") || !strcmp(s, "-safer")) {
			fprintf(stderr, "ignoring %s in -appshare mode.\n", s);
		} else if (!strcmp(s, "-appshare")) {
			;
		} else {
			fprintf(stderr, "unrecognized 'x11vnc -appshare' option: %s\n", s);
			exiter("", 1);
		}
	}

	if (getenv("X11VNC_APPSHARE_DEBUG")) {
		appshare_debug = atoi(getenv("X11VNC_APPSHARE_DEBUG"));
	}

	/* let user override name for multiple instances: */
	if (getenv("X11VNC_APPSHARE_COMMAND_PROPNAME")) {
		cmd_atom_str = strdup(getenv("X11VNC_APPSHARE_COMMAND_PROPNAME"));
	}
	if (getenv("X11VNC_APPSHARE_TICKER_PROPNAME")) {
		ticker_atom_str = strdup(getenv("X11VNC_APPSHARE_TICKER_PROPNAME"));
	}

	if (shell) {
		if (!control || strcmp(control, "internal")) {
			exiter("mode -shell requires '-control internal'\n", 1);
		}
	}

	if (connect_to == NULL && control != NULL) {
		struct stat sb;
		if (stat(control, &sb) == 0) {
			int len = 100 + sb.st_size;
			FILE *f = fopen(control, "r");

			if (f) {
				char *line = (char *) malloc(len);
				connect_to = (char *) calloc(2 * len, 1);
				while (fgets(line, len, f) != NULL) {
					char *q = strchr(line, '\n');
					if (q) *q = '\0';
					q = lblanks(line);
					if (q[0] == '#') {
						continue;
					}
					if (connect_to[0] != '\0') {
						strcat(connect_to, ",");
					}
					strcat(connect_to, q);
				}
				fclose(f);
			}
			fprintf(stderr, "set -connect to: %s\n", connect_to);
		}
	}
	if (0 && connect_to == NULL && control == NULL) {
		exiter("no -connect host or -control file specified.\n", 1);
	}

	if (control) {
		pid_t pid;
		parent_pid = getpid();
		pid = fork();
		if (pid == (pid_t) -1) {
			;
		} else if (pid == 0) {
			be_helper_pid(dpy_str);
			exit(0);
		} else {
			helper_pid = pid;
		}
	}

	dpy = XOpenDisplay(dpy_str);
	if (!dpy) {
		exiter("cannot open display\n", 1);
	}

	root = DefaultRootWindow(dpy);

	xselectinput = SubstructureNotifyMask;
	if (helper_pid > 0) {
		ticker_atom = XInternAtom(dpy, ticker_atom_str, False);
		xselectinput |= PropertyChangeMask;
	}
	XSelectInput(dpy, root, xselectinput);

	cmd_atom = XInternAtom(dpy, cmd_atom_str, False);

	init_cmask();

	sprintf(unique_tag, "X11VNC_APPSHARE_TAG=%d-tag", getpid());

	start_time = dnow();

	if (app_str == NULL) {
		exiter("no -id/-sid window specified.\n", 1);
	} else {
		char *p, *str = strdup(app_str);
		char *alist[AMAX];
		int i, n = 0;

		p = strtok(str, ",");
		while (p) {
			if (n >= AMAX) {
				fprintf(stderr, "ran out of app slots: %s\n", app_str);
				exiter("", 1);
			}
			alist[n++] = strdup(p);
			p = strtok(NULL, ",");
		}
		free(str);

		for (i=0; i < n; i++) {
			Window app = None;
			p = alist[i];
			app = parse_win(p);
			free(p);

			if (app != None) {
				if (!ours(app)) {
					add_app(app);
				}
			}
		}
	}

	set_trackdir();

	signal(SIGINT,  appshare_cleanup);
	signal(SIGTERM, appshare_cleanup);

	rfbLogEnable(0);

	if (connect_to) {
		char *p, *str = strdup(connect_to);
		int n = 0;
		p = strtok(str, ",");
		while (p) {
			clients[n++] = strdup(p);
			p = strtok(NULL, ",");
		}
		free(str);
	} else {
		connect_to = strdup("");
	}

	for (i=0; i < AMAX; i++) {
		if (apps[i] == None) {
			continue;
		}
		fprintf(stdout, "Using app win: 0x%08lx  root: 0x%08lx\n", apps[i], root);
	}
	fprintf(stdout, "\n");

	monitor();

	appshare_cleanup(0);

#endif
	return 0;
}

