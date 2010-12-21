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

/* -- 8to24.c -- */
#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "util.h"
#include "win_utils.h"
#include "xwrappers.h"

int multivis_count = 0;
int multivis_24count = 0;

void check_for_multivis(void);
void bpp8to24(int, int, int, int);
void mark_8bpp(int);

#if SKIP_8TO24
void check_for_multivis(void) {}
void bpp8to24(int x, int y, int z, int t) {}
void mark_8bpp(int x) {}
#else
/* lots... */

static void set_root_cmap(void);
static int check_pointer_in_depth24(void);
static void parse_cmap8to24(void);
static void set_poll_fb(void);
static int check_depth(Window win, Window top, int doall);
static int check_depth_win(Window win, Window top, XWindowAttributes *attr);
static XImage *p_xi(XImage *xi, Visual *visual, int win_depth, int *w);
static int poll_line(int x1, int x2, int y1, int n, sraRegionPtr mod);
static void poll_line_complement(int x1, int x2, int y1, sraRegionPtr mod);
static int poll_8bpp(sraRegionPtr, int);
static void poll_8bpp_complement(sraRegionPtr);
static void mark_rgn_rects(sraRegionPtr mod);
static int get_8bpp_regions(int validate);
static int get_cmap(int j, Colormap cmap);
static void do_8bpp_region(int n, sraRegionPtr mark);
static XImage *cmap_xi(XImage *xi, Window win, int win_depth);
static void transform_rect(sraRect rect, Window win, int win_depth, int cm);

/* struct for keeping info about the 8bpp windows: */
typedef struct window8 {
	Window win;
	Window top;
	int depth;
	int x, y;
	int w, h;
	int map_state;
	Colormap cmap;
	Bool map_installed;
	int fetched;
	double last_fetched;
	sraRegionPtr clip_region;
} window8bpp_t;

enum mark_8bpp_modes {
	MARK_8BPP_ALL = 0,
	MARK_8BPP_POINTER,
	MARK_8BPP_TOP
};


#define NCOLOR 256

static Colormap root_cmap = 0;
static unsigned int *root_rgb = NULL;

static void set_root_cmap(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	static time_t last_set = 0;
	time_t now = time(NULL);
	XWindowAttributes attr;
	static XColor *color = NULL;
	int redo = 0;
	int ncolor = 0;

	RAWFB_RET_VOID

	if (depth > 16) {
		ncolor = NCOLOR;
	} else if (depth > 8) {
		ncolor = 1 << depth;
	} else {
		ncolor = NCOLOR;
	}

	if (!root_rgb) {
		root_rgb = (unsigned int *) malloc(ncolor * sizeof(unsigned int));
	}
	if (!color) {
		color = (XColor *) malloc(ncolor * sizeof(XColor));
	}

	if (now > last_set + 10) {
		redo = 1;
	}
	if (! root_cmap || redo) {
		X_LOCK;
		if (! valid_window(window, &attr, 1)) {
			X_UNLOCK;
			return;
		}
		if (attr.colormap) {
			int i, ncells = ncolor;

			if (depth < 8) {
				ncells = CellsOfScreen(ScreenOfDisplay(dpy, scr));
			}
			for (i=0; i < ncells; i++) {
				color[i].pixel = i;
				color[i].pad = 0;
			}
			last_set = now;
			root_cmap = attr.colormap;
			XQueryColors(dpy, root_cmap, color, ncells);
			for (i=0; i < ncells; i++) {
				unsigned int red, green, blue;
				/* strip out highest 8 bits of values: */
				red   = (color[i].red   & 0xff00) >> 8;
				green = (color[i].green & 0xff00) >> 8;
				blue  = (color[i].blue  & 0xff00) >> 8;

				/*
				 * the maxes should be at 255 already,
				 * but just in case...
				 */
				red   = (main_red_max   * red  )/255;
				green = (main_green_max * green)/255;
				blue  = (main_blue_max  * blue )/255;

				/* shift them over and or together for value */
				red   = red    << main_red_shift;
				green = green  << main_green_shift;
				blue  = blue   << main_blue_shift;

				/* store it in the array to be used later */
				root_rgb[i] = red | green | blue;
			}
		}
		X_UNLOCK;
	}
#endif	/* NO_X11 */
}

/* fixed size array.  Will primarily hold visible 8bpp windows */
#define MAX_8BPP_WINDOWS 64
static window8bpp_t windows_8bpp[MAX_8BPP_WINDOWS];

static int db24 = 0;
static int xgetimage_8to24 = 1;
static double poll_8to24_delay = POLL_8TO24_DELAY;
static double cache_win = 0.0;
static int level2_8to24 = 0;

static int check_pointer_in_depth24(void) {
	int tries = 0, in_24 = 0;
	XWindowAttributes attr;
	Window c, w;
	double now = dnow();

	c = window;

	RAWFB_RET(0)

	if (now > last_keyboard_time + 1.0 && now > last_pointer_time + 1.0) {
		return 0;
	}

	X_LOCK;
	while (c && tries++ < 3) {
		c = query_pointer(c);
		if (valid_window(c, &attr, 1)) 	{
			if (attr.depth == 24) {
				in_24 = 1;
				break;
			}
		}
	}
	X_UNLOCK;
	if (in_24) {
		int x1, y1, x2, y2;
		X_LOCK;
		xtranslate(c, window, 0, 0, &x1, &y1, &w, 1);
		X_UNLOCK;
		x2 = x1 + attr.width;
		y2 = y1 + attr.height;
		x1 = nfix(x1, dpy_x);
		y1 = nfix(y1, dpy_y);
		x2 = nfix(x2, dpy_x+1);
		y2 = nfix(y2, dpy_y+1);
		mark_rect_as_modified(x1, y1, x2, y2, 0);

if (db24 > 1) fprintf(stderr, "check_pointer_in_depth24 %d %d %d %d\n", x1, y1, x2, y2);

		return 1;
	}
	return 0;
}

static void parse_cmap8to24(void) {
	if (cmap8to24_str) {
		char *p, *str = strdup(cmap8to24_str);
		p = strtok(str, ",");
		/* defaults: */
		db24 = 0;
		xgetimage_8to24 = 1;
		poll_8to24_delay = POLL_8TO24_DELAY;
		level2_8to24 = 0;
		cache_win = 0.0;
		while (p) {
			if (strstr(p, "dbg=") == p) {
				db24 = atoi(p + strlen("dbg="));
			} else if (strstr(p, "poll=") == p) {
				poll_8to24_delay = atof(p + strlen("poll="));
			} else if (strstr(p, "cachewin=") == p) {
				cache_win = atof(p + strlen("cachewin="));
			} else if (!strcmp(p, "nogetimage")) {
				xgetimage_8to24 = 0;
			} else if (!strcmp(p, "level2")) {
				level2_8to24 = 1;
			}
			p = strtok(NULL, ",");
		}
		free(str);
	} else {
		if (getenv("DEBUG_8TO24") != NULL) {
			db24 = atoi(getenv("DEBUG_8TO24"));
		}
		if (getenv("NOXGETIMAGE_8TO24") != NULL) {
			xgetimage_8to24 = 0;
		}
	}
}

static char *poll8_fb = NULL, *poll24_fb = NULL;
static int poll8_fb_w = 0, poll8_fb_h = 0;
static int poll24_fb_w = 0, poll24_fb_h = 0;

static void pfb(int fac, char **fb, int *w, int *h)  {
	if (! *fb || *w != dpy_x || *h != dpy_y) {
		if (*fb) {
			free(*fb);
		}
		*fb = (char *) calloc(fac * dpy_x * dpy_y, 1);
		*w = dpy_x;
		*h = dpy_y;
	}
}

static void set_poll_fb(void) {
	/* create polling framebuffers or recreate if too small. */

	if (! xgetimage_8to24) {
		return;		/* this saves a bit of RAM */
	}
	pfb(4, &poll24_fb, &poll24_fb_w, &poll24_fb_h);
	if (depth > 8 && depth <= 16) {
		pfb(2, &poll8_fb,  &poll8_fb_w,  &poll8_fb_h);	/* 2X for rare 16bpp colormap case */
	} else {
		pfb(1, &poll8_fb,  &poll8_fb_w,  &poll8_fb_h);
	}
}

