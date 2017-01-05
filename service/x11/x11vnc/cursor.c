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

/* -- cursor.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "cleanup.h"
#include "screen.h"
#include "scan.h"
#include "unixpw.h"
#include "macosx.h"

int xfixes_present = 0;
int xfixes_first_initialized = 0;
int use_xfixes = 1;
int got_xfixes_cursor_notify = 0;
int cursor_changes = 0;
int alpha_threshold = 240;
double alpha_frac = 0.33;
int alpha_remove = 0;
int alpha_blend = 1;
int alt_arrow = 1;


void first_cursor(void);
void setup_cursors_and_push(void);
void initialize_xfixes(void);
int known_cursors_mode(char *s);
void initialize_cursors_mode(void);
int get_which_cursor(void);
void restore_cursor_shape_updates(rfbScreenInfoPtr s);
void disable_cursor_shape_updates(rfbScreenInfoPtr s);
int cursor_shape_updates_clients(rfbScreenInfoPtr s);
int cursor_pos_updates_clients(rfbScreenInfoPtr s);
void cursor_position(int x, int y);
void set_no_cursor(void);
void set_warrow_cursor(void);
int set_cursor(int x, int y, int which);
int check_x11_pointer(void);
int store_cursor(int serial, unsigned long *data, int w, int h, int cbpp, int xhot, int yhot);
unsigned long get_cursor_serial(int mode);


typedef struct win_str_info {
	char *wm_name;
	char *res_name;
	char *res_class;
} win_str_info_t;

typedef struct cursor_info {
	char *data;	/* data and mask pointers */
	char *mask;
	int wx, wy;	/* size of cursor */
	int sx, sy;	/* shift to its centering point */
	int reverse;	/* swap black and white */
	rfbCursorPtr rfb;
} cursor_info_t;


static void curs_copy(cursor_info_t *dest, cursor_info_t *src);
static void setup_cursors(void);
static void set_rfb_cursor(int which);
static void tree_descend_cursor(int *depth, Window *w, win_str_info_t *winfo);
static rfbCursorPtr pixels2curs(unsigned long *pixels, int w, int h,
    int xhot, int yhot, int Bpp);
static int get_exact_cursor(int init);
static void set_cursor_was_changed(rfbScreenInfoPtr s);


/*
 * Here begins a bit of a mess to experiment with multiple cursors 
 * drawn on the remote background ...
 */
static void curs_copy(cursor_info_t *dest, cursor_info_t *src) {
	if (src->data != NULL) {
		dest->data = strdup(src->data);
	} else {
		dest->data = NULL;
	}
	if (src->mask != NULL) {
		dest->mask = strdup(src->mask);
	} else {
		dest->mask = NULL;
	}
	dest->wx = src->wx;
	dest->wy = src->wy;
	dest->sx = src->sx;
	dest->sy = src->sy;
	dest->reverse = src->reverse;
	dest->rfb = src->rfb;

	if (rotating && rotating_cursors && dest->data != NULL) {
		int tx, ty;
		rotate_curs(dest->data, src->data, src->wx, src->wy, 1);
		rotate_curs(dest->mask, src->mask, src->wx, src->wy, 1);
		rotate_coords(dest->sx, dest->sy, &tx, &ty, src->wx, src->wy);
		dest->sx = tx;
		dest->sy = ty;
		if (! rotating_same) {
			dest->wx = src->wy;
			dest->wy = src->wx;
		}
	}
}

/* empty cursor */
static char* curs_empty_data =
"  "
"  ";

static char* curs_empty_mask =
"  "
"  ";
static cursor_info_t cur_empty = {NULL, NULL, 2, 2, 0, 0, 0, NULL};

/* dot cursor */
static char* curs_dot_data =
"  "
" x";

static char* curs_dot_mask =
"  "
" x";
static cursor_info_t cur_dot = {NULL, NULL, 2, 2, 0, 0, 0, NULL};


/* main cursor */
static char* curs_arrow_data =
"                  "
" x                "
" xx               "
" xxx              "
" xxxx             "
" xxxxx            "
" xxxxxx           "
" xxxxxxx          "
" xxxxxxxx         "
" xxxxx            "
" xx xx            "
" x   xx           "
"     xx           "
"      xx          "
"      xx          "
"                  "
"                  "
"                  ";

static char* curs_arrow_mask =
"xx                "
"xxx               "
"xxxx              "
"xxxxx             "
"xxxxxx            "
"xxxxxxx           "
"xxxxxxxx          "
"xxxxxxxxx         "
"xxxxxxxxxx        "
"xxxxxxxxxx        "
"xxxxxxx           "
"xxx xxxx          "
"xx  xxxx          "
"     xxxx         "
"     xxxx         "
"      xx          "
"                  "
"                  ";
static cursor_info_t cur_arrow = {NULL, NULL, 18, 18, 0, 0, 1, NULL};

static char* curs_arrow2_data =
"                  "
" x                "
" xx               "
" xxx              "
" xxxx             "
" xxxxx            "
" xxxxxx           "
" xxxxxxx          "
" xxxxxxxx         "
" xxxxx            "
" xx xx            "
" x   xx           "
"     xx           "
"      xx          "
"      xx          "
"                  "
"                  "
"                  ";

static char* curs_arrow2_mask =
"xx                "
"xxx               "
"xxxx              "
"xxxxx             "
"xxxxxx            "
"xxxxxxx           "
"xxxxxxxx          "
"xxxxxxxxx         "
"xxxxxxxxxx        "
"xxxxxxxxxx        "
"xxxxxxx           "
"xxx xxxx          "
"xx  xxxx          "
"     xxxx         "
"     xxxx         "
"      xx          "
"                  "
"                  ";
static cursor_info_t cur_arrow2 = {NULL, NULL, 18, 18, 0, 0, 0, NULL};

static char* curs_arrow3_data = 
"                "
" xx             "
" xxxx           "
"  xxxxx         "
"  xxxxxxx       "
"   xxxxxxxx     "
"   xxxxxxxxxx   "
"    xxxxx       "
"    xxxxx       "
"     xx  x      "
"     xx   x     "
"      x    x    "
"      x     x   "
"             x  "
"              x "
"                ";

static char* curs_arrow3_mask = 
"xxx             "
"xxxxx           "
"xxxxxxx         "
" xxxxxxxx       "
" xxxxxxxxxx     "
"  xxxxxxxxxxxx  "
"  xxxxxxxxxxxx  "
"   xxxxxxxxxxx  "
"   xxxxxxx      "
"    xxxxxxx     "
"    xxxx xxx    "
"     xxx  xxx   "
"     xxx   xxx  "
"     xxx    xxx "
"             xxx"
"              xx";

static cursor_info_t cur_arrow3 = {NULL, NULL, 16, 16, 0, 0, 1, NULL};

