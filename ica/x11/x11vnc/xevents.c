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

/* XXX CHECK BEFORE RELEASE */
int grab_buster = 0;
int grab_kbd = 0;
int grab_ptr = 0;
int sync_tod_delay = 3;

void initialize_vnc_connect_prop(void);
void initialize_x11vnc_remote_prop(void);
void initialize_clipboard_atom(void);
void spawn_grab_buster(void);
void sync_tod_with_servertime(void);
void check_keycode_state(void);
void check_autorepeat(void);
void check_xevents(int reset);
void xcut_receive(char *text, int len, rfbClientPtr cl);


static void initialize_xevents(int reset);
static void print_xevent_bases(void);
static void get_prop(char *str, int len, Atom prop);
static void bust_grab(int reset);
static int process_watch(char *str, int parent, int db);
static void grab_buster_watch(int parent, char *dstr);


void initialize_vnc_connect_prop(void) {
	vnc_connect_str[0] = '\0';
	RAWFB_RET_VOID
	vnc_connect_prop = XInternAtom(dpy, "VNC_CONNECT", False);
}

void initialize_x11vnc_remote_prop(void) {
	x11vnc_remote_str[0] = '\0';
	RAWFB_RET_VOID
	x11vnc_remote_prop = XInternAtom(dpy, "X11VNC_REMOTE", False);
}

void initialize_clipboard_atom(void) {
	RAWFB_RET_VOID
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
}

static void initialize_xevents(int reset) {
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
		XSelectInput(dpy, rootwin, PropertyChangeMask);
		X_UNLOCK;
		did_xselect_input = 1;
	}
	if (watch_selection && !did_xcreate_simple_window) {
		/* create fake window for our selection ownership, etc */

		X_LOCK;
		selwin = XCreateSimpleWindow(dpy, rootwin, 0, 0, 1, 1, 0, 0, 0);
		X_UNLOCK;
		did_xcreate_simple_window = 1;
	}

	if (xrandr && !did_xrandr) {
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
		initialize_xfixes();
		did_xfixes = 1;
	}
	if (xdamage_present && !did_xdamage) {
		initialize_xdamage();
		did_xdamage = 1;
	}
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

static void get_prop(char *str, int len, Atom prop) {
	Atom type;
	int format, slen, dlen, i;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;

	for (i=0; i<len; i++) {
		str[i] = '\0';
	}
	if (prop == None) {
		return;
	}
	RAWFB_RET_VOID

	slen = 0;
	
	do {
		if (XGetWindowProperty(dpy, DefaultRootWindow(dpy),
		    prop, nitems/4, len/16, False,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > len) {
				/* too big */
				XFree(data);
				break;
			}
			memcpy(str+slen, data, dlen);
			slen += dlen;
			str[slen] = '\0';
			XFree(data);
		}
	} while (bytes_after > 0);
}

static void bust_grab(int reset) {
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
	Atom ticker_atom = None;
	int sleep = sync_tod_delay * 921 * 1000;
	char propval[200];
	int ev, er, maj, min;
	int db = 0;

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

	ticker_atom = XInternAtom(dpy, "X11VNC_TICKER", False);
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

		get_prop(propval, 128, ticker_atom);
		if (db) fprintf(stderr, "got_prop:   %s\n", propval);

		if (!process_watch(propval, parent, db)) {
			break;
		}
	}
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
	static Atom ticker_atom = None;
	XEvent xev;
	char diff[128];
	static int seq = 0;
	static unsigned long xserver_ticks = 1;
	int i, db = 0;

	RAWFB_RET_VOID

	if (! ticker_atom) {
		ticker_atom = XInternAtom(dpy, "X11VNC_TICKER", False);
	}
	if (! ticker_atom) {
		return;
	}

	XSync(dpy, False);
	while (XCheckTypedEvent(dpy, PropertyNotify, &xev)) {
		;
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
}

