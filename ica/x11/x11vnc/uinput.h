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

#ifndef _X11VNC_UINPUT_H
#define _X11VNC_UINPUT_H

/* -- uinput.h -- */

extern int check_uinput(void);
extern int initialize_uinput(void);
extern void shutdown_uinput(void);
extern int set_uinput_accel(char *str);
extern int set_uinput_thresh(char *str);
extern void set_uinput_reset(int ms);
extern void set_uinput_always(int);
extern void set_uinput_touchscreen(int);
extern void set_uinput_abs(int);
extern char *get_uinput_accel();
extern char *get_uinput_thresh();
extern int get_uinput_reset();
extern int get_uinput_always();
extern int get_uinput_touchscreen();
extern int get_uinput_abs();
extern void parse_uinput_str(char *str);
extern void uinput_pointer_command(int mask, int x, int y, rfbClientPtr client);
extern void uinput_key_command(int down, int keysym, rfbClientPtr client);



#endif /* _X11VNC_UINPUT_H */
