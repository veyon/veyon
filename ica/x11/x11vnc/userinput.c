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

/* -- userinput.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xdamage.h"
#include "xrecord.h"
#include "xinerama.h"
#include "win_utils.h"
#include "xevents.h"
#include "user.h"
#include "scan.h"
#include "cleanup.h"
#include "pointer.h"
#include "rates.h"
#include "keyboard.h"
#include "solid.h"
#include "xrandr.h"
#include "8to24.h"
#include "unixpw.h"
#include "macosx.h"
#include "macosxCGS.h"
#include "cursor.h"
#include "screen.h"
#include "connections.h"

/*
 * user input handling heuristics
 */
int defer_update_nofb = 4;	/* defer a shorter time under -nofb */
int last_scroll_type = SCR_NONE;


int get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win);
void parse_scroll_copyrect(void);
void parse_fixscreen(void);
void parse_wireframe(void);

void set_wirecopyrect_mode(char *str);
void set_scrollcopyrect_mode(char *str);
void initialize_scroll_keys(void);
void initialize_scroll_matches(void);
void initialize_scroll_term(void);
void initialize_max_keyrepeat(void);

int direct_fb_copy(int x1, int y1, int x2, int y2, int mark);
void fb_push(void);
int fb_push_wait(double max_wait, int flags);
void eat_viewonly_input(int max_eat, int keep);

void mark_for_xdamage(int x, int y, int w, int h);
void mark_region_for_xdamage(sraRegionPtr region);
void set_xdamage_mark(int x, int y, int w, int h);
int near_wm_edge(int x, int y, int w, int h, int px, int py);
int near_scrollbar_edge(int x, int y, int w, int h, int px, int py);

void check_fixscreen(void);
int check_xrecord(void);
int check_wireframe(void);
int fb_update_sent(int *count);
int check_user_input(double dt, double dtr, int tile_diffs, int *cnt);
void do_copyregion(sraRegionPtr region, int dx, int dy, int mode);

int check_ncache(int reset, int mode);
int find_rect(int idx, int x, int y, int w, int h);
int bs_restore(int idx, int *nbatch, sraRegionPtr rmask, XWindowAttributes *attr, int clip, int nopad, int *valid, int verb);
int try_to_fix_su(Window win, int idx, Window above, int *nbatch, char *mode);
int try_to_fix_resize_su(Window orig_frame, int orig_x, int orig_y, int orig_w, int orig_h,
    int x, int y, int w, int h, int try_batch);
int lookup_win_index(Window);
void set_ncache_xrootpmap(void);

static void get_client_regions(int *req, int *mod, int *cpy, int *num) ;
static void parse_scroll_copyrect_str(char *scr);
static void parse_wireframe_str(char *wf);
static void destroy_str_list(char **list);
static void draw_box(int x, int y, int w, int h, int restore);
static int do_bdpush(Window wm_win, int x0, int y0, int w0, int h0, int bdx,
    int bdy, int bdskinny);
static int set_ypad(void);
static void scale_mark(int x1, int y1, int x2, int y2, int mark);
static int push_scr_ev(double *age, int type, int bdpush, int bdx, int bdy,
    int bdskinny, int first_push);
static int crfix(int x, int dx, int Lx);
static int scrollability(Window win, int set);
static int eat_pointer(int max_ptr_eat, int keep);
static void set_bdpush(int type, double *last_bdpush, int *pushit);
static int repeat_check(double last_key_scroll);
static int check_xrecord_keys(void);
static int check_xrecord_mouse(void);
static int try_copyrect(Window orig_frame, Window frame, int x, int y, int w, int h,
    int dx, int dy, int *obscured, sraRegionPtr extra_clip, double max_wait, int *nbatch);
static int wireframe_mod_state();
static void check_user_input2(double dt);
static void check_user_input3(double dt, double dtr, int tile_diffs);
static void check_user_input4(double dt, double dtr, int tile_diffs);

winattr_t *cache_list;

/*
 * For -wireframe: find the direct child of rootwin that has the
 * pointer, assume that is the WM frame that contains the application
 * (i.e. wm reparents the app toplevel) return frame position and size
 * if successful.
 */
int get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win) {
#if !NO_X11
	Window r, c;
	XWindowAttributes attr;
	Bool ret;
	int rootx, rooty, wx, wy;
	unsigned int mask;
#endif

#ifdef MACOSX
	if (macosx_console) {
		return macosx_get_wm_frame_pos(px, py, x, y, w, h, frame, win);
	}
#endif

	RAWFB_RET(0)

#if NO_X11
	if (!px || !py || !x || !y || !w || !h || !frame || !win) {}
	return 0;
#else


	ret = XQueryPointer_wr(dpy, rootwin, &r, &c, &rootx, &rooty, &wx, &wy,
	    &mask);

	*frame = c;

	/* current pointer position is returned too */
	*px = rootx;
	*py = rooty;

	if (!ret || ! c || c == rootwin) {
		/* no immediate child */
		return 0;
	}

	/* child window position and size */
	if (! valid_window(c, &attr, 1)) {
		return 0;
	}

	*x = attr.x;
	*y = attr.y;
	*w = attr.width;
	*h = attr.height;

#if 0
	/* more accurate, but the animation is bogus anyway */
	if (attr.border_width > 0) {
		*w += 2 * attr.border_width;
		*h += 2 * attr.border_width;
	}
#endif

	if (win != NULL) {
		*win = descend_pointer(5, c, NULL, 0);
	}

	return 1;
#endif	/* NO_X11 */
}

static int scrollcopyrect_top, scrollcopyrect_bot;
static int scrollcopyrect_left, scrollcopyrect_right;
static double scr_key_time, scr_key_persist;
static double scr_mouse_time, scr_mouse_persist, scr_mouse_maxtime;
static double scr_mouse_pointer_delay;
static double scr_key_bdpush_time, scr_mouse_bdpush_time;

static void parse_scroll_copyrect_str(char *scr) {
	char *p, *str;
	int i;
	char *part[16];

	for (i=0; i<16; i++) {
		part[i] = NULL;
	}

	if (scr == NULL || *scr == '\0') {
		return;
	}

	str = strdup(scr);

	p = strtok(str, ",");
	i = 0;
	while (p) {
		part[i++] = strdup(p);
		p = strtok(NULL, ",");
		if (i >= 16) break;
	}
	free(str);


	/*
	 * Top, Bottom, Left, Right tolerances for scrollbar locations.
	 */
	if ((str = part[0]) != NULL) {
		int t, b, l, r;
		if (sscanf(str, "%d+%d+%d+%d", &t, &b, &l, &r) == 4) {
			scrollcopyrect_top   = t;
			scrollcopyrect_bot   = b;
			scrollcopyrect_left  = l;
			scrollcopyrect_right = r;
		}
		free(str);
	}

	/* key scrolling timing heuristics. */
	if ((str = part[1]) != NULL) {
		double t1, t2, t3;
		if (sscanf(str, "%lf+%lf+%lf", &t1, &t2, &t3) == 3) {
			scr_key_time = t1;
			scr_key_persist = t2;
			scr_key_bdpush_time = t3;
		}
		free(str);
	}

	/* mouse scrolling timing heuristics. */
	if ((str = part[2]) != NULL) {
		double t1, t2, t3, t4, t5;
		if (sscanf(str, "%lf+%lf+%lf+%lf+%lf", &t1, &t2, &t3, &t4,
		    &t5) == 5) {
			scr_mouse_time = t1;
			scr_mouse_persist = t2;
			scr_mouse_bdpush_time = t3;
			scr_mouse_pointer_delay = t4;
			scr_mouse_maxtime = t5;
		}
		free(str);
	}
}

void parse_scroll_copyrect(void) {
	parse_scroll_copyrect_str(SCROLL_COPYRECT_PARMS);
	if (! scroll_copyrect_str) {
		scroll_copyrect_str = strdup(SCROLL_COPYRECT_PARMS);
	}
	parse_scroll_copyrect_str(scroll_copyrect_str);
}

void parse_fixscreen(void) {
	char *str, *p;

	screen_fixup_V = 0.0;
	screen_fixup_C = 0.0;
	screen_fixup_X = 0.0;
	screen_fixup_8 = 0.0;

	if (! screen_fixup_str) {
		return;
	}

	str = strdup(screen_fixup_str);

	p = strtok(str, ",");
	while (p) {
		double t;
		if (*p == 'V' && sscanf(p, "V=%lf", &t) == 1) {
			screen_fixup_V = t;
		} else if (*p == 'C' && sscanf(p, "C=%lf", &t) == 1) {
			screen_fixup_C = t;
		} else if (*p == 'X' && sscanf(p, "X=%lf", &t) == 1) {
			screen_fixup_X = t;
		} else if (*p == 'X' && sscanf(p, "8=%lf", &t) == 1) {
			screen_fixup_8 = t;
		}
		p = strtok(NULL, ",");
	}
	free(str);

	if (screen_fixup_V < 0.0) screen_fixup_V = 0.0;
	if (screen_fixup_C < 0.0) screen_fixup_C = 0.0;
	if (screen_fixup_X < 0.0) screen_fixup_X = 0.0;
	if (screen_fixup_8 < 0.0) screen_fixup_8 = 0.0;
}

/*
WIREFRAME_PARMS "0xff,2,0,30+6+6+6,Alt,0.05+0.3+2.0,8"
                 0xff,2,0,32+8+8+8,all,0.15+0.30+5.0+0.125
shade,linewidth,percent,T+B+L+R,mods,t1+t2+t3+t4
 */
#define LW_MAX 8
static unsigned long wireframe_shade = 0xff;
static int wireframe_lw;
static double wireframe_frac;
static int wireframe_top, wireframe_bot, wireframe_left, wireframe_right;
static double wireframe_t1, wireframe_t2, wireframe_t3, wireframe_t4;
static char *wireframe_mods = NULL;

/*
 * Parse the gory -wireframe string for parameters.
 */
static void parse_wireframe_str(char *wf) {
	char *p, *str;
	int i;
	char *part[16];

	for (i=0; i<16; i++) {
		part[i] = NULL;
	}

	if (wf == NULL || *wf == '\0') {
		return;
	}

	str = strdup(wf);

	/* leading ",", make it start with ignorable string "z" */
	if (*str == ',') {
		char *tmp = (char *) malloc(strlen(str)+2);	
		strcpy(tmp, "z");
		strcat(tmp, str);
		free(str);
		str = tmp;
	}

	p = strtok(str, ",");
	i = 0;
	while (p) {
		part[i++] = strdup(p);
		p = strtok(NULL, ",");
		if (i >= 16) break;
	}
	free(str);


	/* Wireframe shade, color, RGB: */
	if ((str = part[0]) != NULL) {
		unsigned long n;
		int r, g, b, ok = 0;
		XColor cdef;
		Colormap cmap;
		if (dpy && (bpp == 32 || bpp == 16)) {
#if !NO_X11
			X_LOCK;
		 	cmap = DefaultColormap (dpy, scr);
			if (XParseColor(dpy, cmap, str, &cdef) &&
			    XAllocColor(dpy, cmap, &cdef)) {
				r = cdef.red   >> 8;
				g = cdef.green >> 8;
				b = cdef.blue  >> 8;
				if (r == 0 && g == 0) {
					g = 1;	/* need to be > 255 */
				}
				n = 0;
				n |= (r << main_red_shift);
				n |= (g << main_green_shift);
				n |= (b << main_blue_shift);
				wireframe_shade = n;
				ok = 1;
			}
			X_UNLOCK;
#else
			r = g = b = 0;
			cmap = 0;
			cdef.pixel = 0;
#endif
		}
		if (ok) {
			;
		} else if (sscanf(str, "0x%lx", &n) == 1) {
			wireframe_shade = n;	
		} else if (sscanf(str, "%lu", &n) == 1) {
			wireframe_shade = n;	
		} else if (sscanf(str, "%lx", &n) == 1) {
			wireframe_shade = n;	
		}
		free(str);
	}

	/* linewidth: # of pixels wide for the wireframe lines */
	if ((str = part[1]) != NULL) {
		int n;
		if (sscanf(str, "%d", &n) == 1) {
			wireframe_lw = n;	
			if (wireframe_lw < 1) {
				wireframe_lw = 1; 
			}
			if (wireframe_lw > LW_MAX) {
				wireframe_lw = LW_MAX; 
			}
		}
		free(str);
	}

	/* percentage cutoff for opaque move/resize (like WM's) */
	if ((str = part[2]) != NULL) {
		if (*str == '\0') {
			;
		} else if (strchr(str, '.')) {
			wireframe_frac = atof(str);
		} else {
			wireframe_frac = ((double) atoi(str))/100.0;
		}
		free(str);
	}

	/*
	 * Top, Bottom, Left, Right tolerances to guess the wm frame is
	 * being grabbed (Top is traditionally bigger, i.e. titlebar):
	 */
	if ((str = part[3]) != NULL) {
		int t, b, l, r;
		if (sscanf(str, "%d+%d+%d+%d", &t, &b, &l, &r) == 4) {
			wireframe_top   = t;
			wireframe_bot   = b;
			wireframe_left  = l;
			wireframe_right = r;
		}
		free(str);
	}

	/*
	 * wireframe in interior with Modifier down.
	 * 0 => no mods
	 * 1 => all mods
	 * Shift,Alt,Control,Meta,Super,Hyper
	 */
	if (wireframe_mods) {
		free(wireframe_mods);
	}
	wireframe_mods = NULL;
	if ((str = part[4]) != NULL) {
		if (*str == '0' || !strcmp(str, "none")) {
			;
		} else if (*str == '1' || !strcmp(str, "all")) {
			wireframe_mods = strdup("all");	
		} else if (!strcmp(str, "Alt") || !strcmp(str, "Shift")
		    || !strcmp(str, "Control") || !strcmp(str, "Meta")
		    || !strcmp(str, "Super") || !strcmp(str, "Hyper")) {
			wireframe_mods = strdup(str);
		}
	}

	/* check_wireframe() timing heuristics. */
	if ((str = part[5]) != NULL) {
		double t1, t2, t3, t4;
		if (sscanf(str, "%lf+%lf+%lf+%lf", &t1, &t2, &t3, &t4) == 4) {
			wireframe_t1 = t1;
			wireframe_t2 = t2;
			wireframe_t3 = t3;
			wireframe_t4 = t4;
		}
		free(str);
	}
}

/*
 * First parse the defaults and apply any user supplied ones (may be a subset)
 */
void parse_wireframe(void) {
	parse_wireframe_str(WIREFRAME_PARMS);
	if (! wireframe_str) {
		wireframe_str = strdup(WIREFRAME_PARMS);
	}
	parse_wireframe_str(wireframe_str);
}

/*
 * Set wireframe_copyrect based on desired mode.
 */
void set_wirecopyrect_mode(char *str) {
	char *orig = wireframe_copyrect;
	if (str == NULL || *str == '\0') {
		wireframe_copyrect = strdup(wireframe_copyrect_default);
	} else if (!strcmp(str, "always") || !strcmp(str, "all")) {
		wireframe_copyrect = strdup("always");
	} else if (!strcmp(str, "top")) {
		wireframe_copyrect = strdup("top");
	} else if (!strcmp(str, "never") || !strcmp(str, "none")) {
		wireframe_copyrect = strdup("never");
	} else {
		if (! wireframe_copyrect) {
			wireframe_copyrect = strdup(wireframe_copyrect_default);
		} else {
			orig = NULL;
		}
		rfbLog("unknown -wirecopyrect mode: %s, using: %s\n", str,
		    wireframe_copyrect);
	}
	if (orig) {
		free(orig);
	}
}

/*
 * Set scroll_copyrect based on desired mode.
 */
void set_scrollcopyrect_mode(char *str) {
	char *orig = scroll_copyrect;
	if (str == NULL || *str == '\0') {
		scroll_copyrect = strdup(scroll_copyrect_default);
	} else if (!strcmp(str, "always") || !strcmp(str, "all") ||
		    !strcmp(str, "both")) {
		scroll_copyrect = strdup("always");
	} else if (!strcmp(str, "keys") || !strcmp(str, "keyboard")) {
		scroll_copyrect = strdup("keys");
	} else if (!strcmp(str, "mouse") || !strcmp(str, "pointer")) {
		scroll_copyrect = strdup("mouse");
	} else if (!strcmp(str, "never") || !strcmp(str, "none")) {
		scroll_copyrect = strdup("never");
	} else {
		if (! scroll_copyrect) {
			scroll_copyrect = strdup(scroll_copyrect_default);
		} else {
			orig = NULL;
		}
		rfbLog("unknown -scrollcopyrect mode: %s, using: %s\n", str,
		    scroll_copyrect);
	}
	if (orig) {
		free(orig);
	}
}

void initialize_scroll_keys(void) {
	char *str, *p;
	int i, nkeys = 0, saw_builtin = 0;
	int ks_max = 2 * 0xFFFF;

	if (scroll_key_list) {
		free(scroll_key_list);
		scroll_key_list = NULL;
	}
	if (! scroll_key_list_str || *scroll_key_list_str == '\0') {
		return;
	}

	if (strstr(scroll_key_list_str, "builtin")) {
		int k;
		/* add in number of keysyms builtin gives */
		for (k=1; k<ks_max; k++)  {
			if (xrecord_scroll_keysym((rfbKeySym) k)) {
				nkeys++;
			}
		}
	}

	nkeys++;	/* first key, i.e. no commas. */
	p = str = strdup(scroll_key_list_str);
	while(*p) {
		if (*p == ',') {
			nkeys++;	/* additional key. */
		}
		p++;
	}
	
	nkeys++;	/* exclude/include 0 element */
	nkeys++;	/* trailing NoSymbol */

	scroll_key_list = (KeySym *) malloc(nkeys*sizeof(KeySym)); 
	for (i=0; i<nkeys; i++) {
		scroll_key_list[i] = NoSymbol;
	}
	if (*str == '-') {
		scroll_key_list[0] = 1;
		p = strtok(str+1, ",");
	} else {
		p = strtok(str, ",");
	}
	i = 1;
	while (p) {
		if (!strcmp(p, "builtin")) {
			int k;
			if (saw_builtin) {
				p = strtok(NULL, ",");
				continue;
			}
			saw_builtin = 1;
			for (k=1; k<ks_max; k++)  {
				if (xrecord_scroll_keysym((rfbKeySym) k)) {
					scroll_key_list[i++] = (rfbKeySym) k;
				}
			}
		} else {
			unsigned int in;
			if (sscanf(p, "%u", &in) == 1) {
				scroll_key_list[i++] = (rfbKeySym) in;
			} else if (sscanf(p, "0x%x", &in) == 1) {
				scroll_key_list[i++] = (rfbKeySym) in;
			} else if (XStringToKeysym(p) != NoSymbol) { 
				scroll_key_list[i++] = XStringToKeysym(p);
			} else {
				rfbLog("initialize_scroll_keys: skip unknown "
				    "keysym: %s\n", p);
			}
		}
		p = strtok(NULL, ",");
	}
	free(str);
}

static void destroy_str_list(char **list) {
	int i = 0;
	if (! list) {
		return;
	}
	while (list[i] != NULL) {
		free(list[i++]);
	}
	free(list);
}

void initialize_scroll_matches(void) {
	char *str, *imp = "__IMPOSSIBLE_STR__";
	int i, n, nkey, nmouse;

	destroy_str_list(scroll_good_all);
	scroll_good_all = NULL;
	destroy_str_list(scroll_good_key);
	scroll_good_key = NULL;
	destroy_str_list(scroll_good_mouse);
	scroll_good_mouse = NULL;

	destroy_str_list(scroll_skip_all);
	scroll_skip_all = NULL;
	destroy_str_list(scroll_skip_key);
	scroll_skip_key = NULL;
	destroy_str_list(scroll_skip_mouse);
	scroll_skip_mouse = NULL;

	/* scroll_good: */
	if (scroll_good_str != NULL && *scroll_good_str != '\0') {
		str = scroll_good_str;
	} else {
		str = scroll_good_str0;
	}
	scroll_good_all = create_str_list(str);

	nkey = 0;
	nmouse = 0;
	n = 0;
	while (scroll_good_all[n] != NULL) {
		char *s = scroll_good_all[n++];
		if (strstr(s, "KEY:") == s) nkey++;
		if (strstr(s, "MOUSE:") == s) nmouse++;
	}
	if (nkey++) {
		scroll_good_key = (char **) malloc(nkey*sizeof(char *));
		for (i=0; i<nkey; i++) scroll_good_key[i] = NULL;
	}
	if (nmouse++) {
		scroll_good_mouse = (char **) malloc(nmouse*sizeof(char *));
		for (i=0; i<nmouse; i++) scroll_good_mouse[i] = NULL;
	}
	nkey = 0;
	nmouse = 0;
	for (i=0; i<n; i++) {
		char *s = scroll_good_all[i];
		if (strstr(s, "KEY:") == s) {
			scroll_good_key[nkey++] = strdup(s+strlen("KEY:"));
			free(s);
			scroll_good_all[i] = strdup(imp);
		} else if (strstr(s, "MOUSE:") == s) {
			scroll_good_mouse[nmouse++]=strdup(s+strlen("MOUSE:"));
			free(s);
			scroll_good_all[i] = strdup(imp);
		}
	}

	/* scroll_skip: */
	if (scroll_skip_str != NULL && *scroll_skip_str != '\0') {
		str = scroll_skip_str;
	} else {
		str = scroll_skip_str0;
	}
	scroll_skip_all = create_str_list(str);

	nkey = 0;
	nmouse = 0;
	n = 0;
	while (scroll_skip_all[n] != NULL) {
		char *s = scroll_skip_all[n++];
		if (strstr(s, "KEY:") == s) nkey++;
		if (strstr(s, "MOUSE:") == s) nmouse++;
	}
	if (nkey++) {
		scroll_skip_key = (char **) malloc(nkey*sizeof(char *));
		for (i=0; i<nkey; i++) scroll_skip_key[i] = NULL;
	}
	if (nmouse++) {
		scroll_skip_mouse = (char **) malloc(nmouse*sizeof(char *));
		for (i=0; i<nmouse; i++) scroll_skip_mouse[i] = NULL;
	}
	nkey = 0;
	nmouse = 0;
	for (i=0; i<n; i++) {
		char *s = scroll_skip_all[i];
		if (strstr(s, "KEY:") == s) {
			scroll_skip_key[nkey++] = strdup(s+strlen("KEY:"));
			free(s);
			scroll_skip_all[i] = strdup(imp);
		} else if (strstr(s, "MOUSE:") == s) {
			scroll_skip_mouse[nmouse++]=strdup(s+strlen("MOUSE:"));
			free(s);
			scroll_skip_all[i] = strdup(imp);
		}
	}
}

void initialize_scroll_term(void) {
	char *str;
	int n;

	destroy_str_list(scroll_term);
	scroll_term = NULL;

	if (scroll_term_str != NULL && *scroll_term_str != '\0') {
		str = scroll_term_str;
	} else {
		str = scroll_term_str0;
	}
	if (!strcmp(str, "none")) {
		return;
	}
	scroll_term = create_str_list(str);

	n = 0;
	while (scroll_term[n] != NULL) {
		char *s = scroll_good_all[n++];
		/* pull parameters out at some point */
		s = NULL;
	}
}

void initialize_max_keyrepeat(void) {
	char *str;
	int lo, hi;

	if (max_keyrepeat_str != NULL && *max_keyrepeat_str != '\0') {
		str = max_keyrepeat_str;
	} else {
		str = max_keyrepeat_str0;
	}

	if (sscanf(str, "%d-%d", &lo, &hi) != 2) {
		rfbLog("skipping invalid -scr_keyrepeat string: %s\n", str);
		sscanf(max_keyrepeat_str0, "%d-%d", &lo, &hi);
	}
	max_keyrepeat_lo = lo;
	max_keyrepeat_hi = hi;
	if (max_keyrepeat_lo < 1) {
		max_keyrepeat_lo = 1;
	}
	if (max_keyrepeat_hi > 40) {
		max_keyrepeat_hi = 40;
	}
}

typedef struct saveline {
	int x0, y0, x1, y1;
	int shift;
	int vert;
	int saved;
	char *data;
} saveline_t;

/*
 * Draw the wireframe box onto the framebuffer.  Saves the real
 * framebuffer data to some storage lines.  Restores previous lines.
 * use restore = 1 to clean up (done with animation).
 * This works with -scale.
 */
static void draw_box(int x, int y, int w, int h, int restore) {
	int x0, y0, x1, y1, i, pixelsize = bpp/8;
	char *dst, *src, *use_fb;
	static saveline_t *save[4];
	static int first = 1, len = 0;
	int max = dpy_x > dpy_y ? dpy_x : dpy_y;
	int use_Bpl, lw = wireframe_lw;
	unsigned long shade = wireframe_shade;
	int color = 0;
	unsigned short us = 0;
	unsigned long ul = 0;

	if (clipshift) {
		x -= coff_x;
		y -= coff_y;
	}

	/* handle -8to24 mode: use 2nd fb only */
	use_fb  = main_fb;
	use_Bpl = main_bytes_per_line; 
	
	if (cmap8to24 && cmap8to24_fb) {
		use_fb = cmap8to24_fb;
		pixelsize = 4;
		if (depth <= 8) {
			use_Bpl *= 4;
		} else if (depth <= 16) {
			use_Bpl *= 2;
		}
	}

	if (max > len) {
		/* create/resize storage lines: */
		for (i=0; i<4; i++) {
			len = max;
			if (! first && save[i]) {
				if (save[i]->data) {
					free(save[i]->data);
					save[i]->data = NULL;
				}
				free(save[i]);
			}
			save[i] = (saveline_t *) malloc(sizeof(saveline_t));
			save[i]->saved = 0;
			save[i]->data = (char *) malloc( (LW_MAX+1)*len*4 );

			/* 
			 * Four types of lines:
			 *	0) top horizontal
			 *	1) bottom horizontal
			 *	2) left vertical
			 *	3) right vertical
			 *
			 * shift means shifted by width or height.
			 */
			if (i == 0) {
				save[i]->vert  = 0;
				save[i]->shift = 0;
			} else if (i == 1) {
				save[i]->vert  = 0;
				save[i]->shift = 1;
			} else if (i == 2) {
				save[i]->vert  = 1;
				save[i]->shift = 0;
			} else if (i == 3) {
				save[i]->vert  = 1;
				save[i]->shift = 1;
			}
		}
	}
	first = 0;

	/*
	 * restore any saved lines. see below for algorithm and
	 * how x0, etc. are used.  we just reverse those steps.
	 */
	for (i=0; i<4; i++) {
		int s = save[i]->shift;
		int yu, y_min = -1, y_max = -1;
		int y_start, y_stop, y_step;

		if (! save[i]->saved) {
			continue;
		}
		x0 = save[i]->x0;
		y0 = save[i]->y0;
		x1 = save[i]->x1;
		y1 = save[i]->y1;
		if (save[i]->vert) {
			y_start = y0+lw;
			y_stop  = y1-lw;
			y_step  = lw*pixelsize;
		} else {
			y_start = y0 - s*lw;
			y_stop  = y_start + lw;
			y_step  = max*pixelsize;
		}
		for (yu = y_start; yu < y_stop; yu++) {
			if (x0 == x1) {
				continue;
			}
			if (yu < 0 || yu >= dpy_y) {
				continue;
			}
			if (y_min < 0 || yu < y_min) {
				y_min = yu;
			}
			if (y_max < 0 || yu > y_max) {
				y_max = yu;
			}
			src = save[i]->data + (yu-y_start)*y_step;
			dst = use_fb + yu*use_Bpl + x0*pixelsize;
			memcpy(dst, src, (x1-x0)*pixelsize);
		}
		if (y_min >= 0) {
if (0) fprintf(stderr, "Mark-1 %d %d %d %d\n", x0, y_min, x1, y_max+1);
			mark_rect_as_modified(x0, y_min, x1, y_max+1, 0);
		}
		save[i]->saved = 0;
	}

if (0) fprintf(stderr, "  DrawBox: %04dx%04d+%04d+%04d B=%d rest=%d lw=%d %.4f\n", w, h, x, y, 2*(w+h)*(2-restore)*pixelsize*lw, restore, lw, dnowx());

	if (restore) {
		return;
	}


	/*
	 * work out shade/color for the wireframe line, could be a color
	 * for 16bpp or 24bpp.
	 */
	if (shade > 255) {
		if (pixelsize == 2) {
			us = (unsigned short) (shade & 0xffff);
			color = 1;
		} else if (pixelsize == 4) {
			ul = (unsigned long) shade;
			color = 1;
		} else {
			shade = shade % 256;
		}
	}

	for (i=0; i<4; i++)  {
		int s = save[i]->shift;
		int yu, y_min = -1, y_max = -1;
		int yblack = -1, xblack1 = -1, xblack2 = -1;
		int y_start, y_stop, y_step;

		if (save[i]->vert) {
			/*
			 * make the narrow x's be on the screen, let
			 * the y's hang off (not drawn).
			 */
			save[i]->x0 = x0 = nfix(x + s*w - s*lw, dpy_x);
			save[i]->y0 = y0 = y;
			save[i]->x1 = x1 = nfix(x + s*w - s*lw + lw, dpy_x);
			save[i]->y1 = y1 = y + h;

			/*
			 * start and stop a linewidth away from true edge,
			 * to avoid interfering with horizontal lines.
			 */
			y_start = y0+lw;
			y_stop  = y1-lw;
			y_step  = lw*pixelsize;

			/* draw a black pixel for the border if lw > 1 */
			if (s) {
				xblack1 = x1-1;
			} else {
				xblack1 = x0;
			}
		} else {
			/*
			 * make the wide x's be on the screen, let the y's
			 * hang off (not drawn).
			 */
			save[i]->x0 = x0 = nfix(x,     dpy_x);
			save[i]->y0 = y0 = y + s*h;
			save[i]->x1 = x1 = nfix(x + w, dpy_x);
			save[i]->y1 = y1 = y0 + lw;
			y_start = y0 - s*lw;
			y_stop  = y_start + lw;
			y_step  = max*pixelsize;

			/* draw a black pixels for the border if lw > 1 */
			if (s) {
				yblack = y_stop - 1;
			} else {
				yblack = y_start;
			}
			xblack1 = x0;
			xblack2 = x1-1;
		}

		/* now loop over the allowed y's for either case */
		for (yu = y_start; yu < y_stop; yu++) {
			if (x0 == x1) {
				continue;
			}
			if (yu < 0 || yu >= dpy_y) {
				continue;
			}

			/* record min and max y's for marking rectangle: */
			if (y_min < 0 || yu < y_min) {
				y_min = yu;
			}
			if (y_max < 0 || yu > y_max) {
				y_max = yu;
			}

			/* save fb data for this line: */
			save[i]->saved = 1;
			src = use_fb + yu*use_Bpl + x0*pixelsize;
			dst = save[i]->data + (yu-y_start)*y_step;
			memcpy(dst, src, (x1-x0)*pixelsize);

			/* apply the shade/color to make the wireframe line: */
			if (! color) {
				memset(src, shade, (x1-x0)*pixelsize);
			} else {
				char *csrc = src;
				unsigned short *usp;
				unsigned long *ulp;
				int k;
				for (k=0; k < x1 - x0; k++) {
					if (pixelsize == 4) {
						ulp = (unsigned long *)csrc;
						*ulp = ul;
					} else if (pixelsize == 2) {
						usp = (unsigned short *)csrc;
						*usp = us;
					}
					csrc += pixelsize;
				}
			}

			/* apply black border for lw >= 2 */
			if (lw > 1) {
				if (yu == yblack) {
					memset(src, 0, (x1-x0)*pixelsize);
				}
				if (xblack1 >= 0) {
					src = src + (xblack1 - x0)*pixelsize;
					memset(src, 0, pixelsize);
				}
				if (xblack2 >= 0) {
					src = src + (xblack2 - x0)*pixelsize;
					memset(src, 0, pixelsize);
				}
			}
		}
		/* mark it for sending: */
		if (save[i]->saved) {
if (0) fprintf(stderr, "Mark-2 %d %d %d %d\n", x0, y_min, x1, y_max+1);
			mark_rect_as_modified(x0, y_min, x1, y_max+1, 0);
		}
	}
}

int direct_fb_copy(int x1, int y1, int x2, int y2, int mark) {
	char *src, *dst;
	int y, pixelsize = bpp/8;
	int xmin = -1, xmax = -1, ymin = -1, ymax = -1;
	int do_cmp = 2;
	double tm;
	int db = 0;

if (db) dtime0(&tm);

	x1 = nfix(x1, dpy_x);
	y1 = nfix(y1, dpy_y);
	x2 = nfix(x2, dpy_x+1);
	y2 = nfix(y2, dpy_y+1);

	if (x1 == x2) {
		return 1;
	}
	if (y1 == y2) {
		return 1;
	}

	X_LOCK;
	for (y = y1; y < y2; y++) {
		XRANDR_SET_TRAP_RET(0, "direct_fb_copy-set");
		copy_image(scanline, x1, y, x2 - x1, 1);
		XRANDR_CHK_TRAP_RET(0, "direct_fb_copy-chk");
		
		src = scanline->data;
		dst = main_fb + y * main_bytes_per_line + x1 * pixelsize;

		if (do_cmp == 0 || !mark) {
			memcpy(dst, src, (x2 - x1)*pixelsize);

		} else if (do_cmp == 1) {
			if (memcmp(dst, src, (x2 - x1)*pixelsize)) {
				if (ymin == -1 || y < ymin) {
					ymin = y;
				}
				if (ymax == -1 || y > ymax) {
					ymax = y;
				}
				memcpy(dst, src, (x2 - x1)*pixelsize);
			}

		} else if (do_cmp == 2) {
			int n, shift, xlo, xhi, k, block = 32;
			char *dst2, *src2;

			for (k=0; k*block < (x2 - x1); k++) {
				shift = k*block;
				xlo = x1  + shift;
				xhi = xlo + block;
				if (xhi > x2) {
					xhi = x2;
				}
				n = xhi - xlo;
				if (n < 1) {
					continue;
				}
				src2 = src + shift*pixelsize;
				dst2 = dst + shift*pixelsize;
				if (memcmp(dst2, src2, n*pixelsize)) {
					if (ymin == -1 || y < ymin) {
						ymin = y;
					}
					if (ymax == -1 || y > ymax) {
						ymax = y;
					}
					if (xmin == -1 || xlo < xmin) {
						xmin = xlo;
					}
					if (xmax == -1 || xhi > xmax) {
						xmax = xhi;
					}
					memcpy(dst2, src2, n*pixelsize);
				}
			}
		}
	}
	X_UNLOCK;

	if (do_cmp == 0) {
		xmin = x1;
		ymin = y1;
		xmax = x2;
		ymax = y2;
	} else if (do_cmp == 1) {
		xmin = x1;
		xmax = x2;
	}

	if (xmin < 0 || ymin < 0 || xmax < 0 || xmin < 0) {
		/* no diffs */
		return 1;
	}

	if (xmax < x2) {
		xmax++;
	}
	if (ymax < y2) {
		ymax++;
	}

	if (mark) {
		mark_rect_as_modified(xmin, ymin, xmax, ymax, 0);
	}

 if (db) {
	fprintf(stderr, "direct_fb_copy: %dx%d+%d+%d - %d  %.4f\n",
		x2 - x1, y2 - y1, x1, y1, mark, dtime(&tm));
 }

	return 1;
}

static int do_bdpush(Window wm_win, int x0, int y0, int w0, int h0, int bdx,
    int bdy, int bdskinny) {

	XWindowAttributes attr;
	sraRectangleIterator *iter;
	sraRect rect;
	sraRegionPtr frame, whole, tmpregion;
	int tx1, ty1, tx2, ty2;
	static Window last_wm_win = None;
	static int last_x, last_y, last_w, last_h;
	int do_fb_push = 0;
	int db = debug_scroll;

	if (wm_win == last_wm_win) {
		attr.x = last_x;
		attr.y = last_y;
		attr.width = last_w;
		attr.height = last_h;
	} else {
		if (!valid_window(wm_win, &attr, 1)) {
			return do_fb_push;
		}
		last_wm_win = wm_win;
		last_x = attr.x;
		last_y = attr.y;
		last_w = attr.width;
		last_h = attr.height;
	}
if (db > 1) fprintf(stderr, "BDP  %d %d %d %d  %d %d %d  %d %d %d %d\n",
	x0, y0, w0, h0, bdx, bdy, bdskinny, last_x, last_y, last_w, last_h);

	/* wm frame: */
	tx1 = attr.x;
	ty1 = attr.y;
	tx2 = attr.x + attr.width;
	ty2 = attr.y + attr.height;

	whole = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	if (clipshift) {
		sraRgnOffset(whole, coff_x, coff_y);
	}
	if (subwin) {
		sraRgnOffset(whole, off_x, off_y);
	}
	frame = sraRgnCreateRect(tx1, ty1, tx2, ty2);
	sraRgnAnd(frame, whole);

	/* scrolling window: */
	tmpregion = sraRgnCreateRect(x0, y0, x0 + w0, y0 + h0);
	sraRgnAnd(tmpregion, whole);

	sraRgnSubtract(frame, tmpregion);
	sraRgnDestroy(tmpregion);

	if (!sraRgnEmpty(frame)) {
		double dt = 0.0, dm;
		dtime0(&dm);
		iter = sraRgnGetIterator(frame);
		while (sraRgnIteratorNext(iter, &rect)) {
			tx1 = rect.x1;
			ty1 = rect.y1;
			tx2 = rect.x2;
			ty2 = rect.y2;

			if (bdskinny > 0) {
				int ok = 0;
				if (nabs(ty2-ty1) <= bdskinny) {
					ok = 1;
				}
				if (nabs(tx2-tx1) <= bdskinny) {
					ok = 1;
				}
				if (! ok) {
					continue;
				}
			}

			if (bdx >= 0) {
				if (bdx < tx1 || tx2 <= bdx) {
					continue;
				}
			}
			if (bdy >= 0) {
				if (bdy < ty1 || ty2 <= bdy) {
					continue;
				}
			}
			if (clipshift) {
				tx1 -= coff_x;
				ty1 -= coff_y;
				tx2 -= coff_x;
				ty2 -= coff_y;
			}
			if (subwin) {
				tx1 -= off_x;
				ty1 -= off_y;
				tx2 -= off_x;
				ty2 -= off_y;
			}

			direct_fb_copy(tx1, ty1, tx2, ty2, 1);

			do_fb_push++;
			dt += dtime(&dm);
if (db > 1) fprintf(stderr, "  BDP(%d,%d-%d,%d)  dt: %.4f\n", tx1, ty1, tx2, ty2, dt);
		}
		sraRgnReleaseIterator(iter);
	}
	sraRgnDestroy(whole);
	sraRgnDestroy(frame);

	return do_fb_push;
}

