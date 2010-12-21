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

/* -- xevents.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xkb_bell.h"
#include "xrandr.h"
#include "xdamage.h"
#include "xrecord.h"
#include "selection.h"
#include "keyboard.h"
#include "cursor.h"
#include "gui.h"
#include "connections.h"
#include "unixpw.h"
#include "cleanup.h"
#include "macosx.h"
#include "screen.h"
#include "pm.h"
#include "pointer.h"
#include "remote.h"
#include "inet.h"

/* XXX CHECK BEFORE RELEASE */
int grab_buster = 0;
int grab_kbd = 0;
int grab_ptr = 0;
int grab_always = 0;
int ungrab_both = 0;
int grab_local = 0;
int sync_tod_delay = 20;

void initialize_vnc_connect_prop(void);
void initialize_x11vnc_remote_prop(void);
void initialize_clipboard_atom(void);
void spawn_grab_buster(void);
void sync_tod_with_servertime(void);
void check_keycode_state(void);
void check_autorepeat(void);
void set_prop_atom(Atom atom);
void check_xevents(int reset);
void xcut_receive(char *text, int len, rfbClientPtr cl);

void kbd_release_all_keys(rfbClientPtr cl);
void set_single_window(rfbClientPtr cl, int x, int y);
void set_server_input(rfbClientPtr cl, int s);
void set_text_chat(rfbClientPtr cl, int l, char *t);
int get_keyboard_led_state_hook(rfbScreenInfoPtr s);
int get_file_transfer_permitted(rfbClientPtr cl);
void get_prop(char *str, int len, Atom prop, Window w);
int guess_dm_gone(int t1, int t2);

static void initialize_xevents(int reset);
static void print_xevent_bases(void);
static void bust_grab(int reset);
static int process_watch(char *str, int parent, int db);
static void grab_buster_watch(int parent, char *dstr);


void initialize_vnc_connect_prop(void) {
	char *prop_str;
	vnc_connect_str[0] = '\0';
	RAWFB_RET_VOID
#if !NO_X11
	prop_str = getenv("VNC_CONNECT");
	if (prop_str == NULL) {
		prop_str = "VNC_CONNECT";
	}
	vnc_connect_prop = XInternAtom(dpy, "VNC_CONNECT", False);
#endif
}

void initialize_x11vnc_remote_prop(void) {
	char *prop_str;
	x11vnc_remote_str[0] = '\0';
	RAWFB_RET_VOID
#if !NO_X11
	prop_str = getenv("X11VNC_REMOTE");
	if (prop_str == NULL) {
		prop_str = "X11VNC_REMOTE";
	}
	x11vnc_remote_prop = XInternAtom(dpy, prop_str, False);
#endif
}

void initialize_clipboard_atom(void) {
	RAWFB_RET_VOID
#if NO_X11
	return;
#else
	clipboard_atom = XInternAtom(dpy, "CLIPBOARD", False);
	if (clipboard_atom == None) {
		if (! quiet) rfbLog("could not find atom CLIPBOARD\n");
		if (watch_clipboard) {
			watch_clipboard = 0;
		}
		if (set_clipboard) {
			set_clipboard = 0;
		}
	}
#endif	/* NO_X11 */
}

/*
      we observed these strings:

      6 gdm_string: Gnome-power-manager
      6 gdm_string: Gnome-session
      6 gdm_string: Gnome-settings-daemon
      6 gdm_string: Login Window
      6 gdm_string: Notify-osd
      6 gdm_string: Panel
     12 gdm_string: Metacity
     12 gdm_string: gnome-power-manager
     12 gdm_string: gnome-session
     12 gdm_string: gnome-settings-daemon
     12 gdm_string: notify-osd
     18 gdm_string: Gdm-simple-greeter
     24 gdm_string: metacity
     36 gdm_string: gdm-simple-greeter

	kdmgreet
	Kdmgreet
 */

static int dm_string(char *str) {
	char *s = getenv("DEBUG_WM_RUNNING");
	if (str == NULL) {
		return 0;
	}
	if (str[0] == '\0') {
		return 0;
	}
	if (0) fprintf(stderr, "dm_string: %s\n", str);
	if (strstr(str, "gdm-") == str || strstr(str, "Gdm-") == str) {
		if (strstr(str, "-greeter") != NULL) {
			if (s) rfbLog("dm_string: %s\n", str);
			return 1;
		}
	}
	if (!strcmp(str, "kdmgreet") || !strcmp(str, "Kdmgreet")) {
		if (s) rfbLog("dm_string: %s\n", str);
		return 1;
	}
	return 0;
}

static int dm_still_running(void) {
#if NO_X11
	return 0;
#else
	Window r, parent;
	Window *winlist;
	unsigned int nc;
	int rc, i;
	static XClassHint *classhint = NULL;
	XErrorHandler old_handler;
	int saw_gdm_name = 0;

	/* some times a window can go away before we get to it */
	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);

	if (! classhint) {
		classhint = XAllocClassHint();
	}

	/* we are xlocked. */
	rc = XQueryTree_wr(dpy, DefaultRootWindow(dpy), &r, &parent, &winlist, &nc);
	if (!rc || winlist == NULL || nc == 0) {
		nc = 0;
	}
	for (i=0; i < (int) nc; i++) {
		char *name = NULL;
		Window w = winlist[i];
		if (XFetchName(dpy, w, &name) && name != NULL) {
			saw_gdm_name += dm_string(name);
			XFree_wr(name);
		}
		classhint->res_name = NULL;
		classhint->res_class = NULL;
		if (XGetClassHint(dpy, w, classhint)) {
			name = classhint->res_name;
			if (name != NULL) {
				saw_gdm_name += dm_string(name);
				XFree_wr(name);
			}
			name = classhint->res_class;
			if (name != NULL) {
				saw_gdm_name += dm_string(name);
				XFree_wr(name);
			}
		}
		if (saw_gdm_name > 0) {
			break;
		}
	}
	if (winlist != NULL) {
		XFree_wr(winlist);
	}

	XSync(dpy, False);
	XSetErrorHandler(old_handler);
	trapped_xerror = 0;

	return saw_gdm_name;
#endif
}

static int wm_running(void) {
	char *s = getenv("DEBUG_WM_RUNNING");
	int ret = 0;
	RAWFB_RET(0)
#if NO_X11
	return 0;
#else
	/*
	 * Unfortunately with recent GDM (v2.28), they run gnome-session,
	 * dbus-launch, and metacity for the Login greeter!  So the simple
	 * XInternAtom checks below no longer work.
         * We also see a similar thing with KDE.
	 */
	if (dm_still_running()) {
		return 0;
	}

	/* we are xlocked. */
	if (XInternAtom(dpy, "_NET_SUPPORTED", True) != None) {
		if (s) rfbLog("wm is running (_NET_SUPPORTED).\n");
		ret++;
	}
	if (XInternAtom(dpy, "_WIN_PROTOCOLS", True) != None) {
		if (s) rfbLog("wm is running (_WIN_PROTOCOLS).\n");
		ret++;
	}
	if (XInternAtom(dpy, "_XROOTPMAP_ID", True) != None) {
		if (s) rfbLog("wm is running (_XROOTPMAP_ID).\n");
		ret++;
	}
	if (XInternAtom(dpy, "_MIT_PRIORITY_COLORS", True) != None) {
		if (s) rfbLog("wm is running (_MIT_PRIORITY_COLORS).\n");
		ret++;
	}
	if (!ret) {
		if (s) rfbLog("wm is not running.\n");
		return 0;
	} else {
		if (s) rfbLog("wm is running ret=%d.\n", ret);
		return 1;
	}
#endif	/* NO_X11 */
	
}

