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

/* -- macosx.c -- */

#include "rfb/rfbconfig.h"
#if (defined(__MACH__) && defined(__APPLE__) && defined(LIBVNCSERVER_HAVE_MACOSX_NATIVE_DISPLAY))

#define DOMAC 1

#else

#define DOMAC 0

#endif

#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "screen.h"
#include "pointer.h"
#include "allowed_input_t.h"
#include "keyboard.h"
#include "cursor.h"
#include "connections.h"
#include "macosxCG.h"
#include "macosxCGP.h"
#include "macosxCGS.h"

void macosx_log(char *);
char *macosx_console_guess(char *str, int *fd);
void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client);
char *macosx_get_fb_addr(void);
int macosx_get_cursor(void);
int macosx_get_cursor_pos(int *, int *);
void macosx_send_sel(char *, int);
void macosx_set_sel(char *, int);
int macosx_valid_window(Window, XWindowAttributes*);

Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return);
int macosx_get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win);

void macosx_add_mapnotify(Window win, int level, int map);
void macosx_add_create(Window win, int level);
void macosx_add_destroy(Window win, int level);
void macosx_add_visnotify(Window win, int level, int obscured);
int macosx_checkevent(XEvent *ev);

void macosx_log(char *str) {
	rfbLog(str);
}

#if (! DOMAC)

void macosx_event_loop(void) {
	return;
}
char *macosx_console_guess(char *str, int *fd) {
	if (!str || !fd) {}
	return NULL;
}
void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	if (!down || !keysym || !client) {}
	return;
}
void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	if (!mask || !x || !y || !client) {}
	return;
}
char *macosx_get_fb_addr(void) {
	return NULL;
}
int macosx_get_cursor(void) {
	return 0;
}
int macosx_get_cursor_pos(int *x, int *y) {
	if (!x || !y) {}
	return 0;
}
void macosx_send_sel(char * str, int len) {
	if (!str || !len) {}
	return;
}
void macosx_set_sel(char * str, int len) {
	if (!str || !len) {}
	return;
}
int macosx_valid_window(Window w, XWindowAttributes* a) {
	if (!w || !a) {}
	return 0;
}
Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return) {
	if (!w || !root_return || !parent_return || !children_return || !nchildren_return) {}
	return (Status) 0;
}
void macosx_add_mapnotify(Window win, int level, int map) {
	if (!win || !level || !map) {}
	return;
}
void macosx_add_create(Window win, int level) {
	if (!win || !level) {}
	return;
}
void macosx_add_destroy(Window win, int level) {
	if (!win || !level) {}
	return;
}
void macosx_add_visnotify(Window win, int level, int obscured) {
	if (!win || !level || !obscured) {}
	return;
}

int macosx_checkevent(XEvent *ev) {
	if (!ev) {}
	return 0;
}


#else 

void macosx_event_loop(void) {
	macosxCG_event_loop();
}

char *macosx_get_fb_addr(void) {
	macosxCG_init();
	return macosxCG_get_fb_addr();
}

int macosx_opengl_get_width(void);
int macosx_opengl_get_height(void);
int macosx_opengl_get_bpp(void);
int macosx_opengl_get_bps(void);
int macosx_opengl_get_spp(void);

char *macosx_console_guess(char *str, int *fd) {
	char *q, *in = strdup(str);
	char *atparms = NULL, *file = NULL;

	macosxCG_init();

	if (strstr(in, "console") != in) {
		rfbLog("console_guess: unrecognized console/fb format: %s\n", str);
		free(in);
		return NULL;
	}

	*fd = -1;

	q = strrchr(in, '@');
	if (q) {
		atparms = strdup(q+1);
		*q = '\0';
	}
	q = strrchr(in, ':');
	if (q) {
		file = strdup(q+1);
		*q = '\0';
	}
	if (! file || file[0] == '\0')  {
		file = strdup("/dev/null");
	}
	rfbLog("console_guess: file is %s\n", file);

	if (! pipeinput_str) {
		pipeinput_str = strdup("MACOSX");
		initialize_pipeinput();
	}

	if (! atparms) {
		int w, h, b, bps, dep;
		unsigned long rm = 0, gm = 0, bm = 0;

		if (macosx_read_opengl) {
			w = macosx_opengl_get_width();
			h = macosx_opengl_get_height();
			b = macosx_opengl_get_bpp();

			bps = macosx_opengl_get_bps();
			dep = macosx_opengl_get_spp() * bps;
			
		} else {
			w = macosxCG_CGDisplayPixelsWide();
			h = macosxCG_CGDisplayPixelsHigh();
			b = macosxCG_CGDisplayBitsPerPixel();

			bps = macosxCG_CGDisplayBitsPerSample();
			dep = macosxCG_CGDisplaySamplesPerPixel() * bps;
		}

		rm = (1 << bps) - 1;
		gm = (1 << bps) - 1;
		bm = (1 << bps) - 1;
		rm = rm << 2 * bps;
		gm = gm << 1 * bps;
		bm = bm << 0 * bps;

		if (b == 8 && rm == 0xff && gm == 0xff && bm == 0xff) {
			/* I don't believe it... */
			rm = 0x07;
			gm = 0x38;
			bm = 0xc0;
		}
		
		/* @66666x66666x32:0xffffffff:... */
		atparms = (char *) malloc(200);
		sprintf(atparms, "%dx%dx%d:%lx/%lx/%lx", w, h, b, rm, gm, bm);
	}
	if (atparms) {
		int gw, gh, gb;
		if (sscanf(atparms, "%dx%dx%d", &gw, &gh, &gb) == 3)  {
			fb_x = gw;	
			fb_y = gh;	
			fb_b = gb;	
		}
	}
	if (! atparms) {
		rfbLog("console_guess: could not get @ parameters.\n");
		return NULL;
	}

	q = (char *) malloc(strlen("map:macosx:") + strlen(file) + 1 + strlen(atparms) + 1);
	sprintf(q, "map:macosx:%s@%s", file, atparms);
	free(atparms);
	return q;
}

