#ifndef _X11VNC_MACOSX_H
#define _X11VNC_MACOSX_H

/* -- macosx.h -- */

extern void macosx_log(char *);
extern char *macosx_console_guess(char *str, int *fd);
extern char *macosx_get_fb_addr(void);
extern void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
extern void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client);
extern void macosx_event_loop(void);
extern int macosx_get_cursor(void);
extern int macosx_get_cursor_pos(int *, int *);
extern int macosx_valid_window(Window, XWindowAttributes*);
extern Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return);
extern int macosx_get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win);
extern void macosx_send_sel(char *, int);
extern void macosx_set_sel(char *, int);

extern void macosx_add_mapnotify(Window win, int level, int map);
extern void macosx_add_create(Window win, int level);
extern void macosx_add_destroy(Window win, int level);
extern void macosx_add_visnotify(Window win, int level, int obscured);
extern int macosx_checkevent(XEvent *ev);

extern Window macosx_click_frame;


#endif /* _X11VNC_MACOSX_H */
