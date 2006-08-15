/*
 * x11vnc: a VNC server for X displays.
 *
 * Copyright (c) 2002-2006 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 *
 *
 * This program is based on the following programs:
 *
 *       the originial x11vnc.c in libvncserver (Johannes E. Schindelin)
 *	 x0rfbserver, the original native X vnc server (Jens Wagner)
 *       krfb, the KDE desktopsharing project (Tim Jansen)
 *
 * The primary goal of this program is to create a portable and simple
 * command-line server utility that allows a VNC viewer to connect
 * to an actual X display (as the above do).  The only non-standard
 * dependency of this program is the static library libvncserver.a.
 * Although in some environments libjpeg.so or libz.so may not be
 * readily available and needs to be installed, they may be found
 * at ftp://ftp.uu.net/graphics/jpeg/ and http://www.gzip.org/zlib/,
 * respectively.  To increase portability it is written in plain C.
 *
 * Another goal is to improve performance and interactive response.
 * The algorithm of x0rfbserver was used as a base.  Many additional
 * heuristics are also applied.
 *
 * Another goal is to add many features that enable and incourage creative
 * usage and application of the tool.  Apologies for the large number
 * of options!
 *
 * To build:
 *
 * Obtain the libvncserver package (http://libvncserver.sourceforge.net).
 * As of 12/2002 this version of x11vnc.c is contained in the libvncserver
 * CVS tree and released in version 0.5.
 *
 * gcc should be used on all platforms.  To build a threaded version put
 * "-D_REENTRANT -DX11VNC_THREADED" in the environment variable CFLAGS
 * or CPPFLAGS (e.g. before running the libvncserver configure).  The
 * threaded mode is a bit more responsive, but can be unstable (e.g.
 * if more than one client the same tight or zrle encoding).
 *
 * Known shortcomings:
 *
 * The screen updates are good, but of course not perfect since the X
 * display must be continuously polled and read for changes and this is
 * slow for most hardware. This can be contrasted with receiving a change
 * callback from the X server, if that were generally possible... (UPDATE:
 * this is handled now with the X DAMAGE extension, but unfortunately
 * that doesn't seem to address the slow read from the video h/w).  So,
 * e.g., opaque moves and similar window activity can be very painful;
 * one has to modify one's behavior a bit.
 *
 * General audio at the remote display is lost unless one separately
 * sets up some audio side-channel such as esd.
 *
 * It does not appear possible to query the X server for the current
 * cursor shape.  We can use XTest to compare cursor to current window's
 * cursor, but we cannot extract what the cursor is... (UPDATE: we now
 * use XFIXES extension for this.  Also on Solaris and IRIX Overlay
 * extensions exists that allow drawing the mouse into the framebuffer)
 * 
 * The current *position* of the remote X mouse pointer is shown with
 * the -cursor option.  Further, if -cursorX or -X is used, a trick
 * is done to at least show the root window cursor vs non-root cursor.
 * (perhaps some heuristic can be done to further distinguish cases...,
 * currently "-cursor some" is a first hack at this)
 *
 * Under XFIXES mode for showing the cursor shape, the cursor may be
 * poorly approximated if it has transparency (alpha channel).
 *
 * Windows using visuals other than the default X visual may have
 * their colors messed up.  When using 8bpp indexed color, the colormap
 * is attempted to be followed, but may become out of date.  Use the
 * -flashcmap option to have colormap flashing as the pointer moves
 * windows with private colormaps (slow).  Displays with mixed depth 8 and
 * 24 visuals will incorrectly display windows using the non-default one.
 * On Sun and Sgi hardware we can to work around this with -overlay.
 *
 * Feature -id <windowid> can be picky: it can crash for things like
 * the window not sufficiently mapped into server memory, etc (UPDATE:
 * we now use the -xrandr mechanisms to trap errors more robustly for
 * this mode).  SaveUnders menus, popups, etc will not be seen.
 *
 * Under some situations the keysym unmapping is not correct, especially
 * if the two keyboards correspond to different languages.  The -modtweak
 * option is the default and corrects most problems. One can use the
 * -xkb option to try to use the XKEYBOARD extension to clear up any
 * remaining problems.
 *
 * Occasionally, a few tile updates can be missed leaving a patch of
 * color that needs to be refreshed.  This may only be when threaded,
 * which is no longer the default.
 *
 * There seems to be a serious bug with simultaneous clients when
 * threaded, currently the only workaround in this case is -nothreads
 * (which is now the default).
 *
 */


/* -- x11vnc.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xdamage.h"
#include "xrecord.h"
#include "xevents.h"
#include "xinerama.h"
#include "xrandr.h"
#include "xkb_bell.h"
#include "win_utils.h"
#include "remote.h"
#include "scan.h"
#include "gui.h"
#include "help.h"
#include "user.h"
#include "cleanup.h"
#include "keyboard.h"
#include "pointer.h"
#include "cursor.h"
#include "userinput.h"
#include "screen.h"
#include "connections.h"
#include "rates.h"
#include "unixpw.h"
#include "inet.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "selection.h"
#include "pm.h"
#include "solid.h"

/*
 * main routine for the x11vnc program
 */

static void check_cursor_changes(void);
static void record_last_fb_update(void);
static int choose_delay(double dt);
static void watch_loop(void);
static int limit_shm(void);
static void check_rcfile(int argc, char **argv);
static void immediate_switch_user(int argc, char* argv[]);
static void print_settings(int try_http, int bg, char *gui_str);
static void check_loop_mode(int argc, char* argv[]);


static void check_cursor_changes(void) {
	static double last_push = 0.0;

	if (unixpw_in_progress) return;

	cursor_changes += check_x11_pointer();

	if (cursor_changes) {
		double tm, max_push = 0.125, multi_push = 0.01, wait = 0.02;
		int cursor_shape, dopush = 0, link, latency, netrate;

		if (! all_clients_initialized()) {
			/* play it safe */
			return;
		}

		if (0) cursor_shape = cursor_shape_updates_clients(screen);
	
		dtime0(&tm);
		link = link_rate(&latency, &netrate);
		if (link == LR_DIALUP) {
			max_push = 0.2;
			wait = 0.05;
		} else if (link == LR_BROADBAND) {
			max_push = 0.075;
			wait = 0.05;
		} else if (link == LR_LAN) {
			max_push = 0.01;
		} else if (latency < 5 && netrate > 200) {
			max_push = 0.01;
		}
		
		if (tm > last_push + max_push) {
			dopush = 1;
		} else if (cursor_changes > 1 && tm > last_push + multi_push) {
			dopush = 1;
		}

		if (dopush) { 
			mark_rect_as_modified(0, 0, 1, 1, 1);
			fb_push_wait(wait, FB_MOD);
			last_push = tm;
		} else {
			rfbPE(0);
		}
	}
	cursor_changes = 0;
}

static void record_last_fb_update(void) {
	static int rbs0 = -1;
	static time_t last_call = 0;
	time_t now = time(NULL);
	int rbs = -1;
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	if (last_fb_bytes_sent == 0) {
		last_fb_bytes_sent = now;
		last_call = now;
	}

	if (now <= last_call + 1) {
		/* check every second or so */
		return;
	}

	if (unixpw_in_progress) return;

	last_call = now;

	if (! screen) {
		return;
	}

	iter = rfbGetClientIterator(screen);
	while( (cl = rfbClientIteratorNext(iter)) ) {
#if 0
		rbs += cl->rawBytesEquivalent;
#else
		rbs += rfbStatGetSentBytesIfRaw(cl);
#endif
	}
	rfbReleaseClientIterator(iter);

	if (rbs != rbs0) {
		rbs0 = rbs;
		if (debug_tiles > 1) {
			printf("record_last_fb_update: %d %d\n",
			    (int) now, (int) last_fb_bytes_sent);
		}
		last_fb_bytes_sent = now;
	}
}

static int choose_delay(double dt) {
	static double t0 = 0.0, t1 = 0.0, t2 = 0.0, now; 
	static int x0, y0, x1, y1, x2, y2, first = 1;
	int dx0, dy0, dx1, dy1, dm, i, msec = waitms;
	double cut1 = 0.15, cut2 = 0.075, cut3 = 0.25;
	double bogdown_time = 0.25, bave = 0.0;
	int bogdown = 1, bcnt = 0;
	int ndt = 8, nave = 3;
	double fac = 1.0;
	int db = 0;
	static double dts[8];

	if (waitms == 0) {
		return waitms;
	}
	if (nofb) {
		return waitms;
	}

	if (first) {
		for(i=0; i<ndt; i++) {
			dts[i] = 0.0;
		}
		first = 0;
	}

	now = dnow();

	/*
	 * first check for bogdown, e.g. lots of activity, scrolling text
	 * from command output, etc.
	 */
	if (nap_ok) {
		dt = 0.0;
	}
	if (! wait_bog) {
		bogdown = 0;

	} else if (button_mask || now < last_keyboard_time + 2*bogdown_time) {
		/*
		 * let scrolls & keyboard input through the normal way
		 * otherwise, it will likely just annoy them.
		 */
		bogdown = 0;

	} else if (dt > 0.0) {
		/*
		 * inspect recent dt's:
		 * 0 1 2 3 4 5 6 7 dt
		 *             ^ ^ ^
		 */
		for (i = ndt - (nave - 1); i < ndt; i++) {
			bave += dts[i];
			bcnt++;
			if (dts[i] < bogdown_time) {
				bogdown = 0;
				break;
			}
		}
		bave += dt;
		bcnt++;
		bave = bave / bcnt;
		if (dt < bogdown_time) {
			bogdown = 0;
		}
	} else {
		bogdown = 0;
	}
	/* shift for next time */
	for (i = 0; i < ndt-1; i++) {
		dts[i] = dts[i+1];
	}
	dts[ndt-1] = dt;

if (0 && dt > 0.0) fprintf(stderr, "dt: %.5f %.4f\n", dt, dnow() - x11vnc_start);
	if (bogdown) {
		if (use_xdamage) {
			/* DAMAGE can queue ~1000 rectangles for a scroll */
			clear_xdamage_mark_region(NULL, 0);
		}
		msec = (int) (1000 * 1.75 * bave);
		if (dts[ndt - nave - 1] > 0.75 * bave) {
			msec = 1.5 * msec;
			set_xdamage_mark(0, 0, dpy_x, dpy_y);
		}
		if (msec > 1500) {
			msec = 1500;
		}
		if (msec < waitms) {
			msec = waitms;
		}
		db = (db || debug_tiles);
		if (db) fprintf(stderr, "bogg[%d] %.3f %.3f %.3f %.3f\n",
		    msec, dts[ndt-4], dts[ndt-3], dts[ndt-2], dts[ndt-1]);
		return msec;
	}

	/* next check for pointer motion, keystrokes, to speed up */
	t2 = dnow();
	x2 = cursor_x;
	y2 = cursor_y;

	dx0 = nabs(x1 - x0);
	dy0 = nabs(y1 - y0);
	dx1 = nabs(x2 - x1);
	dy1 = nabs(y2 - y1);
	if (dx1 > dy1) {
		dm = dx1;
	} else {
		dm = dy1;
	}

	if ((dx0 || dy0) && (dx1 || dy1)) {
		if (t2 < t0 + cut1 || t2 < t1 + cut2 || dm > 20) {
			fac = wait_ui * 1.25;
		}
	} else if ((dx1 || dy1) && dm > 40) {
		fac = wait_ui;
	}

	if (fac == 1 && t2 < last_keyboard_time + cut3) {
		fac = wait_ui;
	}
	msec = (int) ((double) waitms / fac);
	if (msec == 0) {
		msec = 1;
	}

	x0 = x1;
	y0 = y1;
	t0 = t1;

	x1 = x2;
	y1 = y2;
	t1 = t2;

	return msec;
}

/*
 * main x11vnc loop: polls, checks for events, iterate libvncserver, etc.
 */