static char* curs_arrow4_data = 
"                "
" xx             "
" xxxx           "
"  xxxxx         "
"  xxxxxxx       "
"   xxxxxxxx     "
"   xxxxxxxxxx   "
"    xxxxx       "
"    xxxxx       "
"     xx  x      "
"     xx   x     "
"      x    x    "
"      x     x   "
"             x  "
"              x "
"                ";

static char* curs_arrow4_mask = 
"xxx             "
"xxxxx           "
"xxxxxxx         "
" xxxxxxxx       "
" xxxxxxxxxx     "
"  xxxxxxxxxxxx  "
"  xxxxxxxxxxxx  "
"   xxxxxxxxxxx  "
"   xxxxxxx      "
"    xxxxxxx     "
"    xxxx xxx    "
"     xxx  xxx   "
"     xxx   xxx  "
"     xxx    xxx "
"             xxx"
"              xx";

static cursor_info_t cur_arrow4 = {NULL, NULL, 16, 16, 0, 0, 0, NULL};

static char* curs_arrow5_data = 
"x              "
" xx            "
" xxxx          "
"  xxxxx        "
"  xxxxxxx      "
"   xxx         "
"   xx x        "
"    x  x       "
"    x   x      "
"         x     "
"          x    "
"           x   "
"            x  "
"             x "
"              x";

static char* curs_arrow5_mask = 
"xx             "
"xxxx           "
" xxxxx         "
" xxxxxxx       "
"  xxxxxxxx     "
"  xxxxxxxx     "
"   xxxxx       "
"   xxxxxx      "
"    xx xxx     "
"     x  xxx    "
"         xxx   "
"          xxx  "
"           xxx "
"            xxx"
"             xx";

static cursor_info_t cur_arrow5 = {NULL, NULL, 15, 15, 0, 0, 1, NULL};

static char* curs_arrow6_data = 
"x              "
" xx            "
" xxxx          "
"  xxxxx        "
"  xxxxxxx      "
"   xxx         "
"   xx x        "
"    x  x       "
"    x   x      "
"         x     "
"          x    "
"           x   "
"            x  "
"             x "
"              x";

static char* curs_arrow6_mask = 
"xx             "
"xxxx           "
" xxxxx         "
" xxxxxxx       "
"  xxxxxxxx     "
"  xxxxxxxx     "
"   xxxxx       "
"   xxxxxx      "
"    xx xxx     "
"     x  xxx    "
"         xxx   "
"          xxx  "
"           xxx "
"            xxx"
"             xx";

static cursor_info_t cur_arrow6 = {NULL, NULL, 15, 15, 0, 0, 0, NULL};

int alt_arrow_max = 6;
/*
 * It turns out we can at least detect mouse is on the root window so 
 * show it (under -cursor X) with this familiar cursor... 
 */
static char* curs_root_data =
"                  "
"                  "
"  xxx        xxx  "
"  xxxx      xxxx  "
"  xxxxx    xxxxx  "
"   xxxxx  xxxxx   "
"    xxxxxxxxxx    "
"     xxxxxxxx     "
"      xxxxxx      "
"      xxxxxx      "
"     xxxxxxxx     "
"    xxxxxxxxxx    "
"   xxxxx  xxxxx   "
"  xxxxx    xxxxx  "
"  xxxx      xxxx  "
"  xxx        xxx  "
"                  "
"                  ";

static char* curs_root_mask =
"                  "
" xxxx        xxxx "
" xxxxx      xxxxx "
" xxxxxx    xxxxxx "
" xxxxxxx  xxxxxxx "
"  xxxxxxxxxxxxxx  "
"   xxxxxxxxxxxx   "
"    xxxxxxxxxx    "
"     xxxxxxxx     "
"     xxxxxxxx     "
"    xxxxxxxxxx    "
"   xxxxxxxxxxxx   "
"  xxxxxxxxxxxxxx  "
" xxxxxxx  xxxxxxx "
" xxxxxx    xxxxxx "
" xxxxx      xxxxx "
" xxxx        xxxx "
"                  ";
static cursor_info_t cur_root = {NULL, NULL, 18, 18, 8, 8, 1, NULL};

static char* curs_fleur_data = 
"                "
"       xx       "
"      xxxx      "
"     xxxxxx     "
"       xx       "
"   x   xx   x   "
"  xx   xx   xx  "
" xxxxxxxxxxxxxx "
" xxxxxxxxxxxxxx "
"  xx   xx   xx  "
"   x   xx   x   "
"       xx       "
"     xxxxxx     "
"      xxxx      "
"       xx       "
"                ";

static char* curs_fleur_mask = 
"      xxxx      "
"      xxxxx     "
"     xxxxxx     "
"    xxxxxxxx    "
"   x xxxxxx x   "
"  xxx xxxx xxx  "
"xxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxx"
"  xxx xxxx xxx  "
"   x xxxxxx x   "
"    xxxxxxxx    "
"     xxxxxx     "
"      xxxx      "
"      xxxx      ";

static cursor_info_t cur_fleur = {NULL, NULL, 16, 16, 8, 8, 1, NULL};

static char* curs_plus_data = 
"            "
"     xx     "
"     xx     "
"     xx     "
"     xx     "
" xxxxxxxxxx "
" xxxxxxxxxx "
"     xx     "
"     xx     "
"     xx     "
"     xx     "
"            ";

static char* curs_plus_mask = 
"    xxxx    "
"    xxxx    "
"    xxxx    "
"    xxxx    "
"xxxxxxxxxxxx"
"xxxxxxxxxxxx"
"xxxxxxxxxxxx"
"xxxxxxxxxxxx"
"    xxxx    "
"    xxxx    "
"    xxxx    "
"    xxxx    ";
static cursor_info_t cur_plus = {NULL, NULL, 12, 12, 5, 6, 1, NULL};

static char* curs_xterm_data = 
"                "
"     xxx xxx    "
"       xxx      "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"        x       "
"       xxx      "
"     xxx xxx    "
"                ";

static char* curs_xterm_mask = 
"    xxxx xxxx   "
"    xxxxxxxxx   "
"    xxxxxxxxx   "
"      xxxxx     "
"       xxx      "
"       xxx      "
"       xxx      "
"       xxx      "
"       xxx      "
"       xxx      "
"       xxx      "
"       xxx      "
"      xxxxx     "
"    xxxxxxxxx   "
"    xxxxxxxxx   "
"    xxxx xxxx   ";
static cursor_info_t cur_xterm = {NULL, NULL, 16, 16, 8, 8, 1, NULL};

enum cursor_names {
	CURS_EMPTY = 0,
	CURS_DOT,

	CURS_ARROW,
	CURS_WARROW,
	CURS_ROOT,
	CURS_WM,
	CURS_TERM,
	CURS_PLUS,

