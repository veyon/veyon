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

/* -- xrecord.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "win_utils.h"
#include "cleanup.h"
#include "userinput.h"
#include "winattr_t.h"
#include "scrollevent_t.h"
#include "unixpw.h"

#define SCR_EV_MAX 128
scroll_event_t scr_ev[SCR_EV_MAX];
int scr_ev_cnt;

int xrecording = 0;
int xrecord_set_by_keys = 0;
int xrecord_set_by_mouse = 0;
Window xrecord_focus_window = None;
Window xrecord_wm_window = None;
Window xrecord_ptr_window = None;
KeySym xrecord_keysym = NoSymbol;

#define NAMEINFO 2048
char xrecord_name_info[NAMEINFO];

#define SCR_ATTR_CACHE 8
winattr_t scr_attr_cache[SCR_ATTR_CACHE];
static double attr_cache_max_age = 1.5;

Display *rdpy_data = NULL;		/* Data connection for RECORD */
Display *rdpy_ctrl = NULL;		/* Control connection for RECORD */

Display *gdpy_ctrl = NULL;
Display *gdpy_data = NULL;
int xserver_grabbed = 0;

int trap_record_xerror(Display *, XErrorEvent *);

void initialize_xrecord(void);
void zerodisp_xrecord(void);
void shutdown_xrecord(void);
int xrecord_skip_keysym(rfbKeySym keysym);
int xrecord_skip_button(int newb, int old);
int xrecord_scroll_keysym(rfbKeySym keysym);
void check_xrecord_reset(int force);
void xrecord_watch(int start, int setby);


#if LIBVNCSERVER_HAVE_RECORD
static XRecordRange *rr_CA = NULL;
static XRecordRange *rr_CW = NULL;
static XRecordRange *rr_GS = NULL;
static XRecordRange *rr_scroll[10];
static XRecordContext rc_scroll;
static XRecordClientSpec rcs_scroll;
static XRecordRange *rr_grab[10];
static XRecordContext rc_grab;
static XRecordClientSpec rcs_grab;
#endif
static XErrorEvent *trapped_record_xerror_event;

static void xrecord_grabserver(int start);
static int xrecord_vi_scroll_keysym(rfbKeySym keysym);
static int xrecord_emacs_scroll_keysym(rfbKeySym keysym);
static int lookup_attr_cache(Window win, int *cache_index, int *next_index);
#if LIBVNCSERVER_HAVE_RECORD
static void record_CA(XPointer ptr, XRecordInterceptData *rec_data);
static void record_CW(XPointer ptr, XRecordInterceptData *rec_data);
static void record_switch(XPointer ptr, XRecordInterceptData *rec_data);
static void record_grab(XPointer ptr, XRecordInterceptData *rec_data);
static void shutdown_record_context(XRecordContext rc, int bequiet, int reopen);
#endif
static void check_xrecord_grabserver(void);


int trap_record_xerror(Display *d, XErrorEvent *error) {
	trapped_record_xerror = 1;
	trapped_record_xerror_event = error;

	if (d) {} /* unused vars warning: */

	return 0;
}

static void xrecord_grabserver(int start) {
	XErrorHandler old_handler = NULL;
	int rc = 0;

	if (debug_grabs) {
		fprintf(stderr, "xrecord_grabserver%d/%d %.5f\n",
			xserver_grabbed, start, dnowx());
	}

	if (! gdpy_ctrl || ! gdpy_data) {
		return;
	}
#if LIBVNCSERVER_HAVE_RECORD
	if (!start) {
		if (! rc_grab) {
			return;
		}
		XRecordDisableContext(gdpy_ctrl, rc_grab);
		XRecordFreeContext(gdpy_ctrl, rc_grab);
		XFlush_wr(gdpy_ctrl);
		rc_grab = 0;
		return;
	}

	xserver_grabbed = 0;

	rr_grab[0] = rr_GS;
	rcs_grab = XRecordAllClients;

	rc_grab = XRecordCreateContext(gdpy_ctrl, 0, &rcs_grab, 1, rr_grab, 1);
	trapped_record_xerror = 0;
	old_handler = XSetErrorHandler(trap_record_xerror);

	XSync(gdpy_ctrl, True);

	if (! rc_grab || trapped_record_xerror) {
		XCloseDisplay_wr(gdpy_ctrl);
		XCloseDisplay_wr(gdpy_data);
		gdpy_ctrl = NULL;
		gdpy_data = NULL;
		XSetErrorHandler(old_handler);
		return;
	}
	rc = XRecordEnableContextAsync(gdpy_data, rc_grab, record_grab, NULL);
	if (!rc || trapped_record_xerror) {
		XCloseDisplay_wr(gdpy_ctrl);
		XCloseDisplay_wr(gdpy_data);
		gdpy_ctrl = NULL;
		gdpy_data = NULL;
		XSetErrorHandler(old_handler);
		return;
	}
	XFlush_wr(gdpy_data);
	XSetErrorHandler(old_handler);
#else
	if (!rc || !old_handler) {}
#endif
	if (debug_grabs) {
		fprintf(stderr, "xrecord_grabserver-done: %.5f\n", dnowx());
	}
}

void zerodisp_xrecord(void) {
	rdpy_data = NULL;
	rdpy_ctrl = NULL;
	gdpy_data = NULL;
	gdpy_ctrl = NULL;
}

void initialize_xrecord(void) {
	use_xrecord = 0;
	if (! xrecord_present) {
		return;
	}
	if (nofb) {
		return;
	}
	if (noxrecord) {
		return;
	}
	RAWFB_RET_VOID
#if LIBVNCSERVER_HAVE_RECORD

	if (rr_CA) XFree_wr(rr_CA);
	if (rr_CW) XFree_wr(rr_CW);
	if (rr_GS) XFree_wr(rr_GS);

	rr_CA = XRecordAllocRange();
	rr_CW = XRecordAllocRange();
	rr_GS = XRecordAllocRange();
	if (!rr_CA || !rr_CW || !rr_GS) {
		return;
	}
	/* protocol request ranges: */
	rr_CA->core_requests.first = X_CopyArea;
	rr_CA->core_requests.last  = X_CopyArea;
	
	rr_CW->core_requests.first = X_ConfigureWindow;
	rr_CW->core_requests.last  = X_ConfigureWindow;

	rr_GS->core_requests.first = X_GrabServer;
	rr_GS->core_requests.last  = X_UngrabServer;

	X_LOCK;
	/* open a 2nd control connection to DISPLAY: */
	if (rdpy_data) {
		XCloseDisplay_wr(rdpy_data);
		rdpy_data = NULL;
	}
	if (rdpy_ctrl) {
		XCloseDisplay_wr(rdpy_ctrl);
		rdpy_ctrl = NULL;
	}
	rdpy_ctrl = XOpenDisplay_wr(DisplayString(dpy));
	if (!rdpy_ctrl) {
		fprintf(stderr, "rdpy_ctrl open failed: %s / %s / %s / %s\n", getenv("DISPLAY"), DisplayString(dpy), getenv("XAUTHORITY"), getenv("XAUTHORIT_"));
	}
	XSync(dpy, True);
	XSync(rdpy_ctrl, True);
	/* open datalink connection to DISPLAY: */
	rdpy_data = XOpenDisplay_wr(DisplayString(dpy));
	if (!rdpy_data) {
		fprintf(stderr, "rdpy_data open failed\n");
	}
	if (!rdpy_ctrl || ! rdpy_data) {
		X_UNLOCK;
		return;
	}
	disable_grabserver(rdpy_ctrl, 0);
	disable_grabserver(rdpy_data, 0);

	use_xrecord = 1;

	/*
	 * now set up the GrabServer watcher.  We get GrabServer
	 * deadlock in XRecordCreateContext() even with XTestGrabServer
	 * in place, why?  Not sure, so we manually watch for grabs...
	 */
	if (gdpy_data) {
		XCloseDisplay_wr(gdpy_data);
		gdpy_data = NULL;
	}
	if (gdpy_ctrl) {
		XCloseDisplay_wr(gdpy_ctrl);
		gdpy_ctrl = NULL;
	}
	xserver_grabbed = 0;

	gdpy_ctrl = XOpenDisplay_wr(DisplayString(dpy));
	if (!gdpy_ctrl) {
		fprintf(stderr, "gdpy_ctrl open failed\n");
	}
	XSync(dpy, True);
	XSync(gdpy_ctrl, True);
	gdpy_data = XOpenDisplay_wr(DisplayString(dpy));
	if (!gdpy_data) {
		fprintf(stderr, "gdpy_data open failed\n");
	}
	if (gdpy_ctrl && gdpy_data) {
		disable_grabserver(gdpy_ctrl, 0);
		disable_grabserver(gdpy_data, 0);
		xrecord_grabserver(1);
	}
	X_UNLOCK;
#endif
}

