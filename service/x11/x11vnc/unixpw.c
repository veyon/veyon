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

/* -- unixpw.c -- */

#ifdef __linux__
/* some conflict with _XOPEN_SOURCE */
extern int grantpt(int);
extern int unlockpt(int);
extern char *ptsname(int);
#endif

#ifndef DO_NOT_DECLARE_CRYPT
extern char *crypt(const char*, const char *);
#endif

#include "x11vnc.h"
#include "scan.h"
#include "cleanup.h"
#include "xinerama.h"
#include "connections.h"
#include "user.h"
#include "connections.h"
#include "sslhelper.h"
#include "cursor.h"
#include "rates.h"
#include <rfb/default8x16.h>

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID
#define UNIXPW_SU
#endif
#endif

#ifdef IGNORE_GETSPNAM
#undef LIBVNCSERVER_HAVE_GETSPNAM
#define LIBVNCSERVER_HAVE_GETSPNAM 0
#endif

#if LIBVNCSERVER_HAVE_PWD_H && LIBVNCSERVER_HAVE_GETPWNAM
#if LIBVNCSERVER_HAVE_CRYPT || LIBVNCSERVER_HAVE_LIBCRYPT
#define UNIXPW_CRYPT
#if LIBVNCSERVER_HAVE_GETSPNAM
#include <shadow.h>
#endif
#endif
#endif

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if LIBVNCSERVER_HAVE_TERMIOS_H
#include <termios.h>
#endif
#if LIBVNCSERVER_HAVE_SYS_STROPTS_H
#include <sys/stropts.h>
#endif

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#define IS_BSD
#endif
#if (defined(__MACH__) && defined(__APPLE__))
#define IS_BSD
#endif

int white_pixel(void);
void unixpw_screen(int init);
void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init);
void unixpw_accept(char *user);
void unixpw_deny(void);
void unixpw_msg(char *msg, int delay);
int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size, int nodisp);
int unixpw_cmd_run(char *user, char *pass, char *cmd, char *line, int *n);
int crypt_verify(char *user, char *pass);
int cmd_verify(char *user, char *pass);
void unixpw_verify_screen(char *user, char *pass);

static int text_x(void);
static int text_y(void);
static void set_db(void);

int unixpw_in_progress = 0;
int unixpw_denied = 0;
int unixpw_in_rfbPE = 0;
int unixpw_login_viewonly = 0;
int unixpw_tightvnc_xfer_save = 0;
rfbBool unixpw_file_xfer_save = FALSE;
time_t unixpw_last_try_time = 0;
rfbClientPtr unixpw_client = NULL;

int keep_unixpw = 0;
char *keep_unixpw_user = NULL;
char *keep_unixpw_pass = NULL;
char *keep_unixpw_opts = NULL;