int MV_glob = 0;
int MV_count;
int MV_hit;
double MV_start;

void check_for_multivis(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	XWindowAttributes attr;
	int doall = 0;
	int k, i, cnt, diff;
	static int first = 1;
	static Window *stack_old = NULL;
	static int stack_old_len = 0;
	static double last_parse = 0.0;
	static double last_update = 0.0;
	static double last_clear = 0.0;
	static double last_poll = 0.0;
	static double last_fixup = 0.0;
	static double last_call = 0.0;
	static double last_query = 0.0;
	double now = dnow();
	double delay;

	RAWFB_RET_VOID

	if (now > last_parse + 1.0) {
		last_parse = now;
		parse_cmap8to24();
	}
if (db24 > 2) fprintf(stderr, " check_for_multivis: %.4f\n", now - last_call);
	last_call = now;

	if (first) {
		int i;
		/* initialize 8bpp window table: */
		for (i=0; i < MAX_8BPP_WINDOWS; i++) 	{
			windows_8bpp[i].win = None;
			windows_8bpp[i].top = None;
			windows_8bpp[i].map_state = IsUnmapped;
			windows_8bpp[i].cmap = (Colormap) 0;
			windows_8bpp[i].fetched = 0;
			windows_8bpp[i].last_fetched = -1.0;
			windows_8bpp[i].clip_region = NULL;
		}
		set_poll_fb();

		first = 0;
		doall = 1;	/* fetch everything first time */
	}

	if (wireframe_in_progress) {
		return;
	}

	set_root_cmap();

	/*
	 * allocate an "old stack" list of all toplevels.  we compare
	 * this to the current stack to guess stacking order changes.
	 */
	if (!stack_old || stack_old_len < stack_list_len) {
		int n = stack_list_len;
		if (n < 256) {
			n = 256;
		}
		if (stack_old) {
			free(stack_old);
		}
		stack_old = (Window *) calloc(n*sizeof(Window), 1);
		stack_old_len = n;
	}

	/* fill the old stack with visible windows: */
	cnt = 0;
	for (k=0; k < stack_list_num; k++) {
		if (stack_list[k].valid &&
		    stack_list[k].map_state == IsViewable) {
			stack_old[cnt++] = stack_list[k].win;
		}
	}

	/* snapshot + update the current stacking order: */
	/* TUNABLE */
	if (poll_8to24_delay >= POLL_8TO24_DELAY) {
		delay = 3.0 * poll_8to24_delay;
	} else {
		delay = 3.0 * POLL_8TO24_DELAY;	/* 0.15 */
	}
	if (doall || now > last_update + delay) {
		snapshot_stack_list(0, 0.0);
		update_stack_list();
		last_update = now;
	}

	/* look for differences in the visible toplevels: */
	diff = 0;
	cnt = 0;
	for (k=0; k < stack_list_num; k++) {
		if (stack_list[k].valid && stack_list[k].map_state ==
		    IsViewable) {
			if (stack_old[cnt] != stack_list[k].win) {
				diff = 1;
				break;
			}
			cnt++;
		}
	}

	multivis_count = 0;
	multivis_24count = 0;

	/*
	 * every 10 seconds we try to clean out and also refresh the window
	 * info in the 8bpp window table:
	 */
	if (now > last_clear + 10) {
		last_clear = now;
		X_LOCK;
		for (i=0; i < MAX_8BPP_WINDOWS; i++) {
			Window w = windows_8bpp[i].win;
			if (! valid_window(w, &attr, 1)) {
				/* catch windows that went away: */
				windows_8bpp[i].win = None;
				windows_8bpp[i].top = None;
				windows_8bpp[i].map_state = IsUnmapped;
				windows_8bpp[i].cmap = (Colormap) 0;
				windows_8bpp[i].fetched = 0;
				windows_8bpp[i].last_fetched = -1.0;
			}
		}
		X_UNLOCK;
	}

	MV_count = 0;
	MV_hit = 0;
	MV_start = dnow();

	set_root_cmap();

	/* loop over all toplevels, both 8 and 24 depths: */

	X_LOCK;	/* a giant lock around the whole activity */

	for (k=0; k < stack_list_num; k++) {
		Window r, parent;
		Window *list0;
		Status rc;
		unsigned int nc0;
		int i1;
		XErrorHandler old_handler;
		double delay;

		Window win = stack_list[k].win;

		/* TUNABLE */
		if (poll_8to24_delay >= POLL_8TO24_DELAY) {
			delay = 1.5 * poll_8to24_delay;
		} else {
			delay = 1.5 * POLL_8TO24_DELAY;	/* 0.075 */
		}

		if (now < last_query + delay) {
			break;
		}

		if (win == None) {
			continue;
		}

		if (stack_list[k].map_state != IsViewable) {
			int i;
			/*
			 * if the toplevel became unmapped, mark it
			 * for the children as well...
			 */
			for (i=0; i < MAX_8BPP_WINDOWS; i++) {
				if (windows_8bpp[i].top == win) {
					windows_8bpp[i].map_state =
					    stack_list[k].map_state;
				}
			}
		}

		if (check_depth(win, win, doall)) {
			/*
			 * returns 1 if no need to recurse down e.g. It
			 * is 8bpp and we assume all lower ones are too.
			 */
			continue;
		}

		/* we recurse up to two levels down from stack_list windows */

		old_handler = XSetErrorHandler(trap_xerror);
		trapped_xerror = 0;
		rc = XQueryTree_wr(dpy, win, &r, &parent, &list0, &nc0);
		XSetErrorHandler(old_handler);

		if (! rc || trapped_xerror) {
			trapped_xerror = 0;
			continue;
		}
		trapped_xerror = 0;

		/* loop over grandchildren of rootwin: */
		for (i1=0; i1 < (int) nc0; i1++) {
			Window win1 = list0[i1];
			Window *list1;
			unsigned int nc1;
			int i2;

			if (check_depth(win1, win, doall)) {
				continue;
			}

			if (level2_8to24) {
				continue;
			}

			old_handler = XSetErrorHandler(trap_xerror);
			trapped_xerror = 0;
			rc = XQueryTree_wr(dpy, win1, &r, &parent, &list1, &nc1);
			XSetErrorHandler(old_handler);

			if (! rc || trapped_xerror) {
				trapped_xerror = 0;
				continue;
			}
			trapped_xerror = 0;

			/* loop over great-grandchildren of rootwin: */
			for (i2=0; i2< (int) nc1; i2++) {
				Window win2 = list1[i2];

				if (check_depth(win2, win, doall)) {
					continue;
				}
				/* more? Which wm does this? */
			}
			if (nc1) {
				XFree_wr(list1);
			}
		}
		if (nc0) {
			XFree_wr(list0);
		}
	}
	X_UNLOCK;

	last_query = dnow();

MV_glob += MV_count;
if (0) fprintf(stderr, "MV_count: %d hit: %d %.4f  %10.2f\n", MV_count, MV_hit, last_query - MV_start, MV_glob / (last_query - x11vnc_start));

	if (screen_fixup_8 > 0.0 && now > last_fixup + screen_fixup_8) {
		last_fixup = now;
		mark_8bpp(MARK_8BPP_ALL);
		last_poll = now;

	} else if (poll_8to24_delay > 0.0) {
		int area = -1;
		int validate = 0;

		if (diff && multivis_count) {
			validate = 1;
		}
		if (now > last_poll + poll_8to24_delay) {
			sraRegionPtr mod;

			last_poll = now;
			mod = sraRgnCreate();
			area = poll_8bpp(mod, validate);
			if (depth == 24) {
				poll_8bpp_complement(mod);
			}
			mark_rgn_rects(mod);
			sraRgnDestroy(mod);
		}
		if (0 && area < dpy_x * dpy_y / 2 && diff && multivis_count) {
			mark_8bpp(MARK_8BPP_POINTER);
			last_poll = now;
		}

	} else if (diff && multivis_count) {
		mark_8bpp(MARK_8BPP_ALL);
		last_poll = now;

	} else if (depth <= 16 && multivis_24count) {
		static double last_check = 0.0;
		if (now > last_check + 0.4) {
			last_check = now;
			if (check_pointer_in_depth24()) {
				last_poll = now;
			}
		}
	}
if (0) fprintf(stderr, "done: %.4f\n", dnow() - last_query);
#endif	/* NO_X11 */
}

