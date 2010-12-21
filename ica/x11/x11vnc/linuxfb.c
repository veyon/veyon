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


void linux_dev_fb_msg(char *);

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
	} else if (strstr(in, "vt") == in) {
		free(in);
		in = (char *) malloc(strlen("console_") + strlen(str) + 1);
		sprintf(in, "console_%s", str);
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

	do_input = 1;
	if (pipeinput_str) {
		have_uinput = 0;
		do_input = 0;
	} else {
		have_uinput = check_uinput();
	}
	if (strstr(in, "console_vt")) {
		have_uinput = 0;
	}

	if (!strcmp(in, "consolex")) {
		do_input = 0;
	} else if (strstr(in, "console_vtx")) {
		have_uinput = 0;
		do_input = 0;
	} else if (!strcmp(in, "console")) {
		/* current active VT: */
		if (! have_uinput) {
			tty = 0;
		}
	} else {
		int n;
		if (sscanf(in, "console%d", &n) == 1)  {
			tty = n;
			have_uinput = 0;
		} else if (sscanf(in, "console_vt%d", &n) == 1)  {
			tty = n;
			have_uinput = 0;
		}
	}
	if (strstr(in, "console_vt") == in) {
		char tmp[100];
		int fd, rows = 30, cols = 80, w, h;
		sprintf(tmp, "/dev/vcsa%d", tty);
		file = strdup(tmp);
		fd = open(file, O_RDWR);
		if (fd >= 0) {
			read(fd, tmp, 4);
			rows = (unsigned char) tmp[0];
			cols = (unsigned char) tmp[1];
			close(fd);
		}
		w = cols * 8;
		h = rows * 16;
		rfbLog("%s %dx%d\n", file, cols, rows);
		if (getenv("RAWFB_VCSA_BPP")) {
			/* 8bpp, etc */
			int bt = atoi(getenv("RAWFB_VCSA_BPP"));
			if (bt > 0 && bt <=32) {
				sprintf(tmp, "%dx%dx%d", w, h, bt);
			} else {
				sprintf(tmp, "%dx%dx16", w, h);
			}
		} else {
			/* default 16bpp */
			sprintf(tmp, "%dx%dx16", w, h);
		}
		atparms = strdup(tmp);
	}
	rfbLog("console_guess: file is %s\n", file);

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
				if (b <= 8 && (rm == gm && gm == bm)) {
					if (b == 4) {
						rm = 0x07;
						gm = 0x38;
						bm = 0xc0;
					}
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
			rfbLogPerror("open");
			linux_dev_fb_msg(file);
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

	q = (char *) malloc(strlen("mmap:") + strlen(file) + 1 + strlen(atparms) + 1);
	if (strstr(in, "console_vt")) {
		sprintf(q, "snap:%s@%s", file, atparms);
	} else {
		sprintf(q, "map:%s@%s", file, atparms);
	}
	free(atparms);
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

