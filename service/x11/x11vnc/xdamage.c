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

/* -- xdamage.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "userinput.h"
#include "unixpw.h"

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
Damage xdamage = 0;
#endif

#ifndef XDAMAGE
#define XDAMAGE 1
#endif
int use_xdamage = XDAMAGE;	/* use the xdamage rects for scanline hints */
int xdamage_present = 0;

#ifdef MACOSX
int xdamage_max_area = 50000;
#else
int xdamage_max_area = 20000;	/* pixels */
#endif

double xdamage_memory = 1.0;	/* in units of NSCAN */
int xdamage_tile_count = 0, xdamage_direct_count = 0;
double xdamage_scheduled_mark = 0.0;
double xdamage_crazy_time = 0.0;
double xdamage_crazy_delay = 300.0;
sraRegionPtr xdamage_scheduled_mark_region = NULL;
sraRegionPtr *xdamage_regions = NULL;
int xdamage_ticker = 0;
int XD_skip = 0, XD_tot = 0, XD_des = 0;	/* for stats */

void add_region_xdamage(sraRegionPtr new_region);
void clear_xdamage_mark_region(sraRegionPtr markregion, int flush);
int collect_non_X_xdamage(int x_in, int y_in, int w_in, int h_in, int call);
int collect_xdamage(int scancnt, int call);
int xdamage_hint_skip(int y);
void initialize_xdamage(void);
void create_xdamage_if_needed(int force);
void destroy_xdamage_if_needed(void);
void check_xdamage_state(void);

static void record_desired_xdamage_rect(int x, int y, int w, int h);


static void record_desired_xdamage_rect(int x, int y, int w, int h) {
	/*
	 * Unfortunately we currently can't trust an xdamage event
	 * to correspond to real screen damage.  E.g. focus-in for
	 * mozilla (depending on wm) will mark the whole toplevel
	 * area as damaged, when only the border has changed.
	 * Similar things for terminal windows.
	 *
	 * This routine uses some heuristics to detect small enough
	 * damage regions that we will not have a performance problem
	 * if we believe them even though they are wrong.  We record
	 * the corresponding tiles the damage regions touch.
	 */
	int dt_x, dt_y, nt_x1, nt_y1, nt_x2, nt_y2, nt;
	int ix, iy, cnt = 0;
	int area = w*h, always_accept = 0;
	/*
	 * XXX: not working yet, slow and overlaps with scan_display()
	 * probably slow because tall skinny rectangles very inefficient
	 * in general and in direct_fb_copy() (100X slower then horizontal).
	 */
	int use_direct_fb_copy = 0;
	int wh_min, wh_max;
	static int first = 1, udfb = 0;

	/* compiler warning: */
	nt_x1 = 0; nt_y1 = 0; nt_x2 = 0; nt_y2 = 0;

	if (first) {
		if (getenv("XD_DFC")) {
			udfb = 1;
		}
		first = 0;
	}
	if (udfb) {
		use_direct_fb_copy = 1;
	}

	if (xdamage_max_area <= 0) {
		always_accept = 1;
	}

	if (!always_accept && area > xdamage_max_area) {
		return;
	}

	dt_x = w / tile_x;
	dt_y = h / tile_y;

	if (w < h) {
		wh_min = w;
		wh_max = h;
	} else {
		wh_min = h;
		wh_max = w;
	}

	if (!always_accept && dt_y >= 3 && area > 4000)  {
		/*
		 * if it is real it should be caught by a normal scanline
		 * poll, but we might as well keep if small (tall line?).
		 */
		return;
	}
	
	if (use_direct_fb_copy) {
		X_UNLOCK;
		direct_fb_copy(x, y, x + w, y + h, 1);
		xdamage_direct_count++;
		X_LOCK;
	} else if (0 && wh_min < tile_x/4 && wh_max > 30 * wh_min) {
		/* try it for long, skinny rects, XXX still no good */
		X_UNLOCK;
		direct_fb_copy(x, y, x + w, y + h, 1);
		xdamage_direct_count++;
		X_LOCK;
	} else {

		if (ntiles_x == 0 || ntiles_y == 0) {
			/* too early. */
			return;
		}
		nt_x1 = nfix(  (x)/tile_x, ntiles_x);
		nt_x2 = nfix((x+w)/tile_x, ntiles_x);
		nt_y1 = nfix(  (y)/tile_y, ntiles_y);
		nt_y2 = nfix((y+h)/tile_y, ntiles_y);

		/*
		 * loop over the rectangle of tiles (1 tile for a small
		 * input rect).
		 */
		for (ix = nt_x1; ix <= nt_x2; ix++) {
			for (iy = nt_y1; iy <= nt_y2; iy++) {
				nt = ix + iy * ntiles_x;
				cnt++;
				if (! tile_has_xdamage_diff[nt]) {
					XD_des++;
					tile_has_xdamage_diff[nt] = 1;
				}
				/* not used: */
				tile_row_has_xdamage_diff[iy] = 1;
				xdamage_tile_count++;
			}
		}
	}
	if (debug_xdamage > 1) {
		fprintf(stderr, "xdamage: desired: %dx%d+%d+%d\tA: %6d  tiles="
		    "%02d-%02d/%02d-%02d  tilecnt: %d\n", w, h, x, y,
		    w * h, nt_x1, nt_x2, nt_y1, nt_y2, cnt);
	}
}

