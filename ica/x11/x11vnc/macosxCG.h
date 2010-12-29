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

#ifndef _X11VNC_MACOSXCG_H
#define _X11VNC_MACOSXCG_H

/* -- macosxCG.h -- */

extern void macosxCG_init(void);
extern void macosxCG_fini(void);
extern void macosxCG_event_loop(void);
extern char *macosxCG_get_fb_addr(void);

extern int macosxCG_CGDisplayPixelsWide(void);
extern int macosxCG_CGDisplayPixelsHigh(void);
extern int macosxCG_CGDisplayBitsPerPixel(void);
extern int macosxCG_CGDisplayBitsPerSample(void);
extern int macosxCG_CGDisplaySamplesPerPixel(void);
extern int macosxCG_CGDisplayBytesPerRow(void);

extern void macosxCG_pointer_inject(int mask, int x, int y);
extern int macosxCG_get_cursor_pos(int *x, int *y);
extern int macosxCG_get_cursor(void);
extern void macosxCG_init_key_table(void);
extern void macosxCG_keysym_inject(int down, unsigned int keysym);
extern void macosxCG_keycode_inject(int down, int keycode);

extern void macosxCG_refresh_callback_off(void);
extern void macosxCG_refresh_callback_on(void);



#endif /* _X11VNC_MACOSXCG_H */
