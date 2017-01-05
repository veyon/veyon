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

/* -- win_utils.c -- */

#include "x11vnc.h"
#include "xinerama.h"
#include "winattr_t.h"
#include "cleanup.h"
#include "xwrappers.h"
#include "connections.h"
#include "xrandr.h"
#include "macosx.h"

winattr_t *stack_list = NULL;
int stack_list_len = 0;
int stack_list_num = 0;


Window parent_window(Window win, char **name);
int valid_window(Window win, XWindowAttributes *attr_ret, int bequiet);
Bool xtranslate(Window src, Window dst, int src_x, int src_y, int *dst_x,
    int *dst_y, Window *child, int bequiet);
int get_window_size(Window win, int *w, int *h);
void snapshot_stack_list(int free_only, double allowed_age);
int get_boff(void);
int get_bwin(void);
void update_stack_list(void);
Window query_pointer(Window start);
unsigned int mask_state(void);
int pick_windowid(unsigned long *num);
Window descend_pointer(int depth, Window start, char *name_info, int len);
void id_cmd(char *cmd);


Window parent_window(Window win, char **name) {
#if !NO_X11
	Window r, parent;
	Window *list;
	XErrorHandler old_handler;
	unsigned int nchild;
	int rc;
#endif

	if (name != NULL) {
		*name = NULL;
	}
	RAWFB_RET(None)
#if NO_X11
	nox11_exit(1);
	if (!name || !win) {}
	return None;
#else

	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	rc = XQueryTree_wr(dpy, win, &r, &parent, &list, &nchild);
	XSetErrorHandler(old_handler);

	if (! rc || trapped_xerror) {
		trapped_xerror = 0;
		return None;
	}
	trapped_xerror = 0;

	if (list) {
		XFree_wr(list);
	}
	if (parent && name) {
		XFetchName(dpy, parent, name);
	}
	return parent;
#endif	/* NO_X11 */
}

/* trapping utility to check for a valid window: */
int valid_window(Window win, XWindowAttributes *attr_ret, int bequiet) {
	XWindowAttributes attr, *pattr;
#if !NO_X11
	XErrorHandler old_handler;
	int ok = 0;
#endif

	if (attr_ret == NULL) {
		pattr = &attr;
	} else {
		pattr = attr_ret;
	}

	if (win == None) {
		return 0;
	}

#ifdef MACOSX
	if (macosx_console) {
		return macosx_valid_window(win, attr_ret);
	}
#endif

	RAWFB_RET(0)

#if NO_X11
	nox11_exit(1);
	if (!win || !attr_ret || !bequiet) {}
	return 0;
#else

	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	if (XGetWindowAttributes(dpy, win, pattr)) {
		ok = 1;
	}
	if (trapped_xerror && trapped_xerror_event) {
		if (! quiet && ! bequiet) {
			rfbLog("valid_window: trapped XError: %s (0x%lx)\n",
			    xerror_string(trapped_xerror_event), win);
		}
		ok = 0;
	}
	XSetErrorHandler(old_handler);
	trapped_xerror = 0;
	
	return ok;
#endif	/* NO_X11 */
}

Bool xtranslate(Window src, Window dst, int src_x, int src_y, int *dst_x,
    int *dst_y, Window *child, int bequiet) {
	XErrorHandler old_handler = NULL;
	Bool ok = False;

	RAWFB_RET(False)
#if NO_X11
	nox11_exit(1);
	if (!src || !dst || !src_x || !src_y || !dst_x || !dst_y || !child || !bequiet) {}
	if (!old_handler || !ok) {}
	return False;
#else

	trapped_xerror = 0;
	old_handler = XSetErrorHandler(trap_xerror);
	if (XTranslateCoordinates(dpy, src, dst, src_x, src_y, dst_x,
	    dst_y, child)) {
		ok = True;
	}
	if (trapped_xerror && trapped_xerror_event) {
		if (! quiet && ! bequiet) {
			rfbLog("xtranslate: trapped XError: %s (0x%lx)\n",
			    xerror_string(trapped_xerror_event), src);
		}
		ok = False;
	}
	XSetErrorHandler(old_handler);
	trapped_xerror = 0;
	
	return ok;
#endif	/* NO_X11 */
}

