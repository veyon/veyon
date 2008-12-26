/* -- selection.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "connections.h"
#include "unixpw.h"
#include "xwrappers.h"

/*
 * Selection/Cutbuffer/Clipboard handlers.
 */

int own_primary = 0;	/* whether we currently own PRIMARY or not */
int set_primary = 1;
int own_clipboard = 0;	/* whether we currently own CLIPBOARD or not */
int set_clipboard = 1;
int set_cutbuffer = 0;	/* to avoid bouncing the CutText right back */
int sel_waittime = 15;	/* some seconds to skip before first send */
Window selwin;		/* special window for our selection */
Atom clipboard_atom = None;

/*
 * This is where we keep our selection: the string sent TO us from VNC
 * clients, and the string sent BY us to requesting X11 clients.
 */
char *xcut_str_primary = NULL;
char *xcut_str_clipboard = NULL;


void selection_request(XEvent *ev, char *type);
int check_sel_direction(char *dir, char *label, char *sel, int len);
void cutbuffer_send(void);
void selection_send(XEvent *ev);


/*
 * Our callbacks instruct us to check for changes in the cutbuffer
 * and PRIMARY and CLIPBOARD selection on the local X11 display.
 *
 * TODO: check if malloc does not cause performance issues (esp. WRT
 * SelectionNotify handling).
 */
static char cutbuffer_str[PROP_MAX+1];
static char primary_str[PROP_MAX+1];
static char clipboard_str[PROP_MAX+1];

/*
 * An X11 (not VNC) client on the local display has requested the selection
 * from us (because we are the current owner).
 *
 * n.b.: our caller already has the X_LOCK.
 */
void selection_request(XEvent *ev, char *type) {
#if NO_X11
	RAWFB_RET_VOID
	if (!ev || !type) {}
	return;
#else
	XSelectionEvent notify_event;
	XSelectionRequestEvent *req_event;
	XErrorHandler old_handler;
	char *str;
	unsigned int length;
	unsigned char *data;
	static Atom xa_targets = None;
# ifndef XA_LENGTH
	unsigned long XA_LENGTH;
# endif
	RAWFB_RET_VOID

# ifndef XA_LENGTH
	XA_LENGTH = XInternAtom(dpy, "LENGTH", True);
# endif

	req_event = &(ev->xselectionrequest);
	notify_event.type 	= SelectionNotify;
	notify_event.display	= req_event->display;
	notify_event.requestor	= req_event->requestor;
	notify_event.selection	= req_event->selection;
	notify_event.target	= req_event->target;
	notify_event.time	= req_event->time;

	if (req_event->property == None) {
		notify_event.property = req_event->target;
	} else {
		notify_event.property = req_event->property;
	}

	if (!strcmp(type, "PRIMARY")) {
		str = xcut_str_primary;
	} else if (!strcmp(type, "CLIPBOARD")) {
		str = xcut_str_clipboard;
	} else {
		return;
	}
	if (str) {
		length = strlen(str);
	} else {
		length = 0;
	}
	if (debug_sel) {
		rfbLog("%s\trequest event:   owner=0x%x requestor=0x%x sel=%03d targ=%d prop=%d\n",
			type, req_event->owner, req_event->requestor, req_event->selection,
			req_event->target, req_event->property);
	}

	if (xa_targets == None) {
		xa_targets = XInternAtom(dpy, "TARGETS", False);
	}

	/* the window may have gone away, so trap errors */
	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);

	if (ev->xselectionrequest.target == XA_LENGTH) {
		/* length request */
		int ret;
		long llength = (long) length;

		ret = XChangeProperty(ev->xselectionrequest.display,
		    ev->xselectionrequest.requestor,
		    ev->xselectionrequest.property,
		    ev->xselectionrequest.target, 32, PropModeReplace,
		    (unsigned char *) &llength, 1);	/* had sizeof(unsigned int) = 4 before... */
		if (debug_sel) {
			rfbLog("LENGTH: XChangeProperty() -> %d\n", ret);
		}

	} else if (xa_targets != None && ev->xselectionrequest.target == xa_targets) {
		/* targets request */
		int ret;
		Atom targets[2];
		targets[0] = (Atom) xa_targets;
		targets[1] = (Atom) XA_STRING;

		ret = XChangeProperty(ev->xselectionrequest.display,
		    ev->xselectionrequest.requestor,
		    ev->xselectionrequest.property,
		    ev->xselectionrequest.target, 32, PropModeReplace,
		    (unsigned char *) targets, 2);
		if (debug_sel) {
			rfbLog("TARGETS: XChangeProperty() -> %d -- sz1: %d  sz2: %d\n",
			    ret, sizeof(targets[0]), sizeof(targets)/sizeof(targets[0]));
		}

	} else {
		/* data request */
		int ret;

		data = (unsigned char *)str;

		ret = XChangeProperty(ev->xselectionrequest.display,
		    ev->xselectionrequest.requestor,
		    ev->xselectionrequest.property,
		    ev->xselectionrequest.target, 8, PropModeReplace,
		    data, length);
		if (debug_sel) {
			rfbLog("DATA: XChangeProperty() -> %d\n", ret);
		}
	}

	if (! trapped_xerror) {
		int ret = XSendEvent(req_event->display, req_event->requestor, False, 0,
		    (XEvent *)&notify_event);
		if (debug_sel) {
			rfbLog("XSendEvent() -> %d\n", ret);
		}
	}
	if (trapped_xerror) {
		rfbLog("selection_request: ignored XError while sending "
		    "%s selection to 0x%x.\n", type, req_event->requestor);
	}
	XSetErrorHandler(old_handler);
	trapped_xerror = 0;

	XFlush_wr(dpy);