static void watch_loop(void) {
	int cnt = 0, tile_diffs = 0, skip_pe = 0;
	double tm, dtr, dt = 0.0;
	time_t start = time(NULL);

	if (use_threads) {
		rfbRunEventLoop(screen, -1, TRUE);
	}

	while (1) {
		char msg[] = "new client: %s taking unixpw client off hold.\n";

		got_user_input = 0;
		got_pointer_input = 0;
		got_pointer_calls = 0;
		got_keyboard_input = 0;
		got_keyboard_calls = 0;
		urgent_update = 0;

		if (! use_threads) {
			dtime0(&tm);
			if (! skip_pe) {
				if (unixpw && unixpw_in_progress) {
					rfbClientPtr cl = unixpw_client;
					if (cl && cl->onHold) {
						rfbLog(msg, cl->host);
						unixpw_client->onHold = FALSE;
					}
				} else {
					measure_send_rates(1);
				}

				unixpw_in_rfbPE = 1;

				rfbPE(-1);

				unixpw_in_rfbPE = 0;

				if (unixpw && unixpw_in_progress) {
					/* rfbPE loop until logged in. */
					skip_pe = 0;
					continue;
				} else {
					measure_send_rates(0);
					fb_update_sent(NULL);
				}
			} else {
				if (unixpw && unixpw_in_progress) {
					skip_pe = 0;
					continue;
				}
			}
			dtr = dtime(&tm);

			if (! cursor_shape_updates) {
				/* undo any cursor shape requests */
				disable_cursor_shape_updates(screen);
			}
			if (screen && screen->clientHead) {
				int ret = check_user_input(dt, dtr,
				    tile_diffs, &cnt);
				/* true: loop back for more input */
				if (ret == 2) {
					skip_pe = 1;
				}
				if (ret) {
					if (debug_scroll) fprintf(stderr, "watch_loop: LOOP-BACK: %d\n", ret);
					continue;
				}
			}
			/* watch for viewonly input piling up: */
			if ((got_pointer_calls > got_pointer_input) ||
			    (got_keyboard_calls > got_keyboard_input)) {
				eat_viewonly_input(10, 3);
			}
		} else {
#if 0
			if (0 && use_xrecord) {
				/* XXX not working */
				check_xrecord();
			}
#endif
			if (wireframe && button_mask) {
				check_wireframe();
			}
		}
		skip_pe = 0;

		if (shut_down) {
			clean_up_exit(0);
		}

		if (unixpw_in_progress) continue;

		if (! urgent_update) {
			if (do_copy_screen) {
				do_copy_screen = 0;
				copy_screen();
			}

			check_new_clients();
			check_xevents(0);
			check_autorepeat();
			check_pm();
			check_keycode_state();
			check_connect_inputs();		
			check_gui_inputs();		
			check_stunnel();		
			check_openssl();		
			check_https();		
			record_last_fb_update();
			check_padded_fb();		
			check_fixscreen();		
			check_xdamage_state();
			check_xrecord_reset(0);
			check_add_keysyms();
			check_new_passwds();
			if (started_as_root) {
				check_switched_user();
			}

			if (first_conn_timeout < 0) {
				start = time(NULL);
				first_conn_timeout = -first_conn_timeout;
			}
		}

		if (! screen || ! screen->clientHead) {
			/* waiting for a client */
			if (first_conn_timeout) {
				if (time(NULL) - start > first_conn_timeout) {
					rfbLog("No client after %d secs.\n",
					    first_conn_timeout);
					shut_down = 1;
				}
			}
			usleep(200 * 1000);
			continue;
		}

		if (first_conn_timeout && all_clients_initialized()) {
			first_conn_timeout = 0;
		}

		if (nofb) {
			/* no framebuffer polling needed */
			if (cursor_pos_updates) {
				check_x11_pointer();
			}
			continue;
		}

		if (button_mask && (!show_dragging || pointer_mode == 0)) {
			/*
			 * if any button is pressed do not update rfb
			 * screen, but do flush the X11 display.
			 */
			X_LOCK;
			XFlush_wr(dpy);
			X_UNLOCK;
			dt = 0.0;
		} else {
			static double last_dt = 0.0;
			double xdamage_thrash = 0.4; 

			check_cursor_changes();

			/* for timing the scan to try to detect thrashing */

			if (use_xdamage && last_dt > xdamage_thrash)  {
				clear_xdamage_mark_region(NULL, 0);
			}

			if (unixpw_in_progress) continue;
			dtime0(&tm);
			if (use_snapfb) {
				int t, tries = 3;
				copy_snap();
				for (t =0; t < tries; t++) {
					tile_diffs = scan_for_updates(0);
				}
			} else {
				tile_diffs = scan_for_updates(0);
			}
			dt = dtime(&tm);
			if (! nap_ok) {
				last_dt = dt;
			}

 if ((debug_tiles || debug_scroll > 1 || debug_wireframe > 1)
    && (tile_diffs > 4 || debug_tiles > 1)) {
	double rate = (tile_x * tile_y * bpp/8 * tile_diffs) / dt;
	fprintf(stderr, "============================= TILES: %d  dt: %.4f"
	    "  t: %.4f  %.2f MB/s nap_ok: %d\n", tile_diffs, dt,
	    tm - x11vnc_start, rate/1000000.0, nap_ok);
 }

		}

		/* sleep a bit to lessen load */
		if (! urgent_update) {
			int wait = choose_delay(dt);
			if (wait > 2*waitms) {
				/* bog case, break it up */
				nap_sleep(wait, 10);
			} else {
				usleep(wait * 1000);
			}
		}
		cnt++;
	}
}

/* 
 * check blacklist for OSs with tight shm limits.
 */
static int limit_shm(void) {
	int limit = 0;

	if (UT.sysname == NULL) {
		return 0;
	}
	if (!strcmp(UT.sysname, "SunOS")) {
		char *r = UT.release;
		if (*r == '5' && *(r+1) == '.') {
			if (strchr("2345678", *(r+2)) != NULL) {
				limit = 1;
			}
		}
	} else if (!strcmp(UT.sysname, "Darwin")) {
		limit = 1;
	}
	if (limit && ! quiet) {
		fprintf(stderr, "reducing shm usage on %s %s (adding "
		    "-onetile)\n", UT.sysname, UT.release);
	}
	return limit;
}


/*
 * quick-n-dirty ~/.x11vncrc: each line (except # comments) is a cmdline option.
 */
static int argc2 = 0;
static char **argv2;

static void check_rcfile(int argc, char **argv) {
	int i, j, pwlast, norc = 0, argmax = 1024;
	char *infile = NULL;
	char rcfile[1024];
	FILE *rc = NULL; 

	for (i=1; i < argc; i++) {
		if (!strcmp(argv[i], "-printgui")) {
			fprintf(stdout, "%s", get_gui_code());
			fflush(stdout);
			exit(0);
		}
		if (!strcmp(argv[i], "-norc")) {
			norc = 1;
			got_norc = 1;
		}
		if (!strcmp(argv[i], "-QD")) {
			norc = 1;
		}
		if (!strcmp(argv[i], "-rc")) {
			if (i+1 >= argc) {
				fprintf(stderr, "-rc option requires a "
				    "filename\n");
				exit(1);
			} else {
				infile = argv[i+1];
			}
		}
	}
	rc_norc = norc;
	rc_rcfile = strdup("");
	if (norc) {
		;
	} else if (infile != NULL) {
		rc = fopen(infile, "r");
		rc_rcfile = strdup(infile);
		if (rc == NULL) {
			fprintf(stderr, "could not open rcfile: %s\n", infile);
			perror("fopen");
			exit(1);
		}
	} else {
		char *home = get_home_dir();
		if (! home) {
			norc = 1;
		} else {
			strncpy(rcfile, home, 500);
			free(home);

			strcat(rcfile, "/.x11vncrc");
			infile = rcfile;
			rc = fopen(rcfile, "r");
			if (rc == NULL) {
				norc = 1;
			} else {
				rc_rcfile = strdup(rcfile);
				rc_rcfile_default = 1;
			}
		}
	}

	argv2 = (char **) malloc(argmax * sizeof(char *));
	argv2[argc2++] = strdup(argv[0]);

	if (! norc) {
		char line[4096], parm[100], tmp[101];
		char *buf, *tbuf;
		struct stat sbuf;
		int sz;

		if (fstat(fileno(rc), &sbuf) != 0) {
			fprintf(stderr, "problem with %s\n", infile);
			perror("fstat");
			exit(1);
		}
		sz = sbuf.st_size+1;	/* allocate whole file size */
		if (sz < 1024) {
			sz = 1024;
		}

		buf = (char *) malloc(sz);
		buf[0] = '\0';

		while (fgets(line, 4096, rc) != NULL) {
			char *q, *p = line;
			char c;
			int cont = 0;

			q = p;
			c = '\0';
			while (*q) {
				if (*q == '#') {
					if (c != '\\') {
						*q = '\0';
						break;
					}
				}
				c = *q;
				q++;
			}

			q = p;
			c = '\0';
			while (*q) {
				if (*q == '\n') {
					if (c == '\\') {
						cont = 1;
						*q = '\0';
						*(q-1) = ' ';
						break;
					}
					while (isspace((unsigned char) (*q))) {
						*q = '\0';
						if (q == p) {
							break;
						}
						q--;
					}
					break;
				}
				c = *q;
				q++;
			}
			if (q != p && !cont) {
				if (*q == '\0') {
					q--;
				}
				while (isspace((unsigned char) (*q))) {
					*q = '\0';
					if (q == p) {
						break;
					}
					q--;
				}
			}

			p = lblanks(p);

			strncat(buf, p, sz - strlen(buf) - 1);
			if (cont) {
				continue;
			}
			if (buf[0] == '\0') {
				continue;
			}

			i = 0;
			q = buf;
			while (*q) {
				i++;
				if (*q == '\n' || isspace((unsigned char) (*q))) {
					break;
				}
				q++;
			}

			if (i >= 100) {
				fprintf(stderr, "invalid rcfile line: %s/%s\n",
				    p, buf);
				exit(1);
			}

			if (sscanf(buf, "%s", parm) != 1) {
				fprintf(stderr, "invalid rcfile line: %s\n", p);
				exit(1);
			}
			if (parm[0] == '-') {
				strncpy(tmp, parm, 100); 
			} else {
				tmp[0] = '-';
				strncpy(tmp+1, parm, 100); 
			}

			argv2[argc2++] = strdup(tmp);
			if (argc2 >= argmax) {
				fprintf(stderr, "too many rcfile options\n");
				exit(1);
			}
			
			p = buf;
			p += strlen(parm);
			p = lblanks(p);

			if (*p == '\0') {
				buf[0] = '\0';
				continue;
			}

			tbuf = (char *) calloc(strlen(p) + 1, 1);

			j = 0;
			while (*p) {
				if (*p == '\\' && *(p+1) == '#') {
					;
				} else {
					tbuf[j++] = *p;
				}
				p++;
			}

			argv2[argc2++] = strdup(tbuf);
			free(tbuf);
			if (argc2 >= argmax) {
				fprintf(stderr, "too many rcfile options\n");
				exit(1);
			}
			buf[0] = '\0';
		}
		fclose(rc);
		free(buf);
	}
	pwlast = 0;
	for (i=1; i < argc; i++) {
		argv2[argc2++] = strdup(argv[i]);

		if (pwlast || !strcmp("-passwd", argv[i])
		    || !strcmp("-viewpasswd", argv[i])) {
			char *p = argv[i];		
			if (pwlast) {
				pwlast = 0;
			} else {
				pwlast = 1;
			}
			strzero(p);
		}
		if (argc2 >= argmax) {
			fprintf(stderr, "too many rcfile options\n");
			exit(1);
		}
	}
}

static void immediate_switch_user(int argc, char* argv[]) {
	int i;
	for (i=1; i < argc; i++) {
		char *u;

		if (strcmp(argv[i], "-users")) {
			continue;
		}
		if (i == argc - 1) {
			fprintf(stderr, "not enough arguments for: -users\n");
			exit(1);
		}
		if (*(argv[i+1]) != '=') {
			break;
		}

		/* wants an immediate switch: =bob */
		u = strdup(argv[i+1]);
		*u = '+';
		if (strstr(u, "+guess") == u) {
			fprintf(stderr, "invalid user: %s\n", u+1);
			exit(1);
		}
		if (!switch_user(u, 0)) {
			fprintf(stderr, "Could not switch to user: %s\n", u+1);
			exit(1);
		} else {
			fprintf(stderr, "Switched to user: %s\n", u+1);
			started_as_root = 2;
		}
		free(u);
		break;
	}
}

static void quick_pw(char *str) {
	char *p, *q;
	char tmp[1024];
	int db = 0;

	if (db) fprintf(stderr, "quick_pw: %s\n", str);

	if (! str || str[0] == '\0') {
		exit(1);
	}
	if (str[0] != '%') {
		exit(1);
	}
	/*
	 * "%-" or "%stdin" means read one line from stdin.
	 *
	 * "%env" means it is in $UNIXPW env var.
	 *
	 * starting "%/" or "%." means read the first line from that file.
	 *
	 * "%%" or "%" means prompt user.
	 *
	 * otherwise: %user:pass
	 */
	if (!strcmp(str, "%-") || !strcmp(str, "%stdin")) {
		if(fgets(tmp, 1024, stdin) == NULL) {
			exit(1);
		}
		q = strdup(tmp);
	} else if (!strcmp(str, "%env")) {
		if (getenv("UNIXPW") == NULL) {
			exit(1);
		}
		q = strdup(getenv("UNIXPW"));
	} else if (!strcmp(str, "%%") || !strcmp(str, "%")) {
		char *t, inp[1024];
		fprintf(stdout, "username: ");
		if(fgets(tmp, 128, stdin) == NULL) {
			exit(1);
		}
		strcpy(inp, tmp);
		t = strchr(inp, '\n');
		if (t) {
			*t = ':'; 
		} else {
			strcat(inp, ":");
			
		}
		fprintf(stdout, "password: ");
		/* test mode: no_external_cmds does not apply */
		system("stty -echo");
		if(fgets(tmp, 128, stdin) == NULL) {
			fprintf(stdout, "\n");
			system("stty echo");
			exit(1);
		}
		system("stty echo");
		fprintf(stdout, "\n");
		strcat(inp, tmp);
		q = strdup(inp);
	} else if (str[1] == '/' || str[1] == '.') {
		FILE *in = fopen(str+1, "r");
		if (in == NULL) {
			exit(1);
		}
		if(fgets(tmp, 1024, in) == NULL) {
			exit(1);
		}
		q = strdup(tmp);
	} else {
		q = strdup(str+1);
	}
	p = (char *) malloc(strlen(q) + 10);
	strcpy(p, q);
	if (strchr(p, '\n') == NULL) {
		strcat(p, "\n");
	}

	if ((q = strchr(p, ':')) == NULL) {
		exit(1);
	}
	*q = '\0';
	if (db) fprintf(stderr, "'%s' '%s'\n", p, q+1);
	if (unixpw_nis) {
		if (crypt_verify(p, q+1)) {
			fprintf(stdout, "Y %s\n", p);
			exit(0);
		} else {
			fprintf(stdout, "N %s\n", p);
			exit(1);
		}
	} else {
		if (su_verify(p, q+1, NULL, NULL, NULL)) {
			fprintf(stdout, "Y %s\n", p);
			exit(0);
		} else {
			fprintf(stdout, "N %s\n", p);
			exit(1);
		}
	}
	/* NOTREACHED */
	exit(1);
}