int guess_dm_gone(int t1, int t2) {
	int wait = t2;
	char *avoid = getenv("X11VNC_AVOID_WINDOWS");
	time_t tcheck = last_client;

	if (last_open_xdisplay > last_client) {
		/* better time for -display WAIT:... */
		tcheck = last_open_xdisplay;
	}

	if (avoid && !strcmp(avoid, "never")) {
		return 1;
	}
	if (!screen || !screen->clientHead) {
		return 0;
	}
	if (avoid) {
		int n = atoi(avoid);
		if (n > 1) {
			wait = n;
		} else {
			wait = 90;
		}
	} else {
		static time_t saw_wm = 0;

		wait = t2;

		X_LOCK;
		if (wm_running()) {
			if (saw_wm == 0) {
				saw_wm = time(NULL);
			} else if (time(NULL) <= saw_wm + 2) {
				/* try to wait a few seconds after transition */
				;
			} else {
				wait = t1;
			}
		}
		X_UNLOCK;
	}
	if (getenv("DEBUG_WM_RUNNING")) {
		rfbLog("guess_dm_gone: wait=%d\n", wait);
	}
	/* we assume they've logged in OK after wait seconds... */
	if (time(NULL) <= tcheck + wait)  {
		return 0;
	}
	return 1;
}

static void initialize_xevents(int reset) {
#if NO_X11
	RAWFB_RET_VOID
	if (!reset) {}
	return;
#else
	static int did_xselect_input = 0;
	static int did_xcreate_simple_window = 0;
	static int did_vnc_connect_prop = 0;
	static int did_x11vnc_remote_prop = 0;
	static int did_clipboard_atom = 0;
	static int did_xfixes = 0;
	static int did_xdamage = 0;
	static int did_xrandr = 0;

	RAWFB_RET_VOID

	if (reset) {
		did_xselect_input = 0;
		did_xcreate_simple_window = 0;
		did_vnc_connect_prop = 0;
		did_x11vnc_remote_prop = 0;
		did_clipboard_atom = 0;
		did_xfixes = 0;
		did_xdamage = 0;
		did_xrandr = 0;
	}

	if ((watch_selection || vnc_connect) && !did_xselect_input) {
		/*
		 * register desired event(s) for notification.
		 * PropertyChangeMask is for CUT_BUFFER0 changes.
		 * XXX: does this cause a flood of other stuff?
		 */
		X_LOCK;
		xselectinput_rootwin |= PropertyChangeMask;
		XSelectInput_wr(dpy, rootwin, xselectinput_rootwin);

		if (subwin && freeze_when_obscured) {
			XSelectInput_wr(dpy, subwin, VisibilityChangeMask);
		}
		X_UNLOCK;
		did_xselect_input = 1;
	}
	if (watch_selection && !did_xcreate_simple_window) {
		/* create fake window for our selection ownership, etc */

		/*
		 * We try to delay creating selwin until we are past
		 * any GDM, (or other KillInitClients=true) manager.
		 */
		if (guess_dm_gone(8, 45)) {
			X_LOCK;
			selwin = XCreateSimpleWindow(dpy, rootwin, 3, 2, 1, 1, 0, 0, 0);
			X_UNLOCK;
			did_xcreate_simple_window = 1;
			if (! quiet) rfbLog("created selwin: 0x%lx\n", selwin);
		}
	}

	if ((xrandr || xrandr_maybe) && !did_xrandr) {
		initialize_xrandr();
		did_xrandr = 1;
	}
	if (vnc_connect && !did_vnc_connect_prop) {
		initialize_vnc_connect_prop();
		did_vnc_connect_prop = 1;
	}
	if (vnc_connect && !did_x11vnc_remote_prop) {
		initialize_x11vnc_remote_prop();
		did_x11vnc_remote_prop = 1;
	}
	if (run_gui_pid > 0) {
		kill(run_gui_pid, SIGUSR1);
		run_gui_pid = 0;
	}
	if (!did_clipboard_atom) {
		initialize_clipboard_atom();
		did_clipboard_atom = 1;
	}
	if (xfixes_present && use_xfixes && !did_xfixes) {
		/*
		 * We try to delay creating initializing xfixes until
		 * we are past the display manager, due to Xorg bug:
		 * http://bugs.freedesktop.org/show_bug.cgi?id=18451
		 */
		if (guess_dm_gone(8, 45)) {
			initialize_xfixes();
			did_xfixes = 1;
			if (! quiet) rfbLog("called initialize_xfixes()\n");
		}
	}
	if (xdamage_present && !did_xdamage) {
		initialize_xdamage();
		did_xdamage = 1;
	}
#endif	/* NO_X11 */
}

static void print_xevent_bases(void) {
	fprintf(stderr, "X event bases: xkb=%d, xtest=%d, xrandr=%d, "
	    "xfixes=%d, xdamage=%d, xtrap=%d\n", xkb_base_event_type,
	    xtest_base_event_type, xrandr_base_event_type,
	    xfixes_base_event_type, xdamage_base_event_type,
	    xtrap_base_event_type);
	fprintf(stderr, "  MapNotify=%d, ClientMsg=%d PropNotify=%d "
	    "SelNotify=%d, SelRequest=%d\n", MappingNotify, ClientMessage,
	    PropertyNotify, SelectionNotify, SelectionRequest);
	fprintf(stderr, "  SelClear=%d, Expose=%d\n", SelectionClear, Expose);
}