static int set_ypad(void) {
	int ev, ev_tot = scr_ev_cnt;
	static Window last_win = None;
	static double last_time = 0.0;
	static int y_accum = 0, last_sign = 0;
	double now, cut = 0.1;
	int dy_sum = 0, ys = 0, sign;
	int font_size = 15;
	int win_y, scr_y, loc_cut = 4*font_size, y_cut = 10*font_size;
	
	if (!xrecord_set_by_keys || !xrecord_name_info) {
		return 0;
	}
	if (xrecord_name_info[0] == '\0') {
		return 0;
	}
	if (! ev_tot) {
		return 0;
	}
	if (xrecord_keysym == NoSymbol)  {
		return 0;
	}
	if (!xrecord_scroll_keysym(xrecord_keysym)) {
		return 0;
	}
	if (!scroll_term) {
		return 0;
	}
	if (!match_str_list(xrecord_name_info, scroll_term)) {
		return 0;
	}

	for (ev=0; ev < ev_tot; ev++) {
		dy_sum += nabs(scr_ev[ev].dy);
		if (scr_ev[ev].dy < 0) {
			ys--;
		} else if (scr_ev[ev].dy > 0) {
			ys++;
		} else {
			ys = 0;
			break;
		}
		if (scr_ev[ev].win != scr_ev[0].win) {
			ys = 0;
			break;
		}
		if (scr_ev[ev].dx != 0) {
			ys = 0;
			break;
		}
	}
	if (ys != ev_tot && ys != -ev_tot) {
		return 0;
	}
	if (ys < 0) {
		sign = -1;
	} else {
		sign = 1;
	}

	if (sign > 0) {
		/*
		 * this case is not as useful as scrolling near the
		 * bottom of a terminal.  But there are problems for it too.
		 */
		return 0;
	}

	win_y = scr_ev[0].win_y + scr_ev[0].win_h;
	scr_y = scr_ev[0].y + scr_ev[0].h;
	if (nabs(scr_y - win_y) > loc_cut) {
		/* require it to be near the bottom. */
		return 0;
	}

	now = dnow();

	if (now < last_time + cut) {
		int ok = 1;
		if (last_win && scr_ev[0].win != last_win) {
			ok = 0;
		}
		if (last_sign && sign != last_sign) {
			ok = 0;
		}
		if (! ok) {
			last_win = None;
			last_sign = 0;
			y_accum = 0;
			last_time = 0.0;
			return 0;
		}
	} else {
		last_win = None;
		last_sign = 0;
		last_time = 0.0;
		y_accum = 0;
	}

	y_accum += sign * dy_sum;

	if (4 * nabs(y_accum) > scr_ev[0].h && y_cut) {
		;	/* TBD */
	}

	last_sign = sign;
	last_win = scr_ev[0].win;
	last_time = now;

	return y_accum;
}

static void scale_mark(int x1, int y1, int x2, int y2, int mark) {
	int s = 2;
	x1 = nfix(x1 - s, dpy_x);
	y1 = nfix(y1 - s, dpy_y);
	x2 = nfix(x2 + s, dpy_x+1);
	y2 = nfix(y2 + s, dpy_y+1);
	scale_and_mark_rect(x1, y1, x2, y2, mark);
}

#define PUSH_TEST(n)  \
if (n) { \
	double dt = 0.0, tm; dtime0(&tm); \
	fprintf(stderr, "PUSH---\n"); \
	while (dt < 2.0) { rfbPE(50000); dt += dtime(&tm); } \
	fprintf(stderr, "---PUSH\n"); \
}

int batch_dxs[], batch_dys[];
sraRegionPtr batch_reg[];
void batch_push(int ncr, double delay);

static int push_scr_ev(double *age, int type, int bdpush, int bdx, int bdy,
    int bdskinny, int first_push) {
	Window frame, win, win0;
	int x, y, w, h, wx, wy, ww, wh, dx, dy;
	int x0, y0, w0, h0;
	int nx, ny, nw, nh;
	int dret = 1, do_fb_push = 0, obscured;
	int ev, ev_tot = scr_ev_cnt;
	double tm, dt, st, waittime = 0.125;
	double max_age = *age;
	int db = debug_scroll, rrate = get_read_rate();
	sraRegionPtr backfill, whole, tmpregion, tmpregion2;
	int link, latency, netrate;
	int ypad = 0;
	double last_scroll_event_save = last_scroll_event;
	int fast_push = 0, rc;

	/* we return the oldest one. */
	*age = 0.0;

	if (ev_tot == 0) {
		return dret;
	}

	link = link_rate(&latency, &netrate);

	if (link == LR_DIALUP) {
		waittime *= 5;
	} else if (link == LR_BROADBAND) {
		waittime *= 3;
	} else if (latency > 80 || netrate < 40) {
		waittime *= 3;
	}

	backfill = sraRgnCreate();
	whole = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	if (clipshift) {
		sraRgnOffset(whole, coff_x, coff_y);
	}
	if (subwin) {
		sraRgnOffset(whole, off_x, off_y);
	}

	win0 = scr_ev[0].win;
	x0 = scr_ev[0].win_x;
	y0 = scr_ev[0].win_y;
	w0 = scr_ev[0].win_w;
	h0 = scr_ev[0].win_h;

	ypad = set_ypad();

if (db) fprintf(stderr, "ypad: %d  dy[0]: %d ev_tot: %d\n", ypad, scr_ev[0].dy, ev_tot);

	for (ev=0; ev < ev_tot; ev++) {
		double ag;
	
		x   = scr_ev[ev].x;
		y   = scr_ev[ev].y;
		w   = scr_ev[ev].w;
		h   = scr_ev[ev].h;
		dx  = scr_ev[ev].dx;
		dy  = scr_ev[ev].dy;
		win = scr_ev[ev].win;
		wx  = scr_ev[ev].win_x;
		wy  = scr_ev[ev].win_y;
		ww  = scr_ev[ev].win_w;
		wh  = scr_ev[ev].win_h;
		nx  = scr_ev[ev].new_x;
		ny  = scr_ev[ev].new_y;
		nw  = scr_ev[ev].new_w;
		nh  = scr_ev[ev].new_h;
		st  = scr_ev[ev].t;

		ag = (dnow() - servertime_diff) - st;
		if (ag > *age) {
			*age = ag;
		}

		if (dabs(ag) > max_age) {
if (db) fprintf(stderr, "push_scr_ev: TOO OLD: %.4f :: (%.4f - %.4f) "
    "- %.4f \n", ag, dnow(), servertime_diff, st);				
			dret = 0;
			break;
		} else {
if (db) fprintf(stderr, "push_scr_ev: AGE:     %.4f\n", ag);
		}
		if (win != win0) {
if (db) fprintf(stderr, "push_scr_ev: DIFF WIN: 0x%lx != 0x%lx\n", win, win0);
			dret = 0;
			break;
		}
		if (wx != x0 || wy != y0) {
if (db) fprintf(stderr, "push_scr_ev: WIN SHIFT: %d %d, %d %d", wx, x0, wy, y0);
			dret = 0;
			break;
		}
		if (ww != w0 || wh != h0) {
if (db) fprintf(stderr, "push_scr_ev: WIN RESIZE: %d %d, %d %d", ww, w0, wh, h0);
			dret = 0;
			break;
		}
		if (w < 1 || h < 1 || ww < 1 || wh < 1) {
if (db) fprintf(stderr, "push_scr_ev: NEGATIVE h/w: %d %d %d %d\n", w, h, ww, wh);
			dret = 0;
			break;
		}

if (db > 1) fprintf(stderr, "push_scr_ev: got: %d x: %4d y: %3d"
    " w: %4d h: %3d  dx: %d dy: %d %dx%d+%d+%d   win: 0x%lx\n",
    ev, x, y, w, h, dx, dy, w, h, x, y, win);

if (db > 1) fprintf(stderr, "------------ got: %d x: %4d y: %3d"
    " w: %4d h: %3d %dx%d+%d+%d\n",
    ev, wx, wy, ww, wh, ww, wh, wx, wy);

if (db > 1) fprintf(stderr, "------------ got: %d x: %4d y: %3d"
    " w: %4d h: %3d %dx%d+%d+%d\n",
    ev, nx, ny, nw, nh, nw, nh, nx, ny);

		frame = None;
		if (xrecord_wm_window) {
			frame = xrecord_wm_window;
		}
		if (! frame) {
			X_LOCK;
			frame = query_pointer(rootwin);
			X_UNLOCK;
		}
		if (! frame) {
			frame = win;
		}

		dtime0(&tm);

		tmpregion = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
		if (clipshift) {
			sraRgnOffset(tmpregion, coff_x, coff_y);
		}
		if (subwin) {
			sraRgnOffset(tmpregion, off_x, off_y);
		}
		tmpregion2 = sraRgnCreateRect(wx, wy, wx+ww, wy+wh);
		sraRgnAnd(tmpregion2, whole);
		sraRgnSubtract(tmpregion, tmpregion2);
		sraRgnDestroy(tmpregion2);

		/* do the wm frame just incase the above is bogus too. */
		if (frame && frame != win) {
			int k, gotk = -1;
			for (k = stack_list_num - 1; k >= 0; k--) {
				if (stack_list[k].win == frame &&
				    stack_list[k].fetched && 
				    stack_list[k].valid && 
				    stack_list[k].map_state == IsViewable) {
					gotk = k;
					break;
				}
			}
			if (gotk != -1) {
				int tx1, ty1, tx2, ty2;
				tx1 = stack_list[gotk].x;
				ty1 = stack_list[gotk].y;
				tx2 = tx1 + stack_list[gotk].width;
				ty2 = ty1 + stack_list[gotk].height;
				tmpregion2 = sraRgnCreateRect(tx1,ty1,tx2,ty2);
				sraRgnAnd(tmpregion2, whole);
				sraRgnSubtract(tmpregion, tmpregion2);
				sraRgnDestroy(tmpregion2);
			}
		}

		/*
		 * XXX Need to also clip:
		 *	children of win
		 *	siblings of win higher in stacking order.
		 * ignore for now... probably will make some apps
		 * act very strangely.
		 */
		if (ypad) {
			if (ypad < 0) {
				if (h > -ypad) {
					h += ypad;
				} else {
					ypad = 0;
				}
			} else {
				if (h > ypad) {
					y += ypad;
				} else {
					ypad = 0;
				}
			}
		}

		if (fast_push) {
			int nbatch = 0; 
			double delay, d1 = 0.1, d2 = 0.02;
			rc = try_copyrect(frame, frame, x, y, w, h, dx, dy, &obscured,
			    tmpregion, waittime, &nbatch);

			if (first_push) {
				delay = d1;
			} else {
				delay = d2;
			}

			batch_push(nbatch, delay);
			fb_push();
		} else {
			rc = try_copyrect(frame, frame, x, y, w, h, dx, dy, &obscured,
			    tmpregion, waittime, NULL);
			if (rc) {
				last_scroll_type = type;
				dtime0(&last_scroll_event);

				do_fb_push++;
				urgent_update = 1;
				sraRgnDestroy(tmpregion);
PUSH_TEST(0);
			}
		}

		if (! rc) {
			dret = 0;
			sraRgnDestroy(tmpregion);
			break;	
		}
		dt = dtime(&tm);
if (0) fprintf(stderr, "  try_copyrect dt: %.4f\n", dt);

		if (ev > 0) {
			sraRgnOffset(backfill, dx, dy);
			sraRgnAnd(backfill, whole);
		}

		if (ypad) {
			if (ypad < 0) {
				ny += ypad;	
				nh -= ypad;
			} else {
				;
			}
		}

		tmpregion = sraRgnCreateRect(nx, ny, nx + nw, ny + nh);
		sraRgnAnd(tmpregion, whole);
		sraRgnOr(backfill, tmpregion);
		sraRgnDestroy(tmpregion);
	}

	/* try to update the backfill region (new window contents) */
	if (dret != 0) {
		double est, win_area = 0.0, area = 0.0;
		sraRectangleIterator *iter;
		sraRect rect;
		int tx1, ty1, tx2, ty2;

		tmpregion = sraRgnCreateRect(x0, y0, x0 + w0, y0 + h0);
		sraRgnAnd(tmpregion, whole);

		sraRgnAnd(backfill, tmpregion);

		iter = sraRgnGetIterator(tmpregion);
		while (sraRgnIteratorNext(iter, &rect)) {
			tx1 = rect.x1;
			ty1 = rect.y1;
			tx2 = rect.x2;
			ty2 = rect.y2;

			win_area += (tx2 - tx1)*(ty2 - ty1);
		}
		sraRgnReleaseIterator(iter);

		sraRgnDestroy(tmpregion);


		iter = sraRgnGetIterator(backfill);
		while (sraRgnIteratorNext(iter, &rect)) {
			tx1 = rect.x1;
			ty1 = rect.y1;
			tx2 = rect.x2;
			ty2 = rect.y2;

			area += (tx2 - tx1)*(ty2 - ty1);
		}
		sraRgnReleaseIterator(iter);

		est = (area * (bpp/8)) / (1000000.0 * rrate);
if (db) fprintf(stderr, "  area %.1f win_area %.1f est: %.4f", area, win_area, est);
		if (area > 0.90 * win_area) {
if (db) fprintf(stderr, "  AREA_TOO_MUCH");
			dret = 0;
		} else if (est > 0.6) {
if (db) fprintf(stderr, "  EST_TOO_LARGE");
			dret = 0;
		} else if (area <= 0.0) {
			;
		} else {
			dtime0(&tm);
			iter = sraRgnGetIterator(backfill);
			while (sraRgnIteratorNext(iter, &rect)) {
				tx1 = rect.x1;
				ty1 = rect.y1;
				tx2 = rect.x2;
				ty2 = rect.y2;

				if (clipshift) {
					tx1 -= coff_x;
					ty1 -= coff_y;
					tx2 -= coff_x;
					ty2 -= coff_y;
				}
				if (subwin) {
					tx1 -= off_x;
					ty1 -= off_y;
					tx2 -= off_x;
					ty2 -= off_y;
				}
				tx1 = nfix(tx1, dpy_x);
				ty1 = nfix(ty1, dpy_y);
				tx2 = nfix(tx2, dpy_x+1);
				ty2 = nfix(ty2, dpy_y+1);

				dtime(&tm);
if (db) fprintf(stderr, "  DFC(%d,%d-%d,%d)", tx1, ty1, tx2, ty2);
				direct_fb_copy(tx1, ty1, tx2, ty2, 1);
				if (fast_push) {
					fb_push();
				}
				do_fb_push++;
PUSH_TEST(0);
			}
			sraRgnReleaseIterator(iter);

			dt = dtime(&tm);
if (db) fprintf(stderr, "  dfc---- dt: %.4f", dt);

		}
if (db &&  dret) fprintf(stderr, " **** dret=%d", dret);
if (db && !dret) fprintf(stderr, " ---- dret=%d", dret);
if (db) fprintf(stderr, "\n");
	}

if (db && bdpush) fprintf(stderr, "BDPUSH-TIME:  0x%lx\n", xrecord_wm_window);

	if (bdpush && xrecord_wm_window != None) {
		int x, y, w, h;
		x = scr_ev[0].x;
		y = scr_ev[0].y;
		w = scr_ev[0].w;
		h = scr_ev[0].h;
		do_fb_push += do_bdpush(xrecord_wm_window, x, y, w, h,
		    bdx, bdy, bdskinny); 
		if (fast_push) {
			fb_push();
		}
	}

	if (do_fb_push) {
		dtime0(&tm);
		fb_push();
		dt = dtime(&tm);
if (0) fprintf(stderr, "  fb_push dt: %.4f", dt);
		if (scaling) {
			static double last_time = 0.0;
			double now = dnow(), delay = 0.4, first_wait = 3.0;
			double trate;
			int repeating, lat, rate;
			int link = link_rate(&lat, &rate);
			int skip_first = 0;

			if (link == LR_DIALUP || rate < 35) {
				delay *= 4;
			} else if (link != LR_LAN || rate < 100) {
				delay *= 2;
			}

			trate = typing_rate(0.0, &repeating);
			
			if (xrecord_set_by_mouse || repeating >= 3) {
				if (now > last_scroll_event_save + first_wait) {
					skip_first = 1;
				}
			}

			if (skip_first) {
				/* 
				 * try not to send the first one, but a
				 * single keystroke scroll would be OK.
				 */
			} else if (now > last_time + delay) {

				scale_mark(x0, y0, x0 + w0, y0 + h0, 1);
				last_copyrect_fix = now;
			}
			last_time = now;
		}
	}

	sraRgnDestroy(backfill);
	sraRgnDestroy(whole);
	return dret;
}