void add_region_xdamage(sraRegionPtr new_region) {
	sraRegionPtr reg;
	int prev_tick, nreg;

	if (! xdamage_regions) {
		return;
	}

	nreg = (xdamage_memory * NSCAN) + 1;
	prev_tick = xdamage_ticker - 1;
	if (prev_tick < 0) {
		prev_tick = nreg - 1;
	}

	reg = xdamage_regions[prev_tick];  
	if (reg != NULL && new_region != NULL) {
if (debug_xdamage > 1) fprintf(stderr, "add_region_xdamage: prev_tick: %d reg %p  new_region %p\n", prev_tick, (void *)reg, (void *)new_region);
		sraRgnOr(reg, new_region);
	}
}

void clear_xdamage_mark_region(sraRegionPtr markregion, int flush) {
#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	XEvent ev;
	sraRegionPtr tmpregion;
	int count = 0;

	RAWFB_RET_VOID

	if (! xdamage_present || ! use_xdamage) {
		return;
	}
	if (! xdamage) {
		return;
	}
	if (! xdamage_base_event_type) {
		return;
	}
	if (unixpw_in_progress) return;

	X_LOCK;
	if (flush) {
		XFlush_wr(dpy);
	}
	while (XCheckTypedEvent(dpy, xdamage_base_event_type+XDamageNotify, &ev)) {
		count++;
	}
	/* clear the whole damage region */
	XDamageSubtract(dpy, xdamage, None, None);
	X_UNLOCK;

	if (debug_tiles || debug_xdamage) {
		fprintf(stderr, "clear_xdamage_mark_region: %d\n", count);
	}

	if (! markregion) {
		/* NULL means mark the whole display */
		tmpregion = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
		add_region_xdamage(tmpregion);
		sraRgnDestroy(tmpregion);
	} else {
		add_region_xdamage(markregion);
	}
#else
	if (0) flush++;        /* compiler warnings */
	if (0) markregion = NULL;   
#endif
}