int get_window_size(Window win, int *w, int *h) {
	XWindowAttributes attr;
	/* valid_window? */
	if (valid_window(win, &attr, 1)) {
		*w = attr.width;
		*h = attr.height;
		return 1;
	} else {
		return 0;
	}
}

/*
 * For use in the -wireframe stuff, save the stacking order of the direct
 * children of the root window.  Ideally done before we send ButtonPress
 * to the X server.
 */
void snapshot_stack_list(int free_only, double allowed_age) {
	static double last_snap = 0.0, last_free = 0.0;
	double now; 
	int num, rc, i, j;
	unsigned int ui;
	Window r, w;
	Window *list;

	if (! stack_list) {
		stack_list = (winattr_t *) malloc(256*sizeof(winattr_t));
		stack_list_num = 0;
		stack_list_len = 256;
	}

	dtime0(&now);
	if (free_only) {
		/* we really don't free it, just reset to zero windows */
		stack_list_num = 0;
		last_free = now;
		return;
	}

	if (stack_list_num && now < last_snap + allowed_age) {
		return;
	}

	stack_list_num = 0;
	last_free = now;

#ifdef MACOSX
	if (! macosx_console) {
		RAWFB_RET_VOID
	}
#else
	RAWFB_RET_VOID
#endif

#if NO_X11 && !defined(MACOSX)
	num = rc = i = j = 0;	/* compiler warnings */
	ui = 0;
	r = w = None;
	list = NULL;
	return;
#else

	X_LOCK;
	/* no need to trap error since rootwin */
	rc = XQueryTree_wr(dpy, rootwin, &r, &w, &list, &ui);
	num = (int) ui;

	if (! rc) {
		stack_list_num = 0;
		last_free = now;
		last_snap = 0.0;
		X_UNLOCK;
		return;
	}

	last_snap = now;
	if (num > stack_list_len + blackouts) {
		int n = 2*num;
		free(stack_list);
		stack_list = (winattr_t *) malloc(n*sizeof(winattr_t));
		stack_list_len = n;
	}
	j = 0;
	for (i=0; i<num; i++) {
		stack_list[j].win = list[i];
		stack_list[j].fetched = 0;
		stack_list[j].valid = 0;
		stack_list[j].time = now;
		j++;
	}
	for (i=0; i<blackouts; i++) {
		stack_list[j].win = get_boff() + 1;
		stack_list[j].fetched = 1;
		stack_list[j].valid = 1;
		stack_list[j].x = blackr[i].x1;
		stack_list[j].y = blackr[i].y1;
		stack_list[j].width  = blackr[i].x2 - blackr[i].x1;
		stack_list[j].height = blackr[i].y2 - blackr[i].y1;
		stack_list[j].time = now;
		stack_list[j].map_state = IsViewable;
		stack_list[j].rx = -1;
		stack_list[j].ry = -1;
		j++;

if (0) fprintf(stderr, "blackr: %d %dx%d+%d+%d\n", i,
	stack_list[j-1].width, stack_list[j-1].height,
	stack_list[j-1].x, stack_list[j-1].y);

	}
	stack_list_num = num + blackouts;
	if (debug_wireframe > 1) {
		fprintf(stderr, "snapshot_stack_list: num=%d len=%d\n",
		    stack_list_num, stack_list_len);
	}

	XFree_wr(list);
	X_UNLOCK;
#endif	/* NO_X11 */
}

int get_boff(void) {
	if (macosx_console) {
		return 0x1000000;
	} else {
		return 0;		
	}
}

int get_bwin(void) {
	return 10;		
}

