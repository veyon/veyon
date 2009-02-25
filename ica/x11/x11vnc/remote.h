#ifndef _X11VNC_REMOTE_H
#define _X11VNC_REMOTE_H

/* -- remote.h -- */

extern int send_remote_cmd(char *cmd, int query, int wait);
extern int do_remote_query(char *remote_cmd, char *query_cmd, int remote_sync,
    int qdefault);
extern void check_black_fb(void);
extern int check_httpdir(void);
extern void http_connections(int on);
extern int remote_control_access_ok(void);
extern char *process_remote_cmd(char *cmd, int stringonly);

#endif /* _X11VNC_REMOTE_H */