static unsigned char default6x13FontData[2899]={
0x00,0x00,0xA8,0x00,0x88,0x00,0x88,0x00,0x88,0x00,0xA8,0x00,0x00, /* 0 */
0x00,0x00,0x00,0x00,0x20,0x70,0xF8,0x70,0x20,0x00,0x00,0x00,0x00, /* 1 */
0xA8,0x54,0xA8,0x54,0xA8,0x54,0xA8,0x54,0xA8,0x54,0xA8,0x54,0xA8, /* 2 */
0x00,0x00,0xA0,0xA0,0xE0,0xA0,0xA0,0x38,0x10,0x10,0x10,0x00,0x00, /* 3 */
0x00,0x00,0xE0,0x80,0xC0,0x80,0xB8,0x20,0x30,0x20,0x20,0x00,0x00, /* 4 */
0x00,0x00,0x60,0x80,0x80,0x60,0x30,0x28,0x30,0x28,0x28,0x00,0x00, /* 5 */
0x00,0x00,0x80,0x80,0x80,0xE0,0x38,0x20,0x30,0x20,0x20,0x00,0x00, /* 6 */
0x00,0x00,0x30,0x48,0x48,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 7 */
0x00,0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00,0xF8,0x00,0x00,0x00, /* 8 */
0x00,0x00,0x90,0xD0,0xB0,0x90,0x20,0x20,0x20,0x20,0x38,0x00,0x00, /* 9 */
0x00,0x00,0xA0,0xA0,0xA0,0x40,0x40,0x38,0x10,0x10,0x10,0x00,0x00, /* 10 */
0x20,0x20,0x20,0x20,0x20,0x20,0xE0,0x00,0x00,0x00,0x00,0x00,0x00, /* 11 */
0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x20,0x20,0x20,0x20,0x20,0x20, /* 12 */
0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x20,0x20,0x20,0x20,0x20,0x20, /* 13 */
0x20,0x20,0x20,0x20,0x20,0x20,0x3C,0x00,0x00,0x00,0x00,0x00,0x00, /* 14 */
0x20,0x20,0x20,0x20,0x20,0x20,0xFC,0x20,0x20,0x20,0x20,0x20,0x20, /* 15 */
0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 16 */
0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 17 */
0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0x00, /* 18 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00, /* 19 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC, /* 20 */
0x20,0x20,0x20,0x20,0x20,0x20,0x3C,0x20,0x20,0x20,0x20,0x20,0x20, /* 21 */
0x20,0x20,0x20,0x20,0x20,0x20,0xE0,0x20,0x20,0x20,0x20,0x20,0x20, /* 22 */
0x20,0x20,0x20,0x20,0x20,0x20,0xFC,0x00,0x00,0x00,0x00,0x00,0x00, /* 23 */
0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x20,0x20,0x20,0x20,0x20,0x20, /* 24 */
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20, /* 25 */
0x00,0x00,0x00,0x18,0x60,0x80,0x60,0x18,0x00,0xF8,0x00,0x00,0x00, /* 26 */
0x00,0x00,0x00,0xC0,0x30,0x08,0x30,0xC0,0x00,0xF8,0x00,0x00,0x00, /* 27 */
0x00,0x00,0x00,0x00,0x00,0xF8,0x50,0x50,0x50,0x50,0x50,0x00,0x00, /* 28 */
0x00,0x00,0x00,0x00,0x00,0x08,0xF8,0x20,0xF8,0x80,0x00,0x00,0x00, /* 29 */
0x00,0x00,0x30,0x48,0x40,0x40,0xE0,0x40,0x40,0x48,0xB0,0x00,0x00, /* 30 */
0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00, /* 31 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 32 */
0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,0x00, /* 33 */
0x00,0x00,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 34 */
0x00,0x00,0x00,0x50,0x50,0xF8,0x50,0xF8,0x50,0x50,0x00,0x00,0x00, /* 35 */
0x00,0x00,0x20,0x78,0xA0,0xA0,0x70,0x28,0x28,0xF0,0x20,0x00,0x00, /* 36 */
0x00,0x00,0x48,0xA8,0x50,0x10,0x20,0x40,0x50,0xA8,0x90,0x00,0x00, /* 37 */
0x00,0x00,0x00,0x40,0xA0,0xA0,0x40,0xA0,0x98,0x90,0x68,0x00,0x00, /* 38 */
0x00,0x00,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 39 */
0x00,0x10,0x20,0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x20,0x10,0x00, /* 40 */
0x00,0x40,0x20,0x20,0x10,0x10,0x10,0x10,0x10,0x20,0x20,0x40,0x00, /* 41 */
0x00,0x00,0x00,0x20,0xA8,0xF8,0x70,0xF8,0xA8,0x20,0x00,0x00,0x00, /* 42 */
0x00,0x00,0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00,0x00,0x00,0x00, /* 43 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x20,0x40,0x00, /* 44 */
0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00, /* 45 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x70,0x20,0x00, /* 46 */
0x00,0x00,0x08,0x08,0x10,0x10,0x20,0x40,0x40,0x80,0x80,0x00,0x00, /* 47 */
0x00,0x00,0x20,0x50,0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x00, /* 48 */
0x00,0x00,0x20,0x60,0xA0,0x20,0x20,0x20,0x20,0x20,0xF8,0x00,0x00, /* 49 */
0x00,0x00,0x70,0x88,0x88,0x08,0x10,0x20,0x40,0x80,0xF8,0x00,0x00, /* 50 */
0x00,0x00,0xF8,0x08,0x10,0x20,0x70,0x08,0x08,0x88,0x70,0x00,0x00, /* 51 */
0x00,0x00,0x10,0x10,0x30,0x50,0x50,0x90,0xF8,0x10,0x10,0x00,0x00, /* 52 */
0x00,0x00,0xF8,0x80,0x80,0xB0,0xC8,0x08,0x08,0x88,0x70,0x00,0x00, /* 53 */
0x00,0x00,0x70,0x88,0x80,0x80,0xF0,0x88,0x88,0x88,0x70,0x00,0x00, /* 54 */
0x00,0x00,0xF8,0x08,0x10,0x10,0x20,0x20,0x40,0x40,0x40,0x00,0x00, /* 55 */
0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x88,0x88,0x88,0x70,0x00,0x00, /* 56 */
0x00,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x08,0x88,0x70,0x00,0x00, /* 57 */
0x00,0x00,0x00,0x00,0x20,0x70,0x20,0x00,0x00,0x20,0x70,0x20,0x00, /* 58 */
0x00,0x00,0x00,0x00,0x20,0x70,0x20,0x00,0x00,0x30,0x20,0x40,0x00, /* 59 */
0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x08,0x00,0x00, /* 60 */
0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0x00,0x00, /* 61 */
0x00,0x00,0x80,0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x80,0x00,0x00, /* 62 */
0x00,0x00,0x70,0x88,0x88,0x08,0x10,0x20,0x20,0x00,0x20,0x00,0x00, /* 63 */
0x00,0x00,0x70,0x88,0x88,0x98,0xA8,0xA8,0xB0,0x80,0x78,0x00,0x00, /* 64 */
0x00,0x00,0x20,0x50,0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x00,0x00, /* 65 */
0x00,0x00,0xF0,0x48,0x48,0x48,0x70,0x48,0x48,0x48,0xF0,0x00,0x00, /* 66 */
0x00,0x00,0x70,0x88,0x80,0x80,0x80,0x80,0x80,0x88,0x70,0x00,0x00, /* 67 */
0x00,0x00,0xF0,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0xF0,0x00,0x00, /* 68 */
0x00,0x00,0xF8,0x80,0x80,0x80,0xF0,0x80,0x80,0x80,0xF8,0x00,0x00, /* 69 */
0x00,0x00,0xF8,0x80,0x80,0x80,0xF0,0x80,0x80,0x80,0x80,0x00,0x00, /* 70 */
0x00,0x00,0x70,0x88,0x80,0x80,0x80,0x98,0x88,0x88,0x70,0x00,0x00, /* 71 */
0x00,0x00,0x88,0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x88,0x00,0x00, /* 72 */
0x00,0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 73 */
0x00,0x00,0x38,0x10,0x10,0x10,0x10,0x10,0x10,0x90,0x60,0x00,0x00, /* 74 */
0x00,0x00,0x88,0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88,0x88,0x00,0x00, /* 75 */
0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0x00,0x00, /* 76 */
0x00,0x00,0x88,0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88,0x88,0x00,0x00, /* 77 */
0x00,0x00,0x88,0xC8,0xC8,0xA8,0xA8,0x98,0x98,0x88,0x88,0x00,0x00, /* 78 */
0x00,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 79 */
0x00,0x00,0xF0,0x88,0x88,0x88,0xF0,0x80,0x80,0x80,0x80,0x00,0x00, /* 80 */
0x00,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0xA8,0x70,0x08,0x00, /* 81 */
0x00,0x00,0xF0,0x88,0x88,0x88,0xF0,0xA0,0x90,0x88,0x88,0x00,0x00, /* 82 */
0x00,0x00,0x70,0x88,0x80,0x80,0x70,0x08,0x08,0x88,0x70,0x00,0x00, /* 83 */
0x00,0x00,0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00, /* 84 */
0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 85 */
0x00,0x00,0x88,0x88,0x88,0x88,0x50,0x50,0x50,0x20,0x20,0x00,0x00, /* 86 */
0x00,0x00,0x88,0x88,0x88,0x88,0xA8,0xA8,0xA8,0xA8,0x50,0x00,0x00, /* 87 */
0x00,0x00,0x88,0x88,0x50,0x50,0x20,0x50,0x50,0x88,0x88,0x00,0x00, /* 88 */
0x00,0x00,0x88,0x88,0x50,0x50,0x20,0x20,0x20,0x20,0x20,0x00,0x00, /* 89 */
0x00,0x00,0xF8,0x08,0x10,0x10,0x20,0x40,0x40,0x80,0xF8,0x00,0x00, /* 90 */
0x00,0x70,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x70,0x00, /* 91 */
0x00,0x00,0x80,0x80,0x40,0x40,0x20,0x10,0x10,0x08,0x08,0x00,0x00, /* 92 */
0x00,0x70,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x70,0x00, /* 93 */
0x00,0x00,0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 94 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00, /* 95 */
0x00,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 96 */
0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 97 */
0x00,0x00,0x80,0x80,0x80,0xF0,0x88,0x88,0x88,0x88,0xF0,0x00,0x00, /* 98 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x00,0x00, /* 99 */
0x00,0x00,0x08,0x08,0x08,0x78,0x88,0x88,0x88,0x88,0x78,0x00,0x00, /* 100 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,0x00,0x00, /* 101 */
0x00,0x00,0x30,0x48,0x40,0x40,0xF0,0x40,0x40,0x40,0x40,0x00,0x00, /* 102 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x88,0x70, /* 103 */
0x00,0x00,0x80,0x80,0x80,0xB0,0xC8,0x88,0x88,0x88,0x88,0x00,0x00, /* 104 */
0x00,0x00,0x00,0x20,0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 105 */
0x00,0x00,0x00,0x10,0x00,0x30,0x10,0x10,0x10,0x10,0x90,0x90,0x60, /* 106 */
0x00,0x00,0x80,0x80,0x80,0x90,0xA0,0xC0,0xA0,0x90,0x88,0x00,0x00, /* 107 */
0x00,0x00,0x60,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 108 */
0x00,0x00,0x00,0x00,0x00,0xD0,0xA8,0xA8,0xA8,0xA8,0x88,0x00,0x00, /* 109 */
0x00,0x00,0x00,0x00,0x00,0xB0,0xC8,0x88,0x88,0x88,0x88,0x00,0x00, /* 110 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 111 */
0x00,0x00,0x00,0x00,0x00,0xF0,0x88,0x88,0x88,0xF0,0x80,0x80,0x80, /* 112 */
0x00,0x00,0x00,0x00,0x00,0x78,0x88,0x88,0x88,0x78,0x08,0x08,0x08, /* 113 */
0x00,0x00,0x00,0x00,0x00,0xB0,0xC8,0x80,0x80,0x80,0x80,0x00,0x00, /* 114 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0x60,0x10,0x88,0x70,0x00,0x00, /* 115 */
0x00,0x00,0x00,0x40,0x40,0xF0,0x40,0x40,0x40,0x48,0x30,0x00,0x00, /* 116 */
0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x98,0x68,0x00,0x00, /* 117 */
0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0x50,0x50,0x20,0x00,0x00, /* 118 */
0x00,0x00,0x00,0x00,0x00,0x88,0x88,0xA8,0xA8,0xA8,0x50,0x00,0x00, /* 119 */
0x00,0x00,0x00,0x00,0x00,0x88,0x50,0x20,0x20,0x50,0x88,0x00,0x00, /* 120 */
0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x08,0x88,0x70, /* 121 */
0x00,0x00,0x00,0x00,0x00,0xF8,0x10,0x20,0x40,0x80,0xF8,0x00,0x00, /* 122 */
0x00,0x18,0x20,0x20,0x20,0x20,0xC0,0x20,0x20,0x20,0x20,0x18,0x00, /* 123 */
0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00, /* 124 */
0x00,0xC0,0x20,0x20,0x20,0x20,0x18,0x20,0x20,0x20,0x20,0xC0,0x00, /* 125 */
0x00,0x00,0x48,0xA8,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 126 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160 */
0x00,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00, /* 161 */
0x00,0x00,0x20,0x70,0xA8,0xA0,0xA0,0xA8,0x70,0x20,0x00,0x00,0x00, /* 162 */
0x00,0x00,0x30,0x48,0x40,0x40,0xE0,0x40,0x40,0x48,0xB0,0x00,0x00, /* 163 */
0x00,0x00,0x00,0x00,0x88,0x70,0x50,0x50,0x70,0x88,0x00,0x00,0x00, /* 164 */
0x00,0x00,0x88,0x88,0x50,0x50,0xF8,0x20,0xF8,0x20,0x20,0x00,0x00, /* 165 */
0x00,0x00,0x20,0x20,0x20,0x20,0x00,0x20,0x20,0x20,0x20,0x00,0x00, /* 166 */
0x00,0x30,0x48,0x40,0x30,0x48,0x48,0x30,0x08,0x48,0x30,0x00,0x00, /* 167 */
0x00,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 168 */
0x00,0x70,0x88,0xA8,0xD8,0xC8,0xD8,0xA8,0x88,0x70,0x00,0x00,0x00, /* 169 */
0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0xF8,0x00,0x00,0x00,0x00, /* 170 */
0x00,0x00,0x00,0x00,0x28,0x50,0xA0,0xA0,0x50,0x28,0x00,0x00,0x00, /* 171 */
0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x08,0x08,0x00,0x00,0x00,0x00, /* 172 */
0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00, /* 173 */
0x00,0x70,0x88,0xE8,0xD8,0xD8,0xE8,0xD8,0x88,0x70,0x00,0x00,0x00, /* 174 */
0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 175 */
0x00,0x00,0x30,0x48,0x48,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 176 */
0x00,0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00,0xF8,0x00,0x00,0x00, /* 177 */
0x00,0x40,0xA0,0x20,0x40,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 178 */
0x00,0x40,0xA0,0x40,0x20,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 179 */
0x00,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 180 */
0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x98,0xE8,0x80,0x80, /* 181 */
0x00,0x00,0x78,0xE8,0xE8,0xE8,0xE8,0x68,0x28,0x28,0x28,0x00,0x00, /* 182 */
0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00, /* 183 */
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20, /* 184 */
0x00,0x40,0xC0,0x40,0x40,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 185 */
0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0xF8,0x00,0x00,0x00,0x00, /* 186 */
0x00,0x00,0x00,0x00,0xA0,0x50,0x28,0x28,0x50,0xA0,0x00,0x00,0x00, /* 187 */
0x00,0x40,0xC0,0x40,0x40,0xE0,0x08,0x18,0x28,0x38,0x08,0x00,0x00, /* 188 */
0x00,0x40,0xC0,0x40,0x40,0xE0,0x10,0x28,0x08,0x10,0x38,0x00,0x00, /* 189 */
0x00,0x40,0xA0,0x40,0x20,0xA0,0x48,0x18,0x28,0x38,0x08,0x00,0x00, /* 190 */
0x00,0x00,0x20,0x00,0x20,0x20,0x40,0x80,0x88,0x88,0x70,0x00,0x00, /* 191 */
0x00,0x40,0x20,0x00,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 192 */
0x00,0x10,0x20,0x00,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 193 */
0x00,0x30,0x48,0x00,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 194 */
0x00,0x28,0x50,0x00,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 195 */
0x00,0x50,0x50,0x00,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 196 */
0x00,0x20,0x50,0x20,0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00,0x00, /* 197 */
0x00,0x00,0x58,0xA0,0xA0,0xA0,0xB0,0xE0,0xA0,0xA0,0xB8,0x00,0x00, /* 198 */
0x00,0x00,0x70,0x88,0x80,0x80,0x80,0x80,0x80,0x88,0x70,0x20,0x40, /* 199 */
0x00,0x40,0x20,0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00,0x00, /* 200 */
0x00,0x10,0x20,0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00,0x00, /* 201 */
0x00,0x30,0x48,0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00,0x00, /* 202 */
0x00,0x50,0x50,0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00,0x00, /* 203 */
0x00,0x40,0x20,0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 204 */
0x00,0x10,0x20,0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 205 */
0x00,0x30,0x48,0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 206 */
0x00,0x50,0x50,0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 207 */
0x00,0x00,0xF0,0x48,0x48,0x48,0xE8,0x48,0x48,0x48,0xF0,0x00,0x00, /* 208 */
0x00,0x28,0x50,0x00,0x88,0x88,0xC8,0xA8,0x98,0x88,0x88,0x00,0x00, /* 209 */
0x00,0x40,0x20,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 210 */
0x00,0x10,0x20,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 211 */
0x00,0x30,0x48,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 212 */
0x00,0x28,0x50,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 213 */
0x00,0x50,0x50,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 214 */
0x00,0x00,0x00,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00, /* 215 */
0x00,0x08,0x70,0x98,0x98,0xA8,0xA8,0xA8,0xC8,0xC8,0x70,0x80,0x00, /* 216 */
0x00,0x40,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 217 */
0x00,0x10,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 218 */
0x00,0x30,0x48,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 219 */
0x00,0x50,0x50,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 220 */
0x00,0x10,0x20,0x00,0x88,0x88,0x50,0x20,0x20,0x20,0x20,0x00,0x00, /* 221 */
0x00,0x00,0x80,0xF0,0x88,0x88,0x88,0xF0,0x80,0x80,0x80,0x00,0x00, /* 222 */
0x00,0x00,0x60,0x90,0x90,0xA0,0xA0,0x90,0x88,0x88,0xB0,0x00,0x00, /* 223 */
0x00,0x00,0x40,0x20,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 224 */
0x00,0x00,0x10,0x20,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 225 */
0x00,0x00,0x30,0x48,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 226 */
0x00,0x00,0x28,0x50,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 227 */
0x00,0x00,0x50,0x50,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 228 */
0x00,0x30,0x48,0x30,0x00,0x70,0x08,0x78,0x88,0x98,0x68,0x00,0x00, /* 229 */
0x00,0x00,0x00,0x00,0x00,0x70,0x28,0x70,0xA0,0xA8,0x50,0x00,0x00, /* 230 */
0x00,0x00,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x20,0x40, /* 231 */
0x00,0x00,0x40,0x20,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,0x00,0x00, /* 232 */
0x00,0x00,0x10,0x20,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,0x00,0x00, /* 233 */
0x00,0x00,0x30,0x48,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,0x00,0x00, /* 234 */
0x00,0x00,0x50,0x50,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,0x00,0x00, /* 235 */
0x00,0x00,0x40,0x20,0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 236 */
0x00,0x00,0x10,0x20,0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 237 */
0x00,0x00,0x30,0x48,0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 238 */
0x00,0x00,0x50,0x50,0x00,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00, /* 239 */
0x00,0x50,0x20,0x60,0x10,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 240 */
0x00,0x00,0x28,0x50,0x00,0xB0,0xC8,0x88,0x88,0x88,0x88,0x00,0x00, /* 241 */
0x00,0x00,0x40,0x20,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 242 */
0x00,0x00,0x10,0x20,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 243 */
0x00,0x00,0x30,0x48,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 244 */
0x00,0x00,0x28,0x50,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 245 */
0x00,0x00,0x50,0x50,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00, /* 246 */
0x00,0x00,0x00,0x20,0x20,0x00,0xF8,0x00,0x20,0x20,0x00,0x00,0x00, /* 247 */
0x00,0x00,0x00,0x00,0x08,0x70,0x98,0xA8,0xA8,0xC8,0x70,0x80,0x00, /* 248 */
0x00,0x00,0x40,0x20,0x00,0x88,0x88,0x88,0x88,0x98,0x68,0x00,0x00, /* 249 */
0x00,0x00,0x10,0x20,0x00,0x88,0x88,0x88,0x88,0x98,0x68,0x00,0x00, /* 250 */
0x00,0x00,0x30,0x48,0x00,0x88,0x88,0x88,0x88,0x98,0x68,0x00,0x00, /* 251 */
0x00,0x00,0x50,0x50,0x00,0x88,0x88,0x88,0x88,0x98,0x68,0x00,0x00, /* 252 */
0x00,0x00,0x10,0x20,0x00,0x88,0x88,0x88,0x98,0x68,0x08,0x88,0x70, /* 253 */
0x00,0x00,0x00,0x80,0x80,0xB0,0xC8,0x88,0x88,0xC8,0xB0,0x80,0x80, /* 254 */
0x00,0x00,0x50,0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x08,0x88,0x70, /* 255 */
};
static int default6x13FontMetaData[256*5]={
0,6,13,0,-2,13,6,13,0,-2,26,6,13,0,-2,39,6,13,0,-2,52,6,13,0,-2,65,6,13,0,-2,78,6,13,0,-2,91,6,13,0,-2,104,6,13,0,-2,117,6,13,0,-2,130,6,13,0,-2,143,6,13,0,-2,156,6,13,0,-2,169,6,13,0,-2,182,6,13,0,-2,195,6,13,0,-2,208,6,13,0,-2,221,6,13,0,-2,234,6,13,0,-2,247,6,13,0,-2,260,6,13,0,-2,273,6,13,0,-2,286,6,13,0,-2,299,6,13,0,-2,312,6,13,0,-2,325,6,13,0,-2,338,6,13,0,-2,351,6,13,0,-2,364,6,13,0,-2,377,6,13,0,-2,390,6,13,0,-2,403,6,13,0,-2,416,6,13,0,-2,429,6,13,0,-2,442,6,13,0,-2,455,6,13,0,-2,468,6,13,0,-2,481,6,13,0,-2,494,6,13,0,-2,507,6,13,0,-2,520,6,13,0,-2,533,6,13,0,-2,546,6,13,0,-2,559,6,13,0,-2,572,6,13,0,-2,585,6,13,0,-2,598,6,13,0,-2,611,6,13,0,-2,624,6,13,0,-2,637,6,13,0,-2,650,6,13,0,-2,663,6,13,0,-2,676,6,13,0,-2,689,6,13,0,-2,702,6,13,0,-2,715,6,13,0,-2,728,6,13,0,-2,741,6,13,0,-2,754,6,13,0,-2,767,6,13,0,-2,780,6,13,0,-2,793,6,13,0,-2,806,6,13,0,-2,819,6,13,0,-2,832,6,13,0,-2,845,6,13,0,-2,858,6,13,0,-2,871,6,13,0,-2,884,6,13,0,-2,897,6,13,0,-2,910,6,13,0,-2,923,6,13,0,-2,936,6,13,0,-2,949,6,13,0,-2,962,6,13,0,-2,975,6,13,0,-2,988,6,13,0,-2,1001,6,13,0,-2,1014,6,13,0,-2,1027,6,13,0,-2,1040,6,13,0,-2,1053,6,13,0,-2,1066,6,13,0,-2,1079,6,13,0,-2,1092,6,13,0,-2,1105,6,13,0,-2,1118,6,13,0,-2,1131,6,13,0,-2,1144,6,13,0,-2,1157,6,13,0,-2,1170,6,13,0,-2,1183,6,13,0,-2,1196,6,13,0,-2,1209,6,13,0,-2,1222,6,13,0,-2,1235,6,13,0,-2,1248,6,13,0,-2,1261,6,13,0,-2,1274,6,13,0,-2,1287,6,13,0,-2,1300,6,13,0,-2,1313,6,13,0,-2,1326,6,13,0,-2,1339,6,13,0,-2,1352,6,13,0,-2,1365,6,13,0,-2,1378,6,13,0,-2,1391,6,13,0,-2,1404,6,13,0,-2,1417,6,13,0,-2,1430,6,13,0,-2,1443,6,13,0,-2,1456,6,13,0,-2,1469,6,13,0,-2,1482,6,13,0,-2,1495,6,13,0,-2,1508,6,13,0,-2,1521,6,13,0,-2,1534,6,13,0,-2,1547,6,13,0,-2,1560,6,13,0,-2,1573,6,13,0,-2,1586,6,13,0,-2,1599,6,13,0,-2,1612,6,13,0,-2,1625,6,13,0,-2,1638,6,13,0,-2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1651,6,13,0,-2,1664,6,13,0,-2,1677,6,13,0,-2,1690,6,13,0,-2,1703,6,13,0,-2,1716,6,13,0,-2,1729,6,13,0,-2,1742,6,13,0,-2,1755,6,13,0,-2,1768,6,13,0,-2,1781,6,13,0,-2,1794,6,13,0,-2,1807,6,13,0,-2,1820,6,13,0,-2,1833,6,13,0,-2,1846,6,13,0,-2,1859,6,13,0,-2,1872,6,13,0,-2,1885,6,13,0,-2,1898,6,13,0,-2,1911,6,13,0,-2,1924,6,13,0,-2,1937,6,13,0,-2,1950,6,13,0,-2,1963,6,13,0,-2,1976,6,13,0,-2,1989,6,13,0,-2,2002,6,13,0,-2,2015,6,13,0,-2,2028,6,13,0,-2,2041,6,13,0,-2,2054,6,13,0,-2,2067,6,13,0,-2,2080,6,13,0,-2,2093,6,13,0,-2,2106,6,13,0,-2,2119,6,13,0,-2,2132,6,13,0,-2,2145,6,13,0,-2,2158,6,13,0,-2,2171,6,13,0,-2,2184,6,13,0,-2,2197,6,13,0,-2,2210,6,13,0,-2,2223,6,13,0,-2,2236,6,13,0,-2,2249,6,13,0,-2,2262,6,13,0,-2,2275,6,13,0,-2,2288,6,13,0,-2,2301,6,13,0,-2,2314,6,13,0,-2,2327,6,13,0,-2,2340,6,13,0,-2,2353,6,13,0,-2,2366,6,13,0,-2,2379,6,13,0,-2,2392,6,13,0,-2,2405,6,13,0,-2,2418,6,13,0,-2,2431,6,13,0,-2,2444,6,13,0,-2,2457,6,13,0,-2,2470,6,13,0,-2,2483,6,13,0,-2,2496,6,13,0,-2,2509,6,13,0,-2,2522,6,13,0,-2,2535,6,13,0,-2,2548,6,13,0,-2,2561,6,13,0,-2,2574,6,13,0,-2,2587,6,13,0,-2,2600,6,13,0,-2,2613,6,13,0,-2,2626,6,13,0,-2,2639,6,13,0,-2,2652,6,13,0,-2,2665,6,13,0,-2,2678,6,13,0,-2,2691,6,13,0,-2,2704,6,13,0,-2,2717,6,13,0,-2,2730,6,13,0,-2,2743,6,13,0,-2,2756,6,13,0,-2,2769,6,13,0,-2,2782,6,13,0,-2,2795,6,13,0,-2,2808,6,13,0,-2,2821,6,13,0,-2,2834,6,13,0,-2,2847,6,13,0,-2,2860,6,13,0,-2,2873,6,13,0,-2,2886,6,13,0,-2,};
static rfbFontData default6x13Font={default6x13FontData, default6x13FontMetaData};