#define VW_CACHE_MAX 1024
static XWindowAttributes vw_cache_attr[VW_CACHE_MAX];
static Window vw_cache_win[VW_CACHE_MAX];

static void set_attr(XWindowAttributes *attr, int j) {
	memcpy((void *) (vw_cache_attr+j), (void *) attr,
	    sizeof(XWindowAttributes));
}
#if 0
static int get_attr(XWindowAttributes *attr, int j) {
	memcpy((void *) attr, (void *) (vw_cache_attr+j),
	    sizeof(XWindowAttributes));
	return 1;
}
#endif

static XWindowAttributes wattr;

static XWindowAttributes *vw_lookup(Window win) {
	static double last_purge = 0.0;
	double now;
	int i, j, k;

	if (win == None) {
		return NULL;
	}

	now = dnow();
	if (now > last_purge + cache_win) {
		last_purge = now;
		for (i=0; i<VW_CACHE_MAX; i++) {
			vw_cache_win[i] = None;
		}
	}

	j = -1;
	k = -1;
	for (i=0; i<VW_CACHE_MAX; i++) {
		if (vw_cache_win[i] == win) {
			j = i;
			break;
		} else if (vw_cache_win[i] == None) {
			k = i;
			break;
		}
	}

	if (j >= 0) {
MV_hit++;
		return vw_cache_attr+j;
		
	} else if (k >= 0) {
		XWindowAttributes attr2;
		int rc = valid_window(win, &attr2, 1);
		if (rc) {
			vw_cache_win[k] = win;
			set_attr(&attr2, k);
			return vw_cache_attr+k;
		} else {
			return NULL;
		}
	} else {
		/* Full */
		int rc = valid_window(win, &wattr, 1);
		if (rc) {
			return &wattr;
		} else {
			return NULL;
		}
	}
}

static int check_depth(Window win, Window top, int doall) {
	XWindowAttributes attr, *pattr;

	/* first see if it is (still) a valid window: */
MV_count++;

	if (cache_win > 0.0) {
		pattr = vw_lookup(win);
		if (pattr == NULL) {
			return 1;	/* indicate done */
		}
	} else {
		if (! valid_window(win, &attr, 1)) {
			return 1;	/* indicate done */
		}
		pattr = &attr;
	}

	if (! doall && pattr->map_state != IsViewable) {
		/*
		 * store results anyway...  this may lead to table
		 * filling up, but currently this allows us to update
		 * state of onetime mapped windows.
		 */
		check_depth_win(win, top, pattr);
		return 1;	/* indicate done */
	} else if (check_depth_win(win, top, pattr)) {
		return 1;	/* indicate done */
	} else {
		return 0;	/* indicate not done */
	}
}

static int check_depth_win(Window win, Window top, XWindowAttributes *attr) {
	int store_it = 0;
	/*
	 * only store windows with depth not equal to the default visual's
	 * depth note some windows can have depth == 0 ... (skip them).
	 */
	if (attr->depth > 0) {
		if (depth == 24 && attr->depth != 24) {
			store_it = 1;
		} else if (depth <= 16 && root_cmap && attr->colormap != root_cmap) {
			store_it = 1;
		}
	}

	if (store_it) {
		int i, j = -1, none = -1, nomap = -1;
		int newc = 0;
		if (attr->map_state == IsViewable) {
			/* count the visible ones: */
			multivis_count++;
			if (attr->depth == 24) {
				multivis_24count++;
			}
if (db24 > 1) fprintf(stderr, "multivis: 0x%lx %d\n", win, attr->depth);
		}

		/* try to find a table slot for this window: */
		for (i=0; i < MAX_8BPP_WINDOWS; i++) {
			if (none < 0 && windows_8bpp[i].win == None) {
				/* found first None */
				none = i;
			}
			if (windows_8bpp[i].win == win) {
				/* found myself */
				j = i;
				break;
			}
			if (nomap < 0 && windows_8bpp[i].win != None &&
			    windows_8bpp[i].map_state != IsViewable) {
				/* found first unmapped */
				nomap = i;
			}
		}
		if (j < 0) {
			if (attr->map_state != IsViewable) {
				/* no slot and not visible: not worth keeping */
				return 1;
			} else if (none >= 0) {
				/* put it in the first None slot */
				j = none;
				newc = 1;
			} else if (nomap >=0) {
				/* put it in the first unmapped slot */
				j = nomap;
			}
			/* otherwise we cannot store it... */
		}

if (db24 > 1) fprintf(stderr, "multivis: 0x%lx ms: %d j: %d no: %d nm: %d dep=%d\n", win, attr->map_state, j, none, nomap, attr->depth);

		/* store if if we found a slot j: */
		if (j >= 0) {
			Window w;
			int x, y;
			int now_vis = 0;

			if (attr->map_state == IsViewable &&
			    windows_8bpp[j].map_state != IsViewable) {
				now_vis = 1;
			}
if (db24 > 1) fprintf(stderr, "multivis: STORE 0x%lx j: %3d ms: %d dep=%d\n", win, j, attr->map_state, attr->depth);
			windows_8bpp[j].win = win;
			windows_8bpp[j].top = top;
			windows_8bpp[j].depth = attr->depth;
			windows_8bpp[j].map_state = attr->map_state;
			windows_8bpp[j].cmap = attr->colormap;
			windows_8bpp[j].map_installed = attr->map_installed;
			windows_8bpp[j].w = attr->width;
			windows_8bpp[j].h = attr->height;
			windows_8bpp[j].fetched = 1;
			windows_8bpp[j].last_fetched = dnow();

			/* translate x y to be WRT the root window (not parent) */
			xtranslate(win, window, 0, 0, &x, &y, &w, 1);
			windows_8bpp[j].x = x;
			windows_8bpp[j].y = y;

			if (newc || now_vis) {
if (db24) fprintf(stderr, "new/now_vis: 0x%lx %d/%d\n", win, newc, now_vis);
				/* mark it immediately if a new one: */
				X_UNLOCK;	/* dont forget the giant lock */
				mark_rect_as_modified(x, y, x + attr->width,
				    y + attr->height, 0);
				X_LOCK;
			}
		} else {
			/*
			 * Error: could not find a slot.
			 * perhaps keep age and expire old ones??
			 */
if (db24) fprintf(stderr, "multivis: CANNOT STORE 0x%lx j=%d\n", win, j);
			for (i=0; i < MAX_8BPP_WINDOWS; i++) {
if (db24 > 1) fprintf(stderr, "          ------------ 0x%lx i=%d\n", windows_8bpp[i].win, i);
			}
			
		}
		return 1;
	}
	return 0;
}

/* polling line XImage */
static XImage *p_xi(XImage *xi, Visual *visual, int win_depth, int *w) {
	RAWFB_RET(NULL)

#if NO_X11
	if (!xi || !visual || !win_depth || !w) {}
	return NULL;
#else
	if (xi == NULL || *w < dpy_x) {
		char *d;
		if (xi) {
			XDestroyImage(xi);
		}
		if (win_depth != 24) {
			if (win_depth > 8) {
				d = (char *) malloc(dpy_x * 2);
			} else {
				d = (char *) malloc(dpy_x * 1);
			}
		} else {
			d = (char *) malloc(dpy_x * 4);
		}
		*w = dpy_x;
		xi = XCreateImage(dpy, visual, win_depth, ZPixmap, 0, d,
		    dpy_x, 1, 8, 0);
	}
	return xi;
#endif	/* NO_X11 */
}

