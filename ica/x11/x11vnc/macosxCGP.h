#ifndef _X11VNC_MACOSXCGP_H
#define _X11VNC_MACOSXCGP_H

/* -- macosxCGP.h -- */

extern int macosxCGP_save_dim(void);
extern int macosxCGP_restore_dim(void);
extern int macosxCGP_save_sleep(void);
extern int macosxCGP_restore_sleep(void);
extern int macosxCGP_init_dimming(void);
extern int macosxCGP_undim(void);
extern int macosxCGP_dim_shutdown(void);
extern void macosxCGP_screensaver_timer_off(void);
extern void macosxCGP_screensaver_timer_on(void);


#endif /* _X11VNC_MACOSXCGP_H */