	CURS_DYN1,
	CURS_DYN2,
	CURS_DYN3,
	CURS_DYN4,
	CURS_DYN5,
	CURS_DYN6,
	CURS_DYN7,
	CURS_DYN8,
	CURS_DYN9,
	CURS_DYN10,
	CURS_DYN11,
	CURS_DYN12,
	CURS_DYN13,
	CURS_DYN14,
	CURS_DYN15,
	CURS_DYN16
};

#define CURS_DYN_MIN CURS_DYN1
#define CURS_DYN_MAX CURS_DYN16
#define CURS_DYN_NUM (CURS_DYN_MAX - CURS_DYN_MIN + 1)

#define CURS_MAX 32
static cursor_info_t *cursors[CURS_MAX];

void first_cursor(void) {
	if (! screen) {
		return;
	}
	if (! show_cursor) {
		LOCK(screen->cursorMutex);
		screen->cursor = NULL;
		UNLOCK(screen->cursorMutex);
	} else {
		got_xfixes_cursor_notify++;
		set_rfb_cursor(get_which_cursor());
		set_cursor_was_changed(screen);
	}
}

static void setup_cursors(void) {
	rfbCursorPtr rfb_curs;
	char *scale = NULL;
	int i, j, n = 0;
	int w_in = 0, h_in = 0;
	static int first = 1;

	if (verbose || use_threads) {
		rfbLog("setting up %d cursors...\n", CURS_MAX);
	}

	if (first) {
		for (i=0; i<CURS_MAX; i++) {
			cursors[i] = NULL;
		}
	}
	first = 0;

	if (screen) {
		LOCK(screen->cursorMutex);
		screen->cursor = NULL;
	}

	for (i=0; i<CURS_MAX; i++) {
		cursor_info_t *ci;
		if (cursors[i]) {
			/* clear out any existing ones: */
			ci = cursors[i];
			if (ci->rfb) {
				/* this is the rfbCursor part: */
				if (ci->rfb->richSource) {
					free(ci->rfb->richSource);
					ci->rfb->richSource = NULL;
				}
				if (ci->rfb->source) {
					free(ci->rfb->source);
					ci->rfb->source = NULL;
				}
				if (ci->rfb->mask) {
					free(ci->rfb->mask);
					ci->rfb->mask = NULL;
				}
				free(ci->rfb);
				ci->rfb = NULL;
			}
			if (ci->data) {
				free(ci->data);
				ci->data = NULL;
			}
			if (ci->mask) {
				free(ci->mask);
				ci->mask = NULL;
			}
			free(ci);
			ci = NULL;
		}

		/* create new struct: */
		ci = (cursor_info_t *) malloc(sizeof(cursor_info_t));
		ci->data = NULL; 
		ci->mask = NULL; 
		ci->wx = 0;
		ci->wy = 0;
		ci->sx = 0;
		ci->sy = 0;
		ci->reverse = 0;
		ci->rfb = NULL;
		cursors[i] = ci;
	}

	/* clear any xfixes cursor cache (no freeing is done) */
	get_exact_cursor(1);

	/* manually fill in the data+masks: */
	cur_empty.data	= curs_empty_data;
	cur_empty.mask	= curs_empty_mask;

	cur_dot.data	= curs_dot_data;
	cur_dot.mask	= curs_dot_mask;

	cur_arrow.data	= curs_arrow_data;
	cur_arrow.mask	= curs_arrow_mask;
	cur_arrow2.data	= curs_arrow2_data;
	cur_arrow2.mask	= curs_arrow2_mask;
	cur_arrow3.data	= curs_arrow3_data;
	cur_arrow3.mask	= curs_arrow3_mask;
	cur_arrow4.data	= curs_arrow4_data;
	cur_arrow4.mask	= curs_arrow4_mask;
	cur_arrow5.data	= curs_arrow5_data;
	cur_arrow5.mask	= curs_arrow5_mask;
	cur_arrow6.data	= curs_arrow6_data;
	cur_arrow6.mask	= curs_arrow6_mask;

	cur_root.data	= curs_root_data;
	cur_root.mask	= curs_root_mask;

	cur_plus.data	= curs_plus_data;
	cur_plus.mask	= curs_plus_mask;

	cur_fleur.data	= curs_fleur_data;
	cur_fleur.mask	= curs_fleur_mask;

	cur_xterm.data	= curs_xterm_data;
	cur_xterm.mask	= curs_xterm_mask;

	curs_copy(cursors[CURS_EMPTY], &cur_empty);	n++;
	curs_copy(cursors[CURS_DOT],   &cur_dot);	n++;

	if (alt_arrow < 1 || alt_arrow > alt_arrow_max) {
		alt_arrow = 1;
	}
	if (alt_arrow == 1) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow);	n++;
	} else if (alt_arrow == 2) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow2);	n++;
	} else if (alt_arrow == 3) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow3);	n++;
	} else if (alt_arrow == 4) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow4);	n++;
	} else if (alt_arrow == 5) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow5);	n++;
	} else if (alt_arrow == 6) {
		curs_copy(cursors[CURS_ARROW], &cur_arrow6);	n++;
	} else {
		alt_arrow = 1;
		curs_copy(cursors[CURS_ARROW], &cur_arrow);	n++;
	}
	curs_copy(cursors[CURS_WARROW], &cur_arrow2);	n++;

	curs_copy(cursors[CURS_ROOT], &cur_root);	n++;
	curs_copy(cursors[CURS_WM],   &cur_fleur);	n++;
	curs_copy(cursors[CURS_TERM], &cur_xterm);	n++;
	curs_copy(cursors[CURS_PLUS], &cur_plus);	n++;

	if (scale_cursor_str) {
		scale = scale_cursor_str;
	} else if (scaling && scale_str) {
		scale = scale_str;
	}
	if (scale && sscanf(scale, "%dx%d", &i, &j) == 2) {
		if (wdpy_x > 0) {
			w_in = wdpy_x; 
			h_in = wdpy_y; 
		} else {
			w_in = dpy_x; 
			h_in = dpy_y; 
		}
	}

	/* scale = NULL zeroes everything */
	parse_scale_string(scale, &scale_cursor_fac_x, &scale_cursor_fac_y, &scaling_cursor,
	    &scaling_cursor_blend, &j, &j, &scaling_cursor_interpolate,
	    &scale_cursor_numer, &scale_cursor_denom, w_in, h_in);

	for (i=0; i<n; i++) {
		/* create rfbCursors for the special cursors: */

		cursor_info_t *ci = cursors[i];

		if (scaling_cursor && (scale_cursor_fac_x != 1.0 || scale_cursor_fac_y != 1.0)) {
			int w, h, x, y, k;
			unsigned long *pixels;

			w = ci->wx;
			h = ci->wy;

			pixels = (unsigned long *) malloc(w * h
			    * sizeof(unsigned long));

			k = 0;
			for (y=0; y<h; y++) {
				for (x=0; x<w; x++) {
					char d = ci->data[k];
					char m = ci->mask[k];
					unsigned long *p;

					p = pixels + k;

					/* set alpha on */
					*p = 0xff000000;

					if (d == ' ' && m == ' ') {
						/* alpha off */
						*p = 0x00000000;
					} else if (d != ' ') {
						/* body */
						if (ci->reverse) {
							*p |= 0x00000000;
						} else {
							*p |= 0x00ffffff;
						}
					} else if (m != ' ') {
						/* edge */
						if (ci->reverse) {
							*p |= 0x00ffffff;
						} else {
							*p |= 0x00000000;
						}
					}
					k++;
				}
			}

			rfb_curs = pixels2curs(pixels, w, h, ci->sx, ci->sy,
			    bpp/8);

			free(pixels);

		} else {

			/* standard X cursor */
			rfb_curs = rfbMakeXCursor(ci->wx, ci->wy,
			    ci->data, ci->mask);

			if (ci->reverse) {
				rfb_curs->foreRed   = 0x0000;
				rfb_curs->foreGreen = 0x0000;
				rfb_curs->foreBlue  = 0x0000;
				rfb_curs->backRed   = 0xffff;
				rfb_curs->backGreen = 0xffff;
				rfb_curs->backBlue  = 0xffff;
			}
			rfb_curs->alphaSource = NULL;

			rfb_curs->xhot = ci->sx;
			rfb_curs->yhot = ci->sy;
			rfb_curs->cleanup = FALSE;
			rfb_curs->cleanupSource = FALSE;
			rfb_curs->cleanupMask = FALSE;
			rfb_curs->cleanupRichSource = FALSE;

			if (bpp == 8 && indexed_color) {
				/*
				 * use richsource in PseudoColor for better
				 * looking cursors (i.e. two-color).
				 */
				int x, y, k = 0, bw;
				int black = 0, white = 1;
				char d, m;

				if (dpy) {	/* raw_fb hack */
					black = BlackPixel(dpy, scr);
					white = WhitePixel(dpy, scr);
				}

				rfb_curs->richSource = (unsigned char *)
				    calloc(ci->wx * ci->wy, 1);

				for (y = 0; y < ci->wy; y++) {
				    for (x = 0; x < ci->wx; x++) {
					d = *(ci->data + k);
					m = *(ci->mask + k);
					if (d == ' ' && m == ' ') {
						k++;
						continue;
					} else if (m != ' ' && d == ' ') {
						bw = black;
					} else {
						bw = white;
					}
					if (ci->reverse) {
						if (bw == black) {
							bw = white;
						} else {
							bw = black;
						}
					}
					*(rfb_curs->richSource+k) =
					    (unsigned char) bw;
					k++;
				    }
				}
			}
		}
		ci->rfb = rfb_curs;
	}
	if (screen) {
		UNLOCK(screen->cursorMutex);
	}
	if (verbose) {
		rfbLog("  done.\n");
	}
	rfbLog("\n");
}