void update_stack_list(void) {
	int k;
	double now;
	XWindowAttributes attr;
	int boff, bwin;

	if (! stack_list) {
		return;
	}
	if (! stack_list_num) {
		return;
	}

	dtime0(&now);

	boff = get_boff();
	bwin = get_bwin();
	
	X_LOCK;
	for (k=0; k < stack_list_num; k++) {
		Window win = stack_list[k].win;
		if (win != None && boff <= (int) win && (int) win < boff + bwin) {
			;	/* special, blackout */
		} else if (!valid_window(win, &attr, 1)) {
			stack_list[k].valid = 0;
		} else {
			stack_list[k].valid = 1;
			stack_list[k].x = attr.x;
			stack_list[k].y = attr.y;
			stack_list[k].width = attr.width;
			stack_list[k].height = attr.height;
			stack_list[k].border_width = attr.border_width;
			stack_list[k].depth = attr.depth;
			stack_list[k].class = attr.class;
			stack_list[k].backing_store = attr.backing_store;
			stack_list[k].map_state = attr.map_state;

			/* root_x, root_y not used for stack_list usage: */
			stack_list[k].rx = -1;
			stack_list[k].ry = -1;
		}
		stack_list[k].fetched = 1;
		stack_list[k].time = now;
	}
	X_UNLOCK;
if (0) fprintf(stderr, "update_stack_list[%d]: %.4f  %.4f\n", stack_list_num, now - x11vnc_start, dtime(&now));
}

Window query_pointer(Window start) {
	int rx, ry;
#if !NO_X11
	Window r, c;	/* compiler warnings */
	int wx, wy;
	unsigned int mask;
#endif

#ifdef MACOSX
	if (macosx_console) {
		macosx_get_cursor_pos(&rx, &ry);
	}
#endif

	RAWFB_RET(None)

#if NO_X11
	if (!start) { rx = ry = 0; }
	return None;
#else
	if (start == None) {
		start = rootwin;
	}
	if (XQueryPointer_wr(dpy, start, &r, &c, &rx, &ry, &wx, &wy, &mask)) {
		return c;
	} else {
		return None;
	}
#endif	/* NO_X11 */
}

unsigned int mask_state(void) {
#if NO_X11
	RAWFB_RET(0)
	return 0;
#else
	Window r, c;	
	int rx, ry, wx, wy;
	unsigned int mask;

	RAWFB_RET(0)

	if (XQueryPointer_wr(dpy, rootwin, &r, &c, &rx, &ry, &wx, &wy, &mask)) {
		return mask;
	} else {
		return 0;
	}
#endif	/* NO_X11 */
}

int pick_windowid(unsigned long *num) {
	char line[512];
	int ok = 0, n = 0, msec = 10, secmax = 15;
	FILE *p;

	RAWFB_RET(0)

	if (use_dpy) {
		set_env("DISPLAY", use_dpy);
	}
	/* id */
	if (no_external_cmds || !cmd_ok("id")) {
		rfbLogEnable(1);
		rfbLog("cannot run external commands in -nocmds mode:\n");
		rfbLog("   \"%s\"\n", "xwininfo");
		rfbLog("   exiting.\n");
		clean_up_exit(1);
	}
	close_exec_fds();
	p = popen("xwininfo", "r");

	if (! p) {
		return 0;
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "  Please select the window for x11vnc to poll\n");
	fprintf(stderr, "  by clicking the mouse in that window.\n");
	fprintf(stderr, "\n");

	while (msec * n++ < 1000 * secmax) {
		unsigned long tmp;
		char *q;
		fd_set set;
		struct timeval tv;

		if (screen && screen->clientHead) {
			/* they may be doing the pointer-pick thru vnc: */
			int nfds;
			tv.tv_sec = 0;
			tv.tv_usec = msec * 1000;
			FD_ZERO(&set);
			FD_SET(fileno(p), &set);

			nfds = select(fileno(p)+1, &set, NULL, NULL, &tv);
			
			if (nfds == 0 || nfds < 0) {
				/* 
				 * select timedout or error.
				 * note this rfbPE takes about 30ms too:
				 */
				rfbPE(-1);
				XFlush_wr(dpy);
				continue;
			}
		}
		
		if (fgets(line, 512, p) == NULL) {
			break;
		}
		q = strstr(line, " id: 0x"); 
		if (q) {
			q += 5;
			if (sscanf(q, "0x%lx ", &tmp) == 1) {
				ok = 1;
				*num = tmp;
				fprintf(stderr, "  Picked: 0x%lx\n\n", tmp);
				break;
			}
		}
	}
	pclose(p);
	return ok;
}

