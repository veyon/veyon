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

/* -- xrandr.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "connections.h"
#include "remote.h"
#include "screen.h"
#include "win_utils.h"

time_t last_subwin_trap = 0;
int subwin_trap_count = 0;
XErrorHandler old_getimage_handler;

int xrandr_present = 0;
int xrandr_width  = -1;
int xrandr_height = -1;
int xrandr_rotation = -1;
Time xrandr_timestamp = 0;
Time xrandr_cfg_time = 0;

void initialize_xrandr(void);
int check_xrandr_event(char *msg);
int known_xrandr_mode(char *s);

static int handle_subwin_resize(char *msg);
static void handle_xrandr_change(int new_x, int new_y);


void initialize_xrandr(void) {
	if (xrandr_present && dpy) {
#if LIBVNCSERVER_HAVE_LIBXRANDR
		Rotation rot;

		X_LOCK;
		xrandr_width  = XDisplayWidth(dpy, scr);
		xrandr_height = XDisplayHeight(dpy, scr);
		XRRRotations(dpy, scr, &rot);
		xrandr_rotation = (int) rot;
		if (xrandr || xrandr_maybe) {
			XRRSelectInput(dpy, rootwin, RRScreenChangeNotifyMask);
		} else {
			XRRSelectInput(dpy, rootwin, 0);
		}
		X_UNLOCK;
#endif
	} else if (xrandr) {
		rfbLog("-xrandr mode specified, but no RANDR support on\n");
		rfbLog(" display or in client library. Disabling -xrandr "
		    "mode.\n");
		xrandr = 0;
	}
}

static int handle_subwin_resize(char *msg) {
	int new_x, new_y;
	int i, check = 10, ms = 250;	/* 2.5 secs total... */

	if (msg) {}	/* unused vars warning: */
	if (! subwin) {
		return 0;	/* hmmm... */
	}
	if (! valid_window(subwin, NULL, 0)) {
		rfbLogEnable(1);
		rfbLog("subwin 0x%lx went away!\n", subwin);
		X_UNLOCK;
		clean_up_exit(1);
	}
	if (! get_window_size(subwin, &new_x, &new_y)) {
		rfbLogEnable(1);
		rfbLog("could not get size of subwin 0x%lx\n", subwin);
		X_UNLOCK;
		clean_up_exit(1);
	}
	if (wdpy_x == new_x && wdpy_y == new_y) {
		/* no change */
		return 0;
	}

	/* window may still be changing (e.g. drag resize) */
	for (i=0; i < check; i++) {
		int newer_x, newer_y;
		usleep(ms * 1000);

		if (! get_window_size(subwin, &newer_x, &newer_y)) {
			rfbLogEnable(1);
			rfbLog("could not get size of subwin 0x%lx\n", subwin);
			clean_up_exit(1);
		}
		if (new_x == newer_x && new_y == newer_y) {
			/* go for it... */
			break;
		} else {
			rfbLog("subwin 0x%lx still changing size...\n", subwin);
			new_x = newer_x;
			new_y = newer_y;
		}
	}

	rfbLog("subwin 0x%lx new size: x: %d -> %d, y: %d -> %d\n",
	    subwin, wdpy_x, new_x, wdpy_y, new_y);
	rfbLog("calling handle_xrandr_change() for resizing\n");

	X_UNLOCK;
	handle_xrandr_change(new_x, new_y);
	return 1;
}

