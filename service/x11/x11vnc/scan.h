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

#ifndef _X11VNC_SCAN_H
#define _X11VNC_SCAN_H

/* -- scan.h -- */

extern int nap_ok;
extern int scanlines[];

extern void initialize_tiles(void);
extern void free_tiles(void);
extern void shm_delete(XShmSegmentInfo *shm);
extern void shm_clean(XShmSegmentInfo *shm, XImage *xim);
extern void initialize_polling_images(void);
extern void scale_rect(double factor_x, double factor_y, int blend, int interpolate, int Bpp,
    char *src_fb, int src_bytes_per_line, char *dst_fb, int dst_bytes_per_line,
    int Nx, int Ny, int nx, int ny, int X1, int Y1, int X2, int Y2, int mark);
extern void scale_and_mark_rect(int X1, int Y1, int X2, int Y2, int mark);
extern void mark_rect_as_modified(int x1, int y1, int x2, int y2, int force);
extern int copy_screen(void);
extern int copy_snap(void);
extern void nap_sleep(int ms, int split);
extern void set_offset(void);
extern int scan_for_updates(int count_only);
extern void rotate_curs(char *dst_0, char *src_0, int Dx, int Dy, int Bpp);
extern void rotate_coords(int x, int y, int *xo, int *yo, int dxi, int dyi);
extern void rotate_coords_inverse(int x, int y, int *xo, int *yo, int dxi, int dyi);

#endif /* _X11VNC_SCAN_H */
