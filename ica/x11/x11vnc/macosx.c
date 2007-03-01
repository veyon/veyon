/* -- macosx.c -- */

#include "rfb/rfbconfig.h"
#if (defined(__MACH__) && defined(__APPLE__) && defined(LIBVNCSERVER_HAVE_MACOSX_NATIVE_DISPLAY))

#define DOMAC 1

#else

#define DOMAC 0

#endif

#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "screen.h"
#include "pointer.h"
#include "allowed_input_t.h"
#include "keyboard.h"
#include "cursor.h"
#include "macosxCG.h"
#include "macosxCGP.h"
#include "macosxCGS.h"

char *macosx_console_guess(char *str, int *fd);
void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client);
char *macosx_get_fb_addr(void);
int macosx_get_cursor(void);
int macosx_get_cursor_pos(int *, int *);
void macosx_send_sel(char *, int);
void macosx_set_sel(char *, int);
int macosx_valid_window(Window, XWindowAttributes*);

Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return);

#if (! DOMAC)

void macosx_event_loop(void) {
	return;
}
char *macosx_console_guess(char *str, int *fd) {
	return NULL;
}
void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	return;
}
void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	return;
}
char *macosx_get_fb_addr(void) {
	return NULL;
}
int macosx_get_cursor(void) {
	return 0;
}
int macosx_get_cursor_pos(int *x, int *y) {
	return 0;
}
void macosx_send_sel(char * str, int len) {
	return;
}
void macosx_set_sel(char * str, int len) {
	return;
}
int macosx_valid_window(Window w, XWindowAttributes* a) {
	return 0;
}
Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return) {
	return (Status) 0;
}


int dragum(void) {return 1;}

#else 

void macosx_event_loop(void) {
	macosxCG_event_loop();
}

char *macosx_get_fb_addr(void) {
	macosxCG_init();
	return macosxCG_get_fb_addr();
}

char *macosx_console_guess(char *str, int *fd) {
	char *q, *in = strdup(str);
	char *atparms = NULL, *file = NULL;

	macosxCG_init();

	if (strstr(in, "console") != in) {
		rfbLog("console_guess: unrecognized console/fb format: %s\n", str);
		free(in);
		return NULL;
	}

	*fd = -1;

	q = strrchr(in, '@');
	if (q) {
		atparms = strdup(q+1);
		*q = '\0';
	}
	q = strrchr(in, ':');
	if (q) {
		file = strdup(q+1);
		*q = '\0';
	}
	if (! file || file[0] == '\0')  {
		file = strdup("/dev/null");
	}
	rfbLog("console_guess: file is %s\n", file);

	if (! pipeinput_str) {
		pipeinput_str = strdup("MACOSX");
		initialize_pipeinput();
	}

	if (! atparms) {
		int w, h, b, bps, dep;
		unsigned long rm = 0, gm = 0, bm = 0;

		w = macosxCG_CGDisplayPixelsWide();
		h = macosxCG_CGDisplayPixelsHigh();
		b = macosxCG_CGDisplayBitsPerPixel();

		bps = macosxCG_CGDisplayBitsPerSample();
		dep = macosxCG_CGDisplaySamplesPerPixel() * bps;

		rm = (1 << bps) - 1;
		gm = (1 << bps) - 1;
		bm = (1 << bps) - 1;
		rm = rm << 2 * bps;
		gm = gm << 1 * bps;
		bm = bm << 0 * bps;

		if (b == 8 && rm == 0xff && gm == 0xff && bm == 0xff) {
			/* I don't believe it... */
			rm = 0x07;
			gm = 0x38;
			bm = 0xc0;
		}
		
		/* @66666x66666x32:0xffffffff:... */
		atparms = (char *) malloc(200);
		sprintf(atparms, "%dx%dx%d:%lx/%lx/%lx",
		    w, h, b, rm, gm, bm);
	}
	if (atparms) {
		int gw, gh, gb;
		if (sscanf(atparms, "%dx%dx%d", &gw, &gh, &gb) == 3)  {
			fb_x = gw;	
			fb_y = gh;	
			fb_b = gb;	
		}
	}
	if (! atparms) {
		rfbLog("console_guess: could not get @ parameters.\n");
		return NULL;
	}

	q = (char *) malloc(strlen("map:macosx:") + strlen(file) + 1 + strlen(atparms) + 1);
	sprintf(q, "map:macosx:%s@%s", file, atparms);
	return q;
}

void macosx_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	allowed_input_t input;
	static int last_mask = 0;
	int rc;

	if (0) fprintf(stderr, "macosx_pointer_command: %d %d - %d\n", x, y, mask);

	if (mask >= 0) {
		got_pointer_calls++;
	}

	if (view_only) {
		return;
	}

	get_allowed_input(client, &input);

	if (! input.motion || ! input.button) {
		/* XXX fix me with last_x, last_y, etc. */
		return;
	}

	if (mask >= 0) {
		got_user_input++;
		got_pointer_input++;
		last_pointer_client = client;
		last_pointer_time = time(NULL);
	}

	macosxCG_pointer_inject(mask, x, y);

	if (cursor_x != x || cursor_y != y) {
		last_pointer_motion_time = dnow();
	}

	cursor_x = x;
	cursor_y = y;

	if (last_mask != mask) {
		last_pointer_click_time = dnow();
	}
	last_mask = mask;

	/* record the x, y position for the rfb screen as well. */
	cursor_position(x, y);

	/* change the cursor shape if necessary */
	rc = set_cursor(x, y, get_which_cursor());
	cursor_changes += rc;

	last_event = last_input = last_pointer_input = time(NULL);
}