void check_keycode_state(void) {
	static time_t last_check = 0;
	int delay = 10, noinput = 3;
	time_t now = time(NULL);

	if (! client_count) {
		return;
	}

	RAWFB_RET_VOID

	if (unixpw_in_progress) return;

	/*
	 * periodically update our model of the keycode_state[]
	 * by correlating with the Xserver.  wait for a pause in
	 * keyboard input to be on the safe side.  the idea here
	 * is to remove stale keycode state, not to be perfectly
	 * in sync with the Xserver at every instant of time.
	 */
	if (now > last_check + delay && now > last_keyboard_input + noinput) {
		init_track_keycode_state();
		last_check = now;
	}
}

void check_autorepeat(void) {
	static time_t last_check = 0;
	time_t now = time(NULL);
	int autorepeat_is_on, autorepeat_initially_on, idle_timeout = 300;
	static int idle_reset = 0;

	if (! no_autorepeat || ! client_count) {
		return;
	}
	if (now <= last_check + 1) {
		return;
	}

	if (unixpw_in_progress) return;

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
			if (now > last_msg + idle_timeout && cnt++ < 5) {
				rfbLog("idle keyboard:   turning X autorepeat"
				    " back on.\n");
				last_msg = now;
			}
			autorepeat(1, 1);
			idle_reset = 1;
		}
	} else {
		if (idle_reset) {
			static time_t last_msg = 0;
			static int cnt = 0;
			if (now > last_msg + idle_timeout && cnt++ < 5) {
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

/*
 * This routine is periodically called to check for selection related
 * and other X11 events and respond to them as needed.
 */
void check_xevents(int reset) {
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
	XErrorHandler old_handler;

	RAWFB_RET_VOID

	if (unixpw_in_progress) return;

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
	}

	/* check for CUT_BUFFER0 and VNC_CONNECT changes: */
	if (XCheckTypedEvent(dpy, PropertyNotify, &xev)) {
		if (xev.type == PropertyNotify) {
			if (xev.xproperty.atom == XA_CUT_BUFFER0) {
				/*
				 * Go retrieve CUT_BUFFER0 and send it.
				 *
				 * set_cutbuffer is a flag to try to avoid
				 * processing our own cutbuffer changes.
				 */
				if (have_clients && watch_selection
				    && ! set_cutbuffer) {
					cutbuffer_send();
					sent_some_sel = 1;
				}
				set_cutbuffer = 0;
			} else if (vnc_connect && vnc_connect_prop != None
		    	    && xev.xproperty.atom == vnc_connect_prop) {
				/*
				 * Go retrieve VNC_CONNECT string.
				 */
				read_vnc_connect_prop(0);
			} else if (vnc_connect && x11vnc_remote_prop != None
		    	    && xev.xproperty.atom == x11vnc_remote_prop) {
				/*
				 * Go retrieve X11VNC_REMOTE string.
				 */
				read_x11vnc_remote_prop(0);
			}
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
	if (xrandr) {
		check_xrandr_event("check_xevents");
	}
#endif
#if LIBVNCSERVER_HAVE_LIBXFIXES
	if (xfixes_present && use_xfixes && xfixes_base_event_type) {
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
		    XGetSelectionOwner(dpy, atom) != None) {
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

	last_call = now;
}

/*
 * hook called when a VNC client sends us some "XCut" text (rfbClientCutText).
 */
void xcut_receive(char *text, int len, rfbClientPtr cl) {
	allowed_input_t input;

	RAWFB_RET_VOID

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

	if (! check_sel_direction("recv", "xcut_receive", text, len)) {
		return;
	}

	X_LOCK;

	/* associate this text with PRIMARY (and SECONDARY...) */
	if (set_primary && ! own_primary) {
		own_primary = 1;
		/* we need to grab the PRIMARY selection */
		XSetSelectionOwner(dpy, XA_PRIMARY, selwin, CurrentTime);
		XFlush_wr(dpy);
		if (debug_sel) {
			rfbLog("Own PRIMARY.\n");
		}
	}

	if (set_clipboard && ! own_clipboard && clipboard_atom != None) {
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

	set_cutbuffer = 1;
}


