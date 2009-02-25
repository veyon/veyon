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