void get_prop(char *str, int len, Atom prop, Window w) {
	int i;
#if !NO_X11
	Atom type;
	int format, slen, dlen;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;
#endif

	for (i=0; i<len; i++) {
		str[i] = '\0';
	}
	if (prop == None) {
		return;
	}

	RAWFB_RET_VOID

#if NO_X11
	return;
#else

	slen = 0;
	if (w == None) {
		w = DefaultRootWindow(dpy);
	}

	do {
		if (XGetWindowProperty(dpy, w,
		    prop, nitems/4, len/16, False,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > len) {
				/* too big */
				XFree_wr(data);
				break;
			}
			memcpy(str+slen, data, dlen);
			slen += dlen;
			str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);
#endif	/* NO_X11 */
}

static void bust_grab(int reset) {
#if NO_X11
	if (!reset) {}
	return;
#else
	static int bust_count = 0;
	static time_t last_bust = 0;
	time_t now = time(NULL);
	KeyCode key;
	int button, x, y, nb;

	if (now > last_bust + 180) {
		bust_count = 0;
	}
	if (reset) {
		bust_count = 0;
		return;
	}

	x = 0;
	y = 0;
	button = 0;
	key = NoSymbol;

	nb = 8;
	if (bust_count >= 3 * nb)  {
		fprintf(stderr, "too many bust_grab's %d for me\n", bust_count);
		exit(0);
	}
	if (bust_count % nb == 0) {
		button = 1;
	} else if (bust_count % nb == 1) {
		button = 1;
	} else if (bust_count % nb == 2) {
		key = XKeysymToKeycode(dpy, XK_Escape);
	} else if (bust_count % nb == 3) {
		button = 3;
	} else if (bust_count % nb == 4) {
		key = XKeysymToKeycode(dpy, XK_space);
	} else if (bust_count % nb == 5) {
		x = bust_count * 23;
		y = bust_count * 17;
	} else if (bust_count % nb == 5) {
		button = 2;
	} else if (bust_count % nb == 6) {
		key = XKeysymToKeycode(dpy, XK_a);
	}

	if (key == NoSymbol) {
		key = XKeysymToKeycode(dpy, XK_a);
		if (key == NoSymbol) {
			button = 1;
		}
	}

	bust_count++;

	if (button) {
		/* try button press+release */
		fprintf(stderr, "**bust_grab: button%d  %.4f\n",
		    button, dnowx());
		XTestFakeButtonEvent_wr(dpy, button, True, CurrentTime);
		XFlush_wr(dpy);
		usleep(50 * 1000);
		XTestFakeButtonEvent_wr(dpy, button, False, CurrentTime);
	} else if (x > 0) {
		/* try button motion*/
		int scr = DefaultScreen(dpy);

		fprintf(stderr, "**bust_grab: x=%d y=%d  %.4f\n", x, y,
		    dnowx());
		XTestFakeMotionEvent_wr(dpy, scr, x, y, CurrentTime);
		XFlush_wr(dpy);
		usleep(50 * 1000);

		/* followed by button press */
		button = 1;
		fprintf(stderr, "**bust_grab: button%d\n", button);
		XTestFakeButtonEvent_wr(dpy, button, True, CurrentTime);
		XFlush_wr(dpy);
		usleep(50 * 1000);
		XTestFakeButtonEvent_wr(dpy, button, False, CurrentTime);
	} else {
		/* try Escape or Space press+release */
		fprintf(stderr, "**bust_grab: keycode: %d  %.4f\n",
		    (int) key, dnowx());
		XTestFakeKeyEvent_wr(dpy, key, True, CurrentTime);
		XFlush_wr(dpy);
		usleep(50 * 1000);
		XTestFakeKeyEvent_wr(dpy, key, False, CurrentTime);
	}
	XFlush_wr(dpy);
	last_bust = time(NULL);
#endif	/* NO_X11 */
}

typedef struct _grabwatch {
	int pid;
	int tick;
	unsigned long time;
	time_t change;
} grabwatch_t;
#define GRABWATCH 16

static int grab_buster_delay = 20;
static pid_t grab_buster_pid = 0;

static int grab_npids = 1;

static int process_watch(char *str, int parent, int db) {
	int i, pid, ticker, npids;
	char diff[128];
	unsigned long xtime;
	static grabwatch_t watches[GRABWATCH];
	static int first = 1;
	time_t now = time(NULL);
	static time_t last_bust = 0;
	int too_long, problems = 0;

	if (first) {
		for (i=0; i < GRABWATCH; i++) {
			watches[i].pid = 0;
			watches[i].tick = 0;
			watches[i].time = 0;
			watches[i].change = 0;
		}
		first = 0;
	}

	/* record latest value of prop */
	if (str && *str != '\0') {
		if (sscanf(str, "%d/%d/%lu/%s", &pid, &ticker, &xtime, diff)
		    == 4) {
			int got = -1, free = -1;

			if (db) fprintf(stderr, "grab_buster %d - %d - %lu - %s"
			    "\n", pid, ticker, xtime, diff);

			if (pid == parent && !strcmp(diff, "QUIT")) {
				/* that's it. */
				return 0;
			}
			if (pid == 0 || ticker == 0 || xtime == 0) {
				/* bad prop read. */
				goto badtickerstr;
			}
			for (i=0; i < GRABWATCH; i++) {
				if (watches[i].pid == pid) {
					got = i;
					break;
				}
				if (free == -1 && watches[i].pid == 0) {
					free = i;
				}
			}
			if (got == -1) {
				if (free == -1) {
					/* bad news */;
					free = GRABWATCH - 1;
				}
				watches[free].pid  = pid;
				watches[free].tick = ticker;
				watches[free].time = xtime;
				watches[free].change = now;
				if (db) fprintf(stderr, "grab_buster free slot: %d\n", free);
			} else {
				if (db) fprintf(stderr, "grab_buster got  slot: %d\n", got);
				if (watches[got].tick != ticker) {
					watches[got].change = now;
				}
				if (watches[got].time != xtime) {
					watches[got].change = now;
				}
				watches[got].tick = ticker;
				watches[got].time = xtime;
			}
		} else {
			if (db) fprintf(stderr, "grab_buster bad prop str: %s\n", str);
		}
	}

	badtickerstr:

	too_long = grab_buster_delay;
	if (too_long < 3 * sync_tod_delay) {
		too_long = 3 * sync_tod_delay;
	}

	npids = 0;
	for (i=0; i < GRABWATCH; i++) {
		if (watches[i].pid) {
			npids++;
		}
	}
	grab_npids = npids;
	if (npids > 4) {
		npids = 4;
	}

	/* now check everyone we are tracking */
	for (i=0; i < GRABWATCH; i++) {
		int fac = 1;
		if (!watches[i].pid) {
			continue;
		}
		if (watches[i].change == 0) {
			watches[i].change = now;	/* just to be sure */
			continue;
		}

		pid = watches[i].pid;

		if (pid != parent) {
			fac = 2;
		}
		if (npids > 0) {
			fac *= npids;
		}

		if (now > watches[i].change + fac*too_long) {
			int process_alive = 1;

			fprintf(stderr, "grab_buster: problem with pid: "
			    "%d - %d/%d/%d\n", pid, (int) now,
			    (int) watches[i].change, too_long);

			if (kill((pid_t) pid, 0) != 0) {
				if (1 || errno == ESRCH) {
					process_alive = 0;	
				}
			}

			if (!process_alive) {
				watches[i].pid = 0;
				watches[i].tick = 0;
				watches[i].time = 0;
				watches[i].change = 0;
				fprintf(stderr, "grab_buster: pid gone: %d\n",
				    pid);
				if (pid == parent) {
					/* that's it */
					return 0;
				}
			} else {
				int sleep = sync_tod_delay * 1000 * 1000;

				bust_grab(0);
				problems++;
				last_bust = now;
				usleep(1 * sleep);
				break;
			}
		}
	}

	if (!problems) {
		bust_grab(1);
	}
	return 1;
}

static void grab_buster_watch(int parent, char *dstr) {
#if NO_X11
	RAWFB_RET_VOID
	if (!parent || !dstr) {}
	return;
#else
	Atom ticker_atom = None;
	int sleep = sync_tod_delay * 921 * 1000;
	char propval[200];
	int ev, er, maj, min;
	int db = 0;
	char *ticker_str = "X11VNC_TICKER";

	RAWFB_RET_VOID

	if (grab_buster > 1) {
		db = 1;
	}

	/* overwrite original dpy, we let orig connection sit unused. */
	dpy = XOpenDisplay_wr(dstr);
	if (!dpy) {
		fprintf(stderr, "grab_buster_watch: could not reopen: %s\n",
		    dstr);
		return;
	}
	rfbLogEnable(0);

	/* check for XTEST, etc, and then disable grabs for us */
	if (! XTestQueryExtension_wr(dpy, &ev, &er, &maj, &min)) {
		xtest_present = 0;
	} else {
		xtest_present = 1;
	}
	if (! XETrapQueryExtension_wr(dpy, &ev, &er, &maj)) {
		xtrap_present = 0;
	} else {
		xtrap_present = 1;
	}

	if (! xtest_present && ! xtrap_present) {
		fprintf(stderr, "grab_buster_watch: no grabserver "
		    "protection on display: %s\n", dstr);
		return;
	}
	disable_grabserver(dpy, 0);

	usleep(3 * sleep);

	if (getenv("X11VNC_TICKER")) {
		ticker_str = getenv("X11VNC_TICKER");
	}
	ticker_atom = XInternAtom(dpy, ticker_str, False);
	if (! ticker_atom) {
		fprintf(stderr, "grab_buster_watch: no ticker atom\n");
		return;
	}

	while(1) {
		int slp = sleep;
		if (grab_npids > 1) {
			slp = slp / 8;	
		}
		usleep(slp);
		usleep((int) (0.60 * rfac() * slp));

		if (kill((pid_t) parent, 0) != 0) {
			break;
		}

		get_prop(propval, 128, ticker_atom, None);
		if (db) fprintf(stderr, "got_prop:   %s\n", propval);

		if (!process_watch(propval, parent, db)) {
			break;
		}
	}
#endif	/* NO_X11 */
}

void spawn_grab_buster(void) {
#if LIBVNCSERVER_HAVE_FORK
	pid_t pid;
	int parent = (int) getpid();
	char *dstr = strdup(DisplayString(dpy));

	RAWFB_RET_VOID

	XCloseDisplay_wr(dpy); 
	dpy = NULL;

	if ((pid = fork()) > 0) {
		grab_buster_pid = pid;
		if (! quiet) {
			rfbLog("grab_buster pid is: %d\n", (int) pid);
		}
	} else if (pid == -1) {
		fprintf(stderr, "spawn_grab_buster: could not fork\n");
		rfbLogPerror("fork");
	} else {
		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		grab_buster_watch(parent, dstr);
		exit(0);
	}

	dpy = XOpenDisplay_wr(dstr);
	if (!dpy) {
		rfbLog("failed to reopen display %s in spawn_grab_buster\n",
		    dstr);
		exit(1);
	}
#endif
}

void sync_tod_with_servertime(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	static Atom ticker_atom = None;
	XEvent xev;
	char diff[128];
	static int seq = 0;
	static unsigned long xserver_ticks = 1;
	int i, db = 0;

	RAWFB_RET_VOID

	if (atom_NET_ACTIVE_WINDOW == None) {
		atom_NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
	}
	if (atom_NET_CURRENT_DESKTOP == None) {
		atom_NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", True);
	}
	if (atom_NET_CLIENT_LIST_STACKING == None) {
		atom_NET_CLIENT_LIST_STACKING = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", True);
	}
	if (atom_XROOTPMAP_ID == None) {
		atom_XROOTPMAP_ID = XInternAtom(dpy, "_XROOTPMAP_ID", True);
	}

	if (! ticker_atom) {
		char *ticker_str = "X11VNC_TICKER";
		if (getenv("X11VNC_TICKER")) {
			ticker_str = getenv("X11VNC_TICKER");
		}
		ticker_atom = XInternAtom(dpy, ticker_str, False);
	}
	if (! ticker_atom) {
		return;
	}

	XSync(dpy, False);
	while (XCheckTypedEvent(dpy, PropertyNotify, &xev)) {
		set_prop_atom(xev.xproperty.atom);
	}

	snprintf(diff, 128, "%d/%08d/%lu/%.6f", (int) getpid(), seq++,
	    xserver_ticks, servertime_diff); 
	XChangeProperty(dpy, rootwin, ticker_atom, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) diff, strlen(diff));
	XSync(dpy, False);

	for (i=0; i < 10; i++) {
		int k, got = 0;
		
		for (k=0; k < 5; k++) {
			while (XCheckTypedEvent(dpy, PropertyNotify, &xev)) {
				if (xev.xproperty.atom == ticker_atom) {
					double stime;
					
					xserver_ticks = xev.xproperty.time;
					stime = (double) xev.xproperty.time;
					stime = stime/1000.0;
					servertime_diff = dnow() - stime;
					if (db) rfbLog("set servertime_diff: "
					    "%.6f\n", servertime_diff);
					got = 1;
				}
			}
		}
		if (got) {
			break;
		}
		usleep(1000);
	}
#endif	/* NO_X11 */
}

