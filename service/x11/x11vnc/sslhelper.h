/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

#ifndef _X11VNC_SSLHELPER_H
#define _X11VNC_SSLHELPER_H

/* -- sslhelper.h -- */


#define OPENSSL_INETD   1
#define OPENSSL_VNC     2
#define OPENSSL_VNC6    3
#define OPENSSL_HTTPS   4
#define OPENSSL_HTTPS6  5
#define OPENSSL_REVERSE 6

extern int openssl_sock;
extern int openssl_sock6;
extern int openssl_port_num;
extern int https_sock;
extern int https_sock6;
extern pid_t openssl_last_helper_pid;
extern char *openssl_last_ip;
extern char *certret_str;
extern char *dhret_str;
extern char *new_dh_params;

extern void raw_xfer(int csock, int s_in, int s_out);

extern int openssl_present(void);
extern void openssl_init(int);
extern void openssl_port(int);
extern void https_port(int);
extern void check_openssl(void);
extern void check_https(void);
extern void ssl_helper_pid(pid_t pid, int sock);
extern void accept_openssl(int mode, int presock);
extern char *find_openssl_bin(void);
extern char *get_saved_pem(char *string, int create);
extern char *get_ssl_verify_file(char *str_in);
extern char *create_tmp_pem(char *path, int prompt);


#endif /* _X11VNC_SSLHELPER_H */
