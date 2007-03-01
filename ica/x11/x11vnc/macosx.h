#ifndef _X11VNC_MACOSX_H
#define _X11VNC_MACOSX_H

/* -- macosx.h -- */

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
extern void macosx_send_sel(char *, int);
extern void macosx_set_sel(char *, int);



#endif /* _X11VNC_MACOSX_H */