void check_keycode_state(void) {
	static time_t last_check = 0;
	int delay = 10, noinput = 3;
	time_t now = time(NULL);

	if (! client_count) {
		return;
	}
	if (unixpw_in_progress) return;

	RAWFB_RET_VOID

	/*
	 * periodically update our model of the keycode_state[]
	 * by correlating with the Xserver.  wait for a pause in
	 * keyboard input to be on the safe side.  the idea here
	 * is to remove stale keycode state, not to be perfectly
	 * in sync with the Xserver at every instant of time.
	 */
	if (now > last_check + delay && now > last_keyboard_input + noinput) {
		X_LOCK;
		init_track_keycode_state();
		X_UNLOCK;
		last_check = now;
	}
}

/*
 * To use the experimental -grablocal option configure like this:
 * env CPPFLAGS=-DENABLE_GRABLOCAL LDFLAGS=-lXss ./configure
 */
#ifdef ENABLE_GRABLOCAL
#include <X11/extensions/scrnsaver.h>

void check_local_grab(void) {
	static double last_check = 0.0;
	double now;

	if (grab_local <= 0) {
		return;
	}
	if (! client_count) {
		return;
	}
	if (unixpw_in_progress) return;

	if (last_rfb_key_injected <= 0.0 && last_rfb_ptr_injected <= 0.0) {
		return;
	}

	RAWFB_RET_VOID

	now = dnow();

	if (now > last_check + 0.1)   {
#if !NO_X11
		int ret;
		double idle;
		XScreenSaverInfo info;
		static int save_viewonly = -1, local_is_idle = -1, db = -1;

		if (debug_keyboard) db = debug_keyboard;
		if (debug_pointer ) db = debug_pointer;

		if (db < 0) {
			if (getenv("LOCAL_GRAB_DEBUG")) {
				db = atoi(getenv("LOCAL_GRAB_DEBUG"));
			} else {
				db = 0;
			}
		}

		ret = XScreenSaverQueryInfo(dpy, RootWindowOfScreen(
		    ScreenOfDisplay(dpy, 0)), &info);

		if (ret) {
			double tlatest_rfb = 0.0;

			idle = ((double) info.idle)/1000.0;
			now = dnow();

			if (last_rfb_key_injected > 0.0) {
				tlatest_rfb = last_rfb_key_injected;
			}
			if (last_rfb_ptr_injected > tlatest_rfb) {
				tlatest_rfb = last_rfb_ptr_injected;
			}
			if (db > 1) fprintf(stderr, "idle: %.4f latest: %.4f dt: %.4f\n", idle, now - tlatest_rfb, idle - (now - tlatest_rfb));

			if (now - tlatest_rfb <= idle + 0.005) {
				/* 0.005 is 5ms tolerance */
			} else if (idle < grab_local) {
				if (local_is_idle < 0 || local_is_idle) {
					save_viewonly = view_only;
					view_only = 1;
					if (db) {
						rfbLog("check_local_grab: set viewonly\n");
					}
				}
				
				local_is_idle = 0;
			} else {
				if (!local_is_idle && save_viewonly >= 0) {
					view_only = save_viewonly;
					if (db) {
						rfbLog("check_local_grab: restored viewonly; %d\n", view_only);
					}
				}
				local_is_idle = 1;
			}
		}
#endif
		last_check = dnow();
	}
}
#endif

