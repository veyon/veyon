#ifndef _X11VNC_BLACKOUT_T_H
#define _X11VNC_BLACKOUT_T_H

/* -- blackout_t.h -- */

typedef struct bout {
	int x1, y1, x2, y2;
} blackout_t;

#define BO_MAX 32
typedef struct tbout {
	blackout_t bo[BO_MAX];	/* hardwired max rectangles. */
	int cover;
	int count;
} tile_blackout_t;

#endif /* _X11VNC_BLACKOUT_T_H */
