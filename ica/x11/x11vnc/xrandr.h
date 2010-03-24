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

#ifndef _X11VNC_XRANDR_H
#define _X11VNC_XRANDR_H

/* -- xrandr.h -- */

extern time_t last_subwin_trap;
extern int subwin_trap_count;
extern XErrorHandler old_getimage_handler;

extern int xrandr_present;
extern int xrandr_width;
extern int xrandr_height;
extern int xrandr_rotation;
extern Time xrandr_timestamp;
extern Time xrandr_cfg_time;

extern void initialize_xrandr(void);
extern int check_xrandr_event(char *msg);
extern int known_xrandr_mode(char *s);

#define XRANDR_SET_TRAP_RET(x,y)  \
	if (subwin || xrandr) { \
		trapped_getimage_xerror = 0; \
		old_getimage_handler = XSetErrorHandler(trap_getimage_xerror); \
		if (check_xrandr_event(y)) { \
			trapped_getimage_xerror = 0; \
			XSetErrorHandler(old_getimage_handler);	 \
			X_UNLOCK; \
			return(x); \
		} \
	}
#define XRANDR_CHK_TRAP_RET(x,y)  \
	if (subwin || xrandr) { \
		if (trapped_getimage_xerror) { \
			if (subwin) { \
				static int last = 0; \
				subwin_trap_count++; \
				if (time(NULL) > last_subwin_trap + 60) { \
					rfbLog("trapped GetImage xerror" \
					    " in SUBWIN mode. [%d]\n", \
					    subwin_trap_count); \
					last_subwin_trap = time(NULL); \
					last = subwin_trap_count; \
				} \
				if (subwin_trap_count - last > 30) { \
					/* window probably iconified */ \
					usleep(1000*1000); \
				} \
			} else { \
				rfbLog("trapped GetImage xerror" \
				    " in XRANDR mode.\n"); \
			} \
			trapped_getimage_xerror = 0; \
			XSetErrorHandler(old_getimage_handler);	 \
			XFlush_wr(dpy); \
			check_xrandr_event(y); \
			X_UNLOCK; \
			return(x); \
		} \
	}

#endif /* _X11VNC_XRANDR_H */