Window macosx_click_frame = None;

void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	allowed_input_t input;
	static int last_mask = 0;
	int rc;

	if (0) fprintf(stderr, "macosx_pointer_command: %d %d - %d\n", x, y, mask);

	if (mask >= 0) {
		got_pointer_calls++;
	}

	if (view_only) {
		return;
	}

	get_allowed_input(client, &input);

	if (! input.motion || ! input.button) {
		/* XXX fix me with last_x, last_y, etc. */
		return;
	}

	if (mask >= 0) {
		got_user_input++;
		got_pointer_input++;
		last_pointer_client = client;
		last_pointer_time = time(NULL);
	}
	if (last_mask != mask) {
		if (0) fprintf(stderr, "about to inject mask change %d -> %d: %.4f\n", last_mask, mask, dnowx());
		if (mask) {
			int px, py, x, y, w, h;
			macosx_click_frame = None;
			if (!macosx_get_wm_frame_pos(&px, &py, &x, &y, &w, &h, &macosx_click_frame, NULL)) {
				macosx_click_frame = None;
			}
		}
	}

	macosxCG_pointer_inject(mask, x, y);

	if (cursor_x != x || cursor_y != y) {
		last_pointer_motion_time = dnow();
	}

	cursor_x = x;
	cursor_y = y;

	if (last_mask != mask) {
		last_pointer_click_time = dnow();
		if (ncache > 0) {
			/* XXX Y */
			int i;
if (0) fprintf(stderr, "about to get all windows:           %.4f\n", dnowx());
			for (i=0; i < 2; i++) {
				macosxCGS_get_all_windows();
				if (0) fprintf(stderr, "!");
				if (macosx_checkevent(NULL)) {
					break;
				}
			}
if (0) fprintf(stderr, "\ndone:                               %.4f\n", dnowx());
		}
	}
	last_mask = mask;

	/* record the x, y position for the rfb screen as well. */
	cursor_position(x, y);

	/* change the cursor shape if necessary */
	rc = set_cursor(x, y, get_which_cursor());
	cursor_changes += rc;

	last_event = last_input = last_pointer_input = time(NULL);
}

void init_key_table(void) {
	macosxCG_init_key_table();
}

void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	allowed_input_t input;
	if (debug_keyboard) fprintf(stderr, "macosx_key_command: %d %s\n", (int) keysym, down ? "down" : "up");

	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		return;
	}

	init_key_table();
	macosxCG_keysym_inject((int) down, (unsigned int) keysym);
}

extern void macosxGCS_poll_pb(void);

int macosx_get_cursor_pos(int *x, int *y) {
	macosxCG_get_cursor_pos(x, y);
	if (nofb) {
		/* good time to poll the pasteboard */
		macosxGCS_poll_pb();
	}
	return 1;
}

static char *cuttext = NULL;
static int cutlen = 0;

void macosx_send_sel(char *str, int len) {
	if (screen && all_clients_initialized()) {
		if (cuttext) {
			int n = cutlen;
			if (len < n) {
				n = len;
			}
			if (!memcmp(str, cuttext, (size_t) n)) {
				/* the same text we set pasteboard to ... */
				return;
			}
		}
		if (debug_sel) {
			rfbLog("macosx_send_sel: %d\n", len);
		}
		rfbSendServerCutText(screen, str, len);
	}
}

