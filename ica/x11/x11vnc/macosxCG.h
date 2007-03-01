#ifndef _X11VNC_MACOSXCG_H
#define _X11VNC_MACOSXCG_H

/* -- macosxCG.h -- */

extern void macosxCG_init(void);
extern void macosxCG_event_loop(void);
extern char *macosxCG_get_fb_addr(void);

extern int macosxCG_CGDisplayPixelsWide(void);
extern int macosxCG_CGDisplayPixelsHigh(void);
extern int macosxCG_CGDisplayBitsPerPixel(void);
extern int macosxCG_CGDisplayBitsPerSample(void);
extern int macosxCG_CGDisplaySamplesPerPixel(void);

extern void macosxCG_pointer_inject(int mask, int x, int y);
extern int macosxCG_get_cursor_pos(int *x, int *y);
extern int macosxCG_get_cursor(void);
extern void macosxCG_init_key_table(void);
extern void macosxCG_key_inject(int down, unsigned int keysym);


#endif /* _X11VNC_MACOSXCG_H */