void setup_cursors_and_push(void) {
	setup_cursors();
	first_cursor();
}

/*
 * Descends window tree at pointer until the window cursor matches the current 
 * cursor.  So far only used to detect if mouse is on root background or not.
 * (returns 0 in that case, 1 otherwise).
 *
 */
static void tree_descend_cursor(int *depth, Window *w, win_str_info_t *winfo) {
#if NO_X11
	RAWFB_RET_VOID
	if (!depth || !w || !winfo) {}
	return;
#else
	Window r, c;
	int i, rx, ry, wx, wy;
	unsigned int mask;
	Window wins[10];
	int descend, maxtries = 10;
	char *name, *s = multiple_cursors_mode;
	static XClassHint *classhint = NULL;
	int nm_info = 1;
	XErrorHandler old_handler;

	RAWFB_RET_VOID

	if (!strcmp(s, "default") || !strcmp(s, "X") || !strcmp(s, "arrow")) {
		nm_info = 0;
	}

	*(winfo->wm_name)   = '\0';
	*(winfo->res_name)  = '\0';
	*(winfo->res_class) = '\0';

	for (i=0; i < maxtries; i++) {
		wins[i] = None;
	}

	/* some times a window can go away before we get to it */
	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);

	c = window;
	descend = -1;

	while (c) {
		wins[++descend] = c;
		if (descend >= maxtries - 1) {
			break;
		}
		if ( XTestCompareCurrentCursorWithWindow_wr(dpy, c) ) {
			break;
		}
		/* TBD: query_pointer() */
		XQueryPointer_wr(dpy, c, &r, &c, &rx, &ry, &wx, &wy, &mask);
	}

	if (nm_info) {
		int got_wm_name = 0, got_res_name = 0, got_res_class = 0;

		if (! classhint) {
			classhint = XAllocClassHint();
		}

		for (i = descend; i >=0; i--) {
			c = wins[i];
			if (! c) {
				continue;
			}
			
			if (! got_wm_name && XFetchName(dpy, c, &name)) {
				if (name) {
					if (*name != '\0') {
						strcpy(winfo->wm_name, name);
						got_wm_name = 1;
					}
					XFree_wr(name);
				}
			}
			if (classhint && (! got_res_name || ! got_res_class)) {
			    if (XGetClassHint(dpy, c, classhint)) {
				char *p;
				p = classhint->res_name;
				if (p) {
					if (*p != '\0' && ! got_res_name) {
						strcpy(winfo->res_name, p);
						got_res_name = 1;
					}
					XFree_wr(p);
					classhint->res_name = NULL;
				}
				p = classhint->res_class;
				if (p) {
					if (*p != '\0' && ! got_res_class) {
						strcpy(winfo->res_class, p);
						got_res_class = 1;
					}
					XFree_wr(p);
					classhint->res_class = NULL;
				}
			    }
			}
		}
	}

	XSetErrorHandler(old_handler);
	trapped_xerror = 0;

	*depth = descend;
	*w = wins[descend];
#endif	/* NO_X11 */
}

void initialize_xfixes(void) {
#if LIBVNCSERVER_HAVE_LIBXFIXES
	if (xfixes_present) {
		X_LOCK;
		if (use_xfixes) {
			XFixesSelectCursorInput(dpy, rootwin,
				XFixesDisplayCursorNotifyMask);
		} else {
			XFixesSelectCursorInput(dpy, rootwin, 0);
		}
		X_UNLOCK;
		xfixes_first_initialized = 1;
	}
#endif
}

