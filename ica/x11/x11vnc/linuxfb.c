/* -- linuxfb.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "xinerama.h"
#include "screen.h"
#include "pointer.h"
#include "allowed_input_t.h"
#include "uinput.h"
#include "keyboard.h"
#include "macosx.h"

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if LIBVNCSERVER_HAVE_LINUX_FB_H
#include <linux/fb.h>
#endif

char *console_guess(char *str, int *fd);
void console_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
void console_pointer_command(int mask, int x, int y, rfbClientPtr client);

char *console_guess(char *str, int *fd) {
	char *q, *in = strdup(str);
	char *atparms = NULL, *file = NULL;
	int do_input, have_uinput, tty = -1;

#ifdef MACOSX
	return macosx_console_guess(str, fd);
#endif


	if (strstr(in, "/dev/fb") == in) {
		free(in);
		in = (char *) malloc(strlen("console:") + strlen(str) + 1);
		sprintf(in, "console:%s", str);
	} else if (strstr(in, "fb") == in) {
		free(in);
		in = (char *) malloc(strlen("console:/dev/") + strlen(str) + 1);
		sprintf(in, "console:/dev/%s", str);
	}

	if (strstr(in, "console") != in) {
		rfbLog("console_guess: unrecognized console/fb format: %s\n", str);
		free(in);
		return NULL;
	}

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
		file = strdup("/dev/fb");
	}
	if (strstr(file, "fb") == file) {
		q = (char *) malloc(strlen("/dev/") + strlen(file) + 1);
		sprintf(q, "/dev/%s", file);
		free(file);
		file = q;
	}
	if (!strcmp(file, "/dev/fb")) {
		/* sometimes no sylink fb -> fb0 */
		struct stat sbuf;
		if (stat(file, &sbuf) != 0) {
			free(file);
			file = strdup("/dev/fb0");
		}
	}
	rfbLog("console_guess: file is %s\n", file);

	do_input = 1;
	if (pipeinput_str) {
		have_uinput = 0;
		do_input = 0;
	} else {
		have_uinput = check_uinput();
	}

	if (!strcmp(in, "consolex")) {
		do_input = 0;
	} else if (!strcmp(in, "console")) {
		/* current active VT: */
		if (! have_uinput) {
			tty = 0;
		}
	} else {
		int n;
		if (sscanf(in, "console%d", &n) != 1)  {
			tty = n;
			have_uinput = 0;
		}
	}

	if (! atparms) {
#if LIBVNCSERVER_HAVE_LINUX_FB_H
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
		struct fb_var_screeninfo var_info;
		int d = open(file, O_RDWR);
		if (d >= 0) {
			int w, h, b;
			unsigned long rm = 0, gm = 0, bm = 0;
			if (ioctl(d, FBIOGET_VSCREENINFO, &var_info) != -1) {
				w = (int) var_info.xres;
				h = (int) var_info.yres;
				b = (int) var_info.bits_per_pixel;

				rm = (1 << var_info.red.length)   - 1;
				gm = (1 << var_info.green.length) - 1;
				bm = (1 << var_info.blue.length)  - 1;
				rm = rm << var_info.red.offset;
				gm = gm << var_info.green.offset;
				bm = bm << var_info.blue.offset;

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
				*fd = d;
			} else {
				perror("ioctl");
				close(d);
			}
		} else {
			rfbLog("could not open: %s\n", file);
			perror("open");
			close(d);
		}
#endif
#endif
	}

	if (atparms) {
		int gw, gh, gb;
		if (sscanf(atparms, "%dx%dx%d", &gw, &gh, &gb) == 3)  {
			fb_x = gw;	
			fb_y = gh;	
			fb_b = gb;	
		}
	}

	if (do_input) {
		if (tty >=0 && tty < 64) {
			pipeinput_str = (char *) malloc(10);
			sprintf(pipeinput_str, "CONSOLE%d", tty);
			rfbLog("console_guess: file pipeinput %s\n",
			    pipeinput_str);
			initialize_pipeinput();
		} else if (have_uinput) {
			pipeinput_str = strdup("UINPUT");
			rfbLog("console_guess: file pipeinput %s\n",
			    pipeinput_str);
			initialize_pipeinput();
		}
	}

	if (! atparms) {
		rfbLog("console_guess: could not get @ parameters.\n");
		return NULL;
	}

	q = (char *) malloc(strlen("map:") + strlen(file) + 1 + strlen(atparms) + 1);
	sprintf(q, "map:%s@%s", file, atparms);
	return q;
}

