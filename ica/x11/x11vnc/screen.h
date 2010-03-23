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

#ifndef _X11VNC_SCREEN_H
#define _X11VNC_SCREEN_H

/* -- screen.h -- */

extern void set_greyscale_colormap(void);
extern void set_hi240_colormap(void);
extern void unset_colormap(void);
extern void set_colormap(int reset);
extern void set_nofb_params(int restore);
extern void set_raw_fb_params(int restore);
extern void do_new_fb(int reset_mem);
extern void free_old_fb(void);
extern void check_padded_fb(void);
extern void install_padded_fb(char *geom);
extern XImage *initialize_xdisplay_fb(void);
extern XImage *initialize_raw_fb(int);
extern void parse_scale_string(char *str, double *factor_x, double *factor_y, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom, int w_in, int h_in);
extern int parse_rotate_string(char *str, int *mode);
extern int scale_round(int len, double fac);
extern void initialize_screen(int *argc, char **argv, XImage *fb);
extern void set_vnc_desktop_name(void);
extern void announce(int lport, int ssl, char *iface);

extern char *vnc_reflect_guess(char *str, char **raw_fb_addr);
extern void vnc_reflect_process_client(void);
extern rfbBool vnc_reflect_send_pointer(int x, int y, int mask);
extern rfbBool vnc_reflect_send_key(uint32_t key, rfbBool down);
extern rfbBool vnc_reflect_send_cuttext(char *str, int len);

extern int rawfb_reset;
extern int rawfb_dev_video;
extern int rawfb_vnc_reflect;

#endif /* _X11VNC_SCREEN_H */
