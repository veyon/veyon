/* -- solid.c -- */

#include "x11vnc.h"
#include "win_utils.h"
#include "xwrappers.h"
#include "connections.h"

char *guess_desktop(void);
void solid_bg(int restore);


static void usr_bin_path(int restore);
static int dt_cmd(char *cmd);
static char *cmd_output(char *cmd);
static void solid_root(char *color);
static void solid_cde(char *color);
static void solid_gnome(char *color);
static void solid_kde(char *color);


static void usr_bin_path(int restore) {
	static char *oldpath = NULL;
	char *newpath;
	char addpath[] = "/usr/bin:/bin:";

	if (restore) {
		if (oldpath) {
			set_env("PATH", oldpath);
			free(oldpath);
			oldpath = NULL;
		}
		return;
	}

	if (getenv("PATH")) {
		oldpath = strdup(getenv("PATH"));
	} else {
		oldpath = strdup("/usr/bin");
	}
	newpath = (char *) malloc(strlen(oldpath) + strlen(addpath) + 1);
	newpath[0] = '\0';
	strcat(newpath, addpath);
	strcat(newpath, oldpath);
	set_env("PATH", newpath);
	free(newpath);
}

static int dt_cmd(char *cmd) {
	int rc;

	RAWFB_RET(0)

	if (!cmd || *cmd == '\0') {
		return 0;
	}

	/* dt */
	if (no_external_cmds || !cmd_ok("dt")) {
		rfbLog("cannot run external commands in -nocmds mode:\n");
		rfbLog("   \"%s\"\n", cmd);
		rfbLog("   dt_cmd: returning 1\n");
		return 1;
	}

	if (getenv("DISPLAY") == NULL) {
		set_env("DISPLAY", DisplayString(dpy));
	}

	rfbLog("running command:\n  %s\n", cmd);
	usr_bin_path(0);
	rc = system(cmd);
	usr_bin_path(1);

	if (rc >= 256) {
		rc = rc/256;
	}
	return rc;
}

static char *cmd_output(char *cmd) {
	FILE *p;
	static char output[50000];
	char line[1024];
	int rc;

	if (!cmd || *cmd == '\0') {
		return "";
	}

	if (no_external_cmds) {
		rfbLog("cannot run external commands in -nocmds mode:\n");
		rfbLog("   \"%s\"\n", cmd);
		rfbLog("   cmd_output: null string.\n");
		return "";
	}

	rfbLog("running pipe:\n  %s\n", cmd);
	usr_bin_path(0);
	p = popen(cmd, "r");
	usr_bin_path(1);

	output[0] = '\0';

	while (fgets(line, 1024, p) != NULL) {
		if (strlen(output) + strlen(line) + 1 < 50000) {
			strcat(output, line);
		}
	}
	rc = pclose(p);
	return(output);
}

