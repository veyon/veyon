#ifndef _X11VNC_8TO24_H
#define _X11VNC_8TO24_H

/* -- 8to24.h -- */

extern int multivis_count;
extern int multivis_24count;

extern void check_for_multivis(void);
extern void bpp8to24(int, int, int, int);
extern void mark_8bpp(int);

#endif /* _X11VNC_8TO24_H */