static int poll_line(int x1, int x2, int y1, int n, sraRegionPtr mod) {
#if NO_X11
	RAWFB_RET(1)
	if (!x1 || !x2 || !y1 || !n || !mod) {}
	return 1;
#else
	int fac, n_off, w, xo, yo;
	char *poll_fb, *dst, *src;
	int w2, xl, xh, stride = 32;
	int inrun = 0, rx1 = -1, rx2 = -1;

	static XImage *xi8 = NULL, *xi24 = NULL, *xi_r;
	static int xi8_w = 0, xi24_w = 0;

	XErrorHandler old_handler = NULL;
	XImage *xi;
	Window c, win = windows_8bpp[n].win;

	static XWindowAttributes attr;
	static Window last_win = None; 
	static double last_time = 0.0;
	double now;

	sraRegionPtr rect;
	int mx1, mx2, my1, my2;
	int ns = NSCAN/2;

	RAWFB_RET(1)

	if (win == None) {
		return 1;
	}
	if (windows_8bpp[n].map_state != IsViewable) {
		return 1;
	}
	if (! xgetimage_8to24) {
		return 1;
	}

	X_LOCK;
	now = dnow();
	if (last_win != None && win == last_win && now < last_time + 0.5) {
		;	/* use previous attr */
	} else {
		if (! valid_window(win, &attr, 1)) {
			X_UNLOCK;
			last_win = None;
			return 0;
		}
		last_time = now;
		last_win = win;
	}

	if (attr.depth > 16 && attr.depth != 24) {
		X_UNLOCK;
		return 1;
	} else if (attr.depth <= 16) {
		xi = xi8 = p_xi(xi8, attr.visual, attr.depth, &xi8_w);

		poll_fb = poll8_fb;
		if (attr.depth > 8) {
			fac = 2;
		} else {
			fac = 1;
		}
		n_off = poll8_fb_w * y1 + x1;
	} else {
		xi = xi24 = p_xi(xi24, attr.visual, 24, &xi24_w);

		poll_fb = poll24_fb;
		fac = 4;
		n_off = poll24_fb_w * y1 + x1;
	}

	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;

	/* xtranslate() not used to save two XSetErrorHandler calls */
	XTranslateCoordinates(dpy, win, window, 0, 0, &xo, &yo, &c);

	xo = x1 - xo;
	yo = y1 - yo;
	w = x2 - x1;

	if (trapped_xerror || xo < 0 || yo < 0 || xo + w > attr.width) {
if (db24 > 2) fprintf(stderr, "avoid bad match...\n");
		XSetErrorHandler(old_handler);
		trapped_xerror = 0;
		X_UNLOCK;
		return 0;
	}

	trapped_xerror = 0;
	xi_r = XGetSubImage(dpy, win, xo, yo, w, 1, AllPlanes, ZPixmap, xi,
	    0, 0);
	XSetErrorHandler(old_handler);

	X_UNLOCK;

	if (! xi_r || trapped_xerror) {
		trapped_xerror = 0;
		return 0;
	}
	trapped_xerror = 0;

	src = xi->data;
	dst = poll_fb + fac * n_off;

	inrun = 0;

	xl = x1;
	while (xl < x2) {
		xh = xl + stride;
		if (xh > x2) {
			xh = x2;
		}
		w2 = xh - xl;
		if (memcmp(dst, src, fac * w2)) {
			if (inrun) {
				rx2 = xh;
			} else {
				rx1 = xl;
				rx2 = xh;
				inrun = 1;
			}
		} else {
			if (inrun) {
				mx1 = rx1;
				mx2 = rx2;
				my1 = nfix(y1 - ns, dpy_y);
				my2 = nfix(y1 + ns, dpy_y+1);

				rect = sraRgnCreateRect(mx1, my1, mx2, my2);
				sraRgnOr(mod, rect);
				sraRgnDestroy(rect);	
				inrun = 0;
			}
		}

		xl += stride;
		dst += fac * stride;
		src += fac * stride;
	}

	if (inrun) {
		mx1 = rx1;
		mx2 = rx2;
		my1 = nfix(y1 - ns, dpy_y);
		my2 = nfix(y1 + ns, dpy_y+1);

		rect = sraRgnCreateRect(mx1, my1, mx2, my2);
		sraRgnOr(mod, rect);
		sraRgnDestroy(rect);	
	}
	return 1;
#endif	/* NO_X11 */
}

static void poll_line_complement(int x1, int x2, int y1, sraRegionPtr mod) {
	int n_off, w, xl, xh, stride = 32;
	char *dst, *src;
	int inrun = 0, rx1 = -1, rx2 = -1;
	sraRegionPtr rect;
	int mx1, mx2, my1, my2;
	int ns = NSCAN/2;

	if (depth != 24) {
		return;
	}
	if (! cmap8to24_fb) {
		return;
	}
	if (! xgetimage_8to24) {
		return;
	}

	n_off = main_bytes_per_line * y1 + 4 * x1;

	src = main_fb + n_off;
	dst = cmap8to24_fb + n_off;

	inrun = 0;

	xl = x1;
	while (xl < x2) {
		xh = xl + stride;
		if (xh > x2) {
			xh = x2;
		}
		w = xh - xl;
		if (memcmp(dst, src, 4 * w)) {
			if (inrun) {
				rx2 = xh;
			} else {
				rx1 = xl;
				rx2 = xh;
				inrun = 1;
			}
		} else {
			if (inrun) {
				mx1 = rx1;
				mx2 = rx2;
				my1 = nfix(y1 - ns, dpy_y);
				my2 = nfix(y1 + ns, dpy_y+1);

				rect = sraRgnCreateRect(mx1, my1, mx2, my2);
				sraRgnOr(mod, rect);
				sraRgnDestroy(rect);	

				inrun = 0;
			}
		}

		xl += stride;
		dst += 4 * stride;
		src += 4 * stride;
	}

	if (inrun) {
		mx1 = rx1;
		mx2 = rx2;
		my1 = nfix(y1 - ns, dpy_y);
		my2 = nfix(y1 + ns, dpy_y+1);

		rect = sraRgnCreateRect(mx1, my1, mx2, my2);
		sraRgnOr(mod, rect);
		sraRgnDestroy(rect);	

		inrun = 0;
	}
}

#define CMAPMAX 64
static Colormap cmaps[CMAPMAX];
static int ncmaps;

static int poll_8bpp(sraRegionPtr mod, int validate) {
	int i, y, ysh, map_count;
	static int ycnt = 0;
	sraRegionPtr line;
	sraRect rect;
	sraRectangleIterator *iter;
	int br = 0, area = 0;
	static double last_call = 0.0;
	
	map_count = get_8bpp_regions(validate);

if (db24 > 1) fprintf(stderr, "poll_8bpp mc: %d\n", map_count);

	if (! map_count) {
		return 0;
	}

	set_poll_fb();

	ysh = scanlines[(ycnt++) % NSCAN];
if (db24 > 2) fprintf(stderr, "poll_8bpp: ysh: %2d  %.4f\n", ysh, dnow() - last_call);
	last_call = dnow();

	for (i=0; i < MAX_8BPP_WINDOWS; i++) {
		sraRegionPtr reg = windows_8bpp[i].clip_region;

		if (! reg || sraRgnEmpty(reg)) {
			continue;
		}
		y = ysh;
		while (y < dpy_y) {
			line = sraRgnCreateRect(0, y, dpy_x, y+1);

			if (sraRgnAnd(line, reg)) {
				iter = sraRgnGetIterator(line);
				while (sraRgnIteratorNext(iter, &rect)) {
					if (! poll_line(rect.x1, rect.x2,
					    rect.y1, i, mod)) {
						br = 1;
						break;	/* exception */
					}
				}
				sraRgnReleaseIterator(iter);
			}

			sraRgnDestroy(line);
			y += NSCAN;
			if (br) break;
		}
		if (br) break;
	}

	iter = sraRgnGetIterator(mod);
	while (sraRgnIteratorNext(iter, &rect)) {
		area += nabs((rect.x2 - rect.x1)*(rect.y2 - rect.y1));
	}
	sraRgnReleaseIterator(iter);

	return area;
}

