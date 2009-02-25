#ifndef _X11VNC_XKB_BELL_H
#define _X11VNC_XKB_BELL_H

/* -- xkb_bell.h -- */

extern int xkb_base_event_type;

extern void initialize_xkb(void);
extern void initialize_watch_bell(void);
extern void check_bell_event(void);

#endif /* _X11VNC_XKB_BELL_H */