static rfbCursorPtr pixels2curs(unsigned long *pixels, int w, int h,
    int xhot, int yhot, int Bpp) {
	rfbCursorPtr c;
	static unsigned long black = 0, white = 1;
	static int first = 1;
	char *bitmap, *rich, *alpha;
	char *pixels_new = NULL;
	int n_opaque, n_trans, n_alpha, len, histo[256];
	int send_alpha = 0, alpha_shift = 0, thresh;
	int i, x, y;

	if (first && dpy) {	/* raw_fb hack */
		X_LOCK;
		black = BlackPixel(dpy, scr);
		white = WhitePixel(dpy, scr);
		X_UNLOCK;
		first = 0;
	}

	if (cmap8to24 && cmap8to24_fb && depth <= 16) {
		if (Bpp <= 2) {
			Bpp = 4;
		}
	}

	if (scaling_cursor && (scale_cursor_fac_x != 1.0 || scale_cursor_fac_y != 1.0)) {
		int W, H;
		char *pixels_use = (char *) pixels;
		unsigned int *pixels32 = NULL;

		W = w;
		H = h;

		w = scale_round(W, scale_cursor_fac_x);
		h = scale_round(H, scale_cursor_fac_y);

		pixels_new = (char *) malloc(4*w*h);

		if (sizeof(unsigned long) == 8) {
			int i, j, k = 0;
			/*
			 * to avoid 64bpp code in scale_rect() we knock
			 * down to unsigned int on 64bit machines:
			 */
			pixels32 = (unsigned int*) malloc(4*W*H);
			for (j=0; j<H; j++) {
			    for (i=0; i<W; i++) {
				*(pixels32+k) = 0xffffffff & (*(pixels+k));
				k++;
			    }
			}
			pixels_use = (char *) pixels32;
		}

		scale_rect(scale_cursor_fac_x, scale_cursor_fac_y, scaling_cursor_blend,
		    scaling_cursor_interpolate,
		    4, pixels_use, 4*W, pixels_new, 4*w,
		    W, H, w, h, 0, 0, W, H, 0);

		if (sizeof(unsigned long) == 8) {
			int i, j, k = 0;
			unsigned long *pixels64;
			unsigned int* source = (unsigned int*) pixels_new;
			/*
			 * now knock it back up to unsigned long:
			 */
			pixels64 = (unsigned long*) malloc(8*w*h);
			for (j=0; j<h; j++) {
			    for (i=0; i<w; i++) {
				*(pixels64+k) = (unsigned long) (*(source+k));
				k++;
			    }
			}
			free(pixels_new);
			pixels_new = (char *) pixels64;
			if (pixels32) {
				free(pixels32);
				pixels32 = NULL;
			}
		}
			
		pixels = (unsigned long *) pixels_new;

		xhot = scale_round(xhot, scale_cursor_fac_x);
		yhot = scale_round(yhot, scale_cursor_fac_y);
	}

	len = w * h;
	/* for bitmap data */
	bitmap = (char *) malloc(len+1);
	bitmap[len] = '\0';

	/* for rich cursor pixel data */
	rich  = (char *)calloc(Bpp*len, 1);
	alpha = (char *)calloc(1*len, 1);

	n_opaque = 0;
	n_trans = 0;
	n_alpha = 0;
	for (i=0; i<256; i++) {
		histo[i] = 0;
	}

	i = 0;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			unsigned long a;

			a = 0xff000000 & (*(pixels+i));
			a = a >> 24;	/* alpha channel */
			if (a > 0) {
				n_alpha++;
			}
			histo[a]++;
			if (a < (unsigned int) alpha_threshold) {
				n_trans++;
			} else {
				n_opaque++;
			}
			i++;
		}
	}
	if (alpha_blend) {
		send_alpha = 0;
		if (Bpp == 4) {
			send_alpha = 1;
		}
		alpha_shift = 24;
		if (main_red_shift == 24 || main_green_shift == 24 ||
		    main_blue_shift == 24)  {
			alpha_shift = 0;	/* XXX correct? */
		}
	}
	if (n_opaque >= alpha_frac * n_alpha) {
		thresh = alpha_threshold;
	} else {
		n_opaque = 0;
		for (i=255; i>=0; i--) {
			n_opaque += histo[i];
			thresh = i;
			if (n_opaque >= alpha_frac * n_alpha) {
				break;
			}
		}
	}

	i = 0;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			unsigned long r, g, b, a;
			unsigned int ui;
			char *p;

			a = 0xff000000 & (*(pixels+i));
			a = a >> 24;	/* alpha channel */

			if (a < (unsigned int) thresh) {
				bitmap[i] = ' ';
			} else {
				bitmap[i] = 'x';
			}

			r = 0x00ff0000 & (*(pixels+i));
			g = 0x0000ff00 & (*(pixels+i));
			b = 0x000000ff & (*(pixels+i));
			r = r >> 16;	/* red */
			g = g >> 8;	/* green */
			b = b >> 0;	/* blue */

			if (alpha_remove && a != 0) {
				r = (255 * r) / a;
				g = (255 * g) / a;
				b = (255 * b) / a;
				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;
			}

			if (indexed_color) {
				/*
				 * Choose black or white for
				 * PseudoColor case.
				 */
				int value = (r+g+b)/3;
				if (value > 127) {
					ui = white;
				} else {
					ui = black;
				}
			} else {
				/*
				 * Otherwise map the RGB data onto
				 * the framebuffer format:
				 */
				r = (main_red_max   * r)/255;
				g = (main_green_max * g)/255;
				b = (main_blue_max  * b)/255;
				ui = 0;
				ui |= (r << main_red_shift);
				ui |= (g << main_green_shift);
				ui |= (b << main_blue_shift);
				if (send_alpha) {
					ui |= (a << alpha_shift);
				}
			}

			/* insert value into rich source: */
			p = rich + Bpp*i;

			if (Bpp == 1) {
				*((unsigned char *)p)
				= (unsigned char) ui;
			} else if (Bpp == 2) {
				*((unsigned short *)p)
				= (unsigned short) ui;
			} else if (Bpp == 3) {
				*((unsigned char *)p)
				= (unsigned char) ((ui & 0x0000ff) >> 0);
				*((unsigned char *)(p+1))
				= (unsigned char) ((ui & 0x00ff00) >> 8);
				*((unsigned char *)(p+2))
				= (unsigned char) ((ui & 0xff0000) >> 16);
			} else if (Bpp == 4) {
				*((unsigned int *)p)
				= (unsigned int) ui;
			}

			/* insert alpha value into alpha source: */
			p = alpha + i;
			*((unsigned char *)p) = (unsigned char) a;

			i++;
		}
	}

	/* create the cursor with the bitmap: */
	c = rfbMakeXCursor(w, h, bitmap, bitmap);
	free(bitmap);

	if (pixels_new) {
		free(pixels_new);
	}

	/* set up the cursor parameters: */
	c->xhot = xhot;
	c->yhot = yhot;
	c->cleanup = FALSE;
	c->cleanupSource = FALSE;
	c->cleanupMask = FALSE;
	c->cleanupRichSource = FALSE;
	c->richSource = (unsigned char *) rich;

	/* zeroes mean interpolate the rich cursor somehow and use B+W */
	c->foreRed   = 0;
	c->foreGreen = 0;
	c->foreBlue  = 0;
	c->backRed   = 0;
	c->backGreen = 0;
	c->backBlue  = 0;

	c->source = NULL;

	if (alpha_blend && !indexed_color) {
		c->alphaSource = (unsigned char *) alpha;
		c->alphaPreMultiplied = TRUE;
	} else {
		free(alpha);
		c->alphaSource = NULL;
	}
	return c;
}