int collect_non_X_xdamage(int x_in, int y_in, int w_in, int h_in, int call) {
	sraRegionPtr tmpregion;
	sraRegionPtr reg;
	static int rect_count = 0;
	int nreg, ccount = 0, dcount = 0, ecount = 0;
	static time_t last_rpt = 0;
	time_t now;
	double tm, dt;
	int x, y, w, h, x2, y2;

if (call && debug_xdamage > 1) fprintf(stderr, "collect_non_X_xdamage: %d %d %d %d - %d / %d\n", x_in, y_in, w_in, h_in, call, use_xdamage);

	if (! use_xdamage) {
		return 0;
	}
	if (! xdamage_regions) {
		return 0;
	}

	dtime0(&tm);

	nreg = (xdamage_memory * NSCAN) + 1;

	if (call == 0) {
		xdamage_ticker = (xdamage_ticker+1) % nreg;
		xdamage_direct_count = 0;
		reg = xdamage_regions[xdamage_ticker];  
		if (reg != NULL) {
			sraRgnMakeEmpty(reg);
		}
	} else {
		if (xdamage_ticker < 0) {
			xdamage_ticker = 0;
		}
		reg = xdamage_regions[xdamage_ticker];  
	}
	if (reg == NULL) {
		return 0;
	}

	if (x_in < 0) {
		return 0;
	}


	x = x_in;
	y = y_in;
	w = w_in;
	h = h_in;

	/* translate if needed */
	if (clipshift) {
		/* set coords relative to fb origin */
		if (0 && rootshift) {
			/*
			 * Note: not needed because damage is
			 * relative to subwin, not rootwin.
			 */
			x = x - off_x;
			y = y - off_y;
		}
		if (clipshift) {
			x = x - coff_x;
			y = y - coff_y;
		}

		x2 = x + w;		/* upper point */
		x  = nfix(x,  dpy_x);	/* place both in fb area */
		x2 = nfix(x2, dpy_x+1);
		w = x2 - x;		/* recompute w */
		
		y2 = y + h;
		y  = nfix(y,  dpy_y);
		y2 = nfix(y2, dpy_y+1);
		h = y2 - y;

		if (w <= 0 || h <= 0) {
			return 0;
		}
	}
	if (debug_xdamage > 2) {
		fprintf(stderr, "xdamage: -> event %dx%d+%d+%d area:"
		    " %d  dups: %d  %s reg: %p\n", w, h, x, y, w*h, dcount,
		    (w*h > xdamage_max_area) ? "TOO_BIG" : "", (void *)reg);
	}

	record_desired_xdamage_rect(x, y, w, h);

	tmpregion = sraRgnCreateRect(x, y, x + w, y + h); 
	sraRgnOr(reg, tmpregion);
	sraRgnDestroy(tmpregion);
	rect_count++;
	ccount++;

	if (0 && xdamage_direct_count) {
		fb_push();
	}

	dt = dtime(&tm);
	if ((debug_tiles > 1 && ecount) || (debug_tiles && ecount > 200)
	    || debug_xdamage > 1) {
		fprintf(stderr, "collect_non_X_xdamage(%d): %.4f t: %.4f ev/dup/accept"
		    "/direct %d/%d/%d/%d\n", call, dt, tm - x11vnc_start, ecount,
		    dcount, ccount, xdamage_direct_count); 
	}
	now = time(NULL);
	if (! last_rpt) {
		last_rpt = now;
	}
	if (now > last_rpt + 15) {
		double rat = -1.0;

		if (XD_tot) {
			rat = ((double) XD_skip)/XD_tot;
		}
		if (debug_tiles || debug_xdamage) {
			fprintf(stderr, "xdamage: == scanline skip/tot: "
			    "%04d/%04d =%.3f  rects: %d  desired: %d\n",
			    XD_skip, XD_tot, rat, rect_count, XD_des);
		}
			
		XD_skip = 0;
		XD_tot  = 0;
		XD_des  = 0;
		rect_count = 0;
		last_rpt = now;
	}
	return 0;
}