void macosx_set_sel(char *str, int len) {
	if (screen && all_clients_initialized()) {
		if (cutlen <= len) {
			if (cuttext) {
				free(cuttext);
			}
			cutlen = 2*(len+1);
			cuttext = (char *) calloc(cutlen, 1);
		}
		memcpy(cuttext, str, (size_t) len);
		cuttext[len] = '\0';
		if (debug_sel) {
			rfbLog("macosx_set_sel: %d\n", len);
		}
		macosxGCS_set_pasteboard(str, len);
	}
}

int macosx_get_cursor(void) {
	return macosxCG_get_cursor();
}

typedef struct evdat {
	int win;
	int map;
	int level;
	int vis;
	int type;
} evdat_t;

#define MAX_EVENTS 1024
evdat_t mac_events[MAX_EVENTS];
int mac_events_ptr = 0;
int mac_events_last = 0;

void macosx_add_mapnotify(Window win, int level, int map) {
	int i = mac_events_last++;
	mac_events[i].win = win;
	mac_events[i].level = level;

	if (map) {
		mac_events[i].type = MapNotify;
	} else {
		mac_events[i].type = UnmapNotify;
	}
	mac_events[i].map = map;
	mac_events[i].vis = -1;

	mac_events_last = mac_events_last % MAX_EVENTS;

	return;
}

void macosx_add_create(Window win, int level) {
	int i = mac_events_last++;
	mac_events[i].win = win;
	mac_events[i].level = level;

	mac_events[i].type = CreateNotify;
	mac_events[i].map = -1;
	mac_events[i].vis = -1;

	mac_events_last = mac_events_last % MAX_EVENTS;

	return;
}

void macosx_add_destroy(Window win, int level) {
	int i = mac_events_last++;
	mac_events[i].win = win;
	mac_events[i].level = level;

	mac_events[i].type = DestroyNotify;
	mac_events[i].map = -1;
	mac_events[i].vis = -1;

	mac_events_last = mac_events_last % MAX_EVENTS;

	return;
}

void macosx_add_visnotify(Window win, int level, int obscured) {
	int i = mac_events_last++;
	mac_events[i].win = win;
	mac_events[i].level = level;

	mac_events[i].type = VisibilityNotify;
	mac_events[i].map = -1;

	mac_events[i].vis = 1;
	if (obscured == 0) {
		mac_events[i].vis = VisibilityUnobscured;
	} else if (obscured == 1) {
		mac_events[i].vis = VisibilityPartiallyObscured;
	} else if (obscured == 2) {
		mac_events[i].vis = VisibilityFullyObscured; 	/* NI */
	}

	mac_events_last = mac_events_last % MAX_EVENTS;

	return;
}

int macosx_checkevent(XEvent *ev) {
	int i = mac_events_ptr;

	if (mac_events_ptr == mac_events_last) {
		return 0;
	}
	if (ev == NULL) {
		return mac_events[i].type;
	}

	ev->xany.window = mac_events[i].win;

	if (mac_events[i].type == CreateNotify) {
		ev->type = CreateNotify;
		ev->xany.window = rootwin;
		ev->xcreatewindow.window = mac_events[i].win;
	} else if (mac_events[i].type == DestroyNotify) {
		ev->type = DestroyNotify;
		ev->xdestroywindow.window = mac_events[i].win;
	} else if (mac_events[i].type == VisibilityNotify) {
		ev->type = VisibilityNotify;
		ev->xvisibility.state = mac_events[i].vis;
	} else if (mac_events[i].type == MapNotify) {
		ev->type = MapNotify;
	} else if (mac_events[i].type == UnmapNotify) {
		ev->type = UnmapNotify;
	} else {
		fprintf(stderr, "unknown macosx_checkevent: %d\n", mac_events[i].type);
	}
	mac_events_ptr++;
	mac_events_ptr = mac_events_ptr % MAX_EVENTS;

	return mac_events[i].type;
}

typedef struct windat {
	int win;
	int x, y;
	int width, height;
	int level;
	int mapped;
	int clipped;
	int ncache_only;
} windat_t;

extern int macwinmax; 
extern windat_t macwins[]; 

int macosx_get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win) {
	static int last_idx = -1;
	int x1, x2, y1, y2;
	int idx = -1, k;
	macosxCGS_get_all_windows();
	macosxCG_get_cursor_pos(px, py);

	for (k = 0; k<macwinmax; k++) {
		if (! macwins[k].mapped) {
			continue;
		}
		x1 = macwins[k].x;
		x2 = macwins[k].x + macwins[k].width;
		y1 = macwins[k].y;
		y2 = macwins[k].y + macwins[k].height;
if (debug_wireframe) fprintf(stderr, "%d/%d:	%d %d %d  - %d %d %d\n", k, macwins[k].win, x1, *px, x2, y1, *py, y2);
		if (x1 <= *px && *px < x2) {
			if (y1 <= *py && *py < y2) {
				idx = k;
				break;
			}
		}
	}
	if (idx < 0) {
		return 0;
	}

	*x = macwins[idx].x;
	*y = macwins[idx].y;
	*w = macwins[idx].width;
	*h = macwins[idx].height;
	*frame = (Window) macwins[idx].win;
	if (win != NULL) {
		*win = *frame;
	}

	last_idx = idx;

	return 1;
}

