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

/* -- macosxCGS.c -- */

/*
 * We need to keep this separate from nearly everything else, e.g. rfb.h
 * and the other stuff, otherwise it does not work properly, mouse drags
 * will not work!!
 */
void macosxCGS_unused(void) {}

#if (defined(__MACH__) && defined(__APPLE__))

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

extern CGDirectDisplayID displayID;

void macosxCGS_get_all_windows(void);
int macosxCGS_get_qlook(int);
void macosxGCS_set_pasteboard(char *str, int len);

typedef CGError       CGSError;
typedef long          CGSWindowCount;
typedef void *        CGSConnectionID;
typedef int           CGSWindowID;
typedef CGSWindowID*  CGSWindowIDList;
typedef CGWindowLevel CGSWindowLevel;
typedef NSRect        CGSRect;

extern CGSConnectionID _CGSDefaultConnection ();

extern CGSError CGSGetOnScreenWindowList (CGSConnectionID cid,
    CGSConnectionID owner, CGSWindowCount listCapacity,
    CGSWindowIDList list, CGSWindowCount *listCount);

extern CGSError CGSGetWindowList (CGSConnectionID cid,
    CGSConnectionID owner, CGSWindowCount listCapacity,
    CGSWindowIDList list, CGSWindowCount *listCount);

extern CGSError CGSGetScreenRectForWindow (CGSConnectionID cid,
    CGSWindowID wid, CGSRect *rect);

extern CGWindowLevel CGSGetWindowLevel (CGSConnectionID cid,
    CGSWindowID wid, CGSWindowLevel *level);

typedef enum _CGSWindowOrderingMode {
    kCGSOrderAbove                =  1, /* Window is ordered above target. */
    kCGSOrderBelow                = -1, /* Window is ordered below target. */
    kCGSOrderOut                  =  0  /* Window is removed from the on-screen window list. */
} CGSWindowOrderingMode;

extern OSStatus CGSOrderWindow(const CGSConnectionID cid,
    const CGSWindowID wid, CGSWindowOrderingMode place, CGSWindowID relativeToWindowID);

static CGSConnectionID cid = NULL;

extern void macosx_log(char *);

int macwinmax = 0; 
typedef struct windat {
	int win;
	int x, y;
	int width, height;
	int level;
	int mapped;
	int clipped;
	int ncache_only;
} windat_t;

extern int ncache;

#define MAXWINDAT 4096
windat_t macwins[MAXWINDAT]; 
static CGSWindowID _wins_all[MAXWINDAT]; 
static CGSWindowID _wins_mapped[MAXWINDAT]; 
static CGSWindowCount _wins_all_cnt, _wins_mapped_cnt; 
static int _wins_int[MAXWINDAT]; 

#define WINHISTNUM 32768
#define WINHISTMAX 4
char whist[WINHISTMAX][WINHISTNUM];
int whist_idx = -1;
int qlook[WINHISTNUM];

char is_exist     = 0x1;
char is_mapped    = 0x2;
char is_clipped   = 0x4;
char is_offscreen = 0x8;

extern double dnow(void);
extern double dnowx(void);

extern int dpy_x, dpy_y;
extern int macosx_icon_anim_time;

extern void macosx_add_mapnotify(int, int, int);
extern void macosx_add_create(int, int);
extern void macosx_add_destroy(int, int);
extern void macosx_add_visnotify(int, int, int);

int CGS_levelmax;
int CGS_levels[16];

int macosxCGS_get_qlook(int w) {
	if (w >= WINHISTNUM) {
		return -1;
	}
	return qlook[w];
}

int macosxCGS_find_index(int w) {
	static int last_index = -1;
	int idx;

	if (last_index >= 0) {
		if (macwins[last_index].win == w) {
			return last_index;
		}
	}

	idx = macosxCGS_get_qlook(w);
	if (idx >= 0) {
		if (macwins[idx].win == w) {
			last_index = idx;
			return idx;
		}
	}

	for (idx=0; idx < macwinmax; idx++) {
		if (macwins[idx].win == w) {
			last_index = idx;
			return idx;
		}
	}
	return -1;
}

#if 0
extern void usleep(unsigned long usec);
#else
extern int usleep(useconds_t usec);
#endif

