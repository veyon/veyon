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

/* -- screen.c -- */

#include "x11vnc.h"
#include "xevents.h"
#include "xwrappers.h"
#include "xinerama.h"
#include "xdamage.h"
#include "win_utils.h"
#include "cleanup.h"
#include "userinput.h"
#include "scan.h"
#include "user.h"
#include "rates.h"
#include "pointer.h"
#include "keyboard.h"
#include "cursor.h"
#include "connections.h"
#include "remote.h"
#include "unixpw.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "v4l.h"
#include "linuxfb.h"
#include "macosx.h"
#include "macosxCG.h"
#include "avahi.h"
#include "solid.h"
#include "inet.h"
#include "xrandr.h"
#include "xrecord.h"
#include "pm.h"

#include <rfb/rfbclient.h>

void set_greyscale_colormap(void);
void set_hi240_colormap(void);
void unset_colormap(void);
void set_colormap(int reset);
void set_nofb_params(int restore);
void set_raw_fb_params(int restore);
void do_new_fb(int reset_mem);
void free_old_fb(void);
void check_padded_fb(void);
void install_padded_fb(char *geom);
XImage *initialize_xdisplay_fb(void);
void parse_scale_string(char *str, double *factor_x, double *factor_y, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom, int w_in, int h_in);
int parse_rotate_string(char *str, int *mode);
int scale_round(int len, double fac);
void initialize_screen(int *argc, char **argv, XImage *fb);
void set_vnc_desktop_name(void);
void announce(int lport, int ssl, char *iface);

char *vnc_reflect_guess(char *str, char **raw_fb_addr);
void vnc_reflect_process_client(void);
rfbBool vnc_reflect_send_pointer(int x, int y, int mask);
rfbBool vnc_reflect_send_key(uint32_t key, rfbBool down);
rfbBool vnc_reflect_send_cuttext(char *str, int len);

static void debug_colormap(XImage *fb);
static void set_visual(char *str);
static void nofb_hook(rfbClientPtr cl);
static void remove_fake_fb(void);
static void install_fake_fb(int w, int h, int bpp);
static void initialize_snap_fb(void);
XImage *initialize_raw_fb(int);
static void initialize_clipshift(void);
static int wait_until_mapped(Window win);
static void setup_scaling(int *width_in, int *height_in);

static void check_filexfer(void);
static void record_last_fb_update(void);
static void check_cursor_changes(void);
static int choose_delay(double dt);

int rawfb_reset = -1;
int rawfb_dev_video = 0;
int rawfb_vnc_reflect = 0;

/*
 * X11 and rfb display/screen related routines
 */

/*
 * Some handling of 8bpp PseudoColor colormaps.  Called for initializing
 * the clients and dynamically if -flashcmap is specified.
 */
#define NCOLOR 256

/* this is only for rawfb */
void set_greyscale_colormap(void) {
	int i;
	if (! screen) {
		return;
	}
	/* mutex */
	if (screen->colourMap.data.shorts) {
		free(screen->colourMap.data.shorts);
		screen->colourMap.data.shorts = NULL;
	}

if (0) fprintf(stderr, "set_greyscale_colormap: %s\n", raw_fb_pixfmt);
	screen->colourMap.count = NCOLOR;
	screen->serverFormat.trueColour = FALSE;
	screen->colourMap.is16 = TRUE;
	screen->colourMap.data.shorts = (unsigned short *)
		malloc(3*sizeof(unsigned short) * NCOLOR);

	for(i = 0; i < NCOLOR; i++) {
		unsigned short lvl = i * 256;

		screen->colourMap.data.shorts[i*3+0] = lvl;
		screen->colourMap.data.shorts[i*3+1] = lvl;
		screen->colourMap.data.shorts[i*3+2] = lvl;
	}

	rfbSetClientColourMaps(screen, 0, NCOLOR);
}

/* this is specific to bttv rf tuner card */
void set_hi240_colormap(void) {
	int i;
	if (! screen) {
		return;
	}
	/* mutex */
if (0) fprintf(stderr, "set_hi240_colormap: %s\n", raw_fb_pixfmt);
	if (screen->colourMap.data.shorts) {
		free(screen->colourMap.data.shorts);
		screen->colourMap.data.shorts = NULL;
	}

	screen->colourMap.count = 256;
	screen->serverFormat.trueColour = FALSE;
	screen->colourMap.is16 = TRUE;
	screen->colourMap.data.shorts = (unsigned short *)
		calloc(3*sizeof(unsigned short) * 256, 1);

	for(i = 0; i < 225; i++) {
		int r, g, b;

		r = ( (i/5) % 5 ) * 255.0 / 4 + 0.5;
		g = ( (i/25)    ) * 255.0 / 8 + 0.5;
		b = ( i % 5     ) * 255.0 / 4 + 0.5;

		screen->colourMap.data.shorts[(i+16)*3+0] = (unsigned short) (r * 256);
		screen->colourMap.data.shorts[(i+16)*3+1] = (unsigned short) (g * 256);
		screen->colourMap.data.shorts[(i+16)*3+2] = (unsigned short) (b * 256);
	}

	rfbSetClientColourMaps(screen, 0, 256);
}

/* this is only for rawfb */
void unset_colormap(void) {
	if (! screen) {
		return;
	}
	if (screen->colourMap.data.shorts) {
		free(screen->colourMap.data.shorts);
		screen->colourMap.data.shorts = NULL;
	}
	screen->serverFormat.trueColour = TRUE;
if (0) fprintf(stderr, "unset_colormap: %s\n", raw_fb_pixfmt);
}

/* this is X11 case */
void set_colormap(int reset) {

#if NO_X11
	if (!reset) {}
	return;
#else
	static int init = 1;
	static XColor *color = NULL, *prev = NULL;
	static int ncolor = 0;
	Colormap cmap;
	Visual *vis;
	int i, ncells, diffs = 0;

	if (reset) {
		init = 1;
		ncolor = 0;
		/* mutex */
		if (screen->colourMap.data.shorts) {
			free(screen->colourMap.data.shorts);
			screen->colourMap.data.shorts = NULL;
		}
		if (color) {
			free(color);
			color = NULL;
		}
		if (prev) {
			free(prev);
			prev = NULL;
		}
	}

	if (init) {
		if (depth > 16) {
			ncolor = NCOLOR;
		} else if (depth > 8) {
			ncolor = 1 << depth;
		} else {
			ncolor = NCOLOR;
		}
		/* mutex */
		screen->colourMap.count = ncolor;
		screen->serverFormat.trueColour = FALSE;
		screen->colourMap.is16 = TRUE;
		screen->colourMap.data.shorts = (unsigned short *)
			malloc(3*sizeof(unsigned short) * ncolor);
	}
	if (color == NULL) {
		color = (XColor *) calloc(ncolor * sizeof(XColor), 1);
		prev  = (XColor *) calloc(ncolor * sizeof(XColor), 1);
	}

	for (i=0; i < ncolor; i++) {
		prev[i].red   = color[i].red;
		prev[i].green = color[i].green;
		prev[i].blue  = color[i].blue;
	}

	RAWFB_RET_VOID

	X_LOCK;

	cmap = DefaultColormap(dpy, scr);
	ncells = CellsOfScreen(ScreenOfDisplay(dpy, scr));
	vis = default_visual;

	if (subwin) {
		XWindowAttributes attr;

		if (XGetWindowAttributes(dpy, window, &attr)) {
			cmap = attr.colormap;
			vis = attr.visual;
			ncells = vis->map_entries;
		}
	}

	if (ncells != ncolor) {
		if (! shift_cmap) {
			screen->colourMap.count = ncells;
		}
	}
	if (init && ! quiet) {
		rfbLog("set_colormap: number of cells: %d, "
		    "ncolor(%d) is %d.\n", ncells, depth, ncolor);
	}

	if (flash_cmap && ! init) {
		XWindowAttributes attr;
		Window c;
		int tries = 0;

		c = window;
		while (c && tries++ < 16) {
			c = query_pointer(c);
			if (valid_window(c, &attr, 0)) {
				if (attr.colormap && attr.map_installed) {
					cmap = attr.colormap;
					vis = attr.visual;
					ncells = vis->map_entries;
					break;
				}
			} else {
				break;
			}
		}
	}
	if (ncells > ncolor && ! quiet) {
		rfbLog("set_colormap: big problem: ncells=%d > %d\n",
		    ncells, ncolor);
	}

	if (vis->class == TrueColor || vis->class == DirectColor) {
		/*
		 * Kludge to make 8bpp TrueColor & DirectColor be like
		 * the StaticColor map.  The ncells = 8 is "8 per subfield"
		 * mentioned in xdpyinfo.  Looks OK... perhaps fortuitously.
		 */
		if (ncells == 8 && ! shift_cmap) {
			ncells = ncolor;
		}
	}

	for (i=0; i < ncells; i++) {
		color[i].pixel = i;
		color[i].pad = 0;
	}

	XQueryColors(dpy, cmap, color, ncells);

	X_UNLOCK;

	for(i = ncells - 1; i >= 0; i--) {
		int k = i + shift_cmap;

		screen->colourMap.data.shorts[i*3+0] = color[i].red;
		screen->colourMap.data.shorts[i*3+1] = color[i].green;
		screen->colourMap.data.shorts[i*3+2] = color[i].blue;

		if (prev[i].red   != color[i].red ||
		    prev[i].green != color[i].green || 
		    prev[i].blue  != color[i].blue ) {
			diffs++;
		}

		if (shift_cmap && k >= 0 && k < ncolor) {
			/* kludge to copy the colors to higher pixel values */
			screen->colourMap.data.shorts[k*3+0] = color[i].red;
			screen->colourMap.data.shorts[k*3+1] = color[i].green;
			screen->colourMap.data.shorts[k*3+2] = color[i].blue;
		}
	}

	if (diffs && ! init) {
		if (! all_clients_initialized()) {
			rfbLog("set_colormap: warning: sending cmap "
			    "with uninitialized clients.\n");
		}
		if (shift_cmap) {
			rfbSetClientColourMaps(screen, 0, ncolor);
		} else {
			rfbSetClientColourMaps(screen, 0, ncells);
		}
	}

	init = 0;
#endif	/* NO_X11 */
}

static void debug_colormap(XImage *fb) {
	static int debug_cmap = -1;
	int i, k, *histo;
	int ncolor;

	if (debug_cmap < 0) {
		if (getenv("DEBUG_CMAP") != NULL) {
			debug_cmap = 1;
		} else {
			debug_cmap = 0;
		}
	}
	if (! debug_cmap) {
		return;
	}
	if (! fb) {
		return;
	}
	if (fb->bits_per_pixel > 16) {
		return;
	}
	ncolor = screen->colourMap.count;
	histo = (int *) calloc(ncolor * sizeof(int), 1);

	for (i=0; i < ncolor; i++) {
		histo[i] = 0;
	}
	for (k = 0; k < fb->width * fb->height; k++) {
		unsigned char n;
		char c = *(fb->data + k);

		n = (unsigned char) c;
		histo[n]++;
	}
	fprintf(stderr, "\nColormap histogram for current screen contents:\n");
	for (i=0; i < ncolor; i++) {
		unsigned short r = screen->colourMap.data.shorts[i*3+0];
		unsigned short g = screen->colourMap.data.shorts[i*3+1];
		unsigned short b = screen->colourMap.data.shorts[i*3+2];

		fprintf(stderr, "   %03d: %7d %04x/%04x/%04x", i, histo[i],
		    r, g, b);
		if ((i+1) % 2 == 0)  {
			fprintf(stderr, "\n");
		}
	}
	free(histo);
	fprintf(stderr, "\n");
}

/*
 * Experimental mode to force the visual of the window instead of querying
 * it.  Used for testing, overriding some rare cases (win2vnc), and for
 * -overlay .  Input string can be a decimal or 0x hex or something like
 * TrueColor or TrueColor:24 to force a depth as well.
 *
 * visual_id and possibly visual_depth are set.
 */
static void set_visual(char *str) {
#if NO_X11
	RAWFB_RET_VOID
	if (!str) {}
	return;
#else
	int vis, vdepth, defdepth;
	XVisualInfo vinfo;
	char *p, *vstring = strdup(str);

	RAWFB_RET_VOID

	defdepth = DefaultDepth(dpy, scr);
	visual_id = (VisualID) 0;
	visual_depth = 0;

	if (!strcmp(vstring, "ignore") || !strcmp(vstring, "default")
	    || !strcmp(vstring, "")) {
		free(vstring);
		return;
	}

	/* set visual depth */
	if ((p = strchr(vstring, ':')) != NULL) {
		visual_depth = atoi(p+1);
		*p = '\0';
		vdepth = visual_depth;
	} else {
		vdepth = defdepth; 
	}
	if (! quiet) {
		fprintf(stderr, "\nVisual Info:\n");
		fprintf(stderr, " set_visual(\"%s\")\n", str);
		fprintf(stderr, " visual_depth: %d\n", vdepth);
	}

	/* set visual id number */
	if (strcmp(vstring, "StaticGray") == 0) {
		vis = StaticGray;
	} else if (strcmp(vstring, "GrayScale") == 0) {
		vis = GrayScale;
	} else if (strcmp(vstring, "StaticColor") == 0) {
		vis = StaticColor;
	} else if (strcmp(vstring, "PseudoColor") == 0) {
		vis = PseudoColor;
	} else if (strcmp(vstring, "TrueColor") == 0) {
		vis = TrueColor;
	} else if (strcmp(vstring, "DirectColor") == 0) {
		vis = DirectColor;
	} else {
		unsigned int v_in;
		if (sscanf(vstring, "0x%x", &v_in) != 1) {
			if (sscanf(vstring, "%u", &v_in) == 1) {
				visual_id = (VisualID) v_in;
				return;
			}
			rfbLogEnable(1);
			rfbLog("invalid -visual arg: %s\n", vstring);
			X_UNLOCK;
			clean_up_exit(1);
		}
		visual_id = (VisualID) v_in;
		free(vstring);
		return;
	}

	if (! quiet) fprintf(stderr, " visual: %d\n", vis);
	if (XMatchVisualInfo(dpy, scr, visual_depth, vis, &vinfo)) {
		;
	} else if (XMatchVisualInfo(dpy, scr, defdepth, vis, &vinfo)) {
		;
	} else {
		rfbLogEnable(1);
		rfbLog("could not find visual: %s\n", vstring);
		X_UNLOCK;
		clean_up_exit(1);
	}
	free(vstring);

	/* set numerical visual id. */
	visual_id = vinfo.visualid;
#endif	/* NO_X11 */
}

void set_nofb_params(int restore) {
	static int first = 1;
	static int save[100];
	static char *scroll = NULL;
	int i = 0;

	if (first) {
		first = 0;
		save[i++] = use_xfixes;
		save[i++] = use_xdamage;
		save[i++] = use_xrecord;
		save[i++] = wireframe;
		save[i++] = use_solid_bg;
		save[i++] = overlay;
		save[i++] = overlay_cursor;
		save[i++] = using_shm;
		save[i++] = single_copytile;
		save[i++] = take_naps;
		save[i++] = measure_speeds;
		save[i++] = grab_buster;
		save[i++] = show_cursor;
		save[i++] = cursor_shape_updates;
		save[i++] = cursor_pos_updates;
		save[i++] = ncache;

		scroll = scroll_copyrect;
	}
	if (restore) {
		i = 0;
		use_xfixes            = save[i++];
		use_xdamage           = save[i++];
		use_xrecord           = save[i++];
		wireframe             = save[i++];
		use_solid_bg          = save[i++];
		overlay               = save[i++];
		overlay_cursor        = save[i++];
		using_shm             = save[i++];
		single_copytile       = save[i++];
		take_naps             = save[i++];
		measure_speeds        = save[i++];
		grab_buster           = save[i++];
		show_cursor           = save[i++];
		cursor_shape_updates  = save[i++];
		cursor_pos_updates    = save[i++];
		ncache                = save[i++];

		scroll_copyrect = scroll;

		if (cursor_shape_updates) {
			restore_cursor_shape_updates(screen);
		}
		initialize_cursors_mode();

		return;
	}

	use_xfixes = 0;
	use_xdamage = 0;
	use_xrecord = 0;
	wireframe = 0;

	use_solid_bg = 0;
	overlay = 0;
	overlay_cursor = 0;

	using_shm = 0;
	single_copytile = 1;

	take_naps = 0;
	measure_speeds = 0;

	/* got_grab_buster? */
	grab_buster = 0;

	show_cursor = 0;
	show_multiple_cursors = 0;
	cursor_shape_updates = 0;
	if (! got_cursorpos) {
		cursor_pos_updates = 0;
	}

	ncache = 0;

	scroll_copyrect = "never";

	if (! quiet) {
		rfbLog("disabling: xfixes, xdamage, solid, overlay, shm,\n");
		rfbLog("  wireframe, scrollcopyrect, ncache,\n");
		rfbLog("  noonetile, nap, cursor, %scursorshape\n",
		    got_cursorpos ? "" : "cursorpos, " );
		rfbLog("  in -nofb mode.\n");
	}
}

static char *raw_fb_orig_dpy = NULL;

