#ifndef _X11VNC_XRANDR_H
#define _X11VNC_XRANDR_H

/* -- xrandr.h -- */

extern time_t last_subwin_trap;
extern int subwin_trap_count;
extern XErrorHandler old_getimage_handler;

extern int xrandr_present;
extern int xrandr_width;
extern int xrandr_height;
extern int xrandr_rotation;
extern Time xrandr_timestamp;
extern Time xrandr_cfg_time;

extern void initialize_xrandr(void);
extern int check_xrandr_event(char *msg);
extern int known_xrandr_mode(char *s);

#define XRANDR_SET_TRAP_RET(x,y)  \
	if (subwin || xrandr) { \
		trapped_getimage_xerror = 0; \
		old_getimage_handler = XSetErrorHandler(trap_getimage_xerror); \
		if (check_xrandr_event(y)) { \
			trapped_getimage_xerror = 0; \
			XSetErrorHandler(old_getimage_handler);	 \
			return(x); \
		} \
	}
#define XRANDR_CHK_TRAP_RET(x,y)  \
	if (subwin || xrandr) { \
		if (trapped_getimage_xerror) { \
			if (subwin) { \
				static int last = 0; \
				subwin_trap_count++; \
				if (time(NULL) > last_subwin_trap + 60) { \
					rfbLog("trapped GetImage xerror" \
					    " in SUBWIN mode. [%d]\n", \
					    subwin_trap_count); \
					last_subwin_trap = time(NULL); \
					last = subwin_trap_count; \
				} \
				if (subwin_trap_count - last > 30) { \
					/* window probably iconified */ \
					usleep(1000*1000); \
				} \
			} else { \
				rfbLog("trapped GetImage xerror" \
				    " in XRANDR mode.\n"); \
			} \
			trapped_getimage_xerror = 0; \
			XSetErrorHandler(old_getimage_handler);	 \
			check_xrandr_event(y); \
			X_UNLOCK; \
			return(x); \
		} \
	}

#endif /* _X11VNC_XRANDR_H */
