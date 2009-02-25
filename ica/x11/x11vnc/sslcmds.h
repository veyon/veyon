#ifndef _X11VNC_SSLCMDS_H
#define _X11VNC_SSLCMDS_H

/* -- sslcmds.h -- */

extern void check_stunnel(void);
extern int start_stunnel(int stunnel_port, int x11vnc_port);
extern void stop_stunnel(void);
extern void setup_stunnel(int rport, int *argc, char **argv);
extern char *get_Cert_dir(char *cdir_in, char **tmp_in);
extern void sslGenCA(char *cdir);
extern void sslGenCert(char *ty, char *nm);
extern void sslEncKey(char *path, int info_only);


#endif /* _X11VNC_SSLCMDS_H */