static void print_settings(int try_http, int bg, char *gui_str) {

	fprintf(stderr, "\n");
	fprintf(stderr, "Settings:\n");
	fprintf(stderr, " display:    %s\n", use_dpy ? use_dpy
	    : "null");
#if SMALL_FOOTPRINT < 2
	fprintf(stderr, " authfile:   %s\n", auth_file ? auth_file
	    : "null");
	fprintf(stderr, " subwin:     0x%lx\n", subwin);
	fprintf(stderr, " -sid mode:  %d\n", rootshift);
	fprintf(stderr, " clip:       %s\n", clip_str ? clip_str
	    : "null");
	fprintf(stderr, " flashcmap:  %d\n", flash_cmap);
	fprintf(stderr, " shiftcmap:  %d\n", shift_cmap);
	fprintf(stderr, " force_idx:  %d\n", force_indexed_color);
	fprintf(stderr, " cmap8to24:  %d\n", cmap8to24);
	fprintf(stderr, " 8to24_opts: %s\n", cmap8to24_str ? cmap8to24_str
	    : "null");
	fprintf(stderr, " 24to32:     %d\n", xform24to32);
	fprintf(stderr, " visual:     %s\n", visual_str ? visual_str
	    : "null");
	fprintf(stderr, " overlay:    %d\n", overlay);
	fprintf(stderr, " ovl_cursor: %d\n", overlay_cursor);
	fprintf(stderr, " scaling:    %d %.4f\n", scaling, scale_fac);
	fprintf(stderr, " viewonly:   %d\n", view_only);
	fprintf(stderr, " shared:     %d\n", shared);
	fprintf(stderr, " conn_once:  %d\n", connect_once);
	fprintf(stderr, " timeout:    %d\n", first_conn_timeout);
	fprintf(stderr, " inetd:      %d\n", inetd);
	fprintf(stderr, " filexfer:   %d\n", filexfer);
	fprintf(stderr, " http:       %d\n", try_http);
	fprintf(stderr, " connect:    %s\n", client_connect
	    ? client_connect : "null");
	fprintf(stderr, " connectfile %s\n", client_connect_file
	    ? client_connect_file : "null");
	fprintf(stderr, " vnc_conn:   %d\n", vnc_connect);
	fprintf(stderr, " allow:      %s\n", allow_list ? allow_list
	    : "null");
	fprintf(stderr, " input:      %s\n", allowed_input_str
	    ? allowed_input_str : "null");
	fprintf(stderr, " passfile:   %s\n", passwdfile ? passwdfile
	    : "null");
	fprintf(stderr, " unixpw:     %d\n", unixpw);
	fprintf(stderr, " unixpw_lst: %s\n", unixpw_list ? unixpw_list:"null");
	fprintf(stderr, " ssl:        %s\n", openssl_pem ? openssl_pem:"null");
	fprintf(stderr, " ssldir:     %s\n", ssl_certs_dir ? ssl_certs_dir:"null");
	fprintf(stderr, " ssltimeout  %d\n", ssl_timeout_secs);
	fprintf(stderr, " sslverify:  %s\n", ssl_verify ? ssl_verify:"null");
	fprintf(stderr, " stunnel:    %d\n", use_stunnel);
	fprintf(stderr, " accept:     %s\n", accept_cmd ? accept_cmd
	    : "null");
	fprintf(stderr, " accept:     %s\n", afteraccept_cmd ? afteraccept_cmd
	    : "null");
	fprintf(stderr, " gone:       %s\n", gone_cmd ? gone_cmd
	    : "null");
	fprintf(stderr, " users:      %s\n", users_list ? users_list
	    : "null");
	fprintf(stderr, " using_shm:  %d\n", using_shm);
	fprintf(stderr, " flipbytes:  %d\n", flip_byte_order);
	fprintf(stderr, " onetile:    %d\n", single_copytile);
	fprintf(stderr, " solid:      %s\n", solid_str
	    ? solid_str : "null");
	fprintf(stderr, " blackout:   %s\n", blackout_str
	    ? blackout_str : "null");
	fprintf(stderr, " xinerama:   %d\n", xinerama);
	fprintf(stderr, " xtrap:      %d\n", xtrap_input);
	fprintf(stderr, " xrandr:     %d\n", xrandr);
	fprintf(stderr, " xrandrmode: %s\n", xrandr_mode ? xrandr_mode
	    : "null");
	fprintf(stderr, " padgeom:    %s\n", pad_geometry
	    ? pad_geometry : "null");
	fprintf(stderr, " logfile:    %s\n", logfile ? logfile
	    : "null");
	fprintf(stderr, " logappend:  %d\n", logfile_append);
	fprintf(stderr, " flag:       %s\n", flagfile ? flagfile
	    : "null");
	fprintf(stderr, " rc_file:    \"%s\"\n", rc_rcfile ? rc_rcfile
	    : "null");
	fprintf(stderr, " norc:       %d\n", rc_norc);
	fprintf(stderr, " dbg:        %d\n", crash_debug);
	fprintf(stderr, " bg:         %d\n", bg);
	fprintf(stderr, " mod_tweak:  %d\n", use_modifier_tweak);
	fprintf(stderr, " isolevel3:  %d\n", use_iso_level3);
	fprintf(stderr, " xkb:        %d\n", use_xkb_modtweak);
	fprintf(stderr, " skipkeys:   %s\n",
	    skip_keycodes ? skip_keycodes : "null");
	fprintf(stderr, " sloppykeys: %d\n", sloppy_keys);
	fprintf(stderr, " skip_dups:  %d\n", skip_duplicate_key_events);
	fprintf(stderr, " addkeysyms: %d\n", add_keysyms);
	fprintf(stderr, " xkbcompat:  %d\n", xkbcompat);
	fprintf(stderr, " clearmods:  %d\n", clear_mods);
	fprintf(stderr, " remap:      %s\n", remap_file ? remap_file
	    : "null");
	fprintf(stderr, " norepeat:   %d\n", no_autorepeat);
	fprintf(stderr, " norepeatcnt:%d\n", no_repeat_countdown);
	fprintf(stderr, " nofb:       %d\n", nofb);
	fprintf(stderr, " watchbell:  %d\n", watch_bell);
	fprintf(stderr, " watchsel:   %d\n", watch_selection);
	fprintf(stderr, " watchprim:  %d\n", watch_primary);
	fprintf(stderr, " seldir:     %s\n", sel_direction ?
	    sel_direction : "null");
	fprintf(stderr, " cursor:     %d\n", show_cursor);
	fprintf(stderr, " multicurs:  %d\n", show_multiple_cursors);
	fprintf(stderr, " curs_mode:  %s\n", multiple_cursors_mode
	    ? multiple_cursors_mode : "null");
	fprintf(stderr, " arrow:      %d\n", alt_arrow);
	fprintf(stderr, " xfixes:     %d\n", use_xfixes);
	fprintf(stderr, " alphacut:   %d\n", alpha_threshold);
	fprintf(stderr, " alphafrac:  %.2f\n", alpha_frac);
	fprintf(stderr, " alpharemove:%d\n", alpha_remove);
	fprintf(stderr, " alphablend: %d\n", alpha_blend);
	fprintf(stderr, " cursorshape:%d\n", cursor_shape_updates);
	fprintf(stderr, " cursorpos:  %d\n", cursor_pos_updates);
	fprintf(stderr, " xwarpptr:   %d\n", use_xwarppointer);
	fprintf(stderr, " buttonmap:  %s\n", pointer_remap
	    ? pointer_remap : "null");
	fprintf(stderr, " dragging:   %d\n", show_dragging);
	fprintf(stderr, " wireframe:  %s\n", wireframe_str ?
	    wireframe_str : WIREFRAME_PARMS);
	fprintf(stderr, " wirecopy:   %s\n", wireframe_copyrect ?
	    wireframe_copyrect : wireframe_copyrect_default);
	fprintf(stderr, " scrollcopy: %s\n", scroll_copyrect ?
	    scroll_copyrect : scroll_copyrect_default);
	fprintf(stderr, "  scr_area:  %d\n", scrollcopyrect_min_area);
	fprintf(stderr, "  scr_skip:  %s\n", scroll_skip_str ?
	    scroll_skip_str : scroll_skip_str0);
	fprintf(stderr, "  scr_inc:   %s\n", scroll_good_str ?
	    scroll_good_str : scroll_good_str0);
	fprintf(stderr, "  scr_keys:  %s\n", scroll_key_list_str ?
	    scroll_key_list_str : "null");
	fprintf(stderr, "  scr_term:  %s\n", scroll_term_str ?
	    scroll_term_str : "null");
	fprintf(stderr, "  scr_keyrep: %s\n", max_keyrepeat_str ?
	    max_keyrepeat_str : "null");
	fprintf(stderr, "  scr_parms: %s\n", scroll_copyrect_str ?
	    scroll_copyrect_str : SCROLL_COPYRECT_PARMS);
	fprintf(stderr, " fixscreen:  %s\n", screen_fixup_str ?
	    screen_fixup_str : "null");
	fprintf(stderr, " noxrecord:  %d\n", noxrecord);
	fprintf(stderr, " grabbuster: %d\n", grab_buster);
	fprintf(stderr, " ptr_mode:   %d\n", pointer_mode);
	fprintf(stderr, " inputskip:  %d\n", ui_skip);
	fprintf(stderr, " speeds:     %s\n", speeds_str
	    ? speeds_str : "null");
	fprintf(stderr, " wmdt:       %s\n", wmdt_str
	    ? wmdt_str : "null");
	fprintf(stderr, " debug_ptr:  %d\n", debug_pointer);
	fprintf(stderr, " debug_key:  %d\n", debug_keyboard);
	fprintf(stderr, " defer:      %d\n", defer_update);
	fprintf(stderr, " waitms:     %d\n", waitms);
	fprintf(stderr, " wait_ui:    %.2f\n", wait_ui);
	fprintf(stderr, " nowait_bog: %d\n", !wait_bog);
	fprintf(stderr, " slow_fb:    %.2f\n", slow_fb);
	fprintf(stderr, " readtimeout: %d\n", rfbMaxClientWait/1000);
	fprintf(stderr, " take_naps:  %d\n", take_naps);
	fprintf(stderr, " sb:         %d\n", screen_blank);
	fprintf(stderr, " fbpm:       %d\n", !watch_fbpm);
	fprintf(stderr, " xdamage:    %d\n", use_xdamage);
	fprintf(stderr, "  xd_area:   %d\n", xdamage_max_area);
	fprintf(stderr, "  xd_mem:    %.3f\n", xdamage_memory);
	fprintf(stderr, " sigpipe:    %s\n", sigpipe
	    ? sigpipe : "null");
	fprintf(stderr, " threads:    %d\n", use_threads);
	fprintf(stderr, " fs_frac:    %.2f\n", fs_frac);
	fprintf(stderr, " gaps_fill:  %d\n", gaps_fill);
	fprintf(stderr, " grow_fill:  %d\n", grow_fill);
	fprintf(stderr, " tile_fuzz:  %d\n", tile_fuzz);
	fprintf(stderr, " snapfb:     %d\n", use_snapfb);
	fprintf(stderr, " rawfb:      %s\n", raw_fb_str
	    ? raw_fb_str : "null");
	fprintf(stderr, " pipeinput:  %s\n", pipeinput_str
	    ? pipeinput_str : "null");
	fprintf(stderr, " gui:        %d\n", launch_gui);
	fprintf(stderr, " gui_mode:   %s\n", gui_str
	    ? gui_str : "null");
	fprintf(stderr, " noremote:   %d\n", !accept_remote_cmds);
	fprintf(stderr, " unsafe:     %d\n", !safe_remote_only);
	fprintf(stderr, " privremote: %d\n", priv_remote);
	fprintf(stderr, " safer:      %d\n", more_safe);
	fprintf(stderr, " nocmds:     %d\n", no_external_cmds);
	fprintf(stderr, " deny_all:   %d\n", deny_all);
	fprintf(stderr, " pid:        %d\n", getpid());
	fprintf(stderr, "\n");
#endif
	rfbLog("x11vnc version: %s\n", lastmod);
}


