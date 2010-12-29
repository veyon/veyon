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

#ifndef _X11VNC_OPTIONS_H
#define _X11VNC_OPTIONS_H

/* -- options.h -- */

/* 
 * variables for the command line options
 */
extern int debug;

extern char *use_dpy;
extern int display_N;
extern int auto_port;
extern char *auth_file;
extern char *visual_str;
extern int set_visual_str_to_something;
extern char *logfile;
extern int logfile_append;
extern char *flagfile;
extern char *rm_flagfile;
extern char *passwdfile;
extern int unixpw;
extern int unixpw_nis;
extern char *unixpw_list;
extern char *unixpw_cmd;
extern int unixpw_system_greeter;
extern int unixpw_system_greeter_active;
extern int use_stunnel;
extern int stunnel_port;
extern char *stunnel_pem;
extern int use_openssl;
extern int http_ssl;
extern int ssl_no_fail;
extern char *openssl_pem;
extern char *ssl_certs_dir;
extern char *enc_str;
extern int vencrypt_mode;
extern int vencrypt_kx;
extern int vencrypt_enable_plain_login;
extern int anontls_mode;
extern int create_fresh_dhparams;
extern char *dhparams_file;
extern int http_try_it;
extern int stunnel_http_port;
extern int https_port_num;
extern int https_port_redir;
extern char *ssl_verify;
extern char *ssl_crl;
extern int ssl_initialized;
extern int ssl_timeout_secs;
extern char *ssh_str;
extern pid_t ssh_pid;
extern int usepw;
extern char *blackout_str;
extern int blackout_ptr;
extern char *clip_str;
extern int use_solid_bg;
extern char *solid_str;
extern char *solid_default;

extern char *wmdt_str;

extern char *speeds_str;
extern char *rc_rcfile;
extern int rc_rcfile_default;
extern int rc_norc;
extern int got_norc;
extern int opts_bg;

extern int shared;
extern int connect_once;
extern int got_connect_once;
extern int got_findauth;
extern int deny_all;
extern int accept_remote_cmds;
extern char *remote_prefix;
extern int remote_direct;
extern int query_default;
extern int safe_remote_only;
extern int priv_remote;
extern int more_safe;
extern int no_external_cmds;
extern char *allowed_external_cmds;
extern int started_as_root;
extern int host_lookup;
extern char *unix_sock;
extern int unix_sock_fd;
extern int ipv6_listen;
extern int got_ipv6_listen;
extern int ipv6_listen_fd;
extern int ipv6_http_fd;
extern int noipv6;
extern int noipv4;
extern char *ipv6_client_ip_str;
extern char *users_list;
extern char **user2group;
extern char *allow_list;
extern char *listen_str;
extern char *listen_str6;
extern char *allow_once;
extern char *accept_cmd;
extern char *afteraccept_cmd;
extern char *gone_cmd;
extern int view_only;
extern char *allowed_input_view_only;
extern char *allowed_input_normal;
extern char *allowed_input_str;
extern char *viewonly_passwd;
extern char **passwd_list;
extern int begin_viewonly;
extern int inetd;
extern int tightfilexfer; 
extern int got_ultrafilexfer; 
extern int first_conn_timeout;
extern int ping_interval;
extern int flash_cmap;
extern int shift_cmap;
extern int force_indexed_color;
extern int advertise_truecolor;
extern int advertise_truecolor_reset;
extern int cmap8to24;
extern char *cmap8to24_str;
extern int xform24to32;
extern int launch_gui;

extern int avahi;
extern int vnc_redirect;
extern int vnc_redirect_sock;

extern int use_modifier_tweak;
extern int watch_capslock;
extern int skip_lockkeys;
extern int use_iso_level3;
extern int clear_mods;
extern int nofb;
extern char *raw_fb_str;
extern char *raw_fb_pixfmt;
extern char *raw_fb_full_str;
extern char *freqtab;
extern char *pipeinput_str;
extern char *pipeinput_opts;
extern FILE *pipeinput_fh;
extern int pipeinput_tee;
extern int pipeinput_int;
extern int pipeinput_cons_fd;
extern char *pipeinput_cons_dev;

extern int macosx_nodimming;
extern int macosx_nosleep;
extern int macosx_noscreensaver;
extern int macosx_wait_for_switch;
extern int macosx_mouse_wheel_speed;
extern int macosx_console;
extern int macosx_swap23;
extern int macosx_resize;
extern int macosx_icon_anim_time;
extern int macosx_no_opengl;
extern int macosx_no_rawfb;
extern int macosx_read_opengl;
extern int macosx_read_rawfb;

extern unsigned long subwin;
extern int subwin_wait_mapped;
extern int freeze_when_obscured;
extern int subwin_obscured;

extern int debug_xevents;
extern int debug_xdamage;
extern int debug_wireframe;
extern int debug_tiles;
extern int debug_grabs;
extern int debug_sel;

extern int xtrap_input;
extern int xinerama;
extern int xrandr;
extern int xrandr_maybe;
extern char *xrandr_mode;
extern char *pad_geometry;
extern time_t pad_geometry_time;
extern int use_snapfb;

extern int use_xrecord;
extern int noxrecord;

