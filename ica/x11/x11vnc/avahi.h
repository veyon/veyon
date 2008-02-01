#ifndef _X11VNC_AVAHI_H
#define _X11VNC_AVAHI_H

/* -- avahi.h -- */

extern void avahi_initialise(void);
extern void avahi_advertise(const char *name, const char *host, const uint16_t port);
extern void avahi_reset(void);
extern void avahi_cleanup(void);

#endif /* _X11VNC_AVAHI_H */
