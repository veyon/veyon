/* -- x11vnc_defs.c -- */

#include "x11vnc.h"

int overlay_present = 0;

int xrandr_base_event_type = 0;

int xfixes_base_event_type = 0;
int xtest_base_event_type = 0;
#if LIBVNCSERVER_HAVE_LIBXTRAP
XETC *trap_ctx = NULL;
#endif
int xtrap_base_event_type = 0;
int xdamage_base_event_type = 0;

/*               date +'lastmod: %Y-%m-%d' */
char lastmod[] = "0.8.3 lastmod: 2006-11-06";

/* X display info */

Display *dpy = NULL;		/* the single display screen we connect to */
int scr = 0;
char *xauth_raw_data = NULL;
int xauth_raw_len = 0;
Window window = None, rootwin = None;	/* polled window, root window (usu. same) */
Visual *default_visual = NULL;	/* the default visual (unless -visual) */
int bpp = 0, depth = 0;
int indexed_color = 0;
int dpy_x = 0, dpy_y = 0;		/* size of display */
int fb_x = 0, fb_y = 0, fb_b = 0;	/* fb size and bpp guesses at display */
int off_x, off_y;		/* offsets for -sid */
int wdpy_x, wdpy_y;		/* for actual sizes in case of -clip */
int cdpy_x, cdpy_y, coff_x, coff_y;	/* the -clip params */
int button_mask = 0;		/* button state and info */
int button_mask_prev = 0;
int num_buttons = -1;

/* image structures */
XImage *scanline = NULL;
XImage *fullscreen = NULL;
XImage **tile_row = NULL;	/* for all possible row runs */
XImage *snaprect = NULL;	/* for XShmGetImage (fs_factor) */
XImage *snap = NULL;		/* the full snap fb */
XImage *raw_fb_image = NULL;	/* the raw fb */

/* corresponding shm structures */
XShmSegmentInfo scanline_shm;
XShmSegmentInfo fullscreen_shm;
XShmSegmentInfo *tile_row_shm;	/* for all possible row runs */
XShmSegmentInfo snaprect_shm;

/* rfb screen info */
rfbScreenInfoPtr screen = NULL;
char *rfb_desktop_name = NULL;
char *http_dir = NULL;
char vnc_desktop_name[256];
char *main_fb = NULL;		/* our copy of the X11 fb */
char *rfb_fb = NULL;		/* same as main_fb unless transformation */
char *fake_fb = NULL;		/* used under -padgeom */
char *snap_fb = NULL;		/* used under -snapfb */
char *cmap8to24_fb = NULL;	/* used under -8to24 */
char *rot_fb = NULL;
char *raw_fb = NULL;		/* when used should be main_fb */
char *raw_fb_addr = NULL;
int raw_fb_offset = 0;
int raw_fb_shm = 0;
int raw_fb_mmap = 0;
int raw_fb_seek = 0;
int raw_fb_fd = -1;
int raw_fb_back_to_X = 0;	/* kludge for testing rawfb -> X */

int rfb_bytes_per_line = 0;
int main_bytes_per_line = 0;
int rot_bytes_per_line = 0;
unsigned long  main_red_mask = 0,  main_green_mask = 0,  main_blue_mask = 0;
unsigned short main_red_max = 0,   main_green_max = 0,   main_blue_max = 0;
unsigned short main_red_shift = 0, main_green_shift = 0, main_blue_shift = 0;

/* scaling parameters */
char *scale_str = NULL;
double scale_fac = 1.0;
int scaling = 0;
int scaling_blend = 1;		/* for no blending option (very course) */
int scaling_nomult4 = 0;	/* do not require width = n * 4 */
int scaling_pad = 0;		/* pad out scaled sizes to fit denominator */
int scaling_interpolate = 0;	/* use interpolation scheme when shrinking */
int scaled_x = 0, scaled_y = 0;	/* dimensions of scaled display */
int scale_numer = 0, scale_denom = 0;	/* n/m */
char *rotating_str = NULL;
int rotating = 0;
int rotating_same = 0;
int rotating_cursors = 0;

/* scale cursor */
char *scale_cursor_str = NULL;
double scale_cursor_fac = 1.0;
int scaling_cursor = 0;
int scaling_cursor_blend = 1;
int scaling_cursor_interpolate = 0;
int scale_cursor_numer = 0, scale_cursor_denom = 0;

/* size of the basic tile unit that is polled for changes: */
int tile_x = 32;
int tile_y = 32;
int ntiles, ntiles_x, ntiles_y;

/* arrays that indicate changed or checked tiles. */
unsigned char *tile_has_diff = NULL, *tile_tried = NULL, *tile_copied = NULL;
unsigned char *tile_has_xdamage_diff = NULL, *tile_row_has_xdamage_diff = NULL;

/* times of recent events */
time_t last_event, last_input = 0, last_client = 0;
time_t last_keyboard_input = 0, last_pointer_input = 0; 
time_t last_fb_bytes_sent = 0;
double last_keyboard_time = 0.0;
double last_pointer_time = 0.0;
double last_pointer_click_time = 0.0;
double last_pointer_motion_time = 0.0;
double last_key_to_button_remap_time = 0.0;
double last_copyrect = 0.0;
double last_copyrect_fix = 0.0;
double servertime_diff = 0.0;
double x11vnc_start = 0.0;

int hack_val = 0;

/* last client to move pointer */
rfbClientPtr last_pointer_client = NULL;
rfbClientPtr latest_client = NULL;

int client_count = 0;
int clients_served = 0;

/* more transient kludge variables: */
int cursor_x = 0, cursor_y = 0;		/* x and y from the viewer(s) */
int button_change_x = 0, button_change_y = 0;
int got_user_input = 0;
int got_pointer_input = 0;
int got_pointer_calls = 0;
int got_keyboard_input = 0;
int got_keyboard_calls = 0;
int urgent_update = 0;
int last_keyboard_keycode = 0;
rfbBool last_rfb_down = FALSE;
rfbBool last_rfb_key_accepted = FALSE;
rfbKeySym last_rfb_keysym = 0;
double last_rfb_keytime = 0.0;
int fb_copy_in_progress = 0;	
int drag_in_progress = 0;	
int shut_down = 0;	
int do_copy_screen = 0;	
time_t damage_time = 0;
int damage_delay = 0;

int program_pid = 0;
char *program_name = NULL;
char *program_cmdline = NULL;

struct utsname UT;