static void check_loop_mode(int argc, char* argv[]) {
	int i;
	int loop_mode = 0, loop_sleep = 2000, loop_max = 0;

	for (i=1; i < argc; i++) {
		char *p = argv[i];
		if (strstr(p, "--") == p) {
			p++;
		}
		if (strstr(p, "-loop") == p) {
			char *q;
			loop_mode = 1;
			if ((q = strchr(p, ',')) != NULL) {
				loop_max = atoi(q+1);
				*q = '\0';
			}
			
			q = strpbrk(p, "0123456789");
			if (q) {
				loop_sleep = atoi(q);
				if (loop_sleep <= 0) {
					loop_sleep = 10;
				}
			}
		}
	}
	if (loop_mode && getenv("X11VNC_LOOP_MODE") == NULL) {
#if LIBVNCSERVER_HAVE_FORK
		char **argv2;
		int k, i = 1;

		set_env("X11VNC_LOOP_MODE", "1");
		argv2 = (char **) malloc((argc+1)*sizeof(char *));

		for (k=0; k < argc+1; k++) {
			argv2[k] = NULL;
			if (k < argc) {
				argv2[k] = argv[k];
			}
		}
		while (1) {
			int status;
			pid_t p;
			fprintf(stderr, "\n --- x11vnc loop: %d ---\n\n", i++);
			fflush(stderr);
			usleep(500 * 1000);
			if ((p = fork()) > 0)  {
				fprintf(stderr, " --- x11vnc loop: waiting "
				    "for: %d\n\n", p);
				wait(&status);
			} else if (p == -1) {
				fprintf(stderr, "could not fork\n");
				perror("fork");
				exit(1);
			} else {
				/* loop mode: no_external_cmds does not apply */
				execvp(argv[0], argv2); 
				exit(1);
			}

			if (loop_max > 0 && i > loop_max) {
				fprintf(stderr, "\n --- x11vnc loop: did %d"
				    " done. ---\n\n", loop_max);
				break;
			}
			
			fprintf(stderr, "\n --- x11vnc loop: sleeping %d ms "
			    "---\n\n", loop_sleep);
			usleep(loop_sleep * 1000);
		}
		exit(0);
#else
		fprintf(stderr, "fork unavailable, cannot do -loop mode\n");
		exit(1);
#endif
	}
}
static void store_homedir_passwd(char *file) {
	char str1[32], str2[32], *p, *h, *f;
	struct stat sbuf;

	str1[0] = '\0';
	str2[0] = '\0';

	/* storepasswd */
	if (no_external_cmds || !cmd_ok("storepasswd")) {
		fprintf(stderr, "-nocmds cannot be used with -storepasswd\n");
		exit(1);
	}

	fprintf(stderr, "Enter VNC password: ");
	system("stty -echo");
	if (fgets(str1, 32, stdin) == NULL) {
		system("stty echo");
		exit(1);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Verify password:    ");
	if (fgets(str2, 32, stdin) == NULL) {
		system("stty echo");
		exit(1);
	}
	fprintf(stderr, "\n");
	system("stty echo");

	if ((p = strchr(str1, '\n')) != NULL) {
		*p = '\0';
	}
	if ((p = strchr(str2, '\n')) != NULL) {
		*p = '\0';
	}
	if (strcmp(str1, str2)) {
		fprintf(stderr, "** passwords differ.\n");
		exit(1);
	}
	if (str1[0] == '\0') {
		fprintf(stderr, "** no password supplied.\n");
		exit(1);
	}

	if (file != NULL) {
		f = file;
	} else {
		
		h = getenv("HOME");
		if (! h) {
			fprintf(stderr, "** $HOME not set.\n");
			exit(1);
		}

		f = (char *) malloc(strlen(h) + strlen("/.vnc/passwd") + 1);
		sprintf(f, "%s/.vnc", h);

		if (stat(f, &sbuf) != 0) {
			if (mkdir(f, 0755) != 0) {
				fprintf(stderr, "** could not create directory %s\n", f);
				perror("mkdir");
				exit(1);
			}
		} else if (! S_ISDIR(sbuf.st_mode)) {
			fprintf(stderr, "** not a directory %s\n", f);
			exit(1);
		}

		sprintf(f, "%s/.vnc/passwd", h);
	}
	fprintf(stderr, "Write password to %s?  [y]/n ", f);

	if (fgets(str2, 32, stdin) == NULL) {
		exit(1);
	}
	if (str2[0] == 'n' || str2[0] == 'N') {
		fprintf(stderr, "not creating password.\n");
		exit(1);
	}

	if (rfbEncryptAndStorePasswd(str1, f) != 0) {
		fprintf(stderr, "** error creating password.\n");
		perror("storepasswd");
		exit(1);
	}
	fprintf(stderr, "Password written to: %s\n", f);
	if (stat(f, &sbuf) != 0) {
		exit(1);
	}
	exit(0);
}

#define	SHOW_NO_PASSWORD_WARNING \
	(!got_passwd && !got_rfbauth && (!got_passwdfile || !passwd_list) \
	    && !query_cmd && !remote_cmd && !unixpw && !got_gui_pw \
	    && ! ssl_verify && !inetd)

int main(int argc, char* argv[]) {

	int i, len, tmpi;
	int ev, er, maj, min;
	char *arg;
	int remote_sync = 0;
	char *remote_cmd = NULL;
	char *query_cmd  = NULL;
	char *gui_str = NULL;
	int got_gui_pw = 0;
	int pw_loc = -1, got_passwd = 0, got_rfbauth = 0, nopw = NOPW;
	int got_viewpasswd = 0, got_localhost = 0, got_passwdfile = 0;
	int vpw_loc = -1;
	int dt = 0, bg = 0;
	int got_rfbwait = 0;
	int got_httpdir = 0, try_http = 0;
	int waited_for_client = 0;
	XImage *fb0 = NULL;

	/* used to pass args we do not know about to rfbGetScreen(): */
	int argc_vnc_max = 1024;
	int argc_vnc = 1; char *argv_vnc[2048];

	/* check for -loop mode: */
	check_loop_mode(argc, argv);

	dtime0(&x11vnc_start);

	if (!getuid() || !geteuid()) {
		started_as_root = 1;

		/* check for '-users =bob' */
		immediate_switch_user(argc, argv);
	}

	for (i=0; i < 2048; i++) {
		argv_vnc[i] = NULL;
	}

	argv_vnc[0] = strdup(argv[0]);
	program_name = strdup(argv[0]);
	program_pid = (int) getpid();

	solid_default = strdup(solid_default);	/* for freeing with -R */

	len = 0;
	for (i=1; i < argc; i++) {
		len += strlen(argv[i]) + 4 + 1;
	}
	program_cmdline = (char *) malloc(len+1);
	program_cmdline[0] = '\0';
	for (i=1; i < argc; i++) {
		char *s = argv[i];
		if (program_cmdline[0]) {
			strcat(program_cmdline, " ");
		}
		if (*s == '-') {
			strcat(program_cmdline, s);
		} else {
			strcat(program_cmdline, "{{");
			strcat(program_cmdline, s);
			strcat(program_cmdline, "}}");
		}
	}

	for (i=0; i<ICON_MODE_SOCKS; i++) {
		icon_mode_socks[i] = -1;
	}

	check_rcfile(argc, argv);
	/* kludge for the new array argv2 set in check_rcfile() */
#	define argc argc2
#	define argv argv2

#	define CHECK_ARGC if (i >= argc-1) { \
		fprintf(stderr, "not enough arguments for: %s\n", arg); \
		exit(1); \
	}

	/*
	 * do a quick check for parameters that apply to "utility"
	 * commands, i.e. ones that do not run the server.
	 */
	for (i=1; i < argc; i++) {
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}
		if (!strcmp(arg, "-ssldir")) {
			CHECK_ARGC
			ssl_certs_dir = strdup(argv[++i]);
		}
	}

	/*
	 * do a quick check for -env parameters
	 */
	for (i=1; i < argc; i++) {
		char *p, *q;
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}
		if (!strcmp(arg, "-env")) {
			CHECK_ARGC
			p = strdup(argv[++i]);
			q = strchr(p, '=');
			if (! q) {
				fprintf(stderr, "no -env '=' found: %s\n", p);
				exit(1);
			} else {
				*q = '\0';
			}
			set_env(p, q+1);
			free(p);
		}
	}

	for (i=1; i < argc; i++) {
		/* quick-n-dirty --option handling. */
		arg = argv[i];
		if (strstr(arg, "--") == arg) {
			arg++;
		}

		if (!strcmp(arg, "-display")) {
			CHECK_ARGC
			use_dpy = strdup(argv[++i]);
			if (strstr(use_dpy, "WAIT")) {
				extern char find_display[];
				if (strstr(use_dpy, "cmd=FINDDISPLAY-print")) {
					fprintf(stdout, "%s", find_display);
					exit(0);
				}
			}
		} else if (!strcmp(arg, "-auth") || !strcmp(arg, "-xauth")) {
			CHECK_ARGC
			auth_file = strdup(argv[++i]);
		} else if (!strcmp(arg, "-id") || !strcmp(arg, "-sid")) {
			CHECK_ARGC
			if (!strcmp(arg, "-sid")) {
				rootshift = 1;
			} else {
				rootshift = 0;
			}
			i++;
			if (!strcmp("pick", argv[i])) {
				if (started_as_root) {
					fprintf(stderr, "unsafe: %s pick\n",
					    arg);
					exit(1);
				} else if (! pick_windowid(&subwin)) {
					fprintf(stderr, "invalid %s pick\n",
					    arg);
					exit(1);
				}
			} else if (! scan_hexdec(argv[i], &subwin)) {
				fprintf(stderr, "invalid %s arg: %s\n", arg,
				    argv[i]);
				exit(1);
			}
		} else if (!strcmp(arg, "-waitmapped")) {
			subwin_wait_mapped = 1;
		} else if (!strcmp(arg, "-clip")) {
			CHECK_ARGC
			clip_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-flashcmap")) {
			flash_cmap = 1;
		} else if (!strcmp(arg, "-shiftcmap")) {
			CHECK_ARGC
			shift_cmap = atoi(argv[++i]);
		} else if (!strcmp(arg, "-notruecolor")) {
			force_indexed_color = 1;
		} else if (!strcmp(arg, "-overlay")) {
			overlay = 1;
		} else if (!strcmp(arg, "-overlay_nocursor")) {
			overlay = 1;
			overlay_cursor = 0;
		} else if (!strcmp(arg, "-overlay_yescursor")) {
			overlay = 1;
			overlay_cursor = 2;
#if !SKIP_8TO24
		} else if (!strcmp(arg, "-8to24")) {
			cmap8to24 = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					cmap8to24_str = strdup(s);
					i++;
				}
			}
#endif
		} else if (!strcmp(arg, "-24to32")) {
			xform24to32 = 1;
		} else if (!strcmp(arg, "-visual")) {
			CHECK_ARGC
			visual_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scale")) {
			CHECK_ARGC
			scale_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scale_cursor")) {
			CHECK_ARGC
			scale_cursor_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-viewonly")) {
			view_only = 1;
		} else if (!strcmp(arg, "-noviewonly")) {
			view_only = 0;
			got_noviewonly = 1;
		} else if (!strcmp(arg, "-shared")) {
			shared = 1;
		} else if (!strcmp(arg, "-once")) {
			connect_once = 1;
			got_connect_once = 1;
		} else if (!strcmp(arg, "-many") || !strcmp(arg, "-forever")) {
			connect_once = 0;
		} else if (strstr(arg, "-loop") == arg) {
			;	/* handled above */
		} else if (!strcmp(arg, "-timeout")) {
			CHECK_ARGC
			first_conn_timeout = atoi(argv[++i]);
		} else if (!strcmp(arg, "-users")) {
			CHECK_ARGC
			users_list = strdup(argv[++i]);
		} else if (!strcmp(arg, "-inetd")) {
			inetd = 1;
		} else if (!strcmp(arg, "-nofilexfer")) {
			filexfer = 0;
		} else if (!strcmp(arg, "-filexfer")) {
			filexfer = 1;
		} else if (!strcmp(arg, "-http")) {
			try_http = 1;
		} else if (!strcmp(arg, "-http_ssl")) {
			try_http = 1;
			http_ssl = 1;
		} else if (!strcmp(arg, "-connect")) {
			CHECK_ARGC
			if (strchr(argv[++i], '/')) {
				client_connect_file = strdup(argv[i]);
			} else {
				client_connect = strdup(argv[i]);
			}
		} else if (!strcmp(arg, "-vncconnect")) {
			vnc_connect = 1;
		} else if (!strcmp(arg, "-novncconnect")) {
			vnc_connect = 0;
		} else if (!strcmp(arg, "-allow")) {
			CHECK_ARGC
			allow_list = strdup(argv[++i]);
		} else if (!strcmp(arg, "-localhost")) {
			allow_list = strdup("127.0.0.1");
			got_localhost = 1;
		} else if (!strcmp(arg, "-nolookup")) {
			host_lookup = 0;
		} else if (!strcmp(arg, "-input")) {
			CHECK_ARGC
			allowed_input_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-grabkbd")) {
			grab_kbd = 1;
		} else if (!strcmp(arg, "-grabptr")) {
			grab_ptr = 1;
		} else if (!strcmp(arg, "-viewpasswd")) {
			vpw_loc = i;
			CHECK_ARGC
			viewonly_passwd = strdup(argv[++i]);
			got_viewpasswd = 1;
		} else if (!strcmp(arg, "-passwdfile")) {
			CHECK_ARGC
			passwdfile = strdup(argv[++i]);
			got_passwdfile = 1;
#ifndef NO_SSL_OR_UNIXPW
		} else if (strstr(arg, "-unixpw") == arg) {
			unixpw = 1;
			if (strstr(arg, "-unixpw_nis")) {
				unixpw_nis = 1;
			}
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					unixpw_list = strdup(s);
					i++;
				}
				if (s[0] == '%') {
					unixpw_list = NULL;
					quick_pw(s);
					exit(1);
				}
			}
			if (strstr(arg, "_unsafe")) {
				/* hidden option for testing. */
				set_env("UNIXPW_DISABLE_SSL", "1");
				set_env("UNIXPW_DISABLE_LOCALHOST", "1");
			}
		} else if (!strcmp(arg, "-ssl")) {
			use_openssl = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					openssl_pem = strdup(s);
					i++;
				}
			}
		} else if (!strcmp(arg, "-ssltimeout")) {
			CHECK_ARGC
			ssl_timeout_secs = atoi(argv[++i]);
		} else if (!strcmp(arg, "-ssldir")) {
			CHECK_ARGC
			ssl_certs_dir = strdup(argv[++i]);

		} else if (!strcmp(arg, "-sslverify")) {
			CHECK_ARGC
			ssl_verify = strdup(argv[++i]);

		} else if (!strcmp(arg, "-sslGenCA")) {
			char *cdir = NULL;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					cdir = strdup(s);
					i++;
				}
			}
			sslGenCA(cdir);
			exit(0);
		} else if (!strcmp(arg, "-sslGenCert")) {
			char *ty, *nm = NULL;
			if (i >= argc-1) {
				fprintf(stderr, "Must be:\n");
				fprintf(stderr, "          x11vnc -sslGenCert server ...\n");
				fprintf(stderr, "or        x11vnc -sslGenCert client ...\n");
				exit(1);
			}
			ty = argv[i+1];
			if (strcmp(ty, "server") && strcmp(ty, "client")) {
				fprintf(stderr, "Must be:\n");
				fprintf(stderr, "          x11vnc -sslGenCert server ...\n");
				fprintf(stderr, "or        x11vnc -sslGenCert client ...\n");
				exit(1);
			}
			if (i < argc-2) {
				nm = argv[i+2];
			}
			sslGenCert(ty, nm);
			exit(0);
		} else if (!strcmp(arg, "-sslEncKey")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 0);
			}
			exit(0);
		} else if (!strcmp(arg, "-sslCertInfo")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 1);
			}
			exit(0);
		} else if (!strcmp(arg, "-sslDelCert")) {
			if (i < argc-1) {
				char *s = argv[i+1];
				sslEncKey(s, 2);
			}
			exit(0);

		} else if (!strcmp(arg, "-stunnel")) {
			use_stunnel = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					stunnel_pem = strdup(s);
					i++;
				}
			}
		} else if (!strcmp(arg, "-stunnel3")) {
			use_stunnel = 3;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					stunnel_pem = strdup(s);
					i++;
				}
			}
		} else if (!strcmp(arg, "-https")) {
			https_port_num = 0;
			try_http = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					https_port_num = atoi(s);
					i++;
				}
			}