static void get_client_regions(int *req, int *mod, int *cpy, int *num)  {
	
	rfbClientIteratorPtr i;
	rfbClientPtr cl;

	*req = 0;
	*mod = 0;
	*cpy = 0;
	*num = 0;

	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {
		if (use_threads) LOCK(cl->updateMutex);
		*req += sraRgnCountRects(cl->requestedRegion);
		*mod += sraRgnCountRects(cl->modifiedRegion);
		*cpy += sraRgnCountRects(cl->copyRegion);
		*num += 1;
		if (use_threads) UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(i);
}

/*
 * Wrapper to apply the rfbDoCopyRegion taking into account if scaling
 * is being done.  Note that copyrect under the scaling case is often
 * only approximate.
 */
int DCR_Normal = 0;
int DCR_FBOnly = 1;
int DCR_Direct = 2;

void do_copyregion(sraRegionPtr region, int dx, int dy, int mode)  {
	sraRectangleIterator *iter;
	sraRect rect;
	int Bpp0 = bpp/8, Bpp;
	int x1, y1, x2, y2, w, stride, stride0;
	int sx1, sy1, sx2, sy2, sdx, sdy;
	int req, mod, cpy, ncli;
	char *dst = NULL, *src = NULL;

	last_copyrect = dnow();

	if (rfb_fb == main_fb && ! rotating && mode == DCR_Normal) {
		/* normal case, no -scale or -8to24 */
		get_client_regions(&req, &mod, &cpy, &ncli);
if (0 || debug_scroll > 1) fprintf(stderr, ">>>-rfbDoCopyRect req: %d mod: %d cpy: %d\n", req, mod, cpy); 

		rfbDoCopyRegion(screen, region, dx, dy);

		get_client_regions(&req, &mod, &cpy, &ncli);
if (0 || debug_scroll > 1) fprintf(stderr, "<<<-rfbDoCopyRect req: %d mod: %d cpy: %d\n", req, mod, cpy); 

		return;
	}

	/* rarer case, we need to call rfbDoCopyRect with scaled xy */
	stride0 = dpy_x * Bpp0;

	iter = sraRgnGetReverseIterator(region, dx < 0, dy < 0);
	while(sraRgnIteratorNext(iter, &rect)) {
		int j, c, t;

		x1 = rect.x1;
		y1 = rect.y1;
		x2 = rect.x2;
		y2 = rect.y2;

		for (c= 0; c < 2; c++) {

			Bpp = Bpp0;
			stride = stride0;

			if (c == 0) {
				dst = main_fb + y1*stride + x1*Bpp;
				src = main_fb + (y1-dy)*stride + (x1-dx)*Bpp;

			} else if (c == 1) {
				if (!cmap8to24 || !cmap8to24_fb) {
					continue;
				}
				if (cmap8to24_fb == rfb_fb) {
					if (mode == DCR_FBOnly) {
						;
					} else if (mode == DCR_Direct) {
						;
					} else if (mode == DCR_Normal) {
						continue;
					}
				}
if (0) fprintf(stderr, "copyrect: cmap8to24_fb: mode=%d\n", mode);
				if (cmap8to24) {
					if (depth <= 8) {
						Bpp    = 4 * Bpp0;
						stride = 4 * stride0;
					} else if (depth <= 16) {
						Bpp    = 2 * Bpp0;
						stride = 2 * stride0;
					}
				}
				dst = cmap8to24_fb + y1*stride + x1*Bpp;
				src = cmap8to24_fb + (y1-dy)*stride + (x1-dx)*Bpp;
			}

			w = (x2 - x1)*Bpp; 
			
			if (dy < 0) {
				for (j=y1; j<y2; j++) {
					memmove(dst, src, w);
					dst += stride;
					src += stride;
				}
			} else {
				dst += (y2 - y1 - 1)*stride;
				src += (y2 - y1 - 1)*stride;
				for (j=y2-1; j>=y1; j--) {
					memmove(dst, src, w);
					dst -= stride;
					src -= stride;
				}
			}
		}

		if (mode == DCR_FBOnly) {
			continue;
		}


		if (scaling) {
			sx1 = ((double) x1 / dpy_x) * scaled_x;
			sy1 = ((double) y1 / dpy_y) * scaled_y;
			sx2 = ((double) x2 / dpy_x) * scaled_x;
			sy2 = ((double) y2 / dpy_y) * scaled_y;
			sdx = ((double) dx / dpy_x) * scaled_x;
			sdy = ((double) dy / dpy_y) * scaled_y;
		} else {
			sx1 = x1;
			sy1 = y1;
			sx2 = x2;
			sy2 = y2;
			sdx = dx;
			sdy = dy;
		}
if (0) fprintf(stderr, "sa.. %d %d %d %d %d %d\n", sx1, sy1, sx2, sy2, sdx, sdy);

		if (rotating) {
			rotate_coords(sx1, sy1, &sx1, &sy1, -1, -1);
			rotate_coords(sx2, sy2, &sx2, &sy2, -1, -1);
			if (rotating == ROTATE_X) {
				sdx = -sdx;
			} else if (rotating == ROTATE_Y) {
				sdy = -sdy;
			} else if (rotating == ROTATE_XY) {
				sdx = -sdx;
				sdy = -sdy;
			} else if (rotating == ROTATE_90) {
				t = sdx;
				sdx = -sdy;
				sdy = t;
			} else if (rotating == ROTATE_90X) {
				t = sdx;
				sdx = sdy;
				sdy = t;
			} else if (rotating == ROTATE_90Y) {
				t = sdx;
				sdx = -sdy;
				sdy = -t;
			} else if (rotating == ROTATE_270) {
				t = sdx;
				sdx = sdy;
				sdy = -t;
			}
		}

		/* XXX -1? */
		if (sx2 < 0) sx2 = 0;
		if (sy2 < 0) sy2 = 0;
		
		if (sx2 < sx1) {
			t = sx1;
			sx1 = sx2;
			sx2 = t;
		}
		if (sy2 < sy1) {
			t = sy1;
			sy1 = sy2;
			sy2 = t;
		}
if (0) fprintf(stderr, "sb.. %d %d %d %d %d %d\n", sx1, sy1, sx2, sy2, sdx, sdy);

		if (mode == DCR_Direct) {
			rfbClientIteratorPtr i;
			rfbClientPtr cl;
			sraRegionPtr r = sraRgnCreateRect(sx1, sy1, sx2, sy2);

			i = rfbGetClientIterator(screen);
			while( (cl = rfbClientIteratorNext(i)) ) {
				if (use_threads) LOCK(cl->updateMutex);
				rfbSendCopyRegion(cl, r, sdx, sdy);
				if (use_threads) UNLOCK(cl->updateMutex);
			}
			rfbReleaseClientIterator(i);
			sraRgnDestroy(r);
			
		} else {
			rfbDoCopyRect(screen, sx1, sy1, sx2, sy2, sdx, sdy);
		}
	}
	sraRgnReleaseIterator(iter);
}

void batch_copyregion(sraRegionPtr* region, int *dx, int *dy, int ncr, double delay)  {
	rfbClientIteratorPtr i;
	rfbClientPtr cl;
	int k, direct, mode, nrects = 0, bad = 0;
	double t1, t2, start = dnow();

	for (k=0; k < ncr; k++) {
		sraRectangleIterator *iter;
		sraRect rect;

		iter = sraRgnGetIterator(region[k]);
		while (sraRgnIteratorNext(iter, &rect)) {
			int x1 = rect.x1;
			int y1 = rect.y1;
			int x2 = rect.x2;
			int y2 = rect.y2;
			int ym = dpy_y * (ncache+1);
			int xm = dpy_x;
			if (x1 > xm || y1 > ym || x2 > xm || y2 > ym) {
				if (ncdb) fprintf(stderr, "batch_copyregion: BAD RECTANGLE: %d,%d %d,%d\n", x1, y1, x2, y2);
				bad = 1;
			}
			if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0) {
				if (ncdb) fprintf(stderr, "batch_copyregion: BAD RECTANGLE: %d,%d %d,%d\n", x1, y1, x2, y2);
				bad = 1;
			}
		}
		sraRgnReleaseIterator(iter);
		nrects += sraRgnCountRects(region[k]);
	}
	if (bad || nrects == 0) {
		return;
	}

	if (delay < 0.0) {
		delay = 0.1;
	}
	if (!fb_push_wait(delay, FB_COPY|FB_MOD)) {
		if (use_threads) usleep(100 * 1000);
		fb_push_wait(0.75, FB_COPY|FB_MOD);
	}

	t1 = dnow();

	bad = 0;
	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {

		if (use_threads) LOCK(cl->updateMutex);

		if (cl->ublen != 0) {
			fprintf(stderr, "batch_copyregion: *** BAD ublen != 0: %d\n", cl->ublen);
			bad++;
		}

		if (use_threads) UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(i);

	if (bad) {
		return;
	}

	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {
		rfbFramebufferUpdateMsg *fu;

		if (use_threads) LOCK(cl->updateMutex);

		fu = (rfbFramebufferUpdateMsg *)cl->updateBuf;
		fu->nRects = Swap16IfLE((uint16_t)(nrects));
		fu->type = rfbFramebufferUpdate;

		if (cl->ublen != 0) fprintf(stderr, "batch_copyregion: *** BAD-2 ublen != 0: %d\n", cl->ublen);

		cl->ublen = sz_rfbFramebufferUpdateMsg;

		if (use_threads) UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(i);

	if (rfb_fb == main_fb && !rotating) {
		direct = 0;
		mode = DCR_FBOnly;
	} else {
		direct = 1;
		mode = DCR_Direct;
	}
	for (k=0; k < ncr; k++) {
		do_copyregion(region[k], dx[k], dy[k], mode);
	}

	t2 = dnow();

	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {

		if (use_threads) LOCK(cl->updateMutex);

		if (!direct)  {
			for (k=0; k < ncr; k++) {
				rfbSendCopyRegion(cl, region[k], dx[k], dy[k]);
			}
		}
		rfbSendUpdateBuf(cl);

		if (use_threads) UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(i);

	last_copyrect = dnow();

if (0) fprintf(stderr, "batch_copyregion: nrects: %d nregions: %d  tot=%.4f t10=%.4f t21=%.4f t32=%.4f  %.4f\n",
    nrects, ncr, last_copyrect - start, t1 - start, t2 - t1, last_copyrect - t2, dnowx());

}

void batch_push(int nreg, double delay) {
	int k;
	batch_copyregion(batch_reg, batch_dxs, batch_dys, nreg, delay);
	/* XXX Y */
	fb_push();
	for (k=0; k < nreg; k++) {
		sraRgnDestroy(batch_reg[k]);
	}
}

void fb_push(void) {
	int req0, mod0, cpy0, req1, mod1, cpy1, ncli;
	int db = (debug_scroll || debug_wireframe);
	rfbClientIteratorPtr i;
	rfbClientPtr cl;

	if (use_threads) {
		return;
	}
	
if (db)	get_client_regions(&req0, &mod0, &cpy0, &ncli);

	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {
		if (use_threads) LOCK(cl->updateMutex);
		if (cl->sock >= 0 && !cl->onHold && FB_UPDATE_PENDING(cl) &&
		    !sraRgnEmpty(cl->requestedRegion)) {
			if (!rfbSendFramebufferUpdate(cl, cl->modifiedRegion)) {
				fprintf(stderr, "*** rfbSendFramebufferUpdate *FAILED* #1\n");
				if (cl->ublen) fprintf(stderr, "*** fb_push ublen not zero: %d\n", cl->ublen);
				if (use_threads) UNLOCK(cl->updateMutex);
				break;
			}
			if (cl->ublen) fprintf(stderr, "*** fb_push ublen NOT ZERO: %d\n", cl->ublen);
		}
		if (use_threads) UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(i);

if (db) {
	get_client_regions(&req1, &mod1, &cpy1, &ncli);
	fprintf(stderr, "\nFB_push: req: %d/%d  mod: %d/%d  cpy: %d/%d  %.4f\n",
	req0, req1, mod0, mod1, cpy0, cpy1, dnowx());
}

}

int fb_push_wait(double max_wait, int flags) {
	double tm, dt = 0.0;
	int req, mod, cpy, ncli;
	int ok = 0, first = 1;

	dtime0(&tm);	
	while (dt < max_wait) {
		int done = 1;
		fb_push();
		get_client_regions(&req, &mod, &cpy, &ncli);
		if (flags & FB_COPY && cpy) {
			done = 0;
		}
		if (flags & FB_MOD && mod) {
			done = 0;
		}
		if (flags & FB_REQ && req) {
			done = 0;
		}
		if (done) {
			ok = 1;
			break;
		}
		if (first) {
			first = 0;
			continue;	
		}

		rfbCFD(0);
		usleep(1000);
		dt += dtime(&tm);
	}
	return ok;
}

/* 
 * utility routine for CopyRect of the window (but not CopyRegion)
 */
static int crfix(int x, int dx, int Lx) {
	/* adjust x so that copy source is on screen */
	if (dx > 0) {
		if (x-dx < 0) {
			/* off on the left */
			x = dx;	
		}
	} else {
		if (x-dx >= Lx) {
			/* off on the right */
			x = Lx + dx - 1;
		}
	}
	return x;
}

typedef struct scroll_result {
	Window win;
	double time;
	int result;
} scroll_result_t;

#define SCR_RESULTS_MAX 256
static scroll_result_t scroll_results[SCR_RESULTS_MAX];

static int scrollability(Window win, int set) {
	double oldest = -1.0;
	int i, index = -1, next_index = -1;
	static int first = 1;

	if (first) {
		for (i=0; i<SCR_RESULTS_MAX; i++) {
			scroll_results[i].win = None;
			scroll_results[i].time = 0.0;
			scroll_results[i].result = 0;
		}
		first = 0;
	}

	if (win == None) {
		return 0;
	}
	if (set == SCR_NONE) {
		/* lookup case */
		for (i=0; i<SCR_RESULTS_MAX; i++) {
			if (win == scroll_results[i].win) {
				return scroll_results[i].result;
			}
			if (scroll_results[i].win == None) {
				break;
			}
		}
		return 0;
	}

	for (i=0; i<SCR_RESULTS_MAX; i++) {
		if (oldest == -1.0 || scroll_results[i].time < oldest) {
			next_index = i;
			oldest = scroll_results[i].time;
		}
		if (win == scroll_results[i].win) {
			index = i;
			break;
		}
		if (next_index >= 0 && scroll_results[i].win == None) {
			break;
		}
	}

	if (set == SCR_SUCCESS) {
		set = 1;
	} else if (set == SCR_FAIL) {
		set = -1;
	} else {
		set = 0;
	}
	if (index == -1) {
		scroll_results[next_index].win = win;
		scroll_results[next_index].time = dnow();
		scroll_results[next_index].result = set;
	} else {
		if (scroll_results[index].result == 1) {
			/*
			 * once a success, always a success, until they
			 * forget about us...
			 */
			set = 1;
		} else {
			scroll_results[index].result = set;
		}
		scroll_results[index].time = dnow();
	}

	return set;
}

void eat_viewonly_input(int max_eat, int keep) {
	int i, gp, gk;
	
	for (i=0; i<max_eat; i++) {
		int cont = 0;
		gp = got_pointer_calls;
		gk = got_keyboard_calls;
		rfbCFD(0);
		if (got_pointer_calls > gp)  {
			if (debug_pointer) {
				rfbLog("eat_viewonly_input: pointer: %d\n", i);
			}
			cont++;
		}
		if (got_keyboard_calls > gk)  {
			if (debug_keyboard) {
				rfbLog("eat_viewonly_input: keyboard: %d\n", i);
			}
			cont++;
		}
		if (i >= keep - 1 && ! cont) {
			break;
		}
	}
}

static int eat_pointer(int max_ptr_eat, int keep) {
	int i, count = 0,  gp = got_pointer_input;

	for (i=0; i<max_ptr_eat; i++) {
		rfbCFD(0);
		if (got_pointer_input > gp)  {
			count++;
if (0) fprintf(stderr, "GP*-%d\n", i);
			gp = got_pointer_input;
		} else if (i > keep) {
			break;
		}
	}
	return count;
}

static void set_bdpush(int type, double *last_bdpush, int *pushit) {
	double now, delay = 0.0;
	int link, latency, netrate;

	*pushit = 0;

	if (type == SCR_MOUSE) {
		delay = scr_mouse_bdpush_time;
	} else if (type == SCR_KEY) {
		delay = scr_key_bdpush_time;
	}

	link = link_rate(&latency, &netrate);
	if (link == LR_DIALUP) {
		delay *= 1.5;
	} else if (link == LR_BROADBAND) {
		delay *= 1.25;
	}

	dtime0(&now);
	if (delay > 0.0 && now > *last_bdpush + delay) {
		*pushit = 1;
		*last_bdpush = now;
	}
}

void mark_for_xdamage(int x, int y, int w, int h) {
	int tx1, ty1, tx2, ty2;
	sraRegionPtr tmpregion;

	if (! use_xdamage) {
		return;
	}

	tx1 = nfix(x, dpy_x);
	ty1 = nfix(y, dpy_y);
	tx2 = nfix(x + w, dpy_x+1);
	ty2 = nfix(y + h, dpy_y+1);

	tmpregion = sraRgnCreateRect(tx1, ty1, tx2, ty2);
	add_region_xdamage(tmpregion);
	sraRgnDestroy(tmpregion);
}

void mark_region_for_xdamage(sraRegionPtr region) {
	sraRectangleIterator *iter;
	sraRect rect;
	iter = sraRgnGetIterator(region);
	while (sraRgnIteratorNext(iter, &rect)) {
		int x1 = rect.x1;
		int y1 = rect.y1;
		int x2 = rect.x2;
		int y2 = rect.y2;
		mark_for_xdamage(x1, y1, x2 - x1, y2 - y1);
	}
	sraRgnReleaseIterator(iter);
}

void set_xdamage_mark(int x, int y, int w, int h) {
	sraRegionPtr region;

	if (! use_xdamage) {
		return;
	}
	mark_for_xdamage(x, y, w, h);

	if (xdamage_scheduled_mark == 0.0) {
		xdamage_scheduled_mark = dnow() + 2.0;
	}

	if (xdamage_scheduled_mark_region == NULL) {
		xdamage_scheduled_mark_region = sraRgnCreate();
	}
	region = sraRgnCreateRect(x, y, x + w, y + w);
	sraRgnOr(xdamage_scheduled_mark_region, region);
	sraRgnDestroy(region);
}

static int repeat_check(double last_key_scroll) {
	int repeating;
	double rate = typing_rate(0.0, &repeating);
	double now = dnow(), delay = 0.5;
	if (rate > 2.0 && repeating && now > last_key_scroll + delay) {
		return 0;
	} else {
		return 1;
	}
}

static int check_xrecord_keys(void) {
	static int last_wx, last_wy, last_ww, last_wh;
	double spin = 0.0, tm, tnow;
	int scr_cnt = 0, input = 0, scroll_rep;
	int get_out, got_one = 0, flush1 = 0, flush2 = 0;
	int gk, gk0, ret = 0, db = debug_scroll;
	int fail = 0;
	int link, latency, netrate;

	static double last_key_scroll = 0.0;
	static double persist_start = 0.0;
	static double last_bdpush = 0.0;
	static int persist_count = 0;
	int scroll_keysym = 0;
	double last_scroll, scroll_persist = scr_key_persist;
	double spin_fac = 1.0, scroll_fac = 2.0, noscroll_fac = 0.75;
	double max_spin, max_long_spin = 0.3;
	double set_repeat_in;
	static double set_repeat = 0.0;


	RAWFB_RET(0)

	if (unixpw_in_progress) return 0;

	set_repeat_in = set_repeat;
	set_repeat = 0.0;

	get_out = 1;
	if (got_keyboard_input) {
		get_out = 0;
	}

	dtime0(&tnow);
	if (tnow < last_key_scroll + scroll_persist) {
		get_out = 0;
	}

	if (set_repeat_in > 0.0 && tnow < last_key_scroll + set_repeat_in) {
		get_out = 0;
	}

	if (get_out) {
		persist_start = 0.0;
		persist_count = 0;
		last_bdpush = 0.0;
		if (xrecording) {
			xrecord_watch(0, SCR_KEY);
		}
		return 0;
	}

#if 0
	/* not used for keyboard yet */
	scroll_rep = scrollability(xrecord_ptr_window, SCR_NONE) + 1;
	if (scroll_rep == 1) {
		scroll_rep = 2;		/* if no info, assume the best. */
	}
#endif

	scroll_keysym = xrecord_scroll_keysym(last_rfb_keysym);

	max_spin = scr_key_time;

	if (set_repeat_in > 0.0 && tnow < last_key_scroll + 2*set_repeat_in) {
		max_spin = 2 * set_repeat_in;
	} else if (tnow < last_key_scroll + scroll_persist) {
		max_spin = 1.25*(tnow - last_key_scroll);
	} else if (tnow < last_key_to_button_remap_time + 1.5*scroll_persist) {
		/* mostly a hack I use for testing -remap key -> btn4/btn5 */
		max_spin = scroll_persist;
	} else if (scroll_keysym) {
		if (repeat_check(last_key_scroll)) {
			spin_fac = scroll_fac;
		} else {
			spin_fac = noscroll_fac;
		}
	}
	if (max_spin > max_long_spin) {
		max_spin = max_long_spin;
	}

	/* XXX use this somehow  */
if (0)	link = link_rate(&latency, &netrate);

	gk = gk0 = got_keyboard_input;
	dtime0(&tm);

if (db) fprintf(stderr, "check_xrecord_keys: BEGIN LOOP: scr_ev_cnt: "
    "%d max: %.3f  %.4f\n", scr_ev_cnt, max_spin, tm - x11vnc_start);

	while (1) {

		if (scr_ev_cnt) {
			got_one = 1;

			scrollability(xrecord_ptr_window, SCR_SUCCESS);
			scroll_rep = 2;

			dtime0(&last_scroll);
			last_key_scroll = last_scroll;
			scr_cnt++;
			break;
		}

		X_LOCK;
		flush1 = 1;
		XFlush_wr(dpy);
		X_UNLOCK;

		if (set_repeat_in > 0.0) {
			max_keyrepeat_time = set_repeat_in;
		}

		if (use_threads) {
			usleep(1000);
		} else {
			rfbCFD(1000);
		}
		spin += dtime(&tm);

		X_LOCK;
		if (got_keyboard_input > gk) {
			gk = got_keyboard_input;
			input++;
			if (set_repeat_in) {
				;
			} else if (xrecord_scroll_keysym(last_rfb_keysym)) {
				if (repeat_check(last_key_scroll)) {
					spin_fac = scroll_fac;
				} else {
					spin_fac = noscroll_fac;
				}
			}
if (0 || db) fprintf(stderr, "check_xrecord: more keys: %.3f  0x%x "
    " %.4f  %s  %s\n", spin, last_rfb_keysym, last_rfb_keytime - x11vnc_start,
    last_rfb_down ? "down":"up  ", last_rfb_key_accepted ? "accept":"skip");
			flush2 = 1;
			XFlush_wr(dpy);
		}
#if LIBVNCSERVER_HAVE_RECORD
		SCR_LOCK;
		XRecordProcessReplies(rdpy_data);
		SCR_UNLOCK;
#endif
		X_UNLOCK;

		if (spin >= max_spin * spin_fac) {
if (0 || db) fprintf(stderr, "check_xrecord: SPIN-OUT: %.3f/%.3f\n", spin,
    max_spin * spin_fac);
			fail = 1;
			break;
		}
	}

	max_keyrepeat_time = 0.0;

	if (scr_ev_cnt) {
		int dret, ev = scr_ev_cnt - 1;
		int bdx, bdy, bdskinny, bdpush = 0;
		double max_age = 0.25, age, tm, dt;
		static double last_scr_ev = 0.0;

		last_wx = scr_ev[ev].win_x;
		last_wy = scr_ev[ev].win_y;
		last_ww = scr_ev[ev].win_w;
		last_wh = scr_ev[ev].win_h;

		/* assume scrollbar on rhs: */
		bdx = last_wx + last_ww + 3;
		bdy = last_wy + last_wh/2;
		bdskinny = 32;
			
		if (persist_start == 0.0) {
			bdpush = 0;
		} else {
			set_bdpush(SCR_KEY, &last_bdpush, &bdpush);
		}

		dtime0(&tm);
		age = max_age;
		dret = push_scr_ev(&age, SCR_KEY, bdpush, bdx, bdy, bdskinny, 1);
		dt = dtime(&tm);

		ret = 1 + dret;
		scr_ev_cnt = 0;

		if (ret == 2 && xrecord_scroll_keysym(last_rfb_keysym)) {
			int repeating;
			double time_lo = 1.0/max_keyrepeat_lo;
			double time_hi = 1.0/max_keyrepeat_hi;
			double rate = typing_rate(0.0, &repeating);
if (0 || db) fprintf(stderr, "Typing: dt: %.4f rate: %.1f\n", dt, rate);
			if (repeating) {
				/* n.b. the "quantum" is about 1/30 sec. */
				max_keyrepeat_time = 1.0*dt;
				if (max_keyrepeat_time > time_lo ||
				    max_keyrepeat_time < time_hi) {
					max_keyrepeat_time = 0.0;
				} else {
					set_repeat = max_keyrepeat_time;
if (0 || db) fprintf(stderr, "set max_keyrepeat_time: %.2f\n", max_keyrepeat_time);
				}
			}
		}

		last_scr_ev = dnow();
	}

	if ((got_one && ret < 2) || persist_count) {
		set_xdamage_mark(last_wx, last_wy, last_ww, last_wh);
	}

	if (fail) {
		scrollability(xrecord_ptr_window, SCR_FAIL);
	}

	if (xrecording) {
		if (ret < 2) {
			xrecord_watch(0, SCR_KEY);
		}
	}

	if (ret == 2) {
		if (persist_start == 0.0) {
			dtime(&persist_start);
			last_bdpush = persist_start;
		}
	} else {
		persist_start = 0.0;
		last_bdpush = 0.0;
	}

	/* since we've flushed it, we might as well avoid -input_skip */
	if (flush1 || flush2) {
		got_keyboard_input = 0;
		got_pointer_input = 0;
	}

	return ret;
}

static int check_xrecord_mouse(void) {
	static int last_wx, last_wy, last_ww, last_wh;
	double spin = 0.0, tm, tnow;
	int i, scr_cnt = 0, input = 0, scroll_rep;
	int get_out, got_one = 0, flush1 = 0, flush2 = 0;
	int gp, gp0, ret = 0, db = debug_scroll;
	int gk, gk0;
	int fail = 0;
	int link, latency, netrate;

	int start_x, start_y, last_x, last_y;
	static double last_mouse_scroll = 0.0;
	double last_scroll;
	double max_spin[3], max_long[3], persist[3];
	double flush1_time = 0.01;
	static double last_flush = 0.0;
	double last_bdpush = 0.0, button_up_time = 0.0;
	int button_mask_save;
	int already_down = 0, max_ptr_eat = 20;
	static int want_back_in = 0;
	int came_back_in;
	int first_push = 1;

	int scroll_wheel = 0;
	int btn4 = (1<<3);
	int btn5 = (1<<4);

	RAWFB_RET(0)

	get_out = 1;
	if (button_mask) {
		get_out = 0;
	}
	if (want_back_in) {
		get_out = 0;
	}
	dtime0(&tnow);
if (0) fprintf(stderr, "check_xrecord_mouse: IN xrecording: %d\n", xrecording);

	if (get_out) {
		if (xrecording) {
			xrecord_watch(0, SCR_MOUSE);
		}
		return 0;
	}

	scroll_rep = scrollability(xrecord_ptr_window, SCR_NONE) + 1;
	if (scroll_rep == 1) {
		scroll_rep = 2;		/* if no info, assume the best. */
	}

	if (button_mask_prev) {
		already_down = 1;
	}
	if (want_back_in) {
		came_back_in = 1;
		first_push = 0;
	} else {
		came_back_in = 0;
	}
	want_back_in = 0;

	if (button_mask & (btn4|btn5)) {
		scroll_wheel = 1;
	}

	/*
	 * set up times for the various "reputations"
	 *
	 * 0 => -1, has been tried but never found a scroll.
	 * 1 =>  0, has not been tried.
	 * 2 => +1, has been tried and found a scroll.
	 */

	/* first spin-out time (no events) */
	max_spin[0] = 1*scr_mouse_time;
	max_spin[1] = 2*scr_mouse_time;
	max_spin[2] = 4*scr_mouse_time;
	if (!already_down) {
		for (i=0; i<3; i++) {
			max_spin[i] *= 1.5;
		}
	}

	/* max time between events */
	persist[0] = 1*scr_mouse_persist;
	persist[1] = 2*scr_mouse_persist;
	persist[2] = 4*scr_mouse_persist;

	/* absolute max time in the loop */
	max_long[0] = scr_mouse_maxtime;
	max_long[1] = scr_mouse_maxtime;
	max_long[2] = scr_mouse_maxtime;

	pointer_flush_delay = scr_mouse_pointer_delay;

	/* slow links: */
	link = link_rate(&latency, &netrate);
	if (link == LR_DIALUP) {
		for (i=0; i<3; i++) {
			max_spin[i] *= 2.0;
		}
		pointer_flush_delay *= 2;
	} else if (link == LR_BROADBAND) {
		pointer_flush_delay *= 2;
	}

	gp = gp0 = got_pointer_input;
	gk = gk0 = got_keyboard_input;
	dtime0(&tm);

	/*
	 * this is used for border pushes (bdpush) to guess location
	 * of scrollbar (region rects containing this point are pushed).
	 */
	last_x = start_x = cursor_x;
	last_y = start_y = cursor_y;

if (db) fprintf(stderr, "check_xrecord_mouse: BEGIN LOOP: scr_ev_cnt: "
    "%d max: %.3f  %.4f\n", scr_ev_cnt, max_spin[scroll_rep], tm - x11vnc_start);

	while (1) {
		double spin_check;
		if (scr_ev_cnt) {
			int dret, ev = scr_ev_cnt - 1;
			int bdpush = 0, bdx, bdy, bdskinny;
			double tm, dt, age = 0.35;

			got_one = 1;
			scrollability(xrecord_ptr_window, SCR_SUCCESS);
			scroll_rep = 2;

			scr_cnt++;

			dtime0(&last_scroll);
			last_mouse_scroll = last_scroll;

			if (last_bdpush == 0.0) {
				last_bdpush = last_scroll;
			}

			bdx = start_x;
			bdy = start_y;
			if (clipshift) {
				bdx += coff_x;
				bdy += coff_y;
			}
			if (subwin) {
				bdx += off_x;
				bdy += off_y;
			}
			bdskinny = 32;
			
			set_bdpush(SCR_MOUSE, &last_bdpush, &bdpush);

			dtime0(&tm);

			dret = push_scr_ev(&age, SCR_MOUSE, bdpush, bdx,
			    bdy, bdskinny, first_push);
			if (first_push) first_push = 0;
			ret = 1 + dret;

			dt = dtime(&tm);

if (db) fprintf(stderr, "  dret: %d  scr_ev_cnt: %d dt: %.4f\n",
	dret, scr_ev_cnt, dt);

			last_wx = scr_ev[ev].win_x;
			last_wy = scr_ev[ev].win_y;
			last_ww = scr_ev[ev].win_w;
			last_wh = scr_ev[ev].win_h;
			scr_ev_cnt = 0;

			if (! dret) {
				break;
			}
			if (0 && button_up_time > 0.0) {
				/* we only take 1 more event with button up */
if (db) fprintf(stderr, "check_xrecord: BUTTON_UP_SCROLL: %.3f\n", spin);
				break;
			}
		}


		if (! flush1) {
			if (! already_down || (!scr_cnt && spin>flush1_time)) {
				flush1 = 1;
				X_LOCK;
				XFlush_wr(dpy);
				X_UNLOCK;
				dtime0(&last_flush);
			}
		}

		if (use_threads) {
			usleep(1000);
		} else {
			rfbCFD(1000);
			rfbCFD(0);
		}
		spin += dtime(&tm);

		if (got_pointer_input > gp) {
			flush2 = 1;
			input += eat_pointer(max_ptr_eat, 1);
			gp = got_pointer_input;
		}
		if (got_keyboard_input > gk) {
			gk = got_keyboard_input;
			input++;
		}
		X_LOCK;
#if LIBVNCSERVER_HAVE_RECORD
		SCR_LOCK;
		XRecordProcessReplies(rdpy_data);
		SCR_UNLOCK;
#endif
		X_UNLOCK;

		if (! input) {
			spin_check = 1.5 * max_spin[scroll_rep];
		} else {
			spin_check = max_spin[scroll_rep];
		}

		if (button_up_time > 0.0) {
			if (tm > button_up_time + max_spin[scroll_rep]) {
if (db) fprintf(stderr, "check_xrecord: SPIN-OUT-BUTTON_UP: %.3f/%.3f\n", spin, tm - button_up_time);
				break;
			}
		} else if (!scr_cnt) {
			if (spin >= spin_check) {

if (db) fprintf(stderr, "check_xrecord: SPIN-OUT-1: %.3f/%.3f\n", spin, spin_check);
				fail = 1;
				break;
			}
		} else {
			if (tm >= last_scroll + persist[scroll_rep]) {

if (db) fprintf(stderr, "check_xrecord: SPIN-OUT-2: %.3f/%.3f\n", spin, tm - last_scroll);
				break;
			}
		}
		if (spin >= max_long[scroll_rep]) {

if (db) fprintf(stderr, "check_xrecord: SPIN-OUT-3: %.3f/%.3f\n", spin, max_long[scroll_rep]);
			break;
		}

		if (! button_mask) {
			int doflush = 0;
			if (button_up_time > 0.0) {
				;
			} else if (came_back_in) {
				dtime0(&button_up_time);
				doflush = 1;
			} else if (scroll_wheel) {
if (db) fprintf(stderr, "check_xrecord: SCROLL-WHEEL-BUTTON-UP-KEEP-GOING:  %.3f/%.3f %d/%d %d/%d\n", spin, max_long[scroll_rep], last_x, last_y, cursor_x, cursor_y);
				doflush = 1;
				dtime0(&button_up_time);
			} else if (last_x == cursor_x && last_y == cursor_y) {
if (db) fprintf(stderr, "check_xrecord: BUTTON-UP:  %.3f/%.3f %d/%d %d/%d\n", spin, max_long[scroll_rep], last_x, last_y, cursor_x, cursor_y);
				break;
			} else {
if (db) fprintf(stderr, "check_xrecord: BUTTON-UP-KEEP-GOING:  %.3f/%.3f %d/%d %d/%d\n", spin, max_long[scroll_rep], last_x, last_y, cursor_x, cursor_y);
				doflush = 1;
				dtime0(&button_up_time);
			}
			if (doflush) {
				flush1 = 1;
				X_LOCK;
				XFlush_wr(dpy);
				X_UNLOCK;
				dtime0(&last_flush);
			}
		}

		last_x = cursor_x;
		last_y = cursor_y;
	}

	if (got_one) {
		set_xdamage_mark(last_wx, last_wy, last_ww, last_wh);
	}

	if (fail) {
		scrollability(xrecord_ptr_window, SCR_FAIL);
	}

	/* flush any remaining pointer events. */
	button_mask_save = button_mask;
	pointer_queued_sent = 0;
	last_x = cursor_x;
	last_y = cursor_y;
	pointer_event(-1, 0, 0, NULL);
	pointer_flush_delay = 0.0;

	if (xrecording && pointer_queued_sent && button_mask_save &&
	    (last_x != cursor_x || last_y != cursor_y) ) {
if (db) fprintf(stderr, "  pointer() push yields events on: ret=%d\n", ret);
		if (ret == 2) {
if (db) fprintf(stderr, "  we decide to send ret=3\n");
			want_back_in = 1;
			ret = 3;
			flush2 = 1;
		} else {
			if (ret) {
				ret = 1;
			} else {
				ret = 0;
			}
			xrecord_watch(0, SCR_MOUSE);
		}
	} else {
		if (ret) {
			ret = 1;
		} else {
			ret = 0;
		}
		if (xrecording) {
			xrecord_watch(0, SCR_MOUSE);
		}
	}

	if (flush2) {
		X_LOCK;
		XFlush_wr(dpy);
		XFlush_wr(rdpy_ctrl);
		X_UNLOCK;

		flush2 = 1;
		dtime0(&last_flush);

if (db) fprintf(stderr, "FLUSH-2\n");
	}

	/* since we've flushed it, we might as well avoid -input_skip */
	if (flush1 || flush2) {
		got_keyboard_input = 0;
		got_pointer_input = 0;
	}

	if (ret) {
		return ret;
	} else if (scr_cnt) {
		return 1;
	} else {
		return 0;
	}
}

int check_xrecord(void) {
	int watch_keys = 0, watch_mouse = 0, consider_mouse;
	static int mouse_wants_back_in = 0;

	RAWFB_RET(0)

	if (! use_xrecord) {
		return 0;
	}
	if (unixpw_in_progress) return 0;

	if (skip_cr_when_scaling("scroll")) {
		return 0;
	}

if (0) fprintf(stderr, "check_xrecord: IN xrecording: %d\n", xrecording);

	if (! xrecording) {
		return 0;
	}

	if (!strcmp(scroll_copyrect, "always")) {
		watch_keys = 1;
		watch_mouse = 1;
	} else if (!strcmp(scroll_copyrect, "keys")) {
		watch_keys = 1;
	} else if (!strcmp(scroll_copyrect, "mouse")) {
		watch_mouse = 1;
	}

	if (button_mask || mouse_wants_back_in) {
		consider_mouse = 1;
	} else {
		consider_mouse = 0;
	}
if (0) fprintf(stderr, "check_xrecord: button_mask: %d  mouse_wants_back_in: %d\n", button_mask, mouse_wants_back_in);

	if (watch_mouse && consider_mouse && xrecord_set_by_mouse) {
		int ret = check_xrecord_mouse();
		if (ret == 3) {
			mouse_wants_back_in = 1;
		} else {
			mouse_wants_back_in = 0;
		}
		return ret;
	} else if (watch_keys && xrecord_set_by_keys) {
		mouse_wants_back_in = 0;
		return check_xrecord_keys();
	} else {
		mouse_wants_back_in = 0;
		return 0;
	}
}

#define DB_SET \
	int db  = 0; \
	int db2 = 0; \
	if (debug_wireframe == 1) { \
		db = 1; \
	} \
	if (debug_wireframe == 2) { \
		db2 = 1; \
	} \
	if (debug_wireframe == 3) { \
		db = 1; \
		db2 = 1; \
	}

#define NBATCHMAX 1024
int batch_dxs[NBATCHMAX], batch_dys[NBATCHMAX];
sraRegionPtr batch_reg[NBATCHMAX];

static int try_copyrect(Window orig_frame, Window frame, int x, int y, int w, int h,
    int dx, int dy, int *obscured, sraRegionPtr extra_clip, double max_wait, int *nbatch) {

	static int dt_bad = 0;
	static time_t dt_bad_check = 0;
	int x1, y1, x2, y2, sent_copyrect = 0;
	int req, mod, cpy, ncli;
	double tm, dt;
	DB_SET

	if (nbatch == NULL) {
		get_client_regions(&req, &mod, &cpy, &ncli);
		if (cpy) {
			/* one is still pending... try to force it out: */
			if (!fb_push_wait(max_wait, FB_COPY)) {
				fb_push_wait(max_wait/2, FB_COPY);
			}

			get_client_regions(&req, &mod, &cpy, &ncli);
		}
		if (cpy) {
			return 0;
		}
	}

	*obscured = 0;
	/*
	 * XXX KDE and xfce do some weird things with the 
	 * stacking, it does not match XQueryTree.  Work around
	 * it for now by CopyRect-ing the *whole* on-screen 
	 * rectangle (whether obscured or not!)
	 */
	if (time(NULL) > dt_bad_check + 5) {
		char *dt = guess_desktop();
		if (!strcmp(dt, "kde_maybe_is_ok_now...")) {
			dt_bad = 1;
		} else if (!strcmp(dt, "xfce")) {
			dt_bad = 1;
		} else {
			dt_bad = 0;
		}
		dt_bad_check = time(NULL);
	}

	if (clipshift) {
		x -= coff_x;
		y -= coff_y;
	}
	if (subwin) {
		x -= off_x;
		y -= off_y;
	}
if (db2) fprintf(stderr, "try_copyrect: 0x%lx/0x%lx  bad: %d stack_list_num: %d\n", orig_frame, frame, dt_bad, stack_list_num);

/* XXX Y dt_bad = 0 */
	if (dt_bad && wireframe_in_progress) {
		sraRegionPtr rect;
		/* send the whole thing... */
		x1 = crfix(nfix(x,   dpy_x), dx, dpy_x);
		y1 = crfix(nfix(y,   dpy_y), dy, dpy_y);
		x2 = crfix(nfix(x+w, dpy_x+1), dx, dpy_x+1);
		y2 = crfix(nfix(y+h, dpy_y+1), dy, dpy_y+1);

		rect = sraRgnCreateRect(x1, y1, x2, y2);

		if (blackouts) {
			int i;
			sraRegionPtr bo_rect;
			for (i=0; i<blackouts; i++) {
				bo_rect = sraRgnCreateRect(blackr[i].x1,
				    blackr[i].y1, blackr[i].x2, blackr[i].y2);
				sraRgnSubtract(rect, bo_rect);
				sraRgnDestroy(bo_rect);
			}
		}
		if (!nbatch) {
			do_copyregion(rect, dx, dy, 0);
		} else {
			batch_dxs[*nbatch] = dx;
			batch_dys[*nbatch] = dy;
			batch_reg[*nbatch] = sraRgnCreateRgn(rect);
			(*nbatch)++;
		}
		sraRgnDestroy(rect);

		sent_copyrect = 1;
		*obscured = 1;	/* set to avoid an aggressive push */

	} else if (stack_list_num || dt_bad) {
		int k, tx1, tx2, ty1, ty2;
		sraRegionPtr moved_win, tmp_win, whole;
		sraRectangleIterator *iter;
		sraRect rect;
		int saw_me = 0;
		int orig_x, orig_y;
		int boff, bwin;
		XWindowAttributes attr;

		orig_x = x - dx;
		orig_y = y - dy;

		tx1 = nfix(orig_x,   dpy_x);
		ty1 = nfix(orig_y,   dpy_y);
		tx2 = nfix(orig_x+w, dpy_x+1);
		ty2 = nfix(orig_y+h, dpy_y+1);

if (db2) fprintf(stderr, "moved_win: %4d %3d, %4d %3d  0x%lx ---\n",
	tx1, ty1, tx2, ty2, frame);

		moved_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);

		dtime0(&tm);

		boff = get_boff();
		bwin = get_bwin();

		X_LOCK;

		/*
		 * loop over the stack, top to bottom until we
		 * find our wm frame:
		 */
		for (k = stack_list_num - 1; k >= 0; k--) {
			Window swin;

			if (0 && dt_bad) {
				break;
			}

			swin = stack_list[k].win;
if (db2) fprintf(stderr, "sw: %d/%lx\n", k, swin);
			if (swin == frame || swin == orig_frame) {
 if (db2) {
 saw_me = 1; fprintf(stderr, "  ----------\n");
 } else {
				break;	
 }
			}

			/* skip some unwanted cases: */
#ifndef MACOSX
			if (swin == None) {
				continue;
			}
#endif
			if (boff <= (int) swin && (int) swin < boff + bwin) {
				;	/* blackouts */
			} else if (! stack_list[k].fetched ||
			    stack_list[k].time > tm + 2.0) {
				if (!valid_window(swin, &attr, 1)) {
					stack_list[k].valid = 0;
				} else {
					stack_list[k].valid = 1;
					stack_list[k].x = attr.x;
					stack_list[k].y = attr.y;
					stack_list[k].width = attr.width;
					stack_list[k].height = attr.height;
					stack_list[k].border_width = attr.border_width;
					stack_list[k].depth = attr.depth;
					stack_list[k].class = attr.class;
					stack_list[k].backing_store =
					    attr.backing_store;
					stack_list[k].map_state =
					    attr.map_state;
				}
				stack_list[k].fetched = 1;
				stack_list[k].time = tm;
			}
			if (!stack_list[k].valid) {
				continue;
			}

			attr.x      = stack_list[k].x;
			attr.y      = stack_list[k].y;
			attr.depth  = stack_list[k].depth;
			attr.width  = stack_list[k].width;
			attr.height = stack_list[k].height;
			attr.border_width = stack_list[k].border_width;
			attr.map_state = stack_list[k].map_state;

			if (attr.map_state != IsViewable) {
				continue;
			}
if (db2) fprintf(stderr, "sw: %d/%lx  %dx%d+%d+%d\n", k, swin, stack_list[k].width, stack_list[k].height, stack_list[k].x, stack_list[k].y);

			if (clipshift) {
				attr.x -= coff_x;
				attr.y -= coff_y;
			}
			if (subwin) {
				attr.x -= off_x;
				attr.y -= off_y;
			}

			/*
			 * first subtract any overlap from the initial
			 * window rectangle
			 */

			/* clip the window to the visible screen: */
			tx1 = nfix(attr.x, dpy_x);
			ty1 = nfix(attr.y, dpy_y);
			tx2 = nfix(attr.x + attr.width,  dpy_x+1);
			ty2 = nfix(attr.y + attr.height, dpy_y+1);

if (db2) fprintf(stderr, "  tmp_win-1: %4d %3d, %4d %3d  0x%lx\n",
	tx1, ty1, tx2, ty2, swin);
if (db2 && saw_me) continue;

			/* see if window clips us: */
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			if (sraRgnAnd(tmp_win, moved_win)) {
				*obscured = 1;
if (db2) fprintf(stderr, "         : clips it.\n");
			}
			sraRgnDestroy(tmp_win);

			/* subtract it from our region: */
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			sraRgnSubtract(moved_win, tmp_win);
			sraRgnDestroy(tmp_win);

			/*
			 * next, subtract from the initial window rectangle
			 * anything that would clip it.
			 */

			/* clip the window to the visible screen: */
			tx1 = nfix(attr.x - dx, dpy_x);
			ty1 = nfix(attr.y - dy, dpy_y);
			tx2 = nfix(attr.x - dx + attr.width,  dpy_x+1);
			ty2 = nfix(attr.y - dy + attr.height, dpy_y+1);

if (db2) fprintf(stderr, "  tmp_win-2: %4d %3d, %4d %3d  0x%lx\n",
	tx1, ty1, tx2, ty2, swin);
if (db2 && saw_me) continue;

			/* subtract it from our region: */
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			sraRgnSubtract(moved_win, tmp_win);
			sraRgnDestroy(tmp_win);
		}

		X_UNLOCK;

		if (extra_clip && ! sraRgnEmpty(extra_clip)) {
		    whole = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

		    if (clipshift) {
			sraRgnOffset(extra_clip, -coff_x, -coff_y);
		    }
		    if (subwin) {
			sraRgnOffset(extra_clip, -off_x, -off_y);
		    }

		    iter = sraRgnGetIterator(extra_clip);
		    while (sraRgnIteratorNext(iter, &rect)) {
			/* clip the window to the visible screen: */
			tx1 = rect.x1;
			ty1 = rect.y1;
			tx2 = rect.x2;
			ty2 = rect.y2;
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			sraRgnAnd(tmp_win, whole);

			/* see if window clips us: */
			if (sraRgnAnd(tmp_win, moved_win)) {
				*obscured = 1;
			}
			sraRgnDestroy(tmp_win);

			/* subtract it from our region: */
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			sraRgnSubtract(moved_win, tmp_win);
			sraRgnDestroy(tmp_win);

			/*
			 * next, subtract from the initial window rectangle
			 * anything that would clip it.
			 */
			tmp_win = sraRgnCreateRect(tx1, ty1, tx2, ty2);
			sraRgnOffset(tmp_win, -dx, -dy);

			/* clip the window to the visible screen: */
			sraRgnAnd(tmp_win, whole);

			/* subtract it from our region: */
			sraRgnSubtract(moved_win, tmp_win);
			sraRgnDestroy(tmp_win);
		    }
		    sraRgnReleaseIterator(iter);
		    sraRgnDestroy(whole);
		}

		dt = dtime(&tm);
if (db2) fprintf(stderr, "  stack_work dt: %.4f\n", dt);

		if (*obscured && !strcmp(wireframe_copyrect, "top")) {
			;	/* cannot send CopyRegion */
		} else if (! sraRgnEmpty(moved_win)) {
			sraRegionPtr whole, shifted_region;

			whole = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
			shifted_region = sraRgnCreateRgn(moved_win);
			sraRgnOffset(shifted_region, dx, dy);
			sraRgnAnd(shifted_region, whole);

			sraRgnDestroy(whole);

			/* now send the CopyRegion: */
			if (! sraRgnEmpty(shifted_region)) {
				dtime0(&tm);
				if (!nbatch) {
					do_copyregion(shifted_region, dx, dy, 0);
				} else {
					batch_dxs[*nbatch] = dx;
					batch_dys[*nbatch] = dy;
					batch_reg[*nbatch] = sraRgnCreateRgn(shifted_region);
					(*nbatch)++;
					
				}
				dt = dtime(&tm);
if (0 || db2) fprintf(stderr, "do_copyregion: %d %d %d %d  dx: %d  dy: %d dt: %.4f\n",
	tx1, ty1, tx2, ty2, dx, dy, dt);
				sent_copyrect = 1;
			}
			sraRgnDestroy(shifted_region);
		}
		sraRgnDestroy(moved_win);
	}
	return sent_copyrect;
}
		
int near_wm_edge(int x, int y, int w, int h, int px, int py) {
	/* heuristics: */
	int wf_t = wireframe_top;
	int wf_b = wireframe_bot;
	int wf_l = wireframe_left;
	int wf_r = wireframe_right;

	int near_edge = 0;
	
	if (wf_t || wf_b || wf_l || wf_r) {
		if (nabs(y - py) < wf_t) {
			near_edge = 1;
		}
		if (nabs(y + h - py) < wf_b) {
			near_edge = 1;
		}
		if (nabs(x - px) < wf_l) {
			near_edge = 1;
		}
		if (nabs(x + w - px) < wf_r) {
			near_edge = 1;
		}
	} else {
		/* all zero; always "near" edge: */
		near_edge = 1;
	}
	return near_edge;
}

int near_scrollbar_edge(int x, int y, int w, int h, int px, int py) {
	/* heuristics: */
	int sb_t = scrollcopyrect_top;
	int sb_b = scrollcopyrect_bot;
	int sb_l = scrollcopyrect_left;
	int sb_r = scrollcopyrect_right;

	int near_edge = 0;
	
	if (sb_t || sb_b || sb_l || sb_r) {
		if (nabs(y - py) < sb_t) {
			near_edge = 1;
		}
		if (nabs(y + h - py) < sb_b) {
			near_edge = 1;
		}
		if (nabs(x - px) < sb_l) {
			near_edge = 1;
		}
		if (nabs(x + w - px) < sb_r) {
			near_edge = 1;
		}
	} else {
		/* all zero; always "near" edge: */
		near_edge = 1;
	}
	return near_edge;
}

void check_fixscreen(void) {
	double now = dnow();
	int didfull = 0, db = 0;

	if (!client_count) {
		return;
	}
	if (unixpw_in_progress) return;

	if (screen_fixup_X > 0.0) {
		static double last = 0.0;
		if (now > last + screen_fixup_X) {
			if (db) rfbLog("doing screen_fixup_X\n");
			do_copy_screen = 1;
			last = now;
			didfull = 1;
		}
		
	}
	if (screen_fixup_V > 0.0) {
		static double last = 0.0;
		if (now > last + screen_fixup_V) {
			if (! didfull) {
				refresh_screen(0);
				if (db) rfbLog("doing screen_fixup_V\n");
			}
			last = now;
			didfull = 1;
		}
	}
	if (screen_fixup_C > 0.0) {
		static double last = 0.0;
		if (last_copyrect_fix < last_copyrect &&
		    now > last_copyrect + screen_fixup_C) {
			if (! didfull) {
				refresh_screen(0);
				if (db) rfbLog("doing screen_fixup_C\n");
			}
			last_copyrect_fix = now;
			last = now;
			didfull = 1;
		}
	}
	if (scaling && last_copyrect_fix < last_copyrect) {
		static double last = 0.0;
		double delay = 3.0;
		if (now > last + delay) {
			if (! didfull) {
				scale_and_mark_rect(0, 0, dpy_x, dpy_y, 1);
				if (db) rfbLog("doing scale screen_fixup\n");
			}
			last_copyrect_fix = now;
			last = now;
			didfull = 1;
		}
	}
	if (advertise_truecolor && advertise_truecolor_reset && indexed_color) {
		/* this will reset framebuffer to correct colors, if needed */
		static double dlast = 0.0;
		now = dnow();
		if (now > last_client + 1.0 && now < last_client + 3.0 && now > dlast + 5.0) {
			rfbLog("advertise truecolor reset framebuffer\n");
			do_new_fb(1);
			dlast = dnow();
			return;
		}
	}
}

static int wireframe_mod_state() {
	if (! wireframe_mods) {
		return 0;
	}
	if (!strcmp(wireframe_mods, "all")) {
		if (track_mod_state(NoSymbol, FALSE, FALSE)) {
			return 1;
		} else {
			return 0;
		}

	} else if (!strcmp(wireframe_mods, "Alt")) {
		if (track_mod_state(XK_Alt_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Alt_R, FALSE, FALSE) == 1) {
			return 1;
		}
	} else if (!strcmp(wireframe_mods, "Shift")) {
		if (track_mod_state(XK_Shift_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Shift_R, FALSE, FALSE) == 1) {
			return 1;
		}
	} else if (!strcmp(wireframe_mods, "Control")) {
		if (track_mod_state(XK_Control_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Control_R, FALSE, FALSE) == 1) {
			return 1;
		}
	} else if (!strcmp(wireframe_mods, "Meta")) {
		if (track_mod_state(XK_Meta_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Meta_R, FALSE, FALSE) == 1) {
			return 1;
		}
	} else if (!strcmp(wireframe_mods, "Super")) {
		if (track_mod_state(XK_Super_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Super_R, FALSE, FALSE) == 1) {
			return 1;
		}
	} else if (!strcmp(wireframe_mods, "Hyper")) {
		if (track_mod_state(XK_Hyper_L, FALSE, FALSE) == 1) {
			return 1;
		} else if (track_mod_state(XK_Hyper_R, FALSE, FALSE) == 1) {
			return 1;
		}
	}
	return 0;
}

static int NPP_nreg = 0;
static sraRegionPtr NPP_roffscreen = NULL;
static sraRegionPtr NPP_r_bs_tmp = NULL;
static Window NPP_nwin = None;

void clear_win_events(Window win, int vis) {
#if !NO_X11
	if (dpy && win != None && ncache) {
		XEvent ev;
		XErrorHandler old_handler;
		old_handler = XSetErrorHandler(trap_xerror);
		trapped_xerror = 0;
		while (XCheckTypedWindowEvent(dpy, win, ConfigureNotify, &ev)) {
			if (ncdb) fprintf(stderr, ".");
			if (trapped_xerror) {
				break;
			}
			trapped_xerror = 0;
		}
/* XXX Y */
		if (vis) {
			while (XCheckTypedWindowEvent(dpy, win, VisibilityNotify, &ev)) {
				if (ncdb) fprintf(stderr, "+");
				if (trapped_xerror) {
					break;
				}
				trapped_xerror = 0;
			}
		}
		XSetErrorHandler(old_handler);
		if (ncdb) fprintf(stderr, " 0x%lx\n", win);
	}
#endif
}

void push_borders(sraRect *rects, int nrect) {
		int i, s = 2;
		sraRegionPtr r0, r1, r2;

		r0 = sraRgnCreate(); 
		r1 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 

		for (i=0; i<nrect; i++) {
			int x = rects[i].x1;
			int y = rects[i].y1;
			int w = rects[i].x2;
			int h = rects[i].y2;

			if (w > 0 && h > 0 && w * h > 64 * 64) {
				r2 = sraRgnCreateRect(x - s, y , x , y + h); 
				sraRgnOr(r0, r2); 
				sraRgnDestroy(r2);
				
				r2 = sraRgnCreateRect(x + w, y , x + w + s, y + h); 
				sraRgnOr(r0, r2); 
				sraRgnDestroy(r2);

				r2 = sraRgnCreateRect(x - s, y - s, x + w + s, y + s); 
				sraRgnOr(r0, r2); 
				sraRgnDestroy(r2);
				
				r2 = sraRgnCreateRect(x - s, y , x + w + s, y + h + s); 
				sraRgnOr(r0, r2); 
				sraRgnDestroy(r2);
			}
		}

		sraRgnAnd(r0, r1); 

		if (!sraRgnEmpty(r0)) {
			double d = dnow();
			sraRectangleIterator *iter;
			sraRect rect;
			int db = 0;

			if (db) fprintf(stderr, "SCALE_BORDER\n");
			fb_push_wait(0.05, FB_MOD|FB_COPY);

			iter = sraRgnGetIterator(r0);
			while (sraRgnIteratorNext(iter, &rect)) {
				/* clip the window to the visible screen: */
				int tx1 = rect.x1;
				int ty1 = rect.y1;
				int tx2 = rect.x2;
				int ty2 = rect.y2;
				scale_and_mark_rect(tx1, ty1, tx2, ty2, 1);
			}
			sraRgnReleaseIterator(iter);

			if (db) fprintf(stderr, "SCALE_BORDER %.4f\n", dnow() - d);
			fb_push_wait(0.1, FB_MOD|FB_COPY);
			if (db) fprintf(stderr, "SCALE_BORDER %.4f\n", dnow() - d);
		}
		sraRgnDestroy(r0);
		sraRgnDestroy(r1);
}

void ncache_pre_portions(Window orig_frame, Window frame, int *nidx_in, int try_batch, int *use_batch,
    int orig_x, int orig_y, int orig_w, int orig_h, int x, int y, int w, int h, double ntim) {
	int nidx, np = ncache_pad;

	if (!ntim) {}
	*use_batch = 0;
	*nidx_in = -1;
	NPP_nreg = 0;
	NPP_roffscreen = NULL;
	NPP_r_bs_tmp = NULL;
	NPP_nwin = None;
	
	if (ncache <= 0) {
		return;
	}

	if (rotating) {
		try_batch = 0;
	}

	if (*nidx_in == -1) {
		nidx = lookup_win_index(orig_frame);
		NPP_nwin = orig_frame;
		if (nidx < 0) {
			nidx = lookup_win_index(frame);
			NPP_nwin = frame;
		}
	} else {
		nidx = *nidx_in;
	}
	if (nidx > 0) {
		sraRegionPtr r0, r1, r2;
		int dx, dy;
		int bs_x = cache_list[nidx].bs_x;	
		int bs_y = cache_list[nidx].bs_y;	
		int bs_w = cache_list[nidx].bs_w;	
		int bs_h = cache_list[nidx].bs_h;	

		*nidx_in = nidx;

		if (bs_x < 0) {
			if (!find_rect(nidx, x, y, w, h)) {
				nidx = -1;
				return;
			}
			bs_x = cache_list[nidx].bs_x;
			bs_y = cache_list[nidx].bs_y;
			bs_w = cache_list[nidx].bs_w;
			bs_h = cache_list[nidx].bs_h;
		}
		if (bs_x < 0) {
			nidx = -1;
			return;
		}

		if (try_batch) {
			*use_batch = 1;
		}

		if (ncache_pad) {
			orig_x -= np;	
			orig_y -= np;	
			orig_w += 2 * np;	
			orig_h += 2 * np;	
			x -= np;	
			y -= np;	
			w += 2 * np;	
			h += 2 * np;	
		}

		if (clipshift) {
			orig_x -= coff_x;
			orig_y -= coff_y;
			x -= coff_x;
			y -= coff_y;
		}

		r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 

		r2 = sraRgnCreateRect(orig_x, orig_y, orig_x + orig_w, orig_y + orig_h);
		sraRgnSubtract(r2, r0);
		if (! sraRgnEmpty(r2) && cache_list[nidx].bs_time > 0.0) {
			/* some is initially offscreen */
			dx = bs_x - orig_x;
			dy = bs_y - orig_y;
			sraRgnOffset(r2, dx, dy);
			dx = 0;
			dy = dpy_y;
			sraRgnOffset(r2, dx, dy);
if (ncdb) fprintf(stderr, "FB_COPY: %.4f 1) offscreen:  dx, dy: %d, %d -> %d, %d orig %dx%d+%d+%d bs_xy: %d %d\n",
    dnow() - ntim, bs_x - orig_x, bs_y - orig_y, dx, dy, orig_w, orig_h, orig_x, orig_y, bs_x, bs_y);

			/* 0) save it in the invalid (offscreen) SU portion */
			if (! *use_batch) {
				do_copyregion(r2, dx, dy, 0);
				if (! fb_push_wait(0.2, FB_COPY)) {
					fb_push_wait(0.1, FB_COPY);
				}
			} else {
				batch_dxs[NPP_nreg] = dx;
				batch_dys[NPP_nreg] = dy;
				batch_reg[NPP_nreg++] = sraRgnCreateRgn(r2);
			}
			NPP_roffscreen = sraRgnCreateRgn(r2);
		}
		sraRgnDestroy(r2);

		/* 1) use bs for temp storage of the new save under. */
		r1 = sraRgnCreateRect(x, y, x + w, y + h);
		sraRgnAnd(r1, r0);

		dx = bs_x - x;
		dy = bs_y - y;
		sraRgnOffset(r1, dx, dy);

if (ncdb) fprintf(stderr, "FB_COPY: %.4f 1) use tmp bs:\n", dnow() - ntim);
		if (! *use_batch) {
			do_copyregion(r1, dx, dy, 0);
			if (! fb_push_wait(0.2, FB_COPY)) {
if (ncdb) fprintf(stderr, "FB_COPY: %.4f 1) FAILED.\n", dnow() - ntim);
				fb_push_wait(0.1, FB_COPY);
			}
		} else {
			batch_dxs[NPP_nreg] = dx;
			batch_dys[NPP_nreg] = dy;
			batch_reg[NPP_nreg++] = sraRgnCreateRgn(r1);
		}
		NPP_r_bs_tmp = sraRgnCreateRgn(r1);
		sraRgnDestroy(r0);
		sraRgnDestroy(r1);
	}
}

void ncache_post_portions(int nidx, int use_batch, int orig_x, int orig_y, int orig_w, int orig_h,
    int x, int y, int w, int h, double batch_delay, double ntim) {
	int np = ncache_pad;
	int db = 0;

	if (ncache > 0 && nidx >= 0) {
		sraRegionPtr r0, r1, r2, r3;
		int dx, dy;
		int su_x = cache_list[nidx].su_x;
		int su_y = cache_list[nidx].su_y;
		int su_w = cache_list[nidx].su_w;
		int su_h = cache_list[nidx].su_h;
		int bs_x = cache_list[nidx].bs_x;
		int bs_y = cache_list[nidx].bs_y;
		int bs_w = cache_list[nidx].bs_w;
		int bs_h = cache_list[nidx].bs_h;
		int some_su = 0;

if (db) fprintf(stderr, "su: %dx%d+%d+%d  bs: %dx%d+%d+%d\n", su_w, su_h, su_x, su_y, bs_w, bs_h, bs_x, bs_y);

		if (bs_x < 0) {
			if (!find_rect(nidx, x, y, w, h)) {
				return;
			}
			su_x = cache_list[nidx].su_x;
			su_y = cache_list[nidx].su_y;
			su_w = cache_list[nidx].su_w;
			su_h = cache_list[nidx].su_h;
			bs_x = cache_list[nidx].bs_x;
			bs_y = cache_list[nidx].bs_y;
			bs_w = cache_list[nidx].bs_w;
			bs_h = cache_list[nidx].bs_h;
		}
		if (bs_x < 0) {
			return;
		}

		if (ncache_pad) {
			orig_x -= np;	
			orig_y -= np;	
			orig_w += 2 * np;	
			orig_h += 2 * np;	
			x -= np;	
			y -= np;	
			w += 2 * np;	
			h += 2 * np;	
		}

		if (clipshift) {
			orig_x -= coff_x;
			orig_y -= coff_y;
			x -= coff_x;
			y -= coff_y;
		}

		r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 

		/* 0b) copy this bs part stored in saveunder */
		if (NPP_roffscreen != NULL) {
			dx = x - su_x;
			dy = y - su_y;
			sraRgnOffset(NPP_roffscreen, dx, dy);
			sraRgnAnd(NPP_roffscreen, r0);
			
			if (! use_batch) {
				do_copyregion(NPP_roffscreen, dx, dy, 0);
				if (!fb_push_wait(0.2, FB_COPY)) {
					fb_push_wait(0.1, FB_COPY);
				}
			} else {
				batch_dxs[NPP_nreg] = dx;
				batch_dys[NPP_nreg] = dy;
				batch_reg[NPP_nreg++] = sraRgnCreateRgn(NPP_roffscreen);
			}
			sraRgnDestroy(NPP_roffscreen);
		}

		/* 3) copy from the saveunder to where orig win was */
		r1 = sraRgnCreateRect(orig_x, orig_y, orig_x + orig_w, orig_y + orig_h);
		sraRgnAnd(r1, r0);
		r2 = sraRgnCreateRect(x+np, y+np, x + w-np, y + h-np);
		sraRgnAnd(r2, r0);
		sraRgnSubtract(r1, r2);

		dx = orig_x - su_x;
		dy = orig_y - su_y;
if (db && ncdb) fprintf(stderr, "FB_COPY: %.4f 3) sent_copyrect: su_restore: %d %d\n", dnow() - ntim, dx, dy);
		if (cache_list[nidx].su_time == 0.0) {
			;
		} else if (! use_batch) {
			do_copyregion(r1, dx, dy, 0);
			if (!fb_push_wait(0.2, FB_COPY)) {
if (db && ncdb) fprintf(stderr, "FB_COPY: %.4f 3) FAILED.\n", dnow() - ntim);
				fb_push_wait(0.1, FB_COPY);
			}
		} else {
			batch_dxs[NPP_nreg] = dx;
			batch_dys[NPP_nreg] = dy;
			batch_reg[NPP_nreg++] = sraRgnCreateRgn(r1);
		}
if (db && ncdb) fprintf(stderr, "sent_copyrect: %.4f su_restore: done.\n", dnow() - ntim);
		sraRgnDestroy(r0);
		sraRgnDestroy(r1);
		sraRgnDestroy(r2);

		/* 4) if overlap between orig and displaced, move the corner that will still be su: */
		r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 

		r1 = sraRgnCreateRect(orig_x, orig_y, orig_x + orig_w, orig_y + orig_h);
		sraRgnAnd(r1, r0);
		r2 = sraRgnCreateRect(x, y, x + w, y + h);
		sraRgnAnd(r2, r0);
		r3 = NULL;
		if (sraRgnAnd(r2, r1) && cache_list[nidx].su_time > 0.0) {
			int dx2 = su_x - orig_x;
			int dy2 = su_y - orig_y;

			r3 = sraRgnCreateRgn(r2);
			sraRgnOffset(r2, dx2, dy2); 

			dx = su_x - x;
			dy = su_y - y;
			sraRgnOffset(r3, dx, dy); 

			dx = dx - dx2;
			dy = dy - dy2;

if (db && ncdb) fprintf(stderr, "FB_COPY: %.4f 4) move overlap inside su:\n", dnow() - ntim);
			if (! use_batch) {
				do_copyregion(r3, dx, dy, 0);
				if (!fb_push_wait(0.2, FB_COPY)) {
if (db) fprintf(stderr, "FB_COPY: %.4f 4) FAILED.\n", dnow() - ntim);
					fb_push_wait(0.1, FB_COPY);
				}
			} else {
				batch_dxs[NPP_nreg] = dx;
				batch_dys[NPP_nreg] = dy;
				batch_reg[NPP_nreg++] = sraRgnCreateRgn(r3);
			}
		}
		sraRgnDestroy(r0);
		sraRgnDestroy(r1);
		sraRgnDestroy(r2);

		/* 5) copy our temporary stuff from bs to su: */
		dx = su_x - bs_x;
		dy = su_y - bs_y;
		if (NPP_r_bs_tmp == NULL) {
			r1 = sraRgnCreateRect(su_x, su_y, su_x + su_w, su_y + su_h); 
		} else {
			r1 = sraRgnCreateRgn(NPP_r_bs_tmp);
			sraRgnOffset(r1, dx, dy);
			sraRgnDestroy(NPP_r_bs_tmp);
		}
		if (r3 != NULL) {
			sraRgnSubtract(r1, r3);
			sraRgnDestroy(r3);
		}
if (db) fprintf(stderr, "FB_COPY: %.4f 5) move tmp bs to su:\n", dnow() - ntim);
		if (! use_batch) {
			do_copyregion(r1, dx, dy, 0);
			if (!fb_push_wait(0.2, FB_COPY)) {
if (db) fprintf(stderr, "FB_COPY: %.4f 5) FAILED.\n", dnow() - ntim);
				fb_push_wait(0.1, FB_COPY);
			}
		} else {
			batch_dxs[NPP_nreg] = dx;
			batch_dys[NPP_nreg] = dy;
			batch_reg[NPP_nreg++] = sraRgnCreateRgn(r1);
		}
		if (! sraRgnEmpty(r1)) {
			some_su = 1;
		}
		sraRgnDestroy(r1);

		/* 6) not really necessary, update bs with current view: */
		r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 
		r1 = sraRgnCreateRect(x, y, x + w, y + h);
		sraRgnAnd(r1, r0);
		dx = bs_x - x;
		dy = bs_y - y;
		sraRgnOffset(r1, dx, dy);
if (db) fprintf(stderr, "FB_COPY: %.4f 6) snapshot bs:\n", dnow() - ntim);
		if (! use_batch) {
			do_copyregion(r1, dx, dy, 0);
			if (!fb_push_wait(0.2, FB_COPY)) {
if (db) fprintf(stderr, "FB_COPY: %.4f 6) FAILED.\n", dnow() - ntim);
				fb_push_wait(0.1, FB_COPY);
			}
		} else {
			batch_dxs[NPP_nreg] = dx;
			batch_dys[NPP_nreg] = dy;
			batch_reg[NPP_nreg++] = sraRgnCreateRgn(r1);
		}
		sraRgnDestroy(r0);
		sraRgnDestroy(r1);

		if (use_batch) {
			batch_push(NPP_nreg, batch_delay);
if (ncdb) fprintf(stderr, "FB_COPY: %.4f XX did batch 0x%x %3d su: %dx%d+%d+%d  bs: %dx%d+%d+%d\n", dnow() - ntim,
	(unsigned int) cache_list[nidx].win, nidx, su_w, su_h, su_x, su_y, bs_w, bs_h, bs_x, bs_y);
		}
		cache_list[nidx].x = x + np;
		cache_list[nidx].y = y + np;

		/* XXX Y */
		cache_list[nidx].bs_time = dnow();
		if (some_su) {
			cache_list[nidx].su_time = dnow();
		}
	} else {
		if (use_batch) {
			batch_push(NPP_nreg, batch_delay);
		}
	}

	if (scaling) {
		sraRect rects[2];	

		rects[0].x1 = orig_x;
		rects[0].y1 = orig_y;
		rects[0].x2 = orig_w;
		rects[0].y2 = orig_h;

		rects[1].x1 = x;
		rects[1].y1 = y;
		rects[1].x2 = w;
		rects[1].y2 = h;
		push_borders(rects, 2);
	}
}

void do_copyrect_drag_move(Window orig_frame, Window frame, int *nidx, int try_batch,
    int now_x, int now_y, int orig_w, int orig_h, int x, int y, int w, int h, double batch_delay) {

	int sent_copyrect = 1, obscured = 0;
	int dx, dy;
	int use_batch = 0;
	double ntim = dnow();
	static int nob = -1;
	sraRegionPtr r0, r1;

	if (nob < 0) {
		if (getenv("NOCRBATCH")) {
			nob = 1;
		} else {
			nob = 0;
		}
	}
	if (nob) {
		try_batch = 0;
	}

	dx = x - now_x;
	dy = y - now_y;
	if (dx == 0 && dy == 0) {
		return;
	}
if (ncdb) fprintf(stderr, "do_COPY: now_xy: %d %d, orig_wh: %d %d, xy: %d %d, wh: %d %d\n",now_x, now_y, orig_w, orig_h, x, y, w, h);
	
	ncache_pre_portions(orig_frame, frame, nidx, try_batch, &use_batch,
	    now_x, now_y, orig_w, orig_h, x, y, w, h, ntim);

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y); 
	r1 = sraRgnCreateRect(x, y, x + w, y + h);
	sraRgnAnd(r1, r0);

	dx = x - now_x;
	dy = y - now_y;

	/* make sure the source is on-screen too */
	sraRgnOffset(r1, -dx, -dy);
	sraRgnAnd(r1, r0);
	sraRgnOffset(r1, +dx, +dy);
	sraRgnAnd(r1, r0);	/* just to be sure, problably not needed */

	if (! use_batch) {
		do_copyregion(r1, dx, dy, 0);
		if (!fb_push_wait(0.2, FB_COPY)) {
if (ncdb) fprintf(stderr, "FB_COPY: %.4f 3) *FAILED*\n", dnow() - ntim);
			fb_push_wait(0.1, FB_COPY);
		}
	} else {
		batch_dxs[NPP_nreg] = dx;
		batch_dys[NPP_nreg] = dy;
		batch_reg[NPP_nreg++] = sraRgnCreateRgn(r1);
	}
	sraRgnDestroy(r0);
	sraRgnDestroy(r1);

	if (sent_copyrect) {
		if (use_batch) {
			;
		} else if (! obscured) {
			fb_push_wait(0.1, FB_COPY);
		} else {
			/* no diff for now... */
			fb_push_wait(0.1, FB_COPY);
		}
		ncache_post_portions(*nidx, use_batch,
		    now_x, now_y, orig_w, orig_h, x, y, w, h, batch_delay, ntim);
	}
if (ncdb) fprintf(stderr, "do_COPY: %.4f -- post_portion done.\n", dnow() - ntim);
}

void check_macosx_iconify(Window orig_frame, Window frame, int flush) {
#ifdef MACOSX
	static double last = 0.0;
	double now;
	int j, m = 5, idx = -1, ok = 0, unmapped = 0;

	if (! macosx_console) {
		return;
	}

	now = dnow();
	if (now < last + 0.3) {
		return;
	}
	last = now;

	if (ncache > 0 && orig_frame != None) {
		idx = lookup_win_index(orig_frame);
		if (idx >= 0) {
			if (cache_list[idx].map_state == IsUnmapped) {
if (0) fprintf(stderr, "FAW orig_frame unmapped.\n");
				unmapped = 1;
				m = 3;
			}
		}
	}

	if (unmapped) {
		;
	} else if (orig_frame && macosxCGS_follow_animation_win(orig_frame, -1, 0)) {
		if (0) fprintf(stderr, "FAW orig_frame %d\n", (int) orig_frame);
	} else if (0 && frame && macosxCGS_follow_animation_win(frame, -1, 0)) {
		if (0) fprintf(stderr, "FAW frame      %d\n", (int) frame);
	}
	for (j=0; j<m; j++) {
		macosxCGS_get_all_windows();
		if (macosx_checkevent(NULL)) {
			ok = 1;
			if (0) fprintf(stderr, "Check Event    1\n");
		} else {
			if (0) fprintf(stderr, "Check Event    0\n");
		}
		if (ok) {
			break;
		}
		usleep(10 * 1000);
	}
	if (ok) {
		if (flush) {
			fb_push_wait(0.1, FB_COPY|FB_MOD);
		}
		check_ncache(0, 2);
	}
#else
	if (!orig_frame || !frame || !flush) {}
#endif
}

void check_macosx_click_frame(void) {
#ifdef MACOSX
	if (macosx_console) {
if (0) fprintf(stderr, "macosx_click_frame: 0x%x\n", macosx_click_frame);
		check_macosx_iconify(macosx_click_frame, None, 0);
		macosx_click_frame = None;
		if (button_mask && !macosx_checkevent(NULL)) {
			check_macosx_iconify(None, None, 0);
		}
	}
#endif
}

int clipped(int idx);
void snap_old(void);

int check_copyrect_raise(int idx, Window orig_frame, int try_batch) {
	char *no = "none";
	int doraise = 1;
	int valid;
	XWindowAttributes attr;

	if (! ncache_wf_raises) {
		doraise = 0;
		no = "ncache_wf_raises";
	} else if (cache_list[idx].bs_time == 0.0) {
		doraise = 0;
		no = "bs_time";
	} else if (0 && cache_list[idx].vis_state == VisibilityUnobscured) {
		doraise = 0;
		no = "VisibilityUnobscured";
	} else if (!clipped(idx)) {
		doraise = 0;
		no = "!clipped()";
	}
	if (doraise) {
		int nr = 0, *nb = NULL;
if (ncdb) fprintf(stderr, "--YES, wf_raise\n");
		if (try_batch) {
			nb = &nr;
		}
		valid = 1;
		bs_restore(idx, nb, NULL, &attr, 0, 1, &valid, 1);
		try_to_fix_su(orig_frame, idx, 0x1, nb, NULL);	
		if (nb && nr) {
			batch_push(nr, -1.0);
		}
		fb_push(); /* XXX Y */
	} else {
if (ncdb && no) fprintf(stderr, "--NO,  wf_raise: %s\n", no);
	}
	if (ncache_wf_raises) {
		snapshot_stack_list(0, 0.0);
		snap_old();
	}
	return 1;
}

int set_copyrect_drag(int idx, Window orig_frame, int try_batch) {
	if (idx < 0) {
		return 0;
	}
	if (cache_list[idx].su_time > 0.0) {
		check_copyrect_raise(idx, orig_frame, try_batch);
		return 1;
	}
	return 0;
}

/*
 * Applied just before any check_user_input() modes.  Look for a
 * ButtonPress; find window it happened in; find the wm frame window
 * for it; watch for that window moving or resizing.  If it does, do the
 * wireframe animation.  Do this until ButtonRelease or timeouts occur.
 * Remove wireframe.
 *
 * Under -nowirecopyrect, return control to base scheme
 * (check_user_input() ...) that will repaint the screen with the window
 * in the new postion or size.  Under -wirecopyrect, apply rfbDoCopyRect
 * or rfbDoCopyRegion: this "pollutes" our framebuffer, but the normal
 * polling will quickly repair it. Under happy circumstances, this
 * reduces actual XShmGetImage work (i.e. if we correctly predicted how
 * the X fb has changed.
 *
 * -scale doesn't always work under -wirecopyrect, but the wireframe does.
 *
 * testing of this mode under -threads is incomplete.
 *
 * returns 1 if it did an animation, 0 if no move/resize triggers
 * went off.
 *
 * TBD: see if we can select StructureNotify ConfigureNotify events for
 * the toplevel windows to get better info on moves and resizes.
 */
int check_wireframe(void) {
	Window frame = None, orig_frame = None;
	XWindowAttributes attr;
	int dx, dy;

	int orig_px, orig_py, orig_x, orig_y, orig_w, orig_h;
	int px, py, x, y, w, h;
	int box_x, box_y, box_w, box_h;
	int orig_cursor_x, orig_cursor_y, g, gd;
	int already_down = 0, win_gone = 0, win_unmapped = 0;
	double spin = 0.0, tm, last_ptr = 0.0, last_draw;

	int frame_changed = 0, drew_box = 0, got_2nd_pointer = 0;
	int try_copyrect_drag = 1, do_copyrect_drag = -1;
	int now_x = 0, now_y = 0, nidx = -1;
	double copyrect_drag_delay = -1.0;
	int try_batch = 1;	/* XXX Y */
	int mac_skip = 0;

	int special_t1 = 0, break_reason = 0, last_draw_cnt = 0, gpi = 0;
	static double first_dt_ave = 0.0;
	static int first_dt_cnt = 0;
	static time_t last_save_stacklist = 0;
	int bdown0, bdown, gotui, cnt = 0;
	
	/* heuristics: */
	double first_event_spin   = wireframe_t1;
	double frame_changed_spin = wireframe_t2;
	double max_spin = wireframe_t3;
	double min_draw = wireframe_t4;
	int try_it = 0;
	DB_SET

	if (unixpw_in_progress) return 0;
	if (copyrect_drag_delay) {}

#ifdef MACOSX
	if (macosx_console) {
		;
	} else {
		RAWFB_RET(0)
	}
#else
	RAWFB_RET(0)
#endif

	if (nofb) {
		return 0;
	}
	if (subwin) {
		return 0;	/* don't even bother for -id case */
	}

if (db > 1 && button_mask) fprintf(stderr, "check_wireframe: bm: %d  gpi: %d\n", button_mask, got_pointer_input);

	bdown0 = 0;
	if (button_mask) {
		bdown0 = 1;
	} else if (wireframe_local && display_button_mask) {
		bdown0 = 2;
	}
	if (! bdown0) {
		return 0;	/* no button pressed down */
	}

	gotui = 0;
	if (got_pointer_input) {
		gotui = 1;
	} else if (wireframe_local && display_button_mask) {
		gotui = 2;
	}
	if (!use_threads && !gotui) {
		return 0;	/* need ptr input, e.g. button down, motion */
	}

if (db > 1) fprintf(stderr, "check_wireframe: %d\n", db);

if (db) fprintf(stderr, "\n*** button down!!  x: %d  y: %d\n", cursor_x, cursor_y);

	/*
	 * Query where the pointer is and which child of the root
	 * window.  We will assume this is the frame the window manager
	 * makes when it reparents the toplevel window.
	 */
	X_LOCK;
	if (! get_wm_frame_pos(&px, &py, &x, &y, &w, &h, &frame, NULL)) {
if (db) fprintf(stderr, "NO get_wm_frame_pos-1: 0x%lx\n", frame);
		X_UNLOCK;
#ifdef MACOSX
		check_macosx_click_frame();
#endif
		return 0;
	}
	X_UNLOCK;

	last_get_wm_frame_time = dnow();
	last_get_wm_frame = frame;

if (db) fprintf(stderr, "a: %d  wf: %.3f  A: %d  origfrm: 0x%lx\n", w*h, wireframe_frac, (dpy_x*dpy_y), frame);

	/*
	 * apply the percentage size criterion (allow opaque moves for
	 * small windows)
	 */
	if ((double) w*h < wireframe_frac * (dpy_x * dpy_y)) {
if (db) fprintf(stderr, "small window %.3f\n", ((double) w*h)/(dpy_x * dpy_y));
		return 0;
	}
if (db) fprintf(stderr, "  frame: x: %d  y: %d  w: %d  h: %d  px: %d  py: %d  fr: 0x%lx\n", x, y, w, h, px, py, frame);	

	/*
	 * see if the pointer is within range of the assumed wm frame
	 * decorations on the edge of the window.
	 */

	try_it = near_wm_edge(x, y, w, h, px, py);

	/* Often Alt+ButtonDown starts a window move: */
	if (! try_it && wireframe_mod_state()) {
		try_it = 1;
	}
	if (try_it && clipshift) {
		sraRegionPtr r1, r2;
		int xc = off_x + coff_x;
		int yc = off_y + coff_y;
		r1 = sraRgnCreateRect(x, y, x+w, y+h);
		r2 = sraRgnCreateRect(xc, yc, xc+dpy_x, yc+dpy_y);
		if (!sraRgnAnd(r1, r2)) {
if (db) fprintf(stderr, "OUTSIDE CLIPSHIFT\n");
			try_it = 0;
		}
		sraRgnDestroy(r1);
		sraRgnDestroy(r2);
	}
	if (! try_it) {
if (db) fprintf(stderr, "INTERIOR\n");
#ifdef MACOSX
		check_macosx_click_frame();
#endif
		return 0;
	}

	wireframe_in_progress = 1;

	if (button_mask_prev) {
		already_down = 1;
	}
	
	if (! wireframe_str || !strcmp(wireframe_str, WIREFRAME_PARMS)) {
		int link, latency, netrate;
		static int didmsg = 0;

		link = link_rate(&latency, &netrate);
		if (link == LR_DIALUP || link == LR_BROADBAND) {
			/* slow link, e.g. dialup, increase timeouts: */
			first_event_spin   *= 2.0;
			frame_changed_spin *= 2.0;
			max_spin *= 2.0;
			min_draw *= 1.5;
			if (link == LR_DIALUP) {
				max_spin *= 1.2;
				min_draw *= 1.7;
			}
			if (! didmsg) {
				rfbLog("increased wireframe timeouts for "
				    "slow network connection.\n");
				rfbLog("netrate: %d KB/sec, latency: %d ms\n",
				    netrate, latency);
				didmsg = 1;
			}
		}
	}

	/*
	 * pointer() should have snapped the stacking list for us, if
	 * not, do it now (if the XFakeButtonEvent has been flushed by
	 * now the stacking order may be incorrect).
	 */
	if (strcmp(wireframe_copyrect, "never")) {
		if (already_down) {
			double age = 0.0;
			/*
			 * see if we can reuse the stack list (pause
			 * with button down)
			 */
			if (stack_list_num) {
				int k, got_me = 0;
				for (k = stack_list_num -1; k >=0; k--) {
					if (frame == stack_list[k].win) {
						got_me = 1;
						break;
					}
				}
				if (got_me) {
					age = 1.0;
				}
				snapshot_stack_list(0, age);
			}
		}
		if (! stack_list_num) {
			snapshot_stack_list(0, 0.0);
		}
	}


	/* store initial parameters, we look for changes in them */
	orig_frame = frame;
	orig_px = px;		/* pointer position */
	orig_py = py;
	orig_x = x;		/* frame position */
	orig_y = y;
	orig_w = w;		/* frame size */
	orig_h = h;

	orig_cursor_x = cursor_x;
	orig_cursor_y = cursor_y;

	/* this is the box frame we would draw */
	box_x = x;
	box_y = y; 
	box_w = w;
	box_h = h; 

	dtime0(&tm);

	last_draw = spin;

	/* -threads support for check_wireframe() is rough... crash? */
	if (use_threads) {
		/* purge any stored up pointer events: */
		pointer_event(-1, 0, 0, NULL);
	}

	if (cursor_noshape_updates_clients(screen)) {
		try_batch = 0;
	}
	if (rotating) {
		try_batch = 0;
	}
	if (use_threads && ncache > 0 && ncache_copyrect) {
		try_batch = 0;
	}

	g = got_pointer_input;
	gd = got_local_pointer_input;

	while (1) {

		X_LOCK;
		XFlush_wr(dpy);
		X_UNLOCK;

		/* try to induce/waitfor some more user input */
		if (use_threads) {
			usleep(1000);
		} else if (drew_box && do_copyrect_drag != 1) {
			rfbPE(1000);
		} else {
			rfbCFD(1000);
		}
		if (bdown0 == 2) {
			/*
			 * This is to just update display_button_mask
			 * which will also update got_local_pointer_input.
			 */
			check_x11_pointer();
#if 0
			/* what was this for? */
			Window frame;
			int px, py, x, y, w, h;
#ifdef MACOSX
			if (macosx_console) {
				macosx_get_cursor_pos(&x, &y);
			}
			else
#endif
			get_wm_frame_pos(&px, &py, &x, &y, &w, &h, &frame, NULL);
#endif
		}

		cnt++;
		spin += dtime(&tm);

if (0) fprintf(stderr, "wf-spin: %.3f\n", spin);

		/* check for any timeouts: */
		if (frame_changed) {
			double delay;
			/* max time we play this game: */
			if (spin > max_spin) {
if (db || db2) fprintf(stderr, " SPIN-OUT-MAX: %.3f\n", spin);
				break_reason = 1;
				break;
			}
			/* watch for pointer events slowing down: */
			if (special_t1) {
				delay = max_spin;
			} else {
				delay = 2.0* frame_changed_spin;
				if (spin > 3.0 * frame_changed_spin) {
					delay = 1.5 * delay;
				}
			}
			if (spin > last_ptr + delay) {
if (db || db2) fprintf(stderr, " SPIN-OUT-NOT-FAST: %.3f\n", spin);
				break_reason = 2;
				break;
			}
		} else if (got_2nd_pointer) {
			/*
			 * pointer is moving, max time we wait for wm
			 * move or resize to be detected
			 */
			if (spin > frame_changed_spin) {
if (db || db2) fprintf(stderr, " SPIN-OUT-NOFRAME-SPIN: %.3f\n", spin);
				break_reason = 3;
				break;
			}
		} else {
			/* max time we wait for any pointer input */
			if (spin > first_event_spin) {
if (db || db2) fprintf(stderr, " SPIN-OUT-NO2ND_PTR: %.3f\n", spin);
				break_reason = 4;
				break;
			}
		}

		gpi = 0;
		/* see if some pointer input occurred: */
		if (got_pointer_input > g ||
		    (wireframe_local && (got_local_pointer_input > gd))) {

if (db) fprintf(stderr, "  ++pointer event!! [%02d]  dt: %.3f  x: %d  y: %d  mask: %d\n",
    got_2nd_pointer+1, spin, cursor_x, cursor_y, button_mask);	

			g = got_pointer_input;
			gd = got_local_pointer_input;
			gpi = 1;

			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;

			/* periodically try to let the wm get moving: */
			if (!frame_changed && got_2nd_pointer % 4 == 0) {
				if (got_2nd_pointer == 0) {
					usleep(50 * 1000);
				} else {
					usleep(25 * 1000);
				}
			}
			got_2nd_pointer++;
			last_ptr = spin;

			/*
			 * see where the pointer currently is.  It may
			 * not be our starting frame (i.e. mouse now
			 * outside of the moving window).
			 */
			frame = 0x0;
			X_LOCK;

			if (! get_wm_frame_pos(&px, &py, &x, &y, &w, &h,
			    &frame, NULL)) {
				frame = 0x0;
if (db) fprintf(stderr, "NO get_wm_frame_pos-2: 0x%lx\n", frame);
			}

			if (frame != orig_frame) {
				/* see if our original frame is still there */
				if (!valid_window(orig_frame, &attr, 1)) {
					X_UNLOCK;
					/* our window frame went away! */
					win_gone = 1;
if (db) fprintf(stderr, "FRAME-GONE: 0x%lx\n", orig_frame);
					break_reason = 5;
					break;
				}
				if (attr.map_state == IsUnmapped) {
					X_UNLOCK;
					/* our window frame is now unmapped! */
					win_unmapped = 1;
if (db) fprintf(stderr, "FRAME-UNMAPPED: 0x%lx\n", orig_frame);
					break_reason = 5;
					break;
				}

if (db) fprintf(stderr, "OUT-OF-FRAME: old: x: %d  y: %d  px: %d py: %d 0x%lx\n", x, y, px, py, frame);

				/* new parameters for our frame */
				x = attr.x;	/* n.b. rootwin is parent */
				y = attr.y;
				w = attr.width;
				h = attr.height;
			}
			X_UNLOCK;

if (db) fprintf(stderr, "  frame: x: %d  y: %d  w: %d  h: %d  px: %d  py: %d  fr: 0x%lx\n", x, y, w, h, px, py, frame);	
if (db) fprintf(stderr, "        MO,PT,FR: %d/%d %d/%d %d/%d\n", cursor_x - orig_cursor_x, cursor_y - orig_cursor_y, px - orig_px, py - orig_py, x - orig_x, y - orig_y);	

			if (frame_changed && frame != orig_frame) {
if (db) fprintf(stderr, "CHANGED and window switch: 0x%lx\n", frame);
			}
			if (frame_changed && px - orig_px != x - orig_x) {
if (db) fprintf(stderr, "MOVED and diff DX\n");
			}
			if (frame_changed && py - orig_py != y - orig_y) {
if (db) fprintf(stderr, "MOVED and diff DY\n");
			}

			/* check and see if our frame has been resized: */
			if (!frame_changed && (w != orig_w || h != orig_h)) {
				int n;
				if (!already_down) {
					first_dt_ave += spin;
					first_dt_cnt++;
				}
				n = first_dt_cnt ? first_dt_cnt : 1;
				frame_changed = 2;

if (db) fprintf(stderr, "WIN RESIZE  1st-dt: %.3f\n", first_dt_ave/n);
			}

			/* check and see if our frame has been moved: */
			if (!frame_changed && (x != orig_x || y != orig_y)) {
				int n;
				if (!already_down) {
					first_dt_ave += spin;
					first_dt_cnt++;
				}
				n = first_dt_cnt ? first_dt_cnt : 1;
				frame_changed = 1;
if (db) fprintf(stderr, "FRAME MOVE  1st-dt: %.3f\n", first_dt_ave/n);
			}
		}

		/*
		 * see if it is time to draw any or a new wireframe box
		 */

		if (frame_changed) {
			int drawit = 0;
			if (x != box_x || y != box_y) {
				/* moved since last */
if (0) fprintf(stderr, "DRAW1 %d %d\n", x - box_x, y - box_y);
				drawit = 1;
			} else if (w != box_w || h != box_h) {
				/* resize since last */
				drawit = 1;
			}
			if (drawit) {
				int doit = 0;
				/*
				 * check time (to avoid too much
				 * animations on slow machines
				 * or links).
				 */
				if (gpi) {
					if (spin > last_draw + min_draw || ! drew_box) {
						doit = 1;
					}
					if (macosx_console && doit && !mac_skip) {
						if (x != box_x && y != box_y && w != box_w && h != box_h) {
							doit = 0;
						} else if (!button_mask) {
							doit = 0;
						}
						mac_skip++;
					}
				} else {
					if (drew_box && cnt > last_draw_cnt) 	{
						doit = 1;
if (0) fprintf(stderr, "*** NO GPI DRAW_BOX\n");
					}
				}
		
				if (doit) {
					if (try_copyrect_drag && ncache > 0) {
						if (!ncache_copyrect) {
							do_copyrect_drag = 0;
						} else if (w != box_w || h != box_h) {
							do_copyrect_drag = 0;
						} else if (do_copyrect_drag < 0) {
							Window fr = orig_frame;
							int idx = lookup_win_index(fr);
							if (idx < 0) {
								fr = frame;
								idx = lookup_win_index(fr);
							}
							if (idx >= 0) {
								do_copyrect_drag = set_copyrect_drag(idx, fr, try_batch);
								if (do_copyrect_drag) {
									min_draw *= 0.66;
								}
								nidx = idx;
							} else {
								do_copyrect_drag = 0;
							}
							now_x = orig_x;
							now_y = orig_y;
						}
						if (do_copyrect_drag) {
							if (orig_w != w || orig_h != h) {
								do_copyrect_drag = 0;
							}
						}
					}

					if (do_copyrect_drag <= 0) {
						if (ncache <= 0) {
							;
						} else if (!drew_box && ncache_wf_raises) {
							Window fr = orig_frame;
							int idx = lookup_win_index(fr);
							if (idx < 0) {
								fr = frame;
								idx = lookup_win_index(fr);
							}
							if (idx >= 0) {
								check_copyrect_raise(idx, fr, try_batch);
							}
						}
						draw_box(x, y, w, h, 0);
						fb_push(); /* XXX Y */
						rfbPE(1000);
					} else {
#ifndef NO_NCACHE
						int tb = use_threads ? 0 : try_batch;
						do_copyrect_drag_move(orig_frame, frame, &nidx,
						    tb, now_x, now_y, orig_w, orig_h, x, y, w, h,
						    copyrect_drag_delay);
						now_x = x;
						now_y = y;
						if (copyrect_drag_delay == -1.0) {
							copyrect_drag_delay = 0.04;
						}
#endif
					}
					drew_box = 1;
					last_wireframe = dnow();

					last_draw = spin;
					last_draw_cnt = cnt;
				}
			}
			box_x = x;
			box_y = y;
			box_w = w;
			box_h = h;
		}

		/* 
		 * Now (not earlier) check if the button has come back up.
		 * we check here to get a better location and size of
		 * the final window.
		 */
		bdown = 0;
		if (button_mask) {
			bdown = 1;
		} else if (wireframe_local && display_button_mask) {
			bdown = 2;
		}
		if (! bdown) {
if (db || db2) fprintf(stderr, "NO button_mask\n");
			break_reason = 6;
			break;	
		}
	}

	if (! drew_box) {
		/* nice try, but no move or resize detected.  cleanup. */
		if (stack_list_num) {
			stack_list_num = 0;
		}
		wireframe_in_progress = 0;
		if (macosx_console && (break_reason == 6 || break_reason == 5)) {
			check_macosx_iconify(orig_frame, frame, drew_box);
		}
		return 0;
	}

	/* remove the wireframe */
	if (do_copyrect_drag <= 0) {
		draw_box(0, 0, 0, 0, 1);
		fb_push(); /* XXX Y */
	} else {
		int tb = use_threads ? 0 : try_batch;
		do_copyrect_drag_move(orig_frame, frame, &nidx,
		    tb, now_x, now_y, orig_w, orig_h, x, y, w, h, -1.0);
		fb_push_wait(0.15, FB_COPY|FB_MOD);
	}

	dx = x - orig_x;
	dy = y - orig_y;

	/*
	 * see if we can apply CopyRect or CopyRegion to the change:
	 */
	if (!strcmp(wireframe_copyrect, "never")) {
		;
	} else if (win_gone || win_unmapped) {
		;
	} else if (skip_cr_when_scaling("wireframe")) {
		;
	} else if (w != orig_w || h != orig_h) {
		if (ncache > 0) {
			try_to_fix_resize_su(orig_frame, orig_x, orig_y, orig_w, orig_h, x, y, w, h, try_batch);
			X_LOCK;
			clear_win_events(orig_frame, 1);
			if (frame != orig_frame) {
				clear_win_events(frame, 1);
			}
			X_UNLOCK;
		}
	} else if (dx == 0 && dy == 0) {
		;
	} else if (do_copyrect_drag > 0) {
		X_LOCK;
		clear_win_events(NPP_nwin, 0);
		X_UNLOCK;
	} else {
		int spin_ms = (int) (spin * 1000 * 1000);
		int obscured, sent_copyrect = 0;

		int nidx = -1;
		int use_batch = 0;
		double ntim;

		/*
		 * set a timescale comparable to the spin time,
		 * but not too short or too long.
		 */
		if (spin_ms < 30) {
			spin_ms = 30;
		} else if (spin_ms > 400) {
			spin_ms = 400;
		}
		ntim = dnow();

		/* try to flush the wireframe removal: */
if (ncdb && ncache) fprintf(stderr, "\nSEND_COPYRECT  %.4f %.4f\n", dnowx(), dnow() - ntim);

		if (! fb_push_wait(0.15, FB_COPY|FB_MOD)) {

if (ncdb && ncache) fprintf(stderr, "FB_COPY *FAILED*, try one more... %.4f", dnow() - ntim);

			if (! fb_push_wait(0.15, FB_COPY|FB_MOD)) {

if (ncdb && ncache) fprintf(stderr, "FB_COPY *FAILED* again! %.4f", dnow() - ntim);

			}
		}

		ncache_pre_portions(orig_frame, frame, &nidx, try_batch, &use_batch,
		    orig_x, orig_y, orig_w, orig_h, x, y, w, h, ntim);

		/* 2) try to send a clipped copyrect of translation: */

		if (! try_batch) {
			sent_copyrect = try_copyrect(orig_frame, frame, x, y, w, h, dx, dy,
			    &obscured, NULL, 0.15, NULL);
		} else {
			try_copyrect(orig_frame, frame, x, y, w, h, dx, dy,
			    &obscured, NULL, 0.15, &NPP_nreg);	/* XXX */
			sent_copyrect = 1;
			use_batch = 1;
		}

if ((ncache || db) && ncdb) fprintf(stderr, "sent_copyrect: %d - obs: %d  frame: 0x%lx\n", sent_copyrect, obscured, frame);
		if (sent_copyrect) {
			/* try to push the changes to viewers: */
			if (use_batch) {
				;
			} else if (! obscured) {
				fb_push_wait(0.1, FB_COPY);
			} else {
				/* no diff for now... */
				fb_push_wait(0.1, FB_COPY);
			}
			ncache_post_portions(nidx, use_batch,
			    orig_x, orig_y, orig_w, orig_h, x, y, w, h, -1.0, ntim);
			X_LOCK;
			clear_win_events(NPP_nwin, 0);
			X_UNLOCK;

			if (scaling && !use_batch) {
				static double last_time = 0.0;
				double now = dnow(), delay = 0.35;

				fb_push_wait(0.1, FB_COPY);

				if (now > last_time + delay) {
					int xt = x, yt = y;

					if (clipshift) {
						xt -= coff_x;
						yt -= coff_y;
					}
					if (subwin) {
						xt -= off_x;
						yt -= off_y;
					}

					scale_mark(xt, yt, xt+w, yt+h, 1);
					last_time = now;
					last_copyrect_fix = now;
				}
			}
		}
	}

	if (stack_list_num) {
		/* clean up stack_list for next time: */
		if (break_reason == 1 || break_reason == 2) {
			/*
			 * save the stack list, perhaps the user has
			 * paused with button down.
			 */
			last_save_stacklist = time(NULL);
		} else {
			stack_list_num = 0;
		}
	}

	/* final push (for -nowirecopyrect) */
	rfbPE(1000);
	wireframe_in_progress = 0;

	if (1) {
	/* In principle no longer needed...  see draw_box() */
	    if (frame_changed && cmap8to24 /* && multivis_count */) {
		/* handle -8to24 kludge, mark area and check 8bpp... */
		int x1, x2, y1, y2, f = 16;
		x1 = nmin(box_x, orig_x) - f;
		y1 = nmin(box_y, orig_y) - f;
		x2 = nmax(box_x + box_w, orig_x + orig_w) + f;
		y2 = nmax(box_y + box_h, orig_y + orig_h) + f;
		x1 = nfix(x1, dpy_x);
		x2 = nfix(x2, dpy_x+1);
		y1 = nfix(y1, dpy_y);
		y2 = nfix(y2, dpy_y+1);
		if (0) {
			check_for_multivis();
			mark_rect_as_modified(x1, y1, x2, y2, 0);
		} else {
			if (1) {
				bpp8to24(x1, y1, x2, y2);
			} else {
				bpp8to24(0, 0, dpy_x, dpy_y);
			}
		}
	    }
	}

	urgent_update = 1;
	if (use_xdamage) {
		/* DAMAGE can queue ~1000 rectangles for a move */
		clear_xdamage_mark_region(NULL, 1);
		xdamage_scheduled_mark = dnow() + 2.0;
	}

	if (macosx_console && (break_reason == 6 || break_reason == 5)) {
		check_macosx_iconify(orig_frame, frame, drew_box);
	}

	return 1;
}

/*
 * We need to handle user input, particularly pointer input, carefully.
 * This function is only called when non-threaded.  Note that
 * rfbProcessEvents() only processes *one* pointer event per call,
 * so if we interlace it with scan_for_updates(), we can get swamped
 * with queued up pointer inputs.  And if the pointer inputs are inducing
 * large changes on the screen (e.g. window drags), the whole thing
 * bogs down miserably and only comes back to life at some time after
 * one stops moving the mouse.  So, to first approximation, we are trying
 * to eat as much user input here as we can using some hints from the
 * duration of the previous scan_for_updates() call (in dt).
 *
 * note: we do this even under -nofb
 *
 * return of 1 means watch_loop should short-circuit and reloop,
 * return of 0 means watch_loop should proceed to scan_for_updates().
 * (this is for pointer_mode == 1 mode, the others do it all internally,
 * cnt is also only for that mode).
 */

static void check_user_input2(double dt) {

	int eaten = 0, miss = 0, max_eat = 50, do_flush = 1;
	int g, g_in;
	double spin = 0.0, tm;
	double quick_spin_fac  = 0.40;
	double grind_spin_time = 0.175;

	dtime0(&tm);
	g = g_in = got_pointer_input;
	if (!got_pointer_input) {
		return;
	}
	/*
	 * Try for some "quick" pointer input processing.
	 *
	 * About as fast as we can, we try to process user input calling
	 * rfbProcessEvents or rfbCheckFds.  We do this for a time on
	 * order of the last scan_for_updates() time, dt, but if we stop
	 * getting user input we break out.  We will also break out if
	 * we have processed max_eat inputs.
	 *
	 * Note that rfbCheckFds() does not send any framebuffer updates,
	 * so is more what we want here, although it is likely they have
	 * all be sent already.
	 */
	while (1) {
		if (show_multiple_cursors) {
			rfbPE(1000);
		} else {
			rfbCFD(1000);
		}
		rfbCFD(0);

		spin += dtime(&tm);

		if (spin > quick_spin_fac * dt) {
			/* get out if spin time comparable to last scan time */
			break;
		}
		if (got_pointer_input > g) {
			int i, max_extra = max_eat / 2;
			g = got_pointer_input;
			eaten++;
			for (i=0; i<max_extra; i++)  {
				rfbCFD(0);
				if (got_pointer_input > g) {
					g = got_pointer_input;
					eaten++;
				} else if (i > 1) {
					break;
				}
			}
			X_LOCK;
			do_flush = 0;
if (0) fprintf(stderr, "check_user_input2-A: XFlush %.4f\n", tm);
			XFlush_wr(dpy);
			X_UNLOCK;
			if (eaten < max_eat) {
				continue;
			}
		} else {
			miss++;
		}
		if (miss > 1) {	/* 1 means out on 2nd miss */
			break;
		}
	}
	if (do_flush) {
		X_LOCK;
if (0) fprintf(stderr, "check_user_input2-B: XFlush %.4f\n", tm);
		XFlush_wr(dpy);
		X_UNLOCK;
	}


	/*
	 * Probably grinding with a lot of fb I/O if dt is this large.
	 * (need to do this more elegantly)
	 *
	 * Current idea is to spin our wheels here *not* processing any
	 * fb I/O, but still processing the user input.  This user input
	 * goes to the X display and changes it, but we don't poll it
	 * while we "rest" here for a time on order of dt, the previous
	 * scan_for_updates() time.  We also break out if we miss enough
	 * user input.
	 */
	if (dt > grind_spin_time) {
		int i, ms, split = 30;
		double shim;

		/*
		 * Break up our pause into 'split' steps.  We get at
		 * most one input per step.
		 */
		shim = 0.75 * dt / split;

		ms = (int) (1000 * shim);

		/* cutoff how long the pause can be */
		if (split * ms > 300) {
			ms = 300 / split;
		}

		spin = 0.0;
		dtime0(&tm);

		g = got_pointer_input;
		miss = 0;
		for (i=0; i<split; i++) {
			usleep(ms * 1000);
			if (show_multiple_cursors) {
				rfbPE(1000);
			} else {
				rfbCFD(1000);
			}
			spin += dtime(&tm);
			if (got_pointer_input > g) {
				int i, max_extra = max_eat / 2;
				for (i=0; i<max_extra; i++)  {
					rfbCFD(0);
					if (got_pointer_input > g) {
						g = got_pointer_input;
					} else if (i > 1) {
						break;
					}
				}
				X_LOCK;
if (0) fprintf(stderr, "check_user_input2-C: XFlush %.4f\n", tm);
				XFlush_wr(dpy);
				X_UNLOCK;
				miss = 0;
			} else {
				miss++;
			}
			g = got_pointer_input;
			if (miss > 2) {
				break;
			}
			if (1000 * spin > ms * split)  {
				break;
			}
		}
	}
}

static void check_user_input3(double dt, double dtr, int tile_diffs) {

	int allowed_misses, miss_tweak, i, g, g_in;
	int last_was_miss, consecutive_misses;
	double spin, spin_max, tm, to, dtm;
	int rfb_wait_ms = 2;
	static double dt_cut = 0.075;
	int gcnt, ginput;
	static int first = 1;

	if (dtr || tile_diffs) {} /* unused vars warning: */

	if (first) {
		char *p = getenv("SPIN");
		if (p) {
			double junk;
			sscanf(p, "%lf,%lf", &dt_cut, &junk);
		}
		first = 0;
	}

	if (!got_pointer_input) {
		return;
	}


	if (dt < dt_cut) {
		dt = dt_cut;	/* this is to try to avoid early exit */
	}
	spin_max = 0.5;

	spin = 0.0;		/* amount of time spinning */
	allowed_misses = 10;	/* number of ptr inputs we can miss */
	miss_tweak = 8;
	last_was_miss = 0;
	consecutive_misses = 1;
	gcnt = 0;
	ginput = 0;

	dtime0(&tm);
	to = tm;	/* last time we did rfbPE() */

	g = g_in = got_pointer_input;

	while (1) {
		int got_input = 0;

		gcnt++;

		if (button_mask) {
			drag_in_progress = 1;
		}

		rfbCFD(rfb_wait_ms * 1000);

		dtm = dtime(&tm);
		spin += dtm;

		if (got_pointer_input == g) {
			if (last_was_miss) {
				consecutive_misses++;
			}
			last_was_miss = 1;
		} else {
			ginput++;
			if (ginput % miss_tweak == 0) {
				allowed_misses++;
			}
			consecutive_misses = 1;
			last_was_miss = 0;
		}

		if (spin > spin_max) {
			/* get out if spin time over limit */
			break;

		} else if (got_pointer_input > g) {
			/* received some input, flush to display. */
			got_input = 1;
			g = got_pointer_input;
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
		} else if (--allowed_misses <= 0) {
			/* too many misses */
			break;
		} else if (consecutive_misses >=3) {
			/* too many misses */
			break;
		} else {
			/* these are misses */
			int wms = 0;
			if (gcnt == 1 && button_mask) {
				/*
				 * missed our first input, wait
				 * for a defer time. (e.g. on
				 * slow link) hopefully client
				 * will batch them.
				 */
				wms = 50;
			} else if (button_mask) {
				wms = 10;
			} else {
			}
			if (wms) {
				usleep(wms * 1000);
			}
		}
	}

	if (ginput >= 2) {
		/* try for a couple more quick ones */
		for (i=0; i<2; i++) {
			rfbCFD(rfb_wait_ms * 1000);
		}
	}

	drag_in_progress = 0;
}

int fb_update_sent(int *count) {
	static int last_count = 0;
	int sent = 0, rc = 0;
	rfbClientIteratorPtr i;
	rfbClientPtr cl;

	if (nofb) {
		return 0;
	}

	i = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(i)) ) {
#if 0
		sent += cl->framebufferUpdateMessagesSent;
#else
#if LIBVNCSERVER_HAS_STATS
		sent += rfbStatGetMessageCountSent(cl, rfbFramebufferUpdate);
#endif
#endif
	}
	rfbReleaseClientIterator(i);
	if (sent != last_count) {
		rc = 1;
	}
	if (count != NULL) {
		*count = sent;
	}
	last_count = sent;
	return rc; 
}