static int in_login = 0, in_passwd = 0, tries = 0;
static int char_row = 0, char_col = 0;
static int char_x = 0, char_y = 0, char_w = 8, char_h = 16;

static int db = 0;

int white_pixel(void) {
	static unsigned long black_pix = 0, white_pix = 1, set = 0;

	RAWFB_RET(0xffffff)

	if (depth <= 8 && ! set) {
		X_LOCK;
		black_pix = BlackPixel(dpy, scr);
		white_pix = WhitePixel(dpy, scr);
		X_UNLOCK;
		set = 1;
	}
	if (depth <= 8) {
		return (int) white_pix;
	} else if (depth < 24) {
		return 0xffff;
	} else {
		return 0xffffff;
	}
}

int black_pixel(void) {
	static unsigned long black_pix = 0, white_pix = 1, set = 0;

	RAWFB_RET(0x000000)

	if (depth <= 8 && ! set) {
		X_LOCK;
		black_pix = BlackPixel(dpy, scr);
		white_pix = WhitePixel(dpy, scr);
		X_UNLOCK;
		set = 1;
	}
	if (depth <= 8) {
		return (int) black_pix;
	} else if (depth < 24) {
		return 0x0000;
	} else {
		return 0x000000;
	}
}

static void unixpw_mark(void) {
	if (scaling) {
		mark_rect_as_modified(0, 0, scaled_x, scaled_y, 1);
	} else {
		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	}
}