#endif
		} else if (!strcmp(arg, "-nopw")) {
			nopw = 1;
		} else if (!strcmp(arg, "-usepw")) {
			usepw = 1;
		} else if (!strcmp(arg, "-storepasswd")) {
			if (argc == i+1) {
				store_homedir_passwd(NULL);
				exit(0);
			}
			if (argc == i+2) {
				store_homedir_passwd(argv[i+1]);
				exit(0);
			}
			if (argc >= i+4 || rfbEncryptAndStorePasswd(argv[i+1],
			    argv[i+2]) != 0) {
				fprintf(stderr, "-storepasswd failed\n");
				exit(1);
			} else {
				fprintf(stderr, "stored passwd in file %s\n",
				    argv[i+2]);
				exit(0);
			}
		} else if (!strcmp(arg, "-accept")) {
			CHECK_ARGC
			accept_cmd = strdup(argv[++i]);
		} else if (!strcmp(arg, "-afteraccept")) {
			CHECK_ARGC
			afteraccept_cmd = strdup(argv[++i]);
		} else if (!strcmp(arg, "-gone")) {
			CHECK_ARGC
			gone_cmd = strdup(argv[++i]);
		} else if (!strcmp(arg, "-noshm")) {
			using_shm = 0;
		} else if (!strcmp(arg, "-flipbyteorder")) {
			flip_byte_order = 1;
		} else if (!strcmp(arg, "-onetile")) {
			single_copytile = 1;
		} else if (!strcmp(arg, "-solid")) {
			use_solid_bg = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (s[0] != '-') {
					solid_str = strdup(s);
					i++;
				}
			}
			if (! solid_str) {
				solid_str = strdup(solid_default);
			}
		} else if (!strcmp(arg, "-blackout")) {
			CHECK_ARGC
			blackout_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-xinerama")) {
			xinerama = 1;
		} else if (!strcmp(arg, "-xtrap")) {
			xtrap_input = 1;
		} else if (!strcmp(arg, "-xrandr")) {
			xrandr = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (known_xrandr_mode(s)) {
					xrandr_mode = strdup(s);
					i++;
				}
			}
		} else if (!strcmp(arg, "-rotate")) {
			CHECK_ARGC
			rotating_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-padgeom")
		    || !strcmp(arg, "-padgeometry")) {
			CHECK_ARGC
			pad_geometry = strdup(argv[++i]);
		} else if (!strcmp(arg, "-o") || !strcmp(arg, "-logfile")) {
			CHECK_ARGC
			logfile_append = 0;
			logfile = strdup(argv[++i]);
		} else if (!strcmp(arg, "-oa") || !strcmp(arg, "-logappend")) {
			CHECK_ARGC
			logfile_append = 1;
			logfile = strdup(argv[++i]);
		} else if (!strcmp(arg, "-flag")) {
			CHECK_ARGC
			flagfile = strdup(argv[++i]);
		} else if (!strcmp(arg, "-rc")) {
			i++;	/* done above */
		} else if (!strcmp(arg, "-norc")) {
			;	/* done above */
		} else if (!strcmp(arg, "-env")) {
			i++;	/* done above */
		} else if (!strcmp(arg, "-h") || !strcmp(arg, "-help")) {
			print_help(0);
		} else if (!strcmp(arg, "-?") || !strcmp(arg, "-opts")) {
			print_help(1);
		} else if (!strcmp(arg, "-V") || !strcmp(arg, "-version")) {
			fprintf(stdout, "x11vnc: %s\n", lastmod);
			exit(0);
		} else if (!strcmp(arg, "-license") ||
		    !strcmp(arg, "-copying") || !strcmp(arg, "-warranty")) {
			print_license();
		} else if (!strcmp(arg, "-dbg")) {
			crash_debug = 1;
		} else if (!strcmp(arg, "-nodbg")) {
			crash_debug = 0;
		} else if (!strcmp(arg, "-q") || !strcmp(arg, "-quiet")) {
			quiet = 1;
		} else if (!strcmp(arg, "-bg") || !strcmp(arg, "-background")) {
#if LIBVNCSERVER_HAVE_SETSID
			bg = 1;
			opts_bg = bg;
#else
			fprintf(stderr, "warning: -bg mode not supported.\n");
#endif
		} else if (!strcmp(arg, "-modtweak")) {
			use_modifier_tweak = 1;
		} else if (!strcmp(arg, "-nomodtweak")) {
			use_modifier_tweak = 0;
			got_nomodtweak = 1;
		} else if (!strcmp(arg, "-isolevel3")) {
			use_iso_level3 = 1;
		} else if (!strcmp(arg, "-xkb")) {
			use_modifier_tweak = 1;
			use_xkb_modtweak = 1;
		} else if (!strcmp(arg, "-noxkb")) {
			use_xkb_modtweak = 0;
			got_noxkb = 1;
		} else if (!strcmp(arg, "-capslock")) {
			watch_capslock = 1;
		} else if (!strcmp(arg, "-skip_lockkeys")) {
			skip_lockkeys = 1;
		} else if (!strcmp(arg, "-xkbcompat")) {
			xkbcompat = 1;
		} else if (!strcmp(arg, "-skip_keycodes")) {
			CHECK_ARGC
			skip_keycodes = strdup(argv[++i]);
		} else if (!strcmp(arg, "-sloppy_keys")) {
			sloppy_keys++;
		} else if (!strcmp(arg, "-skip_dups")) {
			skip_duplicate_key_events = 1;
		} else if (!strcmp(arg, "-noskip_dups")) {
			skip_duplicate_key_events = 0;
		} else if (!strcmp(arg, "-add_keysyms")) {
			add_keysyms++;
		} else if (!strcmp(arg, "-noadd_keysyms")) {
			add_keysyms = 0;
		} else if (!strcmp(arg, "-clear_mods")) {
			clear_mods = 1;
		} else if (!strcmp(arg, "-clear_keys")) {
			clear_mods = 2;
		} else if (!strcmp(arg, "-remap")) {
			CHECK_ARGC
			remap_file = strdup(argv[++i]);
		} else if (!strcmp(arg, "-norepeat")) {
			no_autorepeat = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s == '-') {
					s++;
				}
				if (isdigit((unsigned char) (*s))) {
					no_repeat_countdown = atoi(argv[++i]);
				}
			}
		} else if (!strcmp(arg, "-repeat")) {
			no_autorepeat = 0;
		} else if (!strcmp(arg, "-nofb")) {
			nofb = 1;
		} else if (!strcmp(arg, "-nobell")) {
			watch_bell = 0;
			sound_bell = 0;
		} else if (!strcmp(arg, "-nosel")) {
			watch_selection = 0;
			watch_primary = 0;
			watch_clipboard = 0;
		} else if (!strcmp(arg, "-noprimary")) {
			watch_primary = 0;
		} else if (!strcmp(arg, "-nosetprimary")) {
			set_primary = 0;
		} else if (!strcmp(arg, "-noclipboard")) {
			watch_clipboard = 0;
		} else if (!strcmp(arg, "-nosetclipboard")) {
			set_clipboard = 0;
		} else if (!strcmp(arg, "-seldir")) {
			CHECK_ARGC
			sel_direction = strdup(argv[++i]);
		} else if (!strcmp(arg, "-cursor")) {
			show_cursor = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (known_cursors_mode(s)) {
					multiple_cursors_mode = strdup(s);
					i++;
					if (!strcmp(s, "none")) {
						show_cursor = 0;
					}
				} 
			}
		} else if (!strcmp(arg, "-nocursor")) { 
			multiple_cursors_mode = strdup("none");
			show_cursor = 0;
		} else if (!strcmp(arg, "-arrow")) {
			CHECK_ARGC
			alt_arrow = atoi(argv[++i]);
		} else if (!strcmp(arg, "-xfixes")) { 
			use_xfixes = 1;
		} else if (!strcmp(arg, "-noxfixes")) { 
			use_xfixes = 0;
		} else if (!strcmp(arg, "-alphacut")) {
			CHECK_ARGC
			alpha_threshold = atoi(argv[++i]);
		} else if (!strcmp(arg, "-alphafrac")) {
			CHECK_ARGC
			alpha_frac = atof(argv[++i]);
		} else if (!strcmp(arg, "-alpharemove")) {
			alpha_remove = 1;
		} else if (!strcmp(arg, "-noalphablend")) {
			alpha_blend = 0;
		} else if (!strcmp(arg, "-nocursorshape")) {
			cursor_shape_updates = 0;
		} else if (!strcmp(arg, "-cursorpos")) {
			cursor_pos_updates = 1;
			got_cursorpos = 1;
		} else if (!strcmp(arg, "-nocursorpos")) {
			cursor_pos_updates = 0;
		} else if (!strcmp(arg, "-xwarppointer")) {
			use_xwarppointer = 1;
		} else if (!strcmp(arg, "-buttonmap")) {
			CHECK_ARGC
			pointer_remap = strdup(argv[++i]);
		} else if (!strcmp(arg, "-nodragging")) {
			show_dragging = 0;
		} else if (!strcmp(arg, "-wireframe")
		    || !strcmp(arg, "-wf")) {
			wireframe = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s != '-') {
					wireframe_str = strdup(argv[++i]);
				}
			}
		} else if (!strcmp(arg, "-nowireframe")
		    || !strcmp(arg, "-nowf")) {
			wireframe = 0;
		} else if (!strcmp(arg, "-wirecopyrect")
		    || !strcmp(arg, "-wcr")) {
			CHECK_ARGC
			set_wirecopyrect_mode(argv[++i]);
			got_wirecopyrect = 1;
		} else if (!strcmp(arg, "-nowirecopyrect")
		    || !strcmp(arg, "-nowcr")) {
			set_wirecopyrect_mode("never");
		} else if (!strcmp(arg, "-debug_wireframe")
		    || !strcmp(arg, "-dwf")) {
			debug_wireframe++;
		} else if (!strcmp(arg, "-scrollcopyrect")
		    || !strcmp(arg, "-scr")) {
			CHECK_ARGC
			set_scrollcopyrect_mode(argv[++i]);
			got_scrollcopyrect = 1;
		} else if (!strcmp(arg, "-noscrollcopyrect")
		    || !strcmp(arg, "-noscr")) {
			set_scrollcopyrect_mode("never");
		} else if (!strcmp(arg, "-scr_area")) {
			int tn;
			CHECK_ARGC
			tn = atoi(argv[++i]);
			if (tn >= 0) {
				scrollcopyrect_min_area = tn;
			}
		} else if (!strcmp(arg, "-scr_skip")) {
			CHECK_ARGC
			scroll_skip_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scr_inc")) {
			CHECK_ARGC
			scroll_good_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scr_keys")) {
			CHECK_ARGC
			scroll_key_list_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scr_term")) {
			CHECK_ARGC
			scroll_term_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scr_keyrepeat")) {
			CHECK_ARGC
			max_keyrepeat_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-scr_parms")) {
			CHECK_ARGC
			scroll_copyrect_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-fixscreen")) {
			CHECK_ARGC
			screen_fixup_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-debug_scroll")
		    || !strcmp(arg, "-ds")) {
			debug_scroll++;
		} else if (!strcmp(arg, "-noxrecord")) {
			noxrecord = 1;
		} else if (!strcmp(arg, "-pointer_mode")
		    || !strcmp(arg, "-pm")) {
			char *p, *s;
			CHECK_ARGC
			s = argv[++i];
			if ((p = strchr(s, ':')) != NULL) {
				ui_skip = atoi(p+1);
				if (! ui_skip) ui_skip = 1;
				*p = '\0';
			}
			if (atoi(s) < 1 || atoi(s) > pointer_mode_max) {
				rfbLog("pointer_mode out of range 1-%d: %d\n",
				    pointer_mode_max, atoi(s));
			} else {
				pointer_mode = atoi(s);
				got_pointer_mode = pointer_mode;
			}
		} else if (!strcmp(arg, "-input_skip")) {
			CHECK_ARGC
			ui_skip = atoi(argv[++i]);
			if (! ui_skip) ui_skip = 1;
		} else if (!strcmp(arg, "-allinput")) {
			all_input = 1;
		} else if (!strcmp(arg, "-noallinput")) {
			all_input = 0;
		} else if (!strcmp(arg, "-speeds")) {
			CHECK_ARGC
			speeds_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-wmdt")) {
			CHECK_ARGC
			wmdt_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-debug_pointer")
		    || !strcmp(arg, "-dp")) {
			debug_pointer++;
		} else if (!strcmp(arg, "-debug_keyboard")
		    || !strcmp(arg, "-dk")) {
			debug_keyboard++;
		} else if (!strcmp(arg, "-defer")) {
			CHECK_ARGC
			defer_update = atoi(argv[++i]);
			got_defer = 1;
		} else if (!strcmp(arg, "-wait")) {
			CHECK_ARGC
			waitms = atoi(argv[++i]);
		} else if (!strcmp(arg, "-wait_ui")) {
			CHECK_ARGC
			wait_ui = atof(argv[++i]);
		} else if (!strcmp(arg, "-nowait_bog")) {
			wait_bog = 0;
		} else if (!strcmp(arg, "-slow_fb")) {
			CHECK_ARGC
			slow_fb = atof(argv[++i]);
		} else if (!strcmp(arg, "-readtimeout")) {
			CHECK_ARGC
			rfbMaxClientWait = atoi(argv[++i]) * 1000;
		} else if (!strcmp(arg, "-nap")) {
			take_naps = 1;
		} else if (!strcmp(arg, "-nonap")) {
			take_naps = 0;
		} else if (!strcmp(arg, "-sb")) {
			CHECK_ARGC
			screen_blank = atoi(argv[++i]);
		} else if (!strcmp(arg, "-nofbpm")) {
			watch_fbpm = 1;
		} else if (!strcmp(arg, "-fbpm")) {
			watch_fbpm = 0;
		} else if (!strcmp(arg, "-xdamage")) {
			use_xdamage = 1;
		} else if (!strcmp(arg, "-noxdamage")) {
			use_xdamage = 0;
		} else if (!strcmp(arg, "-xd_area")) {
			int tn;
			CHECK_ARGC
			tn = atoi(argv[++i]);
			if (tn >= 0) {
				xdamage_max_area = tn;
			}
		} else if (!strcmp(arg, "-xd_mem")) {
			double f;
			CHECK_ARGC
			f = atof(argv[++i]);
			if (f >= 0.0) {
				xdamage_memory = f;
			}
		} else if (!strcmp(arg, "-sigpipe")) {
			CHECK_ARGC
			if (known_sigpipe_mode(argv[++i])) {
				sigpipe = strdup(argv[i]);
			} else {
				fprintf(stderr, "invalid -sigpipe arg: %s, must"
				    " be \"ignore\" or \"exit\"\n", argv[i]);
				exit(1);
			}
#if LIBVNCSERVER_HAVE_LIBPTHREAD
		} else if (!strcmp(arg, "-threads")) {
			use_threads = 1;
		} else if (!strcmp(arg, "-nothreads")) {
			use_threads = 0;
#endif
		} else if (!strcmp(arg, "-fs")) {
			CHECK_ARGC
			fs_frac = atof(argv[++i]);
		} else if (!strcmp(arg, "-gaps")) {
			CHECK_ARGC
			gaps_fill = atoi(argv[++i]);
		} else if (!strcmp(arg, "-grow")) {
			CHECK_ARGC
			grow_fill = atoi(argv[++i]);
		} else if (!strcmp(arg, "-fuzz")) {
			CHECK_ARGC
			tile_fuzz = atoi(argv[++i]);
		} else if (!strcmp(arg, "-debug_tiles")
		    || !strcmp(arg, "-dbt")) {
			debug_tiles++;
		} else if (!strcmp(arg, "-debug_grabs")) {
			debug_grabs++;
		} else if (!strcmp(arg, "-debug_sel")) {
			debug_sel++;
		} else if (!strcmp(arg, "-grab_buster")) {
			grab_buster++;
		} else if (!strcmp(arg, "-nograb_buster")) {
			grab_buster = 0;
		} else if (!strcmp(arg, "-snapfb")) {
			use_snapfb = 1;
		} else if (!strcmp(arg, "-rawfb")) {
			CHECK_ARGC
			raw_fb_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-freqtab")) {
			CHECK_ARGC
			freqtab = strdup(argv[++i]);
		} else if (!strcmp(arg, "-pipeinput")) {
			CHECK_ARGC
			pipeinput_str = strdup(argv[++i]);
		} else if (!strcmp(arg, "-gui")) {
			launch_gui = 1;
			if (i < argc-1) {
				char *s = argv[i+1];
				if (*s != '-') {
					gui_str = strdup(s);
					if (strstr(gui_str, "setp")) {
						got_gui_pw = 1;
					}
					i++;
				} 
			}
		} else if (!strcmp(arg, "-remote") || !strcmp(arg, "-R")
		    || !strcmp(arg, "-r") || !strcmp(arg, "-remote-control")) {
			char *str;
			CHECK_ARGC
			i++;
			str = argv[i];
			if (*str == '-') {
				/* accidental leading '-' */
				str++;
			}
			if (!strcmp(str, "ping")) {
				query_cmd = strdup(str);
			} else {
				remote_cmd = strdup(str);
			}
			if (remote_cmd && strchr(remote_cmd, ':') == NULL) {
			    /* interpret -R -scale 3/4 at least */
		 	    if (i < argc-1 && *(argv[i+1]) != '-') {
				int n;

				/* it must be the parameter value */
				i++;
				n = strlen(remote_cmd) + strlen(argv[i]) + 2;

				str = (char *) malloc(n);
				sprintf(str, "%s:%s", remote_cmd, argv[i]);
				free(remote_cmd);
				remote_cmd = str;
			    }
			}
			quiet = 1;
			xkbcompat = 0;
		} else if (!strcmp(arg, "-query") || !strcmp(arg, "-Q")) {
			CHECK_ARGC
			query_cmd = strdup(argv[++i]);
			quiet = 1;
			xkbcompat = 0;
		} else if (!strcmp(arg, "-QD")) {
			CHECK_ARGC
			query_cmd = strdup(argv[++i]);
			query_default = 1;
		} else if (!strcmp(arg, "-sync")) {
			remote_sync = 1;
		} else if (!strcmp(arg, "-nosync")) {
			remote_sync = 0;
		} else if (!strcmp(arg, "-noremote")) {
			accept_remote_cmds = 0;
		} else if (!strcmp(arg, "-yesremote")) {
			accept_remote_cmds = 1;
		} else if (!strcmp(arg, "-unsafe")) {
			safe_remote_only = 0;
		} else if (!strcmp(arg, "-privremote")) {
			priv_remote = 1;
		} else if (!strcmp(arg, "-safer")) {
			more_safe = 1;
		} else if (!strcmp(arg, "-nocmds")) {
			no_external_cmds = 1;
		} else if (!strcmp(arg, "-allowedcmds")) {
			CHECK_ARGC
			allowed_external_cmds = strdup(argv[++i]);
		} else if (!strcmp(arg, "-deny_all")) {
			deny_all = 1;
		} else if (!strcmp(arg, "-httpdir")) {
			CHECK_ARGC
			http_dir = strdup(argv[++i]);
			got_httpdir = 1;
		} else {
			if (!strcmp(arg, "-desktop") && i < argc-1) {
				dt = 1;
				rfb_desktop_name = strdup(argv[i+1]);
			}
			if (!strcmp(arg, "-passwd")) {
				pw_loc = i;
				got_passwd = 1;
			}
			if (!strcmp(arg, "-rfbauth")) {
				got_rfbauth = 1;
			}
			if (!strcmp(arg, "-rfbwait")) {
				got_rfbwait = 1;
			}
			if (!strcmp(arg, "-deferupdate")) {
				got_deferupdate = 1;
			}
			if (!strcmp(arg, "-rfbport") && i < argc-1) {
				got_rfbport = atoi(argv[i+1]);
			}
			if (!strcmp(arg, "-alwaysshared ")) {
				got_alwaysshared = 1;
			}
			if (!strcmp(arg, "-nevershared")) {
				got_nevershared = 1;
			}
			if (!strcmp(arg, "-listen") && i < argc-1) {
				listen_str = strdup(argv[i+1]);
			}
			/* otherwise copy it for libvncserver use below. */
			if (argc_vnc < argc_vnc_max) {
				argv_vnc[argc_vnc++] = strdup(arg);
			} else {
				rfbLog("too many arguments.\n");
				exit(1);
			}
		}
	}


	if (getenv("X11VNC_LOOP_MODE")) {
		if (bg) {
			if (! quiet) {
				fprintf(stderr, "disabling -bg in -loop "
				    "mode\n");
			}
			bg = 0;
		}
		if (inetd) {
			if (! quiet) {
				fprintf(stderr, "disabling -inetd in -loop "
				    "mode\n");
			}
			inetd = 0;
		}
	}

	if (launch_gui && (query_cmd || remote_cmd)) {
		launch_gui = 0;
		gui_str = NULL;
	}
	if (more_safe) {
		launch_gui = 0;
	}
	if (launch_gui) {
		int sleep = 0;
		if (SHOW_NO_PASSWORD_WARNING && !nopw) {
			sleep = 2;
		}
		do_gui(gui_str, sleep);
	}
	if (logfile) {
		int n;
		if (logfile_append) {
			n = open(logfile, O_WRONLY|O_CREAT|O_APPEND, 0666);
		} else {
			n = open(logfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		}
		if (n < 0) {
			fprintf(stderr, "error opening logfile: %s\n", logfile);
			perror("open");
			exit(1);
		}
		if (dup2(n, 2) < 0) {
			fprintf(stderr, "dup2 failed\n");
			perror("dup2");
			exit(1);
		}
		if (n > 2) {
			close(n);
		}
	}
	if (inetd && quiet && !logfile) {
		int n;
		/*
		 * Redir stderr to /dev/null under -inetd and -quiet
		 * but no -o logfile.  Typical problem:
		 *   Xlib:  extension "RECORD" missing on display ":1.0".
		 * If they want this info, they should use -o logfile,
		 * or no -q and 2>logfile.
		 */
		n = open("/dev/null", O_WRONLY);
		if (n >= 0) {
			if (dup2(n, 2) >= 0) {
				if (n > 2) {
					close(n);
				}
			}
		}
	}
	if (! quiet && ! inetd) {
		int i;
		for (i=1; i < argc_vnc; i++) {
			rfbLog("passing arg to libvncserver: %s\n", argv_vnc[i]);
			if (!strcmp(argv_vnc[i], "-passwd")) {
				i++;
			}
		}
	}

	if (remote_cmd || query_cmd) {
		/*
		 * no need to open DISPLAY, just write it to the file now
		 * similar for query_default.
		 */
		if (client_connect_file || query_default) {
			int rc = do_remote_query(remote_cmd, query_cmd,
			     remote_sync, query_default);
			fflush(stderr);
			fflush(stdout);
			exit(rc);
		}
	}

	if (usepw && ! got_rfbauth && ! got_passwd && ! got_passwdfile && !unixpw) {
		char *f, *h = getenv("HOME");
		struct stat sbuf;
		int found = 0, set_rfbauth = 0;

		if (!h) {
			rfbLog("HOME unset in -usepw mode.\n");
			exit(1);
		}
		f = (char *) malloc(strlen(h)+strlen("/.vnc/passwdfile") + 1);

		sprintf(f, "%s/.vnc/passwd", h);
		if (stat(f, &sbuf) == 0) {
			found = 1;
			if (! quiet) {
				rfbLog("-usepw: found %s\n", f);
			}
			got_rfbauth = 1;
			set_rfbauth = 1;
		}

		sprintf(f, "%s/.vnc/passwdfile", h);
		if (! found && stat(f, &sbuf) == 0) {
			found = 1;
			if (! quiet) {
				rfbLog("-usepw: found %s\n", f);
			}
			got_passwdfile = 1;
			passwdfile = strdup(f);
		}

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H
#if LIBVNCSERVER_HAVE_WAITPID
		if (! found) {
			pid_t pid = fork();
			if (pid < 0) {
				;
			} else if (pid == 0) {
				execlp(argv[0], argv[0], "-storepasswd",
				    (char *) NULL);
				exit(1);
			} else {
				int s;
				waitpid(pid, &s, 0); 
				if (WIFEXITED(s) && WEXITSTATUS(s) == 0) {
					got_rfbauth = 1;
					set_rfbauth = 1;
					found = 1;
				}
			}
		}
#endif
#endif
#endif

		if (set_rfbauth) {
			sprintf(f, "%s/.vnc/passwd", h);
			if (argc_vnc < 100) {
				argv_vnc[argc_vnc++] = strdup("-rfbauth");
			} else {
				exit(1);
			}
			if (argc_vnc < 100) {
				argv_vnc[argc_vnc++] = strdup(f);
			} else {
				exit(1);
			}
		}
		if (! found) {
			fprintf(stderr, "x11vnc -usepw: could not find"
			    " a password to use.\n");
			exit(1);
		}
		free(f);
	}

	if (got_rfbauth && (got_passwd || got_viewpasswd || got_passwdfile)) {
		fprintf(stderr, "option -rfbauth is incompatible with:\n");
		fprintf(stderr, "            -passwd, -viewpasswd, and -passwdfile\n");
		exit(1);
	}
	if (got_passwdfile && (got_passwd || got_viewpasswd)) {
		fprintf(stderr, "option -passwdfile is incompatible with:\n");
		fprintf(stderr, "            -passwd and -viewpasswd\n");
		exit(1);
	}

	/*
	 * If -passwd was used, clear it out of argv.  This does not
	 * work on all UNIX, have to use execvp() in general...
	 */
	if (pw_loc > 0) {
		int i;
		for (i=pw_loc; i <= pw_loc+1; i++) {
			if (i < argc) {
				char *p = argv[i];		
				strzero(p);
			}
		}
	} else if (passwdfile) {
		/* read passwd(s) from file */
		if (read_passwds(passwdfile)) {
			argv_vnc[argc_vnc++] = strdup("-passwd");
			argv_vnc[argc_vnc++] = strdup(passwd_list[0]);
			got_passwd = 1;
			pw_loc = 100;	/* just for pw_loc check below */
		}
	}
	if (vpw_loc > 0) {
		int i;
		for (i=vpw_loc; i <= vpw_loc+1; i++) {
			if (i < argc) {
				char *p = argv[i];		
				strzero(p);
			}
		}
	} 
#ifdef HARDWIRE_PASSWD
	if (!got_rfbauth && !got_passwd) {
		argv_vnc[argc_vnc++] = strdup("-passwd");
		argv_vnc[argc_vnc++] = strdup(HARDWIRE_PASSWD);
		got_passwd = 1;
		pw_loc = 100;
	}
#endif
#ifdef HARDWIRE_VIEWPASSWD
	if (!got_rfbauth && got_passwd && !viewonly_passwd && !passwd_list) {
		viewonly_passwd = strdup(HARDWIRE_VIEWPASSWD);
	}
#endif
	if (viewonly_passwd && pw_loc < 0) {
		rfbLog("-passwd must be supplied when using -viewpasswd\n");
		exit(1);
	}

	if (SHOW_NO_PASSWORD_WARNING) {
		char message[] = "-rfbauth, -passwdfile, -passwd password, "
		    "or -unixpw required.";
		if (! nopw) {
			nopassword_warning_msg(got_localhost);
		}
#if PASSWD_REQUIRED
		rfbLog("%s\n", message);
		exit(1);
#endif
#if PASSWD_UNLESS_NOPW
		if (! nopw) {
			rfbLog("%s\n", message);
			exit(1);
		}
#endif
		message[0] = '\0';	/* avoid compiler warning */
	}

	if (more_safe) {
		if (! quiet) {
			rfbLog("-safer mode:\n");
			rfbLog("   vnc_connect=0\n");
			rfbLog("   accept_remote_cmds=0\n");
			rfbLog("   safe_remote_only=1\n");
			rfbLog("   launch_gui=0\n");
		}
		vnc_connect = 0;
		accept_remote_cmds = 0;
		safe_remote_only = 1;
		launch_gui = 0;
	}

	if (unixpw) {
		if (inetd) {
			use_stunnel = 0;
		}
		if (! use_stunnel && ! use_openssl) {
			if (have_ssh_env()) {
				char *s = getenv("SSH_CONNECTION");
				if (! s) s = getenv("SSH_CLIENT");
				if (! s) s = "SSH_CONNECTION";
				fprintf(stderr, "\n");
				rfbLog("Skipping -ssl/-stunnel contraint in"
				    " -unixpw\n");
				rfbLog("mode, assuming your SSH encryption"
				    " is: %s\n", s);
				rfbLog("Setting -localhost in SSH + -unixpw"
				    " mode.\n");
				fprintf(stderr, "\n");
				allow_list = strdup("127.0.0.1");
				got_localhost = 1;
				if (! nopw) {
					usleep(2000*1000);
				}
			} else if (getenv("UNIXPW_DISABLE_SSL")) {
				rfbLog("Skipping -ssl/-stunnel requirement"
				    " due to\n");
				rfbLog("UNIXPW_DISABLE_SSL setting.\n");
			} else {
				if (openssl_present()) {
					rfbLog("set -ssl in -unixpw mode.\n");
					use_openssl = 1;
				} else if (inetd) {
					rfbLog("could not set -ssl in -inetd"
					    " + -unixpw mode.\n");
					exit(1);
				} else {
					rfbLog("set -stunnel in -unixpw mode.\n");
					use_stunnel = 1;
				}
			}
		}
		if (use_threads) {
			if (! quiet) {
				rfbLog("disabling -threads under -unixpw\n");
			}
			use_threads = 0;
		}
	}
	if (use_stunnel && ! got_localhost) {
		if (! getenv("STUNNEL_DISABLE_LOCALHOST") &&
		    ! getenv("UNIXPW_DISABLE_LOCALHOST")) {
			if (! quiet) {
				rfbLog("Setting -localhost in -stunnel mode.\n");
			}
			allow_list = strdup("127.0.0.1");
			got_localhost = 1;
		}
	}
	if (ssl_verify && ! use_stunnel && ! use_openssl) {
		rfbLog("-sslverify must be used with -ssl or -stunnel\n");
		exit(1);
	}
	if (https_port_num >= 0 && ! use_openssl) {
		rfbLog("-https must be used with -ssl\n");
		exit(1);
	}

	/* fixup settings that do not make sense */
		
	if (use_threads && nofb && cursor_pos_updates) {
		if (! quiet) {
			rfbLog("disabling -threads under -nofb -cursorpos\n");
		}
		use_threads = 0;
	}
	if (tile_fuzz < 1) {
		tile_fuzz = 1;
	}
	if (waitms < 0) {
		waitms = 0;
	}

	if (alpha_threshold < 0) {
		alpha_threshold = 0;
	}
	if (alpha_threshold > 256) {
		alpha_threshold = 256;
	}
	if (alpha_frac < 0.0) {
		alpha_frac = 0.0;
	}
	if (alpha_frac > 1.0) {
		alpha_frac = 1.0;
	}
	if (alpha_blend) {
		alpha_remove = 0;
	}

	if (cmap8to24 && overlay) {
		if (! quiet) {
			rfbLog("disabling -overlay in -8to24 mode.\n");
		}
		overlay = 0;
	}

	if (filexfer && view_only) {
		if (! quiet) {
			rfbLog("setting -nofilexfer in -viewonly mode.\n");
		}
		/* how to undo via -R? */
		filexfer = 0;
	}

	if (inetd) {
		shared = 0;
		connect_once = 1;
		bg = 0;
		if (use_stunnel) {
			exit(1);
		}
		/* others? */
	}

	if (flip_byte_order && using_shm && ! quiet) {
		rfbLog("warning: -flipbyte order only works with -noshm\n");
	}

	if (! wireframe_copyrect) {
		set_wirecopyrect_mode(NULL);
	}
	if (! scroll_copyrect) {
		set_scrollcopyrect_mode(NULL);
	}
	if (screen_fixup_str) {
		parse_fixscreen();
	}
	initialize_scroll_matches();
	initialize_scroll_term();
	initialize_max_keyrepeat();

	/* increase rfbwait if threaded */
	if (use_threads && ! got_rfbwait) {
		rfbMaxClientWait = 604800000;
	}

	/* no framebuffer (Win2VNC) mode */

	if (nofb) {
		/* disable things that do not make sense with no fb */
		set_nofb_params(0);

		if (! got_deferupdate && ! got_defer) {
			/* reduce defer time under -nofb */
			defer_update = defer_update_nofb;
		}
		if (got_pointer_mode < 0) {
			pointer_mode = POINTER_MODE_NOFB;
		}
	}

	if (raw_fb_str) {
		set_raw_fb_params(0);
	}
	if (! got_deferupdate) {
		char tmp[40];
		sprintf(tmp, "%d", defer_update);
		argv_vnc[argc_vnc++] = strdup("-deferupdate");
		argv_vnc[argc_vnc++] = strdup(tmp);
	}

	if (debug_pointer || debug_keyboard) {
		if (bg || quiet) {
			rfbLog("disabling -bg/-q under -debug_pointer"
			    "/-debug_keyboard\n");
			bg = 0;
			quiet = 0;
		}
	}

	/* initialize added_keysyms[] array to zeros */
	add_keysym(NoSymbol);

	/* tie together cases of -localhost vs. -listen localhost */
	if (! listen_str) {
		if (allow_list && !strcmp(allow_list, "127.0.0.1")) {
			listen_str = strdup("localhost");
			argv_vnc[argc_vnc++] = strdup("-listen");
			argv_vnc[argc_vnc++] = strdup(listen_str);
		}
	} else if (!strcmp(listen_str, "localhost") ||
	    !strcmp(listen_str, "127.0.0.1")) {
		allow_list = strdup("127.0.0.1");
	}

	/* set OS struct UT */
	uname(&UT);

	initialize_crash_handler();

	if (! quiet) {
		print_settings(try_http, bg, gui_str);
	} else {
		rfbLogEnable(0);
	}

	X_INIT;
	SCR_INIT;
	
	/* open the X display: */
	if (auth_file) {
		set_env("XAUTHORITY", auth_file);
	}
#if LIBVNCSERVER_HAVE_XKEYBOARD
	/*
	 * Disable XKEYBOARD before calling XOpenDisplay()
	 * this should be used if there is ambiguity in the keymapping. 
	 */
	if (xkbcompat) {
		Bool rc = XkbIgnoreExtension(True);
		if (! quiet) {
			rfbLog("Disabling xkb XKEYBOARD extension. rc=%d\n",
			    rc);
		}
		if (watch_bell) {
			watch_bell = 0;
			if (! quiet) rfbLog("Disabling bell.\n");
		}
	}
#else
	watch_bell = 0;
	use_xkb_modtweak = 0;
#endif

#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
	if (filexfer) {
		rfbRegisterTightVNCFileTransferExtension();
	} else {
		rfbUnregisterTightVNCFileTransferExtension();
	}
#endif

	initialize_allowed_input();

	if (users_list && strstr(users_list, "lurk=")) {
		if (use_dpy) {
			rfbLog("warning: -display does not make sense in "
			    "\"lurk=\" mode...\n");
		}
		lurk_loop(users_list);
	} else if (use_dpy && strstr(use_dpy, "WAIT:") == use_dpy) {
		waited_for_client = wait_for_client(&argc_vnc, argv_vnc,
		    try_http && ! got_httpdir);
	}

	if (use_dpy) {
		dpy = XOpenDisplay_wr(use_dpy);
	} else if ( (use_dpy = getenv("DISPLAY")) ) {
		dpy = XOpenDisplay_wr(use_dpy);
	} else {
		dpy = XOpenDisplay_wr("");
	}

	if (! dpy && raw_fb_str) {
		rfbLog("continuing without X display in -rawfb mode, "
		    "hold on tight..\n");
		goto raw_fb_pass_go_and_collect_200_dollars;
	}

	if (! dpy && ! use_dpy && ! getenv("DISPLAY")) {
		int i, s = 4;
		rfbLogEnable(1);
		rfbLog("\a\n");
		rfbLog("*** XOpenDisplay failed. No -display or DISPLAY.\n");
		rfbLog("*** Trying \":0\" in %d seconds.  Press Ctrl-C to"
		    " abort.\n", s);
		rfbLog("*** ");
		for (i=1; i<=s; i++)  {
			fprintf(stderr, "%d ", i);
			sleep(1);
		}
		fprintf(stderr, "\n");
		use_dpy = ":0";
		dpy = XOpenDisplay_wr(use_dpy);
		if (dpy) {
			rfbLog("*** XOpenDisplay of \":0\" successful.\n");
		}
		rfbLog("\n");
		if (quiet) rfbLogEnable(0);
	}

	if (! dpy) {
		char *d = use_dpy;
		if (!d) d = getenv("DISPLAY");
		if (!d) d = "null";
		rfbLogEnable(1);
		fprintf(stderr, "\n");
		rfbLog("***************************************\n", d);
		rfbLog("*** XOpenDisplay failed (%s)\n", d);
		xopen_display_fail_message(d);
		exit(1);
	} else if (use_dpy) {
		if (! quiet) rfbLog("Using X display %s\n", use_dpy);
	} else {
		if (! quiet) rfbLog("Using default X display.\n");
	}

	scr = DefaultScreen(dpy);
	rootwin = RootWindow(dpy, scr);

	if (! quiet) {
		rfbLog("\n");
		rfbLog("------------------ USEFUL INFORMATION ------------------\n");
	}

	if (remote_cmd || query_cmd) {
		int rc = do_remote_query(remote_cmd, query_cmd, remote_sync,
		    query_default);
		XFlush_wr(dpy);
		fflush(stderr);
		fflush(stdout);
		usleep(30 * 1000);	/* still needed? */
		XCloseDisplay_wr(dpy);
		exit(rc);
	}

	if (priv_remote) {
		if (! remote_control_access_ok()) {
			rfbLog("** Disabling remote commands in -privremote "
			    "mode.\n");
			accept_remote_cmds = 0;
		}
	}

	sync_tod_with_servertime();
	if (grab_buster) {
		spawn_grab_buster();
	}

#if LIBVNCSERVER_HAVE_LIBXFIXES
	if (! XFixesQueryExtension(dpy, &xfixes_base_event_type, &er)) {
		if (! quiet) {
			rfbLog("Disabling XFIXES mode: display does not "
			    "support it.\n");
		}
		xfixes_base_event_type = 0;
		xfixes_present = 0;
	} else {
		xfixes_present = 1;
	}
#endif
	if (! xfixes_present) {
		use_xfixes = 0;
	}

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	if (! XDamageQueryExtension(dpy, &xdamage_base_event_type, &er)) {
		if (! quiet) {
			rfbLog("Disabling X DAMAGE mode: display does not "
			    "support it.\n");
		}
		xdamage_base_event_type = 0;
		xdamage_present = 0;
	} else {
		xdamage_present = 1;
	}
#endif
	if (! xdamage_present) {
		use_xdamage = 0;
	}
	if (! quiet && xdamage_present && use_xdamage) {
		rfbLog("X DAMAGE available on display, using it for"
		    " polling hints.\n");
		rfbLog("  To disable this behavior use: "
		    "'-noxdamage'\n");
	}

	if (! quiet && wireframe) {
		rfbLog("Wireframing: -wireframe mode is in effect for window "
		    "moves.\n");
		rfbLog("  If this yields undesired behavior (poor response, "
		    "painting\n");
		rfbLog("  errors, etc) it may be disabled:\n");
		rfbLog("   - use '-nowf' to disable wireframing completely.\n");
		rfbLog("   - use '-nowcr' to disable the Copy Rectangle after "
		    "the\n");
		rfbLog("     moved window is released in the new position.\n");
		rfbLog("  Also see the -help entry for tuning parameters.\n");
		rfbLog("  You can press 3 Alt_L's (Left \"Alt\" key) in a row"
		    " to \n");
		rfbLog("  repaint the screen, also see the -fixscreen option"
		    " for\n");
		rfbLog("  periodic repaints.\n");
		if (scale_str && !strstr(scale_str, "nocr")) {
			rfbLog("  Note: '-scale' is on and this can cause "
			    "more problems.\n");
		}
	}

	overlay_present = 0;
#if defined(SOLARIS_OVERLAY) && !NO_X11
	if (! XQueryExtension(dpy, "SUN_OVL", &maj, &ev, &er)) {
		if (! quiet && overlay) {
			rfbLog("Disabling -overlay: SUN_OVL "
			    "extension not available.\n");
		}
	} else {
		overlay_present = 1;
	}
#endif
#if defined(IRIX_OVERLAY) && !NO_X11
	if (! XReadDisplayQueryExtension(dpy, &ev, &er)) {
		if (! quiet && overlay) {
			rfbLog("Disabling -overlay: IRIX ReadDisplay "
			    "extension not available.\n");
		}
	} else {
		overlay_present = 1;
	}
#endif
	if (overlay && !overlay_present) {
		overlay = 0;
		overlay_cursor = 0;
	}

	/* cursor shapes setup */
	if (! multiple_cursors_mode) {
		multiple_cursors_mode = strdup("default");
	}
	if (show_cursor) {
		if(!strcmp(multiple_cursors_mode, "default")
		    && xfixes_present && use_xfixes) {
			free(multiple_cursors_mode);
			multiple_cursors_mode = strdup("most");

			if (! quiet) {
				rfbLog("XFIXES available on display, resetting"
				    " cursor mode\n");
				rfbLog("  to: '-cursor most'.\n");
				rfbLog("  to disable this behavior use: "
				    "'-cursor arrow'\n");
				rfbLog("  or '-noxfixes'.\n");
			}
		}
		if(!strcmp(multiple_cursors_mode, "most")) {
			if (xfixes_present && use_xfixes &&
			    overlay_cursor == 1) {
				if (! quiet) {
					rfbLog("using XFIXES for cursor "
					    "drawing.\n");
				}
				overlay_cursor = 0;
			}
		}
	}

	if (overlay) {
		using_shm = 0;
		if (flash_cmap && ! quiet) {
			rfbLog("warning: -flashcmap may be "
			    "incompatible with -overlay\n");
		}
		if (show_cursor && overlay_cursor) {
			char *s = multiple_cursors_mode;
			if (*s == 'X' || !strcmp(s, "some") ||
			    !strcmp(s, "arrow")) {
				/*
				 * user wants these modes, so disable fb cursor
				 */
				overlay_cursor = 0;
			} else {
				/*
				 * "default" and "most", we turn off
				 * show_cursor since it will automatically
				 * be in the framebuffer.
				 */
				show_cursor = 0;
			}
		}
	}

	initialize_cursors_mode();

	/* check for XTEST */
	if (! XTestQueryExtension_wr(dpy, &ev, &er, &maj, &min)) {
		if (! quiet) {
			rfbLog("WARNING: XTEST extension not available "
			    "(either missing from\n");
			rfbLog("  display or client library libXtst "
			    "missing at build time).\n");
			rfbLog("  MOST user input (pointer and keyboard) "
			    "will be DISCARDED.\n");
			rfbLog("  If display does have XTEST, be sure to "
			    "build x11vnc with\n");
			rfbLog("  a working libXtst build environment "
			    "(e.g. libxtst-dev,\n");
			rfbLog("  or other packages).\n");
			rfbLog("No XTEST extension, switching to "
			    "-xwarppointer mode for\n");
			rfbLog("  pointer motion input.\n");
		}
		xtest_present = 0;
		use_xwarppointer = 1;
	} else {
		xtest_present = 1;
		xtest_base_event_type = ev;
		if (maj <= 1 || (maj == 2 && min <= 2)) {
			/* no events defined as of 2.2 */
			xtest_base_event_type = 0;
		}
	}

	if (! XETrapQueryExtension_wr(dpy, &ev, &er, &maj)) {
		xtrap_present = 0;
	} else {
		xtrap_present = 1;
		xtrap_base_event_type = ev;
	}

	/*
	 * Window managers will often grab the display during resize,
	 * etc, using XGrabServer().  To avoid deadlock (our user resize
	 * input is not processed) we tell the server to process our
	 * requests during all grabs:
	 */
	disable_grabserver(dpy, 0);

	/* check for RECORD */
	if (! XRecordQueryVersion_wr(dpy, &maj, &min)) {
		xrecord_present = 0;
	} else {
		xrecord_present = 1;
	}

	initialize_xrecord();

	tmpi = 1;
	if (scroll_copyrect) {
		if (strstr(scroll_copyrect, "never")) {
			tmpi = 0;
		}
	} else if (scroll_copyrect_default) {
		if (strstr(scroll_copyrect_default, "never")) {
			tmpi = 0;
		}
	}
	if (! xrecord_present) {
		tmpi = 0;
	}
#if !LIBVNCSERVER_HAVE_RECORD
	tmpi = 0;
#endif
	if (! quiet && tmpi) {
		rfbLog("Scroll Detection: -scrollcopyrect mode is in effect "
		    "to\n");
		rfbLog("  use RECORD extension to try to detect scrolling "
		    "windows\n");
		rfbLog("  (induced by either user keystroke or mouse input).\n");
		rfbLog("  If this yields undesired behavior (poor response, "
		    "painting\n");
		rfbLog("  errors, etc) it may be disabled via: '-noscr'\n");
		rfbLog("  Also see the -help entry for tuning parameters.\n");
		rfbLog("  You can press 3 Alt_L's (Left \"Alt\" key) in a row"
		    " to \n");
		rfbLog("  repaint the screen, also see the -fixscreen option"
		    " for\n");
		rfbLog("  periodic repaints.\n");
		if (scale_str && !strstr(scale_str, "nocr")) {
			rfbLog("  Note: '-scale' is on and this can cause "
			    "more problems.\n");
		}
	}

	/* check for OS with small shm limits */
	if (using_shm && ! single_copytile) {
		if (limit_shm()) {
			single_copytile = 1;
		}
	}

	single_copytile_orig = single_copytile;

	/* check for MIT-SHM */
	if (! XShmQueryExtension_wr(dpy)) {
		xshm_present = 0;
		if (! using_shm) {
			if (! quiet) {
				rfbLog("info: display does not support"
				    " XShm.\n");
			}
		} else {
		    if (! quiet) {
			rfbLog("warning: XShm extension is not available.\n");
			rfbLog("For best performance the X Display should be"
			    " local. (i.e.\n");
			rfbLog("the x11vnc and X server processes should be"
			    " running on\n");
			rfbLog("the same machine.)\n");
#if LIBVNCSERVER_HAVE_XSHM
			rfbLog("Restart with -noshm to override this.\n");
		    }
		    exit(1);
#else
			rfbLog("Switching to -noshm mode.\n");
		    }
		    using_shm = 0;
#endif
		}
	}

