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

#ifndef _X11VNC_UNIXPW_H
#define _X11VNC_UNIXPW_H

/* -- unixpw.h -- */

extern int white_pixel(void);
extern void unixpw_screen(int init);
extern void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init);
extern void unixpw_accept(char *user);
extern void unixpw_deny(void);
extern void unixpw_msg(char *msg, int delay);
extern int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size, int nodisp);
extern int unixpw_cmd_run(char *user, char *pass, char *cmd, char *line, int *n);
extern int crypt_verify(char *user, char *pass);
extern int cmd_verify(char *user, char *pass);
extern int unixpw_verify(char *user, char *pass);
extern void unixpw_verify_screen(char *user, char *pass);

extern int unixpw_in_progress;
extern int unixpw_denied;
extern int unixpw_in_rfbPE;
extern int unixpw_login_viewonly;
extern int unixpw_tightvnc_xfer_save;
extern rfbBool unixpw_file_xfer_save;
extern time_t unixpw_last_try_time;
extern rfbClientPtr unixpw_client;
extern int keep_unixpw;
extern char *keep_unixpw_user;
extern char *keep_unixpw_pass;
extern char *keep_unixpw_opts;

#endif /* _X11VNC_UNIXPW_H */
