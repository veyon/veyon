/* -- options.c -- */

#define _X11VNC_OPTIONS_H
#include "x11vnc.h"

/* 
 * variables for the command line options
 */
int debug = 0;

char *use_dpy = NULL;		/* -display */
char *auth_file = NULL;		/* -auth/-xauth */
char *visual_str = NULL;	/* -visual */
int set_visual_str_to_something = 0;
char *logfile = NULL;		/* -o, -logfile */
int logfile_append = 0;
char *flagfile = NULL;		/* -flag */
char *passwdfile = NULL;	/* -passwdfile */
int unixpw = 0;			/* -unixpw */
int unixpw_nis = 0;		/* -unixpw_nis */
char *unixpw_list = NULL;
char *unixpw_cmd = NULL;
int use_stunnel = 0;		/* -stunnel */
int stunnel_port = 0;
char *stunnel_pem = NULL;
int use_openssl = 0;
int http_ssl = 0;
int ssl_no_fail = 0;
char *openssl_pem = NULL;
char *ssl_certs_dir = NULL;
int https_port_num = -1;
char *ssl_verify = NULL;
int ssl_initialized = 0;
int ssl_timeout_secs = -1;
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
int deny_all = 0;		/* global locking of new clients */
#ifndef REMOTE_DEFAULT
#define REMOTE_DEFAULT 1
#endif
int accept_remote_cmds = REMOTE_DEFAULT;	/* -noremote */
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
char *users_list = NULL;	/* -users */
char *allow_list = NULL;	/* for -allow and -localhost */
char *listen_str = NULL;
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
#ifndef FILEXFER
#define FILEXFER 1
#endif
int filexfer = FILEXFER; 
int first_conn_timeout = 0;	/* -timeout */
int flash_cmap = 0;		/* follow installed colormaps */
int shift_cmap = 0;		/* ncells < 256 and needs shift of pixel values */
int force_indexed_color = 0;	/* whether to force indexed color for 8bpp */
int cmap8to24 = 0;		/* -8to24 */
int xform24to32 = 0;		/* -24to32 */
char *cmap8to24_str = NULL;
int launch_gui = 0;		/* -gui */

int use_modifier_tweak = 1;	/* use the shift/altgr modifier tweak */
int watch_capslock = 0;		/* -capslock */
int skip_lockkeys = 0;		/* -skip_lockkeys */
int use_iso_level3 = 0;		/* ISO_Level3_Shift instead of Mode_switch */
int clear_mods = 0;		/* -clear_mods (1) and -clear_keys (2) */
int nofb = 0;			/* do not send any fb updates */
char *raw_fb_str = NULL;	/* used under -rawfb */
char *raw_fb_pixfmt = NULL;
char *freqtab = NULL;
char *pipeinput_str = NULL;	/* -pipeinput [tee,reopen,keycodes:]cmd */
char *pipeinput_opts = NULL;
FILE *pipeinput_fh = NULL;
int pipeinput_tee = 0;
int pipeinput_int = 0;
int pipeinput_cons_fd = -1;
char *pipeinput_cons_dev = NULL;

unsigned long subwin = 0x0;	/* -id, -sid */
int subwin_wait_mapped = 0;

int debug_xevents = 0;		/* -R debug_xevents:1 */
int debug_xdamage = 0;		/* -R debug_xdamage:1 or 2 ... */
int debug_wireframe = 0;
int debug_tiles = 0;
int debug_grabs = 0;
int debug_sel = 0;

int xtrap_input = 0;		/* -xtrap for user input insertion */
int xinerama = XINERAMA;	/* -xinerama */
int xrandr = 0;			/* -xrandr */
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
char *scroll_skip_str0 = "##Soffice.bin,##StarOffice";
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
int waitms = 30;
double wait_ui = 2.0;
double slow_fb = 0.0;
int wait_bog = 1;
int defer_update = 30;	/* deferUpdateTime ms to wait before sends. */
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


#if LIBVNCSERVER_HAVE_FBPM
int watch_fbpm = 1;	/* -nofbpm */
#else
int watch_fbpm = 0;
#endif

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
#if LIBVNCSERVER_HAVE_LIBPTHREAD && defined(X11VNC_THREADED)
int use_threads = 1;
#else
int use_threads = 0;
#endif

/* info about command line opts */
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

