#ifndef _X11VNC_CLEANUP_H
#define _X11VNC_CLEANUP_H

/* -- cleanup.h -- */

extern int trapped_xerror;
extern int trapped_xioerror;
extern int trapped_getimage_xerror;
extern int trapped_record_xerror;
extern XErrorEvent *trapped_xerror_event;

extern int crash_debug;

extern void clean_shm(int quick);
extern void clean_up_exit (int ret);

extern int trap_xerror(Display *d, XErrorEvent *error);
extern int trap_xioerror(Display *d);
extern int trap_getimage_xerror(Display *d, XErrorEvent *error);
extern char *xerror_string(XErrorEvent *error);
extern void initialize_crash_handler(void);
extern void initialize_signals(void);
extern void unset_signals(void);
extern int known_sigpipe_mode(char *s);

#endif /* _X11VNC_CLEANUP_H */