int macosx_valid_window(Window w, XWindowAttributes* a) {
	static int last_idx = -1;
	int win = (int) w;
	int i, k, idx = -1;

	if (last_idx >= 0 && last_idx < macwinmax) {
		if (macwins[last_idx].win == win) {
			idx = last_idx;
		}
	}

	if (idx < 0) {
		idx = macosxCGS_get_qlook(w);
		if (idx >= 0 && idx < macwinmax) {
			if (macwins[idx].win != win) {
				idx = -1;
			}
		} else {
			idx = -1;
		}
	}

	if (idx < 0) {
		for (i = 0; i<macwinmax; i++) {
			k = i;
			if (i == -1)  {
				if (last_idx >= 0 && last_idx < macwinmax) {
					k = last_idx;
				} else {
					last_idx = -1;
					continue;
				}
			}
			if (macwins[k].win == win) {
				idx = k;
				break;
			}
		}
	}
	if (idx < 0) {
		return 0;
	}

	a->x = macwins[idx].x;
	a->y = macwins[idx].y;
	a->width  = macwins[idx].width;
	a->height = macwins[idx].height;
	a->depth = depth;
	a->border_width = 0;
	a->backing_store = 0;
	if (macwins[idx].mapped) {
		a->map_state = IsViewable;
	} else {
		a->map_state = IsUnmapped;
	}

	last_idx = idx;
	
	return 1;
}

#define QTMAX 2048
static Window cret[QTMAX];

extern int CGS_levelmax;
extern int CGS_levels[];

Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return) {

	int i, n, k;

	*root_return = (Window) 0;
	*parent_return = (Window) 0;
	if (!w) {}

	macosxCGS_get_all_windows();

	n = 0;
	for (k = CGS_levelmax - 1; k >= 0; k--) {
		for (i = macwinmax - 1; i >= 0; i--) {
			if (n >= QTMAX) break;
			if (macwins[i].level == CGS_levels[k]) {
if (0) fprintf(stderr, "k=%d i=%d n=%d\n", k, i, n);
				cret[n++] = (Window) macwins[i].win;
			}
		}
	}
	*children_return = cret;
	*nchildren_return = (unsigned int) macwinmax;

	return (Status) 1;
}

int macosx_check_offscreen(int win) {
	sraRegionPtr r0, r1;
	int x1, y1, x2, y2;
	int ret;
	int i = macosxCGS_find_index(win);

	if (i < 0) {
		return 0;
	}

	x1 = macwins[i].x;
	y1 = macwins[i].y;
	x2 = macwins[i].x + macwins[i].width;
	y2 = macwins[i].y + macwins[i].height;

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r1 = sraRgnCreateRect(x1, y1, x2, y2);

	if (sraRgnAnd(r1, r0)) {
		ret = 0;
	} else {
		ret = 1;
	}
	sraRgnDestroy(r0);
	sraRgnDestroy(r1);

	return ret;
}

int macosx_check_clipped(int win, int *list, int n) {
	sraRegionPtr r0, r1, r2;
	int x1, y1, x2, y2;
	int ret = 0;
	int k, j, i = macosxCGS_find_index(win);

	if (i < 0) {
		return 0;
	}

	x1 = macwins[i].x;
	y1 = macwins[i].y;
	x2 = macwins[i].x + macwins[i].width;
	y2 = macwins[i].y + macwins[i].height;

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r1 = sraRgnCreateRect(x1, y1, x2, y2);
	sraRgnAnd(r1, r0);

	for (k = 0; k < n; k++) {
		j = macosxCGS_find_index(list[k]);	/* XXX slow? */
		if (j < 0) {
			continue;
		}
		x1 = macwins[j].x;
		y1 = macwins[j].y;
		x2 = macwins[j].x + macwins[j].width;
		y2 = macwins[j].y + macwins[j].height;
		r2 = sraRgnCreateRect(x1, y1, x2, y2);
		if (sraRgnAnd(r2, r1)) {
			ret = 1;
			sraRgnDestroy(r2);
			break;
		}
		sraRgnDestroy(r2);
	}
	sraRgnDestroy(r0);
	sraRgnDestroy(r1);

	return ret;
}


#endif 	/* LIBVNCSERVER_HAVE_MACOSX_NATIVE_DISPLAY */

