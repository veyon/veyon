#ifndef _X11VNC_USER_H
#define _X11VNC_USER_H

/* -- user.h -- */

extern void check_switched_user(void);
extern void lurk_loop(char *str);
extern int switch_user(char *, int);
extern int read_passwds(char *passfile);
extern void install_passwds(void);
extern void check_new_passwds(int force);
extern int wait_for_client(int *argc, char** argv, int http);
extern rfbBool custom_passwd_check(rfbClientPtr cl, const char *response, int len);
extern char *xdmcp_insert;

#endif /* _X11VNC_USER_H */