Window descend_pointer(int depth, Window start, char *name_info, int len) {
#if NO_X11
	RAWFB_RET(None)
	if (!depth || !start || !name_info || !len) {}
	return None;
#else
	Window r, c, clast = None;
	int i, rx, ry, wx, wy;
	int written = 0, filled = 0;
	char *store = NULL;
	unsigned int m;
	static XClassHint *classhint = NULL;
	static char *nm_cache = NULL;
	static int nm_cache_len = 0;
	static Window prev_start = None;

	RAWFB_RET(None)

	if (! classhint) {
		classhint = XAllocClassHint();
	}

	if (! nm_cache) {
		nm_cache = (char *) malloc(1024);
		nm_cache_len = 1024;
		nm_cache[0] = '\0';
	}
	if (name_info && nm_cache_len < len) {
		if (nm_cache) {
			free(nm_cache);
		}
		nm_cache_len = 2*len;
		nm_cache = (char *) malloc(nm_cache_len);
	}

	if (name_info) {
		if (start != None && start == prev_start) {
			store = NULL;
			strncpy(name_info, nm_cache, len);
		} else {
			store = name_info;
			name_info[0] = '\0';
		}
	}

	if (start != None) {
		c = start;
		if (name_info) {
			prev_start = start;
		}
	} else {
		c = rootwin;	
	}

	for (i=0; i<depth; i++) {
		clast = c;
		if (store && ! filled) {
			char *name;
			if (XFetchName(dpy, clast, &name) && name != NULL) {
				int l = strlen(name);
				if (written + l+2 < len) {
					strcat(store, "^^");
					written += 2;
					strcat(store, name);
					written += l;
				} else {
					filled = 1;
				}
				XFree_wr(name);
			}
		}
		if (store && classhint && ! filled) {
			classhint->res_name = NULL;
			classhint->res_class = NULL;
			if (XGetClassHint(dpy, clast, classhint)) {
				int l = 0;
				if (classhint->res_class) {
					l += strlen(classhint->res_class); 
				}
				if (classhint->res_name) {
					l += strlen(classhint->res_name); 
				}
				if (written + l+4 < len) {
					strcat(store, "##");
					if (classhint->res_class) {
						strcat(store,
						    classhint->res_class);
					}
					strcat(store, "++");
					if (classhint->res_name) {
						strcat(store,
						    classhint->res_name);
					}
					written += l+4;
				} else {
					filled = 1;
				}
				if (classhint->res_class) {
					XFree_wr(classhint->res_class);
				}
				if (classhint->res_name) {
					XFree_wr(classhint->res_name);
				}
			}
		}
		if (! XQueryPointer_wr(dpy, c, &r, &c, &rx, &ry, &wx, &wy, &m)) {
			break;
		}
		if (! c) {
			break;
		}
	}
	if (start != None && name_info) {
		strncpy(nm_cache, name_info, nm_cache_len);
	}

	return clast;
#endif	/* NO_X11 */
}