void shutdown_xrecord(void) {
#if LIBVNCSERVER_HAVE_RECORD

	if (debug_grabs) {
		fprintf(stderr, "shutdown_xrecord%d %.5f\n",
			xserver_grabbed, dnowx());
	}

	if (rr_CA) XFree_wr(rr_CA);
	if (rr_CW) XFree_wr(rr_CW);
	if (rr_GS) XFree_wr(rr_GS);

	rr_CA = NULL;
	rr_CW = NULL;
	rr_GS = NULL;

	X_LOCK;
	if (rdpy_ctrl && rc_scroll) {
		XRecordDisableContext(rdpy_ctrl, rc_scroll);
		XRecordFreeContext(rdpy_ctrl, rc_scroll);
		XSync(rdpy_ctrl, False);
		rc_scroll = 0;
	}
		
	if (gdpy_ctrl && rc_grab) {
		XRecordDisableContext(gdpy_ctrl, rc_grab);
		XRecordFreeContext(gdpy_ctrl, rc_grab);
		XSync(gdpy_ctrl, False);
		rc_grab = 0;
	}
		
	if (rdpy_data) {
		XCloseDisplay_wr(rdpy_data);
		rdpy_data = NULL;
	}
	if (rdpy_ctrl) {
		XCloseDisplay_wr(rdpy_ctrl);
		rdpy_ctrl = NULL;
	}
	if (gdpy_data) {
		XCloseDisplay_wr(gdpy_data);
		gdpy_data = NULL;
	}
	if (gdpy_ctrl) {
		XCloseDisplay_wr(gdpy_ctrl);
		gdpy_ctrl = NULL;
	}
	xserver_grabbed = 0;
	X_UNLOCK;
#endif
	use_xrecord = 0;

	if (debug_grabs) {
		fprintf(stderr, "shutdown_xrecord-done: %.5f\n", dnowx());
	}
}

int xrecord_skip_keysym(rfbKeySym keysym) {
	KeySym sym = (KeySym) keysym;
	int ok = -1, matched = 0;

	if (scroll_key_list) {
		int k, exclude = 0;
		if (scroll_key_list[0]) {
			exclude = 1;
		}
		k = 1;
		while (scroll_key_list[k] != NoSymbol) {
			if (scroll_key_list[k++] == sym) {
				matched = 1;
				break;
			}
		}
		if (exclude) {
			if (matched) {
				return 1;
			} else {
				ok = 1;
			}
		} else {
			if (matched) {
				ok = 1;
			} else {
				ok = 0;
			}
		}
	}
	if (ok == 1) {
		return 0;
	} else if (ok == 0) {
		return 1;
	}

	/* apply various heuristics: */

	if (IsModifierKey(sym)) {
		/* Shift, Control, etc, usu. generate no scrolls */
		return 1;
	}
	if (sym == XK_space && scroll_term) {
		/* space in a terminal is usu. full page... */
		Window win;
		static Window prev_top = None;
		int size = 256;
		static char name[256];

		X_LOCK;
		win = query_pointer(rootwin);
		X_UNLOCK;
		if (win != None && win != rootwin) {
			if (prev_top != None && win == prev_top) {
				;	/* use cached result */
			} else {
				prev_top = win;
				X_LOCK;
				win = descend_pointer(6, win, name, size);
				X_UNLOCK;
			}
			if (match_str_list(name, scroll_term)) {
				return 1;
			}
		}
	}

	/* TBD use typing_rate() so */
	return 0;
}

int xrecord_skip_button(int new_button, int old) {
	/* unused vars warning: */
	if (new_button || old) {}

	return 0;
}

static int xrecord_vi_scroll_keysym(rfbKeySym keysym) {
	KeySym sym = (KeySym) keysym;
	if (sym == XK_J || sym == XK_j || sym == XK_K || sym == XK_k) {
		return 1;	/* vi */
	}
	if (sym == XK_D || sym == XK_d || sym == XK_U || sym == XK_u) {
		return 1;	/* Ctrl-d/u */
	}
	if (sym == XK_Z || sym == XK_z) {
		return 1;	/* zz, zt, zb .. */
	}
	return 0;
}

static int xrecord_emacs_scroll_keysym(rfbKeySym keysym) {
	KeySym sym = (KeySym) keysym;
	if (sym == XK_N || sym == XK_n || sym == XK_P || sym == XK_p) {
		return 1;	/* emacs */
	}
	/* Must be some more ... */
	return 0;
}

int xrecord_scroll_keysym(rfbKeySym keysym) {
	KeySym sym = (KeySym) keysym;
	/* X11/keysymdef.h */

	if (sym == XK_Return || sym == XK_KP_Enter || sym == XK_Linefeed) {
		return 1;	/* Enter */
	}
	if (sym==XK_Up || sym==XK_KP_Up || sym==XK_Down || sym==XK_KP_Down) {
		return 1;	/* U/D arrows */
	}
	if (sym == XK_Left || sym == XK_KP_Left || sym == XK_Right ||
	    sym == XK_KP_Right) {
		return 1;	/* L/R arrows */
	}
	if (xrecord_vi_scroll_keysym(keysym)) {
		return 1;
	}
	if (xrecord_emacs_scroll_keysym(keysym)) {
		return 1;
	}
	return 0;
}

static int lookup_attr_cache(Window win, int *cache_index, int *next_index) {
	double now, t, oldest = 0.0;
	int i, old_index = -1, count = 0;
	Window cwin;

	*cache_index = -1;
	*next_index  = -1;
	
	if (win == None) {
		return 0;
	}
	if (attr_cache_max_age == 0.0) {
		return 0;
	}

	dtime0(&now);
	for (i=0; i < SCR_ATTR_CACHE; i++) {

		cwin = scr_attr_cache[i].win;
		t = scr_attr_cache[i].time;

		if (now > t + attr_cache_max_age) {
			/* expire it even if it is the one we want */
			scr_attr_cache[i].win = cwin = None;
			scr_attr_cache[i].fetched = 0;
			scr_attr_cache[i].valid = 0;
		}

		if (*next_index == -1 && cwin == None) {
			*next_index = i;
		}
		if (*next_index == -1) {
			/* record oldest */
			if (old_index == -1 || t < oldest) {
				oldest = t;
				old_index = i;
			}
		}
		if (cwin != None) {
			count++;
		}
		if (cwin == win) {
			if (*cache_index == -1) {
				*cache_index = i;
			} else {
				/* remove dups */
				scr_attr_cache[i].win = None;
				scr_attr_cache[i].fetched = 0;
				scr_attr_cache[i].valid = 0;
			}
		}
	}
	if (*next_index == -1) {
		*next_index = old_index;
	}

if (0) fprintf(stderr, "lookup_attr_cache count: %d\n", count);
	if (*cache_index != -1) {
		return 1;
	} else {
		return 0;
	}
}


static XID xrecord_seq = 0;
static double xrecord_start = 0.0;

