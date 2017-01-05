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

/* -- options.c -- */

#define _X11VNC_OPTIONS_H
#include "x11vnc.h"

/* 
 * variables for the command line options
 */
int debug = 0;

char *use_dpy = NULL;		/* -display */
int display_N = 0;
int auto_port = 0;
char *auth_file = NULL;		/* -auth/-xauth */
char *visual_str = NULL;	/* -visual */
int set_visual_str_to_something = 0;
char *logfile = NULL;		/* -o, -logfile */
int logfile_append = 0;
char *flagfile = NULL;		/* -flag */
char *rm_flagfile = NULL;	/* -rmflag */
char *passwdfile = NULL;	/* -passwdfile */
int unixpw = 0;			/* -unixpw */
int unixpw_nis = 0;		/* -unixpw_nis */
char *unixpw_list = NULL;
char *unixpw_cmd = NULL;
int unixpw_system_greeter = 0;
int unixpw_system_greeter_active = 0;
int use_stunnel = 0;		/* -stunnel */
int stunnel_port = 0;
char *stunnel_pem = NULL;
int use_openssl = 0;
int http_ssl = 0;
int ssl_no_fail = 0;
char *openssl_pem = NULL;
char *ssl_certs_dir = NULL;
char *enc_str = NULL;
int vencrypt_mode = VENCRYPT_SUPPORT;
int vencrypt_kx = VENCRYPT_BOTH;
int vencrypt_enable_plain_login = 0;
int anontls_mode = ANONTLS_SUPPORT;
int create_fresh_dhparams = 0;
char *dhparams_file = NULL;
int http_try_it = 0;
int stunnel_http_port = 0;
int https_port_num = -1;
int https_port_redir = 0;
char *ssl_verify = NULL;
char *ssl_crl = NULL;
int ssl_initialized = 0;
int ssl_timeout_secs = -1;
char *ssh_str = NULL;
pid_t ssh_pid = 0;
int usepw = USEPW;
char *blackout_str = NULL;	/* -blackout */
int blackout_ptr = 0;
char *clip_str = NULL;		/* -clip */
int use_solid_bg = 0;		/* -solid */
char *solid_str = NULL;
char *solid_default = "cyan4";

char *wmdt_str = NULL;		/* -wmdt */

char *speeds_str = NULL;	/* -speeds */

char *rc_rcfile = NULL;		/* -rc */
int rc_rcfile_default = 0;
int rc_norc = 0;
int got_norc = 0;
int opts_bg = 0;

#ifndef VNCSHARED
int shared = 0;			/* share vnc display. */
#else
int shared = 1;
#endif
#ifndef FOREVER
int connect_once = 1;		/* disconnect after first connection session. */
#else
int connect_once = 0;
#endif
int got_connect_once = 0;
int got_findauth = 0;
int deny_all = 0;		/* global locking of new clients */
#ifndef REMOTE_DEFAULT
#define REMOTE_DEFAULT 1
#endif
int accept_remote_cmds = REMOTE_DEFAULT;	/* -noremote */
char *remote_prefix = NULL;
int remote_direct = 0;
int query_default = 0;
int safe_remote_only = 1;	/* -unsafe */
int priv_remote = 0;		/* -privremote */
int more_safe = 0;		/* -safer */
#ifndef EXTERNAL_COMMANDS
#define EXTERNAL_COMMANDS 1
#endif
#if EXTERNAL_COMMANDS
int no_external_cmds = 0;	/* -nocmds */
#else
int no_external_cmds = 1;	/* cannot be turned back on. */
#endif
char *allowed_external_cmds = NULL;
int started_as_root = 0;
int host_lookup = 1;
char *unix_sock = NULL;
int unix_sock_fd = -1;
#if X11VNC_LISTEN6
int ipv6_listen = 1;		/* -6 / -no6 */
int got_ipv6_listen = 1;
#else
int ipv6_listen = 0;		/* -6 / -no6 */
int got_ipv6_listen = 0;
#endif
int ipv6_listen_fd = -1;
int ipv6_http_fd = -1;
int noipv6 = 0;
int noipv4 = 0;
char *ipv6_client_ip_str = NULL;
char *users_list = NULL;	/* -users */
char **user2group = NULL;
char *allow_list = NULL;	/* for -allow and -localhost */
char *listen_str = NULL;
char *listen_str6 = NULL;
char *allow_once = NULL;	/* one time -allow */
char *accept_cmd = NULL;	/* for -accept */
char *afteraccept_cmd = NULL;	/* for -afteraccept */
char *gone_cmd = NULL;		/* for -gone */
#ifndef VIEWONLY
#define VIEWONLY 0
#endif
int view_only = VIEWONLY;		/* clients can only watch. */
char *allowed_input_view_only = NULL;
char *allowed_input_normal = NULL;
char *allowed_input_str = NULL;
char *viewonly_passwd = NULL;	/* view only passwd. */
char **passwd_list = NULL;	/* for -passwdfile */
int begin_viewonly = -1;
int inetd = 0;			/* spawned from inetd(8) */
#ifndef TIGHTFILEXFER
#define TIGHTFILEXFER 0
#endif
int tightfilexfer = TIGHTFILEXFER; 
int got_ultrafilexfer = 0; 
int first_conn_timeout = 0;	/* -timeout */
int ping_interval = 0;		/* -ping */
int flash_cmap = 0;		/* follow installed colormaps */
int shift_cmap = 0;		/* ncells < 256 and needs shift of pixel values */
int force_indexed_color = 0;	/* whether to force indexed color for 8bpp */
int advertise_truecolor = 0;
int advertise_truecolor_reset = 0;
int cmap8to24 = 0;		/* -8to24 */
int xform24to32 = 0;		/* -24to32 */
char *cmap8to24_str = NULL;
int launch_gui = 0;		/* -gui */

