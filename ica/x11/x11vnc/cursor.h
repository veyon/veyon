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

#ifndef _X11VNC_CURSOR_H
#define _X11VNC_CURSOR_H

/* -- cursor.h -- */

extern int xfixes_present;
extern int xfixes_first_initialized;
extern int use_xfixes;
extern int got_xfixes_cursor_notify;
extern int cursor_changes;
extern int alpha_threshold;
extern double alpha_frac;
extern int alpha_remove;
extern int alpha_blend;
extern int alt_arrow;
extern int alt_arrow_max;


extern void first_cursor(void);
extern void setup_cursors_and_push(void);
extern void initialize_xfixes(void);
extern int known_cursors_mode(char *s);
extern void initialize_cursors_mode(void);
extern int get_which_cursor(void);
extern void restore_cursor_shape_updates(rfbScreenInfoPtr s);
extern void disable_cursor_shape_updates(rfbScreenInfoPtr s);
extern int cursor_shape_updates_clients(rfbScreenInfoPtr s);
extern int cursor_noshape_updates_clients(rfbScreenInfoPtr s);
extern int cursor_pos_updates_clients(rfbScreenInfoPtr s);
extern void cursor_position(int x, int y);
extern void set_no_cursor(void);
extern void set_warrow_cursor(void);
extern int set_cursor(int x, int y, int which);
extern int check_x11_pointer(void);
extern int store_cursor(int serial, unsigned long *data, int w, int h, int cbpp, int xhot, int yhot);
extern unsigned long get_cursor_serial(int mode);

#endif /* _X11VNC_CURSOR_H */
