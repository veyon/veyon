/* -- xinerama.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "blackout_t.h"
#include "scan.h"

/*
 * routines related to xinerama and blacking out rectangles
 */

/* blacked-out region (-blackout, -xinerama) */

#define BLACKR_MAX 100
blackout_t blackr[BLACKR_MAX];	/* hardwired max blackouts */
tile_blackout_t *tile_blackout;
int blackouts = 0;

void initialize_blackouts_and_xinerama(void);
void push_sleep(int n);
void push_black_screen(int n);
void refresh_screen(int push);
void zero_fb(int x1, int y1, int x2, int y2);


static void initialize_blackouts(char *list);
static void blackout_tiles(void);
static void initialize_xinerama (void);


/*
 * Take a comma separated list of geometries: WxH+X+Y and register them as
 * rectangles to black out from the screen.
 */
static void initialize_blackouts(char *list) {
	char *p, *blist = strdup(list);
	int x, y, X, Y, h, w, t;

	p = strtok(blist, ", \t");
	while (p) {
		if (!strcmp("noptr", p)) {
			blackout_ptr = 1;
			rfbLog("pointer will be blocked from blackout "
			    "regions\n");
			p = strtok(NULL, ", \t");
			continue;
		}
		if (! parse_geom(p, &w, &h, &x, &y, dpy_x, dpy_y)) {
			if (*p != '\0') {
				rfbLog("skipping invalid geometry: %s\n", p);
			}
			p = strtok(NULL, ", \t");
			continue;
		}
		w = nabs(w);
		h = nabs(h);
		x = nfix(x, dpy_x);
		y = nfix(y, dpy_y);
		X = x + w;
		Y = y + h;
		X = nfix(X, dpy_x+1);
		Y = nfix(Y, dpy_y+1);
		if (x > X) {
			t = X; X = x; x = t;
		}
		if (y > Y) {
			t = Y; Y = y; y = t;
		}
		if (x < 0 || x > dpy_x || y < 0 || y > dpy_y ||
		    X < 0 || X > dpy_x || Y < 0 || Y > dpy_y ||
		    x == X || y == Y) {
			rfbLog("skipping invalid blackout geometry: %s x="
			    "%d-%d,y=%d-%d,w=%d,h=%d\n", p, x, X, y, Y, w, h);
		} else {
			rfbLog("blackout rect: %s: x=%d-%d y=%d-%d\n", p,
			    x, X, y, Y);

			/*
			 * note that the black out is x1 <= x but x < x2
			 * for the region. i.e. the x2, y2 are outside
			 * by 1 pixel. 
			 */
			blackr[blackouts].x1 = x;
			blackr[blackouts].y1 = y;
			blackr[blackouts].x2 = X;
			blackr[blackouts].y2 = Y;
			blackouts++;
			if (blackouts >= BLACKR_MAX) {
				rfbLog("too many blackouts: %d\n", blackouts);
				break;
			}
		}
		p = strtok(NULL, ", \t");
	}
	free(blist);
}

/*
 * Now that all blackout rectangles have been constructed, see what overlap
 * they have with the tiles in the system.  If a tile is touched by a
 * blackout, record information.
 */