static unsigned long last_cursor = 0;
static int last_index = 0;
static time_t curs_times[CURS_MAX];
static unsigned long curs_index[CURS_MAX];

unsigned long get_cursor_serial(int mode) {
	if (mode == 0) {
		return last_cursor;
	} else if (mode == 1) {
		return (unsigned long) last_index;
	} else {
		return (unsigned long) last_index;
	}
}

static int get_exact_cursor(int init) {
	int which = CURS_ARROW;

	if (init) {
		/* zero out our cache (cursors are not freed) */
		int i;
		for (i=0; i<CURS_MAX; i++) {
			curs_times[i] = 0;
			curs_index[i] = 0;
		}
		last_cursor = 0;
		last_index = 0;
		return -1;
	}

#ifdef MACOSX
	if (macosx_console) {
		return macosx_get_cursor();
	}
#endif

	if (rawfb_vnc_reflect) {
		int last_idx = (int) get_cursor_serial(1);
		if (last_idx) {
			which = last_idx;
		}
		return which;
	}
	if (xfixes_present && dpy) {
#if LIBVNCSERVER_HAVE_LIBXFIXES
		int last_idx = (int) get_cursor_serial(1);
		XFixesCursorImage *xfc;

		if (last_idx) {
			which = last_idx;
		}
		if (! xfixes_first_initialized) {
			return which;
		}

		X_LOCK;
		if (! got_xfixes_cursor_notify && xfixes_base_event_type) {
			/* try again for XFixesCursorNotify event */
			XEvent xev;
			if (XCheckTypedEvent(dpy, xfixes_base_event_type +
			    XFixesCursorNotify, &xev)) {
				got_xfixes_cursor_notify++;
			}
		}
		if (! got_xfixes_cursor_notify) {
			/* evidently no cursor change, just return last one */
			X_UNLOCK;
			return which;
		}
		got_xfixes_cursor_notify = 0;

		/* retrieve the cursor info + pixels from server: */
		xfc = XFixesGetCursorImage(dpy);
		X_UNLOCK;
		if (! xfc) {
			/* failure. */
			return which;
		}

		which = store_cursor(xfc->cursor_serial, xfc->pixels,
		    xfc->width, xfc->height, 32, xfc->xhot, xfc->yhot);

		X_LOCK;
		XFree_wr(xfc);
		X_UNLOCK;
#endif
	}
	return(which);
}

int store_cursor(int serial, unsigned long *data, int w, int h, int cbpp,
    int xhot, int yhot) {
	int which = CURS_ARROW;
	int use, oldest, i;
	time_t oldtime, now;

#if 0
fprintf(stderr, "sc: %d  %d/%d %d - %d %d\n", serial, w, h, cbpp, xhot, yhot);
#endif

	oldest = CURS_DYN_MIN;
	if (screen && screen->cursor == cursors[oldest]->rfb) {
		oldest++;
	}
	oldtime = curs_times[oldest];
	now = time(NULL);
	for (i = CURS_DYN_MIN; i <= CURS_DYN_MAX; i++) {
		if (screen && screen->cursor == cursors[i]->rfb) {
			;
		} else if (curs_times[i] < oldtime) {
			/* watch for oldest one to overwrite */
			oldest = i;
			oldtime = curs_times[i];
		}
		if (serial == (int) curs_index[i]) {
			/*
			 * got a hit with an existing cursor,
			 * use that one.
			 */
#ifdef MACOSX
			if (now > curs_times[i] + 1) {
				continue;
			}
#endif
			last_cursor = curs_index[i];
			curs_times[i] = now;
			last_index = i;
			return last_index;
		}
	}

	/* we need to create the cursor and overwrite oldest */
	use = oldest;
	if (cursors[use]->rfb) {
		/* clean up oldest if it exists */
		if (cursors[use]->rfb->richSource) {
			free(cursors[use]->rfb->richSource);
			cursors[use]->rfb->richSource = NULL;
		}
		if (cursors[use]->rfb->alphaSource) {
			free(cursors[use]->rfb->alphaSource);
			cursors[use]->rfb->alphaSource = NULL;
		}
		if (cursors[use]->rfb->source) {
			free(cursors[use]->rfb->source);
			cursors[use]->rfb->source = NULL;
		}
		if (cursors[use]->rfb->mask) {
			free(cursors[use]->rfb->mask);
			cursors[use]->rfb->mask = NULL;
		}
		free(cursors[use]->rfb);
		cursors[use]->rfb = NULL;
	}

	if (rotating && rotating_cursors) {
		char *dst;
		int tx, ty;

		dst = (char *) malloc(w * h * cbpp/8);
		rotate_curs(dst, (char *) data, w, h, cbpp/8);

		memcpy(data, dst, w * h * cbpp/8);
		free(dst);

		rotate_coords(xhot, yhot, &tx, &ty, w, h);
		xhot = tx;
		yhot = ty;
		if (! rotating_same) {
			int tmp = w;
			w = h;
			h = tmp;
		}
	}

	/* place cursor into our collection */
	cursors[use]->rfb = pixels2curs(data, w, h, xhot, yhot, bpp/8);

	/* update time and serial index: */
	curs_times[use] = now;
	curs_index[use] = serial;
	last_index = use;
	last_cursor = serial;

	which = last_index;

	return which;
}

int known_cursors_mode(char *s) {
/*
 * default:	see initialize_cursors_mode() for default behavior.
 * arrow:	unchanging white arrow.
 * Xn*:		show X on root background.  Optional n sets treedepth.
 * some:	do the heuristics for root, wm, term detection.
 * most:	if display have overlay or xfixes, show all cursors,
 *		otherwise do the same as "some"
 * none:	show no cursor.
 */
	if (strcmp(s, "default") && strcmp(s, "arrow") && *s != 'X' &&
	    strcmp(s, "some") && strcmp(s, "most") && strcmp(s, "none")) {
		return 0;
	} else {
		return 1;
	}
}

