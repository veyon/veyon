#ifndef _X11VNC_WINATTR_T_H
#define _X11VNC_WINATTR_T_H

/* -- winattr_t.h -- */

typedef struct winattr {
	Window win;
	int fetched;
	int valid;
	int x, y;
	int width, height;
	int depth;
	int class;
	int backing_store;
	int map_state;
	int rx, ry;
	double time; 
} winattr_t;

#endif /* _X11VNC_WINATTR_T_H */
