/* -- userinput.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xdamage.h"
#include "xrecord.h"
#include "xinerama.h"
#include "win_utils.h"
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

/*
 * user input handling heuristics
 */
int defer_update_nofb = 6;	/* defer a shorter time under -nofb */
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
void fb_push_wait(double max_wait, int flags);
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


static void get_client_regions(int *req, int *mod, int *cpy, int *num) ;
static void do_copyregion(sraRegionPtr region, int dx, int dy) ;
static void parse_scroll_copyrect_str(char *scr);
static void parse_wireframe_str(char *wf);
static void destroy_str_list(char **list);
static void draw_box(int x, int y, int w, int h, int restore);
static int do_bdpush(Window wm_win, int x0, int y0, int w0, int h0, int bdx,
    int bdy, int bdskinny);
static int set_ypad(void);
static void scale_mark(int x1, int y1, int x2, int y2);
static int push_scr_ev(double *age, int type, int bdpush, int bdx, int bdy,
    int bdskinny);
static int crfix(int x, int dx, int Lx);
static int scrollability(Window win, int set);
static int eat_pointer(int max_ptr_eat, int keep);
static void set_bdpush(int type, double *last_bdpush, int *pushit);
static int repeat_check(double last_key_scroll);
static int check_xrecord_keys(void);
static int check_xrecord_mouse(void);
static int try_copyrect(Window frame, int x, int y, int w, int h, int dx, int dy,
    int *obscured, sraRegionPtr extra_clip, double max_wait);
static int wireframe_mod_state();
static void check_user_input2(double dt);
static void check_user_input3(double dt, double dtr, int tile_diffs);
static void check_user_input4(double dt, double dtr, int tile_diffs);


/*
 * For -wireframe: find the direct child of rootwin that has the
 * pointer, assume that is the WM frame that contains the application
 * (i.e. wm reparents the app toplevel) return frame position and size
 * if successful.
 */