static void blackout_tiles(void) {
	int tx, ty;
	int debug_bo = 0;
	if (! blackouts) {
		return;
	}
	if (getenv("DEBUG_BLACKOUT") != NULL) {
		debug_bo = 1;
	}

	/* 
	 * to simplify things drop down to single copy mode, etc...
	 */
	single_copytile = 1;
	/* loop over all tiles. */
	for (ty=0; ty < ntiles_y; ty++) {
		for (tx=0; tx < ntiles_x; tx++) {
			sraRegionPtr tile_reg, black_reg;
			sraRect rect;
			sraRectangleIterator *iter;
			int n, b, x1, y1, x2, y2, cnt;

			/* tile number and coordinates: */
			n = tx + ty * ntiles_x;
			x1 = tx * tile_x;
			y1 = ty * tile_y;
			x2 = x1 + tile_x;
			y2 = y1 + tile_y;
			if (x2 > dpy_x) {
				x2 = dpy_x;
			}
			if (y2 > dpy_y) {
				y2 = dpy_y;
			}

			/* make regions for the tile and the blackouts: */
			black_reg = (sraRegionPtr) sraRgnCreate();
			tile_reg  = (sraRegionPtr) sraRgnCreateRect(x1, y1,
			    x2, y2);

			tile_blackout[n].cover = 0;
			tile_blackout[n].count = 0;

			/* union of blackouts */
			for (b=0; b < blackouts; b++) {
				sraRegionPtr tmp_reg = (sraRegionPtr)
				    sraRgnCreateRect(blackr[b].x1,
				    blackr[b].y1, blackr[b].x2, blackr[b].y2);

				sraRgnOr(black_reg, tmp_reg);
				sraRgnDestroy(tmp_reg);
			}

			if (! sraRgnAnd(black_reg, tile_reg)) {
				/*
				 * no intersection for this tile, so we
				 * are done.
				 */
				sraRgnDestroy(black_reg);
				sraRgnDestroy(tile_reg);
				continue;
			}

			/*
			 * loop over rectangles that make up the blackout
			 * region:
			 */
			cnt = 0;
			iter = sraRgnGetIterator(black_reg);
			while (sraRgnIteratorNext(iter, &rect)) {

				/* make sure x1 < x2 and y1 < y2 */
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

				/* store coordinates */
				tile_blackout[n].bo[cnt].x1 = rect.x1;
				tile_blackout[n].bo[cnt].y1 = rect.y1;
				tile_blackout[n].bo[cnt].x2 = rect.x2;
				tile_blackout[n].bo[cnt].y2 = rect.y2;

				/* note if the tile is completely obscured */
				if (rect.x1 == x1 && rect.y1 == y1 &&
				    rect.x2 == x2 && rect.y2 == y2) {
					tile_blackout[n].cover = 2;
					if (debug_bo) {
 						fprintf(stderr, "full: %d=%d,%d"
						    "  (%d-%d)  (%d-%d)\n",
						    n, tx, ty, x1, x2, y1, y2);
					}
				} else {
					tile_blackout[n].cover = 1;
					if (debug_bo) {
						fprintf(stderr, "part: %d=%d,%d"
						    "  (%d-%d)  (%d-%d)\n",
						    n, tx, ty, x1, x2, y1, y2);
					}
				}

				if (++cnt >= BO_MAX) {
					rfbLog("too many blackout rectangles "
					    "for tile %d=%d,%d.\n", n, tx, ty);
					break;
				}
			}
			sraRgnReleaseIterator(iter);

			sraRgnDestroy(black_reg);
			sraRgnDestroy(tile_reg);

			tile_blackout[n].count = cnt;
			if (debug_bo && cnt > 1) {
 				rfbLog("warning: multiple region overlaps[%d] "
				    "for tile %d=%d,%d.\n", cnt, n, tx, ty);
			}
		}
	}
}