static void solid_root(char *color) {
	Window expose;
	static XImage *image = NULL;
	Pixmap pixmap;
	XGCValues gcv;
	GC gc;
	XSetWindowAttributes swa;
	Visual visual;
	unsigned long mask, pixel;
	XColor cdef;
	Colormap cmap;

	RAWFB_RET_VOID

	if (subwin || window != rootwin) {
		rfbLog("cannot set subwin to solid color, must be rootwin\n");
		return;
	}

	/* create the "clear" window just for generating exposures */
	swa.override_redirect = True;
	swa.backing_store = NotUseful;
	swa.save_under = False;
	swa.background_pixmap = None;
	visual.visualid = CopyFromParent;
	mask = (CWOverrideRedirect|CWBackingStore|CWSaveUnder|CWBackPixmap);
	expose = XCreateWindow(dpy, window, 0, 0, wdpy_x, wdpy_y, 0, depth,
	    InputOutput, &visual, mask, &swa);

	if (! color) {
		/* restore the root window from the XImage snapshot */
		pixmap = XCreatePixmap(dpy, window, wdpy_x, wdpy_y, depth);

		if (! image) {
			/* whoops */
			XDestroyWindow(dpy, expose);
			rfbLog("no root snapshot available.\n");
			return;
		}

		
		/* draw the image to a pixmap: */
		gcv.function = GXcopy;
		gcv.plane_mask = AllPlanes;
		gc = XCreateGC(dpy, window, GCFunction|GCPlaneMask, &gcv);

		XPutImage(dpy, pixmap, gc, image, 0, 0, 0, 0, wdpy_x, wdpy_y);

		gcv.foreground = gcv.background = BlackPixel(dpy, scr);
		gc = XCreateGC(dpy, window, GCForeground|GCBackground, &gcv);

		rfbLog("restoring root snapshot...\n");
		/* set the pixmap as the bg: */
		XSetWindowBackgroundPixmap(dpy, window, pixmap);
		XFreePixmap(dpy, pixmap);
		XClearWindow(dpy, window);
		XFlush_wr(dpy);
		
		/* generate exposures */
		XMapWindow(dpy, expose);
		XSync(dpy, False);
		XDestroyWindow(dpy, expose);
		return;
	}

	if (! image) {
		/* need to retrieve a snapshot of the root background: */
		Window iwin;
		XSetWindowAttributes iswa;

		/* create image window: */
		iswa.override_redirect = True;
		iswa.backing_store = NotUseful;
		iswa.save_under = False;
		iswa.background_pixmap = ParentRelative;

		iwin = XCreateWindow(dpy, window, 0, 0, wdpy_x, wdpy_y, 0,
		    depth, InputOutput, &visual, mask, &iswa);

		rfbLog("snapshotting background...\n");

		XMapWindow(dpy, iwin);
		XSync(dpy, False);
		image = XGetImage(dpy, iwin, 0, 0, wdpy_x, wdpy_y, AllPlanes,
		    ZPixmap);
		XSync(dpy, False);
		XDestroyWindow(dpy, iwin);
	}

	/* use black for low colors or failure */
	pixel = BlackPixel(dpy, scr);
	if (depth > 8 || strcmp(color, solid_default)) {
		cmap = DefaultColormap (dpy, scr);
		if (XParseColor(dpy, cmap, color, &cdef) &&
		    XAllocColor(dpy, cmap, &cdef)) {
			pixel = cdef.pixel;
		} else {
			rfbLog("error parsing/allocing color: %s\n", color);
		}
	}

	rfbLog("setting solid background...\n");
	XSetWindowBackground(dpy, window, pixel);
	XMapWindow(dpy, expose);
	XSync(dpy, False);
	XDestroyWindow(dpy, expose);
}