void console_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	static int control = 0, alt = 0;
	allowed_input_t input;

	if (debug_keyboard) fprintf(stderr, "console_key_command: %d %s\n", (int) keysym, down ? "down" : "up");

	if (pipeinput_cons_fd < 0) {
		return;		
	}
	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		return;
	}

	/* From LinuxVNC.c: */
	if (keysym == XK_Control_L || keysym == XK_Control_R) {
		if (! down) {
			if (control > 0) {
				control--;
			}
		} else {
			control++;
		}
		return;
	}
	if (keysym == XK_Alt_L || keysym == XK_Alt_R) {
		if (! down) {
			if (alt > 0) {
				alt--;
			}
		} else {
			alt++;
		}
		return;
	}
	if (!down) {
		return;
	}
	if (keysym == XK_Escape) {
		keysym = 27;
	}
	if (control) {
		/* shift down to the "control" zone */
		if (keysym >= 'a' && keysym <= 'z') {
			keysym -= ('a' - 1);
		} else if (keysym >= 'A' && keysym <= 'Z') {
			keysym -= ('A' - 1);
		} else {
			keysym = 0xffff;
		}
	} else if (alt) {
		/* shift up to the upper half Latin zone */
		if (keysym >= '!' && keysym <= '~') {
			keysym += 128;
		}
	}
	if (debug_keyboard) fprintf(stderr, "keysym now: %d\n", (int) keysym);
	if (keysym == XK_Tab) {
		keysym = '\t';
	} else if (keysym == XK_Return || keysym == XK_KP_Enter) {
		keysym = '\r';
	} else if (keysym == XK_BackSpace) {
		keysym = 8;
	} else if (keysym == XK_Home || keysym == XK_KP_Home) {
		keysym = 1;
	} else if (keysym == XK_End || keysym == XK_KP_End) {
		keysym = 5;
	} else if (keysym == XK_Up || keysym == XK_KP_Up) {
		keysym = 16;
	} else if (keysym == XK_Down || keysym == XK_KP_Down) {
		keysym = 14;
	} else if (keysym == XK_Right || keysym == XK_KP_Right) {
		keysym = 6;
	} else if (keysym == XK_Next || keysym == XK_KP_Next) {
		keysym = 6;
	} else if (keysym == XK_Left || keysym == XK_KP_Left) {
		keysym = 2;
	} else if (keysym == XK_Prior || keysym == XK_KP_Prior) {
		keysym = 2;
	} else {
		if (keysym >= XK_KP_Multiply && keysym <= XK_KP_Equal) {
			keysym -= 0xFF80;
		}
	}
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCSTI)
	if (keysym < 0x100) {
		if (ioctl(pipeinput_cons_fd, TIOCSTI, &keysym) != -1) {
			return;
		}
		perror("ioctl");
		close(pipeinput_cons_fd);
		pipeinput_cons_fd = -1;
		if (! pipeinput_cons_dev) {
			return;
		}
		pipeinput_cons_fd = open(pipeinput_cons_dev, O_WRONLY);
		if (pipeinput_cons_fd < 0) {
			rfbLog("pipeinput: could not reopen %s\n",
			    pipeinput_cons_dev);
			perror("open");
			return;
		}
		if (ioctl(pipeinput_cons_fd, TIOCSTI, &keysym) == -1) {
			perror("ioctl");
			close(pipeinput_cons_fd);
			pipeinput_cons_fd = -1;
			rfbLog("pipeinput: could not reopen %s\n",
			    pipeinput_cons_dev);
		}
	}
#endif

	if (client) {}
}

void console_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	/* do not forget viewonly perms */
	if (mask || x || y || client) {}
}

