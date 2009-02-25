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

extern void rfbPE(long usec);
extern void rfbCFD(long usec);
extern double rect_overlap(int x1, int y1, int x2, int y2, int X1, int Y1,
    int X2, int Y2);
extern char *choose_title(char *display);


#define NONUL(x) ((x) ? (x) : "")

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
 * Not sure why... but when threaded we have to mutex our X11 calls to
 * avoid XIO crashes.
 */
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
extern MUTEX(x11Mutex);
#endif

#define X_INIT INIT_MUTEX(x11Mutex)

#if 1
#define X_LOCK       LOCK(x11Mutex)
#define X_UNLOCK   UNLOCK(x11Mutex)
#else
extern int hxl;
#define X_LOCK       fprintf(stderr, "*** X_LOCK**[%05d]  %d%s\n", \
	__LINE__, hxl, hxl ? "  BAD-PRE-LOCK":""); LOCK(x11Mutex); hxl = 1;
#define X_UNLOCK     fprintf(stderr, "    x_unlock[%05d]  %d%s\n", \
	__LINE__, hxl, !hxl ? "  BAD-PRE-UNLOCK":""); UNLOCK(x11Mutex); hxl = 0;
#endif

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
extern MUTEX(scrollMutex);
#endif
#define SCR_LOCK   if (use_threads) {LOCK(scrollMutex);}
#define SCR_UNLOCK if (use_threads) {UNLOCK(scrollMutex);}
#define SCR_INIT INIT_MUTEX(scrollMutex)

#endif /* _X11VNC_UTIL_H */
