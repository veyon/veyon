#ifndef _X11VNC_LINUXFB_H
#define _X11VNC_LINUXFB_H

/* -- linuxfb.h -- */
extern char *console_guess(char *str, int *fd);
extern void console_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
extern void console_pointer_command(int mask, int x, int y, rfbClientPtr client);


#endif /* _X11VNC_LINUXFB_H */