void check_autorepeat(void) {
	static time_t last_check = 0;
	static int idle_timeout = -300, idle_reset = 0;
	time_t now = time(NULL);
	int autorepeat_is_on, autorepeat_initially_on;

	if (! no_autorepeat || ! client_count) {
		return;
	}
	if (now <= last_check + 1) {
		return;
	}

	if (unixpw_in_progress) return;

	if (idle_timeout < 0) {
		if (getenv("X11VNC_IDLE_TIMEOUT")) {
			idle_timeout = atoi(getenv("X11VNC_IDLE_TIMEOUT"));
		}
		if (idle_timeout < 0) {
			idle_timeout = -idle_timeout;
		}
	}

	last_check = now;

	autorepeat_is_on = get_autorepeat_state();
	autorepeat_initially_on = get_initial_autorepeat_state();

	if (view_only) {
		if (! autorepeat_is_on) {
			autorepeat(1, 1);
		}
		return;
	}

	if (now > last_keyboard_input + idle_timeout) {
		/* autorepeat should be on when idle */
		if (! autorepeat_is_on && autorepeat_initially_on) {
			static time_t last_msg = 0;
			static int cnt = 0;
			if (now > last_msg + idle_timeout && cnt++ < 10) {
				rfbLog("idle keyboard:   turning X autorepeat"
				    " back on.\n");
				last_msg = now;
			}
			autorepeat(1, 1);
			idle_reset = 1;
		}
	} else {
		if (idle_reset) {
			int i, state[256], didmsg = 0, pressed = 0;
			int mwt = 600, mmax = 20;
			static int msgcnt = 0;
			static double lastmsg = 0.0;

			for (i=0; i<256; i++) {
				state[i] = 0;
			}
			if (use_threads) {X_LOCK;}
			get_keystate(state);
			if (use_threads) {X_UNLOCK;}

			for (i=0; i<256; i++) {
				if (state[i] != 0) {
					/* better wait until all keys are up  */
					pressed++;
					if (msgcnt < mmax || dnow() > lastmsg + mwt) {
						char *str = "unset";
#if !NO_X11
						if (use_threads) {X_LOCK;}
						str = XKeysymToString(XKeycodeToKeysym(dpy, i, 0));
						if (use_threads) {X_UNLOCK;}
#endif
						str = str ? str : "nosymbol";
						didmsg++;
						rfbLog("active keyboard: waiting until "
						    "all keys are up. key_down=%d %s.  "
						    "If the key is inaccessible via keyboard, "
						    "consider 'x11vnc -R clear_all'\n", i, str);
					}
				}
			}
			if (didmsg > 0) {
				msgcnt++;
				if (msgcnt == mmax) {
					rfbLog("active keyboard: last such "
					    "message for %d secs.\n", mwt);
				}
				lastmsg = dnow();
			}
			if (pressed > 0) {
				return;
			}
		}
		if (idle_reset) {
			static time_t last_msg = 0;
			static int cnt = 0;
			if (now > last_msg + idle_timeout && cnt++ < 10) {
				rfbLog("active keyboard: turning X autorepeat"
				    " off.\n");
				last_msg = now;
			}
			autorepeat(0, 1);
			idle_reset = 0;

		} else if (no_repeat_countdown && autorepeat_is_on) {
			int n = no_repeat_countdown - 1;
			if (n >= 0) {
				rfbLog("Battling with something for "
				    "-norepeat!! (%d resets left)\n", n);
			} else {
				rfbLog("Battling with something for "
				    "-norepeat!!\n");
			}
			if (no_repeat_countdown > 0) {
				no_repeat_countdown--;
			}
			autorepeat(1, 0);
			autorepeat(0, 0);
		}
	}
}

void set_prop_atom(Atom atom) {
	if (atom == None) return;
	if (atom == atom_NET_ACTIVE_WINDOW) got_NET_ACTIVE_WINDOW = dnow();
	if (atom == atom_NET_CURRENT_DESKTOP) got_NET_CURRENT_DESKTOP = dnow();
	if (atom == atom_NET_CLIENT_LIST_STACKING) got_NET_CLIENT_LIST_STACKING = dnow();
	if (atom == atom_XROOTPMAP_ID) got_XROOTPMAP_ID = dnow();
}

/*
 * This routine is periodically called to check for selection related
 * and other X11 events and respond to them as needed.
 */
