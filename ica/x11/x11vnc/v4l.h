#ifndef _X11VNC_V4L_H
#define _X11VNC_V4L_H

/* -- v4l.h -- */
extern char *v4l_guess(char *str, int *fd);
extern void v4l_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
extern void v4l_pointer_command(int mask, int x, int y, rfbClientPtr client);


#endif /* _X11VNC_V4L_H */