void initialize_cursors_mode(void) {
	char *s = multiple_cursors_mode;
	if (!s || !known_cursors_mode(s)) {
		rfbLog("unknown cursors mode: %s\n", s);
		rfbLog("resetting cursors mode to \"default\"\n");
		if (multiple_cursors_mode) free(multiple_cursors_mode);
		multiple_cursors_mode = strdup("default");
		s = multiple_cursors_mode;
	}
	if (!strcmp(s, "none")) {
		show_cursor = 0;
	} else {
		/* we do NOT set show_cursor = 1, let the caller do that */
	}

	show_multiple_cursors = 0;
	if (show_cursor) {
		if (!strcmp(s, "default")) {
			if(multiple_cursors_mode) free(multiple_cursors_mode);
			multiple_cursors_mode = strdup("X");
			s = multiple_cursors_mode;
		}
		if (*s == 'X' || !strcmp(s, "some") || !strcmp(s, "most")) {
			show_multiple_cursors = 1;
		} else {
			show_multiple_cursors = 0;
			/* hmmm, some bug going back to arrow mode.. */
			set_rfb_cursor(CURS_ARROW);
		}
		if (screen) {
			set_cursor_was_changed(screen);
		}
	} else {
		if (screen) {
			LOCK(screen->cursorMutex);
			screen->cursor = NULL;
			UNLOCK(screen->cursorMutex);
			set_cursor_was_changed(screen);
		}
	}
}

int get_which_cursor(void) {
	int which = CURS_ARROW;
	int db = 0;

	if (show_multiple_cursors) {
		int depth = 0, rint;
		static win_str_info_t winfo;
		static int first = 1, depth_cutoff = -1;
		Window win = None;
		XErrorHandler old_handler;
		int mode = 0;

		if (drag_in_progress || button_mask) {
			/* XXX not exactly what we want for menus */
			if (! cursor_drag_changes) {
				return -1;
			}
		}

		if (!strcmp(multiple_cursors_mode, "arrow")) {
			/* should not happen... */
			return CURS_ARROW;
		} else if (!strcmp(multiple_cursors_mode, "default")) {
			mode = 0;
		} else if (!strcmp(multiple_cursors_mode, "X")) {
			mode = 1;
		} else if (!strcmp(multiple_cursors_mode, "some")) {
			mode = 2;
		} else if (!strcmp(multiple_cursors_mode, "most")) {
			mode = 3;
		}

		if (rawfb_vnc_reflect && mode > -1) {
			rint = get_exact_cursor(0);
			return rint;
		}
		if (mode == 3) {
			if ((xfixes_present && use_xfixes) || macosx_console) {
				if (db) fprintf(stderr, "get_which_cursor call get_exact_cursor\n");
				rint = get_exact_cursor(0);
				return rint;
			}
		}

		if (depth_cutoff < 0) {
			int din;
			if (sscanf(multiple_cursors_mode, "X%d", &din) == 1) {
				depth_cutoff = din;
			} else {
				depth_cutoff = 0;
			}
		}

		if (first) {
			winfo.wm_name   = (char *) malloc(1024);
			winfo.res_name  = (char *) malloc(1024);
			winfo.res_class = (char *) malloc(1024);
		}
		first = 0;
		
		X_LOCK;
		tree_descend_cursor(&depth, &win, &winfo);
		X_UNLOCK;

		if (depth <= depth_cutoff && !subwin) {
			which = CURS_ROOT;

		} else if (mode == 2 || mode == 3) {
			int which0 = which;

			/* apply crude heuristics to choose a cursor... */
			if (win && dpy) {
				int ratio = 10, x, y;
				unsigned int w, h, bw, d;  
				Window r;

#if !NO_X11
				trapped_xerror = 0;
				X_LOCK;
				old_handler = XSetErrorHandler(trap_xerror);

				/* "narrow" windows are WM */
				if (XGetGeometry(dpy, win, &r, &x, &y, &w, &h,
				    &bw, &d)) {
					if (w > ratio * h || h > ratio * w) {
						which = CURS_WM;
					}
				}
				XSetErrorHandler(old_handler);
				X_UNLOCK;
				trapped_xerror = 0;
#else
				if (!r || !d || !bw || !h || !w || !y || !x || !ratio || !old_handler) {}
#endif	/* NO_X11 */
			}
			if (which == which0) {
				/* the string "term" means I-beam. */
				char *name, *class;
				lowercase(winfo.res_name);
				lowercase(winfo.res_class);
				name  = winfo.res_name;
				class = winfo.res_class;
				if (strstr(name, "term")) {
					which = CURS_TERM;
				} else if (strstr(class, "term")) {
					which = CURS_TERM;
				} else if (strstr(name,  "text")) {
					which = CURS_TERM;
				} else if (strstr(class, "text")) {
					which = CURS_TERM;
				} else if (strstr(name,  "onsole")) {
					which = CURS_TERM;
				} else if (strstr(class, "onsole")) {
					which = CURS_TERM;
				} else if (strstr(name,  "cmdtool")) {
					which = CURS_TERM;
				} else if (strstr(class, "cmdtool")) {
					which = CURS_TERM;
				} else if (strstr(name,  "shelltool")) {
					which = CURS_TERM;
				} else if (strstr(class, "shelltool")) {
					which = CURS_TERM;
				}
			}
		}
	}
	if (db) fprintf(stderr, "get_which_cursor which: %d\n", which);
	return which;
}

static void set_cursor_was_changed(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (! s) {
		return;
	}
	iter = rfbGetClientIterator(s);
	LOCK(screen->cursorMutex);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		cl->cursorWasChanged = TRUE;
	}
	UNLOCK(screen->cursorMutex);
	rfbReleaseClientIterator(iter);
}

#if 0
/* not yet used */
static void set_cursor_was_moved(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (! s) {
		return;
	}
	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		cl->cursorWasMoved = TRUE;
	}
	rfbReleaseClientIterator(iter);
}
#endif

void restore_cursor_shape_updates(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int count = 0;

	if (! s || ! s->clientHead) {
		return;
	}
	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		int changed = 0;
		ClientData *cd = (ClientData *) cl->clientData;

		if (! cd) {
			continue;
		}
		if (cd->had_cursor_shape_updates) {
			rfbLog("restoring enableCursorShapeUpdates for client"
			    " 0x%x\n", cl);
			cl->enableCursorShapeUpdates = TRUE;	
			changed = 1;
		}
		if (cd->had_cursor_pos_updates) {
			rfbLog("restoring enableCursorPosUpdates for client"
			    " 0x%x\n", cl);
			cl->enableCursorPosUpdates = TRUE;	
			changed = 1;
		}
		if (changed) {
			cl->cursorWasChanged = TRUE;
			count++;
		}
	}
	rfbReleaseClientIterator(iter);
}