static void poll_8bpp_complement(sraRegionPtr mod) {
	int i, y, ysh;
	static int ycnt = 0;
	sraRegionPtr disp, line;
	sraRect rect;
	sraRectangleIterator *iter;

	disp = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

	ysh = scanlines[(ycnt++) % NSCAN];

	for (i=0; i < MAX_8BPP_WINDOWS; i++) {
		sraRegionPtr reg = windows_8bpp[i].clip_region;

		if (! reg) {
			continue;
		}
		if (windows_8bpp[i].map_state != IsViewable) {
			continue;	
		}
		sraRgnSubtract(disp, reg);
	}

	y = ysh;
	while (y < dpy_y) {
		line = sraRgnCreateRect(0, y, dpy_x, y+1);

		if (sraRgnAnd(line, disp)) {
			iter = sraRgnGetIterator(line);
			while (sraRgnIteratorNext(iter, &rect)) {
				poll_line_complement(rect.x1, rect.x2,
				    rect.y1, mod);
			}
			sraRgnReleaseIterator(iter);
		}

		sraRgnDestroy(line);

		y += NSCAN;
	}

	sraRgnDestroy(disp);
}

static void mark_rgn_rects(sraRegionPtr mod) {
	sraRect rect;
	sraRectangleIterator *iter;
	int area = 0;

	if (sraRgnEmpty(mod)) {
		return;
	}
	
	iter = sraRgnGetIterator(mod);
	while (sraRgnIteratorNext(iter, &rect)) {
		mark_rect_as_modified(rect.x1, rect.y1, rect.x2, rect.y2, 0);
		area += nabs((rect.x2 - rect.x1)*(rect.y2 - rect.y1));
	}
	sraRgnReleaseIterator(iter);

if (db24 > 1) fprintf(stderr, " mark_rgn_rects area: %d\n", area);
}

static int get_8bpp_regions(int validate) {

	XWindowAttributes attr;
	int i, k, mapcount = 0;

	/* initialize color map list */
	ncmaps = 0;
	for (i=0; i < CMAPMAX; i++) {
		cmaps[i] = (Colormap) 0;
	}

	/* loop over the table of 8bpp windows: */
	for (i=0; i < MAX_8BPP_WINDOWS; i++) {
		sraRegionPtr tmp_reg, tmp_reg2;
		Window c, w = windows_8bpp[i].win;
		int x, y;

		if (windows_8bpp[i].clip_region) {
			sraRgnDestroy(windows_8bpp[i].clip_region);	
		}
		windows_8bpp[i].clip_region = NULL;

		if (w == None) {
			continue;
		}

if (db24 > 1) fprintf(stderr, "get_8bpp_regions: 0x%lx ms=%d dep=%d i=%d\n", w, windows_8bpp[i].map_state, windows_8bpp[i].depth, i);
		if (validate) {
			/*
			 * this could be slow: validating 8bpp windows each
			 * time...
			 */

			X_LOCK;
			if (! valid_window(w, &attr, 1)) {
				X_UNLOCK;
				windows_8bpp[i].win = None;
				windows_8bpp[i].top = None;
				windows_8bpp[i].map_state = IsUnmapped;
				windows_8bpp[i].cmap = (Colormap) 0;
				windows_8bpp[i].fetched = 0;
				windows_8bpp[i].last_fetched = -1.0;
				continue;
			}
			X_UNLOCK;

			windows_8bpp[i].depth = attr.depth;
			windows_8bpp[i].map_state = attr.map_state;
			windows_8bpp[i].cmap = attr.colormap;
			windows_8bpp[i].map_installed = attr.map_installed;
			windows_8bpp[i].w = attr.width;
			windows_8bpp[i].h = attr.height;
			windows_8bpp[i].fetched = 1;
			windows_8bpp[i].last_fetched = dnow();

			if (attr.map_state != IsViewable) {
				continue;
			}

			X_LOCK;
			xtranslate(w, window, 0, 0, &x, &y, &c, 1);
			X_UNLOCK;
			windows_8bpp[i].x = x;
			windows_8bpp[i].y = y;

		} else {
			/* this will be faster: no call to X server: */
			if (windows_8bpp[i].map_state != IsViewable) {
				continue;
			}
			attr.depth = windows_8bpp[i].depth;
			attr.map_state = windows_8bpp[i].map_state;
			attr.colormap = windows_8bpp[i].cmap;
			attr.map_installed = windows_8bpp[i].map_installed;
			attr.width = windows_8bpp[i].w;
			attr.height = windows_8bpp[i].h;

			x =  windows_8bpp[i].x; 
			y =  windows_8bpp[i].y; 
		}

		mapcount++;

		/* tmp region for this 8bpp rectangle: */
		tmp_reg = sraRgnCreateRect(nfix(x, dpy_x), nfix(y, dpy_y),
		    nfix(x + attr.width, dpy_x+1), nfix(y + attr.height, dpy_y+1));

		/* loop over all toplevels, top to bottom clipping: */
		for (k = stack_list_num - 1; k >= 0; k--) {
			Window swin = stack_list[k].win;
			int sx, sy, sw, sh;

if (db24 > 1 && stack_list[k].map_state == IsViewable) fprintf(stderr, "Stack win: 0x%lx %d iv=%d\n", swin, k, stack_list[k].map_state);

			if (swin == windows_8bpp[i].top) {
				/* found our top level: we skip the rest. */
if (db24 > 1) fprintf(stderr, "found top: 0x%lx %d iv=%d\n", swin, k, stack_list[k].map_state);
				break;
			}
			if (stack_list[k].map_state != IsViewable) {
				/* skip unmapped ones: */
				continue;
			}

			/* make a temp rect for this toplevel: */
			sx = stack_list[k].x;
			sy = stack_list[k].y;
			sw = stack_list[k].width;
			sh = stack_list[k].height;

if (db24 > 1) fprintf(stderr, "subtract:  0x%lx %d -- %d %d %d %d\n", swin, k, sx, sy, sw, sh);

			tmp_reg2 = sraRgnCreateRect(nfix(sx, dpy_x),
			    nfix(sy, dpy_y), nfix(sx + sw, dpy_x+1),
			    nfix(sy + sh, dpy_y+1));

			/* subtract it from the 8bpp window region */
			sraRgnSubtract(tmp_reg, tmp_reg2);

			sraRgnDestroy(tmp_reg2);

			if (sraRgnEmpty(tmp_reg)) {
				break;
			}
		}

		if (sraRgnEmpty(tmp_reg)) {
			/* skip this 8bpp if completely clipped away: */
			sraRgnDestroy(tmp_reg);
			continue;
		}

		/* otherwise, store any new colormaps: */
		if (ncmaps < CMAPMAX && attr.colormap != (Colormap) 0) {
			int m, seen = 0;
			for (m=0; m < ncmaps; m++) {
				if (cmaps[m] == attr.colormap) {
					seen = 1;
					break;
				}
			}
			if (!seen && attr.depth <= 16) {
				/* store only new ones: */
				cmaps[ncmaps++] = attr.colormap;
			}
		}

		windows_8bpp[i].clip_region = tmp_reg;
	}

	return mapcount;
}

static XColor *color[CMAPMAX];
static unsigned int *rgb[CMAPMAX];
static int cmap_failed[CMAPMAX];
static int color_init = 0;
int histo[65536];

