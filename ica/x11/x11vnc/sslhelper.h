#ifndef _X11VNC_SSLHELPER_H
#define _X11VNC_SSLHELPER_H

/* -- sslhelper.h -- */


#define OPENSSL_INETD   1
#define OPENSSL_VNC     2
#define OPENSSL_HTTPS   3
#define OPENSSL_REVERSE 4

extern int openssl_sock;
extern int openssl_port_num;
extern int https_sock;
extern pid_t openssl_last_helper_pid;
extern char *openssl_last_ip;
extern char *certret_str;
extern char *dhret_str;
extern char *new_dh_params;

extern void raw_xfer(int csock, int s_in, int s_out);

extern int openssl_present(void);
extern void openssl_init(int);
extern void openssl_port(void);
extern void https_port(void);
extern void check_openssl(void);
extern void check_https(void);
extern void ssl_helper_pid(pid_t pid, int sock);
extern void accept_openssl(int mode, int presock);
extern char *find_openssl_bin(void);
extern char *get_saved_pem(char *string, int create);


#endif /* _X11VNC_SSLHELPER_H */
