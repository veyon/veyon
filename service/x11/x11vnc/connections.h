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

#ifndef _X11VNC_CONNECTIONS_H
#define _X11VNC_CONNECTIONS_H

/* -- connections.h -- */

extern char vnc_connect_str[];
extern Atom vnc_connect_prop;
extern char x11vnc_remote_str[];
extern Atom x11vnc_remote_prop;
extern rfbClientPtr inetd_client;


extern int all_clients_initialized(void);
extern char *list_clients(void);
extern int new_fb_size_clients(rfbScreenInfoPtr s);
extern void close_all_clients(void);
extern void close_clients(char *str);
extern void set_client_input(char *str);
extern void set_child_info(void);
extern int cmd_ok(char *cmd);
extern void client_gone(rfbClientPtr client);
extern void client_gone_chat_helper(rfbClientPtr client);
extern void reverse_connect(char *str);
extern void set_vnc_connect_prop(char *str);
extern void read_vnc_connect_prop(int);
extern void set_x11vnc_remote_prop(char *str);
extern void read_x11vnc_remote_prop(int);
extern void check_connect_inputs(void);
extern void check_gui_inputs(void);
extern rfbClientPtr create_new_client(int sock, int start_thread);
extern enum rfbNewClientAction new_client(rfbClientPtr client);
extern enum rfbNewClientAction new_client_chat_helper(rfbClientPtr client);
extern rfbBool password_check_chat_helper(rfbClientPtr cl, const char* response, int len);
extern void start_client_info_sock(char *host_port_cookie);
extern void send_client_info(char *str);
extern void adjust_grabs(int grab, int quiet);
extern void check_new_clients(void);
extern int accept_client(rfbClientPtr client);
extern void check_ipv6_listen(long usec);
extern void check_unix_sock(long usec);
extern int run_user_command(char *cmd, rfbClientPtr client, char *mode, char *input,
    int len, FILE *output);
extern int check_access(char *addr);
extern void client_set_net(rfbClientPtr client);
extern char *get_xprop(char *prop, Window win);
extern int set_xprop(char *prop, Window win, char *value);
extern char *bcx_xattach(char *str, int *pg_init, int *kg_init);
extern void grab_state(int *ptr_grabbed, int *kbd_grabbed);
extern char *wininfo(Window win, int show_children);

#endif /* _X11VNC_CONNECTIONS_H */