static int get_cmap(int j, Colormap cmap) {
#if NO_X11
	RAWFB_RET(0)
	if (!j || !cmap) {}
	return 0;
#else
	int i, ncells, ncolor;
	XErrorHandler old_handler = NULL;

	RAWFB_RET(0)

	if (depth > 16) {
		/* 24 */
		ncolor = NCOLOR;
	} else if (depth > 8) {
		ncolor = 1 << depth;
	} else {
		ncolor = NCOLOR;
	}
	if (!color_init) {
		int cm;
		for (cm = 0; cm < CMAPMAX; cm++) {
			color[cm] = (XColor *) malloc(ncolor * sizeof(XColor));
			rgb[cm] = (unsigned int *) malloc(ncolor * sizeof(unsigned int));
		}
		color_init = 1;
	}

	if (depth <= 16) {
		/* not working properly for depth 24... */
		X_LOCK;
		ncells = CellsOfScreen(ScreenOfDisplay(dpy, scr));
		X_UNLOCK;
	} else {
		ncells = NCOLOR;
	}

	if (depth > 16) {
		;
	} else if (ncells > ncolor) {
		ncells = ncolor;
	} else if (ncells == 8 && depth != 3) {
		/* XXX. see set_colormap() */
		ncells = 1 << depth;
	}

	/* initialize XColor array: */
	for (i=0; i < ncells; i++) {
		color[j][i].pixel = i;
		color[j][i].pad = 0;
	}
if (db24 > 1) fprintf(stderr, "get_cmap: %d 0x%x ncolor=%d ncells=%d\n", j, (unsigned int) cmap, ncolor, ncells);

	/* try to query the colormap, trap errors */
	X_LOCK;
	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);
	XQueryColors(dpy, cmap, color[j], ncells);
	XSetErrorHandler(old_handler);
	X_UNLOCK;

	if (trapped_xerror) {
		trapped_xerror = 0;
		return 0;
	}
	trapped_xerror = 0;

	/* now map each index to depth 24 RGB */
	for (i=0; i < ncells; i++) {
		unsigned int red, green, blue;
		/* strip out highest 8 bits of values: */
		red   = (color[j][i].red   & 0xff00) >> 8;
		green = (color[j][i].green & 0xff00) >> 8;
		blue  = (color[j][i].blue  & 0xff00) >> 8;

		/*
		 * the maxes should be at 255 already,
		 * but just in case...
		 */
		red   = (main_red_max   * red  )/255;
		green = (main_green_max * green)/255;
		blue  = (main_blue_max  * blue )/255;

if (db24 > 2) fprintf(stderr, " cmap[%02d][%03d]: %03d %03d %03d  0x%08x \n", j, i, red, green, blue, ( red << main_red_shift | green << main_green_shift | blue << main_blue_shift));

		/* shift them over and or together for value */
		red   = red    << main_red_shift;
		green = green  << main_green_shift;
		blue  = blue   << main_blue_shift;

		/* store it in the array to be used later */
		rgb[j][i] = red | green | blue;
	}
	return 1;
#endif	/* NO_X11 */
}

static void do_8bpp_region(int n, sraRegionPtr mark) {
	int k, cm = -1, failed = 0;
	sraRectangleIterator *iter;
	sraRegionPtr clip;
	sraRect rect;

	if (! windows_8bpp[n].clip_region) {
		return;
	}
	if (windows_8bpp[n].win == None) {
		return;
	}
	if (windows_8bpp[n].map_state != IsViewable) {
		return;
	}
if (db24 > 1) fprintf(stderr, "ncmaps: %d\n", ncmaps);

	/* see if XQueryColors failed: */
	for (k=0; k<ncmaps; k++) {
		if (windows_8bpp[n].cmap == cmaps[k]) {
			cm = k;
			if (cmap_failed[k]) {
				failed = 1;
			}
			break;
		}
	}

	if (windows_8bpp[n].depth != 24) {	/* 24 won't have a cmap */
		if (failed || cm == -1) {
			return;
		}
	}

	clip = sraRgnCreateRgn(mark);
	sraRgnAnd(clip, windows_8bpp[n].clip_region);

	/* loop over the rectangles making up region */
	iter = sraRgnGetIterator(clip);
	while (sraRgnIteratorNext(iter, &rect)) {
		if (rect.x1 > rect.x2) {
			int tmp = rect.x2;
			rect.x2 = rect.x1;
			rect.x1 = tmp;
		}
		if (rect.y1 > rect.y2) {
			int tmp = rect.y2;
			rect.y2 = rect.y1;
			rect.y1 = tmp;
		}

		transform_rect(rect, windows_8bpp[n].win,
		    windows_8bpp[n].depth, cm);
	}
	sraRgnReleaseIterator(iter);
	sraRgnDestroy(clip);
}

static XImage *cmap_xi(XImage *xi, Window win, int win_depth) {
#if NO_X11
	if (!xi || !win || !win_depth) {}
	return NULL;
#else
	XWindowAttributes attr;
	char *d;

	if (xi) {
		XDestroyImage(xi);
	}
	if (! dpy || ! valid_window(win, &attr, 1)) {
		return (XImage *) NULL;
	}
	if (win_depth == 24) {
		d = (char *) malloc(dpy_x * dpy_y * 4);
	} else if (win_depth <= 16) {
		if (win_depth > 8) {
			d = (char *) malloc(dpy_x * dpy_y * 2);
		} else {
			d = (char *) malloc(dpy_x * dpy_y * 1);
		}
	} else {
		return (XImage *) NULL;
	}
	return XCreateImage(dpy, attr.visual, win_depth, ZPixmap, 0, d, dpy_x, dpy_y, 8, 0);
#endif	/* NO_X11 */
}