void check_xevents(int reset) {
#if NO_X11
	RAWFB_RET_VOID
	if (!reset) {}
	return;
#else
	XEvent xev;
	int tmp, have_clients = 0;
	static int sent_some_sel = 0;
	static time_t last_call = 0;
	static time_t last_bell = 0;
	static time_t last_init_check = 0;
	static time_t last_sync = 0;
	static time_t last_time_sync = 0;
	time_t now = time(NULL);
	static double last_request = 0.0;
	static double last_xrefresh = 0.0;
	XErrorHandler old_handler;

	if (unixpw_in_progress) return;

	RAWFB_RET_VOID

	if (now > last_init_check+1 || reset) {
		last_init_check = now;
		initialize_xevents(reset);
		if (reset) {
			return;
		}
	}

	if (screen && screen->clientHead) {
		have_clients = 1;
	}

	X_LOCK;
	/*
	 * There is a bug where we have to wait before sending text to
	 * the client... so instead of sending right away we wait a
	 * the few seconds.
	 */

	if (have_clients && watch_selection && !sent_some_sel
	    && now > last_client + sel_waittime) {
		if (XGetSelectionOwner(dpy, XA_PRIMARY) == None) {
			cutbuffer_send();
		}
		sent_some_sel = 1;
	}
	if (! have_clients) {
		/*
		 * If we don't have clients we can miss the X server
		 * going away until a client connects.
		 */
		static time_t last_X_ping = 0;
		if (now > last_X_ping + 5) {
			last_X_ping = now;
			XGetSelectionOwner(dpy, XA_PRIMARY);
		}
	}

	if (have_clients && xrefresh > 0.0 && dnow() > last_xrefresh + xrefresh) {
		XSetWindowAttributes swa;
		Visual visual;
		Window xrf;
		unsigned long mask;

		swa.override_redirect = True;
		swa.backing_store = NotUseful;
		swa.save_under = False;
		swa.background_pixmap = None;
		visual.visualid = CopyFromParent;
		mask = (CWOverrideRedirect|CWBackingStore|CWSaveUnder|CWBackPixmap);

		xrf = XCreateWindow(dpy, window, coff_x, coff_y, dpy_x, dpy_y, 0, CopyFromParent,
		    InputOutput, &visual, mask, &swa);
		if (xrf != None) {
			if (0) fprintf(stderr, "XCreateWindow(%d, %d, %d, %d) 0x%lx\n", coff_x, coff_y, dpy_x, dpy_y, xrf);
			XMapWindow(dpy, xrf);
			XFlush_wr(dpy);
			XDestroyWindow(dpy, xrf);
			XFlush_wr(dpy);
		}
		last_xrefresh = dnow();
	}

	if (now > last_call+1) {
		/* we only check these once a second or so. */
		int n = 0;

		trapped_xerror = 0;
		old_handler = XSetErrorHandler(trap_xerror);

		while (XCheckTypedEvent(dpy, MappingNotify, &xev)) {
			XRefreshKeyboardMapping((XMappingEvent *) &xev);
			n++;
		}
		if (n && use_modifier_tweak) {
			X_UNLOCK;
			initialize_modtweak();
			X_LOCK;
		}
		if (xtrap_base_event_type) {
			int base = xtrap_base_event_type;
			while (XCheckTypedEvent(dpy, base, &xev)) {
				;
			}
		}
		if (xtest_base_event_type) {
			int base = xtest_base_event_type;
			while (XCheckTypedEvent(dpy, base, &xev)) {
				;
			}
		}
		/*
		 * we can get ClientMessage from our XSendEvent() call in 
		 * selection_request().
		 */
		while (XCheckTypedEvent(dpy, ClientMessage, &xev)) {
			;
		}

		XSetErrorHandler(old_handler);
		trapped_xerror = 0;
		last_call = now;
	}

	if (freeze_when_obscured) {
		if (XCheckTypedEvent(dpy, VisibilityNotify, &xev)) {
			if (xev.type == VisibilityNotify && xev.xany.window == subwin) {
				int prev = subwin_obscured;
				if (xev.xvisibility.state == VisibilityUnobscured) {
					subwin_obscured = 0;
				} else if (xev.xvisibility.state == VisibilityPartiallyObscured) {
					subwin_obscured = 1;
				} else {
					subwin_obscured = 2;
				}
				rfbLog("subwin_obscured: %d -> %d\n", prev, subwin_obscured);
			}
		}
	}

	/* check for CUT_BUFFER0, VNC_CONNECT, X11VNC_REMOTE changes: */
	if (XCheckTypedEvent(dpy, PropertyNotify, &xev)) {
		int got_cutbuffer = 0;
		int got_vnc_connect = 0;
		int got_x11vnc_remote = 0;
		static int prop_dbg = -1;

		/* to avoid piling up between calls, read all PropertyNotify now */
		do {
			if (xev.type == PropertyNotify) {
				if (xev.xproperty.atom == XA_CUT_BUFFER0) {
					got_cutbuffer++;
				} else if (vnc_connect && vnc_connect_prop != None
				    && xev.xproperty.atom == vnc_connect_prop) {
					got_vnc_connect++;
				} else if (vnc_connect && x11vnc_remote_prop != None
				    && xev.xproperty.atom == x11vnc_remote_prop) {
					got_x11vnc_remote++;
				}
				set_prop_atom(xev.xproperty.atom);
			}
		} while (XCheckTypedEvent(dpy, PropertyNotify, &xev));

		if (prop_dbg < 0) {
			prop_dbg = 0;
			if (getenv("PROP_DBG")) {
				prop_dbg = 1;
			}
		}

		if (prop_dbg && (got_cutbuffer > 1 || got_vnc_connect > 1 || got_x11vnc_remote > 1)) {
			static double lastmsg = 0.0;
			static int count = 0;
			double now = dnow();

			if (1 && now > lastmsg + 300.0) {
				if (got_cutbuffer > 1) {
					rfbLog("check_xevents: warning: %d cutbuffer events since last check.\n", got_cutbuffer);
				}
				if (got_vnc_connect > 1) {
					rfbLog("check_xevents: warning: %d vnc_connect events since last check.\n", got_vnc_connect);
				}
				if (got_x11vnc_remote > 1) {
					rfbLog("check_xevents: warning: %d x11vnc_remote events since last check.\n", got_x11vnc_remote);
				}
				count++;
				if (count >= 3) {
					lastmsg = now;
					count = 0;
				}
			}
		}

		if (got_cutbuffer)  {
			/*
			 * Go retrieve CUT_BUFFER0 and send it.
			 *
			 * set_cutbuffer is a flag to try to avoid
			 * processing our own cutbuffer changes.
			 */
			if (have_clients && watch_selection && !set_cutbuffer) {
				cutbuffer_send();
				sent_some_sel = 1;
			}
			set_cutbuffer = 0;
		} 
		if (got_vnc_connect) {
			/*
			 * Go retrieve VNC_CONNECT string.
			 */
			read_vnc_connect_prop(0);
		} 
		if (got_x11vnc_remote) {
			/*
			 * Go retrieve X11VNC_REMOTE string.
			 */
			read_x11vnc_remote_prop(0);
		}
	}

	/* do this now that we have just cleared PropertyNotify */
	tmp = 0;
	if (rfac() < 0.6) {
		tmp = 1;
	}
	if (now > last_time_sync + sync_tod_delay + tmp) {
		sync_tod_with_servertime();
		last_time_sync = now;
	}

#if LIBVNCSERVER_HAVE_LIBXRANDR
	if (xrandr || xrandr_maybe) {
		check_xrandr_event("check_xevents");
	}
#endif
#if LIBVNCSERVER_HAVE_LIBXFIXES
	if (xfixes_present && use_xfixes && xfixes_first_initialized && xfixes_base_event_type) {
		if (XCheckTypedEvent(dpy, xfixes_base_event_type +
		    XFixesCursorNotify, &xev)) {
			got_xfixes_cursor_notify++;
		}
	}
#endif

	/* check for our PRIMARY request notification: */
	if (watch_primary || watch_clipboard) {
		int doprimary = 1, doclipboard = 2, which, own = 0;
		double delay = 1.0;
		Atom atom;
		char *req;

		if (XCheckTypedEvent(dpy, SelectionNotify, &xev)) {
			if (xev.type == SelectionNotify &&
			    xev.xselection.requestor == selwin &&
			    xev.xselection.property != None &&
			    xev.xselection.target == XA_STRING) {
				Atom s = xev.xselection.selection;
			        if (s == XA_PRIMARY || s == clipboard_atom) {
					/* go retrieve it and check it */
					if (now > last_client + sel_waittime
					    || sent_some_sel) {
						selection_send(&xev);
					}
				}
			}
		}
		/*
		 * Every second or so, request PRIMARY or CLIPBOARD,
		 * unless we already own it or there is no owner or we
		 * have no clients. 
		 * TODO: even at this low rate we should look into
		 * and performance problems in odds cases (large text,
		 * modem, etc.)
		 */
		which = 0;
		if (watch_primary && watch_clipboard && ! own_clipboard &&
		    ! own_primary) {
			delay = 0.6;
		}
		if (dnow() > last_request + delay) {
			/*
			 * It is not a good idea to do both at the same
			 * time so we must choose one:
			 */
			if (watch_primary && watch_clipboard) {
				static int count = 0;
				if (own_clipboard) {
					which = doprimary;
				} else if (own_primary) {
					which = doclipboard;
				} else if (count++ % 3 == 0) {
					which = doclipboard;
				} else {
					which = doprimary;
				}
			} else if (watch_primary) {
				which = doprimary;
			} else if (watch_clipboard) {
				which = doclipboard;
			}
			last_request = dnow();
		}
		atom = None;
		req = "none";
		if (which == doprimary) {
			own = own_primary;
			atom = XA_PRIMARY;
			req = "PRIMARY";
		} else if (which == doclipboard) {
			own = own_clipboard;
			atom = clipboard_atom;
			req = "CLIPBOARD";
		}
		if (which != 0 && ! own && have_clients &&
		    XGetSelectionOwner(dpy, atom) != None && selwin != None) {
			XConvertSelection(dpy, atom, XA_STRING, XA_STRING,
			    selwin, CurrentTime);
			if (debug_sel) {
				rfbLog("request %s\n", req);
			}
		}
	}

	if (own_primary || own_clipboard) {
		/* we own PRIMARY or CLIPBOARD, see if someone requested it: */
		trapped_xerror = 0;
		old_handler = XSetErrorHandler(trap_xerror);

		if (XCheckTypedEvent(dpy, SelectionRequest, &xev)) {
			if (own_primary && xev.type == SelectionRequest &&
			    xev.xselectionrequest.selection == XA_PRIMARY) {
				selection_request(&xev, "PRIMARY");
			}
			if (own_clipboard && xev.type == SelectionRequest &&
			    xev.xselectionrequest.selection == clipboard_atom) {
				selection_request(&xev, "CLIPBOARD");
			}
		}

		/* we own PRIMARY or CLIPBOARD, see if we no longer own it: */
		if (XCheckTypedEvent(dpy, SelectionClear, &xev)) {
			if (own_primary && xev.type == SelectionClear &&
			    xev.xselectionclear.selection == XA_PRIMARY) {
				own_primary = 0;
				if (xcut_str_primary) {
					free(xcut_str_primary);
					xcut_str_primary = NULL;
				}
				if (debug_sel) {
					rfbLog("Released PRIMARY.\n");
				}
			}
			if (own_clipboard && xev.type == SelectionClear &&
			    xev.xselectionclear.selection == clipboard_atom) {
				own_clipboard = 0;
				if (xcut_str_clipboard) {
					free(xcut_str_clipboard);
					xcut_str_clipboard = NULL;
				}
				if (debug_sel) {
					rfbLog("Released CLIPBOARD.\n");
				}
			}
		}

		XSetErrorHandler(old_handler);
		trapped_xerror = 0;
	}

	if (watch_bell || now > last_bell+1) {
		last_bell = now;
		check_bell_event();
	}
	if (tray_request != None) {
		static time_t last_tray_request = 0;
		if (now > last_tray_request + 2) {
			last_tray_request = now;
			if (tray_embed(tray_request, tray_unembed)) {
				tray_window = tray_request;
				tray_request = None;
			}
		}
	}

#ifndef DEBUG_XEVENTS
#define DEBUG_XEVENTS 1
#endif
#if DEBUG_XEVENTS
	if (debug_xevents) {
		static time_t last_check = 0;
		static time_t reminder = 0;
		static int freq = 0;

		if (! freq) {
			if (getenv("X11VNC_REMINDER_RATE")) {
				freq = atoi(getenv("X11VNC_REMINDER_RATE"));
			} else {
				freq = 300;
			}
		}

		if (now > last_check + 1) {
			int ev_type_max = 300, ev_size = 400;
			XEvent xevs[400];
			int i, tot = XEventsQueued(dpy, QueuedAlready);

			if (reminder == 0 || (tot && now > reminder + freq)) {
				print_xevent_bases();
				reminder = now;
			}
			last_check = now;

			if (tot) {
		    		fprintf(stderr, "Total events queued: %d\n",
				    tot);
			}
			for (i=1; i<ev_type_max; i++) {
				int k, n = 0;
				while (XCheckTypedEvent(dpy, i, xevs+n)) {
					if (++n >= ev_size) {
						break;
					}
				}
				if (n) {
					fprintf(stderr, "  %d%s events of type"
					    " %d queued\n", n,
					    (n >= ev_size) ? "+" : "", i);
				}
				for (k=n-1; k >= 0; k--) {
					XPutBackEvent(dpy, xevs+k);
				}
			}
		}
	}
#endif

	if (now > last_sync + 1200) {
		/* kludge for any remaining event leaks */
		int bugout = use_xdamage ? 500 : 50;
		int qlen, i;
		if (last_sync != 0) {
			qlen = XEventsQueued(dpy, QueuedAlready);
			if (qlen >= bugout) {
				rfbLog("event leak: %d queued, "
				    " calling XSync(dpy, True)\n", qlen);  
				rfbLog("  for diagnostics run: 'x11vnc -R"
				    " debug_xevents:1'\n");
				XSync(dpy, True);
			}
		}
		last_sync = now;

		/* clear these, we don't want any events on them */
		if (rdpy_ctrl) {
			qlen = XEventsQueued(rdpy_ctrl, QueuedAlready);
			for (i=0; i<qlen; i++) {
				XNextEvent(rdpy_ctrl, &xev);
			}
		}
		if (gdpy_ctrl) {
			qlen = XEventsQueued(gdpy_ctrl, QueuedAlready);
			for (i=0; i<qlen; i++) {
				XNextEvent(gdpy_ctrl, &xev);
			}
		}
	}
	X_UNLOCK;

#endif	/* NO_X11 */
}

