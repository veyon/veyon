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

#ifndef _X11VNC_XEVENTS_H
#define _X11VNC_XEVENTS_H

/* -- xevents.h -- */

extern int grab_buster;
extern int grab_kbd;
extern int grab_ptr;
extern int grab_always;
extern int ungrab_both;
extern int grab_local;
extern int sync_tod_delay;

extern void initialize_vnc_connect_prop(void);
extern void initialize_x11vnc_remote_prop(void);
extern void initialize_clipboard_atom(void);
extern void spawn_grab_buster(void);
extern void sync_tod_with_servertime(void);
extern void check_keycode_state(void);
extern void check_autorepeat(void);
extern void set_prop_atom(Atom atom);
extern void check_xevents(int reset);
extern void xcut_receive(char *text, int len, rfbClientPtr cl);

extern void kbd_release_all_keys(rfbClientPtr cl);
extern void set_single_window(rfbClientPtr cl, int x, int y);
extern void set_server_input(rfbClientPtr cl, int s);
extern void set_text_chat(rfbClientPtr cl, int l, char *t);
extern int get_keyboard_led_state_hook(rfbScreenInfoPtr s);
extern int get_file_transfer_permitted(rfbClientPtr cl);
extern void get_prop(char *str, int len, Atom prop, Window w);
extern int guess_dm_gone(int t1, int t2);


#endif /* _X11VNC_XEVENTS_H */