static void solid_cde(char *color) {
	int wsmax = 16;
	static XImage *image[16];
	static Window ws_wins[16];
	static int nws = -1;

	Window expose;
	Pixmap pixmap;
	XGCValues gcv;
	GC gc;
	XSetWindowAttributes swa;
	Visual visual;
	unsigned long mask, pixel;
	XColor cdef;
	Colormap cmap;
	int n;

	RAWFB_RET_VOID

	if (subwin || window != rootwin) {
		rfbLog("cannot set subwin to solid color, must be rootwin\n");
		return;
	}

	/* create the "clear" window just for generating exposures */
	swa.override_redirect = True;
	swa.backing_store = NotUseful;
	swa.save_under = False;
	swa.background_pixmap = None;
	visual.visualid = CopyFromParent;
	mask = (CWOverrideRedirect|CWBackingStore|CWSaveUnder|CWBackPixmap);
	expose = XCreateWindow(dpy, window, 0, 0, wdpy_x, wdpy_y, 0, depth,
	    InputOutput, &visual, mask, &swa);

	if (! color) {
		/* restore the backdrop windows from the XImage snapshots */

		for (n=0; n < nws; n++) {
			Window twin;

			if (! image[n]) {
				continue;
			}

			twin = ws_wins[n];
			if (! twin) {
				twin = rootwin;
			}
			if (! valid_window(twin, NULL, 0)) {
				continue;
			}

			pixmap = XCreatePixmap(dpy, twin, wdpy_x, wdpy_y,
			    depth);
			
			/* draw the image to a pixmap: */
			gcv.function = GXcopy;
			gcv.plane_mask = AllPlanes;
			gc = XCreateGC(dpy, twin, GCFunction|GCPlaneMask, &gcv);

			XPutImage(dpy, pixmap, gc, image[n], 0, 0, 0, 0,
			    wdpy_x, wdpy_y);

			gcv.foreground = gcv.background = BlackPixel(dpy, scr);
			gc = XCreateGC(dpy, twin, GCForeground|GCBackground,
			    &gcv);

			rfbLog("restoring CDE ws%d snapshot to 0x%lx\n",
			    n, twin);
			/* set the pixmap as the bg: */
			XSetWindowBackgroundPixmap(dpy, twin, pixmap);
			XFreePixmap(dpy, pixmap);
			XClearWindow(dpy, twin);
			XFlush_wr(dpy);
		}
		
		/* generate exposures */
		XMapWindow(dpy, expose);
		XSync(dpy, False);
		XDestroyWindow(dpy, expose);
		return;
	}

	if (nws < 0) {
		/* need to retrieve snapshots of the ws backgrounds: */
		Window iwin, wm_win;
		XSetWindowAttributes iswa;
		Atom dt_list, wm_info, type;
		int format;
		unsigned long length, after;
		unsigned char *data;
		unsigned long *dp;	/* crash on 64bit with int */

		nws = 0;

		/* extract the hidden wm properties about backdrops: */

		wm_info = XInternAtom(dpy, "_MOTIF_WM_INFO", True);
		if (wm_info == None) {
			return;
		}

		XGetWindowProperty(dpy, rootwin, wm_info, 0L, 10L, False,
		    AnyPropertyType, &type, &format, &length, &after, &data);

		/*
		 * xprop -notype -root _MOTIF_WM_INFO
		 * _MOTIF_WM_INFO = 0x2, 0x580028
		 */

		if (length < 2 || format != 32 || after != 0) {
			return;
		}

		dp = (unsigned long *) data;
		wm_win = (Window) *(dp+1);	/* 2nd item. */


		dt_list = XInternAtom(dpy, "_DT_WORKSPACE_LIST", True);
		if (dt_list == None) {
			return;
		}

		XGetWindowProperty(dpy, wm_win, dt_list, 0L, 10L, False,
		   AnyPropertyType, &type, &format, &length, &after, &data);

		nws = length;

		if (nws > wsmax) {
			nws = wsmax;
		}
		if (nws < 0) {
			nws = 0;
		}

		rfbLog("special CDE win: 0x%lx, %d workspaces\n", wm_win, nws);
		if (nws == 0) {
			return;
		}

		for (n=0; n<nws; n++) {
			Atom ws_atom;
			char tmp[32];
			Window twin;
			XWindowAttributes attr;
			int i, cnt;

			image[n] = NULL;
			ws_wins[n] = 0x0;

			sprintf(tmp, "_DT_WORKSPACE_INFO_ws%d", n);
			ws_atom = XInternAtom(dpy, tmp, False);
			if (ws_atom == None) {
				continue;
			}
			XGetWindowProperty(dpy, wm_win, ws_atom, 0L, 100L,
			   False, AnyPropertyType, &type, &format, &length,
			   &after, &data);

			if (format != 8 || after != 0) {
				continue;
			}
			/*
			 * xprop -notype -id wm_win
			 * _DT_WORKSPACE_INFO_ws0 = "One", "3", "0x2f2f4a",
			 * "0x63639c", "0x103", "1", "0x58044e"
			 */

			cnt = 0;
			twin = 0x0;
			for (i=0; i< (int) length; i++) {
				if (*(data+i) != '\0') {
					continue;
				}
				cnt++;	/* count nulls to indicate field */
				if (cnt == 6) {
					/* one past the null: */
					char *q = (char *) (data+i+1);
					unsigned long in;
					if (sscanf(q, "0x%lx", &in) == 1) {
						twin = (Window) in;
						break;
					}
				}
			}
			ws_wins[n] = twin;

			if (! twin) {
				twin = rootwin;
			}

			XGetWindowAttributes(dpy, twin, &attr);
			if (twin != rootwin) {
				if (attr.map_state != IsViewable) {
					XMapWindow(dpy, twin);
				}
				XRaiseWindow(dpy, twin);
			}
			XSync(dpy, False);
		
			/* create image window: */
			iswa.override_redirect = True;
			iswa.backing_store = NotUseful;
			iswa.save_under = False;
			iswa.background_pixmap = ParentRelative;
			visual.visualid = CopyFromParent;

			iwin = XCreateWindow(dpy, twin, 0, 0, wdpy_x, wdpy_y,
			    0, depth, InputOutput, &visual, mask, &iswa);

			rfbLog("snapshotting CDE backdrop ws%d 0x%lx -> "
			    "0x%lx ...\n", n, twin, iwin);
			XMapWindow(dpy, iwin);
			XSync(dpy, False);

			image[n] = XGetImage(dpy, iwin, 0, 0, wdpy_x, wdpy_y,
			    AllPlanes, ZPixmap);
			XSync(dpy, False);
			XDestroyWindow(dpy, iwin);
			if (twin != rootwin) {
				XLowerWindow(dpy, twin);
				if (attr.map_state != IsViewable) {
					XUnmapWindow(dpy, twin);
				}
			}
		}
	}
	if (nws == 0) {
		return;
	}

	/* use black for low colors or failure */
	pixel = BlackPixel(dpy, scr);
	if (depth > 8 || strcmp(color, solid_default)) {
		cmap = DefaultColormap (dpy, scr);
		if (XParseColor(dpy, cmap, color, &cdef) &&
		    XAllocColor(dpy, cmap, &cdef)) {
			pixel = cdef.pixel;
		} else {
			rfbLog("error parsing/allocing color: %s\n", color);
		}
	}

	rfbLog("setting solid backgrounds...\n");

	for (n=0; n < nws; n++)  {
		Window twin = ws_wins[n];
		if (image[n] == NULL) {
			continue;
		}
		if (! twin)  {
			twin = rootwin;
		}
		XSetWindowBackground(dpy, twin, pixel);
	}
	XMapWindow(dpy, expose);
	XSync(dpy, False);
	XDestroyWindow(dpy, expose);
}