int collect_xdamage(int scancnt, int call) {
#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	XDamageNotifyEvent *dev;
	XEvent ev;
	sraRegionPtr tmpregion;
	sraRegionPtr reg;
	static int rect_count = 0;
	int nreg, ccount = 0, dcount = 0, ecount = 0;
	static time_t last_rpt = 0;
	time_t now;
	int x, y, w, h, x2, y2;
	int i, dup, next = 0, dup_max = 0;
#define DUPSZ 32
	int dup_x[DUPSZ], dup_y[DUPSZ], dup_w[DUPSZ], dup_h[DUPSZ];
	double tm, dt;
	int mark_all = 0, retries = 0, too_many = 1000, tot_ev = 0;

	RAWFB_RET(0)

	if (scancnt) {} /* unused vars warning: */

	if (! xdamage_present || ! use_xdamage) {
		return 0;
	}
	if (! xdamage) {
		return 0;
	}
	if (! xdamage_base_event_type) {
		return 0;
	}
	if (! xdamage_regions) {
		return 0;
	}

	dtime0(&tm);

	nreg = (xdamage_memory * NSCAN) + 1;

	if (call == 0) {
		xdamage_ticker = (xdamage_ticker+1) % nreg;
		xdamage_direct_count = 0;
		reg = xdamage_regions[xdamage_ticker];  
		if (reg != NULL) {
			sraRgnMakeEmpty(reg);
		}
	} else {
		if (xdamage_ticker < 0) {
			xdamage_ticker = 0;
		}
		reg = xdamage_regions[xdamage_ticker];  
	}
	if (reg == NULL) {
		return 0;
	}


	X_LOCK;
if (0)	XFlush_wr(dpy);
if (0)	XEventsQueued(dpy, QueuedAfterFlush);

	come_back_for_more:

	while (XCheckTypedEvent(dpy, xdamage_base_event_type+XDamageNotify, &ev)) {
		/*
		 * TODO max cut off time in this loop?
		 * Could check QLength and if huge just mark the whole
		 * screen.
		 */
		ecount++;
		tot_ev++;

		if (mark_all) {
			continue;
		}
		if (ecount == too_many) {
			int nqa = XEventsQueued(dpy, QueuedAlready);
			if (nqa >= too_many) {
				static double last_msg = 0.0;
				tmpregion = sraRgnCreateRect(0, 0, dpy_x, dpy_y);
				sraRgnOr(reg, tmpregion);
				sraRgnDestroy(tmpregion);
				if (dnow() > last_msg + xdamage_crazy_delay) {
					rfbLog("collect_xdamage: too many xdamage events %d+%d\n", ecount, nqa);
					last_msg = dnow();
				}
				mark_all = 1;
			}
		}

		if (ev.type != xdamage_base_event_type + XDamageNotify) {
			break;
		}
		dev = (XDamageNotifyEvent *) &ev;
		if (dev->damage != xdamage) {
			continue;	/* not ours! */
		}

		x = dev->area.x;
		y = dev->area.y;
		w = dev->area.width;
		h = dev->area.height;

		/*
		 * we try to manually remove some duplicates because
		 * certain activities can lead to many 10's of dups
		 * in a row.  The region work can be costly and reg is
		 * later used in xdamage_hint_skip loops, so it is good
		 * to skip them if possible.
		 */
		dup = 0;
		for (i=0; i < dup_max; i++) {
			if (dup_x[i] == x && dup_y[i] == y && dup_w[i] == w &&
			    dup_h[i] == h) {
				dup = 1;
				break;
			}
		}
		if (dup) {
			dcount++;
			continue;
		}
		if (dup_max < DUPSZ) {
			next = dup_max;
			dup_max++;
		} else {
			next = (next+1) % DUPSZ;
		}
		dup_x[next] = x;
		dup_y[next] = y;
		dup_w[next] = w;
		dup_h[next] = h;

		/* translate if needed */
		if (clipshift) {
			/* set coords relative to fb origin */
			if (0 && rootshift) {
				/*
				 * Note: not needed because damage is
				 * relative to subwin, not rootwin.
				 */
				x = x - off_x;
				y = y - off_y;
			}
			if (clipshift) {
				x = x - coff_x;
				y = y - coff_y;
			}

			x2 = x + w;		/* upper point */
			x  = nfix(x,  dpy_x);	/* place both in fb area */
			x2 = nfix(x2, dpy_x+1);
			w = x2 - x;		/* recompute w */
			
			y2 = y + h;
			y  = nfix(y,  dpy_y);
			y2 = nfix(y2, dpy_y+1);
			h = y2 - y;

			if (w <= 0 || h <= 0) {
				continue;
			}
		}
		if (debug_xdamage > 2) {
			fprintf(stderr, "xdamage: -> event %dx%d+%d+%d area:"
			    " %d  dups: %d  %s\n", w, h, x, y, w*h, dcount,
			    (w*h > xdamage_max_area) ? "TOO_BIG" : "");
		}

		record_desired_xdamage_rect(x, y, w, h);

		tmpregion = sraRgnCreateRect(x, y, x + w, y + h); 
		sraRgnOr(reg, tmpregion);
		sraRgnDestroy(tmpregion);
		rect_count++;
		ccount++;
	}

	if (mark_all) {
		if (ecount + XEventsQueued(dpy, QueuedAlready) >= 3 * too_many && retries < 3) {
			retries++;
			XFlush_wr(dpy);
			usleep(20 * 1000);
			XFlush_wr(dpy);
			ecount = 0;
			goto come_back_for_more;
		}
	}

	/* clear the whole damage region for next time. XXX check */
	if (call == 1) {
		XDamageSubtract(dpy, xdamage, None, None);
	}
	X_UNLOCK;

	if (tot_ev > 20 * too_many) {
		rfbLog("collect_xdamage: xdamage has gone crazy (screensaver or game?) ev: %d ret: %d\n", tot_ev, retries);
		rfbLog("collect_xdamage: disabling xdamage for %d seconds.\n", (int) xdamage_crazy_delay);
		destroy_xdamage_if_needed();
		X_LOCK;
		XSync(dpy, False);
		while (XCheckTypedEvent(dpy, xdamage_base_event_type+XDamageNotify, &ev)) {
			;
		}
		X_UNLOCK;
		xdamage_crazy_time = dnow();
	}

	if (0 && xdamage_direct_count) {
		fb_push();
	}

	dt = dtime(&tm);
	if ((debug_tiles > 1 && ecount) || (debug_tiles && ecount > 200)
	    || debug_xdamage > 1) {
		fprintf(stderr, "collect_xdamage(%d): %.4f t: %.4f ev/dup/accept"
		    "/direct %d/%d/%d/%d\n", call, dt, tm - x11vnc_start, ecount,
		    dcount, ccount, xdamage_direct_count); 
	}
	now = time(NULL);
	if (! last_rpt) {
		last_rpt = now;
	}
	if (now > last_rpt + 15) {
		double rat = -1.0;

		if (XD_tot) {
			rat = ((double) XD_skip)/XD_tot;
		}
		if (debug_tiles || debug_xdamage) {
			fprintf(stderr, "xdamage: == scanline skip/tot: "
			    "%04d/%04d =%.3f  rects: %d  desired: %d\n",
			    XD_skip, XD_tot, rat, rect_count, XD_des);
		}
			
		XD_skip = 0;
		XD_tot  = 0;
		XD_des  = 0;
		rect_count = 0;
		last_rpt = now;
	}
#else
	if (0) scancnt++;	/* compiler warnings */
	if (0) call++;
	if (0) record_desired_xdamage_rect(0, 0, 0, 0);
#endif
	return 0;
}