int get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win) {
	Window r, c;
	XWindowAttributes attr;
	Bool ret;
	int rootx, rooty, wx, wy;
	unsigned int mask;

#ifdef MACOSX
	if (macosx_console) {
		return macosx_get_wm_frame_pos(px, py, x, y, w, h, frame, win);
	}
#endif

	RAWFB_RET(0)

#if NO_X11
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
	char *part[10];

	for (i=0; i<10; i++) {
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
	char *part[10];

	for (i=0; i<10; i++) {
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
		if (depth == 8) {
			use_Bpl *= 4;
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
			mark_rect_as_modified(x0, y_min, x1, y_max+1, 0);
		}
		save[i]->saved = 0;
	}

	if (restore) {
		return;
	}

if (0) fprintf(stderr, "  DrawBox: %dx%d+%d+%d\n", w, h, x, y);

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

static void scale_mark(int x1, int y1, int x2, int y2) {
	int s = 2;
	x1 = nfix(x1 - s, dpy_x);
	y1 = nfix(y1 - s, dpy_y);
	x2 = nfix(x2 + s, dpy_x+1);
	y2 = nfix(y2 + s, dpy_y+1);
	scale_and_mark_rect(x1, y1, x2, y2);
}

#define PUSH_TEST(n)  \
if (n) { \
	double dt = 0.0, tm; dtime0(&tm); \
	fprintf(stderr, "PUSH---\n"); \
	while (dt < 2.0) { rfbPE(50000); dt += dtime(&tm); } \
	fprintf(stderr, "---PUSH\n"); \
}

static int push_scr_ev(double *age, int type, int bdpush, int bdx, int bdy,
    int bdskinny) {
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

if (db) fprintf(stderr, "ypad: %d  dy[0]: %d\n", ypad, scr_ev[0].dy);

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
		
		if (try_copyrect(frame, x, y, w, h, dx, dy, &obscured,
		    tmpregion, waittime)) {
			last_scroll_type = type;
			dtime0(&last_scroll_event);

			do_fb_push++;
			urgent_update = 1;
			sraRgnDestroy(tmpregion);
			
PUSH_TEST(0);

		} else {
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

				scale_mark(x0, y0, x0 + w0, y0 + h0);
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
		*req += sraRgnCountRects(cl->requestedRegion);
		*mod += sraRgnCountRects(cl->modifiedRegion);
		*cpy += sraRgnCountRects(cl->copyRegion);
		*num += 1;
	}
	rfbReleaseClientIterator(i);
}

/*
 * Wrapper to apply the rfbDoCopyRegion taking into account if scaling
 * is being done.  Note that copyrect under the scaling case is often
 * only approximate.
 */
static void do_copyregion(sraRegionPtr region, int dx, int dy)  {
	sraRectangleIterator *iter;
	sraRect rect;
	int Bpp0 = bpp/8, Bpp;
	int x1, y1, x2, y2, w, stride, stride0;
	int sx1, sy1, sx2, sy2, sdx, sdy;
	int req, mod, cpy, ncli;
	char *dst = NULL, *src = NULL;

	last_copyrect = dnow();

	if (rfb_fb == main_fb && ! rotating) {
		/* normal case, no -scale or -8to24 */
		get_client_regions(&req, &mod, &cpy, &ncli);
if (debug_scroll > 1) fprintf(stderr, "<<<-rfbDoCopyRect req: %d mod: %d cpy: %d\n", req, mod, cpy); 
		rfbDoCopyRegion(screen, region, dx, dy);

		get_client_regions(&req, &mod, &cpy, &ncli);
if (debug_scroll > 1) fprintf(stderr, ">>>-rfbDoCopyRect req: %d mod: %d cpy: %d\n", req, mod, cpy); 

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
				if (! cmap8to24_fb || cmap8to24_fb == rfb_fb) {
					continue;
				}
				if (cmap8to24 && depth == 8) {
					Bpp = 4 * Bpp0;
					stride = 4 * stride0;
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
if (0) fprintf(stderr, "s... %d %d %d %d %d %d\n", sx1, sy1, sx2, sy2, sdx, sdy);
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
if (0) fprintf(stderr, "s... %d %d %d %d %d %d\n", sx1, sy1, sx2, sy2, sdx, sdy);

		rfbDoCopyRect(screen, sx1, sy1, sx2, sy2, sdx, sdy);
	}
	sraRgnReleaseIterator(iter);
}

void fb_push(void) {
	char *httpdir = screen->httpDir;
	int defer = screen->deferUpdateTime;
	int req0, mod0, cpy0, req1, mod1, cpy1, ncli;
	int db = (debug_scroll || debug_wireframe);

	screen->httpDir = NULL;
	screen->deferUpdateTime = 0;

if (db)	get_client_regions(&req0, &mod0, &cpy0, &ncli);

	rfbPE(0);

	screen->httpDir = httpdir;
	screen->deferUpdateTime = defer;

 if (db) {
	get_client_regions(&req1, &mod1, &cpy1, &ncli);
	fprintf(stderr, "\nFB_push: req: %d/%d  mod: %d/%d  cpy: %d/%d  %.4f\n",
	req0, req1, mod0, mod1, cpy0, cpy1, dnow() - x11vnc_start);
 }

}

void fb_push_wait(double max_wait, int flags) {
	double tm, dt = 0.0;
	int req, mod, cpy, ncli;

	dtime0(&tm);	
	while (dt < max_wait) {
		int done = 1;
		rfbCFD(0);
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
			break;
		}

		usleep(1000);
		fb_push();
		dt += dtime(&tm);
	}
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
		dret = push_scr_ev(&age, SCR_KEY, bdpush, bdx, bdy, bdskinny);
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
			    bdy, bdskinny);
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
	pointer(-1, 0, 0, NULL);
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

static int try_copyrect(Window frame, int x, int y, int w, int h, int dx, int dy,
    int *obscured, sraRegionPtr extra_clip, double max_wait) {

	static int dt_bad = 0;
	static time_t dt_bad_check = 0;
	int x1, y1, x2, y2, sent_copyrect = 0;
	int req, mod, cpy, ncli;
	double tm, dt;
	DB_SET

	get_client_regions(&req, &mod, &cpy, &ncli);
	if (cpy) {
		/* one is still pending... try to force it out: */
		fb_push_wait(max_wait, FB_COPY);

		get_client_regions(&req, &mod, &cpy, &ncli);
	}
	if (cpy) {
		return 0;
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
		if (!strcmp(dt, "kde")) {
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
if (db2) fprintf(stderr, "try_copyrect: 0x%lx  bad: %d stack_list_num: %d\n", frame, dt_bad, stack_list_num);

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
		do_copyregion(rect, dx, dy);
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
			if (swin == frame) {
 if (db2) {
 saw_me = 1; fprintf(stderr, "  ----------\n");
 } else {
				break;	
 }
			}
#if 0
fprintf(stderr, "bo: %d/%lx\n", k, swin);
#endif

			/* skip some unwanted cases: */
#ifndef MACOSX
			if (swin == None) {
				continue;
			}
#endif
			if (boff <= swin && swin < boff + bwin) {
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
			attr.map_state = stack_list[k].map_state;

			if (attr.map_state != IsViewable) {
				continue;
			}

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
				do_copyregion(shifted_region, dx, dy);
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
				scale_and_mark_rect(0, 0, dpy_x, dpy_y);
				if (db) rfbLog("doing scale screen_fixup\n");
			}
			last_copyrect_fix = now;
			last = now;
			didfull = 1;
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
	int special_t1 = 0, break_reason = 0;
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
		return 0;
	}
	X_UNLOCK;
if (db) fprintf(stderr, "a: %d  wf: %.3f  A: %d  frm: 0x%lx\n", w*h, wireframe_frac, (dpy_x*dpy_y), frame);

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
	if (! try_it) {
if (db) fprintf(stderr, "INTERIOR\n");
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
		pointer(-1, 0, 0, NULL);
	}

	g = got_pointer_input;
	gd = got_local_pointer_input;

	while (1) {

		X_LOCK;
		XFlush_wr(dpy);
		X_UNLOCK;

		/* try do induce/waitfor some more user input */
		if (use_threads) {
			usleep(1000);
		} else if (drew_box) {
			rfbPE(1000);
		} else {
			rfbCFD(1000);
		}
		if (bdown0 == 2) {
			int freq = 1;
			/*
			 * This is to just update display_button_mask
			 * which will also update got_local_pointer_input.
			 */
			int px, py, x, y, w, h;
			Window frame;
			check_x11_pointer();
#if 0
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
		/* see if some pointer input occurred: */
		if (got_pointer_input > g ||
		    (wireframe_local && (got_local_pointer_input > gd))) {

if (db) fprintf(stderr, "  ++pointer event!! [%02d]  dt: %.3f  x: %d  y: %d  mask: %d\n",
    got_2nd_pointer+1, spin, cursor_x, cursor_y, button_mask);	

			g = got_pointer_input;
			gd = got_local_pointer_input;

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

			/*
			 * see if it is time to draw any or a new wireframe box
			 */
			if (frame_changed) {
				int drawit = 0;
				if (x != box_x || y != box_y) {
					/* moved since last */
					drawit = 1;
				} else if (w != box_w || h != box_h) {
					/* resize since last */
					drawit = 1;
				}
				if (drawit) {
					/*
					 * check time (to avoid too much
					 * animations on slow machines
					 * or links).
					 */
					if (spin > last_draw + min_draw ||
					    ! drew_box) {
						draw_box(x, y, w, h, 0);
						drew_box = 1;
						rfbPE(1000);
						last_draw = spin;
					}
				}
				box_x = x;
				box_y = y;
				box_w = w;
				box_h = h;
			}
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
		return 0;
	}

	/* remove the wireframe */
	draw_box(0, 0, 0, 0, 1);

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
		;
	} else if (dx == 0 && dy == 0) {
		;
	} else {
		int spin_ms = (int) (spin * 1000 * 1000);
		int obscured, sent_copyrect = 0;

		/*
		 * set a timescale comparable to the spin time,
		 * but not too short or too long.
		 */
		if (spin_ms < 30) {
			spin_ms = 30;
		} else if (spin_ms > 400) {
			spin_ms = 400;
		}

		/* try to flush the wireframe removal: */
		fb_push_wait(0.1, FB_COPY|FB_MOD);

		/* try to send a clipped copyrect of translation: */
		sent_copyrect = try_copyrect(frame, x, y, w, h, dx, dy,
		    &obscured, NULL, 0.15);

if (db) fprintf(stderr, "sent_copyrect: %d - obs: %d  frame: 0x%lx\n", sent_copyrect, obscured, frame);
		if (sent_copyrect) {
			/* try to push the changes to viewers: */
			if (! obscured) {
				fb_push_wait(0.1, FB_COPY);
			} else {
				/* no diff for now... */
				fb_push_wait(0.1, FB_COPY);
			}
			if (scaling) {
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

					scale_mark(xt, yt, xt+w, yt+h);
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

	if (0) {
	/* No longer needed.  see draw_box() */
	    if (frame_changed && cmap8to24 && multivis_count) {
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
		check_for_multivis();
		mark_rect_as_modified(x1, y1, x2, y2, 0);
	    }
	}

	urgent_update = 1;
	if (use_xdamage) {
		/* DAMAGE can queue ~1000 rectangles for a move */
		clear_xdamage_mark_region(NULL, 1);
		xdamage_scheduled_mark = dnow() + 2.0;
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
		sent += rfbStatGetMessageCountSent(cl, rfbFramebufferUpdate);
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
			*cnt++;
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
				*cnt++;
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


