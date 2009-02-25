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

void set_greyscale_colormap(void);
void set_hi240_colormap(void);
void unset_colormap(void);
void set_colormap(int reset);
void set_nofb_params(int restore);
void set_raw_fb_params(int restore);
void do_new_fb(int reset_mem);
void free_old_fb(char *old_main, char *old_rfb, char *old_8to24, char *old_snap_fb);
void check_padded_fb(void);
void install_padded_fb(char *geom);
XImage *initialize_xdisplay_fb(void);
void parse_scale_string(char *str, double *factor, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom);
int scale_round(int len, double fac);
void initialize_screen(int *argc, char **argv, XImage *fb);
void set_vnc_desktop_name(void);


static void debug_colormap(XImage *fb);
static void set_visual(char *str);
static void nofb_hook(rfbClientPtr cl);
static void remove_fake_fb(void);
static void install_fake_fb(int w, int h, int bpp);
static void initialize_snap_fb(void);
XImage *initialize_raw_fb(int);
static void initialize_clipshift(void);
static int wait_until_mapped(Window win);
static void announce(int lport, int ssl, char *iface);
static void setup_scaling(int *width_in, int *height_in);

int rawfb_reset = -1;
int rawfb_dev_video = 0;

/*
 * X11 and rfb display/screen related routines
 */

/*
 * Some handling of 8bpp PseudoColor colormaps.  Called for initializing
 * the clients and dynamically if -flashcmap is specified.
 */
#define NCOLOR 256