static void solid_gnome(char *color) {
	char get_color[] = "gconftool-2 --get "
	    "/desktop/gnome/background/primary_color";
	char set_color[] = "gconftool-2 --set "
	    "/desktop/gnome/background/primary_color --type string '%s'";
	char get_option[] = "gconftool-2 --get "
	    "/desktop/gnome/background/picture_options";
	char set_option[] = "gconftool-2 --set "
	    "/desktop/gnome/background/picture_options --type string '%s'";
	static char *orig_color = NULL;
	static char *orig_option = NULL;
	char *cmd;

	RAWFB_RET_VOID
	
	if (! color) {
		if (! orig_color) {
			orig_color = strdup("#FFFFFF");
		}
		if (! orig_option) {
			orig_option = strdup("stretched");
		}
		if (strstr(orig_color, "'") != NULL)  {
			rfbLog("invalid color: %s\n", orig_color);
			return;
		}
		if (strstr(orig_option, "'") != NULL)  {
			rfbLog("invalid option: %s\n", orig_option);
			return;
		}
		cmd = (char *) malloc(strlen(set_option) - 2 +
		    strlen(orig_option) + 1);
		sprintf(cmd, set_option, orig_option);
		dt_cmd(cmd);
		free(cmd);
		cmd = (char *) malloc(strlen(set_color) - 2 +
		    strlen(orig_color) + 1);
		sprintf(cmd, set_color, orig_color);
		dt_cmd(cmd);
		free(cmd);
		return;
	}

	if (! orig_color) {
		char *q;
		if (cmd_ok("dt")) {
			orig_color = strdup(cmd_output(get_color));
		} else {
			orig_color = "";
		}
		if (*orig_color == '\0') {
			orig_color = strdup("#FFFFFF");
		}
		if ((q = strchr(orig_color, '\n')) != NULL) {
			*q = '\0';
		}
	}
	if (! orig_option) {
		char *q;
		if (cmd_ok("dt")) {
			orig_option = strdup(cmd_output(get_option));
		} else {
			orig_color = "";
		}
		if (*orig_option == '\0') {
			orig_option = strdup("stretched");
		}
		if ((q = strchr(orig_option, '\n')) != NULL) {
			*q = '\0';
		}
	}
	if (strstr(color, "'") != NULL)  {
		rfbLog("invalid color: %s\n", color);
		return;
	}
	cmd = (char *) malloc(strlen(set_color) + strlen(color) + 1);
	sprintf(cmd, set_color, color);
	dt_cmd(cmd);
	free(cmd);

	cmd = (char *) malloc(strlen(set_option) + strlen("none") + 1);
	sprintf(cmd, set_option, "none");
	dt_cmd(cmd);
	free(cmd);
}