#ifndef AVAHI
#define AVAHI 0
#endif
int avahi = AVAHI;		/* -avahi, -mdns */
int vnc_redirect = 0;
int vnc_redirect_sock = -1;

int use_modifier_tweak = 1;	/* use the shift/altgr modifier tweak */
int watch_capslock = 0;		/* -capslock */
int skip_lockkeys = 0;		/* -skip_lockkeys */
int use_iso_level3 = 0;		/* ISO_Level3_Shift instead of Mode_switch */
int clear_mods = 0;		/* -clear_mods (1) and -clear_keys (2) -clear_locks (3) */
int nofb = 0;			/* do not send any fb updates */
char *raw_fb_str = NULL;	/* used under -rawfb */
char *raw_fb_pixfmt = NULL;
char *raw_fb_full_str = NULL;
char *freqtab = NULL;
char *pipeinput_str = NULL;	/* -pipeinput [tee,reopen,keycodes:]cmd */
char *pipeinput_opts = NULL;
FILE *pipeinput_fh = NULL;
int pipeinput_tee = 0;
int pipeinput_int = 0;
int pipeinput_cons_fd = -1;
char *pipeinput_cons_dev = NULL;

int macosx_nodimming = 0;	/* Some native MacOSX server settings. */
int macosx_nosleep = 0;
int macosx_noscreensaver = 0;
int macosx_wait_for_switch = 1;
int macosx_mouse_wheel_speed = 5;
int macosx_console = 0;
int macosx_swap23 = 1;
int macosx_resize = 1;
int macosx_icon_anim_time = 450;
int macosx_no_opengl = 0;
int macosx_no_rawfb = 0;
int macosx_read_opengl = 0;
int macosx_read_rawfb = 0;

unsigned long subwin = 0x0;	/* -id, -sid */
int subwin_wait_mapped = 0;
int freeze_when_obscured = 0;
int subwin_obscured = 0;

int debug_xevents = 0;		/* -R debug_xevents:1 */
int debug_xdamage = 0;		/* -R debug_xdamage:1 or 2 ... */
int debug_wireframe = 0;
int debug_tiles = 0;
int debug_grabs = 0;
int debug_sel = 0;

int xtrap_input = 0;		/* -xtrap for user input insertion */
int xinerama = XINERAMA;	/* -xinerama */
int xrandr = 0;			/* -xrandr */
int xrandr_maybe = 1;		/* check for events, but don't trap all calls */
char *xrandr_mode = NULL;
char *pad_geometry = NULL;
time_t pad_geometry_time = 0;
int use_snapfb = 0;

int use_xrecord = 0;
int noxrecord = 0;

char *client_connect = NULL;	/* strings for -connect option */
char *client_connect_file = NULL;
int connect_or_exit = 0;
int vnc_connect = 1;		/* -vncconnect option */
char *connect_proxy = NULL;

