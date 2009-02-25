/* -- win_utils.c -- */

#include "x11vnc.h"
#include "xinerama.h"
#include "winattr_t.h"
#include "cleanup.h"
#include "xwrappers.h"
#include "connections.h"

winattr_t *stack_list = NULL;
int stack_list_len = 0;
int stack_list_num = 0;


Window parent_window(Window win, char **name);
int valid_window(Window win, XWindowAttributes *attr_ret, int bequiet);
Bool xtranslate(Window src, Window dst, int src_x, int src_y, int *dst_x,
    int *dst_y, Window *child, int bequiet);
int get_window_size(Window win, int *x, int *y);
void snapshot_stack_list(int free_only, double allowed_age);
void update_stack_list(void);
Window query_pointer(Window start);
unsigned int mask_state(void);
int pick_windowid(unsigned long *num);
Window descend_pointer(int depth, Window start, char *name_info, int len);


Window parent_window(Window win, char **name) {
	Window r, parent;
	Window *list;
	XErrorHandler old_handler;
	unsigned int nchild;
	int rc;

	if (name != NULL) {
		*name = NULL;
	}
	RAWFB_RET(None)

	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	rc = XQueryTree(dpy, win, &r, &parent, &list, &nchild);
	XSetErrorHandler(old_handler);

	if (! rc || trapped_xerror) {
		trapped_xerror = 0;
		return None;
	}
	trapped_xerror = 0;

	if (list) {
		XFree(list);
	}
	if (parent && name) {
		XFetchName(dpy, parent, name);
	}
	return parent;
}

/* trapping utility to check for a valid window: */
int valid_window(Window win, XWindowAttributes *attr_ret, int bequiet) {
	XErrorHandler old_handler;
	XWindowAttributes attr, *pattr;
	int ok = 0;

	if (attr_ret == NULL) {
		pattr = &attr;
	} else {
		pattr = attr_ret;
	}

	if (win == None) {
		return 0;
	}
	RAWFB_RET(0)

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
}

Bool xtranslate(Window src, Window dst, int src_x, int src_y, int *dst_x,
    int *dst_y, Window *child, int bequiet) {
	XErrorHandler old_handler;
	Bool ok = False;

	RAWFB_RET(False)

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
}

int get_window_size(Window win, int *x, int *y) {
	XWindowAttributes attr;
	/* valid_window? */
	if (valid_window(win, &attr, 1)) {
		*x = attr.width;
		*y = attr.height;
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

	RAWFB_RET_VOID

	X_LOCK;
	/* no need to trap error since rootwin */
	rc = XQueryTree(dpy, rootwin, &r, &w, &list, &ui);
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
		stack_list[j].win = 0x1;
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

	XFree(list);
	X_UNLOCK;
}

void update_stack_list(void) {
	int k;
	double now;
	XWindowAttributes attr;

	if (! stack_list) {
		return;
	}
	if (! stack_list_num) {
		return;
	}

	dtime0(&now);
	
	X_LOCK;
	for (k=0; k < stack_list_num; k++) {
		Window win = stack_list[k].win;
		if (win != None && win < 10) {
			;	/* special, blackout */
		} else if (!valid_window(win, &attr, 1)) {
			stack_list[k].valid = 0;
		} else {
			stack_list[k].valid = 1;
			stack_list[k].x = attr.x;
			stack_list[k].y = attr.y;
			stack_list[k].width = attr.width;
			stack_list[k].height = attr.height;
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
	Window r, c;	
	int rx, ry, wx, wy;
	unsigned int mask;
	RAWFB_RET(None)
	if (start == None) {
		start = rootwin;
	}
	if (XQueryPointer(dpy, start, &r, &c, &rx, &ry, &wx, &wy, &mask)) {
		return c;
	} else {
		return None;
	}
}

unsigned int mask_state(void) {
	Window r, c;	
	int rx, ry, wx, wy;
	unsigned int mask;

	RAWFB_RET(0)

	if (XQueryPointer(dpy, rootwin, &r, &c, &rx, &ry, &wx, &wy, &mask)) {
		return mask;
	} else {
		return 0;
	}
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
				XFree(name);
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
					XFree(classhint->res_class);
				}
				if (classhint->res_name) {
					XFree(classhint->res_name);
				}
			}
		}
		if (! XQueryPointer(dpy, c, &r, &c, &rx, &ry, &wx, &wy, &m)) {
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
}