static void solid_kde(char *color) {
	char set_color[] =
	    "dcop --user '%s' kdesktop KBackgroundIface setColor '%s' 1";
	char bg_off[] =
	    "dcop --user '%s' kdesktop KBackgroundIface setBackgroundEnabled 0";
	char bg_on[] =
	    "dcop --user '%s' kdesktop KBackgroundIface setBackgroundEnabled 1";
	char *cmd, *user = NULL;
	int len;

	RAWFB_RET_VOID

	user = get_user_name();
	if (strstr(user, "'") != NULL)  {
		rfbLog("invalid user: %s\n", user);
		free(user);
		return;
	}

	if (! color) {
		len = strlen(bg_on) + strlen(user) + 1;
		cmd = (char *) malloc(len);
		sprintf(cmd, bg_on, user);
		dt_cmd(cmd);
		free(cmd);
		free(user);

		return;
	}

	if (strstr(color, "'") != NULL)  {
		rfbLog("invalid color: %s\n", color);
		return;
	}

	len = strlen(set_color) + strlen(user) + strlen(color) + 1;
	cmd = (char *) malloc(len);
	sprintf(cmd, set_color, user, color);
	dt_cmd(cmd);
	free(cmd);

	len = strlen(bg_off) + strlen(user) + 1;
	cmd = (char *) malloc(len);
	sprintf(cmd, bg_off, user);
	dt_cmd(cmd);
	free(cmd);
	free(user);
}

char *guess_desktop(void) {
	Atom prop;

	RAWFB_RET("root")

	if (wmdt_str && *wmdt_str != '\0') {
		char *s = wmdt_str;
		lowercase(s);
		if (strstr(s, "xfce")) {
			return "xfce";
		}
		if (strstr(s, "gnome") || strstr(s, "metacity")) {
			return "gnome";
		}
		if (strstr(s, "kde") || strstr(s, "kwin")) {
			return "kde";
		}
		if (strstr(s, "cde")) {
			return "cde";
		}
		return "root";
	}

	if (! dpy) {
		return "";
	}

	prop = XInternAtom(dpy, "XFCE_DESKTOP_WINDOW", True);
	if (prop != None) return "xfce";

	/* special case windowmaker */
	prop = XInternAtom(dpy, "_WINDOWMAKER_WM_PROTOCOLS", True);
	if (prop != None)  return "root";

	prop = XInternAtom(dpy, "_WINDOWMAKER_COMMAND", True);
	if (prop != None) return "root";

	prop = XInternAtom(dpy, "NAUTILUS_DESKTOP_WINDOW_ID", True);
	if (prop != None) return "gnome";

	prop = XInternAtom(dpy, "KWIN_RUNNING", True);
	if (prop != None) return "kde";

	prop = XInternAtom(dpy, "_MOTIF_WM_INFO", True);
	if (prop != None) {
		prop = XInternAtom(dpy, "_DT_WORKSPACE_LIST", True);
		if (prop != None) return "cde";
	}
	return "root";
}

void solid_bg(int restore) {
	static int desktop = -1;
	static int solid_on = 0;
	static char *prev_str;
	char *dtname, *color;

	RAWFB_RET_VOID

	if (started_as_root == 1 && users_list) {
		/* we are still root, don't try. */
		return;
	}

	if (restore) {
		if (! solid_on) {
			return;
		}
		if (desktop == 0) {
			solid_root(NULL);
		} else if (desktop == 1) {
			solid_gnome(NULL);
		} else if (desktop == 2) {
			solid_kde(NULL);
		} else if (desktop == 3) {
			solid_cde(NULL);
		}
		solid_on = 0;
		return;
	}
	if (! solid_str) {
		return;
	}
	if (solid_on && !strcmp(prev_str, solid_str)) {
		return;
	}
	if (strstr(solid_str, "guess:") == solid_str
	    || !strchr(solid_str, ':')) {
		dtname = guess_desktop();
		rfbLog("guessed desktop: %s\n", dtname);
	} else {
		if (strstr(solid_str, "gnome:") == solid_str) {
			dtname = "gnome";
		} else if (strstr(solid_str, "kde:") == solid_str) {
			dtname = "kde";
		} else if (strstr(solid_str, "cde:") == solid_str) {
			dtname = "cde";
		} else {
			dtname = "root";
		}
	}

	color = strchr(solid_str, ':');
	if (! color) {
		color = solid_str;
	} else {
		color++;
		if (*color == '\0') {
			color = solid_default;
		}
	}
	if (!strcmp(dtname, "gnome")) {
		desktop = 1;
		solid_gnome(color);
	} else if (!strcmp(dtname, "kde")) {
		desktop = 2;
		solid_kde(color);
	} else if (!strcmp(dtname, "cde")) {
		desktop = 3;
		solid_cde(color);
	} else {
		desktop = 0;
		solid_root(color);
	}
	if (prev_str) {
		free(prev_str);
	}
	prev_str = strdup(solid_str);
	solid_on = 1;
}