static int text_x(void) {
	return char_x + char_col * char_w;
}

static int text_y(void) {
	return char_y + char_row * char_h;
}

static rfbScreenInfo fscreen;
static rfbScreenInfoPtr pscreen;

static int f1_help = 0;

void unixpw_screen(int init) {
	if (unixpw_cmd) {
		;	/* OK */
	} else if (unixpw_nis) {
#ifndef UNIXPW_CRYPT
	rfbLog("-unixpw_nis is not supported on this OS/machine\n");
	clean_up_exit(1);
#endif
	} else {
#ifndef UNIXPW_SU
	rfbLog("-unixpw is not supported on this OS/machine\n");
	clean_up_exit(1);
#endif
	}
	if (init) {
		int x, y;
		char log[] = "login: ";

		zero_fb(0, 0, dpy_x, dpy_y);

		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);

		x = nfix(dpy_x / 2 -  strlen(log) * char_w, dpy_x);
		y = (int) (dpy_y / 3.5);
		if (unixpw_system_greeter) {
			y = (int) (dpy_y / 3);
		}

		if (scaling) {
			x = (int) (x * scale_fac_x);
			y = (int) (y * scale_fac_y);
			x = nfix(x, scaled_x);
			y = nfix(y, scaled_y);
		}

		if (rotating) {
			fscreen.serverFormat.bitsPerPixel = bpp;
			fscreen.paddedWidthInBytes = rfb_bytes_per_line;
			fscreen.frameBuffer = rfb_fb;
			pscreen = &fscreen;
		} else {
			pscreen = screen;
		}

		if (pscreen && pscreen->width >= 640 && pscreen->height >= 480) {
			rfbDrawString(pscreen, &default6x13Font, 8, 2+1*13, "F1-Help:", white_pixel());
		}
		f1_help = 0;

		if (unixpw_system_greeter) {
			unixpw_system_greeter_active = 0;
			if (use_dpy && strstr(use_dpy, "xdmcp")) {
				if (getenv("X11VNC_SYSTEM_GREETER1")) {
					char moo[] = "Press 'Escape' for System Greeter";
					rfbDrawString(pscreen, &default8x16Font, x-90, y-30, moo, white_pixel());
				} else {
					char moo1[] = "Press 'Escape' for a New Session via System Greeter, or";
					char moo2[] = "otherwise login here to connect to an Existing Session:";
					rfbDrawString(pscreen, &default6x13Font, x-110, y-38, moo1, white_pixel());
					rfbDrawString(pscreen, &default6x13Font, x-110, y-25, moo2, white_pixel());
				}
				set_env("X11VNC_XDM_ONLY", "0");
				unixpw_system_greeter_active = 1;
			}
		}

		rfbDrawString(pscreen, &default8x16Font, x, y, log, white_pixel());

		char_x = x;
		char_y = y;
		char_col = strlen(log);
		char_row = 0;

		set_warrow_cursor();
	}

	unixpw_mark();
}

	
#ifdef MAXPATHLEN
static char slave_str[MAXPATHLEN];
#else
static char slave_str[4096];
#endif

static int used_get_pty_ptmx = 0;

char *get_pty_ptmx(int *fd_p) {
	char *slave;
	int fd = -1, i, ndevs = 4, tmp;
	char *devs[] = { 
		"/dev/ptmx",
		"/dev/ptm/clone",
		"/dev/ptc",
		"/dev/ptmx_bsd"
	};

	*fd_p = -1;

#if LIBVNCSERVER_HAVE_GRANTPT

	for (i=0; i < ndevs; i++) {
#ifdef O_NOCTTY
		fd = open(devs[i], O_RDWR|O_NOCTTY);
#else
		fd = open(devs[i], O_RDWR);
#endif
		if (fd >= 0) {
			break;
		}
	}

	if (fd < 0) {
		rfbLogPerror("open /dev/ptmx");
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCPKT)
	tmp = 0;
	ioctl(fd, TIOCPKT, (char *) &tmp);
#endif

	if (grantpt(fd) != 0) {
		rfbLogPerror("grantpt");
		close(fd);
		return NULL;
	}
	if (unlockpt(fd) != 0) {
		rfbLogPerror("unlockpt");
		close(fd);
		return NULL;
	}

	slave = ptsname(fd);
	if (! slave)  {
		rfbLogPerror("ptsname");
		close(fd);
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCFLUSH)
	ioctl(fd, TIOCFLUSH, (char *) 0);
#endif

	if (strlen(slave) > sizeof(slave_str)/2) {
		rfbLog("get_pty_ptmx: slave string length too long.\n");	
		close(fd);
		return NULL;
	}

	strcpy(slave_str, slave);
	*fd_p = fd;
	return slave_str;

#else
	return NULL;

#endif /* GRANTPT */
}


char *get_pty_loop(int *fd_p) {
	char master_str[16];
	int fd = -1, i;
	char c;

	*fd_p = -1;

	/* for *BSD loop over /dev/ptyXY */

	for (c = 'p'; c <= 'z'; c++) {
		for (i=0; i < 16; i++) {
			sprintf(master_str, "/dev/pty%c%x", c, i);
#ifdef O_NOCTTY
			fd = open(master_str, O_RDWR|O_NOCTTY);
#else
			fd = open(master_str, O_RDWR);
#endif
			if (fd >= 0) {
				break;
			}
		}
		if (fd >= 0) {
			break;
		}
	}
	if (fd < 0) {
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCFLUSH)
	ioctl(fd, TIOCFLUSH, (char *) 0);
#endif

	sprintf(slave_str, "/dev/tty%c%x", c, i);
	*fd_p = fd;
	return slave_str;
}

char *get_pty(int *fd_p) {
	used_get_pty_ptmx = 0;
	if (getenv("BSD_PTY")) {
		return get_pty_loop(fd_p);
	}
#ifdef IS_BSD
	return get_pty_loop(fd_p);
#else
#if LIBVNCSERVER_HAVE_GRANTPT
	used_get_pty_ptmx = 1;
	return get_pty_ptmx(fd_p);
#else
	return get_pty_loop(fd_p);
#endif
#endif
}

void try_to_be_nobody(void) {

#if LIBVNCSERVER_HAVE_PWD_H
	struct passwd *pw;
	pw = getpwnam("nobody");

	if (pw) {
#if LIBVNCSERVER_HAVE_SETUID
		setuid(pw->pw_uid);
#endif
#if LIBVNCSERVER_HAVE_SETEUID
		seteuid(pw->pw_uid);
#endif
#if LIBVNCSERVER_HAVE_SETGID
		setgid(pw->pw_gid);
#endif
#if LIBVNCSERVER_HAVE_SETEGID
		setegid(pw->pw_gid);
#endif
	}
#endif	/* PWD_H */
}


static int slave_fd = -1, alarm_fired = 0;

static void close_alarm (int sig) {
	if (slave_fd >= 0) {
		close(slave_fd);
	}
	alarm_fired = 1;
	if (0) sig = 0;	/* compiler warning */
}

static void kill_child (pid_t pid, int fd) {
	int status;

	slave_fd = -1;
	alarm_fired = 0;
	if (fd >= 0) {
		close(fd);
	}
	kill(pid, SIGTERM);
	waitpid(pid, &status, WNOHANG); 
}

static int scheck(char *str, int n, char *name) {
	int j, i;

	if (! str) {
		return 0;
	}
	j = 0;
	for (i=0; i<n; i++) {
		if (str[i] == '\0') {
			j = 1;
			break;
		}
		if (!strcmp(name, "password")) {
			if (str[i] == '\n') {
				continue;
			}
		}
		if (str[i] < ' ' || str[i] >= 0x7f) {
			rfbLog("scheck: invalid character in %s.\n", name);	
			return 0;
		}
	}
	if (j == 0) {
		rfbLog("scheck: unterminated string in %s.\n", name);	
		return 0;
	}
	return 1;
}

int unixpw_list_match(char *user) {
	if (! unixpw_list || unixpw_list[0] == '\0') {
		return 1;
	} else {
		char *p, *q, *str = strdup(unixpw_list);
		int ok = 0;
		int notmode = 0;

		if (str[0] == '!') {
			notmode = 1;
			ok = 1;
			p = strtok(str+1, ",");
		} else {
			p = strtok(str, ",");
		}
		while (p) {
			if ( (q = strchr(p, ':')) != NULL ) {
				*q = '\0';	/* get rid of options. */
			}
			if (!strcmp(user, p)) {
				if (notmode) {
					ok = 0;
				} else {
					ok = 1;
				}
				break;
			}
			if (!notmode && !strcmp("*", p)) {
				ok = 1;
				break;
			}
			p = strtok(NULL, ",");
		}
		free(str);
		if (! ok) {
			rfbLog("unixpw_list_match: fail for '%s'\n", user);
			return 0;
		} else {
			rfbLog("unixpw_list_match: OK for '%s'\n", user);
			return 1;
		}
	}
}

