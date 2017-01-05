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

#ifndef _X11VNC_UTIL_H
#define _X11VNC_UTIL_H

/* -- util.h -- */

extern int nfix(int i, int n);
extern int nmin(int n, int m);
extern int nmax(int n, int m);
extern int nabs(int n);
extern double dabs(double x);
extern void lowercase(char *str);
extern void uppercase(char *str);
extern char *lblanks(char *str);
extern void strzero(char *str);
extern int scan_hexdec(char *str, unsigned long *num);
extern int parse_geom(char *str, int *wp, int *hp, int *xp, int *yp, int W, int H);
extern void set_env(char *name, char *value);
extern char *bitprint(unsigned int st, int nbits);
extern char *get_user_name(void);
extern char *get_home_dir(void);
extern char *get_shell(void);
extern char *this_host(void);

extern int match_str_list(char *str, char **list);
extern char **create_str_list(char *cslist);

extern double dtime(double *);
extern double dtime0(double *);
extern double dnow(void);
extern double dnowx(void);
extern double rnow(void);
extern double rfac(void);

extern int rfbPE(long usec);
extern void rfbCFD(long usec);
extern double rect_overlap(int x1, int y1, int x2, int y2, int X1, int Y1,
    int X2, int Y2);
extern char *choose_title(char *display);


#define NONUL(x) ((x) ? (x) : "")

/*
	Put this in usleep2() for debug printout.
	fprintf(stderr, "_mysleep: %08d %10.6f %s:%d\n", (x), dnow() - x11vnc_start, __FILE__, __LINE__); \
 */

/* XXX usleep(3) is not thread safe on some older systems... */
extern struct timeval _mysleep;
#define usleep2(x) \
	_mysleep.tv_sec  = (x) / 1000000; \
	_mysleep.tv_usec = (x) % 1000000; \
	select(0, NULL, NULL, NULL, &_mysleep); 
#if !defined(X11VNC_USLEEP)
#undef usleep
#define usleep usleep2
#endif

/*
 * following is based on IsModifierKey in Xutil.h
*/
#define ismodkey(keysym) \
  ((((KeySym)(keysym) >= XK_Shift_L) && ((KeySym)(keysym) <= XK_Hyper_R) && \
  ((KeySym)(keysym) != XK_Caps_Lock) && ((KeySym)(keysym) != XK_Shift_Lock)))

/*
 * When threaded we have to mutex our X11 calls to avoid XIO crashes
 * due to callbacks.
 */
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
extern MUTEX(x11Mutex);
extern MUTEX(scrollMutex);
MUTEX(clientMutex);
MUTEX(inputMutex);
MUTEX(pointerMutex);
#endif

#define X_INIT INIT_MUTEX(x11Mutex)

#if 1
#define X_LOCK       LOCK(x11Mutex)
#define X_UNLOCK   UNLOCK(x11Mutex)
#else
extern int hxl;
#define X_LOCK       fprintf(stderr, "*** X_LOCK**  %d%s  %s:%d\n", \
	hxl, hxl  ? "  BAD-PRE-LOCK":"",   __FILE__, __LINE__);   LOCK(x11Mutex); hxl = 1;
#define X_UNLOCK     fprintf(stderr, "    x_unlock  %d%s  %s:%d\n", \
	hxl, !hxl ? "  BAD-PRE-UNLOCK":"", __FILE__, __LINE__); UNLOCK(x11Mutex); hxl = 0;
#endif

#define SCR_LOCK   if (use_threads) {LOCK(scrollMutex);}
#define SCR_UNLOCK if (use_threads) {UNLOCK(scrollMutex);}
#define SCR_INIT INIT_MUTEX(scrollMutex)

#define CLIENT_LOCK   if (use_threads) {LOCK(clientMutex);}
#define CLIENT_UNLOCK if (use_threads) {UNLOCK(clientMutex);}
#define CLIENT_INIT INIT_MUTEX(clientMutex)

#if 1
#define INPUT_LOCK   if (use_threads) {LOCK(inputMutex);}
#define INPUT_UNLOCK if (use_threads) {UNLOCK(inputMutex);}
#else
#define INPUT_LOCK
#define INPUT_UNLOCK
#endif
#define INPUT_INIT INIT_MUTEX(inputMutex)

#define POINTER_LOCK   if (use_threads) {LOCK(pointerMutex);}
#define POINTER_UNLOCK if (use_threads) {UNLOCK(pointerMutex);}
#define POINTER_INIT INIT_MUTEX(pointerMutex)

/*
 * The sendMutex member was added to libvncserver 0.9.8
 * rfb/rfb.h sets LIBVNCSERVER_SEND_MUTEX if present.
 */
#if LIBVNCSERVER_HAVE_LIBPTHREAD && defined(LIBVNCSERVER_SEND_MUTEX)
#define SEND_LOCK(cl)   if (use_threads)   LOCK((cl)->sendMutex);
#define SEND_UNLOCK(cl) if (use_threads) UNLOCK((cl)->sendMutex);
#else
#define SEND_LOCK(cl)
#define SEND_UNLOCK(cl)
#endif

#endif /* _X11VNC_UTIL_H */