int xdamage_hint_skip(int y) {
	static sraRegionPtr scanline = NULL;
	static sraRegionPtr tmpl_y = NULL;
	int fast_tmpl = 1;
	sraRegionPtr reg, tmpl;
	int ret, i, n, nreg;
#ifndef NO_NCACHE
	static int ncache_no_skip = 0;
	static double last_ncache_no_skip = 0.0;
	static double last_ncache_no_skip_long = 0.0, ncache_fac = 0.25;
#endif

	if (! xdamage_present || ! use_xdamage) {
		return 0;	/* cannot skip */
	}
	if (! xdamage_regions) {
		return 0;	/* cannot skip */
	}

	if (! scanline) {
		/* keep it around to avoid malloc etc, recreate */
		scanline = sraRgnCreate();
	}
	if (! tmpl_y) {
		tmpl_y = sraRgnCreateRect(0, 0, dpy_x, 1);
	}

	nreg = (xdamage_memory * NSCAN) + 1;

#ifndef NO_NCACHE
	if (ncache > 0) {
		if (ncache_no_skip == 0) {
			double now = g_now;
			if (now > last_ncache_no_skip + 8.0) {
				ncache_no_skip = 1;
			} else if (now < last_bs_restore + 0.5) {
				ncache_no_skip = 1;
			} else if (now < last_su_restore + 0.5) {
				ncache_no_skip = 1;
			} else if (now < last_copyrect + 0.5) {
				ncache_no_skip = 1;
			}
			if (ncache_no_skip) {
				last_ncache_no_skip = dnow();
				if (now > last_ncache_no_skip_long + 60.0) {
					ncache_fac = 2.0;
					last_ncache_no_skip_long = now;
				} else {
					ncache_fac = 0.25;
				}
				return 0;
			}
		} else {
			if (ncache_no_skip++ >= ncache_fac*nreg + 4) {
				ncache_no_skip = 0;
			} else {
				return 0;
			}
		}
	}
#endif

	if (fast_tmpl) {
		sraRgnOffset(tmpl_y, 0, y);
		tmpl = tmpl_y;
	} else {
		tmpl = sraRgnCreateRect(0, y, dpy_x, y+1);
	}

	ret = 1;
	for (i=0; i<nreg; i++) {
		/* go back thru the history starting at most recent */
		n = (xdamage_ticker + nreg - i) % nreg;
		reg = xdamage_regions[n];  
		if (reg == NULL) {
			continue;
		}
		if (sraRgnEmpty(reg)) {
			/* checking for emptiness is very fast */
			continue;
		}
		sraRgnMakeEmpty(scanline);
		sraRgnOr(scanline, tmpl);
		if (sraRgnAnd(scanline, reg)) {
			ret = 0;
			break;
		}
	}
	if (fast_tmpl) {
		sraRgnOffset(tmpl_y, 0, -y);
	} else {
		sraRgnDestroy(tmpl);
	}
if (0) fprintf(stderr, "xdamage_hint_skip: %d -> %d\n", y, ret);

	return ret;
}