static void handle_xrandr_change(int new_x, int new_y) {
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	RAWFB_RET_VOID

	/* assumes no X_LOCK */

	/* sanity check xrandr_mode */
	if (! xrandr_mode) {
		xrandr_mode = strdup("default");
	} else if (! known_xrandr_mode(xrandr_mode)) {
		free(xrandr_mode);
		xrandr_mode = strdup("default");
	}
	rfbLog("xrandr_mode: %s\n", xrandr_mode);
	if (!strcmp(xrandr_mode, "exit")) {
		close_all_clients();
		rfbLog("  shutting down due to XRANDR event.\n");
		clean_up_exit(0);
	}
	if (!strcmp(xrandr_mode, "newfbsize") && screen) {
		iter = rfbGetClientIterator(screen);
		while( (cl = rfbClientIteratorNext(iter)) ) {
			if (cl->useNewFBSize) {
				continue;
			}
			rfbLog("  closing client %s (no useNewFBSize"
			    " support).\n", cl->host);
			rfbCloseClient(cl);
			rfbClientConnectionGone(cl);
		}
		rfbReleaseClientIterator(iter);
	}
	
	/* default, resize, and newfbsize create a new fb: */
	rfbLog("check_xrandr_event: trying to create new framebuffer...\n");
	if (new_x < wdpy_x || new_y < wdpy_y) {
		check_black_fb();
	}
	do_new_fb(1);
	rfbLog("check_xrandr_event: fb       WxH: %dx%d\n", wdpy_x, wdpy_y);
}

int check_xrandr_event(char *msg) {
	XEvent xev;

	RAWFB_RET(0)

	/* it is assumed that X_LOCK is on at this point. */

	if (subwin) {
		return handle_subwin_resize(msg);
	}
#if LIBVNCSERVER_HAVE_LIBXRANDR
	if (! xrandr_present) {
		return 0;
	}
	if (! xrandr && ! xrandr_maybe) {
		return 0;
	}


	if (xrandr_base_event_type && XCheckTypedEvent(dpy,
	    xrandr_base_event_type + RRScreenChangeNotify, &xev)) {
		int do_change, qout = 0;
		static int first = 1;
		XRRScreenChangeNotifyEvent *rev;

		rev = (XRRScreenChangeNotifyEvent *) &xev;

		if (first && ! xrandr) {
			fprintf(stderr, "\n");
			if (getenv("X11VNC_DEBUG_XRANDR") == NULL) {
				qout = 1;
			}
		}
		first = 0;
			
		rfbLog("check_xrandr_event():\n");
		rfbLog("Detected XRANDR event at location '%s':\n", msg);

		if (qout) {
			;
		} else {
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
			rfbLog("check_xrandr_event: previous WxH: %dx%d\n",
			    wdpy_x, wdpy_y);
		}

		if (wdpy_x == rev->width && wdpy_y == rev->height &&
		    xrandr_rotation == (int) rev->rotation) {
			rfbLog("check_xrandr_event: no change detected.\n");
			do_change = 0;
			if (! xrandr) {
		    		rfbLog("check_xrandr_event: "
				    "enabling full XRANDR trapping anyway.\n");
				xrandr = 1;
			}
		} else {
			do_change = 1;
			if (! xrandr) {
		    		rfbLog("check_xrandr_event: Resize; "
				    "enabling full XRANDR trapping.\n");
				xrandr = 1;
			}
		}

		xrandr_width  = rev->width;
		xrandr_height = rev->height;
		xrandr_timestamp = rev->timestamp;
		xrandr_cfg_time  = rev->config_timestamp;
		xrandr_rotation = (int) rev->rotation;

		if (! qout) rfbLog("check_xrandr_event: updating config...\n");
		XRRUpdateConfiguration(&xev);

		if (do_change) {
			/* under do_change caller normally returns before its X_UNLOCK */
			X_UNLOCK;
			handle_xrandr_change(rev->width, rev->height);
		}
		if (qout) {
			return do_change;
		}
		rfbLog("check_xrandr_event: current  WxH: %dx%d\n",
		    XDisplayWidth(dpy, scr), XDisplayHeight(dpy, scr));
		rfbLog("check_xrandr_event(): returning control to"
		    " caller...\n");


		return do_change;
	}
#else
	xev.type = 0;
#endif


	return 0;
}

int known_xrandr_mode(char *s) {
/*
 * default:	
 * resize:	the default
 * exit:	shutdown clients and exit.
 * newfbsize:	shutdown clients that do not support NewFBSize encoding.
 */
	if (strcmp(s, "default") && strcmp(s, "resize") && 
	    strcmp(s, "exit") && strcmp(s, "newfbsize")) {
		return 0;
	} else {
		return 1;
	}
}