#if LIBVNCSERVER_HAVE_RECORD
static void record_CA(XPointer ptr, XRecordInterceptData *rec_data) {
	xCopyAreaReq *req;
	Window src = None, dst = None, c;
	XWindowAttributes attr, attr2;
	int src_x, src_y, dst_x, dst_y, rx, ry, rx2, ry2;
	int good = 1, dx = 0, dy = 0, k=0, i;
	unsigned int w, h;
	int dba = 0, db = debug_scroll;
	int cache_index, next_index, valid;
	static int must_equal = -1;

	if (dba || db) {
		if (rec_data->category == XRecordFromClient) {
			req = (xCopyAreaReq *) rec_data->data;
			if (req->reqType == X_CopyArea) {
				src = req->srcDrawable;
				dst = req->dstDrawable;
			}
		}
	}

if (dba || db > 1) fprintf(stderr, "record_CA-%d id_base: 0x%lx  ptr: 0x%lx "
	"seq: 0x%lx rc: 0x%lx  cat: %d  swapped: %d 0x%lx/0x%lx\n", k++,
	rec_data->id_base, (unsigned long) ptr, xrecord_seq, rc_scroll,
	rec_data->category, rec_data->client_swapped, src, dst);

	if (! xrecording) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	if (rec_data->id_base == 0) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	if ((XID) ptr != xrecord_seq) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	if (rec_data->category != XRecordFromClient) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	req = (xCopyAreaReq *) rec_data->data;

	if (req->reqType != X_CopyArea) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	if (must_equal < 0) {
		must_equal = 0;
		if (getenv("X11VNC_SCROLL_MUST_EQUAL")) {
			must_equal = 1;
		}
	}

/*

xterm, gnome-terminal, others.

Note we miss the X_ImageText8 that clears the block cursor.  So there is a
short period of time with a painting error: two cursors, one above the other.

 X_ImageText8 
    draw: 0x8c00017 nChars: 1, gc: 0x8c00013, x: 101, y: 585, chars=' '
 X_ClearArea 
    window: 0x8c00018, x:   2, y: 217, w:  10, h:   5
 X_FillPoly 
    draw: 0x8c00018 gc: 0x8c0000a, shape: 0, coordMode: 0,
 X_FillPoly 
    draw: 0x8c00018 gc: 0x8c0000b, shape: 0, coordMode: 0,
 X_CopyArea 
    src: 0x8c00017, dst: 0x8c00017, gc: 0x8c00013, srcX:  17, srcY:  15, dstX:  17, dstY:   2, w: 480, h: 572
 X_ChangeWindowAttributes 
 X_ClearArea 
    window: 0x8c00017, x:  17, y: 574, w: 480, h:  13
 X_ChangeWindowAttributes 

 */

	src = req->srcDrawable;
	dst = req->dstDrawable;
	src_x = req->srcX;
	src_y = req->srcY;
	dst_x = req->dstX;
	dst_y = req->dstY;
	w = req->width;
	h = req->height;

	if (w*h < (unsigned int) scrollcopyrect_min_area) {
		if (db > 1) fprintf(stderr, "record_CA scroll area too small.\n");
		good = 0;
	} else if (!src || !dst) {
		if (db > 1) fprintf(stderr, "record_CA null src or dst.\n");
		good = 0;
	} else if (scr_ev_cnt >= SCR_EV_MAX) {
		if (db > 1) fprintf(stderr, "record_CA null too many scr events.\n");
		good = 0;
	} else if (must_equal && src != dst) {
		if (db > 1) fprintf(stderr, "record_CA src not equal dst.\n");
		good = 0;
	}

	if (src == dst) {
		dx = dst_x - src_x;
		dy = dst_y - src_y;

		if (dx != 0 && dy != 0) {
			good = 0;
		}
	}

if (!good && (dba || db > 1)) fprintf(stderr, "record_CA-x src_x: %d src_y: %d "
	"dst_x: %d dst_y: %d w: %d h: %d scr_ev_cnt: %d 0x%lx/0x%lx\n",
	src_x, src_y, dst_x, dst_y, w, h, scr_ev_cnt, src, dst);

	if (! good) {
		return;
	}

if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	/*
	 * after all of the above succeeds, now contact X server.
	 * we try to get away with some caching here.
	 */
	if (lookup_attr_cache(src, &cache_index, &next_index)) {
		i = cache_index;
		attr.x = scr_attr_cache[i].x;
		attr.y = scr_attr_cache[i].y;
		attr.width = scr_attr_cache[i].width;
		attr.height = scr_attr_cache[i].height;
		attr.map_state = scr_attr_cache[i].map_state;
		rx = scr_attr_cache[i].rx;
		ry = scr_attr_cache[i].ry;
		valid = scr_attr_cache[i].valid;

	} else {
		valid = valid_window(src, &attr, 1);

		if (valid) {
			if (!xtranslate(src, rootwin, 0, 0, &rx, &ry, &c, 1)) {
				valid = 0;
			}
		}
		if (next_index >= 0) {
			i = next_index;
			scr_attr_cache[i].win = src;
			scr_attr_cache[i].fetched = 1;
			scr_attr_cache[i].valid = valid;
			scr_attr_cache[i].time = dnow();
			if (valid) {
				scr_attr_cache[i].x = attr.x;
				scr_attr_cache[i].y = attr.y;
				scr_attr_cache[i].width = attr.width;
				scr_attr_cache[i].height = attr.height;
				scr_attr_cache[i].border_width = attr.border_width;
				scr_attr_cache[i].depth = attr.depth;
				scr_attr_cache[i].class = attr.class;
				scr_attr_cache[i].backing_store =
				    attr.backing_store;
				scr_attr_cache[i].map_state = attr.map_state;

				scr_attr_cache[i].rx = rx;
				scr_attr_cache[i].ry = ry;
			}
		}
	}

	if (! valid) {
		if (db > 1) fprintf(stderr, "record_CA not valid-1.\n");
		return;
	}
if (db > 1) fprintf(stderr, "record_CA-%d\n", k++);

	if (attr.map_state != IsViewable) {
		if (db > 1) fprintf(stderr, "record_CA not viewable-1.\n");
		return;
	}

	/* recent gdk/gtk windows use different src and dst. for compositing? */
	if (src != dst) {
	    if (lookup_attr_cache(dst, &cache_index, &next_index)) {
		i = cache_index;
		attr2.x = scr_attr_cache[i].x;
		attr2.y = scr_attr_cache[i].y;
		attr2.width = scr_attr_cache[i].width;
		attr2.height = scr_attr_cache[i].height;
		attr2.map_state = scr_attr_cache[i].map_state;
		rx2 = scr_attr_cache[i].rx;
		ry2 = scr_attr_cache[i].ry;
		valid = scr_attr_cache[i].valid;

	    } else {
		valid = valid_window(dst, &attr2, 1);

		if (valid) {
			if (!xtranslate(dst, rootwin, 0, 0, &rx2, &ry2, &c, 1)) {
				valid = 0;
			}
		}
		if (next_index >= 0) {
			i = next_index;
			scr_attr_cache[i].win = dst;
			scr_attr_cache[i].fetched = 1;
			scr_attr_cache[i].valid = valid;
			scr_attr_cache[i].time = dnow();
			if (valid) {
				scr_attr_cache[i].x = attr2.x;
				scr_attr_cache[i].y = attr2.y;
				scr_attr_cache[i].width = attr2.width;
				scr_attr_cache[i].height = attr2.height;
				scr_attr_cache[i].border_width = attr2.border_width;
				scr_attr_cache[i].depth = attr2.depth;
				scr_attr_cache[i].class = attr2.class;
				scr_attr_cache[i].backing_store =
				    attr2.backing_store;
				scr_attr_cache[i].map_state = attr2.map_state;

				scr_attr_cache[i].rx = rx2;
				scr_attr_cache[i].ry = ry2;
			}
		}
	    }

if (dba || db > 1) fprintf(stderr, "record_CA-? src_x: %d src_y: %d "
	"dst_x: %d dst_y: %d w: %d h: %d scr_ev_cnt: %d 0x%lx/0x%lx\n",
	src_x, src_y, dst_x, dst_y, w, h, scr_ev_cnt, src, dst);

		if (! valid) {
			if (db > 1) fprintf(stderr, "record_CA not valid-2.\n");
			return;
		}
		if (attr2.map_state != IsViewable) {
			if (db > 1) fprintf(stderr, "record_CA not viewable-2.\n");
			return;
		}
		dst_x = dst_x - (rx - rx2);
		dst_y = dst_y - (ry - ry2);

		dx = dst_x - src_x;
		dy = dst_y - src_y;

		if (dx != 0 && dy != 0) {
			return;
		}
	}


 if (0 || dba || db) {
	double st, dt;
	st = (double) rec_data->server_time/1000.0;
	dt = (dnow() - servertime_diff) - st;
	fprintf(stderr, "record_CA-%d *FOUND_SCROLL: src: 0x%lx dx: %d dy: %d "
	"x: %d y: %d w: %d h: %d st: %.4f %.4f  %.4f\n", k++, src, dx, dy,
	src_x, src_y, w, h, st, dt, dnowx());
 }

	i = scr_ev_cnt;

	scr_ev[i].win = src;
	scr_ev[i].frame = None;
	scr_ev[i].dx = dx;
	scr_ev[i].dy = dy;
	scr_ev[i].x = rx + dst_x;
	scr_ev[i].y = ry + dst_y;
	scr_ev[i].w = w;
	scr_ev[i].h = h;
	scr_ev[i].t = ((double) rec_data->server_time)/1000.0;
	scr_ev[i].win_x = rx;
	scr_ev[i].win_y = ry;
	scr_ev[i].win_w = attr.width;
	scr_ev[i].win_h = attr.height;
	scr_ev[i].new_x = 0;
	scr_ev[i].new_y = 0;
	scr_ev[i].new_w = 0;
	scr_ev[i].new_h = 0;

	if (dx == 0) {
		if (dy > 0) {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = ry + src_y;
			scr_ev[i].new_w = w;
			scr_ev[i].new_h = dy;
		} else {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = ry + dst_y + h;
			scr_ev[i].new_w = w;
			scr_ev[i].new_h = -dy;
		}
	} else if (dy == 0) {
		if (dx > 0) {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = rx + src_y;
			scr_ev[i].new_w = dx;
			scr_ev[i].new_h = h;
		} else {
			scr_ev[i].new_x = rx + dst_x + w;
			scr_ev[i].new_y = ry + src_y;
			scr_ev[i].new_w = -dx;
			scr_ev[i].new_h = h;
		}
	}

	scr_ev_cnt++;
}