static void check_user_input4(double dt, double dtr, int tile_diffs) {

	int g, g_in, i, ginput, gcnt, tmp;
	int last_was_miss, consecutive_misses;
	int min_frame_size = 10;	/* 10 tiles */
	double spin, tm, to, tc, dtm, rpe_last;
	int rfb_wait_ms = 2;
	static double dt_cut = 0.050;
	static int first = 1;

	int Btile = tile_x * tile_y * bpp/8; 	/* Bytes per tile */
	double Ttile, dt_use;
	double screen_rate = 6000000.;    /* 5 MB/sec */
	double vnccpu_rate = 80 * 100000.; /* 20 KB/sec @ 80X compression */
	double net_rate = 50000.;
	static double Tfac_r = 1.0, Tfac_v = 1.0, Tfac_n = 1.0, Tdelay = 0.001;
	static double dt_min = -1.0, dt_max = -1.0;
	double dt_min_fallback = 0.050;
	static int ssec = 0, total_calls = 0;
	static int push_frame = 0, update_count = 0;

	if (first) {
		char *p = getenv("SPIN");
		if (p) {
			sscanf(p, "%lf,%lf,%lf,%lf", &dt_cut, &Tfac_r, &Tfac_v, &Tfac_n);
		}
		first = 0;
		ssec = time(NULL);

		if (dtr) {}	/* unused vars warning: */
	}

	total_calls++;

	if (dt_min < 0.0 || dt < dt_min) {
		if (dt > 0.0) {
			dt_min = dt;
		}
	}
	if (dt_min < 0.0) {
		/* sensible value for the very 1st call if dt = 0.0 */
		dt_min = dt_min_fallback;
	}
	if (dt_max < 0.0 || dt > dt_max) {
		dt_max = dt;
	}

	if (total_calls > 30 && dt_min > 0.0) {
		static int first = 1;
		/*
		 * dt_min will soon be the quickest time to do
		 * one scan_for_updates with no tiles copied.
		 * use this (instead of copy_tiles) to estimate
		 * screen read rate.
		 */
		screen_rate = (main_bytes_per_line * ntiles_y) / dt_min;
		if (first) {
			rfbLog("measured screen read rate: %.2f Bytes/sec\n",
			    screen_rate);
		}
		first = 0;
	}

	dtime0(&tm);

	if (dt < dt_cut) {
		dt_use = dt_cut;
	} else {
		dt_use = dt;
	}

	if (push_frame) {
		int cnt, iter = 0;
		double tp, push_spin = 0.0;
		dtime0(&tp);
		while (push_spin < dt_use * 0.5) {
			fb_update_sent(&cnt);
			if (cnt != update_count) {
				break;
			}
			/* damn, they didn't push our frame! */
			iter++;
			rfbPE(rfb_wait_ms * 1000);
			
			push_spin += dtime(&tp);
		}
		if (iter) {
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
		}
		push_frame = 0;
		update_count = 0;
	}

	/*
	 * when we first enter we require some pointer input
	 */
	if (!got_pointer_input) {
		return;
	}

	vnccpu_rate = get_raw_rate();

	if ((tmp = get_read_rate()) != 0) {
		screen_rate = (double) tmp;
	}
	if ((tmp = get_net_rate()) != 0) {
		net_rate = (double) tmp;
	}
	net_rate = (vnccpu_rate/get_cmp_rate()) * net_rate;

	if ((tmp = get_net_latency()) != 0) {
		Tdelay = 0.5 * ((double) tmp)/1000.;
	}

	Ttile = Btile * (Tfac_r/screen_rate + Tfac_v/vnccpu_rate + Tfac_n/net_rate);

	spin = 0.0;		/* amount of time spinning */
	last_was_miss = 0;
	consecutive_misses = 1;
	gcnt = 0;
	ginput = 0;

	rpe_last = to = tc = tm;	/* last time we did rfbPE() */
	g = g_in = got_pointer_input;

	tile_diffs = 0;	/* reset our knowlegde of tile_diffs to zero */

	while (1) {
		int got_input = 0;

		gcnt++;

		if (button_mask) {
			/* this varible is used by our pointer handler */
			drag_in_progress = 1;
		}

		/* turn libvncserver crank to process events: */
		rfbCFD(rfb_wait_ms * 1000);

		dtm = dtime(&tm);
		spin += dtm;

		if ( (gcnt == 1 && got_pointer_input > g) || tm-tc > 2*dt_min) {
			tile_diffs = scan_for_updates(1);
			tc = tm;
		}

		if (got_pointer_input == g) {
			if (last_was_miss) {
				consecutive_misses++;
			}
			last_was_miss = 1;
		} else {
			ginput++;
			consecutive_misses = 1;
			last_was_miss = 0;
		}

		if (tile_diffs > min_frame_size && spin > Ttile * tile_diffs + Tdelay) {
			/* we think we can push the frame */
			push_frame = 1;
			fb_update_sent(&update_count);
			break;

		} else if (got_pointer_input > g) {
			/* received some input, flush it to display. */
			got_input = 1;
			g = got_pointer_input;
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;

		} else if (consecutive_misses >= 2) {
			/* too many misses in a row */
			break;

		} else {
			/* these are pointer input misses */
			int wms;
			if (gcnt == 1 && button_mask) {
				/*
				 * missed our first input, wait for
				 * a defer time. (e.g. on slow link)
				 * hopefully client will batch many
				 * of them for the next read.
				 */
				wms = 50;

			} else if (button_mask) {
				wms = 10;
			} else {
				wms = 0;
			}
			if (wms) {
				usleep(wms * 1000);
			}
		}
	}
	if (ginput >= 2) {
		/* try for a couple more quick ones */
		for (i=0; i<2; i++) {
			rfbCFD(rfb_wait_ms * 1000);
		}
	}
	drag_in_progress = 0;
}