int crypt_verify(char *user, char *pass) {
#ifndef UNIXPW_CRYPT
	return 0;
#else
	struct passwd *pwd;
	char *realpw, *cr;
	int n;

	if (! scheck(user, 100, "username")) {
		return 0;
	}
	if (! scheck(pass, 100, "password")) {
		return 0;
	}
	if (! unixpw_list_match(user)) {
		return 0;
	}

	pwd = getpwnam(user);
	if (! pwd) {
		return 0;
	}

	realpw = pwd->pw_passwd;
	if (realpw == NULL || realpw[0] == '\0') {
		return 0;
	}

	if (db > 1) fprintf(stderr, "realpw='%s'\n", realpw);

	if (strlen(realpw) < 12) {
		/* e.g. "x", try getspnam(), sometimes root for inetd, etc */
#if LIBVNCSERVER_HAVE_GETSPNAM
		struct spwd *sp = getspnam(user);
		if (sp != NULL && sp->sp_pwdp != NULL) {
			if (db) fprintf(stderr, "using getspnam()\n");
			realpw = sp->sp_pwdp;
		} else {
			if (db) fprintf(stderr, "skipping getspnam()\n");
		}
#endif
	}

	n = strlen(pass);
	if (pass[n-1] == '\n') {
		pass[n-1] = '\0';
	}

	/* XXX remove need for cast */
	cr = (char *) crypt(pass, realpw);
	if (db > 1) {
		fprintf(stderr, "user='%s' pass='%s' realpw='%s' cr='%s'\n",
		    user, pass, realpw, cr ? cr : "(null)");
	}
	if (cr == NULL || cr[0] == '\0') {
		return 0;
	}
	if (!strcmp(cr, realpw)) {
		return 1;
	} else {
		return 0;
	}
#endif	/* UNIXPW_CRYPT */
}

int unixpw_cmd_run(char *user, char *pass, char *cmd, char *line, int *n) {
	int i, len, rc;
	char *str;
	FILE *out;

	if (! user || ! pass) {
		return 0;
	}
	if (! unixpw_cmd || *unixpw_cmd == '\0') {
		return 0;
	}

	if (! scheck(user, 100, "username")) {
		return 0;
	}
	if (! scheck(pass, 100, "password")) {
		return 0;
	}
	if (! unixpw_list_match(user)) {
		return 0;
	}
	if (cmd == NULL) {
		cmd = "";
	}

	len = strlen(user) + 1 + strlen(pass) + 1 + 1;
	str = (char *) malloc(len);
	if (! str) {
		return 0;
	}
	str[0] = '\0';
	strcat(str, user);
	strcat(str, "\n");
	strcat(str, pass);
	if (!strchr(pass, '\n')) {
		strcat(str, "\n");
	}

	out = tmpfile();
	if (out == NULL) {
		rfbLog("unixpw_cmd_run tmpfile() failed.\n");
		clean_up_exit(1);
	}

	set_env("RFB_UNIXPW_CMD_RUN", cmd);

	rc = run_user_command(unixpw_cmd, unixpw_client, "cmd_verify",
	    str, strlen(str), out);

	set_env("RFB_UNIXPW_CMD_RUN", "");

	for (i=0; i < len; i++) {
		str[i] = '\0';
	}
	free(str);

	fflush(out);
	rewind(out);
	for (i=0; i < (*n) - 1; i++) {
		int c = fgetc(out);
		if (c == EOF) {
			break;
		}
		line[i] = (char) c;
	}
	fclose(out);
	*n = i;

	if (rc == 0) {
		return 1;
	} else {
		return 0;
	}
}


int cmd_verify(char *user, char *pass) {
	int i, len, rc;
	char *str;

	if (! user || ! pass) {
		return 0;
	}
	if (! unixpw_cmd || *unixpw_cmd == '\0') {
		return 0;
	}

	if (! scheck(user, 100, "username")) {
		return 0;
	}
	if (! scheck(pass, 100, "password")) {
		return 0;
	}
	if (! unixpw_list_match(user)) {
		return 0;
	}

	if (unixpw_client) {
		ClientData *cd = (ClientData *) unixpw_client->clientData;
		if (cd) {
			cd->username = strdup(user);
		}
	}

	len = strlen(user) + 1 + strlen(pass) + 1 + 1;
	str = (char *) malloc(len);
	if (! str) {
		return 0;
	}
	str[0] = '\0';
	strcat(str, user);
	strcat(str, "\n");
	strcat(str, pass);
	if (!strchr(pass, '\n')) {
		strcat(str, "\n");
	}

	rc = run_user_command(unixpw_cmd, unixpw_client, "cmd_verify",
	    str, strlen(str), NULL);

	for (i=0; i < len; i++) {
		str[i] = '\0';
	}
	free(str);

	if (rc == 0) {
		return 1;
	} else {
		return 0;
	}
}

int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size, int nodisp) {
#ifndef UNIXPW_SU
	return 0;
#else
	int i, j, status, fd = -1, sfd, tfd, drain_size = 65536, rsize = 0;
	int slow_pw = 1;
	char *slave, *bin_true = NULL, *bin_su = NULL;
	pid_t pid, pidw;
	struct stat sbuf;
	static int first = 1;
	char instr[64], cbuf[10];

	if (first) {
		set_db();
		first = 0;
	}
	rfbLog("su_verify: '%s' for %s.\n", user, cmd ? "command" : "login");
	fflush(stderr);

	if (! scheck(user, 100, "username")) {
		return 0;
	}
	if (! scheck(pass, 100, "password")) {
		return 0;
	}
	if (! unixpw_list_match(user)) {
		return 0;
	}

	/* unixpw */
	if (no_external_cmds || !cmd_ok("unixpw")) {
		rfbLog("su_verify: cannot run external commands.\n");	
		clean_up_exit(1);
	}

#define SU_DEBUG 0
#if SU_DEBUG
	if (stat("/su", &sbuf) == 0) {
		bin_su = "/su";	/* Freesbie read-only-fs /bin/su not suid! */
#else
	if (0) {
		;
#endif
	} else if (stat("/bin/su", &sbuf) == 0) {
		bin_su = "/bin/su";
	} else if (stat("/usr/bin/su", &sbuf) == 0) {
		bin_su = "/usr/bin/su";
	}
	if (bin_su == NULL) {
		rfbLogPerror("existence /bin/su");
		fflush(stderr);
		return 0;
	}

	if (stat("/bin/true", &sbuf) == 0) {
		bin_true = "/bin/true";
	} if (stat("/usr/bin/true", &sbuf) == 0) {
		bin_true = "/usr/bin/true";
	}
	if (cmd != NULL && cmd[0] != '\0') {
		/* this is for ext. cmd su -c "my cmd" after login */
		bin_true = cmd;
	}
	if (bin_true == NULL) {
		rfbLogPerror("existence /bin/true");
		fflush(stderr);
		return 0;
	}

	slave = get_pty(&fd);

	if (slave == NULL) {
		rfbLogPerror("get_pty failed.");
		fflush(stderr);
		return 0;
	}

	if (db) fprintf(stderr, "cmd is: %s\n", cmd);
	if (db) fprintf(stderr, "slave is: %s fd=%d\n", slave, fd);

	if (fd < 0) {
		rfbLogPerror("get_pty fd < 0");
		fflush(stderr);
		return 0;
	}

	fcntl(fd, F_SETFD, 1);

	pid = fork();
	if (pid < 0) {
		rfbLogPerror("fork");
		fflush(stderr);
		close(fd);
		return 0;
	}

	if (pid == 0) {
		/* child */

		int ttyfd;
		ttyfd = -1;	/* compiler warning */

#if LIBVNCSERVER_HAVE_SETSID
		if (setsid() == -1) {
			perror("setsid");
			exit(1);
		}
#else
		if (setpgrp() == -1) {
			perror("setpgrp");
			exit(1);
		}
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCNOTTY)
		ttyfd = open("/dev/tty", O_RDWR);
		if (ttyfd >= 0) {
			(void) ioctl(ttyfd, TIOCNOTTY, (char *) 0);
			close(ttyfd);
		}
#endif	

#endif	/* SETSID */

		close(0);
		close(1);
		close(2);

		sfd = open(slave, O_RDWR);
		if (sfd < 0) {
			exit(1);
		}

/* streams options fixups, handle cases as they are found: */
#if defined(__hpux)
#if LIBVNCSERVER_HAVE_SYS_STROPTS_H
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(I_PUSH)
		if (used_get_pty_ptmx) {
			ioctl(sfd, I_PUSH, "ptem");
			ioctl(sfd, I_PUSH, "ldterm");
			ioctl(sfd, I_PUSH, "ttcompat");
		}
#endif
#endif
#endif

		/* n.b. sfd will be 0 since we closed 0. so dup it to 1 and 2 */
		if (fcntl(sfd, F_DUPFD, 1) == -1) {
			exit(1);
		}
		if (fcntl(sfd, F_DUPFD, 2) == -1) {
			exit(1);
		}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCSCTTY)
		ioctl(sfd, TIOCSCTTY, (char *) 0);
#endif

		if (db > 2) {
			char nam[256];
			unlink("/tmp/isatty");
			tfd = open("/tmp/isatty", O_CREAT|O_WRONLY, 0600);
			if (isatty(sfd)) {
				close(tfd);
				sprintf(nam, "stty -a < %s > /tmp/isatty 2>&1",
				    slave);
				system(nam);
			} else {
				write(tfd, "NOTTTY\n", 7);
				close(tfd);
			}
		}

		chdir("/");

		try_to_be_nobody();
#if LIBVNCSERVER_HAVE_GETUID
		if (getuid() == 0 || geteuid() == 0) {
			exit(1);
		}
#else
		exit(1);
#endif

		set_env("LC_ALL", "C");
		set_env("LANG", "C");
		set_env("SHELL", "/bin/sh");
		if (nodisp) {
			/* this will cause timeout problems with pam_xauth */
			int k;
			for (k=0; k<3; k++) {
				if (getenv("DISPLAY")) {
					char *s = getenv("DISPLAY");
					if (s) *(s-2) = '_';	/* quite... */
				}
				if (getenv("XAUTHORITY")) {
					char *s = getenv("XAUTHORITY");
					if (s) *(s-2) = '_';	/* quite... */
				}
			}
		}

		/* synchronize with parent: */
		write(2, "C", 1);

		if (cmd) {
			execlp(bin_su, bin_su, "-", user, "-c",
			    bin_true, (char *) NULL);
		} else {
			execlp(bin_su, bin_su, user, "-c",
			    bin_true, (char *) NULL);
		}
		exit(1);
	}
	/* parent */

	if (db)	fprintf(stderr, "pid: %d\n", pid);

	/*
	 * set an alarm for blocking read() to close the master
	 * (presumably terminating the child. SIGTERM too...)
	 */
	slave_fd = fd;
	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(10);

	/* synchronize with child: */
	cbuf[0] = '\0';
	cbuf[1] = '\0';
	for (i=0; i<10; i++) {
		int n;
		cbuf[0] = '\0';
		cbuf[1] = '\0';
		n = read(fd, cbuf, 1);
		if (n < 0 && errno == EINTR) {
			continue;
		} else {
			break;
		}
	}

	if (db) {
		fprintf(stderr, "read from child: '%s'\n", cbuf);
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCTRAP)
	{
		int control = 1;
		ioctl(fd, TIOCTRAP, &control);
	}
