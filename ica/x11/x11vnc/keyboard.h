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

#ifndef _X11VNC_KEYBOARD_H
#define _X11VNC_KEYBOARD_H

/* -- keyboard.h -- */
#include "allowed_input_t.h"

extern void get_keystate(int *keystate);
extern void clear_modifiers(int init);
extern int track_mod_state(rfbKeySym keysym, rfbBool down, rfbBool set);
extern void clear_keys(void);
extern void clear_locks(void);
extern int get_autorepeat_state(void);
extern int get_initial_autorepeat_state(void);
extern void autorepeat(int restore, int bequiet);
extern void check_add_keysyms(void);
extern int add_keysym(KeySym keysym);
extern void delete_added_keycodes(int bequiet);
extern void initialize_remap(char *infile);
extern int sloppy_key_check(int key, rfbBool down, rfbKeySym keysym, int *new_kc);
extern void switch_to_xkb_if_better(void);
extern char *short_kmbcf(char *str);
extern void initialize_allowed_input(void);
extern void initialize_modtweak(void);
extern void initialize_keyboard_and_pointer(void);
extern void get_allowed_input(rfbClientPtr client, allowed_input_t *input);
extern double typing_rate(double time_window, int *repeating);
extern int skip_cr_when_scaling(char *mode);
extern void keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client);

#endif /* _X11VNC_KEYBOARD_H */