int show_cursor = 1;		/* show cursor shapes */
int show_multiple_cursors = 0;	/* show X when on root background, etc */
char *multiple_cursors_mode = NULL;
#ifndef CURSOR_DRAG
#define CURSOR_DRAG 0
#endif
int cursor_drag_changes = CURSOR_DRAG;
int cursor_pos_updates = 1;	/* cursor position updates -cursorpos */
int cursor_shape_updates = 1;	/* cursor shape updates -nocursorshape */
int use_xwarppointer = 0;	/* use XWarpPointer instead of XTestFake... */
int always_inject = 0;		/* inject new mouse coordinates even if dx=dy=0 */
int show_dragging = 1;		/* process mouse movement events */
#ifndef WIREFRAME
#define WIREFRAME 1
#endif
int wireframe = WIREFRAME;	/* try to emulate wireframe wm moves */
/* shade,linewidth,percent,T+B+L+R,t1+t2+t3+t4 */
char *wireframe_str = NULL;
char *wireframe_copyrect = NULL;
#ifndef WIREFRAME_COPYRECT
#define WIREFRAME_COPYRECT 1
#endif
#if WIREFRAME_COPYRECT
char *wireframe_copyrect_default = "always";
#else
char *wireframe_copyrect_default = "never";
#endif
int wireframe_in_progress = 0;
int wireframe_local = 1;

#ifndef NCACHE
#ifdef NO_NCACHE
#define NCACHE 0 
#else
#define xxNCACHE -12
#define NCACHE -1
#endif
#endif

#ifdef MACOSX
int ncache = 0;
int ncache_pad = 24;
#else
int ncache = NCACHE;
int ncache_pad = 0;
#endif

#ifndef NCACHE_XROOTPMAP
#define NCACHE_XROOTPMAP 1
#endif
int ncache_xrootpmap = NCACHE_XROOTPMAP;
int ncache0 = 0;
int ncache_default = 10;
int ncache_copyrect = 0;
int ncache_wf_raises = 1;
int ncache_dt_change = 1;
int ncache_keep_anims = 0;
int ncache_old_wm = 0;
int macosx_ncache_macmenu = 0;
int macosx_us_kbd = 0;
int ncache_beta_tester = 0;
int ncdb = 0;

Atom atom_NET_ACTIVE_WINDOW = None;
Atom atom_NET_CURRENT_DESKTOP = None;
Atom atom_NET_CLIENT_LIST_STACKING = None;
Atom atom_XROOTPMAP_ID = None;
double got_NET_ACTIVE_WINDOW = 0.0;
double got_NET_CURRENT_DESKTOP = 0.0;
double got_NET_CLIENT_LIST_STACKING = 0.0;
double got_XROOTPMAP_ID = 0.0;

/* T+B+L+R,tkey+presist_key,tmouse+persist_mouse */
char *scroll_copyrect_str = NULL;
#ifndef SCROLL_COPYRECT
#define SCROLL_COPYRECT 1
#endif
char *scroll_copyrect = NULL;
#if SCROLL_COPYRECT
#if 1
char *scroll_copyrect_default = "always";	/* -scrollcopyrect */
#else
char *scroll_copyrect_default = "keys";
#endif
#else
char *scroll_copyrect_default = "never";
#endif
char *scroll_key_list_str = NULL;
KeySym *scroll_key_list = NULL;

#ifndef SCALING_COPYRECT
#define SCALING_COPYRECT 1
#endif
int scaling_copyrect0 = SCALING_COPYRECT;
int scaling_copyrect  = SCALING_COPYRECT;

int scrollcopyrect_min_area = 60000;	/* minimum rectangle area */
int debug_scroll = 0;
double pointer_flush_delay = 0.0;
double last_scroll_event = 0.0;
int max_scroll_keyrate = 0;
double max_keyrepeat_time = 0.0;
char *max_keyrepeat_str = NULL;
char *max_keyrepeat_str0 = "4-20";
int max_keyrepeat_lo = 1, max_keyrepeat_hi = 40;

char **scroll_good_all = NULL;
char **scroll_good_key = NULL;
char **scroll_good_mouse = NULL;
char *scroll_good_str = NULL;
char *scroll_good_str0 = "##Nomatch";
/*	"##Firefox-bin," */
/*	"##Gnome-terminal," */
/*	"##XTerm", */

char **scroll_skip_all = NULL;
char **scroll_skip_key = NULL;
char **scroll_skip_mouse = NULL;
char *scroll_skip_str = NULL;
char *scroll_skip_str0 = "##Soffice.bin,##StarOffice,##OpenOffice";
/*	"##Konsole,"	 * no problems, known heuristics do not work */

char **scroll_term = NULL;
char *scroll_term_str = NULL;
char *scroll_term_str0 = "term";

char* screen_fixup_str = NULL;
double screen_fixup_V = 0.0;
double screen_fixup_C = 0.0;
double screen_fixup_X = 0.0;
double screen_fixup_8 = 0.0;