int macosxCGS_follow_animation_win(int win, int idx, int grow) {
	double t = dnow();
	int diffs = 0;
	int x, y, w, h;
	int xp = -1, yp = -1, wp = -1, hp = -1;
	CGSRect rect;
	CGSError err; 

	int reps = 0;

	if (cid == NULL) {
		cid = _CGSDefaultConnection();
		if (cid == NULL) {
			return 0;
		}
	}

	if (idx < 0) {
		idx = macosxCGS_find_index(win); 
	}
	if (idx < 0) {
		return 0;
	}

	while (dnow() < t + 0.001 * macosx_icon_anim_time)  {
		err = CGSGetScreenRectForWindow(cid, win, &rect);
		if (err != 0) {
			break;
		}
		x = (int) rect.origin.x;
		y = (int) rect.origin.y;
		w = (int) rect.size.width;
		h = (int) rect.size.height;

		if (grow) {
			macwins[idx].x      = x;
			macwins[idx].y      = y;
			macwins[idx].width  = w;
			macwins[idx].height = h;
		}
	
		if (0) fprintf(stderr, " chase: %03dx%03d+%03d+%03d  %d\n", w, h, x, y, win);
		if (x == xp && y == yp && w == wp && h == hp)  {
			reps++;
			if (reps >= 2) {
				break;
			}
		} else {
			diffs++;
			reps = 0;
		}
		xp = x;
		yp = y;
		wp = w;
		hp = h;
		usleep(50 * 1000);
	}
	if (diffs >= 2) {
		return 1;
	} else {
		return 0;
	}
}

extern int macosx_check_clipped(int win, int *list, int n);
extern int macosx_check_offscreen(int win);

static int check_clipped(int win) {
	int i, n = 0, win2;
	for (i = 0; i < (int) _wins_mapped_cnt; i++) {
		win2 = (int) _wins_mapped[i];
		if (win2 == win) {
			break;
		}
		_wins_int[n++] = win2;
	}
	return macosx_check_clipped(win, _wins_int, n);
}

static int check_offscreen(int win) {
	return macosx_check_offscreen(win);
}

extern int macosx_ncache_macmenu;