extern char *client_connect;
extern char *client_connect_file;
extern int connect_or_exit;
extern int vnc_connect;
extern char *connect_proxy;

extern int show_cursor;
extern int show_multiple_cursors;
extern char *multiple_cursors_mode;
extern int cursor_drag_changes;
extern int cursor_pos_updates;
extern int cursor_shape_updates;
extern int use_xwarppointer;
extern int always_inject;
extern int show_dragging;
extern int wireframe;
extern int wireframe_local;

extern char *wireframe_str;
extern char *wireframe_copyrect;
extern char *wireframe_copyrect_default;
extern int wireframe_in_progress;

extern int ncache;
extern int ncache0;
extern int ncache_default;
extern int ncache_copyrect;
extern int ncache_wf_raises;
extern int ncache_dt_change;
extern int ncache_pad;
extern int ncache_xrootpmap;
extern int ncache_keep_anims;
extern int ncache_old_wm;
extern int macosx_ncache_macmenu;
extern int macosx_us_kbd;
extern int ncache_beta_tester;
extern int ncdb;

extern Atom atom_NET_ACTIVE_WINDOW;
extern Atom atom_NET_CURRENT_DESKTOP;
extern Atom atom_NET_CLIENT_LIST_STACKING;
extern Atom atom_XROOTPMAP_ID;
extern double got_NET_ACTIVE_WINDOW;
extern double got_NET_CURRENT_DESKTOP;
extern double got_NET_CLIENT_LIST_STACKING;
extern double got_XROOTPMAP_ID;

extern char *scroll_copyrect_str;
extern char *scroll_copyrect;
extern char *scroll_copyrect_default;
extern char *scroll_key_list_str;
extern KeySym *scroll_key_list;

extern int scaling_copyrect0;
extern int scaling_copyrect;

extern int scrollcopyrect_min_area;
extern int debug_scroll;
extern double pointer_flush_delay;
extern double last_scroll_event;
extern int max_scroll_keyrate;
extern double max_keyrepeat_time;
extern char *max_keyrepeat_str;
extern char *max_keyrepeat_str0;
extern int max_keyrepeat_lo, max_keyrepeat_hi;

extern char **scroll_good_all;
extern char **scroll_good_key;
extern char **scroll_good_mouse;
extern char *scroll_good_str;
extern char *scroll_good_str0;

extern char **scroll_skip_all;
extern char **scroll_skip_key;
extern char **scroll_skip_mouse;
extern char *scroll_skip_str;
extern char *scroll_skip_str0;

extern char **scroll_term;
extern char *scroll_term_str;
extern char *scroll_term_str0;

extern char* screen_fixup_str;
extern double screen_fixup_V;
extern double screen_fixup_C;
extern double screen_fixup_X;
extern double screen_fixup_8;

extern int no_autorepeat;
extern int no_repeat_countdown;
extern int watch_bell;
extern int sound_bell;
extern int xkbcompat;
extern int use_xkb_modtweak;
extern int skip_duplicate_key_events;
extern char *skip_keycodes;
extern int sloppy_keys;
extern int add_keysyms;

extern char *remap_file;
extern char *pointer_remap;
extern int pointer_mode;
extern int pointer_mode_max;	
extern int single_copytile;
extern int single_copytile_orig;
extern int single_copytile_count;
extern int tile_shm_count;

extern int using_shm;
extern int flip_byte_order;
extern int waitms;
extern int got_waitms;
extern double wait_ui;
extern double slow_fb;
extern double xrefresh;
extern int wait_bog;
extern int extra_fbur;
extern int defer_update;
extern int set_defer;
extern int got_defer;
extern int got_deferupdate;

extern int screen_blank;

extern int no_fbu_blank;
extern int take_naps;
extern int naptile;
extern int napfac;
extern int napmax;
extern int ui_skip;
extern int all_input;
extern int handle_events_eagerly;

extern int watch_fbpm;
extern int watch_dpms;
extern int force_dpms;
extern int client_dpms;
extern int no_ultra_dpms;
extern int no_ultra_ext;
extern int saw_ultra_chat;
extern int saw_ultra_file;
extern int chat_window;
extern rfbClientPtr chat_window_client;

extern int watch_selection;
extern int watch_primary;
extern int watch_clipboard;
extern char *sel_direction;

extern char *sigpipe;

extern VisualID visual_id;
extern int visual_depth;

extern int overlay;
extern int overlay_cursor;

extern double fs_frac;
extern int tile_fuzz;

extern int grow_fill;
extern int gaps_fill;

extern int debug_pointer;
extern int debug_keyboard;

extern int quiet;
extern int verbose;

extern int use_threads;
extern int started_rfbRunEventLoop;
extern int threads_drop_input;

extern int got_noxwarppointer;
extern int got_rfbport;
extern int got_rfbport_val;
extern int got_alwaysshared;
extern int got_nevershared;
extern int got_cursorpos;
extern int got_pointer_mode;
extern int got_noviewonly;
extern int got_wirecopyrect;
extern int got_scrollcopyrect;
extern int got_noxkb;
extern int got_nomodtweak;

#endif /* _X11VNC_OPTIONS_H */