void init_key_table(void) {
	macosxCG_init_key_table();
}

void macosx_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	static int control = 0, alt = 0;
	allowed_input_t input;
	if (debug_keyboard) fprintf(stderr, "macosx_key_command: %d %s\n", (int) keysym, down ? "down" : "up");

	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		return;
	}

	init_key_table();
	macosxCG_key_inject((int) down, (unsigned int) keysym);

}

extern void macosxGCS_poll_pb(void);

int macosx_get_cursor_pos(int *x, int *y) {
	macosxCG_get_cursor_pos(x, y);
	if (nofb) {
		/* good time to poll the pasteboard */
		macosxGCS_poll_pb();
	}
	return 1;
}

static char *cuttext = NULL;
static int cutlen = 0;

void macosx_send_sel(char *str, int len) {
	if (screen && all_clients_initialized()) {
		if (cuttext) {
			int n = cutlen;
			if (len < n) {
				n = len;
			}
			if (!memcmp(str, cuttext, (size_t) n)) {
				/* the same text we set pasteboard to ... */
				return;
			}
		}
		if (debug_sel) {
			rfbLog("macosx_send_sel: %d\n", len);
		}
		rfbSendServerCutText(screen, str, len);
	}
}

void macosx_set_sel(char *str, int len) {
	if (screen && all_clients_initialized()) {
		if (cutlen <= len) {
			if (cuttext) {
				free(cuttext);
			}
			cutlen = 2*(len+1);
			cuttext = (char *) calloc(cutlen, 1);
		}
		memcpy(cuttext, str, (size_t) len);
		cuttext[len] = '\0';
		if (debug_sel) {
			rfbLog("macosx_set_sel: %d\n", len);
		}
		macosxGCS_set_pasteboard(str, len);
	}
}

int macosx_get_cursor(void) {
	return macosxCG_get_cursor();
}

typedef struct windat {
	int win;
	int x, y;
	int width, height;
	int level;
} windat_t;

extern int macwinmax; 
extern windat_t macwins[]; 

int macosx_get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win) {
	static int last_idx = -1;
	int x1, x2, y1, y2;
	int idx = -1, i, k;
	macosxCGS_get_all_windows();
	macosxCG_get_cursor_pos(px, py);

	for (i = -1; i<macwinmax; i++) {
		k = i;
		if (i == -1)  {
			if (last_idx >= 0 && last_idx < macwinmax) {
				k = last_idx;
			} else {
				last_idx = -1;
				continue;
			}
		}
		x1 = macwins[k].x;
		x2 = macwins[k].x + macwins[k].width;
		y1 = macwins[k].y;
		y2 = macwins[k].y + macwins[k].height;
if (debug_wireframe) fprintf(stderr, "%d/%d:	%d %d %d  - %d %d %d\n", k, macwins[k].win, x1, *px, x2, y1, *py, y2);
		if (x1 <= *px && *px < x2) {
			if (y1 <= *py && *py < y2) {
				idx = k;
				break;
			}
		}
	}
	if (idx < 0) {
		return 0;
	}

	*x = macwins[idx].x;
	*y = macwins[idx].y;
	*w = macwins[idx].width;
	*h = macwins[idx].height;
	*frame = (Window) macwins[idx].win;
	if (win != NULL) {
		*win = *frame;
	}

	last_idx = idx;

	return 1;
}

int macosx_valid_window(Window w, XWindowAttributes* a) {
	static int last_idx = -1;
	int win = (int) w;
	int i, k, idx = -1;

	for (i = -1; i<macwinmax; i++) {
		k = i;
		if (i == -1)  {
			if (last_idx >= 0 && last_idx < macwinmax) {
				k = last_idx;
			} else {
				last_idx = -1;
				continue;
			}
		}
		if (macwins[k].win == win) {
			idx = k;
			break;
		}
	}
	if (idx < 0) {
		return 0;
	}

	a->x = macwins[idx].x;
	a->y = macwins[idx].y;
	a->width  = macwins[idx].width;
	a->height = macwins[idx].height;
	a->depth = depth;
	a->border_width = 0;
	a->backing_store = 0;
	a->map_state = IsViewable;

	last_idx = idx;
	
	return 1;
}

#define QTMAX 2048
static Window cret[QTMAX];

extern int CGS_levelmax;
extern int CGS_levels[];

Status macosx_xquerytree(Window w, Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return) {

	int i, n, k, swap;
	int win1, win2;

	*root_return = (Window) 0;
	*parent_return = (Window) 0;

	macosxCGS_get_all_windows();

	n = 0;
	for (k = CGS_levelmax - 1; k >= 0; k--) {
		for (i = macwinmax - 1; i >= 0; i--) {
			if (macwins[i].level == CGS_levels[k]) {
if (0) fprintf(stderr, "k=%d i=%d n=%d\n", k, i, n);
				cret[n++] = (Window) macwins[i].win;
			}
		}
	}
	*children_return = cret;
	*nchildren_return = (unsigned int) macwinmax;

	return (Status) 1;
}

#endif 	/* LIBVNCSERVER_HAVE_MACOSX_NATIVE_DISPLAY */

