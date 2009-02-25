#ifndef _X11VNC_XRECORD_H
#define _X11VNC_XRECORD_H

/* -- xrecord.h -- */
#include "scrollevent_t.h"
#include "winattr_t.h"

extern scroll_event_t scr_ev[];
extern int scr_ev_cnt;
extern int xrecording;
extern int xrecord_set_by_keys;
extern int xrecord_set_by_mouse;
extern Window xrecord_focus_window;
extern Window xrecord_wm_window;
extern Window xrecord_ptr_window;
extern KeySym xrecord_keysym;
extern char xrecord_name_info[];

extern winattr_t scr_attr_cache[];

extern Display *rdpy_data;
extern Display *rdpy_ctrl;

extern Display *gdpy_ctrl;
extern int xserver_grabbed;

extern void initialize_xrecord(void);
extern void shutdown_xrecord(void);
extern int xrecord_skip_keysym(rfbKeySym keysym);
extern int xrecord_skip_button(int new, int old);
extern int xrecord_scroll_keysym(rfbKeySym keysym);
extern void check_xrecord_reset(int force);
extern void xrecord_watch(int start, int setby);

#endif /* _X11VNC_XRECORD_H */