void id_cmd(char *cmd) {
	int rc, dx = 0, dy = 0, dw = 0, dh = 0;
	int x0, y0, w0, h0;
	int x, y, w, h, do_move = 0, do_resize = 0;
	int disp_x = DisplayWidth(dpy, scr);
	int disp_y = DisplayHeight(dpy, scr);
	Window win = subwin;
	XWindowAttributes attr;
	XErrorHandler old_handler = NULL;
	Window twin;

	if (!cmd || !strcmp(cmd, "")) { 
		return;
	}
	if (strstr(cmd, "win=") == cmd) {
		if (! scan_hexdec(cmd + strlen("win="), &win)) {
			rfbLog("id_cmd: incorrect win= hex/dec number: %s\n", cmd);
			return;
		} else {
			char *q = strchr(cmd, ':');
			if (!q) {
				rfbLog("id_cmd: incorrect win=...: hex/dec number: %s\n", cmd);
				return;
			}
			rfbLog("id_cmd:%s set window id to 0x%lx\n", cmd, win);
			cmd = q+1;
		}
	}
	if (!win) {
		rfbLog("id_cmd:%s not in sub-window mode or no win=0xNNNN.\n", cmd);
		return;
	}
#if !NO_X11
	X_LOCK;
	if (!valid_window(win, &attr, 1)) {
		X_UNLOCK;
		return;
	}
	w0 = w = attr.width;
	h0 = h = attr.height;
	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	XTranslateCoordinates(dpy, win, rootwin, 0, 0, &x, &y, &twin);
	x0 = x;
	y0 = y;
	if (strstr(cmd, "move:") == cmd) {
		if (sscanf(cmd, "move:%d%d", &dx, &dy) == 2) {
			x = x + dx;
			y = y + dy;
			do_move = 1;
		}
	} else if (strstr(cmd, "resize:") == cmd) {
		if (sscanf(cmd, "resize:%d%d", &dw, &dh) == 2) {
			w = w + dw;
			h = h + dh;
			do_move = 1;
			do_resize = 1;
		}
	} else if (strstr(cmd, "geom:") == cmd) {
		if (parse_geom(cmd+strlen("geom:"), &w, &h, &x, &y, disp_x, disp_y)) {
			do_move = 1;
			do_resize = 1;
			if (w <= 0) {
				w = w0;
			}
			if (h <= 0) {
				h = h0;
			}
			if (scaling && getenv("X11VNC_APPSHARE_ACTIVE")) {
				x /= scale_fac_x;
				y /= scale_fac_y;
			}
		}
	} else if (!strcmp(cmd, "raise")) {
		rc = XRaiseWindow(dpy, win);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (!strcmp(cmd, "lower")) {
		rc = XLowerWindow(dpy, win);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (!strcmp(cmd, "map")) {
		rc= XMapRaised(dpy, win);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (!strcmp(cmd, "unmap")) {
		rc= XUnmapWindow(dpy, win);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (!strcmp(cmd, "iconify")) {
		rc= XIconifyWindow(dpy, win, scr);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (strstr(cmd, "wm_name:") == cmd) {
		rc= XStoreName(dpy, win, cmd+strlen("wm_name:"));
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (strstr(cmd, "icon_name:") == cmd) {
		rc= XSetIconName(dpy, win, cmd+strlen("icon_name:"));
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else if (!strcmp(cmd, "wm_delete")) {
		XClientMessageEvent ev;
		memset(&ev, 0, sizeof(ev));
		ev.type = ClientMessage;
		ev.send_event = True;
		ev.display = dpy;
		ev.window = win;
		ev.message_type = XInternAtom(dpy, "WM_PROTOCOLS", False);
		ev.format = 32;
		ev.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
		rc = XSendEvent(dpy, win, False, 0, (XEvent *) &ev);
		rfbLog("id_cmd:%s rc=%d\n", cmd, rc);
	} else {
		rfbLog("id_cmd:%s unrecognized command.\n", cmd);
	}
	if (do_move || do_resize) {
		if (w >= disp_x) {
			w = disp_x - 4;
		}
		if (h >= disp_y) {
			h = disp_y - 4;
		}
		if (w < 1) {
			w = 1;
		}
		if (h < 1) {
			h = 1;
		}
		if (x + w > disp_x) {
			x = disp_x - w - 1;
		}
		if (y + h > disp_y) {
			y = disp_y - h - 1;
		}
		if (x < 0) {
			x = 1;
		}
		if (y < 0) {
			y = 1;
		}
		rc = 0;
		rc += XMoveWindow(dpy, win, x, y);
		off_x = x;
		off_y = y;

		rc += XResizeWindow(dpy, win, w, h);

		rfbLog("id_cmd:%s rc=%d dx=%d dy=%d dw=%d dh=%d %dx%d+%d+%d -> %dx%d+%d+%d\n",
		    cmd, rc, dx, dy, dw, dh, w0, h0, x0, y0, w, h, x, h);
	}
	XSync(dpy, False);
	XSetErrorHandler(old_handler);
	if (trapped_xerror) {
		rfbLog("id_cmd:%s trapped_xerror.\n", cmd);
	}
	trapped_xerror = 0;
	if (do_resize) {
		rfbLog("id_cmd:%s calling check_xrandr_event.\n", cmd);
		check_xrandr_event("id_cmd");
	}
	X_UNLOCK;
#endif
}

