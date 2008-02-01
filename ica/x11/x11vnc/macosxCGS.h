#ifndef _X11VNC_MACOSXCGS_H
#define _X11VNC_MACOSXCGS_H

/* -- macosxCGS.h -- */

extern void macosxCGS_get_all_windows(void);
extern int macosxCGS_get_qlook(int);
extern void macosxGCS_set_pasteboard(char *str, int len);
extern int macosxCGS_follow_animation_win(int win, int idx, int grow);
extern int macosxCGS_find_index(int w);


#endif /* _X11VNC_MACOSXCGS_H */