#endif
	
	/*
	 * In addition to checking exit code below, we watch for the
	 * appearance of the string "Password:".  BSD does not seem to
	 * ask for a password trying to su to yourself.  This is the
	 * setting in /etc/pam.d/su:
	 * 	auth      sufficient  pam_self.so
	 * it may be commented out without problem.
	 */
	for (i=0; i< (int) sizeof(instr); i++) {
		instr[i] = '\0';
	}

	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(10);

	j = 0;
	for (i=0; i < (int) strlen("Password:"); i++) {
		char pstr[] = "password:";
		int n, problem;	

		cbuf[0] = '\0';
		cbuf[1] = '\0';

		n = read(fd, cbuf, 1);
		if (n < 0 && errno == EINTR) {
			i--;
			if (i < -1) i = -1;
			continue;
		}

		if (db) {
			fprintf(stderr, "%s", cbuf);
			if (db > 3 && n == 1 && cbuf[0] == ':') {
				char cmd0[32];
				usleep( 100 * 1000 );
				fprintf(stderr, "\n\n");
				sprintf(cmd0, "ps wu %d", pid);
				system(cmd0);
				sprintf(cmd0, "stty -a < %s", slave);
				system(cmd0);
				fprintf(stderr, "\n\n");
			}
		}

		if (n == 1) {
			if (isspace((unsigned char) cbuf[0])) {
				i--;
				if (i < -1) i = -1;
				continue;
			}
			if (j >= (int) sizeof(instr)-1) {
				rfbLog("su_verify: problem finding Password:\n");	
				fflush(stderr);
				return 0;
			}
			instr[j++] = tolower((unsigned char)cbuf[0]);
		}

		problem = 0;
		if (n <= 0) {
			problem = 1;
		} else if (strstr(pstr, instr) != pstr) {
#ifdef _AIX
			if (UT.sysname && strstr(UT.sysname, "AIX")) {
				/* handle: runge's Password: */
				char *luser = (char *) malloc(strlen(user) + 10);

				sprintf(luser, "%s's", user);
				lowercase(luser);
				if (db) fprintf(stderr, "\nAIX luser compare: \"%s\" to \"%s\"\n", luser, instr);
				if (strstr(luser, instr) == luser) {
					if (db) fprintf(stderr, "AIX luser compare: strstr OK.\n");
					if (!strcmp(luser, instr)) {
						if (db) fprintf(stderr, "AIX luser compare: strings equal.\n");
						i = -1;	
						j = 0;
						memset(instr, 0, sizeof(instr));
						free(luser);
						continue;
					} else {
						i--;
						if (i < -1) i = -1;
						free(luser);
						continue;
					}
				} else {
					if (db) fprintf(stderr, "AIX luser compare: problem=1\n");
					problem = 1;
				}
				free(luser);
			} else
#endif
			{
				problem = 1;
			}
		}

		if (problem) {
			if (db) {
				fprintf(stderr, "\"Password:\" did not "
				    "appear: '%s'" " n=%d\n", instr, n);
				if (db > 3 && n == 1 && j < 32) {
					continue;
				}
			}
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			kill_child(pid, fd);
			return 0;
		}
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

	if (db) fprintf(stderr, "\nsending passwd: %s\n", db > 2 ? pass : "****");
	usleep(100 * 1000);
	if (slow_pw) {
		unsigned int k;
		for (k = 0; k < strlen(pass); k++) {
			write(fd, pass+k, 1); 
			usleep(100 * 1000);
		}
	} else {
		write(fd, pass, strlen(pass)); 
	}

	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(15);

	/*
	 * try to drain the output, hopefully never as much as 4096 (motd?)
	 * if we don't drain we may block at waitpid.  If we close(fd), the
	 * make cause child to die by signal.
	 */
	if (rbuf && *rbuf_size > 0) {
		/* asked to return output of command */
		drain_size = *rbuf_size;
		rsize = 0;
	}
	if (db) fprintf(stderr, "\ndraining:\n");
	for (i = 0; i< drain_size; i++) {
		int n;	
		
		cbuf[0] = '\0';
		cbuf[1] = '\0';

		n = read(fd, cbuf, 1);
		if (n < 0 && errno == EINTR) {
			if (db) fprintf(stderr, "\nEINTR n=%d i=%d --", n, i);
			i--;
			if (i < 0) i = 0;
			continue;
		}

		if (db) fprintf(stderr, "\nn=%d i=%d errno=%d %.6f  '%s'", n, i, errno, dnowx(), cbuf);

		if (n <= 0) {
			break;
		}
		if (rbuf && *rbuf_size > 0) {
			rbuf[rsize++] = cbuf[0];
		}
	}
	if (db && rbuf) fprintf(stderr, "\nrbuf: '%s'\n", rbuf);

	if (rbuf && *rbuf_size > 0) {
		char *s = rbuf;
		char *p = strdup(pass);
		int n, o = 0;
		
		n = strlen(p);
		if (p[n-1] == '\n') {
			p[n-1] = '\0';
		}
		/*
		 * usually is: Password: mypassword\r\n\r\n<output-of-command>
		 * and output will have \n -> \r\n
		 */
		if (rbuf[0] == ' ') {
			s++;
			o++;
		}
		if (strstr(s, p) == s) {
			s += strlen(p);
			o += strlen(p);
			for (n = 0; n < 4; n++) {
				if (s[0] == '\r' || s[0] == '\n') {
					s++;
					o++;
				}
			}
		}
		if (o > 0) {
			int i = 0;
			rsize -= o;
			while (o < drain_size) {
				rbuf[i++] = rbuf[o++];
			}
		}
		*rbuf_size = rsize;
		strzero(p);
		free(p);
	}

	if (db) fprintf(stderr, "\n--\n");

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

	slave_fd = -1;
	
	pidw = waitpid(pid, &status, 0); 
	close(fd);

	if (pid != pidw) {
		return 0;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		return 1; /* this is the only return of success. */
	} else {
		return 0;
	}
#endif	/* UNIXPW_SU */
}

int unixpw_verify(char *user, char *pass) {
	int ok = 0;
	if (unixpw_cmd) {
		if (cmd_verify(user, pass)) {
			rfbLog("unixpw_verify: cmd_verify login for '%s'"
			    " succeeded.\n", user);
			fflush(stderr);
			ok = 1;
		} else {
			rfbLog("unixpw_verify: cmd_verify login for '%s'"
			    " failed.\n", user);
			fflush(stderr);
			usleep(3000*1000);
			ok = 0;
		}
	} else if (unixpw_nis) {
		if (crypt_verify(user, pass)) {
			rfbLog("unixpw_verify: crypt_verify login for '%s'"
			    " succeeded.\n", user);
			fflush(stderr);
			ok = 1;
		} else {
			rfbLog("unixpw_verify: crypt_verify login for '%s'"
			    " failed.\n", user);
			fflush(stderr);
			usleep(3000*1000);
			ok = 0;
		}
	} else {
		if (su_verify(user, pass, NULL, NULL, NULL, 1)) {
			rfbLog("unixpw_verify: su_verify login for '%s'"
			    " succeeded.\n", user);
			fflush(stderr);
			ok = 1;
		} else {
			rfbLog("unixpw_verify: su_verify login for '%s'"
			    " failed.\n", user);
			fflush(stderr);
			/* use su(1)'s sleep */
			ok = 0;
		}
	}
	return ok;
}

static int skip_it = 0;

static void progress_skippy(void) {
	int i, msec = get_net_latency();	/* probabaly not set yet.. */

	if (msec > 300) {
		msec = 300;
	} else if (msec <= 100) {
		msec = 100;
	}

	skip_it = 1;
	for (i = 0; i < 5; i++) {
		if (i == 2) {
			rfbPE(msec * 1000);
		} else {
			rfbPE(-1);
		}
		usleep(10*1000);
	}
	skip_it = 0;

	usleep(50*1000);
}

void check_unixpw_userprefs(void) {
	char *prefs = getenv("FD_USERPREFS");
	if (keep_unixpw_user == NULL || keep_unixpw_opts == NULL) {
		return;
	}
#if LIBVNCSERVER_HAVE_PWD_H
	if (prefs != NULL && !strchr(prefs, '/')) {
		struct passwd *pw = getpwnam(keep_unixpw_user);
		if (pw != NULL) {
			char *file;
			FILE *f;

			file = (char *) malloc(strlen(pw->pw_dir) + 1 + strlen(prefs) + 1);
			sprintf(file, "%s/%s", pw->pw_dir, prefs);

			f = fopen(file, "r");
			if (f) {
				char *t, *q, buf[1024];
				memset(buf, 0, sizeof(buf));

				fgets(buf, 1024, f);
				fclose(f);

				q = strchr(buf, '\n');
				if (q) *q = '\0';
				q = strchr(buf, '\r');
				if (q) *q = '\0';

				rfbLog("read user prefs %s: %s\n", file, buf);

				if (buf[0] == '#') buf[0] = '\0';

				t = (char *) malloc(strlen(keep_unixpw_opts) + 1 + strlen(buf) + 1);
				sprintf(t, "%s,%s", keep_unixpw_opts, buf); 
				free(keep_unixpw_opts);
				keep_unixpw_opts = t;
			} else {
				rfbLog("could not read user prefs %s\n", file);
				rfbLogPerror("fopen");
			}
			free(file);
		}
	}
#endif
}