#if LIBVNCSERVER_HAVE_XKEYBOARD
	/* check for XKEYBOARD */
	initialize_xkb();
	initialize_watch_bell();
	if (!xkb_present && use_xkb_modtweak) {
		if (! quiet) {
			rfbLog("warning: disabling xkb modtweak."
			    " XKEYBOARD ext. not present.\n");
		}
		use_xkb_modtweak = 0;
	}
#endif

	if (xkb_present && !use_xkb_modtweak && !got_noxkb) {
		if (use_modifier_tweak) {
			switch_to_xkb_if_better();
		}
	}

#if LIBVNCSERVER_HAVE_LIBXRANDR
	if (! XRRQueryExtension(dpy, &xrandr_base_event_type, &er)) {
		if (xrandr && ! quiet) {
			rfbLog("Disabling -xrandr mode: display does not"
			    " support X RANDR.\n");
		}
		xrandr_base_event_type = 0;
		xrandr = 0;
		xrandr_present = 0;
	} else {
		xrandr_present = 1;
	}
#endif

	check_pm();

	if (! quiet) {
		rfbLog("--------------------------------------------------------\n");
		rfbLog("\n");
	}

	raw_fb_pass_go_and_collect_200_dollars:

	if (! dt) {
		static char str[] = "-desktop";
		argv_vnc[argc_vnc++] = str;
		argv_vnc[argc_vnc++] = choose_title(use_dpy);
		rfb_desktop_name = strdup(argv_vnc[argc_vnc-1]);
	}
	
	/*
	 * Create the XImage corresponding to the display framebuffer.
	 */

	fb0 = initialize_xdisplay_fb();

	/*
	 * In some cases (UINPUT touchscreens) we need the dpy_x dpy_y
	 * to initialize pipeinput. So we do it after fb is created.
	 */
	initialize_pipeinput();

	/*
	 * n.b. we do not have to X_LOCK any X11 calls until watch_loop()
	 * is called since we are single-threaded until then.
	 */

	initialize_screen(&argc_vnc, argv_vnc, fb0);

	if (waited_for_client) {
		if (fake_fb) {
			free(fake_fb);
			fake_fb = NULL;
		}
		if (use_solid_bg && client_count) {
			solid_bg(0);
		}
		if (accept_cmd && strstr(accept_cmd, "popup") == accept_cmd) {
			rfbClientIteratorPtr iter;
			rfbClientPtr cl, cl0 = NULL;
			int i = 0;
			iter = rfbGetClientIterator(screen);
			while( (cl = rfbClientIteratorNext(iter)) ) {
				i++;	
				if (i != 1) {
					rfbLog("WAIT popup: too many clients\n");
					clean_up_exit(1);
				}
				cl0 = cl;
			}
			rfbReleaseClientIterator(iter);
			if (i != 1 || cl0 == NULL) {
				rfbLog("WAIT popup: no clients.\n");
				clean_up_exit(1);
			}
			if (! accept_client(cl0)) {
				rfbLog("WAIT popup: denied.\n");
				clean_up_exit(1);
			}
			rfbLog("waited_for_client: popup accepted.\n");
			cl0->onHold = FALSE;
		}
	}

	if (! waited_for_client) {
		if (try_http && ! got_httpdir && check_httpdir()) {
			http_connections(1);
		}
	}

	initialize_tiles();

	/* rectangular blackout regions */
	initialize_blackouts_and_xinerama();

	/* created shm or XImages when using_shm = 0 */
	initialize_polling_images();

	initialize_signals();

	initialize_speeds();

	initialize_keyboard_and_pointer();

	if (inetd && use_openssl) {
		if (! waited_for_client) {
			accept_openssl(OPENSSL_INETD);
		}
	}
	if (! inetd && ! use_openssl) {
		if (! screen->port || screen->listenSock < 0) {
			rfbLogEnable(1);
			rfbLog("Error: could not obtain listening port.\n");
			clean_up_exit(1);
		}
	}
	if (! quiet) {
		rfbLog("screen setup finished.\n");
		if (SHOW_NO_PASSWORD_WARNING && !nopw) {
			rfbLog("\n");
			rfbLog("WARNING: You are running x11vnc WITHOUT"
			    " a password.  See\n");
			rfbLog("WARNING: the warning message printed above"
			    " for more info.\n");
		}
	}
	set_vnc_desktop_name();

#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SETSID
	if (bg) {
		/* fork into the background now */
		int p, n;
		if ((p = fork()) > 0)  {
			exit(0);
		} else if (p == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "could not fork\n");
			perror("fork");
			clean_up_exit(1);
		}
		if (setsid() == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "setsid failed\n");
			perror("setsid");
			clean_up_exit(1);
		}
		/* adjust our stdio */
		n = open("/dev/null", O_RDONLY);
		dup2(n, 0);
		dup2(n, 1);
		if (! logfile) {
			dup2(n, 2);
		}
		if (n > 2) {
			close(n);
		}
	}
#endif

	watch_loop();

	return(0);

#undef argc
#undef argv

}