typedef struct cw_event {
	Window win;
	int x, y, w, h;
} cw_event_t;

#define MAX_CW 128
static cw_event_t cw_events[MAX_CW];

static void record_CW(XPointer ptr, XRecordInterceptData *rec_data) {
	xConfigureWindowReq *req;
	Window win = None, c;
	Window src = None, dst = None;
	XWindowAttributes attr;
	int absent = 0x100000;
	int src_x, src_y, dst_x, dst_y, rx, ry;
	int good = 1, dx, dy, k=0, i, j, match, list[3];
	int f_x, f_y, f_w, f_h;
	int x, y, w, h;
	int x0, y0, w0, h0, x1, y1, w1, h1, x2, y2, w2, h2;
	static int index = 0;
	unsigned int vals[4];
	unsigned tmask;
	char *data;
	int dba = 0, db = debug_scroll;
	int cache_index, next_index, valid;

	if (db) {
		if (rec_data->category == XRecordFromClient) {
			req = (xConfigureWindowReq *) rec_data->data;
			if (req->reqType == X_ConfigureWindow) {
				src = req->window;
			}
		}
	}

if (dba || db > 1) fprintf(stderr, "record_CW-%d id_base: 0x%lx  ptr: 0x%lx "
	"seq: 0x%lx rc: 0x%lx  cat: %d  swapped: %d 0x%lx/0x%lx\n", k++,
	rec_data->id_base, (unsigned long) ptr, xrecord_seq, rc_scroll,
	rec_data->category, rec_data->client_swapped, src, dst);


	if (! xrecording) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if ((XID) ptr != xrecord_seq) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (rec_data->id_base == 0) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (rec_data->category == XRecordStartOfData) {
		index = 0;
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (rec_data->category != XRecordFromClient) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (rec_data->client_swapped) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	req = (xConfigureWindowReq *) rec_data->data;

	if (req->reqType != X_ConfigureWindow) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	tmask = req->mask;

	tmask &= ~CWX;
	tmask &= ~CWY;
	tmask &= ~CWWidth;
	tmask &= ~CWHeight;

	if (tmask) {
		/* require no more than these 4 flags */
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	f_x = req->mask & CWX;
	f_y = req->mask & CWY;
	f_w = req->mask & CWWidth;
	f_h = req->mask & CWHeight;

	if (! f_x || ! f_y)  {
		if (f_w && f_h) {
			;	/* netscape 4.x style */
		} else {
			return;
		}
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if ( (f_w && !f_h) || (!f_w && f_h) ) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);
		
	for (i=0; i<4; i++) {
		vals[i] = 0;
	}

	data = (char *)req;
	data += sz_xConfigureWindowReq;

	for (i=0; i<req->length; i++) {
		unsigned int v;
		/*
		 * We use unsigned int for the values.  There were
		 * some crashes on 64bit machines with unsigned longs.
		 * Need to check that X protocol sends 32bit values.
		 */
		v = *( (unsigned int *) data);
if (db > 1) fprintf(stderr, "  vals[%d]  0x%x/%d\n", i, v, v);
		vals[i] = v;
		data += sizeof(unsigned int);
	}

	if (index >= MAX_CW) {
		int i, j;

		/* FIXME, circular, etc. */
		for (i=0; i<2; i++) {
			j = MAX_CW - 2 + i;
			cw_events[i].win = cw_events[j].win;
			cw_events[i].x = cw_events[j].x;
			cw_events[i].y = cw_events[j].y;
			cw_events[i].w = cw_events[j].w;
			cw_events[i].h = cw_events[j].h;
		}
		index = 2;
	}

	if (! f_x && ! f_y) {
		/* netscape 4.x style  CWWidth,CWHeight */
		vals[2] = vals[0];
		vals[3] = vals[1];
		vals[0] = 0;
		vals[1] = 0;
	}

	cw_events[index].win = req->window;

	if (! f_x) {
		cw_events[index].x = absent;
	} else {
		cw_events[index].x = (int) vals[0];
	}
	if (! f_y) {
		cw_events[index].y = absent;
	} else {
		cw_events[index].y = (int) vals[1];
	}

	if (! f_w) {
		cw_events[index].w = absent;
	} else {
		cw_events[index].w = (int) vals[2];
	}
	if (! f_h) {
		cw_events[index].h = absent;
	} else {
		cw_events[index].h = (int) vals[3];
	}

	x = cw_events[index].x;
	y = cw_events[index].y;
	w = cw_events[index].w;
	h = cw_events[index].h;
	win = cw_events[index].win;

if (dba || db) fprintf(stderr, "  record_CW ind: %d win: 0x%lx x: %d y: %d w: %d h: %d\n",
	index, win, x, y, w, h);

	index++;

	if (index < 3) {
		good = 0;
	} else if (w != absent && h != absent &&
	    w*h < scrollcopyrect_min_area) {
		good = 0;
	}

	if (! good) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	match = 0;
	for (j=index - 1; j >= 0; j--) {
		if (cw_events[j].win == win) {
			list[match++] = j;
		}
		if (match >= 3) {
			break;
		}
	}

	if (match != 3) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

/*

Mozilla:

Up arrow: window moves down a bit (dy > 0):

 X_ConfigureWindow 
    length: 7, window: 0x2e000cd, mask: 0xf, v0 0,  v1 -18,  v2 760,  v3 906,  v4 327692,  v5 48234701,  v6 3, 
        CW-mask: CWX,CWY,CWWidth,CWHeight,
 X_ConfigureWindow 
    length: 5, window: 0x2e000cd, mask: 0x3, v0 0,  v1 0,  v2 506636,  v3 48234701,  v4 48234511, 
        CW-mask: CWX,CWY,
 X_ConfigureWindow 
    length: 7, window: 0x2e000cd, mask: 0xf, v0 0,  v1 0,  v2 760,  v3 888,  v4 65579,  v5 0,  v6 108009, 
        CW-mask: CWX,CWY,CWWidth,CWHeight,

Down arrow: window moves up a bit (dy < 0):

 X_ConfigureWindow 
    length: 7, window: 0x2e000cd, mask: 0xf, v0 0,  v1 0,  v2 760,  v3 906,  v4 327692,  v5 48234701,  v6 262147, 
        CW-mask: CWX,CWY,CWWidth,CWHeight,
 X_ConfigureWindow 
    length: 5, window: 0x2e000cd, mask: 0x3, v0 0,  v1 -18,  v2 506636,  v3 48234701,  v4 48234511, 
        CW-mask: CWX,CWY,
 X_ConfigureWindow 
    length: 7, window: 0x2e000cd, mask: 0xf, v0 0,  v1 0,  v2 760,  v3 888,  v4 96555,  v5 48265642,  v6 48265262, 
        CW-mask: CWX,CWY,CWWidth,CWHeight,


Netscape 4.x

Up arrow:
71.76142   0.01984 X_ConfigureWindow
    length: 7, window: 0x9800488, mask: 0xf, v0 0,  v1 -15,  v2 785,  v3 882,  v4 327692,  v5 159384712,  v6 1769484,
        CW-mask: CWX,CWY,CWWidth,CWHeight,
71.76153   0.00011 X_ConfigureWindow
    length: 5, window: 0x9800488, mask: 0xc, v0 785,  v1 867,  v2 329228,  v3 159384712,  v4 159383555,
        CW-mask:       CWWidth,CWHeight,
                XXX,XXX
71.76157   0.00003 X_ConfigureWindow
    length: 5, window: 0x9800488, mask: 0x3, v0 0,  v1 0,  v2 131132,  v3 159385313,  v4 328759,
        CW-mask: CWX,CWY,
                         XXX,XXX

Down arrow:
72.93147   0.01990 X_ConfigureWindow
    length: 5, window: 0x9800488, mask: 0xc, v0 785,  v1 882,  v2 328972,  v3 159384712,  v4 159383555,
        CW-mask:       CWWidth,CWHeight,
                XXX,XXX
72.93156   0.00009 X_ConfigureWindow
    length: 5, window: 0x9800488, mask: 0x3, v0 0,  v1 -15,  v2 458764,  v3 159384712,  v4 159383567,
        CW-mask: CWX,CWY,
72.93160   0.00004 X_ConfigureWindow
    length: 7, window: 0x9800488, mask: 0xf, v0 0,  v1 0,  v2 785,  v3 867,  v4 131132,  v5 159385335,  v6 328759,
        CW-mask: CWX,CWY,CWWidth,CWHeight,


sadly, probably need to handle some more...

 */
	x0 = cw_events[list[2]].x;
	y0 = cw_events[list[2]].y;
	w0 = cw_events[list[2]].w;
	h0 = cw_events[list[2]].h;

	x1 = cw_events[list[1]].x;
	y1 = cw_events[list[1]].y;
	w1 = cw_events[list[1]].w;
	h1 = cw_events[list[1]].h;

	x2 = cw_events[list[0]].x;
	y2 = cw_events[list[0]].y;
	w2 = cw_events[list[0]].w;
	h2 = cw_events[list[0]].h;

	/* see NS4 XXX's above: */
	if (w2 == absent || h2 == absent) {
		/* up arrow */
		if (w2 == absent) {
			w2 = w1;
		}
		if (h2 == absent) {
			h2 = h1;
		}
	}
	if (x1 == absent || y1 == absent) {
		/* up arrow */
		if (x1 == absent) {
			x1 = x2;
		}
		if (y1 == absent) {
			y1 = y2;
		}
	}
	if (x0 == absent || y0 == absent) {
		/* down arrow */
		if (x0 == absent) {
			/* hmmm... what to do */
			x0 = x2;
		}
		if (y0 == absent) {
			y0 = y2;
		}
	}

if (dba) fprintf(stderr, "%d/%d/%d/%d  %d/%d/%d/%d  %d/%d/%d/%d\n", x0, y0, w0, h0, x1, y1, w1, h1, x2, y2, w2, h2);

	dy = y1 - y0;
	dx = x1 - x0;

	src_x = x2;
	src_y = y2;
	w = w2;
	h = h2;

	/* check w and h before we modify them */
	if (w <= 0 || h <= 0) {
		good = 0;
	} else if (w == absent || h == absent) {
		good = 0;
	}
	if (! good) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (dy > 0) {
		h -= dy;	
	} else {
		h += dy;	
		src_y -= dy;
	}
	if (dx > 0) {
		w -= dx;	
	} else {
		w += dx;	
		src_x -= dx;
	}

	dst_x = src_x + dx;
	dst_y = src_y + dy;

	if (x0 == absent || x1 == absent || x2 == absent) {
		good = 0;
	} else if (y0 == absent || y1 == absent || y2 == absent) {
		good = 0;
	} else if (dx != 0 && dy != 0) {
		good = 0;
	} else if (w0 - w2 != nabs(dx)) {
		good = 0;
	} else if (h0 - h2 != nabs(dy)) {
		good = 0;
	} else if (scr_ev_cnt >= SCR_EV_MAX) {
		good = 0;
	}

	if (! good) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	/*
	 * geometry OK.
	 * after all of the above succeeds, now contact X server.
	 */
	if (lookup_attr_cache(win, &cache_index, &next_index)) {
		i = cache_index;
		attr.x = scr_attr_cache[i].x;
		attr.y = scr_attr_cache[i].y;
		attr.width = scr_attr_cache[i].width;
		attr.height = scr_attr_cache[i].height;
		attr.map_state = scr_attr_cache[i].map_state;
		rx = scr_attr_cache[i].rx;
		ry = scr_attr_cache[i].ry;
		valid = scr_attr_cache[i].valid;

if (0) fprintf(stderr, "lookup_attr_cache hit:  %2d %2d 0x%lx %d\n",
    cache_index, next_index, win, valid);

	} else {
		valid = valid_window(win, &attr, 1);

if (0) fprintf(stderr, "lookup_attr_cache MISS: %2d %2d 0x%lx %d\n",
    cache_index, next_index, win, valid);

		if (valid) {
			if (!xtranslate(win, rootwin, 0, 0, &rx, &ry, &c, 1)) {
				valid = 0;
			}
		}
		if (next_index >= 0) {
			i = next_index;
			scr_attr_cache[i].win = win;
			scr_attr_cache[i].fetched = 1;
			scr_attr_cache[i].valid = valid;
			scr_attr_cache[i].time = dnow();
			if (valid) {
				scr_attr_cache[i].x = attr.x;
				scr_attr_cache[i].y = attr.y;
				scr_attr_cache[i].width = attr.width;
				scr_attr_cache[i].height = attr.height;
				scr_attr_cache[i].depth = attr.depth;
				scr_attr_cache[i].class = attr.class;
				scr_attr_cache[i].backing_store =
				    attr.backing_store;
				scr_attr_cache[i].map_state = attr.map_state;

				scr_attr_cache[i].rx = rx;
				scr_attr_cache[i].ry = ry;
			}
		}
	}

	if (! valid) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

	if (attr.map_state != IsViewable) {
		return;
	}
if (db > 1) fprintf(stderr, "record_CW-%d\n", k++);

 if (0 || dba || db) {
	double st, dt;
	st = (double) rec_data->server_time/1000.0;
	dt = (dnow() - servertime_diff) - st;
	fprintf(stderr, "record_CW-%d *FOUND_SCROLL: win: 0x%lx dx: %d dy: %d "
	"x: %d y: %d w: %d h: %d  st: %.4f  dt: %.4f  %.4f\n", k++, win,
	dx, dy, src_x, src_y, w, h, st, dt, dnowx());
 }

	i = scr_ev_cnt;

	scr_ev[i].win = win;
	scr_ev[i].frame = None;
	scr_ev[i].dx = dx;
	scr_ev[i].dy = dy;
	scr_ev[i].x = rx + dst_x;
	scr_ev[i].y = ry + dst_y;
	scr_ev[i].w = w;
	scr_ev[i].h = h;
	scr_ev[i].t = ((double) rec_data->server_time)/1000.0;
	scr_ev[i].win_x = rx;
	scr_ev[i].win_y = ry;
	scr_ev[i].win_w = attr.width;
	scr_ev[i].win_h = attr.height;
	scr_ev[i].new_x = 0;
	scr_ev[i].new_y = 0;
	scr_ev[i].new_w = 0;
	scr_ev[i].new_h = 0;

	if (dx == 0) {
		if (dy > 0) {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = ry + src_y;
			scr_ev[i].new_w = w;
			scr_ev[i].new_h = dy;
		} else {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = ry + dst_y + h;
			scr_ev[i].new_w = w;
			scr_ev[i].new_h = -dy;
		}
	} else if (dy == 0) {
		if (dx > 0) {
			scr_ev[i].new_x = rx + src_x;
			scr_ev[i].new_y = rx + src_y;
			scr_ev[i].new_w = dx;
			scr_ev[i].new_h = h;
		} else {
			scr_ev[i].new_x = rx + dst_x + w;
			scr_ev[i].new_y = ry + src_y;
			scr_ev[i].new_w = -dx;
			scr_ev[i].new_h = h;
		}
	}

	/* indicate we have a new one */
	scr_ev_cnt++;

	index = 0;
}

static void record_switch(XPointer ptr, XRecordInterceptData *rec_data) {
	static int first = 1;
	xReq *req;

	if (first) {
		int i;
		for (i=0; i<SCR_ATTR_CACHE; i++) {
			scr_attr_cache[i].win = None;
			scr_attr_cache[i].fetched = 0;
			scr_attr_cache[i].valid = 0;
			scr_attr_cache[i].time = 0.0;
		}
		first = 0;
	}

	/* should handle control msgs, start/stop/etc */
	if (rec_data->category == XRecordStartOfData) {
		record_CW(ptr, rec_data);
	} else if (rec_data->category == XRecordEndOfData) {
		;
	} else if (rec_data->category == XRecordClientStarted) {
		;
	} else if (rec_data->category == XRecordClientDied) {
		;
	} else if (rec_data->category == XRecordFromServer) {
		;
	}

	if (rec_data->category != XRecordFromClient) {
		XRecordFreeData(rec_data);
		return;
	}

	req = (xReq *) rec_data->data;

	if (req->reqType == X_CopyArea) {
		record_CA(ptr, rec_data);
	} else if (req->reqType == X_ConfigureWindow) {
		record_CW(ptr, rec_data);
	} else {
		;
	}
	XRecordFreeData(rec_data);
}

static void record_grab(XPointer ptr, XRecordInterceptData *rec_data) {
	xReq *req;
	int db = 0;

	if (debug_grabs) db = 1;

	/* should handle control msgs, start/stop/etc */
	if (rec_data->category == XRecordStartOfData) {
		;
	} else if (rec_data->category == XRecordEndOfData) {
		;
	} else if (rec_data->category == XRecordClientStarted) {
		;
	} else if (rec_data->category == XRecordClientDied) {
		;
	} else if (rec_data->category == XRecordFromServer) {
		;
	}

	if (rec_data->category != XRecordFromClient) {
		XRecordFreeData(rec_data);
		return;
	}

	req = (xReq *) rec_data->data;

	if (req->reqType == X_GrabServer) {
		double now = dnowx();
		xserver_grabbed++;
		if (db) rfbLog("X server Grabbed:    %d %.5f\n", xserver_grabbed, now);
		if (xserver_grabbed > 1) {
			/* 
			 * some apps do multiple grabs... very unlikely
			 * two apps will be doing it at same time.
			 */
			xserver_grabbed = 1;
		}
	} else if (req->reqType == X_UngrabServer) {
		double now = dnowx();
		xserver_grabbed--;
		if (xserver_grabbed < 0) {
			xserver_grabbed = 0;
		}
		if (db) rfbLog("X server Un-Grabbed: %d %.5f\n", xserver_grabbed, now);
	} else {
		;
	}
	XRecordFreeData(rec_data);

	/* unused vars warning: */
	if (ptr) {}
}
#endif

static void check_xrecord_grabserver(void) {
#if LIBVNCSERVER_HAVE_RECORD
	int last_val, cnt = 0, i, max = 10;
	double d;
	if (!gdpy_ctrl || !gdpy_data) {
		return;
	}
	if (unixpw_in_progress) return;

	dtime0(&d);
	XFlush_wr(gdpy_ctrl);
	for (i=0; i<max; i++) {
		last_val = xserver_grabbed;
		XRecordProcessReplies(gdpy_data);
		if (xserver_grabbed != last_val) {
			cnt++;
		} else if (i > 2) {
			break;
		}
	}
	if (cnt) {
		XFlush_wr(gdpy_ctrl);
	}
 if (debug_grabs && cnt > 0) {
	d = dtime(&d);
fprintf(stderr, "check_xrecord_grabserver: cnt=%d i=%d %.4f\n", cnt, i, d);
 }
#endif
}

#if LIBVNCSERVER_HAVE_RECORD
static void shutdown_record_context(XRecordContext rc, int bequiet, int reopen) {
	int ret1, ret2;
	int verb = (!bequiet && !quiet);

	RAWFB_RET_VOID
	if (0 || debug_scroll) {
		rfbLog("shutdown_record_context(0x%lx, %d, %d)\n", rc,
		    bequiet, reopen);
		verb = 1;
	}

	ret1 = XRecordDisableContext(rdpy_ctrl, rc);
	if (!ret1 && verb) {
		rfbLog("XRecordDisableContext(0x%lx) failed.\n", rc);	
	}
	ret2 = XRecordFreeContext(rdpy_ctrl, rc);
	if (!ret2 && verb) {
		rfbLog("XRecordFreeContext(0x%lx) failed.\n", rc);	
	}
	XFlush_wr(rdpy_ctrl);

	if (reopen == 2 && ret1 && ret2) {
		reopen = 0;	/* 2 means reopen only on failure  */
	}
	if (reopen && gdpy_ctrl) {
		check_xrecord_grabserver();
		if (xserver_grabbed) {
			rfbLog("shutdown_record_context: skip reopen,"
			    " server grabbed\n");	
			reopen = 0;
		}
	}
	if (reopen) {
		char *dpystr = DisplayString(dpy);

		if (debug_scroll) {
			rfbLog("closing RECORD data connection.\n");
		}
		XCloseDisplay_wr(rdpy_data);
		rdpy_data = NULL;

		if (debug_scroll) {
			rfbLog("closing RECORD control connection.\n");
		}
		XCloseDisplay_wr(rdpy_ctrl);
		rdpy_ctrl = NULL;

		rdpy_ctrl = XOpenDisplay_wr(dpystr);

		if (! rdpy_ctrl) {
			rfbLog("Failed to reopen RECORD control connection:"
			    "%s\n", dpystr);
			rfbLog("  disabling RECORD scroll detection.\n");
			use_xrecord = 0;
			return;
		}
		XSync(dpy, False);

		disable_grabserver(rdpy_ctrl, 0);
		XSync(rdpy_ctrl, True);

		rdpy_data = XOpenDisplay_wr(dpystr);

		if (! rdpy_data) {
			rfbLog("Failed to reopen RECORD data connection:"
			    "%s\n", dpystr);
			rfbLog("  disabling RECORD scroll detection.\n");
			XCloseDisplay_wr(rdpy_ctrl);
			rdpy_ctrl = NULL;
			use_xrecord = 0;
			return;
		}
		disable_grabserver(rdpy_data, 0);

		if (debug_scroll || (! bequiet && reopen == 2)) {
			rfbLog("reopened RECORD data and control display"
			    " connections: %s\n", dpystr);
		}
	}
}
#endif

void check_xrecord_reset(int force) {
	static double last_reset = 0.0;
	int reset_time  = 60, require_idle  = 10;
	int reset_time2 = 600, require_idle2 = 40;
	double now = 0.0;
	XErrorHandler old_handler = NULL;

	if (gdpy_ctrl) {
		X_LOCK;
		check_xrecord_grabserver();
		X_UNLOCK;
	} else {
		/* more dicey if not watching grabserver */
		reset_time = reset_time2;
		require_idle = require_idle2;
	}

	if (!use_xrecord) {
		return;
	}
	if (xrecording) {
		return;
	}
	if (button_mask) {
		return;
	}
	if (xserver_grabbed) {
		return;
	}

	if (unixpw_in_progress) return;

#if LIBVNCSERVER_HAVE_RECORD
	if (! rc_scroll) {
		return;
	}
	now = dnow();
	if (last_reset == 0.0) {
		last_reset = now;
		return;
	}
	/*
	 * try to wait for a break in input to reopen the displays
	 * this is only to avoid XGrabServer deadlock on the repopens.
	 */
	if (force) {
		;
	} else if (now < last_reset + reset_time) {
		return;
	} else if (now < last_pointer_click_time + require_idle)  {
		return;
	} else if (now < last_keyboard_time + require_idle)  {
		return;
	}
	X_LOCK;
	trapped_record_xerror = 0;
	old_handler = XSetErrorHandler(trap_record_xerror);

	/* unlikely, but check again since we will definitely be doing it. */
	if (gdpy_ctrl) {
		check_xrecord_grabserver();
		if (xserver_grabbed) {
			XSetErrorHandler(old_handler);
			X_UNLOCK;
			return;
		}
	}
	
	shutdown_record_context(rc_scroll, 0, 1);
	rc_scroll = 0;

	XSetErrorHandler(old_handler);
	X_UNLOCK;

	last_reset = now;
#else
	if (!old_handler || now == 0.0 || !last_reset || !force) {}
#endif
}

#define RECORD_ERROR_MSG(tag) \
	if (! quiet) { \
		static int cnt = 0; \
		static time_t last = 0; \
		int show = 0; \
		cnt++; \
		if (debug_scroll || cnt < 20) { \
			show = 1; \
		} else if (cnt == 20) { \
			last = time(NULL); \
			rfbLog("disabling RECORD XError messages for 600s\n"); \
			show = 1; \
		} else if (time(NULL) > last + 600) { \
			cnt = 0; \
			show = 1; \
		} \
		if (show) { \
			rfbLog("trapped RECORD XError: %s %s %d/%d/%d (0x%lx)\n", \
			    tag, xerror_string(trapped_record_xerror_event), \
			    (int) trapped_record_xerror_event->error_code, \
			    (int) trapped_record_xerror_event->request_code, \
			    (int) trapped_record_xerror_event->minor_code, \
			    (int) trapped_record_xerror_event->resourceid); \
		} \
	}

void xrecord_watch(int start, int setby) {
#if LIBVNCSERVER_HAVE_RECORD
	Window focus, wm, c, clast;
	static double create_time = 0.0;
	int rc;
	int do_shutdown = 0;
	int reopen_dpys = 1;
	XErrorHandler old_handler = NULL;
	static Window last_win = None, last_result = None;
#endif
	int db = debug_scroll;
	double now;
	static double last_error = 0.0;

if (0) db = 1;

	if (nofb) {
		xrecording = 0;
		return;
	}
	if (use_threads) {
		/* XXX not working.  Still?  Painting errors. */
		static int first = 1;
		if (first) {
			if (use_xrecord && !getenv("XRECORD_THREADS")) {
				rfbLog("xrecord_watch: disabling scroll detection in -threads mode.\n");
				rfbLog("xrecord_watch: Set -env XRECORD_THREADS=1 to enable it.\n");
				use_xrecord = 0;
				xrecording = 0;
			}
			first = 0;
		}
		if (!use_xrecord && !xrecording) {
			return;
		}
	}

	dtime0(&now);
	if (now < last_error + 0.5) {
		return;
	}

	if (gdpy_ctrl) {
		X_LOCK;
		check_xrecord_grabserver();
		X_UNLOCK;
		if (xserver_grabbed) {
if (db || debug_grabs) fprintf(stderr, "xrecord_watch: %d/%d  out xserver_grabbed\n", start, setby);
			return;
		}
	}

#if LIBVNCSERVER_HAVE_RECORD
	if (! start) {
		int shut_reopen = 2, shut_time = 25;
if (db || debug_grabs) fprintf(stderr, "XRECORD OFF: %d/%d  %.4f\n", xrecording, setby, now - x11vnc_start);
		xrecording = 0;
		if (! rc_scroll) {
			xrecord_focus_window = None;
			xrecord_wm_window = None;
			xrecord_ptr_window = None;
			xrecord_keysym = NoSymbol;
			rcs_scroll = 0;
			return;
		}

		if (! do_shutdown && now > create_time + shut_time) {
			/* XXX unstable if we keep a RECORD going forever */
			do_shutdown = 1;
		}

		SCR_LOCK;
		
		if (do_shutdown) {
if (db > 1) fprintf(stderr, "=== shutdown-scroll 0x%lx\n", rc_scroll);
			X_LOCK;
			trapped_record_xerror = 0;
			old_handler = XSetErrorHandler(trap_record_xerror);

			shutdown_record_context(rc_scroll, 0, shut_reopen);
			rc_scroll = 0;

			/*
			 * n.b. there is a grabserver issue wrt
			 * XRecordCreateContext() even though rdpy_ctrl
			 * is set imprevious to grabs.  Perhaps a bug
			 * in the X server or library...
			 *
			 * If there are further problems, a thought
			 * to recreate rc_scroll right after the
			 * reopen.
			 */

			if (! use_xrecord) {
				XSetErrorHandler(old_handler);
				X_UNLOCK;
				SCR_UNLOCK;
				return;
			}

			XRecordProcessReplies(rdpy_data);

			if (trapped_record_xerror) {
				RECORD_ERROR_MSG("shutdown");
				last_error = now;
			}

			XSetErrorHandler(old_handler);
			X_UNLOCK;
			SCR_UNLOCK;

		} else {
			if (rcs_scroll) {
if (db > 1) fprintf(stderr, "=== disab-scroll 0x%lx 0x%lx\n", rc_scroll, rcs_scroll);
				X_LOCK;
				trapped_record_xerror = 0;
				old_handler =
				    XSetErrorHandler(trap_record_xerror);

				rcs_scroll = XRecordCurrentClients;
				XRecordUnregisterClients(rdpy_ctrl, rc_scroll,
				    &rcs_scroll, 1);
				XRecordDisableContext(rdpy_ctrl, rc_scroll);
				XFlush_wr(rdpy_ctrl);
				XRecordProcessReplies(rdpy_data);

				if (trapped_record_xerror) {
					RECORD_ERROR_MSG("disable");

					shutdown_record_context(rc_scroll,
					    0, reopen_dpys);
					rc_scroll = 0;

					last_error = now;

					if (! use_xrecord) {
						XSetErrorHandler(old_handler);
						X_UNLOCK;
						SCR_UNLOCK;
						return;
					}
				}
				XSetErrorHandler(old_handler);
				X_UNLOCK;
			}
		}

		SCR_UNLOCK;
		/*
		 * XXX if we do a XFlush_wr(rdpy_ctrl) here we get:
		 *

		X Error of failed request:  XRecordBadContext
		  Major opcode of failed request:  145 (RECORD)
		  Minor opcode of failed request:  5 (XRecordEnableContext)
		  Context in failed request:  0x2200013
		  Serial number of failed request:  29
		  Current serial number in output stream:  29

		 *
		 * need to figure out what is going on... since it may lead
		 * infrequent failures.
		 */
		xrecord_focus_window = None;
		xrecord_wm_window = None;
		xrecord_ptr_window = None;
		xrecord_keysym = NoSymbol;
		rcs_scroll = 0;
		return;
	}
if (db || debug_grabs) fprintf(stderr, "XRECORD ON:  %d/%d  %.4f\n", xrecording, setby, now - x11vnc_start);

	if (xrecording) {
		return;
	}

	if (do_shutdown && rc_scroll) {
		static int didmsg = 0;
		/* should not happen... */
		if (0 || !didmsg) {
			rfbLog("warning: do_shutdown && rc_scroll\n");
			didmsg = 1;
		}
		xrecord_watch(0, SCR_NONE);
	}

	xrecording = 0;
	xrecord_focus_window = None;
	xrecord_wm_window = None;
	xrecord_ptr_window = None;
	xrecord_keysym = NoSymbol;
	xrecord_set_by_keys  = 0;
	xrecord_set_by_mouse = 0;

	/* get the window with focus and mouse pointer: */
	clast = None;
	focus = None;
	wm = None;

	X_LOCK;
	SCR_LOCK;
#if 0
	/*
	 * xrecord_focus_window / focus not currently used... save a
	 * round trip to the X server for now.
	 * N.B. our heuristic is inaccurate: if he is scrolling and
	 * drifts off of the scrollbar onto another application we
	 * will catch that application, not the starting ones.
	 * check_xrecord_{keys,mouse} mitigates this somewhat by
	 * delaying calls to xrecord_watch as much as possible.
	 */
	XGetInputFocus(dpy, &focus, &i);
#endif

	wm = query_pointer(rootwin);
	if (wm) {
		c = wm;
	} else {
		c = rootwin;
	}

	/* descend a bit to avoid wm frames: */
	if (c != rootwin && c == last_win) {
		/* use cached results to avoid roundtrips: */
		clast = last_result;
	} else if (scroll_good_all == NULL && scroll_skip_all == NULL) {
		/* more efficient if name info not needed. */
		xrecord_name_info[0] = '\0';
		clast = descend_pointer(6, c, NULL, 0);
	} else {
		char *nm;
		int matched_good = 0, matched_skip = 0;

		clast = descend_pointer(6, c, xrecord_name_info, NAMEINFO);
if (db) fprintf(stderr, "name_info: %s\n", xrecord_name_info);

		nm = xrecord_name_info;

		if (scroll_good_all) {
			matched_good += match_str_list(nm, scroll_good_all);
		}
		if (setby == SCR_KEY && scroll_good_key) {
			matched_good += match_str_list(nm, scroll_good_key);
		}
		if (setby == SCR_MOUSE && scroll_good_mouse) {
			matched_good += match_str_list(nm, scroll_good_mouse);
		}
		if (scroll_skip_all) {
			matched_skip += match_str_list(nm, scroll_skip_all);
		}
		if (setby == SCR_KEY && scroll_skip_key) {
			matched_skip += match_str_list(nm, scroll_skip_key);
		}
		if (setby == SCR_MOUSE && scroll_skip_mouse) {
			matched_skip += match_str_list(nm, scroll_skip_mouse);
		}

		if (!matched_good && matched_skip) {
			clast = None;
		}
	}
	if (c != rootwin) {
		/* cache results for possible use next call */
		last_win = c;
		last_result = clast;
	}

	if (!clast || clast == rootwin) {
if (db) fprintf(stderr, "--- xrecord_watch: SKIP.\n");
		X_UNLOCK;
		SCR_UNLOCK;
		return;
	}

	/* set protocol request ranges: */
	rr_scroll[0] = rr_CA;
	rr_scroll[1] = rr_CW;

	/*
	 * start trapping... there still are some occasional failures
	 * not yet understood, likely some race condition WRT the 
	 * context being setup.
	 */
	trapped_record_xerror = 0;
	old_handler = XSetErrorHandler(trap_record_xerror);

	if (! rc_scroll) {
		/* do_shutdown case or first time in */

		if (gdpy_ctrl) {
			/*
			 * Even though rdpy_ctrl is impervious to grabs
			 * at this point, we still get deadlock, why?
			 * It blocks in the library find_display() call.
			 */
			check_xrecord_grabserver();
			if (xserver_grabbed) {
				XSetErrorHandler(old_handler);
				X_UNLOCK;
				SCR_UNLOCK;
				return;
			}
		}
		rcs_scroll = (XRecordClientSpec) clast;
		rc_scroll = XRecordCreateContext(rdpy_ctrl, 0, &rcs_scroll, 1,
		    rr_scroll, 2);

		if (! do_shutdown) {
			XSync(rdpy_ctrl, False);
		}
if (db) fprintf(stderr, "NEW rc:    0x%lx\n", rc_scroll);
		if (rc_scroll) {
			dtime0(&create_time);
		} else {
			rcs_scroll = 0;
		}

	} else if (! do_shutdown) {
		if (rcs_scroll) {
			/*
			 * should have been unregistered in xrecord_watch(0)...
			 */
			rcs_scroll = XRecordCurrentClients;
			XRecordUnregisterClients(rdpy_ctrl, rc_scroll,
			    &rcs_scroll, 1);

if (db > 1) fprintf(stderr, "=2= unreg-scroll 0x%lx 0x%lx\n", rc_scroll, rcs_scroll);

		}
		
		rcs_scroll = (XRecordClientSpec) clast;

if (db > 1) fprintf(stderr, "=-=   reg-scroll 0x%lx 0x%lx\n", rc_scroll, rcs_scroll);

		if (!XRecordRegisterClients(rdpy_ctrl, rc_scroll, 0,
		    &rcs_scroll, 1, rr_scroll, 2)) {
			if (1 || now > last_error + 60) {
				rfbLog("failed to register client 0x%lx with"
				    " X RECORD context rc_scroll.\n", clast);
			}
			last_error = now;
			rcs_scroll = 0;
			/* continue on for now... */
		}
	}

	XFlush_wr(rdpy_ctrl);

if (db) fprintf(stderr, "rc_scroll: 0x%lx\n", rc_scroll);
	if (trapped_record_xerror) {
		RECORD_ERROR_MSG("register");
	}

	if (! rc_scroll) {
		XSetErrorHandler(old_handler);
		X_UNLOCK;
		SCR_UNLOCK;
		use_xrecord = 0;
		rfbLog("failed to create X RECORD context rc_scroll.\n");
		rfbLog("  switching to -noscrollcopyrect mode.\n");
		return;
	} else if (! rcs_scroll || trapped_record_xerror) {
		/* try again later */
		shutdown_record_context(rc_scroll, 0, reopen_dpys);
		rc_scroll = 0;
		last_error = now;

		XSetErrorHandler(old_handler);
		X_UNLOCK;
		SCR_UNLOCK;
		return;
	}

	xrecord_focus_window = focus;
#if 0
	/* xrecord_focus_window currently unused. */
	if (! xrecord_focus_window) {
		xrecord_focus_window = clast;
	}
#endif
	xrecord_wm_window = wm;
	if (! xrecord_wm_window) {
		xrecord_wm_window = clast;
	}

	xrecord_ptr_window = clast;

	xrecording = 1;
	xrecord_seq++;
	dtime0(&xrecord_start);

	rc = XRecordEnableContextAsync(rdpy_data, rc_scroll, record_switch,
	    (XPointer) xrecord_seq);

	if (!rc || trapped_record_xerror) {
		if (1 || now > last_error + 60) {
			rfbLog("failed to enable RECORD context "
			    "rc_scroll: 0x%lx rc: %d\n", rc_scroll, rc);
			if (trapped_record_xerror) {
				RECORD_ERROR_MSG("enable-failed");
			}
		}
		shutdown_record_context(rc_scroll, 0, reopen_dpys);
		rc_scroll = 0;
		last_error = now;
		xrecording = 0;
		/* continue on for now... */
	}
	XSetErrorHandler(old_handler);

	/* XXX this may cause more problems than it solves... */
	if (use_xrecord) {
		XFlush_wr(rdpy_data);
	}

	X_UNLOCK;
	SCR_UNLOCK;
#endif
}