int check_user_input(double dt, double dtr, int tile_diffs, int *cnt) {

	if (rawfb_vnc_reflect) {
		if (got_user_input) {
			if (0) vnc_reflect_process_client();
		}
		if (got_user_input && *cnt % ui_skip != 0) {
			/* every n-th drops thru to scan */
			*cnt = *cnt + 1;
			return 1;	/* short circuit watch_loop */
		}
	}
#ifdef MACOSX
	if (! macosx_console) {
		RAWFB_RET(0)
	}
#else
	RAWFB_RET(0)
#endif

	if (use_xrecord) {
		int rc = check_xrecord();
		/*
		 * 0: nothing found, proceed to other user input schemes.
		 * 1: events found, want to do a screen update now.
		 * 2: events found, want to loop back for some more.
		 * 3: events found, want to loop back for some more,
		 *    and not have rfbPE() called.
		 *
		 * For 0, we precede below, otherwise return rc-1.
		 */
if (debug_scroll && rc > 1) fprintf(stderr, "  CXR: check_user_input ret %d\n", rc - 1);
		if (rc == 0) {
			;	/* proceed below. */
		} else {
			return rc - 1;
		}
	}

	if (wireframe) {
		if (check_wireframe()) {
			return 0;
		}
	}

	if (pointer_mode == 1) {
		if ((got_user_input || ui_skip < 0) && *cnt % ui_skip != 0) {
			/* every ui_skip-th drops thru to scan */
			*cnt = *cnt + 1;
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
			return 1;	/* short circuit watch_loop */
		} else {
			return 0;
		}
	}
	if (pointer_mode >= 2 && pointer_mode <= 4) {
		if (got_keyboard_input) {
			/*
			 * for these modes, short circuit watch_loop on
			 * *keyboard* input.
			 */
			if (*cnt % ui_skip != 0) {
				*cnt = *cnt + 1; 
				return 1;
			}
		}
		/* otherwise continue below with pointer input method */
	}

	if (pointer_mode == 2) {
		check_user_input2(dt);
	} else if (pointer_mode == 3) {
		check_user_input3(dt, dtr, tile_diffs);
	} else if (pointer_mode == 4) {
		check_user_input4(dt, dtr, tile_diffs);
	}
	return 0;
}

#if defined(NO_NCACHE) || (NO_X11 && !defined(MACOSX))
int check_ncache(int a, int b) {
	if (!a || !b) {}
	ncache = 0;
	return 0;
}
int lookup_win_index(Window win) {
	if (!win) {}
	return -1;
}
int find_rect(int idx, int x, int y, int w, int h) {
	if (!idx || !x || !y || !w || !h) {}
	return 0;
}
void snap_old(void) {
	return;
}
int clipped(int idx) {
	if (!idx) {}
	return 0;
}
int bs_restore(int idx, int *nbatch, sraRegionPtr rmask, XWindowAttributes *attr, int clip, int nopad, int *valid, int verb) {
	if (!idx || !nbatch || !rmask || !attr || !clip || !nopad || !valid || !verb) {}
	return 0;
}
int try_to_fix_su(Window win, int idx, Window above, int *nbatch, char *mode) {
	if (!win || !idx || !above || !nbatch || !mode) {}
	return 0;
}
int try_to_fix_resize_su(Window orig_frame, int orig_x, int orig_y, int orig_w, int orig_h,
    int x, int y, int w, int h, int try_batch) {
	if (!orig_frame || !orig_x || !orig_y || !orig_w || !orig_h || !x || !y || !w || !h || !try_batch) {}
	return 0;
}
void set_ncache_xrootpmap(void) {
	return;
}
#else
/* maybe ncache.c it if works */

winattr_t* cache_list = NULL;
int cache_list_num = 0;
int cache_list_len = 0;

void snapshot_cache_list(int free_only, double allowed_age) {
	static double last_snap = 0.0, last_free = 0.0;
	double now; 
	int num, rc, i;
	unsigned int ui;
	Window r, w;
	Window *list;
	int start = 512;

	if (! cache_list) {
		cache_list = (winattr_t *) calloc(start*sizeof(winattr_t), 1);
		cache_list_num = 0;
		cache_list_len = start;
	}

	dtime0(&now);
	if (free_only) {
		/* we really don't free it, just reset to zero windows */
		cache_list_num = 0;
		last_free = now;
		return;
	}

	if (cache_list_num && now < last_snap + allowed_age) {
		return;
	}

	cache_list_num = 0;
	last_free = now;

#ifdef MACOSX
	if (! macosx_console) {
		RAWFB_RET_VOID
	}
#else
	RAWFB_RET_VOID
#endif


#if NO_X11 && !defined(MACOSX)
	num = rc = i = 0;	/* compiler warnings */
	ui = 0;
	r = w = None;
	list = NULL;
	return;
#else

	X_LOCK;
	/* no need to trap error since rootwin */
	rc = XQueryTree_wr(dpy, rootwin, &r, &w, &list, &ui);
	X_UNLOCK;
	num = (int) ui;

	if (! rc) {
		cache_list_num = 0;
		last_free = now;
		last_snap = 0.0;
		return;
	}

	last_snap = now;
	if (num > cache_list_len) {
		int n = 2*num;
		n = num + 3;
		free(cache_list);
		cache_list = (winattr_t *) calloc(n*sizeof(winattr_t), 1);
		cache_list_len = n;
	}
	for (i=0; i<num; i++) {
		cache_list[i].win = list[i];
		cache_list[i].fetched = 0;
		cache_list[i].valid = 0;
		cache_list[i].time = now;
		cache_list[i].selectinput = 0;
		cache_list[i].vis_cnt = 0;
		cache_list[i].map_cnt = 0;
		cache_list[i].unmap_cnt = 0;
		cache_list[i].create_cnt = 0;
		cache_list[i].vis_state = -1;
		cache_list[i].above = None;
	}
	if (num == 0) {
		cache_list[0].win = None;
		cache_list[0].fetched = 0;
		cache_list[0].valid = 0;
		cache_list[0].time = now;
		cache_list[0].selectinput = 0;
		cache_list[0].vis_cnt = 0;
		cache_list[0].map_cnt = 0;
		cache_list[0].unmap_cnt = 0;
		cache_list[0].create_cnt = 0;
		cache_list[0].vis_state = -1;
		cache_list[0].above = None;
		num++;
	}

	cache_list_num = num;

	if (num) {
		X_LOCK;
		XFree_wr(list);
		X_UNLOCK;
	}
#endif	/* NO_X11 */
}

void quick_snap(Window *wins, int *size) {
	int num, rc, i;
	unsigned int ui;
	Window r, w;
	Window *list;

#ifdef MACOSX
	if (1 || ! macosx_console) {
		RAWFB_RET_VOID
	}
#else
	RAWFB_RET_VOID
#endif


#if NO_X11 && !defined(MACOSX)
	num = rc = i = 0;	/* compiler warnings */
	ui = 0;
	r = w = None;
	list = NULL;
	return;
#else

	X_LOCK;
	/* no need to trap error since rootwin */
	rc = XQueryTree_wr(dpy, rootwin, &r, &w, &list, &ui);
	X_UNLOCK;
	num = (int) ui;

	if (! rc || num == 0) {
		*size = 0;
		return;
	} else {
		int m = *size;
		if (num < m) {
			m = num;
		}
		for (i=0; i < m; i++) {
			wins[i] = list[i];
		}
		if (num) {
			X_LOCK;
			XFree_wr(list);
			X_UNLOCK;
		}
		*size = m;
	}
#endif	/* NO_X11 */
}

int get_bs_n(int y) {
	int n;
	for (n = 1; n < ncache; n += 2) {
		if (n*dpy_y <= y && y < (n+1)*dpy_y) {
			return n;
		}
	}
	return -1;
}

#define NRECENT 32
Window recent[NRECENT];
int    recidx[NRECENT];
int rlast, rfree;

int lookup_win_index(Window win) {
	int k, idx = -1;
	int foundfree = 0;
	static int s1 = 0, s2 = 0, s3 = 0;

	if (win == rootwin || win == None) {
		return -1;
	}
	for (k = 0; k < NRECENT; k++) {
		if (recent[k] == win) {
			int k2 = recidx[k];
			if (cache_list[k2].win == win) {
				idx = k2;
if (0) fprintf(stderr, "recentA(shortcut): %d  0x%lx\n", idx, win);
				s1++;
				break;
			}
		}
	}
	if (idx < 0) {
		for(k=0; k<cache_list_num; k++) {
			if (!foundfree && cache_list[k].win == None) {
				rfree = k;
				foundfree = 1;
			}
			if (cache_list[k].win == win) {
				idx = k;
if (0) fprintf(stderr, "recentB(normal): %d  0x%lx\n", idx, win);
				s2++;
				break;
			}
		}
		if (idx >= 0) {
			recent[rlast] = win;
			recidx[rlast++] = idx;
			rlast = rlast % NRECENT;
		}
	}
	if (idx < 0) {
if (ncdb) fprintf(stderr, "recentC(fail): %d  0x%lx\n", idx, win);
		s3++;
	}
	if (s1 + s2 + s3 >= 1000) {
if (ncdb) fprintf(stderr, "lookup_win_index recent hit stats: %d/%d/%d\n", s1, s2, s3);
		s1 = s2 = s3 = 0;
	}
	return idx;
}

int lookup_free_index(void) {
	int k;

	if (rfree >= 0) {
		if (cache_list[rfree].win == None) {
if (ncdb) fprintf(stderr, "lookup_freeA: %d\n", rfree);
			return rfree;
		}
	}
	rfree = -1;
	for(k=0; k<cache_list_num; k++) {
		if (cache_list[k].win == None) {
			rfree = k;
			break;
		}
	}
	if (rfree < 0) {
		if (ncdb) fprintf(stderr, "*** LOOKUP_FREE_INDEX: incrementing cache_list_num %d/%d\n", cache_list_num, cache_list_len);

		rfree = cache_list_num++;
		if (rfree >= cache_list_len)  {
			int i, n = 2*cache_list_len;
			winattr_t *cache_new;

			if (ncdb) fprintf(stderr, "lookup_free_index: growing cache_list_len: %d -> %d\n", cache_list_len, n);

			cache_new = (winattr_t *) calloc(n*sizeof(winattr_t), 1);
			for (i=0; i<cache_list_num-1; i++) {
				cache_new[i] = cache_list[i]; 
			}
			cache_list_len = n;
			free(cache_list);
			cache_list = cache_new;
		}
		cache_list[rfree].win = None;
		cache_list[rfree].fetched = 0;
		cache_list[rfree].valid = 0;
		cache_list[rfree].time = 0.0;
		cache_list[rfree].selectinput = 0;
		cache_list[rfree].vis_cnt = 0;
		cache_list[rfree].map_cnt = 0;
		cache_list[rfree].unmap_cnt = 0;
		cache_list[rfree].create_cnt = 0;
		cache_list[rfree].vis_state = -1;
		cache_list[rfree].above = None;
	}

if (ncdb) fprintf(stderr, "lookup_freeB: %d\n", rfree);
	return rfree;
}

#define STACKMAX 4096
Window old_stack[STACKMAX];
Window new_stack[STACKMAX];
Window old_stack_map[STACKMAX];
Window new_stack_map[STACKMAX];
int old_stack_index[STACKMAX];
int old_stack_mapped[STACKMAX];
int old_stack_n = 0;
int new_stack_n = 0;
int old_stack_map_n = 0;
int new_stack_map_n = 0;

void snap_old(void) {
	int i;
	old_stack_n = STACKMAX;
	quick_snap(old_stack, &old_stack_n);
if (0) fprintf(stderr, "snap_old: %d  %.4f\n", old_stack_n, dnowx());
#if 0
	for (i= old_stack_n - 1; i >= 0; i--) {
		int idx = lookup_win_index(old_stack[i]);
		if (idx >= 0) {
			if (cache_list[idx].map_state == IsViewable) {
				if (ncdb) fprintf(stderr, "   %03d  0x%x\n", i, old_stack[i]);
			}
		}
	}
#endif
	for (i=0; i < old_stack_n; i++) {
		old_stack_mapped[i] = -1;
	}
}

void snap_old_index(void) {
	int i, idx;
	for (i=0; i < old_stack_n; i++) {
		idx = lookup_win_index(old_stack[i]);
		old_stack_index[i] = idx;
		if (idx >= 0) {
			if (cache_list[idx].map_state == IsViewable) {
				old_stack_mapped[i] = 1;
			} else {
				old_stack_mapped[i] = 0;
			}
		}
	}
}

int lookup_old_stack_index(int ic) {
	int idx = old_stack_index[ic];

	if (idx < 0) {
		return -1;
	}
	if (cache_list[idx].win != old_stack[ic]) {
		snap_old_index();
	}
	idx = old_stack_index[ic];
	if (idx < 0 || cache_list[idx].win != old_stack[ic]) {
		return -1;
	}
	if (cache_list[idx].map_state == IsViewable) {
		old_stack_mapped[ic] = 1;
	} else {
		old_stack_mapped[ic] = 0;
	}
	return idx;
}

#define STORE(k, w, attr) \
	if (0) fprintf(stderr, "STORE(%d) = 0x%lx\n", k, w); \
	cache_list[k].win = w;  \
	cache_list[k].fetched = 1;  \
	cache_list[k].valid = 1;  \
	cache_list[k].x = attr.x;  \
	cache_list[k].y = attr.y;  \
	cache_list[k].width = attr.width;  \
	cache_list[k].height = attr.height;  \
	cache_list[k].border_width = attr.border_width;  \
	cache_list[k].map_state = attr.map_state; \
	cache_list[k].time = dnow();

#if 0
	cache_list[k].width = attr.width   + 2*attr.border_width;  \
	cache_list[k].height = attr.height + 2*attr.border_width;  \

#endif

#define CLEAR(k) \
	if (0) fprintf(stderr, "CLEAR(%d)\n", k); \
	cache_list[k].bs_x = -1;  \
	cache_list[k].bs_y = -1;  \
	cache_list[k].bs_w = -1;  \
	cache_list[k].bs_h = -1;  \
	cache_list[k].su_x = -1;  \
	cache_list[k].su_y = -1;  \
	cache_list[k].su_w = -1;  \
	cache_list[k].su_h = -1;  \
	cache_list[k].time = 0.0;  \
	cache_list[k].bs_time = 0.0;  \
	cache_list[k].su_time = 0.0;  \
	cache_list[k].vis_obs_time = 0.0;  \
	cache_list[k].vis_unobs_time = 0.0;

#define DELETE(k) \
	if (0) fprintf(stderr, "DELETE(%d) = 0x%lx\n", k, cache_list[k].win); \
	cache_list[k].win = None;  \
	cache_list[k].fetched = 0;  \
	cache_list[k].valid = 0; \
	cache_list[k].selectinput = 0; \
	cache_list[k].vis_cnt = 0; \
	cache_list[k].map_cnt = 0; \
	cache_list[k].unmap_cnt = 0; \
	cache_list[k].create_cnt = 0; \
	cache_list[k].vis_state = -1; \
	cache_list[k].above = None; \
	free_rect(k);	/* does CLEAR(k) */

static	char unk[32];

char *Etype(int type) {
	if (type == KeyPress)		return "KeyPress";
	if (type == KeyRelease)		return "KeyRelease";
	if (type == ButtonPress)	return "ButtonPress";
	if (type == ButtonRelease)	return "ButtonRelease";
	if (type == MotionNotify)	return "MotionNotify";
	if (type == EnterNotify)	return "EnterNotify";
	if (type == LeaveNotify)	return "LeaveNotify";
	if (type == FocusIn)		return "FocusIn";
	if (type == FocusOut)		return "FocusOut";
	if (type == KeymapNotify)	return "KeymapNotify";
	if (type == Expose)		return "Expose";
	if (type == GraphicsExpose)	return "GraphicsExpose";
	if (type == NoExpose)		return "NoExpose";
	if (type == VisibilityNotify)	return "VisibilityNotify";
	if (type == CreateNotify)	return "CreateNotify";
	if (type == DestroyNotify)	return "DestroyNotify";
	if (type == UnmapNotify)	return "UnmapNotify";
	if (type == MapNotify)		return "MapNotify";
	if (type == MapRequest)		return "MapRequest";
	if (type == ReparentNotify)	return "ReparentNotify";
	if (type == ConfigureNotify)	return "ConfigureNotify";
	if (type == ConfigureRequest)	return "ConfigureRequest";
	if (type == GravityNotify)	return "GravityNotify";
	if (type == ResizeRequest)	return "ResizeRequest";
	if (type == CirculateNotify)	return "CirculateNotify";
	if (type == CirculateRequest)	return "CirculateRequest";
	if (type == PropertyNotify)	return "PropertyNotify";
	if (type == SelectionClear)	return "SelectionClear";
	if (type == SelectionRequest)	return "SelectionRequest";
	if (type == SelectionNotify)	return "SelectionNotify";
	if (type == ColormapNotify)	return "ColormapNotify";
	if (type == ClientMessage)	return "ClientMessage";
	if (type == MappingNotify)	return "MappingNotify";
	if (type == LASTEvent)		return "LASTEvent";
	sprintf(unk, "Unknown %d", type);
	return unk;
}
char *VState(int state) {
	if (state == VisibilityFullyObscured)		return "VisibilityFullyObscured";
	if (state == VisibilityPartiallyObscured)	return "VisibilityPartiallyObscured";
	if (state == VisibilityUnobscured)		return "VisibilityUnobscured";
	sprintf(unk, "Unknown %d", state);
	return unk;
}
char *MState(int state) {
	if (state == IsViewable)	return "IsViewable";
	if (state == IsUnmapped)	return "IsUnmapped";
	sprintf(unk, "Unknown %d", state);
	return unk;
}
sraRegionPtr rect_reg[64];
sraRegionPtr zero_rects = NULL;

int free_rect(int idx) {
	int n, ok = 0;
	sraRegionPtr r1, r2;
	int x, y, w, h;

	if (idx < 0 || idx >= cache_list_num) {
if (0) fprintf(stderr, "free_rect: bad index: %d\n", idx);
		clean_up_exit(1);
	}

	x = cache_list[idx].bs_x;
	y = cache_list[idx].bs_y;
	w = cache_list[idx].bs_w;
	h = cache_list[idx].bs_h;

	if (x < 0) {
		CLEAR(idx);
if (dnow() > last_client + 5 && ncdb) fprintf(stderr, "free_rect: already bs_x invalidated: %d bs_x: %d\n", idx, x);
		return 1;
	}

	r2 = sraRgnCreateRect(x, y, x+w, y+h);

	n = get_bs_n(y);
	if (n >= 0) {
		r1 = rect_reg[n];
		sraRgnOr(r1, r2);
		ok = 1;
	}

	if (zero_rects) {
		sraRgnOr(zero_rects, r2);
		x = cache_list[idx].su_x;
		y = cache_list[idx].su_y;
		w = cache_list[idx].su_w;
		h = cache_list[idx].su_h;
		if (x >= 0) {
			sraRgnDestroy(r2);
			r2 = sraRgnCreateRect(x, y, x+w, y+h);
			sraRgnOr(zero_rects, r2);
		}
	}
	sraRgnDestroy(r2);

	CLEAR(idx);
if (! ok && ncdb) fprintf(stderr, "**** free_rect: not-found %d\n", idx);
	return ok;
}

int fr_BIG1 = 0;
int fr_BIG2 = 0;
int fr_REGION = 0;
int fr_GRID = 0;
int fr_EXPIRE = 0;
int fr_FORCE = 0;
int fr_FAIL = 0;
int fr_BIG1t = 0;
int fr_BIG2t = 0;
int fr_REGIONt = 0;
int fr_GRIDt = 0;
int fr_EXPIREt = 0;
int fr_FORCEt = 0;
int fr_FAILt = 0;

void expire_rects1(int idx, int w, int h, int *x_hit, int *y_hit, int big1, int big2, int cram) {
	sraRegionPtr r1, r2, r3;
	int x = -1, y = -1, n;

	if (*x_hit < 0) {
		int i, k, old[10], N = 4;
		double dold[10], fa, d, d1, d2, d3;
		int a0 = w * h, a1;

		for (k=1; k<=N; k++) {
			old[k] = -1;
			dold[k] = -1.0;
		}
		for (i=0; i<cache_list_num; i++) {
			int wb = cache_list[i].bs_w;
			int hb = cache_list[i].bs_h;
			if (cache_list[i].bs_x < 0) {
				continue;
			}
			if (w > wb || h > hb) {
				continue;
			}
			if (wb == 0 || hb == 0) {
				continue;
			}
			if (a0 == 0) {
				continue;
			}
			if (i == idx) {
				continue;
			}
			a1 = wb * hb;
			fa = ((double) a1) / a0;
			k = (int) fa;

			if (k < 1) k = 1;
			if (k > N) continue;

			d1 = cache_list[i].time;
			d2 = cache_list[i].bs_time;
			d3 = cache_list[i].su_time;

			d = d1;
			if (d2 > d) d = d2;
			if (d3 > d) d = d3;

			if (dold[k] == -1.0 || d < dold[k]) {
				old[k] = i;
				dold[k] = d;
			}
		}

		for (k=1; k<=N; k++) {
			if (old[k] >= 0) {
				int ik = old[k];
				int k_x = cache_list[ik].bs_x;
				int k_y = cache_list[ik].bs_y;
				int k_w = cache_list[ik].bs_w;
				int k_h = cache_list[ik].bs_h;

if (ncdb) fprintf(stderr, ">>**--**>> found rect via EXPIRE: %d 0x%lx -- %dx%d+%d+%d %d %d --  %dx%d+%d+%d  A: %d/%d\n",
    ik, cache_list[ik].win, w, h, x, y, *x_hit, *y_hit, k_w, k_h, k_x, k_y, k_w * k_h, w * h);

				free_rect(ik);
				fr_EXPIRE++;
				fr_EXPIREt++;
				*x_hit = k_x;
				*y_hit = k_y;
				n = get_bs_n(*y_hit);
				if (n >= 0) {
					r1 = rect_reg[n];
					r2 = sraRgnCreateRect(*x_hit, *y_hit, *x_hit + w, *y_hit + h);
					sraRgnSubtract(r1, r2);
					sraRgnDestroy(r2);
				} else {
					fprintf(stderr, "failure to find y n in find_rect\n");
					clean_up_exit(1);
				}
				break;
			}
		}
	}

	/* next, force ourselves into some corner, expiring many */
	if (*x_hit < 0) {
		int corner_x = (int) (2 * rfac());
		int corner_y = (int) (2 * rfac());
		int x0 = 0, y0 = 0, i, nrand, nr = ncache/2;
		if (nr == 1) {
			nrand = 1;
		} else {
			if (! big1) {
				nrand = 1;
			} else {
				if (big2 && nr > 2) {
					nrand =  1 + (int) ((nr - 2) * rfac());
					nrand += 2; 
				} else {
					nrand =  1 + (int) ((nr - 1) * rfac());
					nrand += 1; 
				}
			}
		}
		if (nrand < 0 || nrand > nr) {
			nrand = nr;
		}
		if (cram && big1) {
			corner_x = 1;
		}

		y0 += dpy_y; 
		if (nrand > 1) {
			y0 += 2 * (nrand - 1) * dpy_y; 
		}
		if (corner_y) {
			y0 += dpy_y - h; 
		}
		if (corner_x) {
			x0 += dpy_x - w; 
		}
		r1 = sraRgnCreateRect(x0, y0, x0+w, y0+h);

		for (i=0; i<cache_list_num; i++) {
			int xb = cache_list[i].bs_x;
			int yb = cache_list[i].bs_y;
			int wb = cache_list[i].bs_w;
			int hb = cache_list[i].bs_h;
			if (xb < 0) {
				continue;
			}
			if (nabs(yb - y0) > dpy_y) {
				continue;
			}
			r2 = sraRgnCreateRect(xb, yb, xb+wb, yb+hb);
			if (sraRgnAnd(r2, r1)) {
				free_rect(i);
			}
			sraRgnDestroy(r2);
		}
		*x_hit = x0;
		*y_hit = y0;
		r3 = rect_reg[2*nrand-1];
		sraRgnSubtract(r3, r1);
		sraRgnDestroy(r1);

if (ncdb) fprintf(stderr, ">>**--**>> found rect via FORCE: %dx%d+%d+%d -- %d %d\n", w, h, x, y, *x_hit, *y_hit);

		fr_FORCE++;
		fr_FORCEt++;
	}
}

