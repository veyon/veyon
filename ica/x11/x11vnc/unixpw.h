#ifndef _X11VNC_UNIXPW_H
#define _X11VNC_UNIXPW_H

/* -- unixpw.h -- */

extern void unixpw_screen(int init);
extern void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init);
extern void unixpw_accept(char *user);
extern void unixpw_deny(void);
extern void unixpw_msg(char *msg, int delay);
extern int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size);
extern int crypt_verify(char *user, char *pass);

extern int unixpw_in_progress;
extern int unixpw_denied;
extern int unixpw_in_rfbPE;
extern int unixpw_login_viewonly;
extern time_t unixpw_last_try_time;
extern rfbClientPtr unixpw_client;
extern int keep_unixpw;
extern char *keep_unixpw_user;
extern char *keep_unixpw_pass;
extern char *keep_unixpw_opts;

#endif /* _X11VNC_UNIXPW_H */
