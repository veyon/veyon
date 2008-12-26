#ifndef _X11VNC_XINERAMA_H
#define _X11VNC_XINERAMA_H

/* -- xinerama.h -- */
#include "blackout_t.h"

extern blackout_t blackr[];
extern tile_blackout_t *tile_blackout;
extern int blackouts;

extern void initialize_blackouts_and_xinerama(void);
extern void push_sleep(int n);
extern void push_black_screen(int n);
extern void refresh_screen(int push);
extern void zero_fb(int x1, int y1, int x2, int y2);
extern void check_xinerama_clip(void);

#endif /* _X11VNC_XINERAMA_H */