void set_greyscale_colormap(void) {
	int i;
	if (! screen) {
		return;
	}
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

void set_colormap(int reset) {
	static int init = 1;
	static XColor color[NCOLOR], prev[NCOLOR];
	Colormap cmap;
	Visual *vis;
	int i, ncells, diffs = 0;

	if (reset) {
		init = 1;
		if (screen->colourMap.data.shorts) {
			free(screen->colourMap.data.shorts);
			screen->colourMap.data.shorts = NULL;
		}
	}
if (0) fprintf(stderr, "unset_colormap: %d\n", reset);

	if (init) {
		screen->colourMap.count = NCOLOR;
		screen->serverFormat.trueColour = FALSE;
		screen->colourMap.is16 = TRUE;
		screen->colourMap.data.shorts = (unsigned short *)
			malloc(3*sizeof(unsigned short) * NCOLOR);
	}

	for (i=0; i < NCOLOR; i++) {
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

	if (ncells != NCOLOR) {
		if (init && ! quiet) {
			rfbLog("set_colormap: number of cells is %d "
			    "instead of %d.\n", ncells, NCOLOR);
		}
		if (! shift_cmap) {
			screen->colourMap.count = ncells;
		}
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
	if (ncells > NCOLOR && ! quiet) {
		rfbLog("set_colormap: big problem: ncells=%d > %d\n",
		    ncells, NCOLOR);
	}

	if (vis->class == TrueColor || vis->class == DirectColor) {
		/*
		 * Kludge to make 8bpp TrueColor & DirectColor be like
		 * the StaticColor map.  The ncells = 8 is "8 per subfield"
		 * mentioned in xdpyinfo.  Looks OK... perhaps fortuitously.
		 */
		if (ncells == 8 && ! shift_cmap) {
			ncells = NCOLOR;
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

		if (shift_cmap && k >= 0 && k < NCOLOR) {
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
			rfbSetClientColourMaps(screen, 0, NCOLOR);
		} else {
			rfbSetClientColourMaps(screen, 0, ncells);
		}
	}

	init = 0;
}

static void debug_colormap(XImage *fb) {
	static int debug_cmap = -1;
	int i, k, histo[NCOLOR];

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
	if (fb->bits_per_pixel > 8) {
		return;
	}

	for (i=0; i < NCOLOR; i++) {
		histo[i] = 0;
	}
	for (k = 0; k < fb->width * fb->height; k++) {
		unsigned char n;
		char c = *(fb->data + k);

		n = (unsigned char) c;
		histo[n]++;
	}
	fprintf(stderr, "\nColormap histogram for current screen contents:\n");
	for (i=0; i < NCOLOR; i++) {
		unsigned short r = screen->colourMap.data.shorts[i*3+0];
		unsigned short g = screen->colourMap.data.shorts[i*3+1];
		unsigned short b = screen->colourMap.data.shorts[i*3+2];

		fprintf(stderr, "   %03d: %7d %04x/%04x/%04x", i, histo[i],
		    r, g, b);
		if ((i+1) % 2 == 0)  {
			fprintf(stderr, "\n");
		}
	}
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
}

void set_nofb_params(int restore) {
	static int first = 1;
	static int save[100];
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

	if (! quiet) {
		rfbLog("disabling: xfixes, xdamage, solid, overlay, shm,\n");
		rfbLog("  wireframe, scrollcopyrect,\n");
		rfbLog("  noonetile, nap, cursor, %scursorshape\n",
		    got_cursorpos ? "" : "cursorpos, " );
		rfbLog("  in -nofb mode.\n");
	}
}

static char *raw_fb_orig_dpy = NULL;

void set_raw_fb_params(int restore) {
	static int first = 1;
	static int vo0, us0, sm0, ws0, wp0, wc0, wb0, na0, tn0;  
	static int xr0, sb0, re0;
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
		noxrecord = re0;
		multiple_cursors_mode = mc0;

		if (! dpy && raw_fb_orig_dpy) {
			dpy = XOpenDisplay_wr(raw_fb_orig_dpy);
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

	if (! quiet) {
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
		if (! quiet) {
		   rfbLog("rawfb: -noviewonly mode: still sending mouse and\n");
		   rfbLog("rawfb:   keyboard input to the X DISPLAY!!\n");
		}
	} else {
		/* Normal case: */
		if (! view_only) {
			if (! quiet) rfbLog("  rawfb: setting view_only\n");
			view_only = 1;
		}
		if (watch_selection) {
			if (! quiet) rfbLog("  rawfb: turning off "
			    "watch_selection\n");
			watch_selection = 0;
		}
		if (watch_primary) {
			if (! quiet) rfbLog("  rawfb: turning off "
			    "watch_primary\n");
			watch_primary = 0;
		}
		if (watch_clipboard) {
			if (! quiet) rfbLog("  rawfb: turning off "
			    "watch_clipboard\n");
			watch_clipboard = 0;
		}
		if (watch_bell) {
			if (! quiet) rfbLog("  rawfb: turning off watch_bell\n");
			watch_bell = 0;
		}
		if (no_autorepeat) {
			if (! quiet) rfbLog("  rawfb: turning off "
			    "no_autorepeat\n");
			no_autorepeat = 0;
		}
		if (use_solid_bg) {
			if (! quiet) rfbLog("  rawfb: turning off "
			    "use_solid_bg\n");
			use_solid_bg = 0;
		}
		multiple_cursors_mode = strdup("arrow");
	}
	if (0 && use_snapfb) {
		if (! quiet) rfbLog("  rawfb: turning off use_snapfb\n");
		use_snapfb = 0;
	}
	if (using_shm) {
		if (! quiet) rfbLog("  rawfb: turning off using_shm\n");
		using_shm = 0;
	}
	if (take_naps) {
		if (! quiet) rfbLog("  rawfb: turning off take_naps\n");
		take_naps = 0;
	}
	if (xrandr) {
		if (! quiet) rfbLog("  rawfb: turning off xrandr\n");
		xrandr = 0;
	}
	if (! noxrecord) {
		if (! quiet) rfbLog("  rawfb: turning off xrecord\n");
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

	rfbLog("framebuffer requested in -nofb mode by client %s\n", cl->host);
	/* ignore xrandr */
	RAWFB_RET_VOID
	fb = XGetImage_wr(dpy, window, 0, 0, dpy_x, dpy_y, AllPlanes, ZPixmap);
	main_fb = fb->data;
	rfb_fb = main_fb;
	screen->frameBuffer = rfb_fb;
	screen->displayHook = NULL;
}

void free_old_fb(char *old_main, char *old_rfb, char *old_8to24, char *old_snap_fb) {
	if (old_main) {
		free(old_main);
	}
	if (old_rfb) {
		if (old_rfb != old_main) {
			free(old_rfb);
		}
	}
	if (old_8to24) {
		if (old_8to24 != old_main && old_8to24 != old_rfb) {
			free(old_8to24);
		}
	}
	if (old_snap_fb) {
		if (old_snap_fb != old_main && old_snap_fb != old_rfb &&
		    old_snap_fb != old_8to24) {
			free(old_snap_fb);
		}
	}
}

void do_new_fb(int reset_mem) {
	XImage *fb;

	/* for threaded we really should lock libvncserver out. */
	if (use_threads) {
		rfbLog("warning: changing framebuffers while threaded may\n");
		rfbLog(" not work, do not use -threads if problems arise.\n");
	}

	if (reset_mem == 1) {
		/* reset_mem == 2 is a hack for changing users... */
		clean_shm(0);
		free_tiles();
	}

	free_old_fb(main_fb, rfb_fb, cmap8to24_fb, snap_fb);

	if (raw_fb == main_fb || raw_fb == rfb_fb) {
		raw_fb = NULL;
	}
	main_fb = NULL;
	rfb_fb = NULL;
	cmap8to24_fb = NULL;
	snap_fb = NULL;

	fb = initialize_xdisplay_fb();

	initialize_screen(NULL, NULL, fb);

	if (reset_mem) {
		initialize_tiles();
		initialize_blackouts_and_xinerama();
		initialize_polling_images();
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

static void install_fake_fb(int w, int h, int bpp) {
	int bpc;
	if (! screen) {
		return;
	}
	if (fake_fb) {
		free(fake_fb);
	}
	fake_fb = (char *) calloc(w*h*bpp/8, 1);
	if (! fake_fb) {
		rfbLog("could not create fake fb: %dx%d %d\n", w, h, bpp);
		return;
	}
	bpc = guess_bits_per_color(bpp);
	rfbLog("installing fake fb: %dx%d %d\n", w, h, bpp);
	rfbLog("rfbNewFramebuffer(0x%x, 0x%x, %d, %d, %d, %d, %d)\n",
	    screen, fake_fb, w, h, bpc, 1, bpp/8);

	rfbNewFramebuffer(screen, fake_fb, w, h, bpc, 1, bpp/8);
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

#define RAWFB_MMAP 1
#define RAWFB_FILE 2
#define RAWFB_SHM  3

XImage *initialize_raw_fb(int reset) {
	char *str, *q;
	int w, h, b, shmid = 0;
	unsigned long rm = 0, gm = 0, bm = 0, tm;
	static XImage ximage_struct;	/* n.b.: not (XImage *) */
	static XImage ximage_struct_snap;
	int closedpy = 1, i, m, db = 0;

	static char *last_file = NULL;
	static int last_mode = 0;

	if (reset && last_mode) {
		int fd;
		if (last_mode != RAWFB_MMAP && last_mode != RAWFB_FILE) {
			return NULL;
		}
		if (last_mode == RAWFB_MMAP) {
			munmap(raw_fb_addr, raw_fb_mmap);
		}
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
		return NULL;
	}
	
	if (raw_fb_addr || raw_fb_seek) {
		if (raw_fb_shm) {
			shmdt(raw_fb_addr);
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
	if (!strcasecmp(raw_fb_str, "NULL") || !strcasecmp(raw_fb_str, "ZERO")) {
		raw_fb_str = strdup("map:/dev/zero@640x480x32");
	}
	if (!strcasecmp(raw_fb_str, "RAND")) {
		raw_fb_str = strdup("file:/dev/urandom@128x128x16");
	}

	if ( (q = strstr(raw_fb_str, "setup:")) == raw_fb_str) {
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
		str = strdup(raw_fb_str);
	}
	if (str[0] == '+') {
		char *t = strdup(str+1);
		free(str);
		str = t;
		closedpy = 0;
		if (! window) {
			window = rootwin;
		}
	}

	raw_fb_shm = 0;
	raw_fb_mmap = 0;
	raw_fb_seek = 0;
	raw_fb_fd = -1;
	raw_fb_addr = NULL;
	raw_fb_offset = 0;

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
	} else if (strstr(str, "cons") == str || strstr(str, "/dev/fb") == str) {
		char *str2 = console_guess(str, &raw_fb_fd);
		if (str2 == NULL) {
			rfbLog("console_guess failed for: %s\n", str);
			clean_up_exit(1);
		}
		str = str2;
		rfbLog("console_guess returned: %s\n", str);
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
	/* @WxHxB */
	if (sscanf(q, "@%dx%dx%d", &w, &h, &b) != 3) {
		rfbLogEnable(1);
		rfbLog("invalid rawfb str: %s\n", str);
		clean_up_exit(1);
	}
	*q = '\0';

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
			char *new;
			int len = strlen("map:") + strlen(str) + 1;
			rfbLog("no type prefix: %s\n", raw_fb_str);
			rfbLog("  but file exists, so assuming: map:%s\n",
			    raw_fb_str);
			new = (char *) malloc(len);
			strcpy(new, "map:");
			strcat(new, str);
			free(str);
			str = new;
		}
	}

	if (sscanf(str, "shm:%d", &shmid) == 1) {
		/* shm:N */
#if LIBVNCSERVER_HAVE_XSHM
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

		last_file = strdup(q);

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
			clean_up_exit(1);
		}
		raw_fb_fd = fd;

		if (xform24to32) {
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

		if (do_mmap) {
#if LIBVNCSERVER_HAVE_MMAP
			raw_fb_addr = mmap(0, size, PROT_READ, MAP_SHARED,
			    fd, 0);

			if (raw_fb_addr == MAP_FAILED || raw_fb_addr == NULL) {
				rfbLogEnable(1);
				rfbLog("failed to mmap file: %s, %s\n", q, str);
				rfbLog("   raw_fb_addr: %p\n", raw_fb_addr);
				rfbLogPerror("mmap");
				clean_up_exit(1);
			}
			raw_fb_mmap = size;

			rfbLog("rawfb: mmap file: %s\n", q);
			rfbLog("   w: %d h: %d b: %d addr: %p sz: %d\n", w, h,
			    b, raw_fb_addr, size);
			last_mode = RAWFB_MMAP;
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

	if (! raw_fb_image) {
		raw_fb_image = &ximage_struct;
	}

	initialize_clipshift();

	raw_fb = (char *) malloc(dpy_x * dpy_y * b/8);
	raw_fb_image->data = raw_fb;
	raw_fb_image->format = ZPixmap;
	raw_fb_image->width  = dpy_x;
	raw_fb_image->height = dpy_y;
	raw_fb_image->bits_per_pixel = b;
	raw_fb_image->bytes_per_line = dpy_x*b/8;
	raw_fb_image->bitmap_unit = -1;

	if (use_snapfb && (raw_fb_seek || raw_fb_mmap)) {
		int b_use = b;
		if (snap_fb) {
			free(snap_fb);
		}
		if (b_use == 32 && xform24to32) {
			b_use = 24;
		}
		snap_fb = (char *) malloc(dpy_x * dpy_y * b_use/8);
		snap = &ximage_struct_snap;
		snap->data = snap_fb;
		snap->format = ZPixmap;
		snap->width  = dpy_x;
		snap->height = dpy_y;
		snap->bits_per_pixel = b_use;
		snap->bytes_per_line = dpy_x*b_use/8;
		snap->bitmap_unit = -1;
	}

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
	if (! raw_fb_image->depth) { 
		raw_fb_image->depth = (b == 32) ? 24 : b;
	}

	depth = raw_fb_image->depth;

	if (raw_fb_image->depth == 15) {
		/* unresolved bug with RGB555... */
		depth++;
	}

	if (clipshift) {
		memset(raw_fb, 0xff, dpy_x * dpy_y * b/8);
	} else if (raw_fb_addr && ! xform24to32) {
		memcpy(raw_fb, raw_fb_addr + raw_fb_offset, dpy_x*dpy_y*b/8);
	} else {
		memset(raw_fb, 0xff, dpy_x * dpy_y * b/8);
	}

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
			rfbLog("skipping invalid -clip WxH+X+Y: %s\n",
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
}

/*
 * initialize a fb for the X display
 */
XImage *initialize_xdisplay_fb(void) {
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
			rfbLog("initialize_xdisplay_fb: vis_str set to: %s ",
			    vis_str);
			visual_id = (VisualID) 0;
			visual_depth = 0;
			set_visual_str_to_something = 1;
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
		XFree(vinfo);
	}

	if (! quiet) {
		rfbLog("Default visual ID: 0x%x\n",
		    (int) XVisualIDFromVisual(default_visual));
	}

	again:
	if (subwin) {
		int shift = 0;
		int subwin_x, subwin_y;
		int disp_x = DisplayWidth(dpy, scr);
		int disp_y = DisplayHeight(dpy, scr);
		Window twin;
		/* subwins can be a dicey if they are changing size... */
		trapped_xerror = 0;
		old_handler = XSetErrorHandler(trap_xerror);
		XTranslateCoordinates(dpy, window, rootwin, 0, 0, &subwin_x,
		    &subwin_y, &twin);

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

		if (shift) {
			XMoveWindow(dpy, window, subwin_x, subwin_y);
		}
		XMapRaised(dpy, window);
		XRaiseWindow(dpy, window);
		XFlush_wr(dpy);
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
		fb->data = (char *) malloc(fb->bytes_per_line * fb->height);

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
		if (trapped_xerror) {
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

	} else if (! fb && try == 1) {
		/* try once more */
		usleep(250 * 1000);
		goto again;
	}
	if (use_snapfb) {
		initialize_snap_fb();
	}
	X_UNLOCK;

	if (fb->bits_per_pixel == 24 && ! quiet) {
		rfbLog("warning: 24 bpp may have poor performance.\n");
	}
	return fb;
}

void parse_scale_string(char *str, double *factor, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom) {

	int m, n;
	char *p, *tstr;
	double f;

	*factor = 1.0;
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
		if (sscanf(tstr, "%lf", &f) != 1) {
			rfbLogEnable(1);
			rfbLog("invalid -scale arg: %s\n", tstr);
			clean_up_exit(1);
		}
		*factor = (double) f;
		/* look for common fractions from small ints: */
		for (n=2; n<=10; n++) {
			for (m=1; m<n; m++) {
				test = ((double) m)/ n;
				diff = *factor - test;
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
		if (*factor < 0.01) {
			rfbLogEnable(1);
			rfbLog("-scale factor too small: %f\n", scale_fac);
			clean_up_exit(1);
		}
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
		*factor = ((double) m)/ n;
		if (*factor < 0.01) {
			rfbLogEnable(1);
			rfbLog("-scale factor too small: %f\n", *factor);
			clean_up_exit(1);
		}
		*numer = m;
		*denom = n;
	}
	if (*factor == 1.0) {
		if (! quiet) {
			rfbLog("scaling disabled for factor %f\n", *factor);
		}
	} else {
		*scaling = 1;
	}
	free(tstr);
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

	parse_scale_string(scale_str, &scale_fac, &scaling, &scaling_blend,
	    &scaling_nomult4, &scaling_pad, &scaling_interpolate,
	    &scale_numer, &scale_denom);

	if (scaling) {
		width  = scale_round(width,  scale_fac);
		height = scale_round(height, scale_fac);
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
		} else if (depth == 8) {
			/* need to cook up the screen fb to be depth 24 */
			fb_bpp = 32;
			fb_depth = 24;
		} else {
			if (!quiet) rfbLog("disabling -8to24 mode:"
			    " default depth not 24 or 8\n");
			cmap8to24 = 0;
		}
	}

	setup_scaling(&width, &height);

	if (scaling) {
		rfbLog("scaling screen: %dx%d -> %dx%d  scale_fac=%.5f\n",
		    fb->width, fb->height, scaled_x, scaled_y, scale_fac);

		rfb_bytes_per_line = (main_bytes_per_line / fb->width) * width;
	} else {
		rfb_bytes_per_line = main_bytes_per_line;
	}

	if (cmap8to24 && depth == 8) {
		rfb_bytes_per_line *= 4;
	}

	/*
	 * These are just hints wrt pixel format just to let
	 * rfbGetScreen/rfbNewFramebuffer proceed with reasonable
	 * defaults.  We manually set them in painful detail below.
	 */
	bits_per_color = guess_bits_per_color(fb_bpp);

	/* n.b. samplesPerPixel (set = 1 here) seems to be unused. */
	if (create_screen) {
		if (use_openssl) {
			openssl_init();
		} else if (use_stunnel) {
			setup_stunnel(0, argc, argv);
		}
		screen = rfbGetScreen(argc, argv, width, height,
		    bits_per_color, 1, fb_bpp/8);
		if (screen && http_dir) {
			http_connections(1);
		}
	} else {
		/* set set frameBuffer member below. */
		rfbLog("rfbNewFramebuffer(0x%x, 0x%x, %d, %d, %d, %d, %d)\n",
		    screen, NULL, width, height, bits_per_color, 1, fb_bpp/8);

		/* these are probably overwritten, but just to be safe: */
		screen->bitsPerPixel = fb_bpp;
		screen->depth = fb_depth;

		rfbNewFramebuffer(screen, NULL, width, height,
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
	screen->paddedWidthInBytes = rfb_bytes_per_line;
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

	if (cmap8to24 && depth == 8 && dpy) {
		XVisualInfo vinfo;
		/* more cooking up... */
		have_masks = 2;
		/* need to fetch TrueColor visual */
		X_LOCK;
		if (dpy && XMatchVisualInfo(dpy, scr, 24, TrueColor, &vinfo)) {
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

	if (! have_masks && screen->serverFormat.bitsPerPixel == 8
	    && dpy && CellsOfScreen(ScreenOfDisplay(dpy, scr))) {
		/* indexed color */
		if (!quiet) {
			rfbLog("\n");
			rfbLog("X display %s is 8bpp indexed color\n",
			    DisplayString(dpy));
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
	if (!quiet) {
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
		screen->displayHook = nofb_hook;
	} else {
		main_fb = fb->data;
		rfb_fb = NULL;
		cmap8to24_fb = NULL;

		if (cmap8to24) {
			int n = main_bytes_per_line * fb->height;
			if (depth == 8) {
				n *= 4;
			}
			cmap8to24_fb = (char *) malloc(n);
			memset(cmap8to24_fb, 0, n);
		}
		if (scaling) {
			rfb_fb = (char *) malloc(rfb_bytes_per_line * height);
			memset(rfb_fb, 0, rfb_bytes_per_line * height);
		} else if (cmap8to24) {
			rfb_fb = cmap8to24_fb;	
		} else {
			rfb_fb = main_fb;
		}
	}
	screen->frameBuffer = rfb_fb;
	if (!quiet) {
		fprintf(stderr, " rfb_fb:      %p\n", rfb_fb);
		fprintf(stderr, " main_fb:     %p\n", main_fb);
		fprintf(stderr, " 8to24_fb:    %p\n", cmap8to24_fb);
		fprintf(stderr, " snap_fb:     %p\n", snap_fb);
		fprintf(stderr, " raw_fb:      %p\n", raw_fb);
		fprintf(stderr, " fake_fb:     %p\n", fake_fb);
		fprintf(stderr, "\n");
	}

	/* may need, bpp, main_red_max, etc. */
	parse_wireframe();
	parse_scroll_copyrect();

	setup_cursors_and_push();

	if (scaling || cmap8to24) {
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
		do_copy_screen = 1;
		
		/* done for framebuffer change case */
		return;
	}

	/*
	 * the rest is screen server initialization, etc, only needed
	 * at screen creation time.
	 */

	/* event callbacks: */
	screen->newClientHook = new_client;
	screen->kbdAddEvent = keyboard;
	screen->ptrAddEvent = pointer;
	screen->setXCutText = xcut_receive;

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

	} else if (! got_rfbport) {
		screen->autoPort = TRUE;
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
	}

	rfbInitServer(screen);

	if (use_openssl) {
		openssl_port();
		if (https_port_num >= 0) {
			https_port();
		}
	}

	install_passwds();
}

static void announce(int lport, int ssl, char *iface) {
	
	char *host = this_host();
	char *tvdt;

	if (! ssl) {
		tvdt = "The VNC desktop is:     ";
	} else {
		tvdt = "The SSL VNC desktop is: ";
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
				fprintf(stderr, "%s %s\n", tvdt,
				    vnc_desktop_name);
			} else {
				snprintf(vnc_desktop_name, sz, "%s:%d",
				    host, lport);
				fprintf(stderr, "%s %s\n", tvdt,
				    vnc_desktop_name);
			}
		} else if (lport >= 5900) {
			snprintf(vnc_desktop_name, sz, "%s:%d",
			    host, lport - 5900);
			fprintf(stderr, "%s %s\n", tvdt, vnc_desktop_name);
			if (lport >= 6000) {
				rfbLog("possible aliases:  %s:%d, "
				    "%s::%d\n", host, lport,
				    host, lport);
			}
		} else {
			snprintf(vnc_desktop_name, sz, "%s:%d",
			    host, lport);
			fprintf(stderr, "%s %s\n", tvdt, vnc_desktop_name);
			rfbLog("possible alias:    %s::%d\n",
			    host, lport);
		}
	}
}

static void announce_http(int lport, int ssl, char *iface) {
	
	char *host = this_host();
	char *jvu;

	if (ssl == 1) {
		jvu = "Java SSL viewer URL:     https";
	} else if (ssl == 2) {
		jvu = "Java SSL viewer URL:     http";
	} else {
		jvu = "Java viewer URL:         http";
	}

	if (iface != NULL && *iface != '\0' && strcmp(iface, "any")) {
		host = iface;
	}
	if (host != NULL) {
		if (! inetd) {
			fprintf(stderr, "%s://%s:%d/\n", jvu, host, lport);
		}
	}
}

void set_vnc_desktop_name(void) {
	sprintf(vnc_desktop_name, "unknown");
	if (inetd) {
		sprintf(vnc_desktop_name, "%s/inetd-no-further-clients",
		    this_host());
	}
	if (screen->port) {

		if (! quiet) {
			rfbLog("\n");
		}

		if (use_openssl) {
			announce(screen->port, 1, listen_str);
		} else {
			announce(screen->port, 0, listen_str);
		}
		if (stunnel_port) {
			announce(stunnel_port, 1, NULL);
		}
		if (screen->httpListenSock > -1 && screen->httpPort) {
			if (use_openssl) {
				announce_http(screen->port, 1, listen_str);
				if (https_port_num >= 0) {
					announce_http(https_port_num, 1,
					    listen_str);
				}
				announce_http(screen->httpPort, 2, listen_str);
			} else if (use_stunnel) {
				announce_http(screen->httpPort, 2, listen_str);
			} else {
				announce_http(screen->httpPort, 0, listen_str);
			}
		}
		
		fflush(stderr);	
		if (inetd) {
			;	/* should not occur (port != 0) */
		} else {
			fprintf(stdout, "PORT=%d\n", screen->port);
			if (stunnel_port) {
				fprintf(stdout, "SSLPORT=%d\n", stunnel_port);
			} else if (use_openssl) {
				fprintf(stdout, "SSLPORT=%d\n", screen->port);
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
		}
		fflush(stdout);	
	}
}


