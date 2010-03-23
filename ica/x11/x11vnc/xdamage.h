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

#ifndef _X11VNC_XDAMAGE_H
#define _X11VNC_XDAMAGE_H

/* -- xdamage.h -- */

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
extern Damage xdamage;
#endif
extern int use_xdamage;
extern int xdamage_present;
extern int xdamage_max_area;
extern double xdamage_memory;
extern int xdamage_tile_count, xdamage_direct_count;
extern double xdamage_scheduled_mark;
extern double xdamage_crazy_time;
extern double xdamage_crazy_delay;
extern sraRegionPtr xdamage_scheduled_mark_region;
extern sraRegionPtr *xdamage_regions;
extern int xdamage_ticker;
extern int XD_skip, XD_tot, XD_des;

extern void add_region_xdamage(sraRegionPtr new_region);
extern void clear_xdamage_mark_region(sraRegionPtr markregion, int flush);
extern int collect_non_X_xdamage(int x_in, int y_in, int w_in, int h_in, int call);
extern int collect_xdamage(int scancnt, int call);
extern int xdamage_hint_skip(int y);
extern void initialize_xdamage(void);
extern void create_xdamage_if_needed(int force);
extern void destroy_xdamage_if_needed(void);
extern void check_xdamage_state(void);

#endif /* _X11VNC_XDAMAGE_H */