void unixpw_verify_screen(char *user, char *pass) {
	int x, y;
	char li[] = "Login incorrect";
	char ls[] = "Login succeeded";
	char log[] = "login: ";
	char *colon = NULL;
	ClientData *cd = NULL;
	int ok;

if (db) fprintf(stderr, "unixpw_verify: '%s' '%s'\n", user, db > 1 ? pass : "********");
	rfbLog("unixpw_verify: '%s'\n", user ? user : "(null)");

	if (user) {
		colon = strchr(user, ':');
	}
	if (colon) {
		*colon = '\0';
		rfbLog("unixpw_verify: colon: '%s'\n", user);
	}
	fflush(stderr);
	if (unixpw_client) {
		cd = (ClientData *) unixpw_client->clientData;
		if (cd) {
			char *str = (char *)malloc(strlen("UNIX:") +
			    strlen(user) + 1);
			sprintf(str, "UNIX:%s", user);	
			if (cd->username) {
				free(cd->username);
			}
			cd->username = str;
		}
	}

	ok = unixpw_verify(user, pass);

	if (ok) {
		char_row++;
		char_col = 0;

		x = text_x();
		y = text_y();
		rfbDrawString(pscreen, &default8x16Font, x, y, ls, white_pixel());
		unixpw_mark();

		progress_skippy();

		unixpw_accept(user);

		if (keep_unixpw) {
			keep_unixpw_user = strdup(user);
			keep_unixpw_pass = strdup(pass);
			if (colon) {
				keep_unixpw_opts = strdup(colon+1);
			} else {
				keep_unixpw_opts = strdup("");
			}
			check_unixpw_userprefs();
		}

		if (colon) *colon = ':';

		return;
	}
	if (colon) *colon = ':';

	if (tries < 2) {
		char_row++;
		char_col = 0;

		x = text_x();
		y = text_y();
		rfbDrawString(pscreen, &default8x16Font, x, y, li, white_pixel());

		char_row += 2;

		x = text_x();
		y = text_y();
		rfbDrawString(pscreen, &default8x16Font, x, y, log, white_pixel());

		char_col = strlen(log);

		unixpw_mark();

		unixpw_last_try_time = time(NULL);
		unixpw_keystroke(0, 0, 2);
		tries++;
	} else {
		unixpw_deny();
	}
}

static void set_db(void) {
	if (getenv("DEBUG_UNIXPW")) {
		db = atoi(getenv("DEBUG_UNIXPW"));
		rfbLog("DEBUG_UNIXPW: %d\n", db);
	}
}

void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init) {
	int x, y, i, rc, nmax = 100;
	static char user_r[100], user[100], pass[100];
	static int  u_cnt = 0, p_cnt = 0, t_cnt = 0, first = 1;
	static int echo = 1;
	char keystr[100];
	char *str;

	if (skip_it) {
		return;
	}

	if (first) {
		set_db();
		first = 0;
		for (i=0; i < nmax; i++) {
			user_r[i] = '\0';
			user[i] = '\0';
			pass[i] = '\0';
		}
	}

	if (init) {
		in_login = 1;
		in_passwd = 0;
		unixpw_denied = 0;
		echo = 1;
		if (init == 1) {
			tries = 0;
		}

		u_cnt = 0;
		p_cnt = 0;
		t_cnt = 0;
		for (i=0; i<nmax; i++) {
			user[i] = '\0';
			pass[i] = '\0';
		}
		if (keep_unixpw_user) {
			free(keep_unixpw_user);
			keep_unixpw_user = NULL;
		}
		if (keep_unixpw_pass) {
			strzero(keep_unixpw_pass);
			free(keep_unixpw_pass);
			keep_unixpw_pass = NULL;
		}
		if (keep_unixpw_opts) {
			strzero(keep_unixpw_opts);
			free(keep_unixpw_opts);
			keep_unixpw_opts = NULL;
		}

		return;
	}

	if (unixpw_denied) {
		rfbLog("unixpw_keystroke: unixpw_denied state: 0x%x\n", (int) keysym);
		return;
	}
	if (keysym <= 0) {
		rfbLog("unixpw_keystroke: bad keysym1: 0x%x\n", (int) keysym);
		return;
	}

	/* rfbKeySym = uint32_t */
	/* KeySym = XID = CARD32 = (unsigned long or unsigned int on LONG64) */
	X_LOCK;
	str = XKeysymToString(keysym);
	X_UNLOCK;
	if (str == NULL) {
		rfbLog("unixpw_keystroke: bad keysym2: 0x%x\n", (int) keysym);
		return;
	}

	rc = snprintf(keystr, 100, "%s", str);
	if (rc < 1 || rc > 90) {
		rfbLog("unixpw_keystroke: bad keysym3: 0x%x\n", (int) keysym);
		return;
	}

	if (db > 2) {
		fprintf(stderr, "%s / %s  0x%x %s\n", in_login ? "login":"pass ",
		    down ? "down":"up  ", keysym, keystr);
	}

	if (keysym == XK_Return || keysym == XK_Linefeed || keysym == XK_Tab) {
		/* let "up" pass down below for Return case */
		if (down) {
			return;
		}
	} else if (! down) {
		return;
	}
	if (keysym == XK_F1) {
		char h1[] = "F1-Help:  For 'login:' type in the username and press Enter, then for 'Password:' enter the password.";
		char hf[] = "  Once logged in, username's X session will be searched for and if found then attached to.";
		char hc[] = "  Once logged in, username's X session is sought and attached to, otherwise a new session is created.";
		char hx[] = "  Once logged in, username's X session is sought and attached to, otherwise a login greeter is presented.";
		char h2[] = "  Specify options after a ':' like this:  username:opt,opt=val,...    Where an opt may be any of:";
		char h3[] = "    scale=... (n/m); scale_cursor=... (sc=); solid (so); id=; repeat; clear_mods (cm); clear_keys (ck);";
		char h4[] = "    clear_all (ca); speeds=... (sp=); readtimeout=... (rd=) rotate=... (ro=); noncache (nc) (nc=n);";
		char h5[] = "    geom=WxHxD (ge=); nodisplay=... (nd=); viewonly (vo); tag=...; gnome kde twm fvwm mwm dtwm wmaker";
		char h6[] = "    xfce lxde enlightenment Xsession failsafe.   Examples:  fred:3/4,so,cm  wilma:geom=1024x768x16,kde";
		int ch = 13, p;
		if (!pscreen || pscreen->width < 640 || pscreen->height < 480) {
			return;
		}
		if (f1_help) {
			p = black_pixel();
			f1_help = 0;
		} else {
			p = white_pixel();
			f1_help = 1;
			unixpw_last_try_time = time(NULL) + 45;
		}
		rfbDrawString(pscreen, &default6x13Font, 8, 2+1*ch, h1, p);
		if (use_dpy == NULL) {
			;
		} else if (strstr(use_dpy, "cmd=FINDDISPLAY")) {
			rfbDrawString(pscreen, &default6x13Font, 8, 2+2*ch, hf, p);
		} else if (strstr(use_dpy, "cmd=FINDCREATEDISPLAY")) {
			if (strstr(use_dpy, "xdmcp")) {
				rfbDrawString(pscreen, &default6x13Font, 8, 2+2*ch, hx, p);
			} else {
				rfbDrawString(pscreen, &default6x13Font, 8, 2+2*ch, hc, p);
			}
		}
		rfbDrawString(pscreen, &default6x13Font, 8, 2+3*ch, h2, p);
		rfbDrawString(pscreen, &default6x13Font, 8, 2+4*ch, h3, p);
		rfbDrawString(pscreen, &default6x13Font, 8, 2+5*ch, h4, p);
		rfbDrawString(pscreen, &default6x13Font, 8, 2+6*ch, h5, p);
		rfbDrawString(pscreen, &default6x13Font, 8, 2+7*ch, h6, p);
		if (!f1_help) {
			rfbDrawString(pscreen, &default6x13Font, 8, 2+1*ch, "F1-Help:", white_pixel());
		}
		unixpw_mark();
		return;
	}
	if (unixpw_system_greeter_active && keysym == XK_Escape) {
		char *u = get_user_name();
		if (keep_unixpw) {
			char *colon = strchr(user, ':');
			keep_unixpw_user = strdup(u);
			keep_unixpw_pass = strdup("");
			if (colon) {
				keep_unixpw_opts = strdup(colon+1);
			} else {
				keep_unixpw_opts = strdup("");
			}
			check_unixpw_userprefs();
		}
		unixpw_system_greeter_active = 2;
		set_env("X11VNC_XDM_ONLY", "1");
		rfbLog("unixpw_system_greeter: VNC client pressed 'Escape'. Allowing\n");
		rfbLog("unixpw_system_greeter: a *FREE* (no password) connection to\n");
		rfbLog("unixpw_system_greeter: the system XDM/GDM/KDM login greeter.\n");
		if (1) {
			char msg[] = " Please wait... ";
			rfbDrawString(pscreen, &default8x16Font,
			    text_x(), text_y(), msg, white_pixel());
			unixpw_mark();

			progress_skippy();
		}
		unixpw_accept(u);
		free(u);
		return;
	}

	if (in_login && keysym == XK_Escape && u_cnt == 0) {
		echo = 0;	
		rfbLog("unixpw_keystroke: echo off.\n");
		return;
	}

	t_cnt++;

	if (in_login) {
		if (keysym == XK_BackSpace || keysym == XK_Delete) {
			if (u_cnt > 0) {
				user[u_cnt-1] = '\0';
				u_cnt--;

				x = text_x();
				y = text_y();
				if (scaling) {
					int x2 = x / scale_fac_x;
					int y2 = y / scale_fac_y;
					int w2 = char_w / scale_fac_x;
					int h2 = char_h / scale_fac_y;

					x2 = nfix(x2, dpy_x);
					y2 = nfix(y2, dpy_y);
					
					zero_fb(x2 - w2, y2 - h2, x2, y2);
					mark_rect_as_modified(x2 - w2,
					    y2 - h2, x2, y2, 0);
				} else {
					zero_fb(x - char_w, y - char_h, x, y);
					mark_rect_as_modified(x - char_w,
					    y - char_h, x, y, 0);
				}
				char_col--;
			}

			return;
		}

		if (keysym == XK_Return || keysym == XK_Linefeed || keysym == XK_Tab) {
			char pw[] = "Password: ";

			if (down) {
				/*
				 * require Up so the Return Up is not processed
				 * by the normal session after login.
				 * (actually we already returned above)
				 */
				return;
			}

			if (t_cnt == 1) {
				/* accidental initial return, e.g. from xterm */
				return;
			}

			in_login = 0;
			in_passwd = 1;

			char_row++;
			char_col = 0;

			x = text_x();
			y = text_y();
			rfbDrawString(pscreen, &default8x16Font, x, y, pw,
			    white_pixel());

			char_col = strlen(pw);
			unixpw_mark();
			return;
		}

		if (u_cnt == 0 && keysym == XK_Up) {
			/*
			 * Allow user to hit Up arrow at beginning to
			 * regain their username plus any options.
			 */
			int i;
			for (i=0; i < nmax; i++) {
				user[i] = '\0';
			}
			for (i=0; i < nmax; i++) {
				char str[10];
				user[u_cnt++] = user_r[i];
				if (user_r[i] == '\0') {
					break;
				}
				str[0] = (char) user_r[i];
				str[1] = '\0';

				x = text_x();
				y = text_y();
				if (echo) {
					rfbDrawString(pscreen, &default8x16Font, x, y,
					    str, white_pixel());
				}
				mark_rect_as_modified(x, y-char_h, x+char_w,
				    y, scaling);
				char_col++;
				usleep(10*1000);
			}
			return;
		}

		if (keysym < ' ' || keysym >= 0x7f) {
			/* require normal keyboard characters for username */
			rfbLog("unixpw_keystroke: bad keysym4: 0x%x\n", (int) keysym);
			return;
		}

		if (u_cnt >= nmax - 1) {
			/* user[u_cnt=99] will be '\0' */
			rfbLog("unixpw_deny: username too long: %d\n", u_cnt);
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			unixpw_deny();
			return;
		}

#if 0
		user[u_cnt++] = keystr[0];
#else
		user[u_cnt++] = (char) keysym;
		for (i=0; i < nmax; i++) {
			/* keep a full copy of username */
			user_r[i] = user[i];
		}
		keystr[0] = (char) keysym;
#endif
		keystr[1] = '\0';

		x = text_x();
		y = text_y();

if (db && db <= 2) fprintf(stderr, "u_cnt: %d %d/%d ks: 0x%x  '%s'\n", u_cnt, x, y, keysym, keystr);

		if (echo ) {
			rfbDrawString(pscreen, &default8x16Font, x, y, keystr, white_pixel());
		}

		mark_rect_as_modified(x, y-char_h, x+char_w, y, scaling);
		char_col++;

		return;

	} else if (in_passwd) {
		if (keysym == XK_BackSpace || keysym == XK_Delete) {
			if (p_cnt > 0) {
				pass[p_cnt-1] = '\0';
				p_cnt--;
			}
			return;
		}
		if (keysym == XK_Return || keysym == XK_Linefeed) {
			if (down) {
				/*
				 * require Up so the Return Up is not processed
				 * by the normal session after login.
				 * (actually we already returned above)
				 */
				return;
			}

			if (1) {
				char msg[] = " Please wait... ";
				rfbDrawString(pscreen, &default8x16Font,
				    text_x(), text_y(), msg, white_pixel());
				unixpw_mark();

				progress_skippy();
			}

			in_login = 0;
			in_passwd = 0;

			pass[p_cnt++] = '\n';
			unixpw_verify_screen(user, pass);
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			return;
		}

		if (keysym < ' ' || keysym >= 0x7f) {
			/* require normal keyboard characters for password */
			return;
		}

		if (p_cnt >= nmax - 2) {
			/* pass[u_cnt=98] will be '\n' */
			/* pass[u_cnt=99] will be '\0' */
			rfbLog("unixpw_deny: password too long: %d\n", p_cnt);
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			unixpw_deny();
			return;
		}

		pass[p_cnt++] = (char) keysym;

		return;

	} else {
		/* should not happen... anyway clean up a bit. */
		u_cnt = 0;
		p_cnt = 0;
		for (i=0; i<nmax; i++) {
			user_r[i] = '\0';
			user[i] = '\0';
			pass[i] = '\0';
		}

		return;
	}
}