void expire_rects2(int idx, int w, int h, int *x_hit, int *y_hit, int big1, int big2, int cram) {
	sraRegionPtr r1, r2, r3;
	int x = -1, y = -1, n, i, j, k;
	int nwgt_max = 128, nwgt = 0;
	int type[128];
	int val[4][128];
	double wgt[128], norm;
	int Expire = 1, Force = 2;
	int do_expire = 1;
	int do_force = 1;
	double now = dnow(), r;
	double newest = -1.0, oldest = -1.0, basetime;
	double map_factor = 0.25;

	for (i=0; i<cache_list_num; i++) {
		double d, d1, d2;

		d1 = cache_list[i].bs_time;
		d2 = cache_list[i].su_time;

		d = d1;
		if (d2 > d) d = d2;

		if (d == 0.0) {
			continue;
		}

		if (oldest == -1.0 || d < oldest) {
			oldest = d;
		}
		if (newest == -1.0 || d > newest) {
			newest = d;
		}
	}
	if (newest == -1.0) {
		newest = now;
	}
	if (oldest == -1.0) {
		oldest = newest - 1800;
	}

	basetime = newest + 0.1 * (newest - oldest);

	if (do_expire) {
		int old[10], N = 4;
		double dold[10], fa, d, d1, d2;
		int a0 = w * h, a1;

		for (k=1; k<=N; k++) {
			old[k] = -1;
			dold[k] = -1.0;
		}
		for (i=0; i<cache_list_num; i++) {
			int wb = cache_list[i].bs_w;
			int hb = cache_list[i].bs_h;
			if (cache_list[i].bs_x < 0) {
				continue;
			}
			if (w > wb || h > hb) {
				continue;
			}
			if (wb == 0 || hb == 0) {
				continue;
			}
			if (a0 == 0) {
				continue;
			}
			if (i == idx) {
				continue;
			}

			a1 = wb * hb;
			fa = ((double) a1) / a0;
			k = (int) fa;

			if (k < 1) k = 1;
			if (k > N) continue;

			d1 = cache_list[i].bs_time;
			d2 = cache_list[i].su_time;

			d = d1;
			if (d2 > d) d = d2;
			if (d == 0.0) d = oldest;

			if (dold[k] == -1.0 || d < dold[k]) {
				old[k] = i;
				dold[k] = d;
			}
		}

		for (k=1; k<=N; k++) {
			if (old[k] >= 0) {
				int ik = old[k];
				int k_w = cache_list[ik].bs_w;
				int k_h = cache_list[ik].bs_h;

				wgt[nwgt] =  (basetime - dold[k]) / (k_w * k_h);
				if (cache_list[ik].map_state == IsViewable) {
					wgt[nwgt] *= map_factor;
				}
				type[nwgt] = Expire;
				val[0][nwgt] = ik;
if (ncdb) fprintf(stderr, "Expire[%02d]   %9.5f  age=%9.4f  area=%8d  need=%8d\n", nwgt, 10000 * wgt[nwgt], basetime - dold[k], k_w * k_h, w*h);
				nwgt++;
				if (nwgt >= nwgt_max) {
					break;
				}
			}
		}
	}

	/* next, force ourselves into some corner, expiring many rect */
	if (do_force) {
		int corner_x, corner_y;
		int x0, y0;

		for (n = 1; n < ncache; n += 2) {
		    if (big1 && ncache > 2 && n == 1) {
			continue;
		    }
		    if (big2 && ncache > 4 && n <= 3) {
			continue;
		    }
		    for (corner_x = 0; corner_x < 2; corner_x++) {
			if (cram && big1 && corner_x == 0) {
				continue;
			}
			for (corner_y = 0; corner_y < 2; corner_y++) {
				double age = 0.0, area = 0.0, amap = 0.0, a;
				double d, d1, d2, score;
				int nc = 0;

				x0 = 0;
				y0 = 0;
				y0 += n * dpy_y; 

				if (corner_y) {
					y0 += dpy_y - h; 
				}
				if (corner_x) {
					x0 += dpy_x - w; 
				}
				r1 = sraRgnCreateRect(x0, y0, x0+w, y0+h);

				for (i=0; i<cache_list_num; i++) {
					int xb = cache_list[i].bs_x;
					int yb = cache_list[i].bs_y;
					int wb = cache_list[i].bs_w;
					int hb = cache_list[i].bs_h;

					if (xb < 0) {
						continue;
					}
					if (nabs(yb - y0) > dpy_y) {
						continue;
					}

					r2 = sraRgnCreateRect(xb, yb, xb+wb, yb+hb);
					if (! sraRgnAnd(r2, r1)) {
						sraRgnDestroy(r2);
						continue;
					}
					sraRgnDestroy(r2);

					a = wb * hb;

					d1 = cache_list[i].bs_time;
					d2 = cache_list[i].su_time;

					d = d1;
					if (d2 > d) d = d2;
					if (d == 0.0) d = oldest;

					if (cache_list[i].map_state == IsViewable) {
						amap += a;
					}
					area += a;
					age += (basetime - d) * a;
					nc++;
				}
				if (nc == 0) {
					score = 999999.9;
				} else {
					double fac;
					age = age / area;
					score = age / area;
					fac = 1.0 * (1.0 - amap/area) + map_factor * (amap/area);
					score *= fac;
				}

				wgt[nwgt] =  score;
				type[nwgt] = Force;
				val[0][nwgt] = n;
				val[1][nwgt] = x0;
				val[2][nwgt] = y0;
if (ncdb) fprintf(stderr, "Force [%02d]   %9.5f  age=%9.4f  area=%8d  amap=%8d  need=%8d\n", nwgt, 10000 * wgt[nwgt], age, (int) area, (int) amap, w*h);
				nwgt++;
				if (nwgt >= nwgt_max) break;
				sraRgnDestroy(r1);
			}
			if (nwgt >= nwgt_max) break;
		    }
		    if (nwgt >= nwgt_max) break;
		}
	}

	if (nwgt == 0) {
if (ncdb) fprintf(stderr, "nwgt=0\n");
		*x_hit = -1;
		return;
	}

	norm = 0.0;
	for (i=0; i < nwgt; i++) {
		norm += wgt[i];
	}
	for (i=0; i < nwgt; i++) {
		wgt[i] /= norm; 
	}

	r = rfac();

	norm = 0.0;
	for (j=0; j < nwgt; j++) {
		norm += wgt[j];
if (ncdb) fprintf(stderr, "j=%2d  acc=%.6f r=%.6f\n", j, norm, r); 
		if (r < norm) {
			break;
		}
	}
	if (j >= nwgt) {
		j = nwgt - 1;
	}

	if (type[j] == Expire) {
		int ik = val[0][j];
		int k_x = cache_list[ik].bs_x;
		int k_y = cache_list[ik].bs_y;
		int k_w = cache_list[ik].bs_w;
		int k_h = cache_list[ik].bs_h;

if (ncdb) fprintf(stderr, ">>**--**>> found rect [%d] via RAN EXPIRE: %d 0x%lx -- %dx%d+%d+%d %d %d --  %dx%d+%d+%d  A: %d/%d\n",
	get_bs_n(*y_hit), ik, cache_list[ik].win, w, h, x, y, *x_hit, *y_hit, k_w, k_h, k_x, k_y, k_w * k_h, w * h);

		free_rect(ik);
		fr_EXPIRE++;
		fr_EXPIREt++;
		*x_hit = k_x;
		*y_hit = k_y;
		n = get_bs_n(*y_hit);
		if (n >= 0) {
			r1 = rect_reg[n];
			r2 = sraRgnCreateRect(*x_hit, *y_hit, *x_hit + w, *y_hit + h);
			sraRgnSubtract(r1, r2);
			sraRgnDestroy(r2);
		} else {
			fprintf(stderr, "failure to find y n in find_rect\n");
			clean_up_exit(1);
		}
		
	} else if (type[j] == Force) {

		int x0 = val[1][j];
		int y0 = val[2][j];
		n = val[0][j];
		
		r1 = sraRgnCreateRect(x0, y0, x0+w, y0+h);

		for (i=0; i<cache_list_num; i++) {
			int xb = cache_list[i].bs_x;
			int yb = cache_list[i].bs_y;
			int wb = cache_list[i].bs_w;
			int hb = cache_list[i].bs_h;
			if (xb < 0) {
				continue;
			}
			if (nabs(yb - y0) > dpy_y) {
				continue;
			}
			r2 = sraRgnCreateRect(xb, yb, xb+wb, yb+hb);
			if (sraRgnAnd(r2, r1)) {
				free_rect(i);
			}
			sraRgnDestroy(r2);
		}
		*x_hit = x0;
		*y_hit = y0;
		r3 = rect_reg[2*n-1];
		sraRgnSubtract(r3, r1);
		sraRgnDestroy(r1);

if (ncdb) fprintf(stderr, ">>**--**>> found rect [%d] via RAN FORCE: %dx%d+%d+%d -- %d %d\n", n, w, h, x, y, *x_hit, *y_hit);

		fr_FORCE++;
		fr_FORCEt++;
	}
}

void expire_rects(int idx, int w, int h, int *x_hit, int *y_hit, int big1, int big2, int cram) {
	int method = 2;
	if (method == 1) {
		expire_rects1(idx, w, h, x_hit, y_hit, big1, big2, cram);
	} else if (method == 2) {
		expire_rects2(idx, w, h, x_hit, y_hit, big1, big2, cram);
	}
}