void macosxCGS_get_all_windows(void) {
	static double last = 0.0;
	static int totcnt = 0;
	double dt = 0.0, now = dnow();
	int i, db = 0, whist_prv = 0, maxwin = 0, whist_skip = 0;
	CGSWindowCount cap = (CGSWindowCount) MAXWINDAT;
	CGSError err; 

	CGS_levelmax = 0;
	CGS_levels[CGS_levelmax++] = (int) kCGDraggingWindowLevel;	/* 500 ? */
	if (0) CGS_levels[CGS_levelmax++] = (int) kCGHelpWindowLevel;		/* 102 ? */
	if (macosx_ncache_macmenu) CGS_levels[CGS_levelmax++] = (int) kCGPopUpMenuWindowLevel;	/* 101 pulldown menu */
	CGS_levels[CGS_levelmax++] = (int) kCGMainMenuWindowLevelKey;	/*  24 ? */
	CGS_levels[CGS_levelmax++] = (int) kCGModalPanelWindowLevel;	/*   8 open dialog box */
	CGS_levels[CGS_levelmax++] = (int) kCGFloatingWindowLevel;	/*   3 ? */
	CGS_levels[CGS_levelmax++] = (int) kCGNormalWindowLevel;	/*   0 regular window */

	if (cid == NULL) {
		cid = _CGSDefaultConnection();
		if (cid == NULL) {
			return;
		}
	}

	if (dt > 0.0 && now < last + dt) {
		return;
	}

	last = now;

	macwinmax = 0; 

	totcnt++;

	if (ncache > 0) {
		whist_prv = whist_idx++;
		if (whist_prv < 0) {
			whist_skip = 1;
			whist_prv = 0;
		}
		whist_idx = whist_idx % WINHISTMAX;
		for (i=0; i < WINHISTNUM; i++) {
			whist[whist_idx][i] = 0;
			qlook[i] = -1;
		}
	}

	err = CGSGetWindowList(cid, NULL, cap, _wins_all, &_wins_all_cnt);

if (db) fprintf(stderr, "cnt: %d err: %d\n", (int) _wins_all_cnt, err);

	if (err != 0) {
		return;
	}
	
	for (i=0; i < (int) _wins_all_cnt; i++) {
		CGSRect rect;
		CGSWindowLevel level;
		int j, keepit = 0;
		err = CGSGetScreenRectForWindow(cid, _wins_all[i], &rect);
		if (err != 0) {
			continue;
		}
		if (rect.origin.x == 0 && rect.origin.y == 0) {
			if (rect.size.width == dpy_x) {
				if (rect.size.height == dpy_y) {
					continue;
				}
			}
		}
		err = CGSGetWindowLevel(cid, _wins_all[i], &level);
		if (err != 0) {
			continue;
		}
		for (j=0; j<CGS_levelmax; j++) {
			if ((int) level == CGS_levels[j]) {
				keepit = 1;
				break;
			}
		}
		if (! keepit) {
			continue;
		}

		macwins[macwinmax].level  = (int) level;
		macwins[macwinmax].win    = (int) _wins_all[i];
		macwins[macwinmax].x      = (int) rect.origin.x;
		macwins[macwinmax].y      = (int) rect.origin.y;
		macwins[macwinmax].width  = (int) rect.size.width;
		macwins[macwinmax].height = (int) rect.size.height;
		macwins[macwinmax].mapped = 0;
		macwins[macwinmax].clipped = 0;
		macwins[macwinmax].ncache_only = 0;
		if (level == kCGPopUpMenuWindowLevel) {
			macwins[macwinmax].ncache_only = 1;
		}

if (0 || db) fprintf(stderr, "i=%03d ID: %06d  x: %03d  y: %03d  w: %03d h: %03d level: %d\n", i, _wins_all[i],
    (int) rect.origin.x, (int) rect.origin.y,(int) rect.size.width, (int) rect.size.height, (int) level);

		if (macwins[macwinmax].win < WINHISTNUM) {
			qlook[macwins[macwinmax].win] = macwinmax;
			if (macwins[macwinmax].win > maxwin) {
				maxwin = macwins[macwinmax].win;
			}
		}

		macwinmax++;
	}

	err = CGSGetOnScreenWindowList(cid, NULL, cap, _wins_mapped, &_wins_mapped_cnt);

if (db) fprintf(stderr, "cnt: %d err: %d\n", (int) _wins_mapped_cnt, err);

	if (err != 0) {
		return;
	}
	
	for (i=0; i < (int) _wins_mapped_cnt; i++) {
		int j, idx = -1;
		int win = (int) _wins_mapped[i];

		if (0 <= win && win < WINHISTNUM) {
			j = qlook[win];
			if (j >= 0 && macwins[j].win == win) {
				idx = j; 
			}
		}
		if (idx < 0) {
			for (j=0; j < macwinmax; j++) {
				if (macwins[j].win == win) {
					idx = j; 
					break;
				}
			}
		}
		if (idx >= 0) {
			macwins[idx].mapped = 1;
		}
	}

	if (ncache > 0) {
		int nv= 0, NBMAX = 64;
		int nv_win[64];
		int nv_lvl[64];
		int nv_vis[64];

		for (i=0; i < macwinmax; i++) {
			int win = macwins[i].win;
			char prev, curr;

			if (win >= WINHISTNUM) {
				continue;
			}

			whist[whist_idx][win] |= is_exist;
			if (macwins[i].mapped) {
				whist[whist_idx][win] |= is_mapped;
				if (check_clipped(win)) {
					whist[whist_idx][win] |= is_clipped;
					macwins[i].clipped = 1;
				}
				if (check_offscreen(win)) {
					whist[whist_idx][win] |= is_offscreen;
				}
			} else {
				whist[whist_idx][win] |= is_offscreen;
			}

			curr = whist[whist_idx][win];
			prev = whist[whist_prv][win];

			if (whist_skip) {
				;
			} else if ( !(prev & is_mapped) && (curr & is_mapped)) {
				/* MapNotify */
				if (0) fprintf(stderr, "MapNotify:   %d/%d  %d               %.4f tot=%d\n", prev, curr, win, dnowx(), totcnt); 
				macosx_add_mapnotify(win, macwins[i].level, 1);
				if (0) macosxCGS_follow_animation_win(win, i, 1);

			} else if ( !(curr & is_mapped) && (prev & is_mapped)) {
				/* UnmapNotify */
				if (0) fprintf(stderr, "UnmapNotify: %d/%d  %d               %.4f A tot=%d\n", prev, curr, win, dnowx(), totcnt); 
				macosx_add_mapnotify(win, macwins[i].level, 0);
			} else if ( !(prev & is_exist) && (curr & is_exist)) {
				/* CreateNotify */
				if (0) fprintf(stderr, "CreateNotify:%d/%d  %d               %.4f whist: %d/%d 0x%x tot=%d\n", prev, curr, win, dnowx(), whist_prv, whist_idx, win, totcnt); 
				macosx_add_create(win, macwins[i].level);
				if (curr & is_mapped) {
					if (0) fprintf(stderr, "MapNotify:   %d/%d  %d               %.4f tot=%d\n", prev, curr, win, dnowx(), totcnt); 
					macosx_add_mapnotify(win, macwins[i].level, 1);
				}
			}
			if (whist_skip) {
				;
			} else if (nv >= NBMAX) {
				;
			} else if (!(curr & is_mapped)) {
				;
			} else if (!(prev & is_mapped)) {
				if (1) {
					;
				} else if (curr & is_clipped) {
					if (0) fprintf(stderr, "VisibNotify: %d/%d  %d               OBS tot=%d\n", prev, curr, win, totcnt); 
					nv_win[nv] = win;
					nv_lvl[nv] = macwins[i].level;
					nv_vis[nv++] = 1;
				} else {
					if (0) fprintf(stderr, "VisibNotify: %d/%d  %d               UNOBS tot=%d\n", prev, curr, win, totcnt); 
					nv_win[nv] = win;
					nv_lvl[nv] = macwins[i].level;
					nv_vis[nv++] = 0;
				}
			} else {
				if        ( !(prev & is_clipped) &&  (curr & is_clipped) ) {
					if (0) fprintf(stderr, "VisibNotify: %d/%d  %d               OBS tot=%d\n", prev, curr, win, totcnt); 
					nv_win[nv] = win;
					nv_lvl[nv] = macwins[i].level;
					nv_vis[nv++] = 1;
				} else if (  (prev & is_clipped) && !(curr & is_clipped) ) {
					if (0) fprintf(stderr, "VisibNotify: %d/%d  %d               UNOBS tot=%d\n", prev, curr, win, totcnt); 
					nv_win[nv] = win;
					nv_lvl[nv] = macwins[i].level;
					nv_vis[nv++] = 0;
				}
			}
		}
		for (i=0; i < maxwin; i++) {
			char prev, curr;
			int win = i;
			int q = qlook[i];
			int lvl = 0;

			if (whist_skip) {
				break;
			}

			if (q >= 0) {
				lvl = macwins[q].level;	
			}
			curr = whist[whist_idx][win];
			prev = whist[whist_prv][win];
			if (!(curr & is_exist) && (prev & is_exist)) {
				if (prev & is_mapped) {
					if (0) fprintf(stderr, "UnmapNotify: %d/%d  %d               %.4f B tot=%d\n", prev, curr, win, dnowx(), totcnt); 
					macosx_add_mapnotify(win, lvl, 0);
				}
				/* DestroyNotify */
				if (0) fprintf(stderr, "DestroNotify:%d/%d  %d               %.4f tot=%d\n", prev, curr, win, dnowx(), totcnt); 
				macosx_add_destroy(win, lvl);
			}
		}
		if (nv) {
			int k;
			for (k = 0; k < nv; k++) {
				macosx_add_visnotify(nv_win[k], nv_lvl[k], nv_vis[k]);
			}
		}
	}
}