static void apply_opts (char *user) {
	char *p, *q, *str, *opts = NULL, *opts_star = NULL;
	rfbClientPtr cl;
	ClientData *cd;
	int i, notmode = 0;

	if (! unixpw_client) {
		rfbLog("apply_opts: unixpw_client is NULL\n");
		clean_up_exit(1);
	}
	cd = (ClientData *) unixpw_client->clientData;
	cl = unixpw_client;

	if (! cd) {
		rfbLog("apply_opts: no ClientData\n");
	}
	
	if (user && cd) {
		if (cd->unixname) {
			free(cd->unixname);
		}
		cd->unixname = strdup(user);
		rfbLog("apply_opts: set unixname to: %s\n", cd->unixname);
	}

	if (! unixpw_list) {
		return;
	}
	str = strdup(unixpw_list);

	/* apply any per-user options. */
	if (str[0] == '!') {
		p = strtok(str+1, ",");
		notmode = 1;
	} else {
		p = strtok(str, ",");
	}
	while (p) {
		if ( (q = strchr(p, ':')) != NULL ) {
			*q = '\0';	/* get rid of options. */
		} else {
			p = strtok(NULL, ",");
			continue;
		}
		if (user && !strcmp(user, p)) {
			/* will not happen in notmode */
			opts = strdup(q+1);
		}
		if (!strcmp("*", p)) {
			opts_star = strdup(q+1);
		}
		p = strtok(NULL, ",");
	}
	free(str);

	for (i=0; i < 2; i++) {
		char *s = (i == 0) ? opts_star : opts;
		if (s == NULL) {
			continue;
		}
		p = strtok(s, "+");
		while (p) {
			if (!strcmp(p, "viewonly")) {
				cl->viewOnly = TRUE;
				if (cd) {
					strncpy(cd->input, "-", CILEN);
				}
			} else if (!strcmp(p, "fullaccess")) {
				cl->viewOnly = FALSE;
				if (cd) {
					strncpy(cd->input, "-", CILEN);
				}
			} else if ((q = strstr(p, "input=")) == p) {
				q += strlen("input=");
				if (cd) {
					strncpy(cd->input, q, CILEN);
				}
			} else if (!strcmp(p, "deny")) {
				cl->viewOnly = TRUE;
				unixpw_deny();
				break;
			}
			p = strtok(NULL, "+");
		}
		free(s);
	}
}

void unixpw_accept(char *user) {
	apply_opts(user);

	if (!use_stunnel) {
		ssl_helper_pid(0, -2);	/* waitall */
	}

	if (accept_cmd && strstr(accept_cmd, "popup") == accept_cmd) {
		if (use_dpy && strstr(use_dpy, "WAIT:") == use_dpy &&
		    dpy == NULL) {
			/* handled in main() */
			unixpw_client->onHold = TRUE;
		} else if (! accept_client(unixpw_client)) {
			unixpw_deny();
			return;
		}
	}

	if (started_as_root == 1 && users_list
	    && strstr(users_list, "unixpw=") == users_list) {
		if (getuid() && geteuid()) {
			rfbLog("unixpw_accept: unixpw= but not root\n");
			started_as_root = 2;
		} else {
			char *u = (char *)malloc(strlen(user)+1); 

			u[0] = '\0';
			if (!strcmp(users_list, "unixpw=")) {
				sprintf(u, "+%s", user);
			} else {
				char *p, *str = strdup(users_list);
				p = strtok(str + strlen("unixpw="), ",");
				while (p) {
					if (!strcmp(p, user)) {
						sprintf(u, "+%s", user);
						break;
					}
					p = strtok(NULL, ",");
				}
				free(str);
			}
			
			if (u[0] == '\0') {
				rfbLog("unixpw_accept skipping switch to user: %s\n", user);
			} else if (switch_user(u, 0)) {
				rfbLog("unixpw_accept switched to user: %s\n", user);
			} else {
				rfbLog("unixpw_accept failed to switch to user: %s\n", user);
			}
			free(u);
		}
	}

	if (unixpw_login_viewonly) {
		unixpw_client->viewOnly = TRUE;
	}
	unixpw_in_progress = 0;
	/* mutex */
	screen->permitFileTransfer = unixpw_file_xfer_save;
	if ((tightfilexfer = unixpw_tightvnc_xfer_save)) {
		/* this doesn't work: the current client is never registered! */
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
		rfbLog("rfbRegisterTightVNCFileTransferExtension: 1\n");
                rfbRegisterTightVNCFileTransferExtension();
#endif
	}
	unixpw_client = NULL;
	mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	if (macosx_console) {
		refresh_screen(1);
	}
}

void unixpw_deny(void) {
	int x, y, i;
	char pd[] = "Permission denied.";

	rfbLog("unixpw_deny: %d, %d\n", unixpw_denied, unixpw_in_progress);
	if (! unixpw_denied) {
		unixpw_denied = 1;

		char_row += 2;
		char_col = 0;
		x = char_x + char_col * char_w;
		y = char_y + char_row * char_h;

		rfbDrawString(pscreen, &default8x16Font, x, y, pd, white_pixel());
		unixpw_mark();

		for (i=0; i<5; i++) {
			rfbPE(-1);
			rfbPE(-1);
			usleep(500 * 1000);
		}
	}

	if (unixpw_client) {
		rfbCloseClient(unixpw_client);
		rfbClientConnectionGone(unixpw_client);
		rfbPE(-1);
	}

	unixpw_in_progress = 0;
	/* mutex */
	screen->permitFileTransfer = unixpw_file_xfer_save;
	if ((tightfilexfer = unixpw_tightvnc_xfer_save)) {
#ifdef LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
		rfbLog("rfbRegisterTightVNCFileTransferExtension: 2\n");
                rfbRegisterTightVNCFileTransferExtension();
#endif
	}
	unixpw_client = NULL;
	copy_screen();
}

void unixpw_msg(char *msg, int delay) {
	int x, y, i;

	char_row += 2;
	char_col = 0;
	x = char_x + char_col * char_w;
	y = char_y + char_row * char_h;

	rfbDrawString(pscreen, &default8x16Font, x, y, msg, white_pixel());
	unixpw_mark();

	for (i=0; i<5; i++) {
		rfbPE(-1);
		rfbPE(-1);
		rfbPE(50 * 1000);
		rfbPE(-1);
		usleep(500 * 1000);
		if (i >= delay) {
			break;
		}
	}
}