void set_raw_fb_params(int restore) {
	static int first = 1;
	static int vo0, us0, sm0, ws0, wp0, wc0, wb0, na0, tn0;  
	static int xr0, xrm0, sb0, re0;
	static char *mc0;

	/*
	 * set turn off a bunch of parameters not compatible with 
	 * -rawfb mode: 1) ignoring the X server 2) ignoring user input. 
	 */
	
	if (first) {
		/* at least save the initial settings... */
		vo0 = view_only;
		ws0 = watch_selection;
		wp0 = watch_primary;
		wc0 = watch_clipboard;
		wb0 = watch_bell;
		na0 = no_autorepeat;
		sb0 = use_solid_bg;

		us0 = use_snapfb;
		sm0 = using_shm;
		tn0 = take_naps;
		xr0 = xrandr;
		xrm0 = xrandr_maybe;
		re0 = noxrecord;
		mc0 = multiple_cursors_mode;

		first = 0;
	}

	if (restore) {
		view_only = vo0;
		watch_selection = ws0;
		watch_primary = wp0;
		watch_clipboard = wc0;
		watch_bell = wb0;
		no_autorepeat = na0;
		use_solid_bg = sb0;

		use_snapfb = us0;
		using_shm = sm0;
		take_naps = tn0;
		xrandr = xr0;
		xrandr_maybe = xrm0;
		noxrecord = re0;
		multiple_cursors_mode = mc0;

		if (! dpy && raw_fb_orig_dpy) {
			dpy = XOpenDisplay_wr(raw_fb_orig_dpy);
			last_open_xdisplay = time(NULL);
			if (dpy) {
				if (! quiet) rfbLog("reopened DISPLAY: %s\n",
				    raw_fb_orig_dpy);
				scr = DefaultScreen(dpy);
				rootwin = RootWindow(dpy, scr);
				check_xevents(1);
			} else {
				if (! quiet) rfbLog("WARNING: failed to reopen "
				    "DISPLAY: %s\n", raw_fb_orig_dpy);
			}
		}
		return;
	}

	if (verbose) {
		rfbLog("set_raw_fb_params: modifying settings for "
		    "-rawfb mode.\n");
	}

	if (got_noviewonly) {
		/*
		 * The user input parameters are not unset under
		 * -noviewonly... this usage should be very rare
		 * (i.e. rawfb but also send user input to the X
		 * display, most likely using /dev/fb0 for some reason...)
		 */
		if (verbose) {
		   rfbLog("rawfb: -noviewonly mode: still sending mouse and\n");
		   rfbLog("rawfb:   keyboard input to the X DISPLAY!!\n");
		}
	} else {
		/* Normal case: */
#if 0
		if (! view_only && ! pipeinput_str) {
			if (! quiet) rfbLog("  rawfb: setting view_only\n");
			view_only = 1;
		}
#endif
		if (raw_fb_str && strstr(raw_fb_str, "vnc") == raw_fb_str) {
			;
		} else if (watch_selection) {
			if (verbose) rfbLog("  rawfb: turning off "
			    "watch_selection\n");
			watch_selection = 0;
		}
		if (watch_primary) {
			if (verbose) rfbLog("  rawfb: turning off "
			    "watch_primary\n");
			watch_primary = 0;
		}
		if (watch_clipboard) {
			if (verbose) rfbLog("  rawfb: turning off "
			    "watch_clipboard\n");
			watch_clipboard = 0;
		}
		if (watch_bell) {
			if (verbose) rfbLog("  rawfb: turning off watch_bell\n");
			watch_bell = 0;
		}
		if (no_autorepeat) {
			if (verbose) rfbLog("  rawfb: turning off "
			    "no_autorepeat\n");
			no_autorepeat = 0;
		}
		if (use_solid_bg) {
			if (verbose) rfbLog("  rawfb: turning off "
			    "use_solid_bg\n");
			use_solid_bg = 0;
		}
#ifndef MACOSX
		if (raw_fb_str && strstr(raw_fb_str, "vnc") == raw_fb_str) {
			;
		} else {
			multiple_cursors_mode = strdup("arrow");
		}
#endif
	}
	if (using_shm) {
		if (verbose) rfbLog("  rawfb: turning off using_shm\n");
		using_shm = 0;
	}
	if (take_naps) {
		if (verbose) rfbLog("  rawfb: turning off take_naps\n");
		take_naps = 0;
	}
	if (xrandr) {
		if (verbose) rfbLog("  rawfb: turning off xrandr\n");
		xrandr = 0;
	}
	if (xrandr_maybe) {
		if (verbose) rfbLog("  rawfb: turning off xrandr_maybe\n");
		xrandr_maybe = 0;
	}
	if (! noxrecord) {
		if (verbose) rfbLog("  rawfb: turning off xrecord\n");
		noxrecord = 1;
	}
}

/*
 * Presumably under -nofb the clients will never request the framebuffer.
 * However, we have gotten such a request... so let's just give them
 * the current view on the display.  n.b. x2vnc and perhaps win2vnc
 * requests a 1x1 pixel for some workaround so sadly this evidently
 * nearly always happens.
 */
static void nofb_hook(rfbClientPtr cl) {
	XImage *fb;
	XImage raw;

	rfbLog("framebuffer requested in -nofb mode by client %s\n", cl->host);
	/* ignore xrandr */

	if (raw_fb && ! dpy) {
		fb = &raw;
		fb->data = (char *)malloc(32);
	} else {
		int use_real_ximage = 0;
		if (use_real_ximage) {
			fb = XGetImage_wr(dpy, window, 0, 0, dpy_x, dpy_y, AllPlanes, ZPixmap);
		} else {
			fb = &raw;
			fb->data = (char *) calloc(dpy_x*dpy_y*bpp/8, 1);
		}
	}
	main_fb = fb->data;
	rfb_fb = main_fb;
	/* mutex */
	screen->frameBuffer = rfb_fb;
	screen->displayHook = NULL;
}

void free_old_fb(void) {
	char *fbs[16];
	int i, j, nfb = 0, db = 0; 

	fbs[nfb++] = main_fb;		main_fb = NULL;
	fbs[nfb++] = rfb_fb;		rfb_fb = NULL;
	fbs[nfb++] = cmap8to24_fb;	cmap8to24_fb = NULL;
	fbs[nfb++] = snap_fb;		snap_fb = NULL;
	fbs[nfb++] = rot_fb;		rot_fb = NULL;
	fbs[nfb++] = raw_fb;		raw_fb = NULL;

	for (i=0; i < nfb; i++) {
		char *fb = fbs[i];
		int freeit = 1;
		if (! fb || fb < (char *) 0x10) {
			continue;
		}
		for (j=0; j < i; j++) {
			if (fb == fbs[j]) {
				freeit = 0;
				break;
			}
		}
		if (freeit) {
			if (db) fprintf(stderr, "free: %i %p\n", i, fb);
			free(fb);
		} else {
			if (db) fprintf(stderr, "skip: %i %p\n", i, fb);
		}
	}
}

static char _lcs_tmp[128];
static int _bytes0_size = 128, _bytes0[128];

static char *lcs(rfbClientPtr cl) {
	sprintf(_lcs_tmp, "%d/%d/%d/%d/%d-%d/%d/%d",
		!!(cl->newFBSizePending),
		!!(cl->cursorWasChanged),
		!!(cl->cursorWasMoved),
		!!(cl->reverseConnection),
		cl->state,
		cl->modifiedRegion  ? !!(sraRgnEmpty(cl->modifiedRegion))  : 2,
		cl->requestedRegion ? !!(sraRgnEmpty(cl->requestedRegion)) : 2,
		cl->copyRegion      ? !!(sraRgnEmpty(cl->copyRegion))      : 2
	);
	return _lcs_tmp;
}

static int lock_client_sends(int lock) {
	static rfbClientPtr *cls = NULL;
	static int cls_len = 0;
	static int blocked = 0;
	static int state = 0;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	char *s;

	if (!use_threads || !screen) {
		return 0;
	}
	if (lock < 0) {
		return state;
	}
	state = lock;

	if (lock) {
		if (cls_len < client_count + 128) {
			if (cls != NULL) {
				free(cls);
			}
			cls_len = client_count + 256;
			cls = (rfbClientPtr *) calloc(cls_len * sizeof(rfbClientPtr), 1);
		}
		
		iter = rfbGetClientIterator(screen);
		blocked = 0;
		while ((cl = rfbClientIteratorNext(iter)) != NULL) {
			s = lcs(cl);
			SEND_LOCK(cl);
			rfbLog("locked client:   %p  %.6f %s\n", cl, dnowx(), s);
			cls[blocked++] = cl;
		}
		rfbReleaseClientIterator(iter);
	} else {
		int i;
		for (i=0; i < blocked; i++) {
			cl = cls[i];
			if (cl != NULL) {
				s = lcs(cl);
				SEND_UNLOCK(cl)
				rfbLog("unlocked client: %p  %.6f %s\n", cl, dnowx(), s);
			}
			cls[i] = NULL;
		}
		blocked = 0;
	}
	return state;
}

static void settle_clients(int init) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int fb_pend, i, ms = 1000;
	char *s;

	if (!use_threads || !screen) {
		return;
	}

	if (init) {
		iter = rfbGetClientIterator(screen);
		i = 0;
		while ((cl = rfbClientIteratorNext(iter)) != NULL) {
			if (i < _bytes0_size) {
				_bytes0[i] = rfbStatGetSentBytesIfRaw(cl);
			}
			i++;
		}
		rfbReleaseClientIterator(iter);

		if (getenv("X11VNC_THREADS_NEW_FB_SLEEP")) {
			ms = atoi(getenv("X11VNC_THREADS_NEW_FB_SLEEP"));
		} else if (subwin) {
			ms = 250;
		} else {
			ms = 500;
		}
		usleep(ms * 1000);
		return;
	}

	if (getenv("X11VNC_THREADS_NEW_FB_SLEEP")) {
		ms = atoi(getenv("X11VNC_THREADS_NEW_FB_SLEEP"));
	} else if (subwin) {
		ms = 500;
	} else {
		ms = 1000;
	}
	usleep(ms * 1000);

	for (i=0; i < 5; i++) {
		fb_pend = 0;
		iter = rfbGetClientIterator(screen);
		while ((cl = rfbClientIteratorNext(iter)) != NULL) {
			s = lcs(cl);
			if (cl->newFBSizePending) {
				fb_pend++;
				rfbLog("pending fb size: %p  %.6f %s\n", cl, dnowx(), s);
			}
		}
		rfbReleaseClientIterator(iter);
		if (fb_pend > 0) {
			rfbLog("do_new_fb: newFBSizePending extra -threads sleep (%d)\n", i+1); 
			usleep(ms * 1000);
		} else {
			break;
		}
	}
	for (i=0; i < 5; i++) {
		int stuck = 0, tot = 0, j = 0;
		iter = rfbGetClientIterator(screen);
		while ((cl = rfbClientIteratorNext(iter)) != NULL) {
			if (j < _bytes0_size) {
				int db = rfbStatGetSentBytesIfRaw(cl) - _bytes0[j];
				int Bpp = cl->format.bitsPerPixel / 8;

				s = lcs(cl);
				rfbLog("addl bytes sent: %p  %.6f %s  %d  %d\n",
				    cl, dnowx(), s, db, _bytes0[j]);

				if (i==0) {
					if (db < Bpp * dpy_x * dpy_y) {
						stuck++;
					}
				} else if (i==1) {
					if (db < 0.5 * Bpp * dpy_x * dpy_y) {
						stuck++;
					}
				} else {
					if (db <= 0) {
						stuck++;
					}
				}
			}
			tot++;
			j++;
		}
		rfbReleaseClientIterator(iter);
		if (stuck > 0) {
			rfbLog("clients stuck:  %d/%d  sleep(%d)\n", stuck, tot, i);
			usleep(2 * ms * 1000);
		} else {
			break;
		}
	}
}

static void prep_clients_for_new_fb(void) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (!use_threads || !screen) {
		return;
	}
	iter = rfbGetClientIterator(screen);
	while ((cl = rfbClientIteratorNext(iter)) != NULL) {
		if (!cl->newFBSizePending) {
			rfbLog("** set_new_fb_size_pending client:   %p\n", cl);
			cl->newFBSizePending = TRUE;
		}
		cl->cursorWasChanged = FALSE;
		cl->cursorWasMoved = FALSE;
	}
	rfbReleaseClientIterator(iter);
}

void do_new_fb(int reset_mem) {
	XImage *fb;

	/* for threaded we really should lock libvncserver out. */
	if (use_threads) {
		int ms = 1000;
		if (getenv("X11VNC_THREADS_NEW_FB_SLEEP")) {
			ms = atoi(getenv("X11VNC_THREADS_NEW_FB_SLEEP"));
		} else if (subwin) {
			ms = 500;
		} else {
			ms = 1000;
		}
		rfbLog("Warning: changing framebuffers in threaded mode may be unstable.\n");
		threads_drop_input = 1;
		usleep(ms * 1000);
	}

	INPUT_LOCK;
	lock_client_sends(1);

	if (use_threads) {
		settle_clients(1);
	}

#ifdef MACOSX
	if (macosx_console) {
		macosxCG_fini();
	}
#endif
	if (reset_mem == 1) {
		/* reset_mem == 2 is a hack for changing users... */
		clean_shm(0);
		free_tiles();
	}

	free_old_fb();

	fb = initialize_xdisplay_fb();

	initialize_screen(NULL, NULL, fb);

	if (reset_mem) {
		initialize_tiles();
		initialize_blackouts_and_xinerama();
		initialize_polling_images();
	}
	if (ncache) {
		check_ncache(1, 0);
	}

	prep_clients_for_new_fb();
	lock_client_sends(0);
	INPUT_UNLOCK;

	if (use_threads) {
		/* need to let things settle... */
		settle_clients(0);
		threads_drop_input = 0;
	}
}

static void remove_fake_fb(void) {
	if (! screen) {
		return;
	}
	rfbLog("removing fake fb: 0x%x\n", fake_fb);

	do_new_fb(1);

	/*
	 * fake_fb is freed in do_new_fb(), but we set to NULL here to
	 * indicate it is gone.
	 */
	fake_fb = NULL;
}

static void rfb_new_framebuffer(rfbScreenInfoPtr rfbScreen, char *framebuffer,
    int width,int height, int bitsPerSample,int samplesPerPixel,
    int bytesPerPixel) {

	rfbNewFramebuffer(rfbScreen, framebuffer, width, height, bitsPerSample,
	    samplesPerPixel, bytesPerPixel);

}

static void install_fake_fb(int w, int h, int bpp) {
	int bpc;
	if (! screen) {
		return;
	}
	lock_client_sends(1);
	if (fake_fb) {
		free(fake_fb);
	}
	fake_fb = (char *) calloc(w*h*bpp/8, 1);
	if (! fake_fb) {
		rfbLog("could not create fake fb: %dx%d %d\n", w, h, bpp);
		lock_client_sends(0);
		return;
	}
	bpc = guess_bits_per_color(bpp);
	rfbLog("installing fake fb: %dx%d %d\n", w, h, bpp);
	rfbLog("rfbNewFramebuffer(0x%x, 0x%x, %d, %d, %d, %d, %d)\n",
	    screen, fake_fb, w, h, bpc, 1, bpp/8);

	rfb_new_framebuffer(screen, fake_fb, w, h, bpc, 1, bpp/8);
	lock_client_sends(0);
}

void check_padded_fb(void) {
	if (! fake_fb) {
		return;
	}
	if (unixpw_in_progress) return;

	if (time(NULL) > pad_geometry_time+1 && all_clients_initialized()) {
		remove_fake_fb();
	}
}

void install_padded_fb(char *geom) {
	int w, h;
	int ok = 1;
	if (! geom || *geom == '\0') {
		ok = 0;
	} else if (sscanf(geom, "%dx%d", &w, &h) != 2)  {
		ok = 0;
	}
	w = nabs(w);
	h = nabs(h);

	if (w < 5) w = 5;
	if (h < 5) h = 5;

	if (!ok) {
		rfbLog("skipping invalid pad geometry: '%s'\n", NONUL(geom));
		return;
	}
	install_fake_fb(w, h, bpp);
	pad_geometry_time = time(NULL);
}

static void initialize_snap_fb(void) {
	RAWFB_RET_VOID
	if (snap_fb) {
		free(snap_fb);
	}
	snap = XGetImage_wr(dpy, window, 0, 0, dpy_x, dpy_y, AllPlanes,
	    ZPixmap);
	snap_fb = snap->data;
}

static rfbClient* client = NULL;

void vnc_reflect_bell(rfbClient *cl) {
	if (cl) {}
	if (sound_bell) {
		if (unixpw_in_progress) {
			return;
		}
		if (! all_clients_initialized()) {
			rfbLog("vnc_reflect_bell: not sending bell: "
			    "uninitialized clients\n");
		} else {
			if (screen && client_count) {
				rfbSendBell(screen);
			}
		}
	}
}

void vnc_reflect_recv_cuttext(rfbClient *cl, const char *str, int len) {
	if (cl) {}
	if (unixpw_in_progress) {
		return;
	}
	if (! watch_selection) {
		return;
	}
	if (! all_clients_initialized()) {
		rfbLog("vnc_reflect_recv_cuttext: no send: uninitialized clients\n");
		return; /* some clients initializing, cannot send */ 
	}
	rfbSendServerCutText(screen, (char *)str, len);
}

void vnc_reflect_got_update(rfbClient *cl, int x, int y, int w, int h) {
	if (cl) {}
	if (use_xdamage) {
		static int first = 1;
		if (first) {
			collect_non_X_xdamage(-1, -1, -1, -1, 0);
			first = 0;
		}
		collect_non_X_xdamage(x, y, w, h, 1);
	}
}

void vnc_reflect_got_cursorshape(rfbClient *cl, int xhot, int yhot, int width, int height, int bytesPerPixel) {
	static int serial = 1;
	int i, j;
	char *pixels = NULL;
	unsigned long r, g, b;
	unsigned int ui = 0;
	unsigned long red_mask, green_mask, blue_mask;

	if (cl) {}
	if (unixpw_in_progress) {
		return;
	}
	if (! all_clients_initialized()) {
		rfbLog("vnc_reflect_got_copyshape: no send: uninitialized clients\n");
		return; /* some clients initializing, cannot send */ 
	}
	if (! client->rcSource) {
		return;
	}
	if (bytesPerPixel != 1 && bytesPerPixel != 2 && bytesPerPixel != 4) {
		return;
	}

	red_mask   = (client->format.redMax   << client->format.redShift);
	green_mask = (client->format.greenMax << client->format.greenShift);
	blue_mask  = (client->format.blueMax  << client->format.blueShift);

	pixels = (char *)malloc(4*width*height);
	for (j=0; j<height; j++) {
		for (i=0; i<width; i++) {
			unsigned int* uip;
			unsigned char* uic;
			int m;
			if (bytesPerPixel == 1) {
				unsigned char* p = (unsigned char *) client->rcSource;
				ui = (unsigned long) *(p + j * width + i);
			} else if (bytesPerPixel == 2) {
				unsigned short* p = (unsigned short *) client->rcSource;
				ui = (unsigned long) *(p + j * width + i);
			} else if (bytesPerPixel == 4) {
				unsigned int* p = (unsigned int *) client->rcSource;
				ui = (unsigned long) *(p + j * width + i);
			}
			r = (red_mask   & ui) >> client->format.redShift;
			g = (green_mask & ui) >> client->format.greenShift;
			b = (blue_mask  & ui) >> client->format.blueShift;

			r = (255 * r) / client->format.redMax;
			g = (255 * g) / client->format.greenMax;
			b = (255 * b) / client->format.blueMax;

			ui = (r << 16 | g << 8 | b << 0) ;

			uic = (unsigned char *)client->rcMask;
			m = (int) *(uic + j * width + i);
			if (m) {
				ui |= 0xff000000;
			}
			uip = (unsigned int *)pixels;
			*(uip + j * width + i) = ui;
		}
	}

	store_cursor(serial++, (unsigned long*) pixels, width, height, 32, xhot, yhot);
	free(pixels);
	set_cursor(cursor_x, cursor_y, get_which_cursor());
}

