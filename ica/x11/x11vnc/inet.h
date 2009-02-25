#ifndef _X11VNC_INET_H
#define _X11VNC_INET_H

/* -- inet.h -- */

extern char *host2ip(char *host);
extern char *raw2host(char *raw, int len);
extern char *raw2ip(char *raw);
extern char *ip2host(char *ip);
extern int dotted_ip(char *host);
extern int get_remote_port(int sock);
extern int get_local_port(int sock);
extern char *get_remote_host(int sock);
extern char *get_local_host(int sock);
extern char *ident_username(rfbClientPtr client);
extern int find_free_port(int start, int end);
extern int have_ssh_env(void);

#endif /* _X11VNC_INET_H */