static void initialize_xinerama (void) {
#if !LIBVNCSERVER_HAVE_LIBXINERAMA
	rfbLog("Xinerama: Library libXinerama is not available to determine\n");
	rfbLog("Xinerama: the head geometries, consider using -blackout\n");
	rfbLog("Xinerama: if the screen is non-rectangular.\n");
#else
	XineramaScreenInfo *sc, *xineramas;
	sraRegionPtr black_region, tmp_region;
	sraRectangleIterator *iter;
	sraRect rect;
	char *bstr, *tstr;
	int ev, er, i, n, rcnt;

	RAWFB_RET_VOID

	if (! XineramaQueryExtension(dpy, &ev, &er)) {
		if (verbose) {
			rfbLog("Xinerama: disabling: display does not support it.\n");
		}
		xinerama = 0;
		xinerama_present = 0;
		return;
	}
	if (! XineramaIsActive(dpy)) {
		/* n.b. change to XineramaActive(dpy, window) someday */
		if (verbose) {
			rfbLog("Xinerama: disabling: not active on display.\n");
		}
		xinerama = 0;
		xinerama_present = 0;
		return;
	} 
	xinerama_present = 1;

	/* n.b. change to XineramaGetData() someday */
	xineramas = XineramaQueryScreens(dpy, &n);
	if (verbose) {
		rfbLog("Xinerama: number of sub-screens: %d\n", n);
	}

	if (n == 1) {
		if (verbose) {
			rfbLog("Xinerama: no blackouts needed (only one"
			    " sub-screen)\n");
		}
		XFree(xineramas);
		return;		/* must be OK w/o change */
	}

	black_region = sraRgnCreateRect(0, 0, dpy_x, dpy_y);

	sc = xineramas;
	for (i=0; i<n; i++) {
		int x, y, w, h;
		
		x = sc->x_org;
		y = sc->y_org;
		w = sc->width;
		h = sc->height;

		tmp_region = sraRgnCreateRect(x, y, x + w, y + h);

		sraRgnSubtract(black_region, tmp_region);
		sraRgnDestroy(tmp_region);
		sc++;
	}
	XFree(xineramas);

	if (sraRgnEmpty(black_region)) {
		rfbLog("Xinerama: no blackouts needed (screen fills"
		    " rectangle)\n");
		sraRgnDestroy(black_region);
		return;
	}

	/* max len is 10000x10000+10000+10000 (23 chars) per geometry */
	rcnt = (int) sraRgnCountRects(black_region);
	bstr = (char *) malloc(30 * (rcnt+1));
	tstr = (char *) malloc(30);
	bstr[0] = '\0';

	iter = sraRgnGetIterator(black_region);
	while (sraRgnIteratorNext(iter, &rect)) {
		int x, y, w, h;

		/* make sure x1 < x2 and y1 < y2 */
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
		x = rect.x1;
		y = rect.y1;
		w = rect.x2 - x;
		h = rect.y2 - y;
		sprintf(tstr, "%dx%d+%d+%d,", w, h, x, y);
		strcat(bstr, tstr);
	}
	sraRgnReleaseIterator(iter);
	initialize_blackouts(bstr);

	free(bstr);
	free(tstr);
#endif
}

void initialize_blackouts_and_xinerama(void) {

	blackouts = 0;
	blackout_ptr = 0;

	if (blackout_str != NULL) {
		initialize_blackouts(blackout_str);
	}
	if (xinerama) {
		initialize_xinerama();
	}
	if (blackouts) {
		blackout_tiles();
		/* schedule a copy_screen(), now is too early. */
		do_copy_screen = 1;
	}
}

void push_sleep(int n) {
	int i;
	for (i=0; i<n; i++) {
		rfbPE(-1);
		if (i != n-1 && defer_update) {
			usleep(defer_update * 1000);
		}
	}
}

/*
 * try to forcefully push a black screen to all connected clients
 */
void push_black_screen(int n) {
	if (!screen) {
		return;
	}
	zero_fb(0, 0, dpy_x, dpy_y);
	mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	push_sleep(n);
}

void refresh_screen(int push) {
	int i;
	if (!screen) {
		return;
	}
	mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	for (i=0; i<push; i++) {
		rfbPE(-1);
	}
}

/*
 * Fill the framebuffer with zero for the prescribed rectangle
 */
void zero_fb(int x1, int y1, int x2, int y2) {
	int pixelsize = bpp/8;
	int line, fill = 0;
	char *dst;
	
	if (x1 < 0 || x2 <= x1 || x2 > dpy_x) {
		return;
	}
	if (y1 < 0 || y2 <= y1 || y2 > dpy_y) {
		return;
	}
	if (! main_fb) {
		return;
	}

	dst = main_fb + y1 * main_bytes_per_line + x1 * pixelsize;
	line = y1;
	while (line++ < y2) {
		memset(dst, fill, (size_t) (x2 - x1) * pixelsize);
		dst += main_bytes_per_line;
	}
}