#endif	/* NO_X11 */
}

int check_sel_direction(char *dir, char *label, char *sel, int len) {
	int db = 0, ok = 1;
	if (debug_sel) {
		db = 1;
	}
	if (sel_direction) {
		if (strstr(sel_direction, "debug")) {
			db = 1;
		}
		if (strcmp(sel_direction, "debug")) {
			if (strstr(sel_direction, dir) == NULL) {
				ok = 0;
			}
		}
	}
	if (db) {
		char str[40];
		int n = 40;
		strncpy(str, sel, n);
		str[n-1] = '\0';
		if (len < n) {
			str[len] = '\0';
		}
		rfbLog("%s: '%s'\n", label, str);
		if (ok) {
			rfbLog("%s: %s-ing it.\n", label, dir);
		} else {
			rfbLog("%s: NOT %s-ing it.\n", label, dir);
		}
	}
	return ok;
}

/*
 * CUT_BUFFER0 property on the local display has changed, we read and
 * store it and send it out to any connected VNC clients.
 *
 * n.b.: our caller already has the X_LOCK.
 */
void cutbuffer_send(void) {
#if NO_X11
	RAWFB_RET_VOID
	return;
#else
	Atom type;
	int format, slen, dlen, len;
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;

	cutbuffer_str[0] = '\0';
	slen = 0;

	RAWFB_RET_VOID

	/* read the property value into cutbuffer_str: */
	do {
		if (XGetWindowProperty(dpy, DefaultRootWindow(dpy),
		    XA_CUT_BUFFER0, nitems/4, PROP_MAX/16, False,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > PROP_MAX) {
				/* too big */
				rfbLog("warning: truncating large CUT_BUFFER0"
				   " selection > %d bytes.\n", PROP_MAX);
				XFree_wr(data);
				break;
			}
			memcpy(cutbuffer_str+slen, data, dlen);
			slen += dlen;
			cutbuffer_str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);

	cutbuffer_str[PROP_MAX] = '\0';

	if (debug_sel) {
		rfbLog("cutbuffer_send: '%s'\n", cutbuffer_str);
	}

	if (! all_clients_initialized()) {
		rfbLog("cutbuffer_send: no send: uninitialized clients\n");
		return; /* some clients initializing, cannot send */ 
	}
	if (unixpw_in_progress) {
		return;
	}

	/* now send it to any connected VNC clients (rfbServerCutText) */
	if (!screen) {
		return;
	}
	len = strlen(cutbuffer_str);
	if (check_sel_direction("send", "cutbuffer_send", cutbuffer_str, len)) {
		rfbSendServerCutText(screen, cutbuffer_str, len);
	}
#endif	/* NO_X11 */
}

/* 
 * "callback" for our SelectionNotify polling.  We try to determine if
 * the PRIMARY selection has changed (checking length and first CHKSZ bytes)
 * and if it has we store it and send it off to any connected VNC clients.
 *
 * n.b.: our caller already has the X_LOCK.
 *
 * TODO: if we were willing to use libXt, we could perhaps get selection
 * timestamps to speed up the checking... XtGetSelectionValue().
 *
 * Also: XFIXES has XFixesSelectSelectionInput().
 */
#define CHKSZ 32

void selection_send(XEvent *ev) {
#if NO_X11
	RAWFB_RET_VOID
	if (!ev) {}
	return;
#else
	Atom type;
	int format, slen, dlen, oldlen, newlen, toobig = 0, len;
	static int err = 0, sent_one = 0;
	char before[CHKSZ], after[CHKSZ];
	unsigned long nitems = 0, bytes_after = 0;
	unsigned char* data = NULL;
	char *selection_str;

	RAWFB_RET_VOID
	/*
	 * remember info about our last value of PRIMARY (or CUT_BUFFER0)
	 * so we can check for any changes below.
	 */
	if (ev->xselection.selection == XA_PRIMARY) {
		if (! watch_primary) {
			return;
		}
		selection_str = primary_str;
		if (debug_sel) {
			rfbLog("selection_send: event PRIMARY   prop: %d  requestor: 0x%x  atom: %d\n",
			    ev->xselection.property, ev->xselection.requestor, ev->xselection.selection);
		}
	} else if (clipboard_atom && ev->xselection.selection == clipboard_atom)  {
		if (! watch_clipboard) {
			return;
		}
		selection_str = clipboard_str;
		if (debug_sel) {
			rfbLog("selection_send: event CLIPBOARD prop: %d  requestor: 0x%x atom: %d\n",
			    ev->xselection.property, ev->xselection.requestor, ev->xselection.selection);
		}
	} else {
		return;
	}
	
	oldlen = strlen(selection_str);
	strncpy(before, selection_str, CHKSZ);

	selection_str[0] = '\0';
	slen = 0;

	/* read in the current value of PRIMARY or CLIPBOARD: */
	do {
		if (XGetWindowProperty(dpy, ev->xselection.requestor,
		    ev->xselection.property, nitems/4, PROP_MAX/16, True,
		    AnyPropertyType, &type, &format, &nitems, &bytes_after,
		    &data) == Success) {

			dlen = nitems * (format/8);
			if (slen + dlen > PROP_MAX) {
				/* too big */
				toobig = 1;
				XFree_wr(data);
				if (err) {	/* cut down on messages */
					break;
				} else {
					err = 5;
				}
				rfbLog("warning: truncating large PRIMARY"
				    "/CLIPBOARD selection > %d bytes.\n",
				    PROP_MAX);
				break;
			}
if (debug_sel) fprintf(stderr, "selection_send: data: '%s' dlen: %d nitems: %lu ba: %lu\n", data, dlen, nitems, bytes_after);
			memcpy(selection_str+slen, data, dlen);
			slen += dlen;
			selection_str[slen] = '\0';
			XFree_wr(data);
		}
	} while (bytes_after > 0);

	if (! toobig) {
		err = 0;
	} else if (err) {
		err--;
	}

	if (! sent_one) {
		/* try to force a send first time in */
		oldlen = -1;
		sent_one = 1;
	}
	if (debug_sel) {
		rfbLog("selection_send:  %s '%s'\n",
		    ev->xselection.selection == XA_PRIMARY ? "PRIMARY  " : "CLIPBOARD",
		    selection_str);
	}

	/* look for changes in the new value */
	newlen = strlen(selection_str);
	strncpy(after, selection_str, CHKSZ);

	if (oldlen == newlen && strncmp(before, after, CHKSZ) == 0) {
		/* evidently no change */
		if (debug_sel) {
			rfbLog("selection_send:  no change.\n");
		}
		return;
	}
	if (newlen == 0) {
		/* do not bother sending a null string out */
		return;
	}

	if (! all_clients_initialized()) {
		rfbLog("selection_send: no send: uninitialized clients\n");
		return; /* some clients initializing, cannot send */ 
	}

	if (unixpw_in_progress) {
		return;
	}

	/* now send it to any connected VNC clients (rfbServerCutText) */
	if (!screen) {
		return;
	}

	len = newlen;
	if (check_sel_direction("send", "selection_send", selection_str, len)) {
		rfbSendServerCutText(screen, selection_str, len);
	}
#endif	/* NO_X11 */
}