#if 1
NSLock *pblock = nil;
NSString *pbstr = nil;
NSString *cuttext = nil;

int pbcnt = -1;
NSStringEncoding pbenc = NSWindowsCP1252StringEncoding;

void macosxGCS_initpb(void) {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	pblock = [[NSLock alloc] init];
	if (![NSPasteboard generalPasteboard]) {
		macosx_log("macosxGCS_initpb: **PASTEBOARD INACCESSIBLE**.\n");
		macosx_log("macosxGCS_initpb: Clipboard exchange will NOT work.\n");
		macosx_log("macosxGCS_initpb: Start x11vnc *inside* Aqua for Clipboard.\n");
		pbcnt = 0;
		pbstr = [[NSString alloc] initWithString:@"\e<PASTEBOARD INACCESSIBLE>\e"]; 
	}
	[pool release];
}

void macosxGCS_set_pasteboard(char *str, int len) {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if (pbcnt != 0) {
		[pblock lock];
		[cuttext release];
		cuttext = [[NSString alloc] initWithData:[NSData dataWithBytes:str length:len] encoding: pbenc];
		if ([[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil]) {
			NS_DURING
				[[NSPasteboard generalPasteboard] setString:cuttext forType:NSStringPboardType];
			NS_HANDLER
				fprintf(stderr, "macosxGCS_set_pasteboard: problem writing to pasteboard\n");
			NS_ENDHANDLER
		} else {
			fprintf(stderr, "macosxGCS_set_pasteboard: problem writing to pasteboard\n");
		}
		[cuttext release];
		cuttext = nil;
		[pblock unlock];
	}
	[pool release];
}

extern void macosx_send_sel(char *, int);

void macosxGCS_poll_pb(void) {

	static double dlast = 0.0;
	double now = dnow();

	if (now < dlast + 0.2) {
		return;
	}
	dlast = now;

   {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[pblock lock];
	if (pbcnt != [[NSPasteboard generalPasteboard] changeCount]) {
		pbcnt = [[NSPasteboard generalPasteboard] changeCount];
		[pbstr release];
		pbstr = nil;
		if ([[NSPasteboard generalPasteboard] availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]]) {
			pbstr = [[[NSPasteboard generalPasteboard] stringForType:NSStringPboardType] copy];
			if (pbstr) {
				NSData *str = [pbstr dataUsingEncoding:pbenc allowLossyConversion:YES];
				if ([str length]) {
					macosx_send_sel((char *) [str bytes], [str length]);
				}
			}
		}
	}
	[pblock unlock];
	[pool release];
   }
}
#endif

#endif	/* __APPLE__ */