extern int rawfb_vnc_reflect;
/*
 * hook called when a VNC client sends us some "XCut" text (rfbClientCutText).
 */
void xcut_receive(char *text, int len, rfbClientPtr cl) {
	allowed_input_t input;

	if (threads_drop_input) {
		return;
	}

	if (unixpw_in_progress) {
		rfbLog("xcut_receive: unixpw_in_progress, skipping.\n");
		return;
	}

	if (!watch_selection) {
		return;
	}
	if (view_only) {
		return;
	}
	if (text == NULL || len == 0) {
		return;
	}
	get_allowed_input(cl, &input);
	if (!input.clipboard) {
		return;
	}
	INPUT_LOCK;

	if (remote_prefix != NULL && strstr(text, remote_prefix) == text) {
		char *result, *rcmd = text + strlen(remote_prefix);
		char *tmp = (char *) calloc(len + 8, 1);

		if (strstr(rcmd, "cmd=") != rcmd && strstr(rcmd, "qry=") != rcmd) {
			strcat(tmp, "qry=");
		}
		strncat(tmp, rcmd, len - strlen(remote_prefix));
		rfbLog("remote_prefix command: '%s'\n", tmp);

		if (use_threads) {
			if (client_connect_file) {
				FILE *f = fopen(client_connect_file, "w");
				if (f) {
					fprintf(f, "%s\n", tmp);
					fclose(f);
					free(tmp);
					INPUT_UNLOCK;
					return;
				}
			}
			if (vnc_connect) {
				sprintf(x11vnc_remote_str, "%s", tmp);
				free(tmp);
				INPUT_UNLOCK;
				return;
			}
		}
		INPUT_UNLOCK;


		result = process_remote_cmd(tmp, 1);
		if (result == NULL ) {
			result = strdup("null");
		} else if (!strcmp(result, "")) {
			free(result);
			result = strdup("none");
		}
		rfbLog("remote_prefix result:  '%s'\n", result);

		free(tmp);
		tmp = (char *) calloc(strlen(remote_prefix) + strlen(result) + 1, 1);

		strcat(tmp, remote_prefix);
		strcat(tmp, result);
		free(result);

		rfbSendServerCutText(screen, tmp, strlen(tmp));
		free(tmp);

		return;
	}

	if (! check_sel_direction("recv", "xcut_receive", text, len)) {
		INPUT_UNLOCK;
		return;
	}

#ifdef MACOSX
	if (macosx_console) {
		macosx_set_sel(text, len);
		INPUT_UNLOCK;
		return;
	}
#endif

	if (rawfb_vnc_reflect) {
		vnc_reflect_send_cuttext(text, len);
		INPUT_UNLOCK;
		return;
	}

	RAWFB_RET_VOID

#if NO_X11
	INPUT_UNLOCK;
	return;
#else

	X_LOCK;

	/* associate this text with PRIMARY (and SECONDARY...) */
	if (set_primary && ! own_primary && selwin != None) {
		own_primary = 1;
		/* we need to grab the PRIMARY selection */
		XSetSelectionOwner(dpy, XA_PRIMARY, selwin, CurrentTime);
		XFlush_wr(dpy);
		if (debug_sel) {
			rfbLog("Own PRIMARY.\n");
		}
	}

	if (set_clipboard && ! own_clipboard && clipboard_atom != None && selwin != None) {
		own_clipboard = 1;
		/* we need to grab the CLIPBOARD selection */
		XSetSelectionOwner(dpy, clipboard_atom, selwin, CurrentTime);
		XFlush_wr(dpy);
		if (debug_sel) {
			rfbLog("Own CLIPBOARD.\n");
		}
	}

	/* duplicate the text string for our own use. */
	if (set_primary) {
		if (xcut_str_primary != NULL) {
			free(xcut_str_primary);
			xcut_str_primary = NULL;
		}
		xcut_str_primary = (char *) malloc((size_t) (len+1));
		strncpy(xcut_str_primary, text, len);
		xcut_str_primary[len] = '\0';	/* make sure null terminated */
		if (debug_sel) {
			rfbLog("Set PRIMARY   '%s'\n", xcut_str_primary);
		}
	}
	if (set_clipboard) {
		if (xcut_str_clipboard != NULL) {
			free(xcut_str_clipboard);
			xcut_str_clipboard = NULL;
		}
		xcut_str_clipboard = (char *) malloc((size_t) (len+1));
		strncpy(xcut_str_clipboard, text, len);
		xcut_str_clipboard[len] = '\0';	/* make sure null terminated */
		if (debug_sel) {
			rfbLog("Set CLIPBOARD '%s'\n", xcut_str_clipboard);
		}
	}

	/* copy this text to CUT_BUFFER0 as well: */
	XChangeProperty(dpy, rootwin, XA_CUT_BUFFER0, XA_STRING, 8,
	    PropModeReplace, (unsigned char *) text, len);
	XFlush_wr(dpy);

	X_UNLOCK;
	INPUT_UNLOCK;

	set_cutbuffer = 1;
#endif	/* NO_X11 */
}

void kbd_release_all_keys(rfbClientPtr cl) {
	if (unixpw_in_progress) {
		rfbLog("kbd_release_all_keys: unixpw_in_progress, skipping.\n");
		return;
	}
	if (cl->viewOnly) {
		return;
	}

	RAWFB_RET_VOID

#if NO_X11
	return;
#else
	if (use_threads) {
		X_LOCK;
	}

	clear_keys();
	clear_modifiers(0);

	if (use_threads) {
		X_UNLOCK;
	}
#endif
}

