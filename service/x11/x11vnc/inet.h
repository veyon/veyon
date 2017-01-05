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

#ifndef _X11VNC_INET_H
#define _X11VNC_INET_H

/* -- inet.h -- */

extern char *host2ip(char *host);
extern char *raw2host(char *raw, int len);
extern char *raw2ip(char *raw);
extern char *ip2host(char *ip);
extern int ipv6_ip(char *host);
extern int dotted_ip(char *host, int partial);
extern int get_remote_port(int sock);
extern int get_local_port(int sock);
extern char *get_remote_host(int sock);
extern char *get_local_host(int sock);
extern char *ident_username(rfbClientPtr client);
extern int find_free_port(int start, int end);
extern int find_free_port6(int start, int end);
extern int have_ssh_env(void);
extern char *ipv6_getnameinfo(struct sockaddr *paddr, int addrlen);
extern char *ipv6_getipaddr(struct sockaddr *paddr, int addrlen);
extern int listen6(int port);
extern int listen_unix(char *file);
extern int accept_unix(int s);
extern int connect_tcp(char *host, int port);
extern int listen_tcp(int port, in_addr_t iface, int try6);

#endif /* _X11VNC_INET_H */
