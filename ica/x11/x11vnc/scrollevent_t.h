#ifndef _X11VNC_SCROLLEVENT_T_H
#define _X11VNC_SCROLLEVENT_T_H

/* -- scrollevent_t.h -- */

typedef struct scroll_event {
	Window win, frame;
	int dx, dy;
	int x, y, w, h;
	double t;
	int win_x, win_y, win_w, win_h;	
	int new_x, new_y, new_w, new_h;	
} scroll_event_t;

#endif /* _X11VNC_SCROLLEVENT_T_H */