void set_single_window(rfbClientPtr cl, int x, int y) {
	int ok = 0;
	if (no_ultra_ext) {
		return;
	}
	if (unixpw_in_progress) {
		rfbLog("set_single_window: unixpw_in_progress, dropping client.\n");
		rfbCloseClient(cl);
		return;
	}
	if (cl->viewOnly) {
		return;
	}

	RAWFB_RET_VOID

#if NO_X11
	return;
#else
	if (x==1 && y==1) {
		if (subwin) {
			subwin = 0x0;
			ok = 1;
		}
	} else {
		Window r, c;
		int rootx, rooty, wx, wy;
		unsigned int mask;

		update_x11_pointer_position(x, y);
		XSync(dpy, False);

		if (XQueryPointer_wr(dpy, rootwin, &r, &c, &rootx, &rooty,
		    &wx, &wy, &mask)) {
			if (c != None) {
				subwin = c;
				ok = 1;
			}
		}
	}

	if (ok) {
		check_black_fb();
		do_new_fb(1);	
	}
#endif

}
void set_server_input(rfbClientPtr cl, int grab) {
	if (no_ultra_ext) {
		return;
	}
	if (unixpw_in_progress) {
		rfbLog("set_server_input: unixpw_in_progress, dropping client.\n");
		rfbCloseClient(cl);
		return;
	}
	if (cl->viewOnly) {
		return;
	}

	RAWFB_RET_VOID

#if NO_X11
	return;
#else
	if (grab) {
		if (!no_ultra_dpms) {
			set_dpms_mode("enable");
			set_dpms_mode("off");
			force_dpms = 1;
		}

		process_remote_cmd("cmd=grabkbd", 0);
		process_remote_cmd("cmd=grabptr", 0);

	} else {
		process_remote_cmd("cmd=nograbkbd", 0);
		process_remote_cmd("cmd=nograbptr", 0);

		if (!no_ultra_dpms) {
			force_dpms = 0;
		}
	}
#endif
}

static int wsock_timeout_sock = -1;

static void wsock_timeout (int sig) {
	rfbLog("sig: %d, wsock_timeout.\n", sig);
	if (wsock_timeout_sock >= 0) {
		close(wsock_timeout_sock);
		wsock_timeout_sock = -1;
	}
}

static void try_local_chat_window(void) {
	int i, port, lsock;
	char cmd[100];
	struct sockaddr_in addr;
	pid_t pid = -1;
#ifdef __hpux
	int addrlen = sizeof(addr);
#else
	socklen_t addrlen = sizeof(addr);
#endif

	for (i = 0; i < 90; i++)  {
		/* find an open port */
		port = 7300 + i;
		/* XXX ::1 fallback */
		lsock = listen_tcp(port, htonl(INADDR_LOOPBACK), 0);
		if (lsock >= 0) {
			break;
		}
		port = 0;
	}

	if (port == 0) {
		return;
	}

	/* have ssvncvncviewer connect back to us (n.b. sockpair fails) */

	sprintf(cmd, "ssvnc -cmd VNC://localhost:%d -chatonly", port);

#if LIBVNCSERVER_HAVE_FORK
	pid = fork();
#endif

	if (pid == -1) {
		perror("fork");
		return;
	} else if (pid == 0) {
		char *args[4];
		int d;
		args[0] = "/bin/sh";
		args[1] = "-c";
		/* "ssvnc -cmd VNC://fd=0 -chatonly"; not working */
		args[2] = cmd;
		args[3] = NULL;

		set_env("VNCVIEWER_PASSWORD", "moo");
#if !NO_X11
		if (dpy != NULL) {
			set_env("DISPLAY", DisplayString(dpy));
		}
#endif
		for (d = 3; d < 256; d++) {
			close(d);
		}

		execvp(args[0], args);
		perror("exec");
		exit(1);
	} else {
		int i, sock = -1;
		rfbNewClientHookPtr new_save;

		signal(SIGALRM, wsock_timeout);
		wsock_timeout_sock = lsock;
		
		alarm(10);
		sock = accept(lsock, (struct sockaddr *)&addr, &addrlen);
		alarm(0);

		signal(SIGALRM, SIG_DFL);
		close(lsock);

		if (sock < 0) {
			return;
		}

		/* mutex */
		new_save = screen->newClientHook;
		screen->newClientHook = new_client_chat_helper;

		chat_window_client = create_new_client(sock, 1);

		screen->newClientHook = new_save;

		if (chat_window_client != NULL) {
			rfbPasswordCheckProcPtr pwchk_save = screen->passwordCheck;
			rfbBool save_shared1 = screen->alwaysShared;
			rfbBool save_shared2 = screen->neverShared;

			screen->alwaysShared = TRUE;
			screen->neverShared  = FALSE;

			screen->passwordCheck = password_check_chat_helper;
			for (i=0; i<30; i++) {
				rfbPE(-1);
				if (!chat_window_client) {
					break;
				}
				if (chat_window_client->state == RFB_NORMAL) {
					break;
				}
			}

			screen->passwordCheck = pwchk_save;
			screen->alwaysShared  = save_shared1;
			screen->neverShared   = save_shared2;
		}
	}
}

void set_text_chat(rfbClientPtr cl, int len, char *txt) {
	int dochat = 1;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl2;
	unsigned int ulen = (unsigned int) len;

	if (no_ultra_ext || ! dochat) {
		return;
	}

	if (unixpw_in_progress) {
		rfbLog("set_text_chat: unixpw_in_progress, dropping client.\n");
		rfbCloseClient(cl);
		return;
	}
#if LIBVNCSERVER_HAS_TEXTCHAT

	if (chat_window && chat_window_client == NULL && ulen == rfbTextChatOpen) {
		try_local_chat_window();
	}

	saw_ultra_chat = 1;

	iter = rfbGetClientIterator(screen);
	while( (cl2 = rfbClientIteratorNext(iter)) ) {
		unsigned int ulen = (unsigned int) len;
		if (cl2 == cl) {
			continue;
		}
		if (cl2->state != RFB_NORMAL) {
			continue;
		}

		SEND_LOCK(cl2);

		if (ulen == rfbTextChatOpen) {
			rfbSendTextChatMessage(cl2, rfbTextChatOpen, "");
		} else if (ulen == rfbTextChatClose) {
			rfbSendTextChatMessage(cl2, rfbTextChatClose, "");
			/* not clear what is going on WRT close and finished... */
			rfbSendTextChatMessage(cl2, rfbTextChatFinished, "");
		} else if (ulen == rfbTextChatFinished) {
			rfbSendTextChatMessage(cl2, rfbTextChatFinished, "");
		} else if (len <= rfbTextMaxSize) {
			rfbSendTextChatMessage(cl2, len, txt);
		}

		SEND_UNLOCK(cl2);
	}
	rfbReleaseClientIterator(iter);

	if (ulen == rfbTextChatClose && cl != NULL) {
		/* not clear what is going on WRT close and finished... */
		SEND_LOCK(cl);
		rfbSendTextChatMessage(cl, rfbTextChatFinished, "");
		SEND_UNLOCK(cl);
	}
#endif
}

int get_keyboard_led_state_hook(rfbScreenInfoPtr s) {
	if (s) {}
	if (unixpw_in_progress) {
		rfbLog("get_keyboard_led_state_hook: unixpw_in_progress, skipping.\n");
		return 0;
	}
	return 0;
}
int get_file_transfer_permitted(rfbClientPtr cl) {
	allowed_input_t input;
	if (unixpw_in_progress) {
		rfbLog("get_file_transfer_permitted: unixpw_in_progress, dropping client.\n");
		rfbCloseClient(cl);
		return FALSE;
	}
if (0) fprintf(stderr, "get_file_transfer_permitted called\n");
	if (view_only) {
		return FALSE;
	}
	if (cl->viewOnly) {
		return FALSE;
	}
	get_allowed_input(cl, &input);
	if (!input.files) {
		return FALSE;
	}
	if (screen->permitFileTransfer) {
		saw_ultra_file = 1;
	}
	return screen->permitFileTransfer;
}