int find_rect(int idx, int x, int y, int w, int h) {
	sraRegionPtr r1, r2;
	sraRectangleIterator *iter;
	sraRect rt;
	int n, x_hit = -1, y_hit = -1;
	int big1 = 0, big2 = 0, cram = 0;
	double fac1 = 0.1, fac2 = 0.25;
	double last_clean = 0.0;
	double now = dnow();
	static int nobigs = -1;

	if (rect_reg[1] == NULL) {
		for (n = 1; n <= ncache; n++) {
			rect_reg[n] = sraRgnCreateRect(0, n * dpy_y, dpy_x, (n+1) * dpy_y);
		}
	} else if (now > last_clean + 60) {
		last_clean = now;
		for (n = 1; n < ncache; n += 2) {
			int i, n2 = n+1;

			/* n */
			sraRgnDestroy(rect_reg[n]);
			r1 = sraRgnCreateRect(0, n * dpy_y, dpy_x, (n+1) * dpy_y);
			for (i=0; i<cache_list_num; i++) {
				int bs_x = cache_list[i].bs_x;
				int bs_y = cache_list[i].bs_y;
				int bs_w = cache_list[i].bs_w;
				int bs_h = cache_list[i].bs_h;
				if (bs_x < 0) {
					continue;
				}
				if (get_bs_n(bs_y) != n) {
					continue;
				}
				r2 = sraRgnCreateRect(bs_x, bs_y, bs_x+bs_w, bs_y+bs_h);
				sraRgnSubtract(r1, r2);
			}
			rect_reg[n] = r1;

			/* n+1 */
			sraRgnDestroy(rect_reg[n2]);
			r1 = sraRgnCreateRect(0, n2 * dpy_y, dpy_x, (n2+1) * dpy_y);
			for (i=0; i<cache_list_num; i++) {
				int bs_x = cache_list[i].bs_x;
				int su_x = cache_list[i].su_x;
				int su_y = cache_list[i].su_y;
				int su_w = cache_list[i].su_w;
				int su_h = cache_list[i].su_h;
				if (bs_x < 0) {
					continue;
				}
				if (get_bs_n(su_y) != n2) {
					continue;
				}
				r2 = sraRgnCreateRect(su_x, su_y, su_x+su_w, su_y+su_h);
				sraRgnSubtract(r1, r2);
			}
			rect_reg[n2] = r1;
		}
	}

	if (idx < 0 || idx >= cache_list_num) {
if (ncdb) fprintf(stderr, "free_rect: bad index: %d\n", idx);
		clean_up_exit(1);
	}

	cache_list[idx].bs_x = -1;
	cache_list[idx].su_x = -1;
	cache_list[idx].bs_time = 0.0;
	cache_list[idx].su_time = 0.0;

	if (ncache_pad) {
		x -= ncache_pad;	
		y -= ncache_pad;	
		w += 2 * ncache_pad;	
		h += 2 * ncache_pad;	
	}

	if (ncache <= 2) {
		cram = 1;
		fac2 = 0.45;
	} else if (ncache <= 4) {
		fac1 = 0.18;
		fac2 = 0.35;
	}
	if (macosx_console && !macosx_ncache_macmenu) {
		if (cram) {
			fac1 *= 1.5;	
			fac2 *= 1.5;	
		} else {
			fac1 *= 2.5;	
			fac2 *= 2.5;	
		}
	}
	if (w * h > fac1 * (dpy_x * dpy_y)) {
		big1 = 1;
	}
	if (w * h > fac2 * (dpy_x * dpy_y)) {
		big2 = 1;
	}

	if (nobigs < 0) {
		if (getenv("NOBIGS")) {
			nobigs = 1;
		} else {
			nobigs = 0;
		}
	}
	if (nobigs) {
		big1 = big2 = 0;
	}

	if (w > dpy_x || h > dpy_y) {
if (ncdb) fprintf(stderr, ">>**--**>> BIG1 rect: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
		fr_BIG1++;
		fr_BIG1t++;
		return 0;
	}
	if (w == dpy_x && h == dpy_y) {
if (ncdb) fprintf(stderr, ">>**--**>> BIG1 rect: %dx%d+%d+%d -- %d %d (FULL DISPLAY)\n", w, h, x, y, x_hit, y_hit);
		fr_BIG1++;
		fr_BIG1t++;
		return 0;
	}
	if (cram && big2) {
if (ncdb) fprintf(stderr, ">>**--**>> BIG2 rect: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
		fr_BIG2++;
		fr_BIG2t++;
		return 0;
	}

	/* first try individual rects of unused region */
	for (n = 1; n < ncache; n += 2) {
		r1 = rect_reg[n];
		r2 = NULL;
		if (big1 && n == 1 && ncache > 2) {
			continue;
		}
		if (big2 && n <= 3 && ncache > 4) {
			continue;
		}
		iter = sraRgnGetIterator(r1);
		while (sraRgnIteratorNext(iter, &rt)) {
			int rw = rt.x2 - rt.x1;
			int rh = rt.y2 - rt.y1;
			if (cram && big1 && rt.x1 < dpy_x/4) {
				continue;
			}
			if (rw >= w && rh >= h) {
				x_hit = rt.x1;
				y_hit = rt.y1;
				if (cram && big1) {
					x_hit = rt.x2 - w;
				}
				r2 = sraRgnCreateRect(x_hit, y_hit, x_hit + w, y_hit + h);
				break;
			}
		}
		sraRgnReleaseIterator(iter);
		if (r2 != NULL) {
if (ncdb) fprintf(stderr, ">>**--**>> found rect via REGION: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
			fr_REGION++;
			fr_REGIONt++;
			sraRgnSubtract(r1, r2);
			sraRgnDestroy(r2);
			break;
		}
	}

	
	/* next try moving corner to grid points */
	if (x_hit < 0) {
	    for (n = 1; n < ncache; n += 2) {
		int rx, ry, Nx = 48, Ny = 24, ny = n * dpy_y;

		if (big1 && n == 1 && ncache > 2) {
			continue;
		}
		if (big2 && n == 3 && ncache > 4) {
			continue;
		}

		r1 = sraRgnCreateRect(0, n * dpy_y, dpy_x, (n+1) * dpy_y);
		sraRgnSubtract(r1, rect_reg[n]);
		r2 = NULL;

		rx = 0;
		while (rx + w <= dpy_x) {
		    ry = 0;
		    if (cram && big1 && rx < dpy_x/4) {
			rx += dpy_x/Nx;
		    	continue;
		    }
		    while (ry + h <= dpy_y) {
			r2 = sraRgnCreateRect(rx, ry+ny, rx + w, ry+ny + h);
			if (sraRgnAnd(r2, r1)) {
				sraRgnDestroy(r2);
				r2 = NULL;
			} else {
				sraRgnDestroy(r2);
				r2 = sraRgnCreateRect(rx, ry+ny, rx + w, ry+ny + h);
				x_hit = rx;
				y_hit = ry+ny;
			}
			ry += dpy_y/Ny;
			if (r2) break;
		    }
		    rx += dpy_x/Nx;
		    if (r2) break;
		}
		sraRgnDestroy(r1);
		if (r2 != NULL) {
			sraRgnSubtract(rect_reg[n], r2);
			sraRgnDestroy(r2);
if (ncdb) fprintf(stderr, ">>**--**>> found rect via GRID: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
			fr_GRID++;
			fr_GRIDt++;
			break;
		}
	    }
	}

	/* next, try expiring the oldest/smallest used bs/su rectangle we fit in */

	if (x_hit < 0) {
		expire_rects(idx, w, h, &x_hit, &y_hit, big1, big2, cram);
	}

	cache_list[idx].bs_x = x_hit;
	cache_list[idx].bs_y = y_hit;
	cache_list[idx].bs_w = w;
	cache_list[idx].bs_h = h;

	cache_list[idx].su_x = x_hit;
	cache_list[idx].su_y = y_hit + dpy_y;
	cache_list[idx].su_w = w;
	cache_list[idx].su_h = h;

	if (x_hit < 0) {
		/* bad news, can it still happen? */
		if (ncdb) fprintf(stderr, ">>**--**>> *FAIL rect: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
		fr_FAIL++;
		fr_FAILt++;
		return 0;
	} else {
		if (0) fprintf(stderr, ">>**--**>> found rect: %dx%d+%d+%d -- %d %d\n", w, h, x, y, x_hit, y_hit);
	}

	if (zero_rects) {
		r1 = sraRgnCreateRect(x_hit, y_hit, x_hit+w, y_hit+h);
		sraRgnSubtract(zero_rects, r1);
		sraRgnDestroy(r1);
		r1 = sraRgnCreateRect(x_hit, y_hit+dpy_y, x_hit+w, y_hit+dpy_y+h);
		sraRgnSubtract(zero_rects, r1);
		sraRgnDestroy(r1);
	}

	return 1;
}

static void cache_cr(sraRegionPtr r, int dx, int dy, double d0, double d1, int *nbatch) {
	if (sraRgnEmpty(r)) {
		return;
	}
	if (nbatch == NULL) {
		if (!fb_push_wait(d0, FB_COPY)) {
			fb_push_wait(d0/2, FB_COPY);
		}
		do_copyregion(r, dx, dy, 0);
		if (!fb_push_wait(d1, FB_COPY)) {
			fb_push_wait(d1/2, FB_COPY);
		}
	} else {
		batch_dxs[*nbatch] = dx;
		batch_dys[*nbatch] = dy;
		batch_reg[*nbatch] = sraRgnCreateRgn(r);
		(*nbatch)++;
	}
}

double save_delay0    = 0.02;
double restore_delay0 = 0.02;
double save_delay1    = 0.05;
double restore_delay1 = 0.05;
static double dtA, dtB;

int valid_wr(int idx, Window win, XWindowAttributes *attr) {
#ifdef MACOSX
	if (macosx_console) {
		/* this is all to avoid animation changing WxH+X+Y... */
		if (idx >= 0) {
			int rc = valid_window(win, attr, 1);
			attr->x = cache_list[idx].x;
			attr->y = cache_list[idx].y;
			attr->width = cache_list[idx].width;
			attr->height = cache_list[idx].height;
			return rc;
		} else {
			return valid_window(win, attr, 1);
		}
	}
#else
	if (!idx) {}
#endif
	return valid_window(win, attr, 1);
}

int clipped(int idx) {
	int ic;	
	sraRegionPtr r0, r1, r2;
	int x1, y1, w1, h1;
	Window win;
	int clip = 0;

	if (idx < 0) {
		return 0;
	}
	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

	x1 = cache_list[idx].x;
	y1 = cache_list[idx].y;
	w1 = cache_list[idx].width;
	h1 = cache_list[idx].height;

	win = cache_list[idx].win;

	r1 = sraRgnCreateRect(x1, y1, x1+w1, y1+h1);
	sraRgnAnd(r1, r0);

	for (ic = old_stack_n - 1; ic >= 0; ic--) {
		int xc, yc, wc, hc, idx2;

		if (old_stack[ic] == win) {
			break;
		}
		if (old_stack_mapped[ic] == 0) {
			continue;
		}
		idx2 = lookup_old_stack_index(ic);
		if (idx2 < 0) {
			continue;
		}
		if (cache_list[idx2].win == win) {
			break;
		}
		if (cache_list[idx2].map_state != IsViewable) {
			continue;
		}
		xc = cache_list[idx2].x;
		yc = cache_list[idx2].y;
		wc = cache_list[idx2].width;
		hc = cache_list[idx2].height;

		r2 = sraRgnCreateRect(xc, yc, xc+wc, yc+hc);
		sraRgnAnd(r2, r0);
		if (sraRgnAnd(r2, r1)) {
if (0) fprintf(stderr, "clip[0x%lx]: 0x%lx, %d/%d\n", win, cache_list[idx2].win, ic, idx2);
			clip = 1;
		}
		sraRgnDestroy(r2);
		if (clip) {
			break;
		}
	}
	sraRgnDestroy(r0);
	sraRgnDestroy(r1);
if (0) fprintf(stderr, "clip[0x%lx]: %s\n", win, clip ? "clipped" : "no-clipped");
	return clip;
}

void clip_region(sraRegionPtr r, Window win) {
	int ic, idx2;	
	sraRegionPtr r1;
	for (ic = old_stack_n - 1; ic >= 0; ic--) {
		int xc, yc, wc, hc;

if (0) fprintf(stderr, "----[0x%lx]: 0x%lx, %d  %d\n", win, old_stack[ic], ic, old_stack_mapped[ic]);
		if (old_stack[ic] == win) {
			break;
		}
		if (old_stack_mapped[ic] == 0) {
			continue;
		}
		idx2 = lookup_old_stack_index(ic);
		if (idx2 < 0) {
			continue;
		}
		if (cache_list[idx2].win == win) {
			break;
		}
		if (cache_list[idx2].map_state != IsViewable) {
			continue;
		}
		xc = cache_list[idx2].x;
		yc = cache_list[idx2].y;
		wc = cache_list[idx2].width;
		hc = cache_list[idx2].height;
		r1 = sraRgnCreateRect(xc, yc, xc+wc, yc+hc);
		if (sraRgnAnd(r1, r)) {
			sraRgnSubtract(r, r1);
if (0) fprintf(stderr, "clip[0x%lx]: 0x%lx, %d/%d\n", win, cache_list[idx2].win, ic, idx2);
		}
		sraRgnDestroy(r1);
	}
}

int bs_save(int idx, int *nbatch, XWindowAttributes *attr, int clip, int only_if_tracking, int *valid, int verb) {
	Window win = cache_list[idx].win;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int x, y, w, h;
	int dx, dy, rc = 1;
	sraRegionPtr r, r0;
	
	x1 = cache_list[idx].x;
	y1 = cache_list[idx].y;
	w1 = cache_list[idx].width;
	h1 = cache_list[idx].height;

if (ncdb && verb) fprintf(stderr, "backingstore save:       0x%lx  %3d clip=%d\n", win, idx, clip);
	
	X_LOCK;
	if (*valid) {
		attr->x = x1;
		attr->y = y1;
		attr->width = w1;
		attr->height = h1;
	} else if (! valid_wr(idx, win, attr)) {
if (ncdb) fprintf(stderr, "bs_save:    not a valid X window: 0x%lx\n", win);
		X_UNLOCK;
		*valid = 0;
		cache_list[idx].valid = 0;
		return 0;
	} else {
		*valid = 1;
	}
	X_UNLOCK;

	if (only_if_tracking && cache_list[idx].bs_x < 0) {
		return 0;
	}

	x2 = attr->x;
	y2 = attr->y;
	w2 = attr->width;
	h2 = attr->height;

	if (cache_list[idx].bs_x < 0) {
		rc = find_rect(idx, x2, y2, w2, h2);
	} else if (w2 > cache_list[idx].bs_w || h2 > cache_list[idx].bs_h) {
		free_rect(idx);
		rc = find_rect(idx, x2, y2, w2, h2);
	}

	x = cache_list[idx].bs_x;
	y = cache_list[idx].bs_y;
	w = cache_list[idx].bs_w;
	h = cache_list[idx].bs_h;

	if (x < 0 || ! rc) {
if (ncdb) fprintf(stderr, "BS_save: FAIL FOR: %d\n", idx);
		return 0;
	}

	if (ncache_pad) {
		x2 -= ncache_pad;	
		y2 -= ncache_pad;	
		w2 += 2 * ncache_pad;	
		h2 += 2 * ncache_pad;	
	}

	if (clipshift) {
		x2 -= coff_x;
		y2 -= coff_y;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
	sraRgnAnd(r, r0);

	if (clip) {
		clip_region(r, win);
	}

	if (sraRgnEmpty(r)) {
if (ncdb && verb) fprintf(stderr, "BS_save: Region Empty: %d\n", idx);
		sraRgnDestroy(r0);
		sraRgnDestroy(r);
		return 0;
	}

	dx = x - x2; 
	dy = y - y2; 

	sraRgnOffset(r, dx, dy);

	dtA =  dnowx();
if (ncdb && verb) fprintf(stderr, "BS_save: %.4f      %d dx=%d dy=%d\n", dtA, idx, dx, dy);
	if (w2 > 0 && h2 > 0) {
		cache_cr(r, dx, dy, save_delay0, save_delay1, nbatch);
	}
	dtB =  dnowx();
if (ncdb && verb) fprintf(stderr, "BS_save: %.4f %.2f %d done.  %dx%d+%d+%d %dx%d+%d+%d  %.2f %.2f\n", dtB, dtB-dtA, idx, w1, h1, x1, y1, w2, h2, x2, y2, cache_list[idx].bs_time - x11vnc_start, dnowx());

	sraRgnDestroy(r0);
	sraRgnDestroy(r);

	last_bs_save = cache_list[idx].bs_time = dnow();

	return 1;
}

int su_save(int idx, int *nbatch, XWindowAttributes *attr, int clip, int *valid, int verb) {
	Window win = cache_list[idx].win;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int x, y, w, h;
	int dx, dy, rc = 1;
	sraRegionPtr r, r0;
	
if (ncdb && verb) fprintf(stderr, "save-unders save:        0x%lx  %3d \n", win, idx);

	x1 = cache_list[idx].x;
	y1 = cache_list[idx].y;
	w1 = cache_list[idx].width;
	h1 = cache_list[idx].height;
	
	X_LOCK;
	if (*valid) {
		attr->x = x1;
		attr->y = y1;
		attr->width = w1;
		attr->height = h1;
	} else if (! valid_wr(idx, win, attr)) {
if (ncdb) fprintf(stderr, "su_save:    not a valid X window: 0x%lx\n", win);
		X_UNLOCK;
		*valid = 0;
		cache_list[idx].valid = 0;
		return 0;
	} else {
		*valid = 1;
	}
	X_UNLOCK;

	x2 = attr->x;
	y2 = attr->y;
	w2 = attr->width;
	h2 = attr->height;

	if (cache_list[idx].bs_x < 0) {
		rc = find_rect(idx, x2, y2, w2, h2);
	} else if (w2 > cache_list[idx].su_w || h2 > cache_list[idx].su_h) {
		free_rect(idx);
		rc = find_rect(idx, x2, y2, w2, h2);
	}
	x = cache_list[idx].su_x;
	y = cache_list[idx].su_y;
	w = cache_list[idx].su_w;
	h = cache_list[idx].su_h;

	if (x < 0 || ! rc) {
if (ncdb) fprintf(stderr, "SU_save: FAIL FOR: %d\n", idx);
		return 0;
	}

	if (ncache_pad) {
		x2 -= ncache_pad;	
		y2 -= ncache_pad;	
		w2 += 2 * ncache_pad;	
		h2 += 2 * ncache_pad;	
	}

	if (clipshift) {
		x2 -= coff_x;
		y2 -= coff_y;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
	sraRgnAnd(r, r0);

	if (clip) {
		clip_region(r, win);
	}

	if (sraRgnEmpty(r)) {
if (ncdb && verb) fprintf(stderr, "SU_save: Region Empty: %d\n", idx);
		sraRgnDestroy(r0);
		sraRgnDestroy(r);
		return 0;
	}


	dx = x - x2; 
	dy = y - y2; 

	sraRgnOffset(r, dx, dy);

	dtA =  dnowx();
if (ncdb && verb) fprintf(stderr, "SU_save: %.4f      %d dx=%d dy=%d\n", dtA, idx, dx, dy);
	if (w2 > 0 && h2 > 0) {
		cache_cr(r, dx, dy, save_delay0, save_delay1, nbatch);
	}
	dtB =  dnowx();
if (ncdb && verb) fprintf(stderr, "SU_save: %.4f %.2f %d done.  %dx%d+%d+%d %dx%d+%d+%d  %.2f %.2f\n", dtB, dtB-dtA, idx, w1, h1, x1, y1, w2, h2, x2, y2, cache_list[idx].su_time - x11vnc_start, dnowx());

	sraRgnDestroy(r0);
	sraRgnDestroy(r);

	last_su_save = cache_list[idx].su_time = dnow();
	
	return 1;
}

int bs_restore(int idx, int *nbatch, sraRegionPtr rmask, XWindowAttributes *attr, int clip, int nopad, int *valid, int verb) {
	Window win = cache_list[idx].win;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int x, y, w, h;
	int dx, dy;
	sraRegionPtr r, r0;

if (ncdb && verb) fprintf(stderr, "backingstore restore:    0x%lx  %3d \n", win, idx);

	x1 = cache_list[idx].x;
	y1 = cache_list[idx].y;
	w1 = cache_list[idx].width;
	h1 = cache_list[idx].height;
	
	X_LOCK;
	if (*valid) {
		attr->x = x1;
		attr->y = y1;
		attr->width = w1;
		attr->height = h1;
	} else if (! valid_wr(idx, win, attr)) {
if (ncdb) fprintf(stderr, "BS_restore: not a valid X window: 0x%lx\n", win);
		*valid = 0;
		X_UNLOCK;
		return 0;
	} else {
		*valid = 1;
	}
	X_UNLOCK;

	x2 = attr->x;
	y2 = attr->y;
	w2 = attr->width;
	h2 = attr->height;

	x = cache_list[idx].bs_x;
	y = cache_list[idx].bs_y;
	w = cache_list[idx].bs_w;
	h = cache_list[idx].bs_h;

	if (x < 0 || cache_list[idx].bs_time == 0.0) {
		return 0;
	}

	if (ncache_pad) {
		if (nopad) {
			x += ncache_pad;	
			y += ncache_pad;	
			w -= 2 * ncache_pad;	
			h -= 2 * ncache_pad;	
		} else {
			x2 -= ncache_pad;	
			y2 -= ncache_pad;	
			w2 += 2 * ncache_pad;	
			h2 += 2 * ncache_pad;	
		}
	}

	if (clipshift) {
		x2 -= coff_x;
		y2 -= coff_y;
	}

	if (w2 > w) {
		w2 = w;
	}
	if (h2 > h) {
		h2 = h;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r = sraRgnCreateRect(x, y, x+w2, y+h2);

	dx = x2 - x; 
	dy = y2 - y; 

	sraRgnOffset(r, dx, dy);
	sraRgnAnd(r, r0);

	if (clip) {
		clip_region(r, win);
	}
	if (rmask != NULL) {
		sraRgnAnd(r, rmask);
	}

	dtA =  dnowx();
if (ncdb && verb) fprintf(stderr, "BS_rest: %.4f      %d dx=%d dy=%d\n", dtA, idx, dx, dy);
	if (w2 > 0 && h2 > 0) {
		cache_cr(r, dx, dy, restore_delay0, restore_delay1, nbatch);
	}
	dtB =  dnowx();
if (ncdb && verb) fprintf(stderr, "BS_rest: %.4f %.2f %d done.  %dx%d+%d+%d %dx%d+%d+%d  %.2f %.2f\n", dtB, dtB-dtA, idx, w1, h1, x1, y1, w2, h2, x2, y2, cache_list[idx].bs_time - x11vnc_start, dnowx());

	sraRgnDestroy(r0);
	sraRgnDestroy(r);

	last_bs_restore = dnow();
	
	return 1;
}

int su_restore(int idx, int *nbatch, sraRegionPtr rmask, XWindowAttributes *attr, int clip, int nopad, int *valid, int verb) {
	Window win = cache_list[idx].win;
	int x1, y1, w1, h1;
	int x2 = 0, y2 = 0, w2 = 0, h2 = 0;
	int x, y, w, h;
	int dx, dy;
	sraRegionPtr r, r0;

if (ncdb && verb) fprintf(stderr, "save-unders  restore:    0x%lx  %3d \n", win, idx);
	
	x1 = cache_list[idx].x;
	y1 = cache_list[idx].y;
	w1 = cache_list[idx].width;
	h1 = cache_list[idx].height;
	
	X_LOCK;
	if (*valid) {
		attr->x = x1;
		attr->y = y1;
		attr->width = w1;
		attr->height = h1;
		x2 = attr->x;
		y2 = attr->y;
		w2 = attr->width;
		h2 = attr->height;
	} else if (! valid_wr(idx, win, attr)) {
if (ncdb) fprintf(stderr, "SU_restore: not a valid X window: 0x%lx\n", win);
		*valid = 0;
		x2 = x1;
		y2 = y1;
		w2 = w1;
		h2 = h1;
	} else {
		x2 = attr->x;
		y2 = attr->y;
		w2 = attr->width;
		h2 = attr->height;
		*valid = 1;
	}
	X_UNLOCK;

	x = cache_list[idx].su_x;
	y = cache_list[idx].su_y;
	w = cache_list[idx].su_w;
	h = cache_list[idx].su_h;

	if (x < 0 || cache_list[idx].bs_x < 0 || cache_list[idx].su_time == 0.0) {
if (ncdb) fprintf(stderr, "SU_rest: su_x/bs_x/su_time: %d %d %.3f\n", x, cache_list[idx].bs_x, cache_list[idx].su_time);
		return 0;
	}

	if (ncache_pad) {
		if (nopad) {
			x += ncache_pad;	
			y += ncache_pad;	
			w -= 2 * ncache_pad;	
			h -= 2 * ncache_pad;	
		} else {
			x2 -= ncache_pad;	
			y2 -= ncache_pad;	
			w2 += 2 * ncache_pad;	
			h2 += 2 * ncache_pad;	
		}
	}

	if (clipshift) {
		x2 -= coff_x;
		y2 -= coff_y;
	}

	if (w2 > w) {
		w2 = w;
	}
	if (h2 > h) {
		h2 = h;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r = sraRgnCreateRect(x, y, x+w2, y+h2);

	dx = x2 - x; 
	dy = y2 - y; 

	sraRgnOffset(r, dx, dy);
	sraRgnAnd(r, r0);

	if (clip) {
		clip_region(r, win);
	}
	if (rmask != NULL) {
		sraRgnAnd(r, rmask);
	}

	dtA =  dnowx();
if (ncdb && verb) fprintf(stderr, "SU_rest: %.4f      %d dx=%d dy=%d\n", dtA, idx, dx, dy);
	if (w2 > 0 && h2 > 0) {
		cache_cr(r, dx, dy, restore_delay0, restore_delay1, nbatch);
	}
	dtB =  dnowx();
if (ncdb && verb) fprintf(stderr, "SU_rest: %.4f %.2f %d done.  %dx%d+%d+%d %dx%d+%d+%d  %.2f %.2f\n", dtB, dtB-dtA, idx, w1, h1, x1, y1, w2, h2, x2, y2, cache_list[idx].su_time - x11vnc_start, dnowx());

	sraRgnDestroy(r0);
	sraRgnDestroy(r);

	last_su_restore = dnow();

	return 1;
}

void check_zero_rects(void) {
	sraRect rt;
	sraRectangleIterator *iter;
	if (! zero_rects) {
		zero_rects = sraRgnCreate();
	}
	if (sraRgnEmpty(zero_rects)) {
		return;
	}
		
	iter = sraRgnGetIterator(zero_rects);
	while (sraRgnIteratorNext(iter, &rt)) {
		zero_fb(rt.x1, rt.y1, rt.x2, rt.y2);
		mark_rect_as_modified(rt.x1, rt.y1, rt.x2, rt.y2, 0);
	}
	sraRgnReleaseIterator(iter);
	sraRgnMakeEmpty(zero_rects);
}

void block_stats(void) {
	int n, k, s1, s2;
	static int t = -1;
	int vcnt, icnt, tcnt, vtot = 0, itot = 0, ttot = 0;
	t++;
	for (n = 1; n < ncache+1; n += 2) {
		double area = 0.0, frac;
		vcnt = 0;
		icnt = 0;
		tcnt = 0;
		for(k=0; k<cache_list_num; k++) {
			XWindowAttributes attr;
			int x = cache_list[k].bs_x;
			int y = cache_list[k].bs_y;
			int w = cache_list[k].bs_w;
			int h = cache_list[k].bs_h;
			int rc = 0;
			Window win = cache_list[k].win;

			if (win == None) {
				continue;
			}
			if (n == 1) {
				X_LOCK;
				rc = valid_window(win, &attr, 1);
				X_UNLOCK;
				if (rc) {
					vtot++;
				} else {
					itot++;
				}
				if (x >= 0) {
					ttot++;
				}
			}
			if (y < n*dpy_y || y > (n+1)*dpy_y) {
				continue;
			}
			if (n != 1) {
				X_LOCK;
				rc = valid_window(win, &attr, 1);
				X_UNLOCK;
			}
			if (rc) {
				vcnt++;
			} else {
				icnt++;
			}
			if (x >= 0) {
				tcnt++;
			}
			if (x < 0) {
				continue;
			}
			area += cache_list[k].width * cache_list[k].height;
			if (! rc && ! macosx_console) {
				char *u = getenv("USER");
				if (u && !strcmp(u, "runge"))	fprintf(stderr, "\a");
				if (ncdb) fprintf(stderr, "\n   *** UNRECLAIMED WINDOW: 0x%lx  %dx%d+%d+%d\n\n", win, w, h, x, y);
				DELETE(k);
			}
			if (t < 3 || (t % 4) == 0 || hack_val || macosx_console) {
				double t1 = cache_list[k].su_time;
				double t2 = cache_list[k].bs_time;
				if (t1 > 0.0) {t1 = dnow() - t1;} else {t1 = -1.0;}
				if (t2 > 0.0) {t2 = dnow() - t2;} else {t2 = -1.0;}
				if (ncdb) fprintf(stderr, "     [%02d] %04d 0x%08lx bs: %04dx%04d+%04d+%05d vw: %04dx%04d+%04d+%04d cl: %04dx%04d+%04d+%04d map=%d su=%9.3f bs=%9.3f cnt=%d/%d\n",
				    n, k, win, w, h, x, y, attr.width, attr.height, attr.x, attr.y,
				    cache_list[k].width, cache_list[k].height, cache_list[k].x, cache_list[k].y,
				    attr.map_state == IsViewable, t1, t2, cache_list[k].create_cnt, cache_list[k].map_cnt); 
			}
		}
		frac = area /(dpy_x * dpy_y);
		if (ncdb) fprintf(stderr, "block[%02d]  %.3f  %8d  trak/val/inval: %d/%d/%d of %d\n", n, frac, (int) area, tcnt, vcnt, icnt, vcnt+icnt);
	}

	if (ncdb) fprintf(stderr, "\n");
	if (ncdb) fprintf(stderr, "block: trak/val/inval %d/%d/%d of %d\n", ttot, vtot, itot, vtot+itot);

	s1 = fr_REGION  + fr_GRID  + fr_EXPIRE  + fr_FORCE  + fr_BIG1  + fr_BIG2  + fr_FAIL;
	s2 = fr_REGIONt + fr_GRIDt + fr_EXPIREt + fr_FORCEt + fr_BIG1t + fr_BIG2t + fr_FAILt;
	if (ncdb) fprintf(stderr, "\n");
	if (ncdb) fprintf(stderr, "find_rect:  REGION/GRID/EXPIRE/FORCE - BIG1/BIG2/FAIL  %d/%d/%d/%d - %d/%d/%d  of %d\n",
	    fr_REGION,  fr_GRID,  fr_EXPIRE,  fr_FORCE,  fr_BIG1,  fr_BIG2,  fr_FAIL, s1);
	if (ncdb) fprintf(stderr, "                                       totals:         %d/%d/%d/%d - %d/%d/%d  of %d\n",
	    fr_REGIONt, fr_GRIDt, fr_EXPIREt, fr_FORCEt, fr_BIG1t, fr_BIG2t, fr_FAILt, s2);

	fr_BIG1 = 0;
	fr_BIG2 = 0;
	fr_REGION = 0;
	fr_GRID = 0;
	fr_EXPIRE = 0;
	fr_FORCE = 0;
	fr_FAIL = 0;
	if (ncdb) fprintf(stderr, "\n");
}

#define NSCHED 128
Window sched_bs[NSCHED];
double sched_tm[NSCHED];
double last_sched_bs = 0.0;

#define SCHED(w, v) \
{ \
	int k, save = -1, empty = 1; \
	for (k=0; k < NSCHED; k++) { \
		if (sched_bs[k] == None) { \
			save = k; \
		} \
		if (sched_bs[k] == w) { \
			save = k; \
			empty = 0; \
			break; \
		} \
	} \
	if (save >= 0) { \
		sched_bs[save] = w; \
		if (empty) { \
			sched_tm[save] = dnow(); \
			if (v && ncdb) fprintf(stderr, "SCHED: %d %f\n", save, dnowx()); \
		} \
	} \
}

void xselectinput(Window w, unsigned long evmask, int sync) {
#if NO_X11
	trapped_xerror = 0;
	trapped_xioerror = 0;
	if (!evmask) {}
#else
	XErrorHandler   old_handler1;
	XIOErrorHandler old_handler2;

	if (macosx_console || !dpy) {
		return;
	}

	old_handler1 = XSetErrorHandler(trap_xerror);
	old_handler2 = XSetIOErrorHandler(trap_xioerror);
	trapped_xerror = 0;
	trapped_xioerror = 0;

	XSelectInput(dpy, w, evmask);

	/*
	 * We seem to need to synchronize right away since the window
	 * might go away quickly.
	 */
	if (sync) {
		XSync(dpy, False);
	} else {
		XFlush_wr(dpy);
	}

	XSetErrorHandler(old_handler1);
	XSetIOErrorHandler(old_handler2);
#endif

	if (trapped_xerror) {
		if (ncdb) fprintf(stderr, "XSELECTINPUT: trapped X Error.");
	}
	if (trapped_xioerror) {
		if (ncdb) fprintf(stderr, "XSELECTINPUT: trapped XIO Error.");
	}
if (sync && ncdb) fprintf(stderr, "XSELECTINPUT: 0x%lx  sync=%d err=%d/%d\n", w, sync, trapped_xerror, trapped_xioerror);
}

Bool xcheckmaskevent(Display *d, long mask, XEvent *ev) {
#ifdef MACOSX
	if (macosx_console) {
		if (macosx_checkevent(ev)) {
			return True;
		} else {
			return False;
		}
	}
#endif
	RAWFB_RET(False);

#if NO_X11
	if (!d || !mask) {}
	return False;
#else
	return XCheckMaskEvent(d, mask, ev);
#endif
}

#include <rfb/default8x16.h>

#define EVMAX 2048
XEvent Ev[EVMAX];
int Ev_done[EVMAX];
int Ev_order[EVMAX];
int Ev_area[EVMAX];
int Ev_tmp[EVMAX];
int Ev_tmp2[EVMAX];
Window Ev_tmpwin[EVMAX];
Window Ev_win[EVMAX];
Window Ev_map[EVMAX];
Window Ev_unmap[EVMAX];
sraRect Ev_rects[EVMAX];

int tmp_stack[STACKMAX];
sraRegionPtr tmp_reg[STACKMAX];

#define CLEAN_OUT \
	for (i=0; i < n; i++) { \
		sraRgnDestroy(tmp_reg[i]); \
	} \
	if (r1) sraRgnDestroy(r1); \
	if (r0) sraRgnDestroy(r0);

int try_to_fix_resize_su(Window orig_frame, int orig_x, int orig_y, int orig_w, int orig_h,
    int x, int y, int w, int h, int try_batch) {

	int idx = lookup_win_index(orig_frame);
	sraRegionPtr r0, r1, r2, r3;
	int sx1, sy1, sw1, sh1, dx, dy;
	int bx1, by1, bw1, bh1;
	int nr = 0, *nbat = NULL;

	if (idx < 0) {
		return 0;
	}
	if (cache_list[idx].bs_x < 0 || cache_list[idx].su_time == 0.0) {
		return 0;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	r1 = sraRgnCreateRect(orig_x, orig_y, orig_x+orig_w, orig_y+orig_h);
	r2 = sraRgnCreateRect(x, y, x+w, y+h);

	sraRgnAnd(r1, r0);
	sraRgnAnd(r2, r0);

	if (try_batch) {
		nbat = &nr;
	}
	
	if (orig_w >= w && orig_h >= h) {

if (0) fprintf(stderr, "Shrinking resize %d  %dx%d+%d+%d -> %dx%d+%d+%d\n", idx, orig_w, orig_h, orig_x, orig_y, w, h, x, y);
		r3 = sraRgnCreateRgn(r1);
		sraRgnSubtract(r3, r2);

		sx1 = cache_list[idx].su_x;
		sy1 = cache_list[idx].su_y;
		sw1 = cache_list[idx].su_w;
		sh1 = cache_list[idx].su_h;

		dx = orig_x - sx1;
		dy = orig_y - sy1;

		cache_cr(r3, dx, dy, 0.075, 0.05, nbat);
		sraRgnDestroy(r3);

		r3 = sraRgnCreateRgn(r1);
		sraRgnAnd(r3, r2);

		dx = sx1 - orig_x;
		dy = sy1 - orig_y;
		sraRgnOffset(r3, dx, dy);

		dx = orig_x - x;
		dy = orig_y - y;
		sraRgnOffset(r3, dx, dy);

		cache_cr(r3, dx, dy, 0.075, 0.05, nbat);
		sraRgnDestroy(r3);

		if (nr) {
			batch_push(nr, -1.0);
		}

		cache_list[idx].x = x;
		cache_list[idx].y = y;
		cache_list[idx].width = w;
		cache_list[idx].height = h;

		cache_list[idx].bs_w = w;
		cache_list[idx].bs_h = h;
		cache_list[idx].su_w = w;
		cache_list[idx].su_h = h;

		cache_list[idx].bs_time = 0.0;
		/* XXX Y */
		if (0) cache_list[idx].su_time = dnow();
	} else {
if (0) fprintf(stderr, "Growing resize %d  %dx%d+%d+%d -> %dx%d+%d+%d\n", idx, orig_w, orig_h, orig_x, orig_y, w, h, x, y);

		sx1 = cache_list[idx].su_x;
		sy1 = cache_list[idx].su_y;
		sw1 = cache_list[idx].su_w;
		sh1 = cache_list[idx].su_h;

		bx1 = cache_list[idx].bs_x;
		by1 = cache_list[idx].bs_y;
		bw1 = cache_list[idx].bs_w;
		bh1 = cache_list[idx].bs_h;
		
		if (find_rect(idx, x, y, w, h)) {
			r3 = sraRgnCreateRgn(r2);
			sraRgnAnd(r3, r1);

			dx = cache_list[idx].su_x - x;
			dy = cache_list[idx].su_y - y;
	
			sraRgnOffset(r3, dx, dy);

			dx = dx - (sx1 - orig_x);
			dy = dy - (sy1 - orig_y);
			
			cache_cr(r3, dx, dy, 0.075, 0.05, nbat);
			sraRgnDestroy(r3);

			r3 = sraRgnCreateRgn(r2);
			sraRgnSubtract(r3, r1);

			dx = cache_list[idx].su_x - x;
			dy = cache_list[idx].su_y - y;
	
			sraRgnOffset(r3, dx, dy);

			cache_cr(r3, dx, dy, 0.075, 0.05, nbat);
			sraRgnDestroy(r3);

			if (nr) {
				batch_push(nr, -1.0);
			}

			cache_list[idx].bs_time = 0.0;
			/* XXX Y */
			if (0) cache_list[idx].su_time = dnow();
		}
	}
	
	sraRgnDestroy(r0);
	sraRgnDestroy(r1);
	sraRgnDestroy(r2);

	return 1;
}

int try_to_fix_su(Window win, int idx, Window above, int *nbatch, char *mode) {
	int i, idx2, n = 0, found = 0, found_above = 0; 	
	sraRegionPtr r0, r1, r2;
	Window win2;
	int x, y, w, h, on = 0;
	int x0, y0, w0, h0;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int unmapped = 0;
	int moved = 0;


	if (mode && !strcmp(mode, "unmapped")) {
		unmapped = 1;
	} else if (mode && !strcmp(mode, "moved")) {
		moved = 1;
	}
	if (idx < 0) {
		return 0;
	}
if (ncdb) fprintf(stderr, "TRY_TO_FIX_SU(%d)  0x%lx  0x%lx was_unmapped=%d map_state=%s\n", idx, win, above, unmapped, MState(cache_list[idx].map_state));

	if (cache_list[idx].map_state != IsViewable && !unmapped) {
		return 0;
	}
	if (cache_list[idx].su_time == 0.0) {
		return 0;
	}
	if (cache_list[idx].bs_x < 0) {
		return 0;
	}

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

	x = cache_list[idx].x;
	y = cache_list[idx].y;
	w = cache_list[idx].width;
	h = cache_list[idx].height;

	r1 = sraRgnCreateRect(x, y, x+w, y+h);

	sraRgnAnd(r1, r0);

	if (sraRgnEmpty(r1)) {
		CLEAN_OUT
		return 0;
	}

	if (unmapped) {
		on = 1;
	}
	if (above == 0x1) {
		on = 1;
	}
	for (i = old_stack_n - 1; i >= 0; i--) {
		win2 = old_stack[i];
		if (win2 == above) {
if (0) fprintf(stderr, "0x%lx turn on:  0x%lx  i=%d\n", win, win2, i);
			on = 1;
			found_above = 1;
		}
		if (win2 == win) {
if (0) fprintf(stderr, "0x%lx turn off: 0x%lx  i=%d\n", win, win2, i);
			found = 1;
			on = 0;
			break;
		}
		if (! on) {
			continue;
		}
		idx2 = lookup_win_index(win2);
		if (idx2 < 0) {
			continue;
		}
		if (cache_list[idx2].map_state != IsViewable) {
			continue;
		}
		if (cache_list[idx2].bs_x < 0) {
			continue;
		}
		/* XXX Invalidate? */

		x2 = cache_list[idx2].x;
		y2 = cache_list[idx2].y;
		w2 = cache_list[idx2].width;
		h2 = cache_list[idx2].height;

		r2 = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
		sraRgnAnd(r2, r0);
		if (! sraRgnAnd(r2, r1)) {
			sraRgnDestroy(r2);
			continue;
		}

		tmp_reg[n] = r2;
		tmp_stack[n++] = idx2;
	}

	if (! found) {
		CLEAN_OUT
		return 0;
	}

	for (i = n - 1; i >= 0; i--) {
		int i2;
		r2 = sraRgnCreateRgn(tmp_reg[i]);
		for (i2 = i + 1; i2 < n; i2++)  {
			sraRgnSubtract(r2, tmp_reg[i2]);
		}
		idx2 = tmp_stack[i];
		if (!sraRgnEmpty(r2)) {
			int dx, dy;
			int dx2, dy2;

			x0 = cache_list[idx2].x;
			y0 = cache_list[idx2].y;
			w0 = cache_list[idx2].width;
			h0 = cache_list[idx2].height;

			x1 = cache_list[idx].su_x;	/* SU -> SU */
			y1 = cache_list[idx].su_y;
			w1 = cache_list[idx].su_w;
			h1 = cache_list[idx].su_h;

			x2 = cache_list[idx2].su_x;
			y2 = cache_list[idx2].su_y;
			w2 = cache_list[idx2].su_w;
			h2 = cache_list[idx2].su_h;

			dx = x2 - x0;
			dy = y2 - y0;
			sraRgnOffset(r2, dx, dy);

			dx2 = x1 - x;
			dy2 = y1 - y;
			dx = dx - dx2;
			dy = dy - dy2;
			cache_cr(r2, dx, dy, save_delay0, save_delay1, nbatch);
		}
		sraRgnDestroy(r2);
	}

	if (unmapped) {
		CLEAN_OUT
		return found_above;
	}

	for (i = n - 1; i >= 0; i--) {
		r2 = sraRgnCreateRgn(tmp_reg[i]);
		idx2 = tmp_stack[i];
		if (!sraRgnEmpty(r2)) {
			int dx, dy;
			int dx2, dy2;

			x0 = cache_list[idx2].x;
			y0 = cache_list[idx2].y;
			w0 = cache_list[idx2].width;
			h0 = cache_list[idx2].height;

			x1 = cache_list[idx].su_x;	/* BS -> SU */
			y1 = cache_list[idx].su_y;
			w1 = cache_list[idx].su_w;
			h1 = cache_list[idx].su_h;

			x2 = cache_list[idx2].bs_x;
			y2 = cache_list[idx2].bs_y;
			w2 = cache_list[idx2].bs_w;
			h2 = cache_list[idx2].bs_h;

			dx = x1 - x;
			dy = y1 - y;
			sraRgnOffset(r2, dx, dy);

			dx2 = x2 - x0;
			dy2 = y2 - y0;
			dx = dx - dx2;
			dy = dy - dy2;
			cache_cr(r2, dx, dy, save_delay0, save_delay1, nbatch);
		}
		sraRgnDestroy(r2);
	}

	CLEAN_OUT
	return found_above;
}

void idx_add_rgn(sraRegionPtr r, sraRegionPtr r0, int idx) {
	int x, y, w, h;
	sraRegionPtr rtmp;
	
	if (idx < 0) {
		return;
	}
	x = cache_list[idx].x;
	y = cache_list[idx].y;
	w = cache_list[idx].width;
	h = cache_list[idx].height;

	rtmp = sraRgnCreateRect(x, y, w, h);
	if (r0) {
		sraRgnAnd(rtmp, r0);
	}
	sraRgnOr(r, rtmp);
	sraRgnDestroy(rtmp);
}

sraRegionPtr idx_create_rgn(sraRegionPtr r0, int idx) {
	int x, y, w, h;
	sraRegionPtr rtmp;
	
	if (idx < 0) {
		return NULL;
	}
	x = cache_list[idx].x;
	y = cache_list[idx].y;
	w = cache_list[idx].width;
	h = cache_list[idx].height;

	rtmp = sraRgnCreateRect(x, y, w, h);
	if (r0) {
		sraRgnAnd(rtmp, r0);
	}
	return rtmp;
}

void scale_mark_xrootpmap(void) {
	char *dst_fb, *src_fb = main_fb;
	int dst_bpl, Bpp = bpp/8, fac = 1;
	int yn = (ncache+1) * dpy_y;
	int yfac = (ncache+2);
	int mark = 1;

	if (!scaling || !rfb_fb || rfb_fb == main_fb) {
		mark_rect_as_modified(0, yn, dpy_x, yn + dpy_y, 0);
		return;
	}

	if (cmap8to24 && cmap8to24_fb) {
		src_fb = cmap8to24_fb;
		if (scaling) {
			if (depth <= 8) {
				fac = 4;
			} else if (depth <= 16) {
				fac = 2;
			}
		}
	}
	dst_fb = rfb_fb;
	dst_bpl = rfb_bytes_per_line;

	scale_rect(scale_fac_x, scale_fac_y, scaling_blend, scaling_interpolate, fac * Bpp,
	    src_fb, fac * main_bytes_per_line, dst_fb, dst_bpl, dpy_x, yfac * dpy_y,
	    scaled_x, yfac * scaled_y, 0, yn, dpy_x, yn + dpy_y, mark);
}

void set_ncache_xrootpmap(void) {
	Atom pmap, type;
	int format;
	unsigned long length, after;
	XImage *image = NULL;
	XErrorHandler old_handler;

	RAWFB_RET_VOID
#if !NO_X11
	if (!ncache) {
		return;
	}
	X_LOCK;
	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	pmap = XInternAtom(dpy, "_XROOTPMAP_ID", True);

	if (use_solid_bg) {
		image = solid_image(NULL);
		if (!quiet) {
			rfbLog("set_ncache_xrootpmap: solid_image\n");
		}
	} else if (pmap != None) {
		Pixmap pixmap = None;
		unsigned char *d_pmap;

		XGetWindowProperty(dpy, rootwin, pmap, 0L, 1L, False,
		    AnyPropertyType, &type, &format, &length, &after, &d_pmap);

		if (length != 0) {
			pixmap = *((Pixmap *) d_pmap);
			if (pixmap != None) {
				image = XGetImage(dpy, pixmap, 0, 0, dpy_x, dpy_y, AllPlanes, ZPixmap);
			}
		}
		if (!quiet) {
			rfbLog("set_ncache_xrootpmap: loading background pixmap: 0x%lx\n", pixmap);
		}
	} else {
		if (!quiet) {
			rfbLog("set_ncache_xrootpmap: trying root background\n");
		}
	}
	if (image == NULL) {
		image = solid_root((char *) 0x1);
	}
	if (image != NULL) {
		char *src, *dst;
		int line;
		int pixelsize = bpp/8;
		int y1 = dpy_y * (ncache+1);

		src = image->data;
		dst = main_fb + y1 * main_bytes_per_line;
		line = 0;
		while (line++ < dpy_y) {
			memcpy(dst, src, dpy_x * pixelsize);
			src += image->bytes_per_line;
			dst += main_bytes_per_line;
		}
		XDestroyImage(image);
		X_UNLOCK;
		scale_mark_xrootpmap();
		X_LOCK;
	} else {
		int yn = (ncache+1) * dpy_y;
		zero_fb(0, yn, dpy_x, yn + dpy_y);
	}
	XSetErrorHandler(old_handler);
	X_UNLOCK;
#endif
}

#define EVLISTMAX 256
#define EV_RESET		0
#define EV_CREATE		1
#define EV_DESTROY		2
#define EV_UNMAP		3
#define EV_MAP			4
#define EV_REPARENT		5
#define EV_CONFIGURE		6
#define EV_CONFIGURE_SIZE	7
#define EV_CONFIGURE_POS	8
#define EV_CONFIGURE_STACK	9
#define EV_VISIBILITY_UNOBS	10
#define EV_VISIBILITY_OBS	11
#define EV_PROPERTY		12
#define EV_OLD_WM_MAP		13
#define EV_OLD_WM_UNMAP		14
#define EV_OLD_WM_OFF		15
#define EV_OLD_WM_NOTMAPPED	16
Window _ev_list[EVLISTMAX];
int _ev_case[EVLISTMAX];
int _ev_list_cnt;

int n_CN = 0, n_RN = 0, n_DN = 0, n_ON = 0, n_MN = 0, n_UN = 0;
int n_VN = 0, n_VN_p = 0, n_VN_u = 0, n_ST = 0, n_PN = 0, n_DC = 0;
int n_ON_sz = 0, n_ON_po = 0, n_ON_st = 0;

int ev_store(Window win, int type) {
	if (type == EV_RESET)  {
		n_CN = 0; n_RN = 0; n_DN = 0; n_ON = 0; n_MN = 0; n_UN = 0;
		n_VN = 0; n_VN_p = 0; n_VN_u = 0; n_ST = 0; n_PN = 0; n_DC = 0;
		n_ON_sz = 0; n_ON_po = 0; n_ON_st = 0;
		_ev_list_cnt = 0;
		return 1;
	}
	if (_ev_list_cnt >= EVLISTMAX) {
		return 0;
	}
	_ev_list[_ev_list_cnt] = win;
	_ev_case[_ev_list_cnt++] = type;
	return 1;
}

int ev_lookup(Window win, int type) {
	int i;
	for(i=0; i < _ev_list_cnt; i++) {
		if (_ev_list[i] == win && _ev_case[i] == type) 	{
			return 1;
		}
	}
	return 0;
}

unsigned long all_ev = SubstructureNotifyMask|StructureNotifyMask|VisibilityChangeMask;
unsigned long win_ev = StructureNotifyMask|VisibilityChangeMask;

void read_events(int *n_in) {
	int n = *n_in;
	Window win, win2;
	XEvent ev;
	
	while (xcheckmaskevent(dpy, all_ev, &Ev[n])) {
		int cfg_size = 0;
		int cfg_pos = 0;
		int cfg_stack = 0;
		int type = Ev[n].type; 
		Window w = None;

		win = Ev[n].xany.window;
		Ev_done[n] = 0;
		Ev_area[n] = 0;
		Ev_win[n] = win;
		Ev_map[n] = None;
		Ev_unmap[n] = None;
		Ev_order[n] = n;

		ev = Ev[n];

		if (type == DestroyNotify)  w = Ev[n].xcreatewindow.window;
		if (type == CreateNotify)   w = Ev[n].xdestroywindow.window;
		if (type == ReparentNotify) w = Ev[n].xreparent.window;
		if (type == UnmapNotify)    w = Ev[n].xunmap.window;
		if (type == MapNotify)      w = Ev[n].xmap.window;
		if (type == Expose)         w = Ev[n].xexpose.window;
		if (type == ConfigureNotify) w = Ev[n].xconfigure.window;
		if (type == VisibilityNotify) w = win;
		if (n == *n_in && ncdb) fprintf(stderr, "\n");
		if (1) {
			char *msg = "";
			int idx = -1, x = 0, y = 0, wd = 0, ht = 0;
			if (w != None) {
				idx = lookup_win_index(w);
				if (idx >= 0) {
					x = cache_list[idx].x;
					y = cache_list[idx].y;
					wd = cache_list[idx].width;
					ht = cache_list[idx].height;
				}
			}
			if (type == VisibilityNotify) {
				msg = VState(Ev[n].xvisibility.state);
			} else if (type == ConfigureNotify) {
				int x_new = Ev[n].xconfigure.x; 
				int y_new = Ev[n].xconfigure.y; 
				int w_new = Ev[n].xconfigure.width; 
				int h_new = Ev[n].xconfigure.height; 
				if (idx >= 0) {
					if (w_new != wd || h_new != ht) {
						msg = "change size";
						cfg_size = 1;
					}
					if (x_new != x || y_new != y) {
						if (!strcmp(msg, "")) {
							msg = "change position";
						}
						cfg_pos = 1;
					} else if (! cfg_size) {
						msg = "change stacking";
						cfg_stack = 1;
					}
				}
			}
			
			if (ncdb) fprintf(stderr, "----- %02d inputev 0x%08lx w: 0x%08lx %04dx%04d+%04d+%04d %s  %s\n", n, win, w, wd, ht, x, y, Etype(type), msg);
		}

		if (win == rootwin) {
			if (type == CreateNotify) {
				win2 = ev.xcreatewindow.window;
				ev_store(win2, EV_CREATE);
				n++;
				n_CN++;
			} else if (type == ReparentNotify) {
				if (ev.xreparent.parent != rootwin) {
					win2 = ev.xreparent.window;
					if (win2 != rootwin) {
						ev_store(win2, EV_REPARENT);
					}
				}
				n++;
				n_RN++;
			} else if (type == PropertyNotify) {
				set_prop_atom(Ev[n].xproperty.atom);
				n++;
				n_PN++;
			} else if (type == MapNotify) {
				win2 = ev.xmap.window;
				ev_store(win2, EV_MAP);
				n++;
				n_CN++;
			} else {
				/* skip rest */
#if 0
				Window w = None;
if (type == DestroyNotify) w = Ev[n].xdestroywindow.window;
if (type == UnmapNotify)   w = Ev[n].xunmap.window;
if (type == MapNotify)     w = Ev[n].xmap.window;
if (type == Expose)        w = Ev[n].xexpose.window;
if (type == ConfigureNotify) w = Ev[n].xconfigure.window;
if (type != ConfigureNotify) fprintf(stderr, "root: skip %s  for 0x%lx\n", Etype(type), w);
#endif

			}
		} else {
			if (type == ReparentNotify) {
				ev_store(win, EV_REPARENT);
				n++;
				n_RN++;
			} else if (type == DestroyNotify) {
				ev_store(win, EV_DESTROY);
				n++;
				n_DN++;
			} else if (type == ConfigureNotify) {
				ev_store(win, EV_CONFIGURE);
				if (cfg_size) {
					ev_store(win, EV_CONFIGURE_SIZE);
					n_ON_sz++;
				}
				if (cfg_pos) {
					ev_store(win, EV_CONFIGURE_POS);
					n_ON_po++;
				}
				if (cfg_stack) {
					ev_store(win, EV_CONFIGURE_STACK);
					n_ON_st++;
				}
				n++;
				n_ON++;
			} else if (type == VisibilityNotify) {
				if (Ev[n].xvisibility.state == VisibilityUnobscured) {
					ev_store(win, EV_VISIBILITY_UNOBS);
					n_VN_u++;
				} else {
					ev_store(win, EV_VISIBILITY_OBS);
					n_VN_p++;
				}
				n++;
				n_VN++;
			} else if (type == MapNotify) {
				ev_store(win, EV_MAP);
				Ev_map[n] = win;
				n++;
				n_MN++;
			} else if (type == UnmapNotify) {
				ev_store(win, EV_UNMAP);
				Ev_unmap[n] = win;
				n++;
				n_UN++;
			} else {
				/* skip rest */
if (ncdb) fprintf(stderr, "----- skip %s\n", Etype(type));
			}
		}
		if (n >= EVMAX) {
			break;
		}
	}
	*n_in = n;
}

int try_to_synthesize_su(int force, int urgent, int *nbatch) {
	int i, idx, idx2, n = 0; 	
	sraRegionPtr r0, r1, r2;
	Window win = None;
	int x0, y0, w0, h0;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int x3, y3, w3, h3;
	XWindowAttributes attr;

	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

	snap_old();

	X_LOCK;
	for (i = old_stack_n - 1; i >= 0; i--) {
		win = old_stack[i];
		if (urgent) {	/* XXX Y resp */
			if (!valid_window(win, &attr, 1)) {
				continue;
			}
			idx = lookup_win_index(win);
			if (idx >= 0) {
				STORE(idx, win, attr);
			}
		} else {
			idx = lookup_win_index(win);
			if (idx >= 0) {
				attr.map_state = cache_list[idx].map_state;
				attr.x = cache_list[idx].x;
				attr.y = cache_list[idx].y;
				attr.width = cache_list[idx].width;
				attr.height = cache_list[idx].height;
			} else {
				attr.map_state = IsUnmapped;
				attr.x = 0;
				attr.y = 0;
				attr.width = 0;
				attr.height = 0;
			}
			
		}
		if (attr.map_state != IsViewable) {
			continue;
		}
if (0) fprintf(stderr, "win: 0x%lx %d  idx=%d\n", win, i, idx);

		x2 = attr.x;
		y2 = attr.y;
		w2 = attr.width;
		h2 = attr.height;

		r2 = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
		sraRgnAnd(r2, r0);

		tmp_reg[n] = r2;
		tmp_stack[n++] = idx;
	}
	X_UNLOCK;

	if (! n) {
		r1 = NULL;
		CLEAN_OUT
		return 0;
	}

	for (i = 0; i < n; i++) {
		int i2, cnt = 0;
		idx = tmp_stack[i];
		if (idx < 0 || cache_list[idx].bs_x < 0) {
			continue;
		}
		r1 = tmp_reg[i];
		if (r1 == NULL || sraRgnEmpty(r1)) {
			continue;
		}
		if (cache_list[idx].su_time > 0.0) {
			if (force) {
if (ncdb) fprintf(stderr, "forcing synth: 0x%lx %d\n", cache_list[idx].win, idx);
			} else {
				continue;
			}
		}
		if (ncache_xrootpmap) {
			int dx, dy;

			x0 = cache_list[idx].x;
			y0 = cache_list[idx].y;
			w0 = cache_list[idx].width;
			h0 = cache_list[idx].height;

			x1 = cache_list[idx].su_x;
			y1 = cache_list[idx].su_y;
			w1 = cache_list[idx].su_w;
			h1 = cache_list[idx].su_h;

			r2 = sraRgnCreateRgn(tmp_reg[i]);
			dx = x1 - x0;
			dy = y1 - y0;

			sraRgnOffset(r2, dx, dy);

			x2 = x0;
			y2 = y0 + (ncache+1) * dpy_y;

			dx = x1 - x2;
			dy = y1 - y2;
			cache_cr(r2, dx, dy, save_delay0, save_delay1, nbatch);
			cnt++;

			sraRgnDestroy(r2);
		}

		for (i2 = n - 1; i2 > i; i2--) {
			r2 = sraRgnCreateRgn(tmp_reg[i2]);
			if (sraRgnAnd(r2, r1)) {
				int dx, dy;
				int dx2, dy2;

				idx2 = tmp_stack[i2];
				/* XXX Y */
				if (idx2 < 0 || cache_list[idx2].bs_x < 0 || cache_list[idx2].bs_time == 0.0) {
					continue;
				}

				x0 = cache_list[idx].x;
				y0 = cache_list[idx].y;
				w0 = cache_list[idx].width;
				h0 = cache_list[idx].height;

				x1 = cache_list[idx].su_x;
				y1 = cache_list[idx].su_y;
				w1 = cache_list[idx].su_w;
				h1 = cache_list[idx].su_h;

				x2 = cache_list[idx2].x;
				y2 = cache_list[idx2].y;
				w2 = cache_list[idx2].width;
				h2 = cache_list[idx2].height;

				x3 = cache_list[idx2].bs_x;
				y3 = cache_list[idx2].bs_y;
				w3 = cache_list[idx2].bs_w;
				h3 = cache_list[idx2].bs_h;

				dx = x1 - x0;
				dy = y1 - y0;
				sraRgnOffset(r2, dx, dy);

				dx2 = x3 - x2;
				dy2 = y3 - y2;
				dx = dx - dx2;
				dy = dy - dy2;
				cache_cr(r2, dx, dy, save_delay0, save_delay1, nbatch);
				cnt++;
			}
			sraRgnDestroy(r2);
		}
		if (cnt) {
			cache_list[idx].su_time = dnow();
		}
if (ncdb) fprintf(stderr, "  try_to_synth_su: 0x%lx %d  idx=%d cnt=%d\n", win, i, idx, cnt);
	}

	r1 = NULL;
	CLEAN_OUT
	return 1;
}

static double last_vis_unobs_time = 0.0;
static double last_vis_obs_time = 0.0;

static int saw_desktop_change = 0;

void check_sched(int try_batch, int *did_sched) {
	static double last_root = 0.0;
	static double last_pixmap = 0.0;
	double refresh = 60.0;
	int i, k, valid;
	Window win;
	XWindowAttributes attr;
	double now = dnow();

	if (now > last_root + refresh) {

if (ncdb) fprintf(stderr, "\n**** checking cache_list[%d]\n\n", cache_list_num);
		block_stats();

		for(k=0; k<cache_list_num; k++) {
			valid = 0;
			win = cache_list[k].win; 
			X_LOCK;
			if (win == None) {
				;
			} else if (cache_list[k].selectinput && cache_list[k].time > now - refresh) {
				valid = 1;
			} else if (valid_window(win, &attr, 1)) {
				STORE(k, win, attr);
				if (! cache_list[k].selectinput) {
					xselectinput(win, win_ev, 0);
					CLEAR(k);
					cache_list[k].selectinput = 1;
				}
				valid = 1;
			} else {
if (ncdb) fprintf(stderr, "DELETE(%d) %dx%d+%d+%d\n", k, cache_list[k].width, cache_list[k].height, cache_list[k].x, cache_list[k].y);
				DELETE(k);
			}
			X_UNLOCK;
/* XXX Y */
			if (valid) {
				if (cache_list[k].create_cnt && cache_list[k].map_state != IsViewable && cache_list[k].map_cnt == 0) {
					if (cache_list[k].bs_x >= 0) {
if (ncdb) fprintf(stderr, "Created window never mapped: freeing(%d) 0x%lx\n", k, win);
						free_rect(k);
					}
				}
			}
		}
		last_root = dnow();
	}

	if (now > last_sched_bs + 0.30) {
		static double last_sched_vis = 0.0;
		int nr = 0, *bat = NULL;

		if (try_batch) {
			bat = &nr;
		}
		if (now < last_wireframe + 2.0) {
			for (i=0; i < NSCHED; i++) {
				sched_bs[i] = None;
			}
		}
		if (now < last_get_wm_frame_time + 1.0) {
			if (last_get_wm_frame != None) {
				int idx = lookup_win_index(last_get_wm_frame);
				if (idx >= 0) {
					if (cache_list[idx].bs_x < 0) {
						int x = cache_list[idx].x;
						int y = cache_list[idx].y;
						int w = cache_list[idx].width;
						int h = cache_list[idx].height;
						if (find_rect(idx, x, y, w, h)) {
							SCHED(last_get_wm_frame, 1);
						}
					}
				}
			}
		}
		
		for (i=0; i < NSCHED; i++) {
			if (sched_bs[i] != None) {
				int idx;
				win = sched_bs[i];	
				if (now < sched_tm[i] + 0.55) {
					continue;
				}
				if (n_MN || n_UN || n_ST || n_DC) {
					sched_tm[i] = now;
					continue;
				}
				idx = lookup_win_index(win);
				if (idx >= 0) {
					int aw = cache_list[idx].width; 
					int ah = cache_list[idx].height; 
					if (cache_list[idx].map_state != IsViewable) {
						;
					} else if (cache_list[idx].vis_state != VisibilityUnobscured) {
						;
					} else if (aw * ah < 64 * 64) {
						;
					} else {
if (ncdb) fprintf(stderr, "*SNAP BS_save: 0x%lx %d %d %d\n", win, aw, ah, cache_list[idx].map_state); 
						valid = 0;
						bs_save(idx, bat, &attr, 1, 0, &valid, 0);
						if (valid) {
							STORE(idx, win, attr);
						} else {
							DELETE(idx);
						}
					}
				} else {
if (ncdb) fprintf(stderr, "*SCHED LOOKUP FAIL: i=%d 0x%lx\n", i, win);
				}
			}
			sched_bs[i] = None;
		}
		*did_sched = 1;

		if (n_MN || n_UN || n_ST || n_DC) {
			if (last_sched_vis < now) {
				last_sched_vis += 1.0;
			}
		} else if (now > last_sched_vis + 3.0 && now > last_wireframe + 2.0) {
			static double last_vis = 0.0;
			int vis_now[32], top_now[32];
			static int vis_prev[32], freq = 0;
			int diff, nv = 32, vis_now_n = 0;
			Window win;

			freq++;

			for (i=0; i < cache_list_num; i++) {
				int ok = 0;
				int top_only = 1;
				int aw = cache_list[i].width; 
				int ah = cache_list[i].height; 
				int map_prev = cache_list[i].map_state;

				win = cache_list[i].win;

				if (saw_desktop_change) {
					top_only = 0;
				}

				if (win == None) {
					continue;
				}
				/* XXX Y resp */
				if (saw_desktop_change || freq % 5 == 0) {
					int vret = 0;
					X_LOCK;
					vret = valid_window(win, &attr, 1);
					X_UNLOCK;
					if (!vret) {
						continue;
					}
					STORE(i, win, attr);
				}
				if (!cache_list[i].valid) {
					continue;
				}
				if (cache_list[i].map_state != IsViewable) {
					continue;
				}
				if (cache_list[i].vis_state == VisibilityFullyObscured) {
					continue;
				}
				if (map_prev != IsViewable) {
					/* we hope to catch it below in the normal event processing */
					continue;
				}
				if (aw * ah < 64 * 64) {
					continue;
				}
				if (top_only) {
					if (cache_list[i].vis_state == VisibilityUnobscured) {
						ok = 1;
					} else if (!clipped(i)) {
						ok = 1;
					}
				} else {
					ok = 1;
				}
				if (ok) {
					if (vis_now_n < nv) {
						vis_now[vis_now_n] = i;
						top_now[vis_now_n++] = top_only;
					}
				}
			}
			diff = 0;
			for (k = 0; k < vis_now_n; k++) {
				if (vis_now[k] != vis_prev[k]) {
					diff = 1;
				}
			}
			if (diff == 0) {
				if (now > last_vis + 45.0) {
					diff = 1;
				}
			}
			if (diff) {
if (ncdb && vis_now_n) fprintf(stderr, "*VIS  snapshot all %.4f\n", dnowx());
				for (k = 0; k < vis_now_n; k++) {
					i = vis_now[k];
					win = cache_list[i].win;
					valid = 0;
if (ncdb) fprintf(stderr, "*VIS  BS_save: 0x%lx %d %d %d\n", win, cache_list[i].width, cache_list[i].height, cache_list[i].map_state); 
					if (now < cache_list[i].vis_unobs_time + 0.75 && now < cache_list[i].vis_obs_time + 0.75) {
						continue;
					}
					bs_save(i, bat, &attr, !top_now[k], 0, &valid, 1);
					if (valid) {
						STORE(i, win, attr);
					} else {
						DELETE(i);
					}
					vis_prev[k] = vis_now[k];
				}
				last_vis = dnow();
			}
			last_sched_vis = dnow();
			if (! n_DC) {
				saw_desktop_change = 0;
			}
			/* XXX Y */
			try_to_synthesize_su(0, 0, bat);
		}

		if (nr) {
			batch_push(nr, -1.0);
		}
		last_sched_bs = dnow();
	}
#if !NO_X11
	if (dpy && atom_XROOTPMAP_ID == None && now > last_pixmap + 5.0) {
		atom_XROOTPMAP_ID = XInternAtom(dpy, "_XROOTPMAP_ID", True);
		last_pixmap = now;
	}
#endif
	if (got_XROOTPMAP_ID > 0.0) {
if (ncdb) fprintf(stderr, "got_XROOTPMAP_ID\n");
		if (ncache_xrootpmap) {
			set_ncache_xrootpmap();
		}
		got_XROOTPMAP_ID = 0.0;
	}
}

int check_ncache(int reset, int mode) {
	static int first = 1;
	static int last_client_count = -1;
	int i, k, n; 
	int did_sched = 0;

	Window win, win2;
	XWindowAttributes attr;
	int valid;
	int try_batch = 1; /* XXX Y */
	int use_batch = 0;
	int nreg = 0, *nbatch;
	int create_cnt;
	int su_fix_cnt;
	int pixels = 0, ttot;
	int desktop_change = 0, n1, n2;
	int desktop_change_old_wm = 0;
	int missed_su_restore = 0;
	int missed_bs_restore = 0;
	sraRegionPtr r0, r;
	sraRegionPtr missed_su_restore_rgn;
	sraRegionPtr missed_bs_restore_rgn;
	sraRegionPtr unmapped_rgn;

	int nrects = 0;
	int nsave, nxsel;
	double now;

	int skipwins_n = 0;
	int skipwins_max = 256;
	Window skipwins[256];

	static char *dt_guess = NULL;
	static double dt_last = 0.0;
	int dt_gnome = 0, gnome_animation = 0;
	int dt_kde = 0;

	if (unixpw_in_progress) return -1;

#ifdef MACOSX
	if (! macosx_console) {
		RAWFB_RET(-1)
	}
	if (! screen) {
		return -1;
	}
#else
	RAWFB_RET(-1)
	if (! screen || ! dpy) {
		return -1;
	}
#endif

	now = dnow();

#ifdef NO_NCACHE
	ncache = 0;
#endif

	if (reset && (first || cache_list_len == 0)) {
		return -1;
	}
	if (use_threads) {
		try_batch = 0;
	}

	if (ncache0) {
		if (reset) {
			;
		} else if (!client_count || !ncache || nofb) {
			static double last_purge = 0.0;
			double delay = client_count ? 0.5 : 2.0;
			if (now > last_purge + delay) {
				int c = 0;
				XEvent ev;
				X_LOCK;
				while (xcheckmaskevent(dpy, all_ev, &ev)) {
					c++;
				}
				X_UNLOCK;
				last_purge = dnow();
if (ncdb && c) fprintf(stderr, "check_ncache purged %d events\n", c); 
			}
			if (!client_count && last_client_count >= 0 &&
			    client_count != last_client_count) {
				/* this should use less RAM when no clients */
				do_new_fb(1);
			}
			last_client_count = client_count;
			return -1;
		}
	}
	last_client_count = client_count;

	if (ncache && ! ncache0) {
		ncache0 = ncache;
	}

	if (! ncache || ! ncache0) {
		return -1;
	}
	if (subwin) {
		return -1;
	}
	if (nofb) {
		return -1;
	}
	if (now < last_client + 4) {
		return -1;
	}
	if (! all_clients_initialized()) {
		/* play it safe */
		return -1;
	}



	if (reset) {
		rfbLog("check_ncache: resetting cache: %d/%d %d %d\n", cache_list_num, cache_list_len, ncache, first);
		for (i=0; i < cache_list_num; i++) {
			free_rect(i);
		}
		for (n = 1; n <= ncache; n++) {
			if (rect_reg[n] != NULL) {
				sraRgnDestroy(rect_reg[n]);
				rect_reg[n] = NULL;
			}
		}
		zero_fb(0, dpy_y, dpy_x, (ncache+1)*dpy_y);
		mark_rect_as_modified(0, dpy_y, dpy_x, (ncache+1)*dpy_y, 0);

		if (ncache_xrootpmap) {
			set_ncache_xrootpmap();
		}

		snap_old();
		return -1;
	}

	if (first) {
		int dx = 10, dy = 24, ds = 0;
		int Dx = dpy_x, Dy = dpy_y;
		first = 0;
		for (i=0; i < NRECENT; i++) {
			recent[i] = None;
		}
		for (i=0; i < NSCHED; i++) {
			sched_bs[i] = None;
		}
		rlast = 0;

		X_LOCK;
		/* event leak with client_count == 0 */
		xselectinput_rootwin |= SubstructureNotifyMask;
		XSelectInput_wr(dpy, rootwin, xselectinput_rootwin);
		X_UNLOCK;

		if (scaling) {
			Dx = scaled_x;
			Dy = scaled_y;
		}
		if (!rotating_same) {
			int t = Dx;
			Dx = Dy;
			Dy = t;
		}

		for (i = 0; i < 3; i++) {
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+1*dy,
			    "This is the Pixel buffer cache region. Your VNC Viewer is not hiding it from you.",
			    white_pixel());
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+2*dy,
			    "Try resizing your VNC Viewer so you don't see it!!",
			    white_pixel());
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+3*dy,
			    "Pay no attention to the man behind the curtain...",
			    white_pixel());
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+4*dy,
			    "To disable caching run the server with:  x11vnc -noncache ...",
			    white_pixel());
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+5*dy,
			    "If there are painting errors press 3 Alt_L's (Left \"Alt\" key) in a row to repaint the screen.",
			    white_pixel());
			rfbDrawString(screen, &default8x16Font, dx, ds + Dy+6*dy,
			    "More info:  http://www.karlrunge.com/x11vnc/faq.html#faq-client-caching",
			    white_pixel());

			ds += 11 * dy;
		}

		snapshot_cache_list(0, 100.0);
		for (i=0; i < cache_list_num; i++) {
			CLEAR(i);
		}
		for (n = 1; n <= ncache; n++) {
			rect_reg[n] = NULL;
		}

		if (ncache_xrootpmap) {
			set_ncache_xrootpmap();
		}

		snap_old();
	}

	check_zero_rects();

if (hack_val == 2) {
	block_stats();
	hack_val = 1;
}
#ifdef MACOSX
	if (macosx_console) {
		static double last_all_windows = 0.0;
		if (! macosx_checkevent(NULL)) {
			if (now > last_all_windows + 0.05) {
				macosxCGS_get_all_windows();
				last_all_windows = dnow();
			}
		}
		/* XXX Y */
		rootwin = -1;
	}
#endif

	n = 0;
	ttot = 0;

	if (dt_guess == NULL || now > dt_last + 60) {
		static char *dt_prev = NULL;
		dt_prev = dt_guess;
		dt_guess = strdup(guess_desktop());
		if (ncache_xrootpmap && dt_prev && dt_guess) {
			if (strcmp(dt_prev, dt_guess)) {
				set_ncache_xrootpmap();
			}
		}
		dt_last = now;
		if (dt_prev) {
			free(dt_prev);
		}
	}
	if (dt_guess && !strcmp(dt_guess, "gnome")) {
		dt_gnome = 1;
	} else if (dt_guess && !strcmp(dt_guess, "kde")) {
		dt_kde = 1;
	}
	if (dt_kde) {
		kde_no_animate(0);
	}

	ev_store(None, EV_RESET);

	X_LOCK;
	for (k = 1; k <= 3; k++) {
		int j, retry = 0;

		if (retry) {}

		nsave = n;

		if (k > 1 && ncdb) fprintf(stderr, "read_events-%d\n", k);
		read_events(&n);

#if 0
		if (dt_gnome && (n_MN || n_UN)) {
			retry = 1;
		} else if (ncache_old_wm && n_ON_po >= 2) {
			retry = 1;
		} else if (n > nsave) {
			/* XXX Y */
			retry = 1;
		}

		if (retry) {
			int n0 = n;
			usleep(25 * 1000);
			XFlush_wr(dpy);
			read_events(&n);
			if (ncdb) fprintf(stderr, "read_events retry: %d -> %d\n", n0, n);
		}
#endif

		if (n > nsave) {
			int n0 = n;

			for (j=0; j<4; j++) {
				if (j < 2) {
					usleep(30 * 1000);
				} else {
					usleep(10 * 1000);
				}
				XFlush_wr(dpy);
				read_events(&n);
				if (ncdb) fprintf(stderr, "read_events retry: %d -> %d\n", n0, n);
				if (n == n0) {
					break;
				}
				n0 = n;
			}
		}
			
		nxsel = 0;

		/* handle creates and reparenting: */
		for (n1 = nsave; n1 < n; n1++) {
			Window win2;
			int idx;
			XEvent ev = Ev[n1];
			win = Ev_win[n1];
			if (ev.type == CreateNotify) {
				win2 = ev.xcreatewindow.window;
				if (ev_lookup(win2, EV_REPARENT) || ev_lookup(win2, EV_DESTROY)) {
					if (skipwins_n < skipwins_max) {
if (ncdb) fprintf(stderr, "SKIPWINS: CreateNotify: 0x%lx %d\n", win2, n1);
						skipwins[skipwins_n++] = win2;
					}
				} else {
					idx = lookup_win_index(win);
					if (idx < 0) {
						idx = lookup_free_index();
						if (idx < 0) {
							continue;
						}
						CLEAR(idx);
					}
if (ncdb) fprintf(stderr, "PRELOOP:  CreateNotify: 0x%lx %d valid_window\n", win2, n1);
					if (valid_window(win2, &attr, 1)) {
						STORE(idx, win2, attr);
						CLEAR(idx);
						cache_list[idx].selectinput = 1;
						cache_list[idx].create_cnt = 1;
if (ncdb) fprintf(stderr, "PRELOOP:  CreateNotify: 0x%lx %d xselectinput\n", win2, n1);
						xselectinput(win2, win_ev, 1);
						nxsel++;
					} else {
						DELETE(idx);
					}
					nxsel++;
				}
			} else if (ev.type == ReparentNotify) {
				if (ev.xreparent.parent != rootwin) {
					win2 = ev.xreparent.window;
					if (win2 != rootwin) {
						idx = lookup_win_index(win2);
if (ncdb) fprintf(stderr, "PRELOOP:  RepartNotify: 0x%lx %d idx=%d\n", win2, n1, idx);
						if (idx >= 0) {
							DELETE(idx);
						}
						if (! ev_lookup(win2, EV_CREATE)) {
							xselectinput(win2, 0, 1);
							nxsel++;
						}
					}
				}
			}
		}
		if (nxsel == 0) {
			break;
		}
	}

	X_UNLOCK;

	if (got_NET_CURRENT_DESKTOP > 0.0) {
		if (dnow() < got_NET_CURRENT_DESKTOP + 0.25) {
			if (ncdb) fprintf(stderr, "***got_NET_CURRENT_DESKTOP n=%d\n", n);
			desktop_change = 1;
			n_DC++;
		} else {
			if (ncdb) fprintf(stderr, "***got_NET_CURRENT_DESKTOP n=%d STALE\n", n);
		}
		got_NET_CURRENT_DESKTOP = 0.0;
	}

	if (n == 0) {
		check_sched(try_batch, &did_sched);
		return 0;
	}
if (ncdb) fprintf(stderr, "\n"); if (ncdb) rfbLog("IN  check_ncache() %d events.  %.4f\n", n, now - x11vnc_start);

	if (try_batch) {
		use_batch = 1;
	}

	if (rotating) {
		use_batch = 0;
	}
	if (cursor_noshape_updates_clients(screen)) {
		use_batch = 0;
	}

	if (! use_batch) {
		nbatch = NULL;
	} else {
		nreg = 0;
		nbatch = &nreg;
	}

	/* XXX Y */
	for (n1 = 0; n1 < n; n1++) {
		Window twin = Ev_map[n1];
		if (twin == None || twin == rootwin) {
			continue;
		}
		for (n2 = 0; n2 < n; n2++) {
			if (Ev_unmap[n2] == twin) {
				if (skipwins_n < skipwins_max) {
if (ncdb) fprintf(stderr, "SKIPWINS: Ev_unmap/map: 0x%lx %d\n", twin, n2);
					skipwins[skipwins_n++] = twin;
					break;
				}
			}
		}
	}

	if (!desktop_change) {
		if (skipwins_n) {
			if (n_MN + n_UN >= 2 + 2*skipwins_n) {
				desktop_change = 1;
				n_DC++;
			}
		} else {
			if (n_MN + n_UN >= 3) {
				desktop_change = 1;
				n_DC++;
			}
		}
	}
	if (ncache_old_wm) {
		int shifts = 0;
		for (i=0; i < n; i++) {
			XEvent ev;
			int type, idx = -1;
			int ik = Ev_order[i];
			int x_new, y_new, w_new, h_new;
			int x_old, y_old, w_old, h_old;
			int old_wm = 0;

			if (Ev_done[ik]) continue;
			win = Ev_win[ik];

			ev = Ev[ik];
			type = ev.type;
			if (type != ConfigureNotify) {
				continue;
			}
			if (ev_lookup(win, EV_MAP)) {
				continue;
			} else if (ev_lookup(win, EV_UNMAP)) {
				continue;
			} else if (ev_lookup(win, EV_DESTROY)) {
				continue;
			}

			idx = lookup_win_index(win);
			if (idx < 0) {
				continue;
			}
			x_new = ev.xconfigure.x; 
			y_new = ev.xconfigure.y; 
			w_new = ev.xconfigure.width; 
			h_new = ev.xconfigure.height; 

			x_old = cache_list[idx].x;
			y_old = cache_list[idx].y;
			w_old = cache_list[idx].width;
			h_old = cache_list[idx].height;

			if (w_new == w_old && h_new == h_old) {
				if (nabs(x_new - x_old) >= dpy_x || nabs(y_new - y_old) >= dpy_y) {
					sraRegionPtr r_old, r_new, r0;
					r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
					r_old = sraRgnCreateRect(x_old, y_old, x_old+w_old, y_old+h_old);
					sraRgnAnd(r_old, r0);
					r_new = sraRgnCreateRect(x_new, y_new, x_new+w_new, y_new+h_new);
					sraRgnAnd(r_new, r0);
					if (cache_list[idx].map_state != IsViewable) {
						ev_store(win, EV_OLD_WM_NOTMAPPED);
					} else if (sraRgnEmpty(r_old) && !sraRgnEmpty(r_new)) {
						old_wm = 1;
						ev_store(win, EV_OLD_WM_MAP);
						Ev_map[i] = win;
					} else if (!sraRgnEmpty(r_old) && sraRgnEmpty(r_new)) {
						ev_store(win, EV_OLD_WM_UNMAP);
						old_wm = -1;
						Ev_unmap[i] = win;
					} else {
						ev_store(win, EV_OLD_WM_OFF);
					}
					sraRgnDestroy(r_old);
					sraRgnDestroy(r_new);
					sraRgnDestroy(r0);
					shifts++;
if (ncdb) fprintf(stderr, "old_wm[%d]  +%04d+%04d  +%04d+%04d  old_wm: %d\n", i, x_old, y_old, x_new, y_new, old_wm);
				}
			}
		}
		if (shifts >= 3) {
if (ncdb) fprintf(stderr, "DESKTOP_CHANGE_OLD_WM: %d\n", shifts);
			desktop_change = 1;
			desktop_change_old_wm = 1;
		}
	}

#define SKIPUMS \
	ok = 1; \
	if (twin == None || twin == rootwin) { \
		continue; \
	} \
	for (ns = 0; ns < skipwins_n; ns++) { \
		if (skipwins[ns] == twin) { \
			ok = 0; \
			break; \
		} \
	}

	if (desktop_change) {
		Window twin;
		int ok, s, k, add, cnt, ns;

		cnt = 0;
		add = 0;
		for (i=0; i < n; i++) {
			twin = Ev_unmap[i];
			SKIPUMS
			if (ok) {
if (ncdb) fprintf(stderr, "U Ev_tmp[%d] = %d\n", cnt, i);
				Ev_tmp[cnt++] = i;
			}
		}
		for (i=0; i < n; i++) {
			twin = Ev_map[i];
			SKIPUMS
			if (ok) {
if (ncdb) fprintf(stderr, "M Ev_tmp[%d] = %d\n", cnt, i);
				Ev_tmp[cnt++] = i;
			}
		}
		for (k = 0; k < cnt; k++) {
			Ev_tmp2[k] = -1;
		}
		/* unmap from top to bottom */
		for (s = old_stack_n - 1; s >= 0; s--) {
			twin = old_stack[s];	
			if (twin == None || twin == rootwin) {
				continue;
			}
			for (k = 0; k < cnt; k++) {
				i = Ev_tmp[k];
				if (twin == Ev_unmap[i]) {
if (ncdb) fprintf(stderr, "U Ev_tmp2[%d] = %d\n", add, i);
					Ev_tmp2[add++] = i;
					break;
				}
			}
		}
		/* map from bottom to top */
		for (s = 0; s < old_stack_n; s++) {
			twin = old_stack[s];	
			if (twin == None || twin == rootwin) {
				continue;
			}
			for (k = 0; k < cnt; k++) {
				i = Ev_tmp[k];
				if (twin == Ev_map[i]) {
if (ncdb) fprintf(stderr, "M Ev_tmp2[%d] = %d\n", add, i);
					Ev_tmp2[add++] = i;
					break;
				}
			}
		}
		k = 0;
		for (i=0; i < n; i++) {
			Window wu, wm;
			int j;
			int oku = 0, okm = 0;
			wu = Ev_unmap[i];
			wm = Ev_map[i];
			ok = 0;
			if (wu != None && wu != rootwin) oku = 1;
			if (wm != None && wm != rootwin) okm = 1;
			if (!oku && !okm) {
				continue;
			}
			if (oku) {
				twin = wu;
				SKIPUMS
				if (!ok) {
					oku = 0;
				}
			}
			if (okm) {
				twin = wm;
				SKIPUMS
				if (!ok) {
					okm = 0;
				}
			}
			if (!oku && !okm) {
				continue;
			}
			j = Ev_tmp2[k++];
			if (j >= 0) {
if (ncdb) fprintf(stderr, "UM Ev_order[%d] = %d oku=%d okm=%d\n", i, j, oku, okm);
				Ev_order[i] = j;
			}
		}
	}

#if 0
	if (desktop_change) {
		Window twin;
		int ok, s, k, add, cnt, ns;

		cnt = 0;
		add = 0;
		for (i=0; i < n; i++) {
			twin = Ev_unmap[i];
			SKIPUMS
			if (ok) {
				Ev_tmp[cnt++] = i;
			}
		}
		for (k = 0; k < cnt; k++) {
			Ev_tmp2[k] = -1;
		}
		/* unmap from top to bottom */
		for (s = old_stack_n - 1; s >= 0; s--) {
			twin = old_stack[s];	
			for (k = 0; k < cnt; k++) {
				i = Ev_tmp[k];
				if (twin == Ev_unmap[i]) {
					Ev_tmp2[add++] = i;
					break;
				}
			}
		}
		k = 0;
		for (i=0; i < n; i++) {
			int j;
			twin = Ev_unmap[i];
			SKIPUMS
			if (ok) {
				j = Ev_tmp2[k++];
				if (j >= 0) {
					Ev_order[i] = j;
				}
			}
		}

		cnt = 0;
		add = 0;
		for (i=0; i < n; i++) {
			twin = Ev_map[i];
			SKIPUMS
			if (ok) {
				Ev_tmp[cnt++] = i;
			}
		}
		for (k = 0; k < cnt; k++) {
			Ev_tmp2[k] = -1;
		}
		/* map from bottom to top */
		for (s = 0; s < old_stack_n; s++) {
			twin = old_stack[s];	
			for (k = 0; k < cnt; k++) {
				i = Ev_tmp[k];
				if (twin == Ev_map[i]) {
					Ev_tmp2[add++] = i;
					break;
				}
			}
		}
		k = 0;
		for (i=0; i < n; i++) {
			int j;
			twin = Ev_map[i];
			SKIPUMS
			if (ok) {
				j = Ev_tmp2[k++];
				if (j >= 0) {
					Ev_order[i] = j;
				}
			}
		}
	}
#endif

	if (!desktop_change && (n_VN_p && !n_UN && (n_MN || n_ON_st))) {
		if (now < last_vis_unobs_time + 0.75 || now < last_vis_obs_time + 0.75) {
			;
		} else if (n_MN <= 2 && n_ON_st <= 1) {
			for (i=0; i < n; i++) {
				XEvent ev;
				int type, idx = -1, state, valid;
				int ik = Ev_order[i];

				if (Ev_done[ik]) continue;
				win = Ev_win[ik];

				ev = Ev[ik];
				type = ev.type;
				if (type != VisibilityNotify) {
					continue;
				}

				state = ev.xvisibility.state;
				if (state == VisibilityUnobscured) {
					continue;
				}
				if (ev_lookup(win, EV_MAP)) {
					continue;
				} else if (ev_lookup(win, EV_UNMAP)) {
					continue;
				} else if (ev_lookup(win, EV_DESTROY)) {
					continue;
				}
				idx = lookup_win_index(win);

				if (idx < 0) {
					continue;
				}
				if (cache_list[idx].vis_state == VisibilityFullyObscured) {
					continue;
				}
				if (now < cache_list[idx].vis_unobs_time + 3.00 || now < cache_list[idx].vis_obs_time + 3.00) {
					continue;
				}

if (ncdb) fprintf(stderr, "----%02d: VisibilityNotify 0x%lx  %3d  (*PRELOOP*) state: %s U/P %d/%d\n", ik, win, idx, VState(state), n_VN_u, n_VN_p);
				valid = 0;
				bs_save(idx, nbatch, &attr, 1, 0, &valid, 1);
				if (valid) {
					STORE(idx, win, attr);
				} else {
					DELETE(idx);
				}

				cache_list[idx].vis_state = state;
				cache_list[idx].vis_obs_time = last_vis_obs_time = dnow();
				Ev_done[ik] = 1;
			}
		}
	}
	if (desktop_change) {
		if (ncache_dt_change) {
			if (ncdb) fprintf(stderr, "GUESSED DESKTOP CHANGE.\n");
			saw_desktop_change = 1;
		} else {
			if (ncdb) fprintf(stderr, "GUESSED DESKTOP CHANGE. Skipping.\n");
			desktop_change = 0;
		}
	}


	create_cnt = 0;
	missed_su_restore = 0;
	missed_bs_restore = 0;
	missed_su_restore_rgn = sraRgnCreate();
	missed_bs_restore_rgn = sraRgnCreate();
	r0 = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
	unmapped_rgn = sraRgnCreate();
	su_fix_cnt = 0;

for (k = 0; k < skipwins_n; k++) {
	if (ncdb) fprintf(stderr, "skipwins[%d] 0x%lx\n", k, skipwins[k]);
}

	X_LOCK;
	for (i=0; i < n; i++) {
		XEvent ev;
		int ns, skip = 0, type, idx = -1;
		int ik = Ev_order[i];

		if (Ev_done[ik]) continue;
		win = Ev_win[ik];

		ev = Ev[ik];
		type = ev.type;
		Ev_done[ik] = 1;

		win2 = win;
		if (win == rootwin) {
			if (type == CreateNotify) {
				win2 = ev.xcreatewindow.window;
			} else if (type == ReparentNotify) {
				win2 = ev.xreparent.window;
			}
		}
		for (ns = 0; ns < skipwins_n; ns++) {
			if (win2 == skipwins[ns]) {
				skip = 1;
				break;
			}
		}
		if (skip) {
if (ncdb) fprintf(stderr, "skip%02d: ** SpecialSkip   0x%lx/0x%lx type: %s\n", ik, win, win2, Etype(type));
			continue;
		}
		
		if (win == rootwin) {
			if (type == CreateNotify) {
				int x=0, y=0, w=0, h=0;
				valid = 0;
				win2 = ev.xcreatewindow.window;
				idx = lookup_win_index(win2);
				if (idx < 0) {
					continue;
				}
				if (cache_list[idx].valid) {
					valid = 1;
					x=cache_list[idx].x;
					y=cache_list[idx].y;
					w=cache_list[idx].width;
					h=cache_list[idx].height;
					if (w*h > 64 * 64 && ev_lookup(win2, EV_MAP)) {
						X_UNLOCK;
						valid = 1;
						su_save(idx, nbatch, &attr, 0, &valid, 1);
						STORE(idx, win2, attr);

						X_LOCK;

						if (! desktop_change) {
							SCHED(win2, 1) 
						}
						create_cnt++;
					}
				}
if (ncdb) fprintf(stderr, "root%02d: ** CreateNotify  0x%lx  %3d  -- %dx%d+%d+%d valid=%d\n", ik, win2, idx, w, h, x, y, valid);

			} else if (type == ReparentNotify) {
				if (ev.xreparent.parent != rootwin) {
					win2 = ev.xreparent.window;
					idx = lookup_win_index(win2);
if (ncdb) fprintf(stderr, "root%02d: ReparentNotifyRM 0x%lx  %3d\n", ik, win2, idx);
				}
			} else {
if (ncdb) fprintf(stderr, "root%02d: ** IgnoringRoot  0x%lx type: %s\n", ik, win, Etype(type));
			}
		} else {
			if (type == ConfigureNotify) {
				int x_new, y_new, w_new, h_new;
				int x_old, y_old, w_old, h_old;
				int stack_change, old_wm = 0;
				Window oabove = None;

				idx = lookup_win_index(win);

				if (idx >= 0) {
					oabove = cache_list[idx].above;
				}

if (ncdb) fprintf(stderr, "----%02d: ConfigureNotify  0x%lx  %3d  -- above: 0x%lx -> 0x%lx  %dx%d+%d+%d\n", ik, win, idx,
    oabove, ev.xconfigure.above, ev.xconfigure.width, ev.xconfigure.height, ev.xconfigure.x, ev.xconfigure.y);

				if (idx < 0) {
					continue;
				}

				x_new = ev.xconfigure.x; 
				y_new = ev.xconfigure.y; 
				w_new = ev.xconfigure.width; 
				h_new = ev.xconfigure.height; 

				x_old = cache_list[idx].x;
				y_old = cache_list[idx].y;
				w_old = cache_list[idx].width;
				h_old = cache_list[idx].height;

				if (desktop_change_old_wm) {
					if (ev_lookup(win, EV_OLD_WM_MAP)) {
						if (Ev_map[ik] == win) {
							old_wm = 1;
						} else {
							old_wm = 2;
						}
					} else if (ev_lookup(win, EV_OLD_WM_UNMAP)) {
						if (Ev_unmap[ik] == win) {
							old_wm = -1;
						} else {
							old_wm = 2;
						}
					} else if (ev_lookup(win, EV_OLD_WM_OFF)) {
						old_wm = 2;
					} else if (ev_lookup(win, EV_OLD_WM_NOTMAPPED)) {
						old_wm = 3;
					}
				}

				if (!old_wm)  {
					if (x_old != x_new || y_old != y_new) {
						/* invalidate su */
						cache_list[idx].su_time = 0.0;
if (ncdb) fprintf(stderr, "          INVALIDATE su: 0x%lx xy: +%d+%d  +%d+%d \n", win, x_old, y_old, x_new, y_new);
					}
					if (w_old != w_new || h_old != h_new) {
						/* invalidate bs */
						cache_list[idx].bs_time = 0.0;
if (ncdb) fprintf(stderr, "          INVALIDATE bs: 0x%lx wh:  %dx%d   %dx%d \n", win, w_old, h_old, w_new, h_new);
					}
				} else {
					int valid;
					X_UNLOCK;
					if (old_wm == 1) {
						/* XXX Y */
if (ncdb) fprintf(stderr, "          OLD_WM_MAP:    0x%lx wh:  %dx%d+%d+%d   %dx%d+%d+%d \n", win, w_old, h_old, x_old, y_old, w_new, h_new, x_new, y_new);
						valid = 0;
						bs_restore(idx, nbatch, NULL, &attr, 0, 0, &valid, 1);

					} else if (old_wm == -1) {
if (ncdb) fprintf(stderr, "          OLD_WM_UNMAP:  0x%lx wh:  %dx%d+%d+%d   %dx%d+%d+%d \n", win, w_old, h_old, x_old, y_old, w_new, h_new, x_new, y_new);
						valid = 1;
						su_restore(idx, nbatch, NULL, &attr, 1, 0, &valid, 1);
					} else {
if (ncdb) fprintf(stderr, "          OLD_WM_OFF::   0x%lx wh:  %dx%d+%d+%d   %dx%d+%d+%d  old_wm=%d\n", win, w_old, h_old, x_old, y_old, w_new, h_new, x_new, y_new, old_wm);
					}
					X_LOCK;
				}

				stack_change = 0;
				if (old_wm) {
					;
				} else if (cache_list[idx].above != ev.xconfigure.above) {
					stack_change = 1;
				} else if (x_new == x_old && y_new == y_old && w_new == w_old && h_new == h_old) {
					stack_change = 1;
				}
				if (stack_change) {
					int i2, ok = 1;
					for (i2=0; i2 < n; i2++)  {
						if (Ev_map[i2] == win) {
							ok = 0;
							break;
						}
					}
					if (ok) {
						if (n_MN == 0 && n_UN == 0) {
							if (su_fix_cnt > 0) {
								ok = 0;
if (ncdb) fprintf(stderr, "          CONF_IGNORE: Too many stacking changes: 0x%lx\n", win);
							}
						}
						
					}
					if (ok) {
						if (ev_lookup(ev.xconfigure.above, EV_UNMAP)) {
							if (ncdb) fprintf(stderr, "        skip try_to_fix_su for GNOME deiconify #1\n");
							if (dt_gnome) {
								gnome_animation = 1;
							}
							ok = 0;
						}
					}
					if (ok && dt_gnome) {
						if (valid_window(ev.xconfigure.above, &attr, 1)) {
							if (attr.map_state != IsViewable) {
								if (ncdb) fprintf(stderr, "        skip try_to_fix_su for GNOME deiconify #2\n");
								gnome_animation = 1;
								ok = 0;
							}
						}
					}
					if (ok) {
						int rc = try_to_fix_su(win, idx, ev.xconfigure.above, nbatch, NULL);	
						if (rc == 0 && su_fix_cnt == 0 && n_MN == 0 && n_UN == 0) {
							X_UNLOCK;
							try_to_synthesize_su(1, 1, nbatch);
							X_LOCK;
						}
						n_ST++;
						su_fix_cnt++;
					}
				}

				cache_list[idx].x = x_new;
				cache_list[idx].y = y_new;
				cache_list[idx].width = w_new;
				cache_list[idx].height = h_new;

				cache_list[idx].above = ev.xconfigure.above;
				cache_list[idx].time = dnow();

			} else if (type == VisibilityNotify) {
				int state = ev.xvisibility.state;
				idx = lookup_win_index(win);
if (ncdb) fprintf(stderr, "----%02d: VisibilityNotify 0x%lx  %3d  state: %s U/P %d/%d\n", ik, win, idx, VState(state), n_VN_u, n_VN_p);

				if (idx < 0) {
					continue;
				}
				if (desktop_change) {
					;
				} else if (macosx_console && n_VN_p == 0) {
					;	/* XXXX not working well yet with UnmapNotify ... */
				} else if (state == VisibilityUnobscured) {
					int ok = 1;
					if (ncache <= 2) {
						ok = 0;
					} else if (ev_lookup(win, EV_MAP)) {
						ok = 0;
					} else if (ev_lookup(win, EV_UNMAP)) {
						ok = 0;
					} else if (ev_lookup(win, EV_DESTROY)) {
						ok = 0;
					} else if (gnome_animation) {
						ok = 0;
					} else {
						/* this is for gnome iconify */
						int i2;
						for (i2=i+1; i2 < n; i2++) {
							int idx2, ik2 = Ev_order[i2];
							sraRegionPtr ro1, ro2;
							Window win2 = Ev_unmap[ik2];

							if (win2 == None) {
								continue;
							}
							idx2 = lookup_win_index(win2);
							if (idx2 < 0) {
								continue;
							}

							ro1 = idx_create_rgn(r0, idx);
							ro2 = idx_create_rgn(r0, idx2);

							if (sraRgnAnd(ro1, ro2)) {
								if (ncdb) fprintf(stderr, "        skip VisibilityUnobscured for GNOME iconify.\n");
								ok = 0;
							}
							sraRgnDestroy(ro1);
							sraRgnDestroy(ro2);
							if (! ok) {
								break;
							}
						}
					}
					if (ok) {
						int x2, y2, w2, h2;
						sraRegionPtr rmask = NULL;
						valid = 0;
						if (dnow() < cache_list[idx].vis_unobs_time + 3.00 && !sraRgnEmpty(unmapped_rgn)) {
							x2 = cache_list[idx].x;
							y2 = cache_list[idx].y;
							w2 = cache_list[idx].width;
							h2 = cache_list[idx].height;
							rmask = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
							sraRgnAnd(rmask, unmapped_rgn);
							if (sraRgnEmpty(rmask)) {
								sraRgnDestroy(rmask);
								rmask = NULL;
							}
						}
						if (ev_lookup(win, EV_CONFIGURE_SIZE)) {
							valid = valid_window(win, &attr, 1);
						} else {
							X_UNLOCK;
							bs_restore(idx, nbatch, rmask, &attr, 0, 1, &valid, 1);
							X_LOCK;
						}
						if (rmask != NULL) {
							sraRgnDestroy(rmask);
						}
						if (valid) {
							STORE(idx, win, attr);

							cache_list[idx].time = dnow();
							cache_list[idx].vis_cnt++;
							Ev_map[ik] = win;
							Ev_rects[nrects].x1 = cache_list[idx].x;
							Ev_rects[nrects].y1 = cache_list[idx].y;
							Ev_rects[nrects].x2 = cache_list[idx].width;
							Ev_rects[nrects].y2 = cache_list[idx].height;
							nrects++;
							SCHED(win, 1) 
						} else {
							DELETE(idx);
						}
					}
				}
				if (state == VisibilityUnobscured) {
					cache_list[idx].vis_unobs_time = last_vis_unobs_time = dnow();
				} else if (cache_list[idx].vis_state == VisibilityUnobscured) {
					cache_list[idx].vis_obs_time = last_vis_obs_time = dnow();
				}
				cache_list[idx].vis_state = state;

			} else if (type == MapNotify) {
				idx = lookup_win_index(win);
if (ncdb) fprintf(stderr, "----%02d: MapNotify        0x%lx  %3d\n", ik, win, idx);

				if (idx < 0) {
					continue;
				}

#if 0
/*
				if (cache_list[idx].map_state == IsUnmapped || desktop_change || macosx_console)
 */
#endif
				if (1) {
					X_UNLOCK;
					if (desktop_change) {
						/* XXX Y */
						int save = 1;
						sraRegionPtr r;
						if (cache_list[idx].su_time != 0.0) {
							save = 0;
						} else if (missed_su_restore) {
							r = idx_create_rgn(r0, idx);
							if (sraRgnAnd(r, missed_su_restore_rgn)) {
								save = 0;
							}
							sraRgnDestroy(r);
						}
						if (missed_bs_restore) {
							r = idx_create_rgn(r0, idx);
							if (sraRgnAnd(r, missed_bs_restore_rgn)) {
								save = 0;
							}
							sraRgnDestroy(r);
						}
						if (save) {
							valid = 0;
							su_save(idx, nbatch, &attr, 1, &valid, 1);
							if (valid) {
								STORE(idx, win, attr);
							}
						}
					} else {
						valid = 0;
						su_save(idx, nbatch, &attr, 0, &valid, 1);
						if (valid) {
							STORE(idx, win, attr);
						}
					}
					valid = 0;
					if (ev_lookup(win, EV_CONFIGURE_SIZE)) {
						X_LOCK;
						valid = valid_window(win, &attr, 1);
						X_UNLOCK;
						idx_add_rgn(missed_bs_restore_rgn, r0, idx);
						missed_bs_restore++;
					} else if (bs_restore(idx, nbatch, NULL, &attr, 0, 0, &valid, 1)) { /* XXX clip? */
						;
					} else {
						idx_add_rgn(missed_bs_restore_rgn, r0, idx);
						missed_bs_restore++;
					}
					if (valid) {
						STORE(idx, win, attr);
					}

					if (macosx_console) {
#ifdef MACOSX
						macosxCGS_follow_animation_win(win, -1, 1);
						if (valid_window(win, &attr, 1)) {
							STORE(idx, win, attr);
							SCHED(win, 1);
						}
						/* XXX Y */
						if (cache_list[idx].vis_state == -1)  {
							cache_list[idx].vis_state = VisibilityUnobscured;
						}
#endif
					}
					X_LOCK;
					pixels += cache_list[idx].width * cache_list[idx].height;
					cache_list[idx].time = dnow();
					cache_list[idx].map_cnt++;
					Ev_map[ik] = win;
					Ev_rects[nrects].x1 = cache_list[idx].x;
					Ev_rects[nrects].y1 = cache_list[idx].y;
					Ev_rects[nrects].x2 = cache_list[idx].width;
					Ev_rects[nrects].y2 = cache_list[idx].height;
					nrects++;

					if (! valid) {
						DELETE(idx);
					}
				}
				cache_list[idx].map_state = IsViewable;

			} else if (type == UnmapNotify) {
				int x2, y2, w2, h2;
				idx = lookup_win_index(win);
if (ncdb) fprintf(stderr, "----%02d: UnmapNotify      0x%lx  %3d\n", ik, win, idx);

				if (idx < 0) {
					continue;
				}
				if (macosx_console) {
					if (mode == 2) {
						cache_list[idx].map_state = IsViewable;
					}
				}

#if 0
/*
				if (cache_list[idx].map_state == IsViewable || desktop_change || macosx_console)
 */
#endif
				if (1) {
					X_UNLOCK;
					if (desktop_change) {
						int save = 1;
						sraRegionPtr r;
						if (cache_list[idx].bs_time > 0.0) {
							save = 0;
						} else if (missed_su_restore) {
							r = idx_create_rgn(r0, idx);
							if (sraRgnAnd(r, missed_su_restore_rgn)) {
								save = 0;
							}
							sraRgnDestroy(r);
						}
						if (missed_bs_restore) {
							r = idx_create_rgn(r0, idx);
							if (sraRgnAnd(r, missed_bs_restore_rgn)) {
								save = 0;
							}
							sraRgnDestroy(r);
						}
						if (save) {
							valid = 0;
							bs_save(idx, nbatch, &attr, 1, 0, &valid, 1);
						}
					} else {
						valid = 0;
						bs_save(idx, nbatch, &attr, 1, 0, &valid, 1);
					}
					valid = 0;
					if (su_restore(idx, nbatch, NULL, &attr, 1, 0, &valid, 1)) {
						try_to_fix_su(win, idx, None, nbatch, "unmapped");	
						if (valid) {
							STORE(idx, win, attr);
						} else {
							DELETE(idx);
						}
					} else {
						idx_add_rgn(missed_su_restore_rgn, r0, idx);
						missed_su_restore++;
					}
					X_LOCK;

					pixels += cache_list[idx].width * cache_list[idx].height;
					cache_list[idx].time = dnow();
					cache_list[idx].unmap_cnt++;
					Ev_unmap[ik] = win;
					Ev_rects[nrects].x1 = cache_list[idx].x;
					Ev_rects[nrects].y1 = cache_list[idx].y;
					Ev_rects[nrects].x2 = cache_list[idx].width;
					Ev_rects[nrects].y2 = cache_list[idx].height;
					nrects++;
				}

				x2 = cache_list[idx].x;
				y2 = cache_list[idx].y;
				w2 = cache_list[idx].width;
				h2 = cache_list[idx].height;
				r = sraRgnCreateRect(x2, y2, x2+w2, y2+h2);
				sraRgnAnd(r, r0); 
				sraRgnOr(unmapped_rgn, r); 
				sraRgnDestroy(r);

				cache_list[idx].map_state = IsUnmapped;

			} else if (type == ReparentNotify) {
				if (ev.xreparent.parent != rootwin) {
					win2 = ev.xreparent.window;
					if (win2 != rootwin) {
						idx = lookup_win_index(win2);
if (ncdb) fprintf(stderr, "----%02d: ReparentNotifyRM 0x%lx  %3d\n", ik, win2, idx);
					}
				}

			} else if (type == DestroyNotify) {
				win2 = ev.xdestroywindow.window;
				idx = lookup_win_index(win2);
if (ncdb) fprintf(stderr, "----%02d: DestroyNotify    0x%lx  %3d\n", ik, win2, idx);

				if (idx >= 0) {
					DELETE(idx);
				}
			} else {
if (ncdb) fprintf(stderr, "igno%02d: ** Ignoring      0x%lx type: %s\n", ik, win, Etype(type));
			}

		}
	}
	X_UNLOCK;

	if (use_batch && nreg) {
		batch_push(nreg, -1.0);
	}
	if (nrects) {
		if (scaling) {
			push_borders(Ev_rects, nrects);
		}
	}

	check_sched(try_batch, &did_sched);

	if (n_CN || n_RN || n_DN || n_MN || n_UN || n_ST || n_DC || did_sched) {
		snap_old();
	}

	sraRgnDestroy(r0);
	sraRgnDestroy(missed_su_restore_rgn);
	sraRgnDestroy(missed_bs_restore_rgn);

if (ncdb) rfbLog("OUT check_ncache(): %.4f %.6f events: %d  pixels: %d\n", dnowx(), dnow() - now, n, pixels);
if (ncdb) fprintf(stderr, "\n");
	return pixels;
}
#endif