rfbBool vnc_reflect_cursor_pos(rfbClient *cl, int x, int y) {
	if (cl) {}
	if (debug_pointer) {
		rfbLog("vnc_reflect_cursor_pos: %d %d\n", x, y);
	}
	if (unixpw_in_progress) {
		if (debug_pointer) {
			rfbLog("vnc_reflect_cursor_pos: unixpw_in_progress%d\n", unixpw_in_progress);
		}
		return TRUE;
	}
	if (! all_clients_initialized()) {
		rfbLog("vnc_reflect_cursor_pos: no send: uninitialized clients\n");
		return TRUE; /* some clients initializing, cannot send */ 
	}

	cursor_position(x, y);
	set_cursor(x, y, get_which_cursor());

	return TRUE;
}

static void from_libvncclient_CopyRectangleFromRectangle(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {
  int i,j;

#define COPY_RECT_FROM_RECT(BPP) \
  { \
    uint##BPP##_t* _buffer=((uint##BPP##_t*)client->frameBuffer)+(src_y-dest_y)*client->width+src_x-dest_x; \
    if (dest_y < src_y) { \
      for(j = dest_y*client->width; j < (dest_y+h)*client->width; j += client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } else { \
      for(j = (dest_y+h-1)*client->width; j >= dest_y*client->width; j-=client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } \
  }

  switch(client->format.bitsPerPixel) {
  case  8: COPY_RECT_FROM_RECT(8);  break;
  case 16: COPY_RECT_FROM_RECT(16); break;
  case 32: COPY_RECT_FROM_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}

void vnc_reflect_got_copyrect(rfbClient *cl, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {
	sraRegionPtr reg;
	int dx, dy, rc = -1;
	static int last_dx = 0, last_dy = 0;
	if (cl) {}
	if (unixpw_in_progress) {
		return;
	}
	if (! all_clients_initialized()) {
		rfbLog("vnc_reflect_got_copyrect: no send: uninitialized clients\n");
		return; /* some clients initializing, cannot send */ 
	}
	dx = dest_x - src_x;
	dy = dest_y - src_y;
	if (dx != last_dx || dy != last_dy) {
		rc = fb_push_wait(0.05, FB_COPY|FB_MOD);
	}
	if (0) fprintf(stderr, "vnc_reflect_got_copyrect: %03dx%03d+%03d+%03d   %3d %3d  rc=%d\n", dest_x, dest_y, w, h, dx, dy, rc);
	reg = sraRgnCreateRect(dest_x, dest_y, dest_x + w, dest_y + h);
	do_copyregion(reg, dx, dy, 0);
	sraRgnDestroy(reg);

	last_dx = dx;
	last_dy = dy;

	from_libvncclient_CopyRectangleFromRectangle(cl, src_x, src_y, w, h, dest_x, dest_y);
}

rfbBool vnc_reflect_resize(rfbClient *cl)  {
	static int first = 1;
	if(cl->frameBuffer) {
		free(cl->frameBuffer);
	}
	cl->frameBuffer= malloc(cl->width * cl->height * cl->format.bitsPerPixel/8);
	rfbLog("vnc_reflect_resize: %dx%dx%d first=%d\n", cl->width, cl->height,
	    cl->format.bitsPerPixel, first);
	if (!first) {
		do_new_fb(1);
	}
	first = 0;
	return cl->frameBuffer ? TRUE : FALSE;
}

#ifdef rfbCredentialTypeX509
static rfbCredential* vnc_reflect_get_credential(rfbClient* client, int type) {
	char *pass = getenv("X11VNC_REFLECT_PASSWORD");
	char *user = getenv("X11VNC_REFLECT_USER");
	char *cert = getenv("X11VNC_REFLECT_CACERT");
	char *ccrl = getenv("X11VNC_REFLECT_CACRL");
	char *clic = getenv("X11VNC_REFLECT_CLIENTCERT");
	char *clik = getenv("X11VNC_REFLECT_CLIENTKEY");
	int db = 0;
	if (client) {}
if (db) fprintf(stderr, "type: %d\n", type);
#ifdef rfbCredentialTypeUser
	if (type == rfbCredentialTypeUser) {
		if (!pass && !user) {
			return NULL;
		} else {
			rfbCredential *rc = (rfbCredential *) calloc(sizeof(rfbCredential), 1);
			rc->userCredential.username = (user ? strdup(user) : NULL);
			rc->userCredential.password = (pass ? strdup(pass) : NULL);
			return rc;
		}
	}
#endif
	if (type == rfbCredentialTypeX509) {
if (db) fprintf(stderr, "cert: %s\n", cert);
if (db) fprintf(stderr, "ccrl: %s\n", ccrl);
if (db) fprintf(stderr, "clic: %s\n", clic);
if (db) fprintf(stderr, "clik: %s\n", clik);
		if (!cert && !ccrl && !clic && !clik) {
			return NULL;
		} else {
			rfbCredential *rc = (rfbCredential *) calloc(sizeof(rfbCredential), 1);
			rc->x509Credential.x509CACertFile     = (cert ? strdup(cert) : NULL);
			rc->x509Credential.x509CACrlFile      = (ccrl ? strdup(ccrl) : NULL);
			rc->x509Credential.x509ClientCertFile = (clic ? strdup(clic) : NULL);
			rc->x509Credential.x509ClientKeyFile  = (clik ? strdup(clik) : NULL);
			return rc;
		}
	}
	return NULL;
}
#endif

static char* vnc_reflect_get_password(rfbClient* client) {
	char *q, *p, *str = getenv("X11VNC_REFLECT_PASSWORD");
	int len = 110;

	if (client) {}

	if (str) {
		len += 2*strlen(str);	
	}
	p = (char *) calloc(len, 1);
	if (!str || strlen(str) == 0) {
		fprintf(stderr, "VNC Reflect Password: ");
		fgets(p, 100, stdin);
	} else {
		if (strstr(str, "file:") == str) {
			FILE *f = fopen(str + strlen("file:"), "r");
			if (f) {
				fgets(p, 100, f);
				fclose(f);
			}
		}
		if (p[0] == '\0') {
			strncpy(p, str, 100);
		}
	}
	q = p;
	while (*q != '\0') {
		if (*q == '\n') {
			*q = '\0';
		}
		q++;
	}
	return p;
}

char *vnc_reflect_guess(char *str, char **raw_fb_addr) {

	static int first = 1;
	char *hp = str + strlen("vnc:");
	char *at = NULL;
	int argc = 0, i;
	char *argv[16];
	char str2[256];
	char *str0 = strdup(str);

	if (client == NULL) {
		int bitsPerSample = 8;
		int samplesPerPixel = 3;
		int bytesPerPixel = 4;
		char *s;
		s = getenv("X11VNC_REFLECT_bitsPerSample");
		if (s) bitsPerSample = atoi(s);
		s = getenv("X11VNC_REFLECT_samplesPerPixel");
		if (s) samplesPerPixel = atoi(s);
		s = getenv("X11VNC_REFLECT_bytesPerPixel");
		if (s) bytesPerPixel = atoi(s);
		rfbLog("rfbGetClient(bitsPerSample=%d, samplesPerPixel=%d, bytesPerPixel=%d)\n",
		    bitsPerSample, samplesPerPixel, bytesPerPixel);
		client = rfbGetClient(bitsPerSample, samplesPerPixel, bytesPerPixel);
	}

	rfbLog("rawfb: %s\n", str);

	at = strchr(hp, '@');
	if (at) {
		*at = '\0';
		at++;
	}

	/* Tobias Doerffel, 2010/10 */
#define USE_AS_ITALC_DEMO_SERVER
#ifdef USE_AS_ITALC_DEMO_SERVER
	client->appData.encodingsString = "raw";
#endif
	client->appData.useRemoteCursor = TRUE;
	client->canHandleNewFBSize = TRUE;

	client->HandleCursorPos = vnc_reflect_cursor_pos;
	client->GotFrameBufferUpdate = vnc_reflect_got_update;
	client->MallocFrameBuffer = vnc_reflect_resize;
	client->Bell = vnc_reflect_bell;
#if 0
	client->SoftCursorLockArea = NULL;
	client->SoftCursorUnlockScreen = NULL;
	client->FinishedFrameBufferUpdate = NULL;
	client->HandleKeyboardLedState = NULL;
	client->HandleTextChat = NULL;
#endif
	client->GotXCutText = vnc_reflect_recv_cuttext;
	client->GotCursorShape = vnc_reflect_got_cursorshape;
	client->GotCopyRect = vnc_reflect_got_copyrect;

	if (getenv("X11VNC_REFLECT_PASSWORD")) {
		client->GetPassword = vnc_reflect_get_password;
	}
#ifdef rfbCredentialTypeX509
	client->GetCredential = NULL;
	if (0 || getenv("LIBVNCCLIENT_GET_CREDENTIAL")) {
		client->GetCredential = vnc_reflect_get_credential;
	}
#endif

	if (first) {
		argv[argc++] = "x11vnc_rawfb_vnc";
		if (strstr(hp, "listen") == hp) {
			char *q = strrchr(hp, ':');
			argv[argc++] = strdup("-listen");
			if (q) {
				client->listenPort = atoi(q+1);
			} else {
				client->listenPort = LISTEN_PORT_OFFSET;
			}
		} else {
			argv[argc++] = strdup(hp);
		}

		if (! rfbInitClient(client, &argc, argv)) {
			rfbLog("vnc_reflector failed for: %s\n", str0);
			clean_up_exit(1);
		}
	}

	if (at) {
		sprintf(str2, "map:/dev/null@%s", at);
	} else {
		unsigned long red_mask, green_mask, blue_mask;
		red_mask   = (client->format.redMax   << client->format.redShift);
		green_mask = (client->format.greenMax << client->format.greenShift);
		blue_mask  = (client->format.blueMax  << client->format.blueShift);
		sprintf(str2, "map:/dev/null@%dx%dx%d:0x%lx/0x%lx/0x%lx",
		    client->width, client->height, client->format.bitsPerPixel,
		    red_mask, green_mask, blue_mask);
	}
	*raw_fb_addr = (char *) client->frameBuffer;
	free(str0);

	if (first) {
		setup_cursors_and_push();

		for (i=0; i<10; i++) {
			vnc_reflect_process_client();
		}
	}
	first = 0;

	return strdup(str2);
}

rfbBool vnc_reflect_send_pointer(int x, int y, int mask) {
	int rc;
	if (mask >= 0) {
		got_user_input++;
		got_pointer_input++;
		last_pointer_time = time(NULL);
	}

	if (clipshift) {
		x += coff_x;
		y += coff_y;
	}

	if (cursor_x != x || cursor_y != y) {
		last_pointer_motion_time = dnow();
	}

	cursor_x = x;
	cursor_y = y;

	/* record the x, y position for the rfb screen as well. */
	cursor_position(x, y);

	/* change the cursor shape if necessary */
	rc = set_cursor(x, y, get_which_cursor());
	cursor_changes += rc;

	return SendPointerEvent(client, x, y, mask);
}

rfbBool vnc_reflect_send_key(uint32_t key, rfbBool down) {
	return SendKeyEvent(client, key, down);
}

rfbBool vnc_reflect_send_cuttext(char *str, int len) {
	return SendClientCutText(client, str, len);
}

void vnc_reflect_process_client(void) {
	int num;
	if (client == NULL) {
		return;
	}
	num = WaitForMessage(client, 1000);
	if (num > 0) {
		if (!HandleRFBServerMessage(client)) {
			rfbLog("vnc_reflect_process_client: read failure to server\n");
			shut_down = 1;
		}
	}
}

void linux_dev_fb_msg(char* q) {
#ifndef WIN32
	if (strstr(q, "/dev/fb") && strstr(UT.sysname, "Linux")) {
		rfbLog("\n");
		rfbLog("On Linux you may need to load a kernel module to enable\n");
		rfbLog("the framebuffer device /dev/fb*; e.g.:\n");
		rfbLog("   vga=0x303 (and others) kernel boot parameter\n");
		rfbLog("   modprobe uvesafb\n");
		rfbLog("   modprobe radeonfb (card specific)\n");
		rfbLog("   modprobe nvidiafb (card specific, others)\n");
		rfbLog("   modprobe vesafb (?)\n");
		rfbLog("   modprobe vga16fb\n");
		rfbLog("\n");
		rfbLog("You may also need root permission to open /dev/fb*\n");
		rfbLog("and/or /dev/tty*.\n");
		rfbLog("\n");
	}
#endif
}

#define RAWFB_MMAP 1
#define RAWFB_FILE 2
#define RAWFB_SHM  3

XImage *initialize_raw_fb(int reset) {
	char *str, *rstr, *q;
	int w, h, b, shmid = 0;
	unsigned long rm = 0, gm = 0, bm = 0, tm;
	static XImage ximage_struct;	/* n.b.: not (XImage *) */
	static XImage ximage_struct_snap;
	int closedpy = 1, i, m, db = 0;
	int do_macosx = 0;
	int do_reflect = 0;
	char *unlink_me = NULL;

	static char *last_file = NULL;
	static int last_mode = 0;

	if (reset && last_mode) {
		int fd;
		if (last_mode != RAWFB_MMAP && last_mode != RAWFB_FILE) {
			return NULL;
		}
#if LIBVNCSERVER_HAVE_MMAP
		if (last_mode == RAWFB_MMAP) {
			munmap(raw_fb_addr, raw_fb_mmap);
		}
#endif
		if (raw_fb_fd >= 0) {
			close(raw_fb_fd);
		}
		raw_fb_fd = -1;
if (db) fprintf(stderr, "initialize_raw_fb reset\n");
			
		fd = -1;
		if (rawfb_dev_video) {
			fd = open(last_file, O_RDWR);
		}
		if (fd < 0) {
			fd = open(last_file, O_RDONLY);
		}
		if (fd < 0) {
			rfbLogEnable(1);
			rfbLog("failed to rawfb file: %s\n", last_file);
			rfbLogPerror("open");
			clean_up_exit(1);
		}
		raw_fb_fd = fd;
#if LIBVNCSERVER_HAVE_MMAP
		if (last_mode == RAWFB_MMAP) {
			raw_fb_addr = mmap(0, raw_fb_mmap, PROT_READ,
			    MAP_SHARED, fd, 0);

			if (raw_fb_addr == MAP_FAILED || raw_fb_addr == NULL) {
				rfbLogEnable(1);
				rfbLog("failed to mmap file: %s\n", last_file);
				rfbLog("   raw_fb_addr: %p\n", raw_fb_addr);
				rfbLogPerror("mmap");
				clean_up_exit(1);
			}
		}
#endif
		return NULL;
	}

#ifdef MACOSX
	if (raw_fb_addr != NULL && macosx_console && raw_fb_addr == macosx_get_fb_addr()) {
		raw_fb_addr = NULL;
	}
#endif

	if (raw_fb_addr || raw_fb_seek) {
		if (raw_fb_shm) {
#if LIBVNCSERVER_HAVE_XSHM || LIBVNCSERVER_HAVE_SHMAT
			shmdt(raw_fb_addr);
#endif
#if LIBVNCSERVER_HAVE_MMAP
		} else if (raw_fb_mmap) {
			munmap(raw_fb_addr, raw_fb_mmap);
			if (raw_fb_fd >= 0) {
				close(raw_fb_fd);
			}
			raw_fb_fd = -1;
#endif
		} else if (raw_fb_seek) {
			if (raw_fb_fd >= 0) {
				close(raw_fb_fd);
			}
			raw_fb_fd = -1;
		}
		raw_fb_addr = NULL;
		raw_fb_mmap = 0;
		raw_fb_seek = 0;
	}
	if (! raw_fb_str) {
		return NULL;
	}

	if (raw_fb_str[0] == '+') {
		rstr = strdup(raw_fb_str+1);
		closedpy = 0;
		if (! window) {
			window = rootwin;
		}
	} else {
		rstr = strdup(raw_fb_str);
	}

	/* testing aliases */
	if (!strcasecmp(rstr, "NULL") || !strcasecmp(rstr, "ZERO")
	    || !strcasecmp(rstr, "NONE")) {
		rstr = strdup("map:/dev/zero@640x480x32");
	} else if (!strcasecmp(rstr, "NULLBIG") || !strcasecmp(rstr, "NONEBIG")) {
		rstr = strdup("map:/dev/zero@1024x768x32");
	}
	if (!strcasecmp(rstr, "RAND")) {
		rstr = strdup("file:/dev/urandom@128x128x16");
	} else if (!strcasecmp(rstr, "RANDBIG")) {
		rstr = strdup("file:/dev/urandom@640x480x16");
	} else if (!strcasecmp(rstr, "RANDHUGE")) {
		rstr = strdup("file:/dev/urandom@1024x768x16");
	}
	if (strstr(rstr, "solid=") == rstr) {
		char *n = rstr + strlen("solid=");
		char tmp[] = "/tmp/rawfb_solid.XXXXXX";
		char str[100];
		unsigned int vals[1024], val;
		int x, y, fd, w = 1024, h = 768;
		if (strstr(n, "0x")) {
			if (sscanf(n, "0x%x", &val) != 1) {
				val = 0;
			}
		}
		if (val == 0) {
			val = get_pixel(n);
		}
		if (val == 0) {
			val = 0xFF00FF;
		}
		fd = mkstemp(tmp);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				vals[x] = val;
			}
			write(fd, (char *)vals, 4 * w);
		}
		close(fd);
		fd = open(tmp, O_WRONLY);
		unlink_me = strdup(tmp);
		sprintf(str, "map:%s@%dx%dx32", tmp, w, h);
		rstr = strdup(str);
	} else if (strstr(rstr, "swirl") == rstr) {
		char tmp[] = "/tmp/rawfb_swirl.XXXXXX";
		char str[100];
		unsigned int val[1024];
		unsigned int c1, c2, c3, c4;
		int x, y, fd, w = 1024, h = 768;
		fd = mkstemp(tmp);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				c1 = 0;
				c2 = ((x+y)*128)/(w+h);
				c3 = (x*128)/w;
				c4 = (y*256)/h;
				val[x] = (c1 << 24) | (c2 << 16) | (c3 << 8) | (c4 << 0);
			}
			write(fd, (char *)val, 4 * w);
		}
		close(fd);
		fd = open(tmp, O_WRONLY);
		unlink_me = strdup(tmp);
		sprintf(str, "map:%s@%dx%dx32", tmp, w, h);
		rstr = strdup(str);
	}


	if ( (q = strstr(rstr, "setup:")) == rstr) {
		FILE *pipe;
		char line[1024], *t;

		set_child_info();
		q += strlen("setup:");
		/* rawfb-setup */
		if (no_external_cmds || !cmd_ok("rawfb-setup")) {
			rfbLogEnable(1);
			rfbLog("cannot run external commands in -nocmds "
			    "mode:\n");
			rfbLog("   \"%s\"\n", q);
			rfbLog("   exiting.\n");
			clean_up_exit(1);
		}
		rfbLog("running command to setup rawfb: %s\n", q);
		close_exec_fds();
		pipe = popen(q, "r");
		if (! pipe) {
			rfbLogEnable(1);
			rfbLog("popen of setup command failed.\n");
			rfbLogPerror("popen");
			clean_up_exit(1);
		}
		line[0] = '\0';
		if (fgets(line, 1024, pipe) == NULL) {
			rfbLogEnable(1);
			rfbLog("read of setup command failed.\n");
			clean_up_exit(1);
		}
		pclose(pipe);
		str = strdup(line);
		t = str;
		while (*t != '\0') {
			if (*t == '\n') {
				*t = '\0';
			}
			t++;
		}
		rfbLog("setup command returned: %s\n", str);

	} else {
		str = strdup(rstr);
	}

	raw_fb_shm = 0;
	raw_fb_mmap = 0;
	raw_fb_seek = 0;
	raw_fb_fd = -1;
	raw_fb_addr = NULL;
	raw_fb_offset = 0;
	raw_fb_bytes_per_line = 0;
	rawfb_vnc_reflect = 0;

	last_mode = 0;
	if (last_file) {
		free(last_file);
		last_file = NULL;
	}
	if (strstr(str, "Video") == str) {
		if (pipeinput_str != NULL) {
			free(pipeinput_str);
		}
		pipeinput_str = strdup("VID");
		initialize_pipeinput();
		str[0] = 'v';
	}

	if (strstr(str, "video") == str || strstr(str, "/dev/video") == str) {
		char *str2 = v4l_guess(str, &raw_fb_fd);
		if (str2 == NULL) {
			rfbLog("v4l_guess failed for: %s\n", str);
			clean_up_exit(1);
		}
		str = str2;
		rfbLog("v4l_guess returned: %s\n", str);
		rawfb_dev_video = 1;
	} else if (strstr(str, "dev/video")) {
		rawfb_dev_video = 1;
	} else if (strstr(str, "console") == str || strstr(str, "fb") == str ||
	    strstr(str, "/dev/fb") == str || strstr(str, "vt") == str) {
		char *str2 = console_guess(str, &raw_fb_fd);
		if (str2 == NULL) {
			rfbLog("console_guess failed for: %s\n", str);
			clean_up_exit(1);
		}
		str = str2;
		rfbLog("console_guess returned: %s\n", str);
	} else if (strstr(str, "vnc:") == str) {
		char *str2 = vnc_reflect_guess(str, &raw_fb_addr);

		rawfb_vnc_reflect = 1;
		do_reflect = 1;

		str = str2;
		rfbLog("vnc_reflector set rawfb str to: %s\n", str);
		if (pipeinput_str == NULL) {
			pipeinput_str = strdup("VNC");
		}
		initialize_pipeinput();
	}

	if (closedpy && !view_only && got_noviewonly) {
		rfbLog("not closing X DISPLAY under -noviewonly option.\n");
		closedpy = 0;
		if (! window) {
			window = rootwin;
		}
	}

	if (! raw_fb_orig_dpy && dpy) {
		raw_fb_orig_dpy = strdup(DisplayString(dpy));
	}
#ifndef BOLDLY_CLOSE_DISPLAY
#define BOLDLY_CLOSE_DISPLAY 1
#endif
#if BOLDLY_CLOSE_DISPLAY
	if (closedpy) {
		if (dpy) {
			rfbLog("closing X DISPLAY: %s in rawfb mode.\n",
			    DisplayString(dpy));
			XCloseDisplay_wr(dpy);	/* yow! */
		}
		dpy = NULL;
	}
#endif

	/*
	 * -rawfb shm:163938442@640x480x32:ff/ff00/ff0000+3000
	 * -rawfb map:/path/to/file@640x480x32:ff/ff00/ff0000
	 * -rawfb file:/path/to/file@640x480x32:ff/ff00/ff0000
	 */

	if (raw_fb_full_str) {
		free(raw_fb_full_str);
	}
	raw_fb_full_str = strdup(str);


	/* +O offset */
	if ((q = strrchr(str, '+')) != NULL) {
		if (sscanf(q, "+%d", &raw_fb_offset) == 1) {
			*q = '\0';
		} else {
			raw_fb_offset = 0;
		}
	}
	/* :R/G/B masks */
	if ((q = strrchr(str, ':')) != NULL) {
		if (sscanf(q, ":%lx/%lx/%lx", &rm, &gm, &bm) == 3) {
			*q = '\0';
		} else if (sscanf(q, ":0x%lx/0x%lx/0x%lx", &rm, &gm, &bm)== 3) {
			*q = '\0';
		} else if (sscanf(q, ":%lu/%lu/%lu", &rm, &gm, &bm) == 3) {
			*q = '\0';
		} else {
			rm = 0;
			gm = 0;
			bm = 0;
		}
	}
	if ((q = strrchr(str, '@')) == NULL) {
		rfbLogEnable(1);
		rfbLog("invalid rawfb str: %s\n", str);
		clean_up_exit(1);
	}

	if (strrchr(q, '-')) {
		char *q2 = strrchr(q, '-');
		raw_fb_bytes_per_line = atoi(q2+1);
		*q2 = '\0';
	}
	/* @WxHxB */
	if (sscanf(q, "@%dx%dx%d", &w, &h, &b) != 3) {
		rfbLogEnable(1);
		rfbLog("invalid rawfb str: %s\n", str);
		clean_up_exit(1);
	}
	*q = '\0';

	if (rm == 0 && gm == 0 && bm == 0) {
		/* guess masks... */
		if (b == 24 || b == 32) {
			rm = 0xff0000;
			gm = 0x00ff00;
			bm = 0x0000ff;
		} else if (b == 16) {
			rm = 0xf800;
			gm = 0x07e0;
			bm = 0x001f;
		} else if (b == 8) {
			rm = 0x07;
			gm = 0x38;
			bm = 0xc0;
		}
	}
	/* we can fake -flipbyteorder to some degree... */
	if (flip_byte_order) {
		if (b == 24 || b == 32) {
			tm = rm;
			rm = bm;
			bm = tm;
		} else if (b == 16) {
			unsigned short s1, s2;
			s1 = (unsigned short) rm;
			s2 = ((0xff & s1) << 8) | ((0xff00 & s1) >> 8);
			rm = (unsigned long) s2;
			s1 = (unsigned short) gm;
			s2 = ((0xff & s1) << 8) | ((0xff00 & s1) >> 8);
			gm = (unsigned long) s2;
			s1 = (unsigned short) bm;
			s2 = ((0xff & s1) << 8) | ((0xff00 & s1) >> 8);
			bm = (unsigned long) s2;
		}
	}

	/* native fb stuff for bpp < 8 only */
	raw_fb_native_bpp = b;
	raw_fb_native_red_mask = rm;
	raw_fb_native_green_mask = gm;
	raw_fb_native_blue_mask = bm;
	raw_fb_native_red_shift = 100;
	raw_fb_native_green_shift = 100;
	raw_fb_native_blue_shift = 100;
	raw_fb_native_red_max = 1;
	raw_fb_native_green_max = 1;
	raw_fb_native_blue_max = 1;
	m = 1;
	for (i=0; i<32; i++)  {
		if (raw_fb_native_red_mask & m) {
			if (raw_fb_native_red_shift == 100) {
				raw_fb_native_red_shift = i;
			}
			raw_fb_native_red_max *= 2;
		}
		if (raw_fb_native_green_mask & m) {
			if (raw_fb_native_green_shift == 100) {
				raw_fb_native_green_shift = i;
			}
			raw_fb_native_green_max *= 2;
		}
		if (raw_fb_native_blue_mask & m) {
			if (raw_fb_native_blue_shift == 100) {
				raw_fb_native_blue_shift = i;
			}
			raw_fb_native_blue_max *= 2;
		}
		m = m << 1;
	}
	raw_fb_native_red_max -= 1;
	raw_fb_native_green_max -= 1;
	raw_fb_native_blue_max -= 1;

	if (b < 8) {
		/* e.g. VGA16 */
		rfbLog("raw_fb_native_bpp: %d 0x%02lx 0x%02lx 0x%02lx %d/%d/%d %d/%d/%d\n", raw_fb_native_bpp,
		    raw_fb_native_red_mask, raw_fb_native_green_mask, raw_fb_native_blue_mask,
		    raw_fb_native_red_max, raw_fb_native_green_max, raw_fb_native_blue_max,
		    raw_fb_native_red_shift, raw_fb_native_green_shift, raw_fb_native_blue_shift);
		raw_fb_expand_bytes = 1;
		b = 8;
		rm = 0x07;
		gm = 0x38;
		bm = 0xc0;
	}
	/* end of stuff for bpp < 8 */

	dpy_x = wdpy_x = w;
	dpy_y = wdpy_y = h;
	off_x = 0;
	off_y = 0;

	if (rawfb_dev_video) {
		if (b == 24) {
			rfbLog("enabling -24to32 for 24bpp video\n");
			xform24to32 = 1;
		} else {
			if (xform24to32) {
				rfbLog("disabling -24to32 for 24bpp video\n");
			}
			xform24to32 = 0;
		}
	}

	if (xform24to32) {
		if (b != 24) {
			rfbLog("warning: -24to32 mode and bpp=%d\n", b);
		}
		b = 32;
	}
	if (strstr(str, "snap:") == str) {
		use_snapfb = 1;
		str[0] = 'f'; str[1] = 'i'; str[2] = 'l'; str[3] = 'e';
	}

	if (strstr(str, "shm:") != str && strstr(str, "mmap:") != str &&
	    strstr(str, "map:") != str && strstr(str, "file:") != str) {
		/* hmmm, not following directions, see if map: applies */
		struct stat sbuf;
		if (stat(str, &sbuf) == 0) {
			char *newstr;
			int len = strlen("map:") + strlen(str) + 1;
			rfbLog("no type prefix: %s\n", raw_fb_str);
			rfbLog("  but file exists, so assuming: map:%s\n",
			    raw_fb_str);
			newstr = (char *) malloc(len);
			strcpy(newstr, "map:");
			strcat(newstr, str);
			free(str);
			str = newstr;
		}
	}

	if (sscanf(str, "shm:%d", &shmid) == 1) {
		/* shm:N */
#if LIBVNCSERVER_HAVE_XSHM || LIBVNCSERVER_HAVE_SHMAT
		raw_fb_addr = (char *) shmat(shmid, 0, SHM_RDONLY);
		if (! raw_fb_addr) {
			rfbLogEnable(1);
			rfbLog("failed to attach to shm: %d, %s\n", shmid, str);
			rfbLogPerror("shmat");
			clean_up_exit(1);
		}
		raw_fb_shm = 1;
		rfbLog("rawfb: shm: %d W: %d H: %d B: %d addr: %p\n",
		    shmid, w, h, b, raw_fb_addr);
		last_mode = RAWFB_SHM;
#else
		rfbLogEnable(1);
		rfbLog("x11vnc was compiled without shm support.\n");
		rfbLogPerror("shmat");
		clean_up_exit(1);
#endif
	} else if (strstr(str, "map:") == str || strstr(str, "mmap:") == str
	    || strstr(str, "file:") == str) {
		/* map:/path/... or file:/path  */
		int fd, do_mmap = 1, size;
		struct stat sbuf;

		if (*str == 'f') {
			do_mmap = 0;
		}
		q = strchr(str, ':');
		q++;

		macosx_console = 0;
		if (strstr(q, "macosx:") == q) {
			/* mmap:macosx:/dev/null@... */
			q += strlen("macosx:");			
			do_macosx = 1;
			do_mmap = 0;
			macosx_console = 1;
		}

		last_file = strdup(q);

#ifndef WIN32
		fd = raw_fb_fd;
		if (fd < 0 && rawfb_dev_video) {
			fd = open(q, O_RDWR);
		}
		if (fd < 0) {
			fd = open(q, O_RDONLY);
		}
		if (fd < 0) {
			rfbLogEnable(1);
			rfbLog("failed to open file: %s, %s\n", q, str);
			rfbLogPerror("open");
			linux_dev_fb_msg(q);
			clean_up_exit(1);
		}
		raw_fb_fd = fd;
#endif

		if (raw_fb_native_bpp < 8) {
			size = w*h*raw_fb_native_bpp/8 + raw_fb_offset;
		} else if (xform24to32) {
			size = w*h*24/8 + raw_fb_offset;
		} else {
			size = w*h*b/8 + raw_fb_offset;
		}
		if (fstat(fd, &sbuf) == 0) {
			if (S_ISREG(sbuf.st_mode)) {
				if (0) size = sbuf.st_size;
			} else {
				rfbLog("raw fb is non-regular file: %s\n", q);
			}
		}

		if (do_macosx) {
			raw_fb_addr = macosx_get_fb_addr();
			raw_fb_mmap = size;
			rfbLog("rawfb: macosx fb: %s\n", q);
			rfbLog("   w: %d h: %d b: %d addr: %p sz: %d\n", w, h,
			    b, raw_fb_addr, size);
			last_mode = 0;
		} else if (do_reflect) {
			raw_fb_mmap = size;
			rfbLog("rawfb: vnc fb: %s\n", q);
			rfbLog("   w: %d h: %d b: %d addr: %p sz: %d\n", w, h,
			    b, raw_fb_addr, size);
			last_mode = 0;

		} else if (do_mmap) {
#if LIBVNCSERVER_HAVE_MMAP
			raw_fb_addr = mmap(0, size, PROT_READ, MAP_SHARED,
			    fd, 0);

			if (raw_fb_addr == MAP_FAILED || raw_fb_addr == NULL) {
				rfbLogEnable(1);
				rfbLog("failed to mmap file: %s, %s\n", q, str);
				rfbLog("   raw_fb_addr: %p\n", raw_fb_addr);
				rfbLogPerror("mmap");

				raw_fb_addr = NULL;
				rfbLog("mmap(2) failed, trying slower lseek(2)\n");
				raw_fb_seek = size;
				last_mode = RAWFB_FILE;

			} else {
				raw_fb_mmap = size;

				rfbLog("rawfb: mmap file: %s\n", q);
				rfbLog("   w: %d h: %d b: %d addr: %p sz: %d\n", w, h,
				    b, raw_fb_addr, size);
				last_mode = RAWFB_MMAP;
			}
#else
			rfbLog("mmap(2) not supported on system, using"
			    " slower lseek(2)\n");
			raw_fb_seek = size;
			last_mode = RAWFB_FILE;
#endif
		} else {
			raw_fb_seek = size;
			last_mode = RAWFB_FILE;

			rfbLog("rawfb: seek file: %s\n", q);
			rfbLog("   W: %d H: %d B: %d sz: %d\n", w, h, b, size);
		}
	} else {
		rfbLogEnable(1);
		rfbLog("invalid rawfb str: %s\n", str);
		clean_up_exit(1);
	}

	if (unlink_me) {
		unlink(unlink_me);
	}

	if (! raw_fb_image) {
		raw_fb_image = &ximage_struct;
	}

	initialize_clipshift();

	if (raw_fb_bytes_per_line == 0) {
		raw_fb_bytes_per_line = dpy_x*b/8;

		/*
		 * Put cases here were we can determine that
		 * raw_bytes_per_line != dpy_x*b/8
		 */
#ifdef MACOSX
		if (do_macosx) {
			raw_fb_bytes_per_line = macosxCG_CGDisplayBytesPerRow();
		}
#endif
	}

	raw_fb_image->bytes_per_line = dpy_x * b/8;
	raw_fb = (char *) malloc(dpy_y * dpy_x * b/8);
	raw_fb_image->data = raw_fb;
	raw_fb_image->format = ZPixmap;
	raw_fb_image->width  = dpy_x;
	raw_fb_image->height = dpy_y;
	raw_fb_image->bits_per_pixel = b;
	raw_fb_image->bitmap_unit = -1;


	if (use_snapfb && (raw_fb_seek || raw_fb_mmap)) {
		int b_use = b;
		if (snap_fb) {
			free(snap_fb);
		}
		if (b_use == 32 && xform24to32) {
			/*
			 * The actual framebuffer (e.g. mapped addr) and
			 * snap fb must be the same bpp.  E.g. both 24bpp.
			 * Reading FROM snap to utility image will be
			 * transformed 24->32 in copy_raw_fb_24_to_32.
			 *
			 * addr -> snap -> (scanline, fullscreen, ...)
			 */
			b_use = 24;
			raw_fb_bytes_per_line = dpy_x * b_use/8;
		}
		snap_fb = (char *) malloc(dpy_y * dpy_x * b_use/8);
		snap = &ximage_struct_snap;
		snap->data = snap_fb;
		snap->format = ZPixmap;
		snap->width  = dpy_x;
		snap->height = dpy_y;
		snap->bits_per_pixel = b_use;
		snap->bytes_per_line = dpy_x * b_use/8;
		snap->bitmap_unit = -1;
	}


	raw_fb_image->red_mask = rm;
	raw_fb_image->green_mask = gm;
	raw_fb_image->blue_mask = bm;

	raw_fb_image->depth = 0;
	m = 1;
	for (i=0; i<32; i++)  {
		if (rm & m) {
			raw_fb_image->depth++;
		}
		if (gm & m) {
			raw_fb_image->depth++;
		}
		if (bm & m) {
			raw_fb_image->depth++;
		}
		m = m << 1;
	}
	if (raw_fb_native_bpp < 8) {
		raw_fb_image->depth = raw_fb_expand_bytes * 8;
	}
	if (! raw_fb_image->depth) { 
		raw_fb_image->depth = (b == 32) ? 24 : b;
	}

	depth = raw_fb_image->depth;

	if (raw_fb_image->depth == 15) {
		/* unresolved bug with RGB555... */
		depth++;
	}

	if (clipshift || raw_fb_native_bpp < 8) {
		memset(raw_fb, 0xff, dpy_y * raw_fb_image->bytes_per_line);
	} else if (raw_fb_addr && ! xform24to32) {
		memcpy(raw_fb, raw_fb_addr + raw_fb_offset, dpy_y * raw_fb_image->bytes_per_line);
	} else {
		memset(raw_fb, 0xff, dpy_y * raw_fb_image->bytes_per_line);
	}

	if (verbose) {
		rfbLog("\n");
		rfbLog("rawfb:  raw_fb  %p\n", raw_fb);
		rfbLog("        format  %d\n", raw_fb_image->format);
		rfbLog("        width   %d\n", raw_fb_image->width);
		rfbLog("        height  %d\n", raw_fb_image->height);
		rfbLog("        bpp     %d\n", raw_fb_image->bits_per_pixel);
		rfbLog("        depth   %d\n", raw_fb_image->depth);
		rfbLog("        bpl     %d\n", raw_fb_image->bytes_per_line);
		if (use_snapfb && snap_fb) {
			rfbLog("        snap_fb %p\n", snap_fb);
		}
	}

	free(str);

	return raw_fb_image;
}

static void initialize_clipshift(void) {
	clipshift = 0;
	cdpy_x = cdpy_y = coff_x = coff_y = 0;
	if (clip_str) {
		int w, h, x, y, bad = 0;
		if (parse_geom(clip_str, &w, &h, &x, &y, wdpy_x, wdpy_y)) {
			if (x < 0) {
				x = 0;
			}
			if (y < 0) {
				y = 0;
			}
			if (x + w > wdpy_x) {
				w = wdpy_x - x;
			}
			if (y + h > wdpy_y) {
				h = wdpy_y - y;
			}
			if (w <= 0 || h <= 0) {
				bad = 1;
			}
		} else {
			bad = 1;
		}
		if (bad) {
			rfbLog("*** ignoring invalid -clip WxH+X+Y: %s\n",
			    clip_str); 
		} else {
			/* OK, change geom behind everyone's back... */
			cdpy_x = w;
			cdpy_y = h;
			coff_x = x;
			coff_y = y;

			clipshift = 1;

			dpy_x = cdpy_x;
			dpy_y = cdpy_y;
		}
	}
}

static int wait_until_mapped(Window win) {
#if NO_X11
	if (!win) {}
	return 0;
#else
	int ms = 50, waittime = 30;
	time_t start = time(NULL);
	XWindowAttributes attr;

	while (1) {
		if (! valid_window(win, NULL, 0)) {
			if (time(NULL) > start + waittime) {
				break;
			}
			usleep(ms * 1000);
			continue;
		}
		if (dpy && ! XGetWindowAttributes(dpy, win, &attr)) {
			return 0;
		}
		if (attr.map_state == IsViewable) {
			return 1;
		}
		usleep(ms * 1000);
	}
	return 0;
#endif	/* NO_X11 */
}

/*
 * initialize a fb for the X display
 */
XImage *initialize_xdisplay_fb(void) {
#if NO_X11
	if (raw_fb_str) {
		return initialize_raw_fb(0);
	}
	return NULL;
#else
	XImage *fb;
	char *vis_str = visual_str;
	int try = 0, subwin_tries = 3;
	XErrorHandler old_handler = NULL;
	int subwin_bs;

	if (raw_fb_str) {
		return initialize_raw_fb(0);
	}

	X_LOCK;
	if (subwin) {
		if (subwin_wait_mapped) {
			wait_until_mapped(subwin);
		}
		if (!valid_window((Window) subwin, NULL, 0)) {
			rfbLogEnable(1);
			rfbLog("invalid sub-window: 0x%lx\n", subwin);
			X_UNLOCK;
			clean_up_exit(1);
		}
	}
	
	if (overlay) {
		/* 
		 * ideally we'd like to not have to cook up the
		 * visual variables but rather let it all come out
		 * of XReadScreen(), however there is no way to get
		 * a default visual out of it, so we pretend -visual
		 * TrueColor:NN was supplied with NN usually 24.
		 */
		char str[32];
		Window twin = subwin ? subwin : rootwin;
		XImage *xi;

		xi = xreadscreen(dpy, twin, 0, 0, 8, 8, False);
		sprintf(str, "TrueColor:%d", xi->depth);
		if (xi->depth != 24 && ! quiet) {
			rfbLog("warning: overlay image has depth %d "
			    "instead of 24.\n", xi->depth);
		}
		XDestroyImage(xi);
		if (visual_str != NULL && ! quiet) {
			rfbLog("warning: replacing '-visual %s' by '%s' "
			    "for use with -overlay\n", visual_str, str);
		}
		vis_str = strdup(str);
	}

	if (xform24to32) {
		if (DefaultDepth(dpy, scr) == 24) {
			vis_str = strdup("TrueColor:32");
			rfbLog("initialize_xdisplay_fb: vis_str set to: %s\n",
			    vis_str);
			visual_id = (VisualID) 0;
			visual_depth = 0;
			set_visual_str_to_something = 1;
		}
	} else if (DefaultDepth(dpy, scr) < 8) {
		/* check very low bpp case, e.g. mono or vga16 */
		Screen *s = DefaultScreenOfDisplay(dpy);
		XImage *xi = XGetImage_wr(dpy, DefaultRootWindow(dpy), 0, 0, 2, 2, AllPlanes,
		    ZPixmap);
		if (xi && xi->bits_per_pixel < 8) {
			int lowbpp = xi->bits_per_pixel; 
			if (!vis_str) {
				char tmp[32];
				sprintf(tmp, "0x%x:8", (int) s->root_visual->visualid);
				vis_str = strdup(tmp);
				rfbLog("initialize_xdisplay_fb: low bpp[%d], vis_str "
				    "set to: %s\n", lowbpp, vis_str);
			}
			if (using_shm) {
				using_shm = 0;
				rfbLog("initialize_xdisplay_fb: low bpp[%d], "
				    "disabling shm\n", lowbpp);
			}
			visual_id = (VisualID) 0;
			visual_depth = 0;
			set_visual_str_to_something = 1;
		}
		if (xi) {
			XDestroyImage(xi);
		}
	}

	if (vis_str != NULL) {
		set_visual(vis_str);
		if (vis_str != visual_str) {
			free(vis_str);
		}
	}
if (0) fprintf(stderr, "vis_str %s\n", vis_str ? vis_str : "notset");

	/* set up parameters for subwin or non-subwin cases: */

	again:

	if (! subwin) {
		/* full screen */
		window = rootwin;
		dpy_x = wdpy_x = DisplayWidth(dpy, scr);
		dpy_y = wdpy_y = DisplayHeight(dpy, scr);
		off_x = 0;
		off_y = 0;
		/* this may be overridden via visual_id below */
		default_visual = DefaultVisual(dpy, scr);
	} else {
		/* single window */
		XWindowAttributes attr;

		window = (Window) subwin;
		if (! XGetWindowAttributes(dpy, window, &attr)) {
			rfbLogEnable(1);
			rfbLog("invalid window: 0x%lx\n", window);
			X_UNLOCK;
			clean_up_exit(1);
		}
		dpy_x = wdpy_x = attr.width;
		dpy_y = wdpy_y = attr.height;

		subwin_bs = attr.backing_store;

		/* this may be overridden via visual_id below */
		default_visual = attr.visual;

		X_UNLOCK;
		set_offset();
		X_LOCK;
	}

	initialize_clipshift();

	/* initialize depth to reasonable value, visual_id may override */
	depth = DefaultDepth(dpy, scr);

if (0) fprintf(stderr, "DefaultDepth: %d  visial_id: %d\n", depth, (int) visual_id);

	if (visual_id) {
		int n;
		XVisualInfo vinfo_tmpl, *vinfo;

		/*
		 * we are in here from -visual or -overlay options
		 * visual_id and visual_depth were set in set_visual().
		 */

		vinfo_tmpl.visualid = visual_id; 
		vinfo = XGetVisualInfo(dpy, VisualIDMask, &vinfo_tmpl, &n);
		if (vinfo == NULL || n == 0) {
			rfbLogEnable(1);
			rfbLog("could not match visual_id: 0x%x\n",
			    (int) visual_id);
			X_UNLOCK;
			clean_up_exit(1);
		}
		default_visual = vinfo->visual;
		depth = vinfo->depth;
		if (visual_depth) {
			/* force it from -visual MooColor:NN */
			depth = visual_depth;
		}
		if (! quiet) {
			fprintf(stderr, " initialize_xdisplay_fb()\n");
			fprintf(stderr, " Visual*:    %p\n",
			    (void *) vinfo->visual);
			fprintf(stderr, " visualid:   0x%x\n",
			    (int) vinfo->visualid);
			fprintf(stderr, " screen:     %d\n", vinfo->screen);
			fprintf(stderr, " depth:      %d\n", vinfo->depth);
			fprintf(stderr, " class:      %d\n", vinfo->class);
			fprintf(stderr, " red_mask:   0x%08lx  %s\n",
			    vinfo->red_mask, bitprint(vinfo->red_mask, 32));
			fprintf(stderr, " green_mask: 0x%08lx  %s\n",
			    vinfo->green_mask, bitprint(vinfo->green_mask, 32));
			fprintf(stderr, " blue_mask:  0x%08lx  %s\n",
			    vinfo->blue_mask, bitprint(vinfo->blue_mask, 32));
			fprintf(stderr, " cmap_size:  %d\n",
			    vinfo->colormap_size);
			fprintf(stderr, " bits b/rgb: %d\n",
			    vinfo->bits_per_rgb);
			fprintf(stderr, "\n");
		}
		XFree_wr(vinfo);
	}

	if (! quiet) {
		rfbLog("Default visual ID: 0x%x\n",
		    (int) XVisualIDFromVisual(default_visual));
	}

	if (subwin) {
		int shift = 0, resize = 0;
		int subwin_x, subwin_y;
		int disp_x = DisplayWidth(dpy, scr);
		int disp_y = DisplayHeight(dpy, scr);
		Window twin;
		/* subwins can be a dicey if they are changing size... */
		trapped_xerror = 0;
		old_handler = XSetErrorHandler(trap_xerror);	/* reset in if(subwin) block below */
		XTranslateCoordinates(dpy, window, rootwin, 0, 0, &subwin_x,
		    &subwin_y, &twin);

		if (wdpy_x > disp_x) {
			resize = 1;
			dpy_x = wdpy_x = disp_x - 4;
		}
		if (wdpy_y > disp_y) {
			resize = 1;
			dpy_y = wdpy_y = disp_y - 4;
		}

		if (subwin_x + wdpy_x > disp_x) {
			shift = 1;
			subwin_x = disp_x - wdpy_x - 3;
		}
		if (subwin_y + wdpy_y > disp_y) {
			shift = 1;
			subwin_y = disp_y - wdpy_y - 3;
		}
		if (subwin_x < 0) {
			shift = 1;
			subwin_x = 1;
		}
		if (subwin_y < 0) {
			shift = 1;
			subwin_y = 1;
		}

		if (resize) {
			XResizeWindow(dpy, window, wdpy_x, wdpy_y);
		}
		if (shift) {
			XMoveWindow(dpy, window, subwin_x, subwin_y);
			off_x = subwin_x;
			off_y = subwin_y;
		}
		XMapRaised(dpy, window);
		XRaiseWindow(dpy, window);
		XSync(dpy, False);
	}
	try++;

	if (nofb) {
		/* 
		 * For -nofb we do not allocate the framebuffer, so we
		 * can save a few MB of memory. 
		 */
		fb = XCreateImage_wr(dpy, default_visual, depth, ZPixmap,
		    0, NULL, dpy_x, dpy_y, BitmapPad(dpy), 0);

	} else if (visual_id) {
		/*
		 * we need to call XCreateImage to supply the visual
		 */
		fb = XCreateImage_wr(dpy, default_visual, depth, ZPixmap,
		    0, NULL, dpy_x, dpy_y, BitmapPad(dpy), 0);
		if (fb) {
			fb->data = (char *) malloc(fb->bytes_per_line * fb->height);
		}

	} else {
		fb = XGetImage_wr(dpy, window, 0, 0, dpy_x, dpy_y, AllPlanes,
		    ZPixmap);
		if (! quiet) {
			rfbLog("Read initial data from X display into"
			    " framebuffer.\n");
		}
	}

	if (subwin) {
		XSetErrorHandler(old_handler);
		if (trapped_xerror || fb == NULL) {
		    rfbLog("trapped GetImage at SUBWIN creation.\n");
		    if (try < subwin_tries) {
			usleep(250 * 1000);
			if (!get_window_size(window, &wdpy_x, &wdpy_y)) {
				rfbLogEnable(1);
				rfbLog("could not get size of subwin "
				    "0x%lx\n", subwin);
				X_UNLOCK;
				clean_up_exit(1);
			}
			goto again;
		    }
		}
		trapped_xerror = 0;

	} else if (fb == NULL) {
		XEvent xev;
		rfbLog("initialize_xdisplay_fb: *** fb creation failed: 0x%x try: %d\n", fb, try);
#if LIBVNCSERVER_HAVE_LIBXRANDR
		if (xrandr_present && xrandr_base_event_type) {
			int cnt = 0;
			while (XCheckTypedEvent(dpy, xrandr_base_event_type + RRScreenChangeNotify, &xev)) {
				XRRScreenChangeNotifyEvent *rev;
				rev = (XRRScreenChangeNotifyEvent *) &xev;

				rfbLog("initialize_xdisplay_fb: XRANDR event while redoing fb[%d]:\n", cnt++);
				rfbLog("  serial:          %d\n", (int) rev->serial);
				rfbLog("  timestamp:       %d\n", (int) rev->timestamp);
				rfbLog("  cfg_timestamp:   %d\n", (int) rev->config_timestamp);
				rfbLog("  size_id:         %d\n", (int) rev->size_index);
				rfbLog("  sub_pixel:       %d\n", (int) rev->subpixel_order);
				rfbLog("  rotation:        %d\n", (int) rev->rotation);
				rfbLog("  width:           %d\n", (int) rev->width);
				rfbLog("  height:          %d\n", (int) rev->height);
				rfbLog("  mwidth:          %d mm\n", (int) rev->mwidth);
				rfbLog("  mheight:         %d mm\n", (int) rev->mheight);
				rfbLog("\n");
				rfbLog("previous WxH: %dx%d\n", wdpy_x, wdpy_y);

				xrandr_width  = rev->width;
				xrandr_height = rev->height;
				xrandr_timestamp = rev->timestamp;
				xrandr_cfg_time  = rev->config_timestamp;
				xrandr_rotation = (int) rev->rotation;

				rfbLog("initialize_xdisplay_fb: updating XRANDR config...\n");
				XRRUpdateConfiguration(&xev);
			}
		}
#endif
		if (try < 5)  {
			XFlush_wr(dpy);
			usleep(250 * 1000);
			if (try < 3) {
				XSync(dpy, False);
			} else if (try >= 3) {
				XSync(dpy, True);
			}
			goto again;
		}
	}
	if (use_snapfb) {
		initialize_snap_fb();
	}

	X_UNLOCK;

	if (fb->bits_per_pixel == 24 && ! quiet) {
		rfbLog("warning: 24 bpp may have poor performance.\n");
	}
	return fb;
#endif	/* NO_X11 */
}

void parse_scale_string(char *str, double *factor_x, double *factor_y, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom, int w_in, int h_in) {

	int m, n;
	char *p, *tstr;
	double f, f2;

	*factor_x = 1.0;
	*factor_y = 1.0;
	*scaling = 0;
	*blend = 1;
	*nomult4 = 0;
	*pad = 0;
	*interpolate = 0;
	*numer = 0, *denom = 0;

	if (str == NULL || str[0] == '\0') {
		return;
	}
	tstr = strdup(str);
	
	if ( (p = strchr(tstr, ':')) != NULL) {
		/* options */
		if (strstr(p+1, "nb") != NULL) {
			*blend = 0;
		}
		if (strstr(p+1, "fb") != NULL) {
			*blend = 2;
		}
		if (strstr(p+1, "n4") != NULL) {
			*nomult4 = 1;
		}
		if (strstr(p+1, "in") != NULL) {
			*interpolate = 1;
		}
		if (strstr(p+1, "pad") != NULL) {
			*pad = 1;
		}
		if (strstr(p+1, "nocr") != NULL) {
			/* global */
			scaling_copyrect = 0;
		} else if (strstr(p+1, "cr") != NULL) {
			/* global */
			scaling_copyrect = 1;
		}
		*p = '\0';
	}

	if (strchr(tstr, '.') != NULL) {
		double test, diff, eps = 1.0e-7;
		if (sscanf(tstr, "%lfx%lf", &f, &f2) == 2) {
			*factor_x = (double) f;
			*factor_y = (double) f2;
		} else if (sscanf(tstr, "%lf", &f) != 1) {
			rfbLogEnable(1);
			rfbLog("invalid -scale arg: %s\n", tstr);
			clean_up_exit(1);
		} else {
			*factor_x = (double) f;
			*factor_y = (double) f;
		}
		/* look for common fractions from small ints: */
		if (*factor_x == *factor_y) {
			for (n=2; n<=10; n++) {
				for (m=1; m<n; m++) {
					test = ((double) m)/ n;
					diff = *factor_x - test;
					if (-eps < diff && diff < eps) {
						*numer = m;
						*denom = n;
						break;
					
					}
				}
				if (*denom) {
					break;
				}
			}
			if (*factor_x < 0.01) {
				rfbLogEnable(1);
				rfbLog("-scale factor too small: %f\n", *factor_x);
				clean_up_exit(1);
			}
		}
	} else if (sscanf(tstr, "%dx%d", &m, &n) == 2 && w_in > 0 && h_in > 0) {
		*factor_x = ((double) m) / ((double) w_in);
		*factor_y = ((double) n) / ((double) h_in);
	} else {
		if (sscanf(tstr, "%d/%d", &m, &n) != 2) {
			if (sscanf(tstr, "%d", &m) != 1) {
				rfbLogEnable(1);
				rfbLog("invalid -scale arg: %s\n", tstr);
				clean_up_exit(1);
			} else {
				/* e.g. -scale 1 or -scale 2 */
				n = 1;
			}
		}
		if (n <= 0 || m <=0) {
			rfbLogEnable(1);
			rfbLog("invalid -scale arg: %s\n", tstr);
			clean_up_exit(1);
		}
		*factor_x = ((double) m)/ n;
		*factor_y = ((double) m)/ n;
		if (*factor_x < 0.01) {
			rfbLogEnable(1);
			rfbLog("-scale factor too small: %f\n", *factor_x);
			clean_up_exit(1);
		}
		*numer = m;
		*denom = n;
	}
	if (*factor_x == 1.0 && *factor_y == 1.0) {
		if (! quiet) {
			rfbLog("scaling disabled for factor %f %f\n", *factor_x, *factor_y);
		}
	} else {
		*scaling = 1;
	}
	free(tstr);
}

int parse_rotate_string(char *str, int *mode) {
	int m = ROTATE_NONE;
	if (str == NULL || !strcmp(str, "") || !strcmp(str, "0")) {
		m = ROTATE_NONE;
	} else if (!strcmp(str, "x")) {
		m = ROTATE_X;
	} else if (!strcmp(str, "y")) {
		m = ROTATE_Y;
	} else if (!strcmp(str, "xy") || !strcmp(str, "yx") ||
	    !strcmp(str,"+180") || !strcmp(str,"-180") || !strcmp(str,"180")) {
		m = ROTATE_XY;
	} else if (!strcmp(str, "+90") || !strcmp(str, "90")) {
		m = ROTATE_90;
	} else if (!strcmp(str, "+90x") || !strcmp(str, "90x")) {
		m = ROTATE_90X;
	} else if (!strcmp(str, "+90y") || !strcmp(str, "90y")) {
		m = ROTATE_90Y;
	} else if (!strcmp(str, "-90") || !strcmp(str, "270") ||
	    !strcmp(str, "+270")) {
		m = ROTATE_270;
	} else {
		rfbLog("invalid -rotate mode: %s\n", str);
	}
	if (mode) {
		*mode = m;
	}
	return m;
}

int scale_round(int len, double fac) {
	double eps = 0.000001;
	
	len = (int) (len * fac + eps);
	if (len < 1) {
		len = 1;
	}
	return len;
}

static void setup_scaling(int *width_in, int *height_in) {
	int width  = *width_in;
	int height = *height_in;

	parse_scale_string(scale_str, &scale_fac_x, &scale_fac_y, &scaling, &scaling_blend,
	    &scaling_nomult4, &scaling_pad, &scaling_interpolate,
	    &scale_numer, &scale_denom, *width_in, *height_in);

	if (scaling) {
		width  = scale_round(width,  scale_fac_x);
		height = scale_round(height, scale_fac_y);
		if (scale_denom && scaling_pad) {
			/* it is not clear this padding is useful anymore */
			rfbLog("width  %% denom: %d %% %d = %d\n", width,
			    scale_denom, width  % scale_denom);
			rfbLog("height %% denom: %d %% %d = %d\n", height,
			    scale_denom, height % scale_denom);
			if (width % scale_denom != 0) {
				int w = width;
				w += scale_denom - (w % scale_denom);
				if (!scaling_nomult4 && w % 4 != 0) {
					/* need to make mult of 4 as well */
					int c = 0;	
					while (w % 4 != 0 && c++ <= 5) {
						w += scale_denom;
					}
				}
				width = w;
				rfbLog("padded width  to: %d (mult of %d%s\n",
				    width, scale_denom, !scaling_nomult4 ?
				    " and 4)" : ")");
			}
			if (height % scale_denom != 0) {
				height += scale_denom - (height % scale_denom);
				rfbLog("padded height to: %d (mult of %d)\n",
				    height, scale_denom);
			}
		}
		if (!scaling_nomult4 && width % 4 != 0 && width > 2) {
			/* reset width to be multiple of 4 */
			int width0 = width;
			if ((width+1) % 4 == 0) {
				width = width+1;
			} else if ((width-1) % 4 == 0) {
				width = width-1;
			} else if ((width+2) % 4 == 0) {
				width = width+2;
			}
			rfbLog("reset scaled width %d -> %d to be a multiple of"
			    " 4 (to\n", width0, width);
			rfbLog("make vncviewers happy). use -scale m/n:n4 to "
			    "disable.\n");
		}
		scaled_x = width;
		scaled_y = height;

		*width_in  = width;
		*height_in = height;
	}
}

static void setup_rotating(void) {
	char *rs = rotating_str;

	rotating_cursors = 1;
	if (rs && strstr(rs, "nc:") == rs) {
		rs += strlen("nc:");
		rotating_cursors = 0;
	}

	rotating = parse_rotate_string(rs, NULL);
	if (! rotating) {
		rotating_cursors = 0;
	}

	if (rotating == ROTATE_90  || rotating == ROTATE_90X ||
	    rotating == ROTATE_90Y || rotating == ROTATE_270) {
		rotating_same = 0;
	} else {
		rotating_same = 1;
	}
}

static rfbBool set_xlate_wrapper(rfbClientPtr cl) {
	static int first = 1;
	if (first) {
		first = 0;
	} else if (ncache) {
		int save = ncache_xrootpmap;
		rfbLog("set_xlate_wrapper: clearing -ncache for new pixel format.\n");
		INPUT_LOCK;
		ncache_xrootpmap = 0;
		check_ncache(1, 0);
		ncache_xrootpmap = save;
		INPUT_UNLOCK;
	}
	return rfbSetTranslateFunction(cl);	
}

/*
 * initialize the rfb framebuffer/screen
 */
void initialize_screen(int *argc, char **argv, XImage *fb) {
	int have_masks = 0;
	int width  = fb->width;
	int height = fb->height;
	int create_screen = screen ? 0 : 1;
	int bits_per_color;
	int fb_bpp, fb_Bpl, fb_depth;
	int locked_sends = 0;

	bpp = fb->bits_per_pixel;

	fb_bpp   = (int) fb->bits_per_pixel;
	fb_Bpl   = (int) fb->bytes_per_line;
	fb_depth = (int) fb->depth;

	rfbLog("initialize_screen: fb_depth/fb_bpp/fb_Bpl %d/%d/%d\n", fb_depth,
	    fb_bpp, fb_Bpl);

	main_bytes_per_line = fb->bytes_per_line;

	if (cmap8to24) {
		if (raw_fb) {
			if (!quiet) rfbLog("disabling -8to24 mode"
			    " in raw_fb mode.\n");
			cmap8to24 = 0;
		} else if (depth == 24) {
			if (fb_bpp != 32)  {
				if (!quiet) rfbLog("disabling -8to24 mode:"
				    " bpp != 32 with depth == 24\n");
				cmap8to24 = 0;
			}
		} else if (depth <= 16) {
			/* need to cook up the screen fb to be depth 24 */
			fb_bpp = 32;
			fb_depth = 24;
		} else {
			if (!quiet) rfbLog("disabling -8to24 mode:"
			    " default depth not 16 or less\n");
			cmap8to24 = 0;
		}
	}

	setup_scaling(&width, &height);

	if (scaling) {
		rfbLog("scaling screen: %dx%d -> %dx%d\n", fb->width, fb->height, scaled_x, scaled_y);
		rfbLog("scaling screen: scale_fac_x=%.5f scale_fac_y=%.5f\n", scale_fac_x, scale_fac_y);

		rfb_bytes_per_line = (main_bytes_per_line / fb->width) * width;
	} else {
		rfb_bytes_per_line = main_bytes_per_line;
	}

	setup_rotating();

	if (rotating) {
		if (! rotating_same) {
			int t, b = main_bytes_per_line / fb->width;
			if (scaling) {
				rot_bytes_per_line = b * height;
			} else {
				rot_bytes_per_line = b * fb->height;
			}
			t = width;
			width = height;		/* The big swap... */
			height = t;
		} else {
			rot_bytes_per_line = rfb_bytes_per_line;
		}
	}

#ifndef NO_NCACHE
	if (ncache > 0 && !nofb) {
# ifdef MACOSX
		if (! raw_fb_str || macosx_console) {
# else
		if (! raw_fb_str) {
# endif
			
			char *new_fb;
			int sz = fb->height * fb->bytes_per_line;
			int ns = 1+ncache;

			if (ncache_xrootpmap) {
				ns++;
			}

			new_fb = (char *) calloc((size_t) (sz * ns), 1);
			if (fb->data) {
				memcpy(new_fb, fb->data, sz);
				free(fb->data);
			}
			fb->data = new_fb;
			fb->height *= (ns);
			height *= (ns);
			ncache0 = ncache;
		}
	}
#endif

	if (cmap8to24) {
		if (depth <= 8) {
			rfb_bytes_per_line *= 4;
			rot_bytes_per_line *= 4;
		} else if (depth <= 16) {
			rfb_bytes_per_line *= 2;
			rot_bytes_per_line *= 2;
		}
	}

	/*
	 * These are just hints wrt pixel format just to let
	 * rfbGetScreen/rfbNewFramebuffer proceed with reasonable
	 * defaults.  We manually set them in painful detail below.
	 */
	bits_per_color = guess_bits_per_color(fb_bpp);

	if (lock_client_sends(-1) == 0) {
		lock_client_sends(1);
		locked_sends = 1;
	}

	/* n.b. samplesPerPixel (set = 1 here) seems to be unused. */
	if (create_screen) {
		if (use_stunnel) {
			setup_stunnel(0, argc, argv);
		}
		if (use_openssl) {
			if (use_stunnel && enc_str && !strcmp(enc_str, "none")) {
				/* emulating HTTPS oneport */
				;
			} else {
				openssl_init(0);
			}
		}
		screen = rfbGetScreen(argc, argv, width, height,
		    bits_per_color, 1, fb_bpp/8);
		if (screen && http_dir) {
			http_connections(1);
		}
		if (unix_sock) {
			unix_sock_fd = listen_unix(unix_sock);
		}
	} else {
		/* set set frameBuffer member below. */
		rfbLog("rfbNewFramebuffer(0x%x, 0x%x, %d, %d, %d, %d, %d)\n",
		    screen, NULL, width, height, bits_per_color, 1, fb_bpp/8);

		/* these are probably overwritten, but just to be safe: */
		screen->bitsPerPixel = fb_bpp;
		screen->depth = fb_depth;

		rfb_new_framebuffer(screen, NULL, width, height,
		    bits_per_color, 1, (int) fb_bpp/8);
	}
	if (! screen) {
		int i;
		rfbLogEnable(1);
		rfbLog("\n");
		rfbLog("failed to create rfb screen.\n");
		for (i=0; i< *argc; i++)  {
			rfbLog("\t[%d]  %s\n", i, argv[i]);
		}
		clean_up_exit(1);
	}

	if (create_screen && *argc != 1) {
		int i;
		rfbLogEnable(1);
		rfbLog("*** unrecognized option(s) ***\n");
		for (i=1; i< *argc; i++)  {
			rfbLog("\t[%d]  %s\n", i, argv[i]);
		}
		rfbLog("For a list of options run: x11vnc -opts\n");
		rfbLog("or for the full help: x11vnc -help\n");
		rfbLog("\n");
		rfbLog("Here is a list of removed or obsolete"
		    " options:\n");
		rfbLog("\n");
		rfbLog("removed: -hints, -nohints\n");
		rfbLog("removed: -cursorposall\n");
		rfbLog("removed: -nofilexfer, now the default.\n");
		rfbLog("\n");
		rfbLog("renamed: -old_copytile, use -onetile\n");
		rfbLog("renamed: -mouse,   use -cursor\n");
		rfbLog("renamed: -mouseX,  use -cursor X\n");
		rfbLog("renamed: -X,       use -cursor X\n");
		rfbLog("renamed: -nomouse, use -nocursor\n");
		rfbLog("renamed: -old_pointer, use -pointer_mode 1\n");
	
		clean_up_exit(1);
	}

	/* set up format from scratch: */
	if (rotating && ! rotating_same) {
		screen->paddedWidthInBytes = rot_bytes_per_line;
	} else {
		screen->paddedWidthInBytes = rfb_bytes_per_line;
	}
	screen->serverFormat.bitsPerPixel = fb_bpp;
	screen->serverFormat.depth = fb_depth;
	screen->serverFormat.trueColour = TRUE;

	screen->serverFormat.redShift   = 0;
	screen->serverFormat.greenShift = 0;
	screen->serverFormat.blueShift  = 0;
	screen->serverFormat.redMax     = 0;
	screen->serverFormat.greenMax   = 0;
	screen->serverFormat.blueMax    = 0;

	/* these main_* formats are used generally. */
	main_red_shift   = 0;
	main_green_shift = 0;
	main_blue_shift  = 0;
	main_red_max     = 0;
	main_green_max   = 0;
	main_blue_max    = 0;
	main_red_mask    = fb->red_mask;
	main_green_mask  = fb->green_mask;
	main_blue_mask   = fb->blue_mask;

	have_masks = ((fb->red_mask|fb->green_mask|fb->blue_mask) != 0);
	if (force_indexed_color) {
		have_masks = 0;
	}

	if (cmap8to24 && depth <= 16 && dpy) {
		XVisualInfo vinfo;
		vinfo.red_mask = 0;
		vinfo.green_mask = 0;
		vinfo.blue_mask = 0;
		/* more cooking up... */
		have_masks = 2;
		/* need to fetch TrueColor visual */
		X_LOCK;
		if (dpy
#if !NO_X11
		    && XMatchVisualInfo(dpy, scr, 24, TrueColor, &vinfo)
#endif
		    ) {
			main_red_mask   = vinfo.red_mask;
			main_green_mask = vinfo.green_mask;
			main_blue_mask  = vinfo.blue_mask;
		} else if (fb->byte_order == LSBFirst) {
			main_red_mask   = 0x00ff0000;
			main_green_mask = 0x0000ff00;
			main_blue_mask  = 0x000000ff;
		} else {
			main_red_mask   = 0x000000ff;
			main_green_mask = 0x0000ff00;
			main_blue_mask  = 0x00ff0000;
		}
		X_UNLOCK;
	}

	if (raw_fb_str && raw_fb_pixfmt) {
		char *fmt = strdup(raw_fb_pixfmt);
		uppercase(fmt);
		if (strstr(fmt, "GREY")) {
			set_greyscale_colormap();
		} else if (strstr(fmt, "HI240")) {
			set_hi240_colormap();
		} else {
			unset_colormap();
		}
		free(fmt);
	}

	if (! have_masks && screen->serverFormat.bitsPerPixel <= 16
	    && dpy && CellsOfScreen(ScreenOfDisplay(dpy, scr))) {
		/* indexed color. we let 1 <= bpp <= 16, but it is normally 8 */
		if (!quiet) {
			rfbLog("\n");
			rfbLog("X display %s is %dbpp indexed color, depth=%d\n",
			    DisplayString(dpy), screen->serverFormat.bitsPerPixel,
			    screen->serverFormat.depth);
			if (! flash_cmap && ! overlay) {
				rfbLog("\n");
				rfbLog("In 8bpp PseudoColor mode if you "
				    "experience color\n");
				rfbLog("problems you may want to enable "
				    "following the\n");
				rfbLog("changing colormap by using the "
				    "-flashcmap option.\n");
				rfbLog("\n");
			}
		}
		if (screen->serverFormat.depth < 8) {
			rfbLog("resetting serverFormat.depth %d -> 8.\n",
			    screen->serverFormat.depth);
			rfbLog("resetting serverFormat.bpp   %d -> 8.\n",
			    screen->serverFormat.bitsPerPixel);
			screen->serverFormat.depth = 8;
			screen->serverFormat.bitsPerPixel = 8;
		}
		if (screen->serverFormat.depth > 8) {
			rfbLog("WARNING: indexed color depth > 8, Tight encoding will crash.\n");
		}

		screen->serverFormat.trueColour = FALSE;
		indexed_color = 1;
		set_colormap(1);
		debug_colormap(fb);
	} else {
		/* 
		 * general case, we call it truecolor, but could be direct
		 * color, static color, etc....
		 */
		if (! quiet) {
			if (raw_fb) {
				rfbLog("\n");
				rfbLog("Raw fb at addr %p is %dbpp depth=%d "
				    "true color\n", raw_fb_addr,
				    fb_bpp, fb_depth);
			} else if (! dpy) {
				;
			} else if (have_masks == 2) {
				rfbLog("\n");
				rfbLog("X display %s is %dbpp depth=%d indexed "
				    "color (-8to24 mode)\n", DisplayString(dpy),
				    fb->bits_per_pixel, fb->depth);
			} else {
				rfbLog("\n");
				rfbLog("X display %s is %dbpp depth=%d true "
				    "color\n", DisplayString(dpy),
				    fb_bpp, fb_depth);
			}
		}

		indexed_color = 0;

		if (!have_masks) {
			int M, B = bits_per_color;

			if (3*B > fb_bpp) B--;
			if (B < 1) B = 1;
			M = (1 << B) - 1;

			rfbLog("WARNING: all TrueColor RGB masks are zero!\n");
			rfbLog("WARNING: cooking something up for VNC fb...  B=%d M=%d\n", B, M);
			main_red_mask   = M;
			main_green_mask = M;
			main_blue_mask  = M;
			main_red_mask   = main_red_mask   << (2*B);
			main_green_mask = main_green_mask << (1*B);
			main_blue_mask  = main_blue_mask  << (0*B);
			fprintf(stderr, " red_mask:   0x%08lx  %s\n", main_red_mask,
			    bitprint(main_red_mask, 32));
			fprintf(stderr, " green_mask: 0x%08lx  %s\n", main_green_mask,
			    bitprint(main_green_mask, 32));
			fprintf(stderr, " blue_mask:  0x%08lx  %s\n", main_blue_mask,
			    bitprint(main_blue_mask, 32));
		}

		/* convert masks to bit shifts and max # colors */
		if (main_red_mask) {
			while (! (main_red_mask
			    & (1 << screen->serverFormat.redShift))) {
				    screen->serverFormat.redShift++;
			}
		}
		if (main_green_mask) {
			while (! (main_green_mask
			    & (1 << screen->serverFormat.greenShift))) {
				    screen->serverFormat.greenShift++;
			}
		}
		if (main_blue_mask) {
			while (! (main_blue_mask
			    & (1 << screen->serverFormat.blueShift))) {
				    screen->serverFormat.blueShift++;
			}
		}
		screen->serverFormat.redMax
		    = main_red_mask   >> screen->serverFormat.redShift;
		screen->serverFormat.greenMax
		    = main_green_mask >> screen->serverFormat.greenShift;
		screen->serverFormat.blueMax
		    = main_blue_mask  >> screen->serverFormat.blueShift;

		main_red_max     = screen->serverFormat.redMax;
		main_green_max   = screen->serverFormat.greenMax;
		main_blue_max    = screen->serverFormat.blueMax;

		main_red_shift   = screen->serverFormat.redShift;
		main_green_shift = screen->serverFormat.greenShift;
		main_blue_shift  = screen->serverFormat.blueShift;
	}

#if !SMALL_FOOTPRINT
	if (verbose) {
		fprintf(stderr, "\n");
		fprintf(stderr, "FrameBuffer Info:\n");
		fprintf(stderr, " width:            %d\n", fb->width);
		fprintf(stderr, " height:           %d\n", fb->height);
		fprintf(stderr, " scaled_width:     %d\n", width);
		fprintf(stderr, " scaled_height:    %d\n", height);
		fprintf(stderr, " indexed_color:    %d\n", indexed_color);
		fprintf(stderr, " bits_per_pixel:   %d\n", fb->bits_per_pixel);
		fprintf(stderr, " depth:            %d\n", fb->depth);
		fprintf(stderr, " red_mask:   0x%08lx  %s\n", fb->red_mask,
		    bitprint(fb->red_mask, 32));
		fprintf(stderr, " green_mask: 0x%08lx  %s\n", fb->green_mask,
		    bitprint(fb->green_mask, 32));
		fprintf(stderr, " blue_mask:  0x%08lx  %s\n", fb->blue_mask,
		    bitprint(fb->blue_mask, 32));
		if (cmap8to24) {
			fprintf(stderr, " 8to24 info:\n");
			fprintf(stderr, " fb_bpp:           %d\n", fb_bpp);
			fprintf(stderr, " fb_depth:         %d\n", fb_depth);
			fprintf(stderr, " red_mask:   0x%08lx  %s\n", main_red_mask,
			    bitprint(main_red_mask, 32));
			fprintf(stderr, " green_mask: 0x%08lx  %s\n", main_green_mask,
			    bitprint(main_green_mask, 32));
			fprintf(stderr, " blue_mask:  0x%08lx  %s\n", main_blue_mask,
			    bitprint(main_blue_mask, 32));
		}
		fprintf(stderr, " red:   max: %3d  shift: %2d\n",
			main_red_max, main_red_shift);
		fprintf(stderr, " green: max: %3d  shift: %2d\n",
			main_green_max, main_green_shift);
		fprintf(stderr, " blue:  max: %3d  shift: %2d\n",
			main_blue_max, main_blue_shift);
		fprintf(stderr, " mainfb_bytes_per_line: %d\n",
			main_bytes_per_line);
		fprintf(stderr, " rfb_fb_bytes_per_line: %d\n",
			rfb_bytes_per_line);
		fprintf(stderr, " rot_fb_bytes_per_line: %d\n",
			rot_bytes_per_line);
		fprintf(stderr, " raw_fb_bytes_per_line: %d\n",
			raw_fb_bytes_per_line);
		switch(fb->format) {
		case XYBitmap:
			fprintf(stderr, " format:     XYBitmap\n"); break;
		case XYPixmap:
			fprintf(stderr, " format:     XYPixmap\n"); break;
		case ZPixmap:
			fprintf(stderr, " format:     ZPixmap\n"); break;
		default:
			fprintf(stderr, " format:     %d\n", fb->format); break;
		}
		switch(fb->byte_order) {
		case LSBFirst:
			fprintf(stderr, " byte_order: LSBFirst\n"); break;
		case MSBFirst:
			fprintf(stderr, " byte_order: MSBFirst\n"); break;
		default:
			fprintf(stderr, " byte_order: %d\n", fb->byte_order);
			break;
		}
		fprintf(stderr, " bitmap_pad:  %d\n", fb->bitmap_pad);
		fprintf(stderr, " bitmap_unit: %d\n", fb->bitmap_unit);
		switch(fb->bitmap_bit_order) {
		case LSBFirst:
			fprintf(stderr, " bitmap_bit_order: LSBFirst\n"); break;
		case MSBFirst:
			fprintf(stderr, " bitmap_bit_order: MSBFirst\n"); break;
		default:
			fprintf(stderr, " bitmap_bit_order: %d\n",
			fb->bitmap_bit_order); break;
		}
	}
	if (overlay && ! quiet) {
		rfbLog("\n");
		rfbLog("Overlay mode enabled:  If you experience color\n");
		rfbLog("problems when popup menus are on the screen, try\n");
		rfbLog("disabling SaveUnders in your X server, one way is\n");
		rfbLog("to start the X server with the '-su' option, e.g.:\n");
		rfbLog("Xsun -su ... see Xserver(1), xinit(1) for more info.\n");
		rfbLog("\n");
	}
#endif
	/* nofb is for pointer/keyboard only handling.  */
	if (nofb) {
		main_fb = NULL;
		rfb_fb = main_fb;
		cmap8to24_fb = NULL;
		rot_fb = NULL;
		screen->displayHook = nofb_hook;
	} else {
		main_fb = fb->data;
		rfb_fb = NULL;
		cmap8to24_fb = NULL;
		rot_fb = NULL;

		if (cmap8to24) {
			int n = main_bytes_per_line * fb->height;
			if (depth <= 8) {
				n *= 4;
			} else if (depth <= 16) {
				n *= 2;
			}
			cmap8to24_fb = (char *) malloc(n);
			memset(cmap8to24_fb, 0, n);
		}

		if (rotating) {
			int n = rot_bytes_per_line * height;
			rot_fb = (char *) malloc(n);
			memset(rot_fb, 0, n);
		}

		if (scaling) {
			int n = rfb_bytes_per_line * height;

			if (rotating && ! rotating_same) {
				n = rot_bytes_per_line * height;
			}

			rfb_fb = (char *) malloc(n);
			memset(rfb_fb, 0, n);

		} else if (cmap8to24) {
			rfb_fb = cmap8to24_fb;	
		} else {
			rfb_fb = main_fb;
		}
	}
	if (rot_fb) {
		screen->frameBuffer = rot_fb;
	} else {
		screen->frameBuffer = rfb_fb;
	}
	if (verbose) {
		fprintf(stderr, " rfb_fb:      %p\n", rfb_fb);
		fprintf(stderr, " main_fb:     %p\n", main_fb);
		fprintf(stderr, " 8to24_fb:    %p\n", cmap8to24_fb);
		fprintf(stderr, " rot_fb:      %p\n", rot_fb);
		fprintf(stderr, " snap_fb:     %p\n", snap_fb);
		fprintf(stderr, " raw_fb:      %p\n", raw_fb);
		fprintf(stderr, " fake_fb:     %p\n", fake_fb);
		fprintf(stderr, "\n");
	}

	/* may need, bpp, main_red_max, etc. */
	parse_wireframe();
	parse_scroll_copyrect();
	setup_cursors_and_push();

	if (scaling || rotating || cmap8to24) {
		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	}

	if (! create_screen) {
		rfbClientIteratorPtr iter;
		rfbClientPtr cl;

		/* 
		 * since bits_per_color above may have been approximate,
		 * try to reset the individual translation tables...
		 * we do not seem to need this with rfbGetScreen()...
		 */
		if (!quiet) rfbLog("calling setTranslateFunction()...\n");
		iter = rfbGetClientIterator(screen);
		while ((cl = rfbClientIteratorNext(iter)) != NULL) {
			screen->setTranslateFunction(cl);
		}
		rfbReleaseClientIterator(iter);
		if (!quiet) rfbLog("  done.\n");
		
		/* done for framebuffer change case */
		if (locked_sends) {
			lock_client_sends(0);
		}

		do_copy_screen = 1;
		return;
	}

	/*
	 * the rest is screen server initialization, etc, only needed
	 * at screen creation time.
	 */

	/* event callbacks: */
	screen->newClientHook = new_client;
	screen->kbdAddEvent = keyboard;
	screen->ptrAddEvent = pointer_event;
	screen->setXCutText = xcut_receive;
	screen->setTranslateFunction = set_xlate_wrapper;

	screen->kbdReleaseAllKeys = kbd_release_all_keys; 
	screen->setSingleWindow = set_single_window; 
	screen->setServerInput = set_server_input; 
	screen->setTextChat = set_text_chat; 
	screen->getFileTransferPermission = get_file_transfer_permitted; 

	/* called from inetd, we need to treat stdio as our socket */
	if (inetd && use_openssl) {
		/* accept_openssl() called later */
		screen->port = 0;
	} else if (inetd) {
		int fd = dup(0);
		if (fd < 0) {
			rfbLogEnable(1);
			rfbErr("dup(0) = %d failed.\n", fd);
			rfbLogPerror("dup");
			clean_up_exit(1);
		}
		fclose(stdin);
		fclose(stdout);
		/* we keep stderr for logging */
		screen->inetdSock = fd;
		screen->port = 0;

	} else if (! got_rfbport && auto_port > 0) {
		int lport = find_free_port(auto_port, auto_port+200);
		screen->autoPort = FALSE;
		screen->port = lport;
	} else if (! got_rfbport) {
		screen->autoPort = TRUE;
	} else if (got_rfbport && got_rfbport_val == 0) {
		screen->autoPort = FALSE;
		screen->port = 0;
	}

	if (! got_nevershared && ! got_alwaysshared) {
		if (shared) {
			screen->alwaysShared = TRUE;
		} else {
			screen->neverShared = TRUE;
		}
		screen->dontDisconnect = TRUE;
	}
	if (! got_deferupdate) {
		screen->deferUpdateTime = defer_update;
	} else {
		defer_update = screen->deferUpdateTime;
	}

	if (noipv4 || getenv("IPV4_FAILS")) {
		rfbBool ap = screen->autoPort;
		int port = screen->port;

		if (getenv("IPV4_FAILS")) {
			rfbLog("TESTING: IPV4_FAILS for rfbInitServer()\n");
		}

		screen->autoPort = FALSE;
		screen->port = 0;

		rfbInitServer(screen);

		screen->autoPort = ap;
		screen->port = port;
		
	} else {
		rfbInitServer(screen);
	}

	if (use_openssl) {
		openssl_port(0);
		if (https_port_num >= 0) {
			https_port(0);
		}
	} else {
		if (ipv6_listen) {
			int fd = -1;
			if (screen->port <= 0) {
				if (got_rfbport) {
					screen->port = got_rfbport_val;
				} else {
					int ap = 5900;
					if (auto_port > 0) {
						ap = auto_port;
					}
					screen->port = find_free_port6(ap, ap+200);
				}
			}
			fd = listen6(screen->port);
			if (fd < 0) {
				ipv6_listen = 0;
				rfbLog("Not listening on IPv6 interface.\n");
			} else {
				rfbLog("Listening %s on IPv6 port %d (socket %d)\n",
				    screen->listenSock < 0 ? "only" : "also",
				    screen->port, fd);
				ipv6_listen_fd = fd;
			}
		}
	}

	install_passwds();

	if (locked_sends) {
		lock_client_sends(0);
	}
	return;
}

#define DO_AVAHI \
	if (avahi) { \
		avahi_initialise(); \
		avahi_advertise(vnc_desktop_name, host, lport); \
		usleep(1000*1000); \
	}

void announce(int lport, int ssl, char *iface) {
	
	char *host = this_host();
	char *tvdt;

	if (remote_direct) {
		return;
	}

	if (! ssl) {
		tvdt = "The VNC desktop is:     ";
	} else {
		if (enc_str && !strcmp(enc_str, "none")) {
			tvdt = "The VNC desktop is:     ";
		} else if (enc_str) {
			tvdt = "The ENC VNC desktop is: ";
		} else {
			tvdt = "The SSL VNC desktop is: ";
		}
	}

	if (iface != NULL && *iface != '\0' && strcmp(iface, "any")) {
		host = iface;
	}
	if (host != NULL) {
		/* note that vncviewer special cases 5900-5999 */
		int sz = 256;
		if (inetd) {
			;	/* should not occur (port) */
		} else if (quiet) {
			if (lport >= 5900) {
				snprintf(vnc_desktop_name, sz, "%s:%d",
				    host, lport - 5900);
				DO_AVAHI
				fprintf(stderr, "\n%s %s\n", tvdt,
				    vnc_desktop_name);
			} else {
				snprintf(vnc_desktop_name, sz, "%s:%d",
				    host, lport);
				DO_AVAHI
				fprintf(stderr, "\n%s %s\n", tvdt,
				    vnc_desktop_name);
			}
		} else if (lport >= 5900) {
			snprintf(vnc_desktop_name, sz, "%s:%d",
			    host, lport - 5900);
			DO_AVAHI
			fprintf(stderr, "\n%s %s\n", tvdt, vnc_desktop_name);
			if (lport >= 6000) {
				rfbLog("possible aliases:  %s:%d, "
				    "%s::%d\n", host, lport,
				    host, lport);
			}
		} else {
			snprintf(vnc_desktop_name, sz, "%s:%d",
			    host, lport);
			DO_AVAHI
			fprintf(stderr, "\n%s %s\n", tvdt, vnc_desktop_name);
			rfbLog("possible alias:    %s::%d\n",
			    host, lport);
		}
	}
}

static void announce_http(int lport, int ssl, char *iface, char *extra) {
	
	char *host = this_host();
	char *jvu;
	int http = 0;

	if (enc_str && !strcmp(enc_str, "none") && !use_stunnel) {
		jvu = "Java viewer URL:         http";
		http = 1;
	} else if (ssl == 1) {
		jvu = "Java SSL viewer URL:     https";
	} else if (ssl == 2) {
		jvu = "Java SSL viewer URL:     http";
		http = 1;
	} else {
		jvu = "Java viewer URL:         http";
		http = 1;
	}

	if (iface != NULL && *iface != '\0' && strcmp(iface, "any")) {
		host = iface;
	}
	if (http && getenv("X11VNC_HTTP_LISTEN_LOCALHOST")) {
		host = "localhost";
	}
	if (host != NULL) {
		if (! inetd) {
			fprintf(stderr, "%s://%s:%d/%s\n", jvu, host, lport, extra);
		}
	}
}

void do_announce_http(void) {
	if (!screen) {
		return;
	}
	if (remote_direct) {
		return;
	}

	/* XXX ipv6? */
	if ((screen->httpListenSock > -1 || ipv6_http_fd > -1) && screen->httpPort) {
		int enc_none = (enc_str && !strcmp(enc_str, "none"));
		char *SPORT = "   (single port)";
		if (use_openssl && ! enc_none) {
			announce_http(screen->port, 1, listen_str, SPORT);
			if (https_port_num >= 0) {
				announce_http(https_port_num, 1,
				    listen_str, "");
			}
			announce_http(screen->httpPort, 2, listen_str, "");
		} else if (use_stunnel) {
			char pmsg[100];
			pmsg[0] = '\0';
			if (stunnel_port) {
				sprintf(pmsg, "?PORT=%d", stunnel_port);
			}
			announce_http(screen->httpPort, 2, listen_str, pmsg);
			if (stunnel_http_port > 0) {
				announce_http(stunnel_http_port, 1, NULL, pmsg);
			}
			if (enc_none) {
				strcat(pmsg, SPORT);
				announce_http(stunnel_port, 1, NULL, pmsg);
			}
		} else {
			announce_http(screen->httpPort, 0, listen_str, "");
			if (enc_none) {
				announce_http(screen->port, 1, NULL, SPORT);
			}
		}
	}
}

void do_mention_java_urls(void) {
	if (! quiet && screen) {
		/* XXX ipv6? */
		if (screen->httpListenSock > -1 && screen->httpPort) {
			rfbLog("\n");
			rfbLog("The URLs printed out below ('Java ... viewer URL') can\n");
			rfbLog("be used for Java enabled Web browser connections.\n");
			if (!stunnel_port && enc_str && !strcmp(enc_str, "none")) {
				;
			} else if (use_openssl || stunnel_port) {
				rfbLog("Here are some additional possibilities:\n");
				rfbLog("\n");
				rfbLog("https://host:port/proxy.vnc (MUST be used if Web Proxy used)\n");
				rfbLog("\n");
				rfbLog("https://host:port/ultra.vnc (Use UltraVNC Java Viewer)\n");
				rfbLog("https://host:port/ultraproxy.vnc (Web Proxy with UltraVNC)\n");
				rfbLog("https://host:port/ultrasigned.vnc (Signed UltraVNC Filexfer)\n");
				rfbLog("\n");
				rfbLog("Where you replace \"host:port\" with that printed below, or\n");
				rfbLog("whatever is needed to reach the host e.g. Internet IP number\n");
				rfbLog("\n");
				rfbLog("Append ?GET=1 to a URL for faster loading or supply:\n");
				rfbLog("-env X11VNC_EXTRA_HTTPS_PARAMS='?GET=1' to cmdline.\n");
			}
		}
		rfbLog("\n");
	}
}

void set_vnc_desktop_name(void) {
	sprintf(vnc_desktop_name, "unknown");
	if (inetd) {
		sprintf(vnc_desktop_name, "%s/inetd-no-further-clients",
		    this_host());
	}
	if (remote_direct) {
		return;
	}
	if (screen->port) {

		do_mention_java_urls();

		if (use_openssl) {
			announce(screen->port, 1, listen_str);
		} else {
			announce(screen->port, 0, listen_str);
		}
		if (stunnel_port) {
			announce(stunnel_port, 1, NULL);
		}

		do_announce_http();
		
		fflush(stderr);	
		if (inetd) {
			;	/* should not occur (port != 0) */
		} else {
			fprintf(stdout, "PORT=%d\n", screen->port);
			if (stunnel_port) {
				fprintf(stdout, "SSLPORT=%d\n", stunnel_port);
			} else if (use_openssl) {
				if (enc_str && !strcmp(enc_str, "none")) {
					;
				} else if (enc_str) {
					fprintf(stdout, "ENCPORT=%d\n", screen->port);
				} else {
					fprintf(stdout, "SSLPORT=%d\n", screen->port);
				}
			}
			fflush(stdout);	
			if (flagfile) {
				FILE *flag = fopen(flagfile, "w");
				if (flag) {
					fprintf(flag, "PORT=%d\n",screen->port);
					if (stunnel_port) {
						fprintf(flag, "SSL_PORT=%d\n",
						    stunnel_port);
					}
					fflush(flag);	
					fclose(flag);
				} else {
					rfbLog("could not open flag file: %s\n",
					    flagfile);
				}
			}
			if (rm_flagfile) {
				int create = 0;
				struct stat sb;
				if (strstr(rm_flagfile, "create:") == rm_flagfile) {
					char *s = rm_flagfile;
					create = 1;
					rm_flagfile = strdup(rm_flagfile + strlen("create:"));
					free(s);
				}
				if (strstr(rm_flagfile, "nocreate:") == rm_flagfile) {
					char *s = rm_flagfile;
					create = 0;
					rm_flagfile = strdup(rm_flagfile + strlen("nocreate:"));
					free(s);
				} else if (stat(rm_flagfile, &sb) != 0) {
					create = 1;
				}
				if (create) {
					FILE *flag = fopen(rm_flagfile, "w");
					if (flag) {
						fprintf(flag, "%d\n", getpid());
						fclose(flag);
					}
				}
			}
		}
		fflush(stdout);	
	}
}

static void check_cursor_changes(void) {
	static double last_push = 0.0;

	if (unixpw_in_progress) return;

	cursor_changes += check_x11_pointer();

	if (cursor_changes) {
		double tm, max_push = 0.125, multi_push = 0.01, wait = 0.02;
		int cursor_shape, dopush = 0, link, latency, netrate;

		if (! all_clients_initialized()) {
			/* play it safe */
			return;
		}

		if (0) cursor_shape = cursor_shape_updates_clients(screen);
	
		dtime0(&tm);
		link = link_rate(&latency, &netrate);
		if (link == LR_DIALUP) {
			max_push = 0.2;
			wait = 0.05;
		} else if (link == LR_BROADBAND) {
			max_push = 0.075;
			wait = 0.05;
		} else if (link == LR_LAN) {
			max_push = 0.01;
		} else if (latency < 5 && netrate > 200) {
			max_push = 0.01;
		}
		
		if (tm > last_push + max_push) {
			dopush = 1;
		} else if (cursor_changes > 1 && tm > last_push + multi_push) {
			dopush = 1;
		}

		if (dopush) { 
			mark_rect_as_modified(0, 0, 1, 1, 1);
			fb_push_wait(wait, FB_MOD);
			last_push = tm;
		} else {
			rfbPE(0);
		}
	}
	cursor_changes = 0;
}

/*
 * These watch_loop() releated were moved from x11vnc.c so we could try
 * to remove -O2 from its compilation.  TDB new file, e.g. watch.c.
 */

static void check_filexfer(void) {
	static time_t last_check = 0;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;
	int transferring = 0; 
	
	if (time(NULL) <= last_check) {
		return;
	}

#if 0
	if (getenv("NOFT")) {
		return;
	}
#endif

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
		if (cl->fileTransfer.receiving) {
			transferring = 1;
			break;
		}
		if (cl->fileTransfer.sending) {
			transferring = 1;
			break;
		}
	}
	rfbReleaseClientIterator(iter);

	if (transferring) {
		double start = dnow();
		while (dnow() < start + 0.5) {
			rfbCFD(5000);
			rfbCFD(1000);
			rfbCFD(0);
		}
	} else {
		last_check = time(NULL);
	}
}

static void record_last_fb_update(void) {
	static int rbs0 = -1;
	static time_t last_call = 0;
	time_t now = time(NULL);
	int rbs = -1;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (last_fb_bytes_sent == 0) {
		last_fb_bytes_sent = now;
		last_call = now;
	}

	if (now <= last_call + 1) {
		/* check every second or so */
		return;
	}

	if (unixpw_in_progress) return;

	last_call = now;

	if (! screen) {
		return;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
#if 0
		rbs += cl->rawBytesEquivalent;
#else
#if LIBVNCSERVER_HAS_STATS
		rbs += rfbStatGetSentBytesIfRaw(cl);
#endif
#endif
	}
	rfbReleaseClientIterator(iter);

	if (rbs != rbs0) {
		rbs0 = rbs;
		if (debug_tiles > 1) {
			fprintf(stderr, "record_last_fb_update: %d %d\n",
			    (int) now, (int) last_fb_bytes_sent);
		}
		last_fb_bytes_sent = now;
	}
}


static int choose_delay(double dt) {
	static double t0 = 0.0, t1 = 0.0, t2 = 0.0, now; 
	static int x0, y0, x1, y1, x2, y2, first = 1;
	int dx0, dy0, dx1, dy1, dm, i, msec = waitms;
	double cut1 = 0.15, cut2 = 0.075, cut3 = 0.25;
	double bogdown_time = 0.25, bave = 0.0;
	int bogdown = 1, bcnt = 0;
	int ndt = 8, nave = 3;
	double fac = 1.0;
	static int db = 0, did_set_defer = 0;
	static double dts[8];
	static int link = LR_UNSET, latency = -1, netrate = -1;
	static double last_link = 0.0;

	if (screen && did_set_defer) {
		/* reset defer in case we changed it */
		screen->deferUpdateTime = defer_update;
	}
	if (waitms == 0) {
		return waitms;
	}
	if (nofb) {
		return waitms;
	}

	if (first) {
		for(i=0; i<ndt; i++) {
			dts[i] = 0.0;
		}
		if (getenv("DEBUG_DELAY")) {
			db = atoi(getenv("DEBUG_DELAY"));
		}
		if (getenv("SET_DEFER")) {
			set_defer = atoi(getenv("SET_DEFER"));
		}
		first = 0;
	}

	now = dnow();

	if (now > last_link + 30.0 || link == LR_UNSET) {
		link = link_rate(&latency, &netrate);
		last_link = now;
	}

	/*
	 * first check for bogdown, e.g. lots of activity, scrolling text
	 * from command output, etc.
	 */
	if (nap_ok) {
		dt = 0.0;
	}
	if (! wait_bog) {
		bogdown = 0;

	} else if (button_mask || now < last_keyboard_time + 2*bogdown_time) {
		/*
		 * let scrolls & keyboard input through the normal way
		 * otherwise, it will likely just annoy them.
		 */
		bogdown = 0;

	} else if (dt > 0.0) {
		/*
		 * inspect recent dt's:
		 * 0 1 2 3 4 5 6 7 dt
		 *             ^ ^ ^
		 */
		for (i = ndt - (nave - 1); i < ndt; i++) {
			bave += dts[i];
			bcnt++;
			if (dts[i] < bogdown_time) {
				bogdown = 0;
				break;
			}
		}
		bave += dt;
		bcnt++;
		bave = bave / bcnt;
		if (dt < bogdown_time) {
			bogdown = 0;
		}
	} else {
		bogdown = 0;
	}
	/* shift for next time */
	for (i = 0; i < ndt-1; i++) {
		dts[i] = dts[i+1];
	}
	dts[ndt-1] = dt;

if (0 && dt > 0.0) fprintf(stderr, "dt: %.5f %.4f\n", dt, dnowx());
	if (bogdown) {
		if (use_xdamage) {
			/* DAMAGE can queue ~1000 rectangles for a scroll */
			clear_xdamage_mark_region(NULL, 0);
		}
		msec = (int) (1000 * 1.75 * bave);
		if (dts[ndt - nave - 1] > 0.75 * bave) {
			msec = 1.5 * msec;
			set_xdamage_mark(0, 0, dpy_x, dpy_y);
		}
		if (msec > 1500) {
			msec = 1500;
		}
		if (msec < waitms) {
			msec = waitms;
		}
		db = (db || debug_tiles);
		if (db) fprintf(stderr, "bogg[%d] %.3f %.3f %.3f %.3f\n",
		    msec, dts[ndt-4], dts[ndt-3], dts[ndt-2], dts[ndt-1]);

		return msec;
	}

	/* next check for pointer motion, keystrokes, to speed up */
	t2 = dnow();
	x2 = cursor_x;
	y2 = cursor_y;

	dx0 = nabs(x1 - x0);
	dy0 = nabs(y1 - y0);
	dx1 = nabs(x2 - x1);
	dy1 = nabs(y2 - y1);

	/* bigger displacement for most recent dt: */
	if (dx1 > dy1) {
		dm = dx1;
	} else {
		dm = dy1;
	}

	if ((dx0 || dy0) && (dx1 || dy1)) {
		/* if mouse moved the previous two times: */
		if (t2 < t0 + cut1 || t2 < t1 + cut2 || dm > 20) {
			/*
			 * if within 0.15s(0) or 0.075s(1) or mouse
			 * moved > 20pixels, set and bump up the cut
			 * down factor.
			 */
			fac = wait_ui * 1.5;
		} else if ((dx1 || dy1) && dm > 40) {
			fac = wait_ui;
		} else {
			/* still 1.0? */
			if (db > 1) fprintf(stderr, "wait_ui: still 1.0\n");
		}
	} else if ((dx1 || dy1) && dm > 40) {
		/* if mouse moved > 40 last time: */
		fac = wait_ui;
	}

	if (fac == 1.0 && t2 < last_keyboard_time + cut3) {
		/* if typed in last 0.25s set wait_ui */
		fac = wait_ui;
	}
	if (fac != 1.0) {
		if (link == LR_LAN || latency <= 3) {
			fac *= 1.5;
		}
	}

	msec = (int) (((double) waitms) / fac);
	if (msec == 0) {
		msec = 1;
	}

	if (set_defer && fac != 1.0 && screen) {
		/* this is wait_ui mode, set defer to match wait: */
		if (set_defer >= 1) {
			screen->deferUpdateTime = msec;
		} else if (set_defer <= -1) {
			screen->deferUpdateTime = 0;
		}
		if (nabs(set_defer) == 2) {
			urgent_update = 1;
		}
		did_set_defer = 1;
	}

	x0 = x1;
	y0 = y1;
	t0 = t1;

	x1 = x2;
	y1 = y2;
	t1 = t2;

	if (db > 1) fprintf(stderr, "wait: %2d defer[%02d]: %2d\n", msec, defer_update, screen->deferUpdateTime);

	return msec;
}

/*
 * main x11vnc loop: polls, checks for events, iterate libvncserver, etc.
 */
void watch_loop(void) {
	int cnt = 0, tile_diffs = 0, skip_pe = 0, wait;
	double tm, dtr, dt = 0.0;
	time_t start = time(NULL);

	if (use_threads && !started_rfbRunEventLoop) {
		started_rfbRunEventLoop = 1;
		rfbRunEventLoop(screen, -1, TRUE);
	}

	while (1) {
		char msg[] = "new client: %s taking unixpw client off hold.\n";
		int skip_scan_for_updates = 0;

		got_user_input = 0;
		got_pointer_input = 0;
		got_local_pointer_input = 0;
		got_pointer_calls = 0;
		got_keyboard_input = 0;
		got_keyboard_calls = 0;
		urgent_update = 0;

		x11vnc_current = dnow();

		if (! use_threads) {
			dtime0(&tm);
			if (! skip_pe) {
				if (unixpw_in_progress) {
					rfbClientPtr cl = unixpw_client;
					if (cl && cl->onHold) {
						rfbLog(msg, cl->host);
						unixpw_client->onHold = FALSE;
					}
				} else {
					measure_send_rates(1);
				}

				unixpw_in_rfbPE = 1;

				/*
				 * do a few more since a key press may
				 * have induced a small change we want to
				 * see quickly (just 1 rfbPE will likely
				 * only process the subsequent "up" event)
				 */
				if (tm < last_keyboard_time + 0.20) {
					rfbPE(0);
					rfbPE(0);
					rfbPE(-1);
					rfbPE(0);
					rfbPE(0);
				} else {
					if (extra_fbur > 0) {
						int i;
						for (i=0; i < extra_fbur; i++) {
							rfbPE(0);
						}
					}
					rfbPE(-1);
				}
				if (x11vnc_current < last_new_client + 0.5) {
					urgent_update = 1;
				}

				unixpw_in_rfbPE = 0;

				if (unixpw_in_progress) {
					/* rfbPE loop until logged in. */
					skip_pe = 0;
					check_new_clients();
					continue;
				} else {
					measure_send_rates(0);
					fb_update_sent(NULL);
				}
			} else {
				if (unixpw_in_progress) {
					skip_pe = 0;
					check_new_clients();
					continue;
				}
			}
			dtr = dtime(&tm);

			if (! cursor_shape_updates) {
				/* undo any cursor shape requests */
				disable_cursor_shape_updates(screen);
			}
			if (screen && screen->clientHead) {
				int ret = check_user_input(dt, dtr, tile_diffs, &cnt);
				/* true: loop back for more input */
				if (ret == 2) {
					skip_pe = 1;
				}
				if (ret) {
					if (debug_scroll) fprintf(stderr, "watch_loop: LOOP-BACK: %d\n", ret);
					continue;
				}
			}
			/* watch for viewonly input piling up: */
			if ((got_pointer_calls > got_pointer_input) ||
			    (got_keyboard_calls > got_keyboard_input)) {
				eat_viewonly_input(10, 3);
			}
		} else {
			/* -threads here. */
			if (unixpw_in_progress) {
				rfbClientPtr cl = unixpw_client;
				if (cl && cl->onHold) {
					rfbLog(msg, cl->host);
					unixpw_client->onHold = FALSE;
				}
			}
			if (use_xrecord) {
				check_xrecord();
			}
			if (wireframe && button_mask) {
				check_wireframe();
			}
		}
		skip_pe = 0;

		if (shut_down) {
			clean_up_exit(0);
		}

		if (unixpw_in_progress) {
			check_new_clients();
			continue;
		}

		if (! urgent_update) {
			if (do_copy_screen) {
				do_copy_screen = 0;
				copy_screen();
			}

			check_new_clients();
			check_ncache(0, 0);
			check_xevents(0);
			check_autorepeat();
			check_pm();
			check_filexfer();
			check_keycode_state();
			check_connect_inputs();
			check_gui_inputs();
			check_stunnel();
			check_openssl();
			check_https();
			record_last_fb_update();
			check_padded_fb();
			check_fixscreen();
			check_xdamage_state();
			check_xrecord_reset(0);
			check_add_keysyms();
			check_new_passwds(0);
#ifdef ENABLE_GRABLOCAL
			if (grab_local) {
				check_local_grab();
			}
#endif
			if (started_as_root) {
				check_switched_user();
			}

			if (first_conn_timeout < 0) {
				start = time(NULL);
				first_conn_timeout = -first_conn_timeout;
			}
		}

		if (rawfb_vnc_reflect) {
			static time_t lastone = 0;
			if (time(NULL) > lastone + 10) {
				lastone = time(NULL);
				vnc_reflect_process_client();
			}
		}

		if (first_conn_timeout) {
			int t = first_conn_timeout;
			if (!clients_served) {
				if (time(NULL) - start > first_conn_timeout) {
					rfbLog("No client after %d secs.\n", t);
					shut_down = 1;
				}
			} else {
				if (!client_normal_count) {
					if (time(NULL) - start > t + 3) {
						rfbLog("No valid client after %d secs.\n", t + 3);
						shut_down = 1;
					}
				}
			}
		}

		if (! screen || ! screen->clientHead) {
			/* waiting for a client */
			usleep(200 * 1000);
			continue;
		}

		if (first_conn_timeout && all_clients_initialized()) {
			first_conn_timeout = 0;
		}

		if (nofb) {
			/* no framebuffer polling needed */
			if (cursor_pos_updates) {
				check_x11_pointer();
			}
#ifdef MACOSX
			else check_x11_pointer();
#endif
			continue;
		}
		if (x11vnc_current < last_new_client + 0.5 && !all_clients_initialized()) {
			continue;
		}
		if (subwin && freeze_when_obscured) {
			/* XXX not working */
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
			check_xevents(0);
			if (subwin_obscured) {
				skip_scan_for_updates = 1;
			}
		}

		if (skip_scan_for_updates) {
			;
		} else if (button_mask && (!show_dragging || pointer_mode == 0)) {
			/*
			 * if any button is pressed in this mode do
			 * not update rfb screen, but do flush the
			 * X11 display.
			 */
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
			dt = 0.0;
		} else {
			static double last_dt = 0.0;
			double xdamage_thrash = 0.4; 
			static int tilecut = -1;

			check_cursor_changes();

			/* for timing the scan to try to detect thrashing */

			if (use_xdamage && last_dt > xdamage_thrash)  {
				clear_xdamage_mark_region(NULL, 0);
			}

			if (unixpw_in_progress) continue;

			if (rawfb_vnc_reflect) {
				vnc_reflect_process_client();
			}

			dtime0(&tm);

#if !NO_X11
			if (xrandr_present && !xrandr && xrandr_maybe) {
				int delay = 180;
				/*  there may be xrandr right after xsession start */
				if (tm < x11vnc_start + delay || tm < last_client + delay) {
					int tw = 20;
					if (auth_file != NULL) {
						tw = 120;
					}
					X_LOCK;
					if (tm < x11vnc_start + tw || tm < last_client + tw) {
						XSync(dpy, False);
					} else {
						XFlush_wr(dpy);
					}
					X_UNLOCK;
				}
				X_LOCK;
				check_xrandr_event("before-scan");
				X_UNLOCK;
			}
#endif
			if (use_snapfb) {
				int t, tries = 3;
				copy_snap();
				for (t=0; t < tries; t++) {
					tile_diffs = scan_for_updates(0);
				}
			} else {
				tile_diffs = scan_for_updates(0);
			}
			dt = dtime(&tm);
			if (! nap_ok) {
				last_dt = dt;
			}

			if (tilecut < 0) {
				if (getenv("TILECUT")) {
					tilecut = atoi(getenv("TILECUT"));
				}
				if (tilecut < 0) tilecut = 4;
			}

			if ((debug_tiles || debug_scroll > 1 || debug_wireframe > 1)
			    && (tile_diffs > tilecut || debug_tiles > 1)) {
				double rate = (tile_x * tile_y * bpp/8 * tile_diffs) / dt;
				fprintf(stderr, "============================= TILES: %d  dt: %.4f"
				    "  t: %.4f  %.2f MB/s nap_ok: %d\n", tile_diffs, dt,
				    tm - x11vnc_start, rate/1000000.0, nap_ok);
			}

		}

		/* sleep a bit to lessen load */
		wait = choose_delay(dt);

		if (urgent_update) {
			;
		} else if (wait > 2*waitms) {
			/* bog case, break it up */
			nap_sleep(wait, 10);
		} else {
			double t1, t2;
			int idt;
			if (extra_fbur > 0) {
				int i;
				for (i=0; i <= extra_fbur; i++) {
					int r = rfbPE(0);
					if (!r) break;
				}
			}

			/* sometimes the sleep is too short, so measure it: */
			t1 = dnow();
			usleep(wait * 1000);
			t2 = dnow();

			idt = (int) (1000. * (t2 - t1));
			if (idt > 0 && idt < wait) {
				/* try to sleep the remainder */
				usleep((wait - idt) * 1000);
			}
		}

		cnt++;
	}
}


