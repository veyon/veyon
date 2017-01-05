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

/* -- xkb_bell.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "connections.h"

/*
 * Bell event handling.  Requires XKEYBOARD extension.
 */
int xkb_base_event_type = 0;

void initialize_xkb(void);
void initialize_watch_bell(void);
void check_bell_event(void);


#if LIBVNCSERVER_HAVE_XKEYBOARD

/*
 * check for XKEYBOARD, set up xkb_base_event_type
 */
void initialize_xkb(void) {
	int ir, reason;
	int op, ev, er, maj, min;
	
	RAWFB_RET_VOID

	if (xkbcompat) {
		xkb_present = 0;
	} else if (! XkbQueryExtension(dpy, &op, &ev, &er, &maj, &min)) {
		if (! quiet) {
			rfbLog("warning: XKEYBOARD extension not present.\n");
		}
		xkb_present = 0;
	} else {
		xkb_present = 1;
	}

	if (! xkb_present) {
		return;
	}

	if (! xauth_raw(1)) {
		return;
	}

	if (! XkbOpenDisplay(DisplayString(dpy), &xkb_base_event_type, &ir,
	    NULL, NULL, &reason) ) {
		if (! quiet) {
			rfbLog("warning: disabling XKEYBOARD. XkbOpenDisplay"
			    " failed.\n");
		}
		xkb_base_event_type = 0;
		xkb_present = 0;
	}
	xauth_raw(0);
}

void initialize_watch_bell(void) {
	if (! xkb_present) {
		if (! quiet) {
			rfbLog("warning: disabling bell. XKEYBOARD ext. "
			    "not present.\n");
		}
		watch_bell = 0;
		sound_bell = 0;
		return;
	}

	RAWFB_RET_VOID

	XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask, 0);

	if (! watch_bell) {
		return;
	}
	if (! XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask,
	    XkbBellNotifyMask) ) {
		if (! quiet) {
			rfbLog("warning: disabling bell. XkbSelectEvents"
			    " failed.\n");
		}
		watch_bell = 0;
		sound_bell = 0;
	}
}

/*
 * We call this periodically to process any bell events that have 
 * taken place.
 */
void check_bell_event(void) {
	XEvent xev;
	XkbAnyEvent *xkb_ev;
	int got_bell = 0;

	if (! xkb_base_event_type) {
		return;
	}
	RAWFB_RET_VOID

	/* caller does X_LOCK */
	if (! XCheckTypedEvent(dpy, xkb_base_event_type, &xev)) {
		return;
	}
	if (! watch_bell) {
		/* we return here to avoid xkb events piling up */
		return;
	}

	xkb_ev = (XkbAnyEvent *) &xev;
	if (xkb_ev->xkb_type == XkbBellNotify) {
		got_bell = 1;
	}

	if (got_bell && sound_bell) {
		if (! all_clients_initialized()) {
			rfbLog("check_bell_event: not sending bell: "
			    "uninitialized clients\n");
		} else {
			if (screen && client_count) {
				rfbSendBell(screen);
			}
		}
	}
}
#else
void initialize_watch_bell(void) {
	watch_bell = 0;
	sound_bell = 0;
}
void check_bell_event(void) {}
#endif


