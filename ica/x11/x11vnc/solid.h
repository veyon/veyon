#ifndef _X11VNC_SOLID_H
#define _X11VNC_SOLID_H

/* -- solid.h -- */

extern char *guess_desktop(void);
extern unsigned long get_pixel(char *color);
extern XImage *solid_image(char *color);
extern void solid_bg(int restore);
extern XImage *solid_root(char *color);
extern void kde_no_animate(int restore);
extern void gnome_no_animate(void);

#endif /* _X11VNC_SOLID_H */