#ifndef NOREPEAT
#define NOREPEAT 1
#endif
int no_autorepeat = NOREPEAT;	/* turn off autorepeat with clients */
int no_repeat_countdown = 2;
int watch_bell = 1;		/* watch for the bell using XKEYBOARD */
int sound_bell = 1;		/* actually send it */
int xkbcompat = 0;		/* ignore XKEYBOARD extension */
int use_xkb_modtweak = 0;	/* -xkb */
#ifndef SKIPDUPS
#define SKIPDUPS 0
#endif
int skip_duplicate_key_events = SKIPDUPS;
char *skip_keycodes = NULL;
int sloppy_keys = 0;
#ifndef ADDKEYSYMS
#define ADDKEYSYMS 1
#endif
int add_keysyms = ADDKEYSYMS;	/* automatically add keysyms to X server */

char *remap_file = NULL;	/* -remap */
char *pointer_remap = NULL;
/* use the various ways of updating pointer */
#ifndef POINTER_MODE_DEFAULT
#define POINTER_MODE_DEFAULT 2
#endif
int pointer_mode = POINTER_MODE_DEFAULT;
int pointer_mode_max = 4;	
int single_copytile = 0;	/* use the old way copy_tiles() */
int single_copytile_orig = 0;
int single_copytile_count = 0;
int tile_shm_count = 0;

int using_shm = 1;		/* whether mit-shm is used */
int flip_byte_order = 0;	/* sometimes needed when using_shm = 0 */
/*
 * waitms is the msec to wait between screen polls.  Not too old h/w shows
 * poll times of 10-35ms, so maybe this value cuts the idle load by 2 or so.
 */
int waitms = 20;
int got_waitms = 0;
double wait_ui = 2.0;
double slow_fb = 0.0;
double xrefresh = 0.0;
int wait_bog = 1;
int extra_fbur = 1;
int defer_update = 20;	/* deferUpdateTime ms to wait before sends. */
int set_defer = 1;
int got_defer = 0;
int got_deferupdate = 0;

int screen_blank = 60;	/* number of seconds of no activity to throttle */
			/* down the screen polls.  zero to disable. */
int no_fbu_blank = 30;	/* nap if no client updates in this many secs. */
int take_naps = 1;	/* -nap/-nonap */
int naptile = 4;	/* tile change threshold per poll to take a nap */
int napfac = 4;		/* time = napfac*waitms, cut load with extra waits */
int napmax = 1500;	/* longest nap in ms. */
int ui_skip = 10;	/* see watchloop.  negative means ignore input */
int all_input = 0;
int handle_events_eagerly = 0;


#if LIBVNCSERVER_HAVE_FBPM
int watch_fbpm = 1;	/* -nofbpm */
#else
int watch_fbpm = 0;
#endif

int watch_dpms = 0;	/* -dpms */
int force_dpms = 0;
int client_dpms = 0;
int no_ultra_dpms = 0;
int no_ultra_ext = 0;
int saw_ultra_chat = 0;
int saw_ultra_file = 0;
int chat_window = 0;
rfbClientPtr chat_window_client = NULL;

int watch_selection = 1;	/* normal selection/cutbuffer maintenance */
int watch_primary = 1;		/* more dicey, poll for changes in PRIMARY */
int watch_clipboard = 1;
char *sel_direction = NULL;	/* "send" or "recv" for one-way */

char *sigpipe = NULL;		/* skip, ignore, exit */

/* visual stuff for -visual override or -overlay */
VisualID visual_id = (VisualID) 0;
int visual_depth = 0;

/* for -overlay mode on Solaris/IRIX.  X server draws cursor correctly.  */
int overlay = 0;
int overlay_cursor = 1;

/* tile heuristics: */
double fs_frac = 0.75;	/* threshold tile fraction to do fullscreen updates. */
int tile_fuzz = 2;	/* tolerance for suspecting changed tiles touching */
			/* a known changed tile. */
int grow_fill = 3;	/* do the grow islands heuristic with this width. */
int gaps_fill = 4;	/* do a final pass to try to fill gaps between tiles. */

int debug_pointer = 0;
int debug_keyboard = 0;

int quiet = 0;
int verbose = 0;

/* threaded vs. non-threaded (default) */
int use_threads = 0;
int started_rfbRunEventLoop = 0;
int threads_drop_input = 0;

/* info about command line opts */
int got_noxwarppointer = 0;
int got_rfbport = 0;
int got_rfbport_val = -1;
int got_alwaysshared = 0;
int got_nevershared = 0;
int got_cursorpos = 0;
int got_pointer_mode = -1;
int got_noviewonly = 0;
int got_wirecopyrect = 0;
int got_scrollcopyrect = 0;
int got_noxkb = 0;
int got_nomodtweak = 0;