void disable_cursor_shape_updates(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	static int changed = 0;
	int count = 0;

	if (! s || ! s->clientHead) {
		return;
	}
	if (unixpw_in_progress) return;

	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		ClientData *cd;
		cd = (ClientData *) cl->clientData;

		if (cl->enableCursorShapeUpdates) {
			if (cd) {
				cd->had_cursor_shape_updates = 1;
			}
			count++;
			if (debug_pointer) {
				rfbLog("%s disable HCSU\n", cl->host);
			}
		}
		if (cl->enableCursorPosUpdates) {
			if (cd) {
				cd->had_cursor_pos_updates = 1;
			}
			count++;
			if (debug_pointer) {
				rfbLog("%s disable HCPU\n", cl->host);
			}
		}
		
		cl->enableCursorShapeUpdates = FALSE;
		cl->enableCursorPosUpdates = FALSE;
		cl->cursorWasChanged = FALSE;
	}
	rfbReleaseClientIterator(iter);

	if (count) {
		changed = 1;
	}
}

int cursor_shape_updates_clients(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int count = 0;

	if (! s) {
		return 0;
	}
	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (cl->enableCursorShapeUpdates) {
			count++;
		}
	}
	rfbReleaseClientIterator(iter);
	return count;
}

int cursor_noshape_updates_clients(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int count = 0;

	if (! s) {
		return 0;
	}
	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (!cl->enableCursorShapeUpdates) {
			count++;
		}
	}
	rfbReleaseClientIterator(iter);
	return count;
}

int cursor_pos_updates_clients(rfbScreenInfoPtr s) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int count = 0;

	if (! s) {
		return 0;
	}
	iter = rfbGetClientIterator(s);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (cl->enableCursorPosUpdates) {
			count++;
		}
	}
	rfbReleaseClientIterator(iter);
	return count;
}

/*
 * Record rfb cursor position screen->cursorX, etc (a la defaultPtrAddEvent())
 * Then set up for sending rfbCursorPosUpdates back
 * to clients that understand them.  This seems to be TightVNC specific.
 */
void cursor_position(int x, int y) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int cnt = 0, nonCursorPosUpdates_clients = 0;
	int x_in = x, y_in = y;

	/* x and y are current positions of X11 pointer on the X11 display */
	if (!screen) {
		return;
	}

	if (scaling) {
		x = ((double) x / dpy_x) * scaled_x;
		x = nfix(x, scaled_x);
		y = ((double) y / dpy_y) * scaled_y;
		y = nfix(y, scaled_y);
	}

	if (clipshift) {
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= dpy_x) x = dpy_x-1;
		if (y >= dpy_y) y = dpy_y-1;
	}

	if (x == screen->cursorX && y == screen->cursorY) {
		return;
	}

	LOCK(screen->cursorMutex);
	screen->cursorX = x;
	screen->cursorY = y;
	UNLOCK(screen->cursorMutex);

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (! cl->enableCursorPosUpdates) {
			nonCursorPosUpdates_clients++;
			continue;
		}
		if (! cursor_pos_updates) {
			continue;
		}
		if (cl == last_pointer_client) {
			/*
			 * special case if this client was the last one to
			 * send a pointer position.
			 */
			if (x_in == cursor_x && y_in == cursor_y) {
				cl->cursorWasMoved = FALSE;
			} else {
				/* an X11 app evidently warped the pointer */
				if (debug_pointer) {
					rfbLog("cursor_position: warp "
					    "detected dx=%3d dy=%3d\n",
					    cursor_x - x, cursor_y - y);
				}
				cl->cursorWasMoved = TRUE;
				cnt++;
			}
		} else {
			cl->cursorWasMoved = TRUE;
			cnt++;
		}
	}
	rfbReleaseClientIterator(iter);

	if (debug_pointer && cnt) {
		rfbLog("cursor_position: sent position x=%3d y=%3d to %d"
		    " clients\n", x, y, cnt);
	}
}

static void set_rfb_cursor(int which) {

	if (! show_cursor) {
		return;
	}
	if (! screen) {
		return;
	}
	
	if (!cursors[which] || !cursors[which]->rfb) {
		rfbLog("non-existent cursor: which=%d\n", which);
		return;
	} else {
		rfbSetCursor(screen, cursors[which]->rfb);
	}
}

void set_no_cursor(void) {
	set_rfb_cursor(CURS_EMPTY);
}

void set_warrow_cursor(void) {
	set_rfb_cursor(CURS_WARROW);
}

int set_cursor(int x, int y, int which) {
	static int last = -1;
	int changed_cursor = 0;

	if (x || y) {} /* unused vars warning: */

	if (which < 0) {
		which = last;	
	}
	if (last < 0 || which != last) {
		set_rfb_cursor(which);
		changed_cursor = 1;
	}
	last = which;

	return changed_cursor;
}

/*
 * routine called periodically to update cursor aspects, this catches
 * warps and cursor shape changes. 
 */
int check_x11_pointer(void) {
	Window root_w, child_w;
	rfbBool ret = 0;
	int root_x, root_y, win_x, win_y;
	int x, y, rint;
	unsigned int mask;

	if (unixpw_in_progress) return 0;

#ifdef MACOSX
	if (macosx_console) {
		ret = macosx_get_cursor_pos(&root_x, &root_y);
	} else {
		RAWFB_RET(0)
	}
#else

	RAWFB_RET(0)

#   if NO_X11
	return 0;
#   endif

#endif


#if ! NO_X11
	if (dpy) {
		X_LOCK;
		ret = XQueryPointer_wr(dpy, rootwin, &root_w, &child_w, &root_x, &root_y,
		    &win_x, &win_y, &mask);
		X_UNLOCK;
	}
#else
	if (!mask || !win_y || !win_x || !child_w || !root_w) {}
#endif	/* NO_X11 */

if (0) fprintf(stderr, "check_x11_pointer %d %d\n", root_x, root_y);
	if (! ret) {
		return 0;
	}
	if (debug_pointer) {
		static int last_x = -1, last_y = -1;
		if (root_x != last_x || root_y != last_y) {
			rfbLog("XQueryPointer:     x:%4d, y:%4d)\n",
			    root_x, root_y);
		}
		last_x = root_x;
		last_y = root_y;
	}

	/* offset subtracted since XQueryPointer relative to rootwin */
	x = root_x - off_x - coff_x;
	y = root_y - off_y - coff_y;

	if (clipshift) {
		static int cnt = 0;
		if (x < 0 || y < 0 || x >= dpy_x || y >= dpy_y)  {
			if (cnt++ % 4 != 0) {
				if (debug_pointer) {
					rfbLog("Skipping cursor_position() outside our clipshift\n");
				}
				return 0;
			}
		}
	}

	/* record the cursor position in the rfb screen */
	cursor_position(x, y);

	/* change the cursor shape if necessary */
	rint = set_cursor(x, y, get_which_cursor());
	return rint;
}