void initialize_xdamage(void) {
	sraRegionPtr *ptr;
	int i, nreg;

	if (! xdamage_present) {
		use_xdamage = 0;
	}
	if (xdamage_regions)  {
		ptr = xdamage_regions;
		while (*ptr != NULL) {
			sraRgnDestroy(*ptr);
			ptr++;
		}
		free(xdamage_regions);
		xdamage_regions = NULL;
	}
	if (use_xdamage) {
		nreg = (xdamage_memory * NSCAN) + 2;
		xdamage_regions = (sraRegionPtr *)
		    malloc(nreg * sizeof(sraRegionPtr));
		for (i = 0; i < nreg; i++) {
			ptr = xdamage_regions+i;
			if (i == nreg - 1) {
				*ptr = NULL;
			} else {
				*ptr = sraRgnCreate();
				sraRgnMakeEmpty(*ptr);
			}
		}
		/* set so will be 0 in first collect_xdamage call */
		xdamage_ticker = -1;
	}
}

void create_xdamage_if_needed(int force) {

	RAWFB_RET_VOID

	if (force) {}

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	if (! xdamage || force) {
		X_LOCK;
		xdamage = XDamageCreate(dpy, window, XDamageReportRawRectangles); 
		XDamageSubtract(dpy, xdamage, None, None);
		X_UNLOCK;
		rfbLog("created   xdamage object: 0x%lx\n", xdamage);
	}
#endif
}

void destroy_xdamage_if_needed(void) {

	RAWFB_RET_VOID

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	if (xdamage) {
		XEvent ev;
		X_LOCK;
		XDamageDestroy(dpy, xdamage);
		XFlush_wr(dpy);
		if (xdamage_base_event_type) {
			while (XCheckTypedEvent(dpy,
			    xdamage_base_event_type+XDamageNotify, &ev)) {
				;
			}
		}
		X_UNLOCK;
		rfbLog("destroyed xdamage object: 0x%lx\n", xdamage);
		xdamage = 0;
	}
#endif
}

void check_xdamage_state(void) {
	if (! xdamage_present) {
		return;
	}
	/*
	 * Create or destroy the Damage object as needed, we don't want
	 * one if no clients are connected.
	 */
	if (xdamage_crazy_time > 0.0 && dnow() < xdamage_crazy_time + xdamage_crazy_delay) {
		return;
	}
	if (client_count && use_xdamage) {
		create_xdamage_if_needed(0);
		if (xdamage_scheduled_mark > 0.0 && dnow() >
		    xdamage_scheduled_mark) {
			if (xdamage_scheduled_mark_region) {
				mark_region_for_xdamage(
				    xdamage_scheduled_mark_region);
				sraRgnDestroy(xdamage_scheduled_mark_region);
				xdamage_scheduled_mark_region = NULL;
			} else {
				mark_for_xdamage(0, 0, dpy_x, dpy_y);
			}
			xdamage_scheduled_mark = 0.0;
		}
	} else {
		destroy_xdamage_if_needed();
	}
}