static void transform_rect(sraRect rect, Window win, int win_depth, int cm) {
#if NO_X11
	RAWFB_RET_VOID
	if (!rect.x1 || !win || !win_depth || !cm) {}
	return;
#else

	char *src, *dst, *poll;
	unsigned int *ui;
	unsigned short *us;
	unsigned char *uc;
	int ps, pixelsize = bpp/8;
	int poll_Bpl;

	int do_getimage = xgetimage_8to24;
	int line, n_off, j, h, w;
	unsigned int hi, idx;
	XWindowAttributes attr;
	XErrorHandler old_handler = NULL;

if (db24 > 1) fprintf(stderr, "transform %4d %4d %4d %4d cm: %d\n", rect.x1, rect.y1, rect.x2, rect.y2, cm);

	RAWFB_RET_VOID

	attr.width = 0;
	attr.height = 0;

	/* now transform the pixels in this rectangle: */
	n_off = main_bytes_per_line * rect.y1 + pixelsize * rect.x1;

	h = rect.y2 - rect.y1;
	w = rect.x2 - rect.x1;

	if (depth != 24) {
		/* need to fetch depth 24 data. */
		do_getimage = 1;
	}

#if 0
	if (do_getimage) {
		X_LOCK;
		vw = valid_window(win, &attr, 1);
		X_UNLOCK;
	}

	if (do_getimage && vw) {
#else
	if (do_getimage) {
#endif
		static XImage *xi_8  = NULL;
		static XImage *xi_24 = NULL;
		XImage *xi = NULL, *xi_r;
		Window c;
		unsigned int wu, hu;
		int xo, yo;

		wu = (unsigned int) w;
		hu = (unsigned int) h;

		X_LOCK;
#define GETSUBIMAGE
#ifdef GETSUBIMAGE
		if (win_depth == 24) {
			if (xi_24 == NULL || xi_24->width != dpy_x ||
			    xi_24->height != dpy_y) {
				xi_24 = cmap_xi(xi_24, win, 24);
			}
			xi = xi_24;
		} else if (win_depth <= 16) {
			if (xi_8 == NULL || xi_8->width != dpy_x ||
			    xi_8->height != dpy_y) {
				if (win_depth > 8) {
					/* XXX */
					xi_8 = cmap_xi(xi_8, win, 16);
				} else {
					xi_8 = cmap_xi(xi_8, win, 8);
				}
			}
			xi = xi_8;
		}
#endif

		if (xi == NULL) {
			rfbLog("transform_rect: xi is NULL\n");
			X_UNLOCK;
			clean_up_exit(1);
		}

		old_handler = XSetErrorHandler(trap_xerror);
		trapped_xerror = 0;

		XTranslateCoordinates(dpy, win, window, 0, 0, &xo, &yo, &c);

		xo = rect.x1 - xo;
		yo = rect.y1 - yo;

if (db24 > 1) fprintf(stderr, "xywh: %d %d %d %d vs. %d %d\n", xo, yo, w, h, attr.width, attr.height);

		if (trapped_xerror || xo < 0 || yo < 0) {
		        /* w > attr.width || h > attr.height */
			XSetErrorHandler(old_handler);
			X_UNLOCK;
			trapped_xerror = 0;
if (db24 > 1) fprintf(stderr, "skipping due to potential bad match...\n");
			return;
		}
		trapped_xerror = 0;

#ifndef GETSUBIMAGE
		xi = XGetImage(dpy, win, xo, yo, wu, hu, AllPlanes, ZPixmap);
		xi_r = xi;
#else
		xi_r = XGetSubImage(dpy, win, xo, yo, wu, hu, AllPlanes,
		    ZPixmap, xi, 0, 0);
#endif
		XSetErrorHandler(old_handler);

		X_UNLOCK;

		if (! xi_r || trapped_xerror) {
			trapped_xerror = 0;
if (db24 > 1) fprintf(stderr, "xi-fail: 0x%p trap=%d  %d %d %d %d\n", (void *)xi, trapped_xerror, xo, yo, w, h);
			return;
		} else {
if (db24 > 1) fprintf(stderr, "xi: 0x%p  %d %d %d %d -- %d %d\n", (void *)xi, xo, yo, w, h, xi->width, xi->height);
		}
		trapped_xerror = 0;

		if (xi->depth > 16 && xi->depth != 24) {
#ifndef GETSUBIMAGE
			X_LOCK;
			XDestroyImage(xi);
			X_UNLOCK;
#endif
if (db24) fprintf(stderr, "xi: wrong depth: %d\n", xi->depth);
			return;
		}

		set_poll_fb();

		if (xi->depth == 24) {
			/* line by line ... */
			int ps1 = 4, fac;
			if (depth <= 8) {
				fac = 4;
			} else if (depth <= 16) {
				fac = 2;
			} else {
				fac = 1;	/* will not happen 24 on 24 */
			}

			src = xi->data;
			dst = cmap8to24_fb + fac * n_off;

			poll = poll24_fb + (poll24_fb_w * rect.y1 + rect.x1) * 4;
			poll_Bpl = poll24_fb_w * 4;

			for (line = 0; line < h; line++) {
				memcpy(dst,  src, w * ps1);
				memcpy(poll, src, w * ps1);

				src += xi->bytes_per_line;
				dst += main_bytes_per_line * fac;
				poll += poll_Bpl;
			}
		} else if (xi->depth <= 16) {
			int ps1, ps2, fac;

			if (depth <= 8) {
				ps1 = 1;
				ps2 = 4;
				fac = 4;
			} else if (depth <= 16) {
				ps1 = 2;
				ps2 = 4;
				fac = 4;
			} else {
				/* should be 24 case */
				ps1 = 1;
				ps2 = pixelsize;
				fac = 1;
			}

			src = xi->data;
			dst = cmap8to24_fb + (fac/ps1) * n_off;

			poll = poll8_fb + poll8_fb_w * rect.y1 * ps1 + rect.x1 * ps1;
			poll_Bpl = poll8_fb_w * ps1;

			/* line by line ... */
			for (line = 0; line < h; line++) {
				/* pixel by pixel... */
				for (j = 0; j < w; j++) {
					if (ps1 == 2) {
						unsigned short *ptmp;
						us    = (unsigned short *) (src + ps1 * j);
						idx   = (int) (*us);
						ptmp  = (unsigned short *) (poll + ps1 * j);
						*ptmp = *us;
					} else {
						uc  = (unsigned char *) (src + ps1 * j);
						idx = (int) (*uc);
						*(poll + ps1 * j) = *uc;
					}
					ui = (unsigned int *) (dst + ps2 * j);
					*ui = rgb[cm][idx];

				}
				src += xi->bytes_per_line;
				dst += main_bytes_per_line * (fac/ps1);
				poll += poll_Bpl;
			}
		}

#ifndef GETSUBIMAGE
		X_LOCK;
		XDestroyImage(xi);
		X_UNLOCK;
#endif

	} else if (! do_getimage) {
		int fac;

		if (depth <= 16) {
			/* cooked up depth 24 TrueColor  */
			/* but currently disabled (high bits no useful?) */
			ps = 4;
			fac = 4;
			/* XXX not correct for depth > 8, but do we ever come here in that case? */
			src = cmap8to24_fb + 4 * n_off;
		} else {
			ps = pixelsize;
			fac = 1;
			src = cmap8to24_fb + n_off;
		}
		
		/* line by line ... */
		for (line = 0; line < h; line++) {
			/* pixel by pixel... */
			for (j = 0; j < w; j++) {

				/* grab 32 bit value */
				ui = (unsigned int *) (src + ps * j);

				/* extract top 8 bits (FIXME: masks?) */
				hi = (*ui) & 0xff000000; 

				/* map to lookup index; rewrite pixel */
				idx = hi >> 24;
				*ui = hi | rgb[cm][idx];
			}
			src += main_bytes_per_line * fac;
		}
	}
#endif	/* NO_X11 */
}

void bpp8to24(int x1, int y1, int x2, int y2) {
	char *src, *dst;
	unsigned char *uc;
	unsigned short *us;
	unsigned int *ui;
	int idx, pixelsize = bpp/8;
	int line, k, i, j, h, w;
	int n_off;
	sraRegionPtr rect;
	int validate = 1;
	static int last_map_count = 0, call_count = 0;
	static double last_get_8bpp_validate = 0.0;
	static double last_snapshot = 0.0;
	double now;
	double dt, d0 = 0.0, t2;

	RAWFB_RET_VOID

	if (! cmap8to24 || ! cmap8to24_fb) {
		/* hmmm, why were we called? */
		return;
	}

if (db24 > 1) fprintf(stderr, "bpp8to24 %d %d %d %d %.4f\n", x1, y1, x2, y2, dnow() - last_get_8bpp_validate);

	call_count++;

	/* clip to display just in case: */
	if (!ncache) {
		x1 = nfix(x1, dpy_x);
		y1 = nfix(y1, dpy_y);
		x2 = nfix(x2, dpy_x+1);
		y2 = nfix(y2, dpy_y+1);
	}

	if (wireframe_in_progress) {
		/*
		 * draw_box() manages cmap8to24_fb for us so we get out as
		 * soon as we can.  No need to cp main_fb -> cmap8to24_fb.
		 */
		return;
	}

	/* copy from main_fb to cmap8to24_fb regardless of 8bpp windows: */

	h = y2 - y1;
	w = x2 - x1;

	if (depth == 24) {
		/* pixelsize = 4 */
		n_off = main_bytes_per_line * y1 + pixelsize * x1;

		src = main_fb      + n_off;
		dst = cmap8to24_fb + n_off;

		/* otherwise, the pixel data as is */
		for (line = 0; line < h; line++) {
			memcpy(dst, src, w * pixelsize);
			src += main_bytes_per_line;
			dst += main_bytes_per_line;
		}
	} else if (depth <= 16) {
		/* need to cook up to depth 24 TrueColor  */
		int ps1 = 1, ps2 = 4;
		if (depth > 8) {
			ps1 = 2;
		}

		/* pixelsize = 1, 2 */
		n_off = main_bytes_per_line * y1 + pixelsize * x1;

		src = main_fb + n_off;
		dst = cmap8to24_fb + (4/ps1) * n_off;

		set_root_cmap();
		if (root_cmap) {
#if 0
			unsigned int hi;
#endif

			/* line by line ... */
			for (line = 0; line < h; line++) {
				/* pixel by pixel... */
				for (j = 0; j < w; j++) {
					if (ps1 == 2) {
						us = (unsigned short *) (src + ps1 * j);
						idx = (int) (*us);
					} else {
						uc = (unsigned char *)  (src + ps1 * j);
						idx = (int) (*uc);
					}
					ui = (unsigned int *)  (dst + ps2 * j);

if (0 && line % 100 == 0 && j % 32 == 0) fprintf(stderr, "%d %d %u  x1=%d y1=%d\n", line, j, root_rgb[idx], x1, y1);
#if 0
					if (do_hibits) {
						hi = idx << 24;
						*ui = hi | rgb[0][idx];
					} else {
					}
#endif
					*ui = root_rgb[idx];
if (db24 > 2) histo[idx]++;
				}
				src += main_bytes_per_line;
				dst += main_bytes_per_line * (4/ps1);
			}
		}
		
	}

	if (last_map_count > MAX_8BPP_WINDOWS/4) {
		/* table is filling up... skip validating sometimes: */
		int skip = 3;
		if (last_map_count > MAX_8BPP_WINDOWS/2) {
			skip = 6;
		} else if (last_map_count > 3*MAX_8BPP_WINDOWS/4) {
			skip = 12;
		}
		if (call_count % skip != 0) {
			validate = 0;
		}
	}

if (db24 > 2) {for(i=0;i<256;i++){histo[i]=0;}}

	now = dnow();
	dt = now - last_get_8bpp_validate;
	/* TUNABLES  */
	if (dt < 0.003) {
		;	/* XXX does this still give painting errors? */
	} else {
		int snapit = 0;
		double delay1, delay2, delay3;
		if (poll_8to24_delay >= POLL_8TO24_DELAY) {
			delay1 = 1.0 * poll_8to24_delay;
			delay2 = 2.0 * poll_8to24_delay;
			delay3 = 10. * poll_8to24_delay;
		} else {
			delay1 = 1.0 * POLL_8TO24_DELAY;	/* 0.05 */
			delay2 = 2.0 * POLL_8TO24_DELAY;	/* 0.1  */
			delay3 = 10. * POLL_8TO24_DELAY;	/* 0.5  */
		}
		if (cache_win > 1.0) {
			delay2 *= 2;
			delay3 *= 2;
		}
		if (dt < delay1) {
			validate = 0;
		}
		if (last_map_count) {
			if (now > last_snapshot + delay2) {
				snapit = 1;
			}
		} else {
			if (now > last_snapshot + delay3) {
				snapit = 1;
			}
		}

		if (snapit) {
			/* less problems if we update the stack frequently */
			snapshot_stack_list(0, 0.0);
if (0) fprintf(stderr, "SNAP time: %.4f\n", dnow() - now);
			update_stack_list();
			last_snapshot = dnow();
if (0) fprintf(stderr, "UPDA time: %.4f\n", last_snapshot - now);
		}

if (0) t2 = dnow();
		last_map_count = get_8bpp_regions(validate);
		if (validate) {
			last_get_8bpp_validate = dnow();
		}
if (0) fprintf(stderr, "get8bpp-%d: %.4f\n", validate, dnow() - t2);
	}
if (db24) d0 = dnow();

if (db24 > 1) fprintf(stderr, "bpp8to24 w=%d h=%d m=%p c=%p r=%p ncmaps=%d\n", w, h, main_fb, cmap8to24_fb, rfb_fb, ncmaps);

	/*
	 * now go back and transform and 8bpp regions to TrueColor in
	 * cmap8to24_fb.
	 */
	if (last_map_count && (ncmaps || depth <= 16)) {
		int i, j;
		int win[MAX_8BPP_WINDOWS];
		int did[MAX_8BPP_WINDOWS];
		int count = 0;

		/*
		 * first, grab all of the associated colormaps from the
		 * X server.  Hopefully just 1 or 2...
		 */
		for (j=0; j<ncmaps; j++) {
			if (! get_cmap(j, cmaps[j])) {
				cmap_failed[j] = 1;
			} else {
				cmap_failed[j] = 0;
			}
if (db24 > 2) fprintf(stderr, "cmap %d  %.4f\n", (int) cmaps[j], dnow() - d0);
		}
		for (i=0; i < MAX_8BPP_WINDOWS; i++) {
			sraRegionPtr reg = windows_8bpp[i].clip_region;
			if (reg) {
				rect = sraRgnCreateRect(x1, y1, x2, y2);
				if (sraRgnAnd(rect, reg)) {
					win[count] = i;
					did[count++] = 0;
				}
				sraRgnDestroy(rect);
			}
		}

		if (count) {

			rect = sraRgnCreateRect(x1, y1, x2, y2);
			/* try to apply lower windows first */
			for (k=0; k < stack_list_num; k++) {
				Window swin = stack_list[k].win;
				for (j=0; j<count; j++) {
					i = win[j];
					if (did[j]) {
						continue;
					}
					if (windows_8bpp[i].top == swin) {
						do_8bpp_region(i, rect);
						did[j] = 1;
						break;
					}
				}
			}
			for (j=0; j<count; j++) {
				if (! did[j]) {
					i = win[j];
					do_8bpp_region(i, rect);
					did[j] = 1;
				}
			}
			sraRgnDestroy(rect);
		}
	}
if (0) fprintf(stderr, "done time: %.4f\n", dnow() - d0);

if (db24 > 2) {for(i=0; i<256;i++) {fprintf(stderr, " cmap histo[%03d] %d\n", i, histo[i]);}}
}

void mark_8bpp(int mode) {
	int i, cnt = 0;
	Window top = None;

	RAWFB_RET_VOID

	if (! cmap8to24 || !cmap8to24_fb) {
		return;
	}
	
	if (mode == MARK_8BPP_TOP) {
		int k;
		for (k = stack_list_num - 1; k >= 0; k--) {
			Window swin = stack_list[k].win;
			for (i=0; i < MAX_8BPP_WINDOWS; i++) {
				if (windows_8bpp[i].win == None) {
					continue;
				}
				if (windows_8bpp[i].map_state != IsViewable) {
					continue;
				}
				if (swin == windows_8bpp[i].top) {
					top = swin;
					break;
				}
			}
			if (top != None) {
				break;
			}
		}
	}

	/* for each mapped 8bpp window, mark it changed: */

	for (i=0; i < MAX_8BPP_WINDOWS; i++) {
		int x1, y1, x2, y2, w, h, f = 32;

		f = 0;	/* skip fuzz, may bring in other windows... */

		if (windows_8bpp[i].win == None) {
			continue;
		}
		if (mode == MARK_8BPP_TOP) {
			if (windows_8bpp[i].top != top) {
				continue;
			}
		}
		if (windows_8bpp[i].map_state != IsViewable) {
			XWindowAttributes attr;
			int vw = 0;

			X_LOCK;
			vw = valid_window(windows_8bpp[i].win, &attr, 1);
			X_UNLOCK;
			if (vw) {
				if (attr.map_state != IsViewable) {
					continue;
				}
			} else {
				continue;
			}
		}

		x1 = windows_8bpp[i].x;
		y1 = windows_8bpp[i].y;
		w  = windows_8bpp[i].w;
		h  = windows_8bpp[i].h;

		x2 = x1 + w;
		y2 = y1 + h;

		if (mode == MARK_8BPP_POINTER) {
			int b = 32;	/* apply some fuzz for wm border */
			if (cursor_x < x1 - b || cursor_y < y1 - b) {
				continue;
			}
			if (cursor_x > x2 + b || cursor_y > y2 + b) {
				continue;
			}
		}

		/* apply fuzz f around each one; constrain to screen */
		x1 = nfix(x1 - f, dpy_x);
		y1 = nfix(y1 - f, dpy_y);
		x2 = nfix(x2 + f, dpy_x+1);
		y2 = nfix(y2 + f, dpy_y+1);

if (db24 > 1) fprintf(stderr, "mark_8bpp: 0x%lx %d %d %d %d\n", windows_8bpp[i].win, x1, y1, x2, y2);

		mark_rect_as_modified(x1, y1, x2, y2, 0);
		cnt++;
	}
	if (cnt) {
		/* push it to viewers if possible. */
		rfbPE(-1);
	}
}

#endif /* SKIP_8TO24 */

