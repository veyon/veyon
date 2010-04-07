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

#ifndef _X11VNC_MACOSX_H
#define _X11VNC_MACOSX_H

/* -- macosx.h -- */

extern void macosx_log(char *);
extern char *macosx_console_guess(char *str, int *fd);
extern char *macosx_get_fb_addr(void);
extern void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
extern void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client);
extern void macosx_event_loop(void);
extern int macosx_get_cursor(void);
extern int macosx_get_cursor_pos(int *, int *);
extern int macosx_valid_window(Window, XWindowAttributes*);
extern Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return);
extern int macosx_get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win);
extern void macosx_send_sel(char *, int);
extern void macosx_set_sel(char *, int);

extern void macosx_add_mapnotify(Window win, int level, int map);
extern void macosx_add_create(Window win, int level);
extern void macosx_add_destroy(Window win, int level);
extern void macosx_add_visnotify(Window win, int level, int obscured);
extern int macosx_checkevent(XEvent *ev);

extern Window macosx_click_frame;


#endif /* _X11VNC_MACOSX_H */
