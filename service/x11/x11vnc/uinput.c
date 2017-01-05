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

/* -- uinput.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "xinerama.h"
#include "screen.h"
#include "pointer.h"
#include "keyboard.h"
#include "allowed_input_t.h"

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
#if LIBVNCSERVER_HAVE_LINUX_INPUT_H
#if LIBVNCSERVER_HAVE_LINUX_UINPUT_H
#define UINPUT_OK
#endif
#endif
#endif

#ifdef UINPUT_OK
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#if !defined(EV_SYN) || !defined(SYN_REPORT)
#undef UINPUT_OK
#endif

#endif


int check_uinput(void);
int initialize_uinput(void);
void shutdown_uinput(void);
int set_uinput_accel(char *str);
int set_uinput_thresh(char *str);
void set_uinput_reset(int ms);
void set_uinput_always(int);
void set_uinput_touchscreen(int);
void set_uinput_abs(int);
char *get_uinput_accel();
char *get_uinput_thresh();
int get_uinput_reset();
int get_uinput_always();
int get_uinput_touchscreen();
int get_uinput_abs();
void parse_uinput_str(char *str);
void uinput_pointer_command(int mask, int x, int y, rfbClientPtr client);
void uinput_key_command(int down, int keysym, rfbClientPtr client);

static void init_key_tracker(void);
static int mod_is_down(void);
static int key_is_down(void);
static void set_uinput_accel_xy(double fx, double fy);
static void ptr_move(int dx, int dy);
static void ptr_rel(int dx, int dy);
static void button_click(int down, int btn);
static int lookup_code(int keysym);

static int fd = -1;
static int direct_rel_fd = -1;
static int direct_abs_fd = -1;
static int direct_btn_fd = -1;
static int direct_key_fd = -1;
static int bmask = 0;
static int db = 0;

static char *injectable = NULL;
static char *uinput_dev = NULL;
static char *tslib_cal = NULL;
static double a[7];
static int uinput_touchscreen = 0;
static int uinput_abs = 0;
static int btn_touch = 0;
static int dragskip = 0;
static int touch_always = 0;
static int touch_pressure = 1;
static int abs_x = 0, abs_y = 0;

static char *devs[] = {
	"/dev/misc/uinput",
	"/dev/input/uinput",
	"/dev/uinput",
	NULL
};

#ifndef O_NDELAY
#ifdef  O_NONBLOCK
#define O_NDELAY O_NONBLOCK
#else
#define O_NDELAY 0
#endif
#endif

/* 
 * User may need to do:
 	modprode uinput
	mknod /dev/input/uinput c 10 223
 */

int check_uinput(void) {
#ifndef UINPUT_OK
	return 0;
#else
	int i;
	if (UT.release) {
		int maj, min;
		/* guard against linux 2.4 */
		if (sscanf(UT.release, "%d.%d.", &maj, &min) == 2) {
			if (maj < 2) {
				return 0;
			} else if (maj == 2) {
				/* hmmm IPAQ 2.4.19-rmk6-pxa1-hh37 works... */
#if 0
				if (min < 6) {
					return 0;
				}
#endif
			}
		}
	}
	fd = -1;
	i = 0;
	while (devs[i] != NULL) {
		if ( (fd = open(devs[i++], O_WRONLY | O_NDELAY)) >= 0) {
			break;
		}
	}
	if (fd < 0) {
		return 0;
	}
	close(fd);
	fd = -1;
	return 1;
#endif
}

static int key_pressed[256];
static int key_ismod[256];

static void init_key_tracker(void) {
	int i;
	for (i = 0; i < 256; i++) {
		key_pressed[i] = 0;
		key_ismod[i] = 0;
	}
	i = lookup_code(XK_Shift_L);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Shift_R);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Control_L);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Control_R);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Alt_L);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Alt_R);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Meta_L);	if (0<=i && i<256) key_ismod[i] = 1;
	i = lookup_code(XK_Meta_R);	if (0<=i && i<256) key_ismod[i] = 1;
}

static int mod_is_down(void) {
	int i;
	if (0) {key_is_down();}
	for (i = 0; i < 256; i++) {
		if (key_pressed[i] && key_ismod[i]) {
			return 1;
		}
	}
	return 0;
}

static int key_is_down(void) {
	int i;
	for (i = 0; i < 256; i++) {
		if (key_pressed[i]) {
			return 1;
		}
	}
	return 0;
}

void shutdown_uinput(void) {
#ifdef UINPUT_OK
	if (fd >= 0) {
		if (db) {
			rfbLog("shutdown_uinput called on fd=%d\n", fd);
		}
		ioctl(fd, UI_DEV_DESTROY);
		close(fd);
		fd = -1;
	}

	/* close direct injection files too: */
	if (direct_rel_fd >= 0) close(direct_rel_fd);
	if (direct_abs_fd >= 0) close(direct_abs_fd);
	if (direct_btn_fd >= 0) close(direct_btn_fd);
	if (direct_key_fd >= 0) close(direct_key_fd);
	direct_rel_fd = -1;
	direct_abs_fd = -1;
	direct_btn_fd = -1;
	direct_key_fd = -1;
#endif
}

/* 
grep BUS_ /usr/include/linux/input.h | awk '{print $2}' | perl -e 'while (<>) {chomp; print "#ifdef $_\n\t\tif(!strcmp(s, \"$_\"))\tudev.id.bustype = $_\n#endif\n"}'
 */
static int get_bustype(char *s) {
#ifdef UINPUT_OK

	if (!s) return 0;

#ifdef BUS_PCI
	if(!strcmp(s, "BUS_PCI"))	return BUS_PCI;
#endif
#ifdef BUS_ISAPNP
	if(!strcmp(s, "BUS_ISAPNP"))	return BUS_ISAPNP;
#endif
#ifdef BUS_USB
	if(!strcmp(s, "BUS_USB"))	return BUS_USB;
#endif
#ifdef BUS_HIL
	if(!strcmp(s, "BUS_HIL"))	return BUS_HIL;
#endif
#ifdef BUS_BLUETOOTH
	if(!strcmp(s, "BUS_BLUETOOTH"))	return BUS_BLUETOOTH;
#endif
#ifdef BUS_VIRTUAL
	if(!strcmp(s, "BUS_VIRTUAL"))	return BUS_VIRTUAL;
#endif
#ifdef BUS_ISA
	if(!strcmp(s, "BUS_ISA"))	return BUS_ISA;
#endif
#ifdef BUS_I8042
	if(!strcmp(s, "BUS_I8042"))	return BUS_I8042;
#endif
#ifdef BUS_XTKBD
	if(!strcmp(s, "BUS_XTKBD"))	return BUS_XTKBD;
#endif
#ifdef BUS_RS232
	if(!strcmp(s, "BUS_RS232"))	return BUS_RS232;
#endif
#ifdef BUS_GAMEPORT
	if(!strcmp(s, "BUS_GAMEPORT"))	return BUS_GAMEPORT;
#endif
#ifdef BUS_PARPORT
	if(!strcmp(s, "BUS_PARPORT"))	return BUS_PARPORT;
#endif
#ifdef BUS_AMIGA
	if(!strcmp(s, "BUS_AMIGA"))	return BUS_AMIGA;
#endif
#ifdef BUS_ADB
	if(!strcmp(s, "BUS_ADB"))	return BUS_ADB;
#endif
#ifdef BUS_I2C
	if(!strcmp(s, "BUS_I2C"))	return BUS_I2C;
#endif
#ifdef BUS_HOST
	if(!strcmp(s, "BUS_HOST"))	return BUS_HOST;
#endif
#ifdef BUS_GSC
	if(!strcmp(s, "BUS_GSC"))	return BUS_GSC;
#endif
#ifdef BUS_ATARI
	if(!strcmp(s, "BUS_ATARI"))	return BUS_ATARI;
#endif
	if (atoi(s) > 0) {
		return atoi(s);
	}

#endif
	return 0;
}

static void load_tslib_cal(void) {
	FILE *f;
	char line[1024], *p;
	int i;

	/* /etc/pointercal -528 33408 -3417516 -44200 408 40292028 56541 */

	/* this is the identity transformation: */
	a[0] = 1.0;
	a[1] = 0.0;
	a[2] = 0.0;
	a[3] = 0.0;
	a[4] = 1.0;
	a[5] = 0.0;
	a[6] = 1.0;

	if (tslib_cal == NULL) {
		return;
	}

	rfbLog("load_tslib_cal: reading %s\n", tslib_cal);
	f = fopen(tslib_cal, "r");
	if (f == NULL) {
		rfbLogPerror("load_tslib_cal: fopen");
		clean_up_exit(1);
	}

	if (fgets(line, sizeof(line), f) == NULL) {
		rfbLogPerror("load_tslib_cal: fgets");
		clean_up_exit(1);
	}
	fclose(f);

	p = strtok(line, " \t");
	i = 0;
	while (p) {
		a[i] = (double) atoi(p);
		rfbLog("load_tslib_cal: a[%d] %.3f\n", i, a[i]);
		p = strtok(NULL, " \t");
		i++;
		if (i >= 7) {
			break;
		}
	}
	if (i != 7) {
		rfbLog("load_tslib_cal: invalid tslib file format: i=%d %s\n",
		    i, tslib_cal);
		clean_up_exit(1);
	}
}


int initialize_uinput(void) {
#ifndef UINPUT_OK
	return 0;
#else
	int i;
	char *s;
	struct uinput_user_dev udev;

	if (fd >= 0) {
		shutdown_uinput();
	}
	fd = -1;

	if (getenv("X11VNC_UINPUT_DEBUG")) {
		db = atoi(getenv("X11VNC_UINPUT_DEBUG"));
		rfbLog("set uinput debug to: %d\n", db);
	}

	if (tslib_cal) {
		load_tslib_cal();	
	}

	init_key_tracker();
	
	if (uinput_dev) {
		if (!strcmp(uinput_dev, "nouinput")) {
			rfbLog("initialize_uinput: not creating uinput device.\n");
			return 1;
		} else {
			fd = open(uinput_dev, O_WRONLY | O_NDELAY);
			rfbLog("initialize_uinput: using: %s %d\n", uinput_dev, fd);
		}
	} else {
		i = 0;
		while (devs[i] != NULL) {
			if ( (fd = open(devs[i], O_WRONLY | O_NDELAY)) >= 0) {
				rfbLog("initialize_uinput: using: %s %d\n",
				    devs[i], fd);
				break;
			}
			i++;
		}
	}
	if (fd < 0) {
		rfbLog("initialize_uinput: could not open an uinput device.\n");
		rfbLogPerror("open");
		if (direct_rel_fd < 0 && direct_abs_fd < 0 && direct_btn_fd < 0 && direct_key_fd < 0) {
			clean_up_exit(1);
		}
		return 1;
	}

	memset(&udev, 0, sizeof(udev));

	strncpy(udev.name, "x11vnc injector", UINPUT_MAX_NAME_SIZE);

	s = getenv("X11VNC_UINPUT_BUS");
	if (s) {
		udev.id.bustype = get_bustype(s);
	} else if (0) {
		udev.id.bustype = BUS_USB;
	}

	s = getenv("X11VNC_UINPUT_VERSION");
	if (s) {
		udev.id.version = atoi(s);
	} else if (0) {
		udev.id.version = 4;
	}

	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);

	ioctl(fd, UI_SET_EVBIT, EV_KEY);

	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for (i=0; i < 256; i++) {
		ioctl(fd, UI_SET_KEYBIT, i);
	}

	ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(fd, UI_SET_KEYBIT, BTN_FORWARD);
	ioctl(fd, UI_SET_KEYBIT, BTN_BACK);

	if (uinput_touchscreen) {
		ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
		rfbLog("uinput: touchscreen enabled.\n");
	}
	if (uinput_touchscreen || uinput_abs) {
		int gw = abs_x, gh = abs_y;
		if (! gw || ! gh) {
			gw = fb_x; gh = fb_y;
		}
		if (! gw || ! gh) {
			gw = dpy_x; gh = dpy_y;
		}
		abs_x = gw;
		abs_y = gh;
		ioctl(fd, UI_SET_EVBIT, EV_ABS);
		ioctl(fd, UI_SET_ABSBIT, ABS_X);
		ioctl(fd, UI_SET_ABSBIT, ABS_Y);
		udev.absmin[ABS_X] = 0;
		udev.absmax[ABS_X] = gw;
		udev.absfuzz[ABS_X] = 0;
		udev.absflat[ABS_X] = 0;
		udev.absmin[ABS_Y] = 0;
		udev.absmax[ABS_Y] = gh;
		udev.absfuzz[ABS_Y] = 0;
		udev.absflat[ABS_Y] = 0;
		rfbLog("uinput: absolute pointer enabled at %dx%d.\n", abs_x, abs_y);
		set_uinput_accel_xy(1.0, 1.0);
	}

	if (db) {
		rfbLog("   udev.name:             %s\n", udev.name);
		rfbLog("   udev.id.bustype:       %d\n", udev.id.bustype);
		rfbLog("   udev.id.vendor:        %d\n", udev.id.vendor);
		rfbLog("   udev.id.product:       %d\n", udev.id.product);
		rfbLog("   udev.id.version:       %d\n", udev.id.version);
		rfbLog("   udev.ff_effects_max:   %d\n", udev.ff_effects_max);
		rfbLog("   udev.absmin[ABS_X]:    %d\n", udev.absmin[ABS_X]);
		rfbLog("   udev.absmax[ABS_X]:    %d\n", udev.absmax[ABS_X]);
		rfbLog("   udev.absfuzz[ABS_X]:   %d\n", udev.absfuzz[ABS_X]);
		rfbLog("   udev.absflat[ABS_X]:   %d\n", udev.absflat[ABS_X]);
		rfbLog("   udev.absmin[ABS_Y]:    %d\n", udev.absmin[ABS_Y]);
		rfbLog("   udev.absmax[ABS_Y]:    %d\n", udev.absmax[ABS_Y]);
		rfbLog("   udev.absfuzz[ABS_Y]:   %d\n", udev.absfuzz[ABS_Y]);
		rfbLog("   udev.absflat[ABS_Y]:   %d\n", udev.absflat[ABS_Y]);
	}

	write(fd, &udev, sizeof(udev));

	if (ioctl(fd, UI_DEV_CREATE) != 0) {
		rfbLog("ioctl(fd, UI_DEV_CREATE) failed.\n");
		rfbLogPerror("ioctl");
		close(fd);
		clean_up_exit(1);
	}
	return 1;
#endif
}

/* these defaults are based on qt-embedded 7/2006 */
static double fudge_x = 0.5;	/* accel=2.0 */
static double fudge_y = 0.5;

static int thresh = 5;
static int thresh_or = 1;

static double resid_x = 0.0;
static double resid_y = 0.0;

static double zero_delay = 0.15;
static double last_button_click = 0.0;

static int uinput_always = 0;

static void set_uinput_accel_xy(double fx, double fy) {
	fudge_x = 1.0/fx;
	fudge_y = 1.0/fy;
	rfbLog("set_uinput_accel:  fx=%.5f fy=%.5f\n", fx, fy);
	rfbLog("set_uinput_accel:  ix=%.5f iy=%.5f\n", fudge_x, fudge_y);
}

static char *uinput_accel_str = NULL;
static char *uinput_thresh_str = NULL;

int set_uinput_accel(char *str) {
	double fx, fy;
	rfbLog("set_uinput_accel: str=%s\n", str);
	if (sscanf(str, "%lf+%lf", &fx, &fy) == 2) {
		set_uinput_accel_xy(fx, fy);
	} else if (sscanf(str, "%lf", &fx) == 1) {
		set_uinput_accel_xy(fx, fx);
	} else {
		rfbLog("invalid UINPUT accel= option: %s\n", str);
		return 0;
	}
	if (uinput_accel_str) {
		free(uinput_accel_str);
	}
	uinput_accel_str = strdup(str);
	return 1;
}

int set_uinput_thresh(char *str) {
	rfbLog("set_uinput_thresh: str=%s\n", str);
	if (str[0] == '+') {
		thresh_or = 0;
	}
	thresh = atoi(str);
	if (uinput_thresh_str) {
		free(uinput_thresh_str);
	}
	uinput_thresh_str = strdup(str);
	return 1;
}

void set_uinput_reset(int ms) {
	zero_delay = (double) ms/1000.;
	rfbLog("set_uinput_reset: %d\n", ms);
}

void set_uinput_always(int a) {
	uinput_always = a;
}

void set_uinput_touchscreen(int b) {
	uinput_touchscreen = b;
}

void set_uinput_abs(int b) {
	uinput_abs = b;
}

char *get_uinput_accel(void) {
	return uinput_accel_str;
}
char *get_uinput_thresh(void) {
	return uinput_thresh_str;
}
int get_uinput_reset(void) {
	return (int) (1000 * zero_delay);
}

int get_uinput_always(void) {
	return uinput_always;
}

int get_uinput_touchscreen(void) {
	return uinput_touchscreen;
}

int get_uinput_abs(void) {
	return uinput_abs;
}

void parse_uinput_str(char *in) {
	char *p, *q, *str = strdup(in);

	if (injectable) {
		free(injectable);
		injectable = strdup("KMB");
	}

	uinput_touchscreen = 0;
	uinput_abs = 0;
	abs_x = abs_y = 0;

	if (tslib_cal) {
		free(tslib_cal);
		tslib_cal = NULL;
	}

	p = strtok(str, ",");
	while (p) {
		if (p[0] == '/') {
			if (uinput_dev) {
				free(uinput_dev);
			}
			uinput_dev = strdup(p);
		} else if (strstr(p, "nouinput") == p) {
			if (uinput_dev) {
				free(uinput_dev);
			}
			uinput_dev = strdup(p);
		} else if (strstr(p, "accel=") == p) {
			q = p + strlen("accel=");
			if (! set_uinput_accel(q)) {
				clean_up_exit(1);
			}
		} else if (strstr(p, "thresh=") == p) {
			q = p + strlen("thresh=");
			set_uinput_thresh(q);

		} else if (strstr(p, "reset=") == p) {
			int n = atoi(p + strlen("reset="));
			set_uinput_reset(n);
		} else if (strstr(p, "always=") == p) {
			int n = atoi(p + strlen("always="));
			set_uinput_always(n);
		} else if (strpbrk(p, "KMB") == p) {
			if (injectable) {
				free(injectable);
			}
			injectable = strdup(p);
		} else if (strstr(p, "touch_always=") == p) {
			touch_always = atoi(p + strlen("touch_always="));
		} else if (strstr(p, "btn_touch=") == p) {
			btn_touch = atoi(p + strlen("btn_touch="));
		} else if (strstr(p, "dragskip=") == p) {
			dragskip = atoi(p + strlen("dragskip="));
		} else if (strstr(p, "touch") == p) {
			int gw, gh;
			q = strchr(p, '=');
			set_uinput_touchscreen(1);
			set_uinput_abs(1);
			if (q && sscanf(q+1, "%dx%d", &gw, &gh) == 2) {
				abs_x = gw;
				abs_y = gh;
			}
		} else if (strstr(p, "abs") == p) {
			int gw, gh;
			q = strchr(p, '=');
			set_uinput_abs(1);
			if (q && sscanf(q+1, "%dx%d", &gw, &gh) == 2) {
				abs_x = gw;
				abs_y = gh;
			}
		} else if (strstr(p, "pressure=") == p) {
			touch_pressure = atoi(p + strlen("pressure="));
		} else if (strstr(p, "direct_rel=") == p) {
			direct_rel_fd = open(p+strlen("direct_rel="), O_WRONLY);
			if (direct_rel_fd < 0) {
				rfbLogPerror("uinput: direct_rel open");
			} else {
				rfbLog("uinput: opened: %s fd=%d\n", p, direct_rel_fd);
			}
		} else if (strstr(p, "direct_abs=") == p) {
			direct_abs_fd = open(p+strlen("direct_abs="), O_WRONLY);
			if (direct_abs_fd < 0) {
				rfbLogPerror("uinput: direct_abs open");
			} else {
				rfbLog("uinput: opened: %s fd=%d\n", p, direct_abs_fd);
			}
		} else if (strstr(p, "direct_btn=") == p) {
			direct_btn_fd = open(p+strlen("direct_btn="), O_WRONLY);
			if (direct_btn_fd < 0) {
				rfbLogPerror("uinput: direct_btn open");
			} else {
				rfbLog("uinput: opened: %s fd=%d\n", p, direct_btn_fd);
			}
		} else if (strstr(p, "direct_key=") == p) {
			direct_key_fd = open(p+strlen("direct_key="), O_WRONLY);
			if (direct_key_fd < 0) {
				rfbLogPerror("uinput: direct_key open");
			} else {
				rfbLog("uinput: opened: %s fd=%d\n", p, direct_key_fd);
			}
		} else if (strstr(p, "tslib_cal=") == p) {
			tslib_cal = strdup(p+strlen("tslib_cal="));
		} else {
			rfbLog("invalid UINPUT option: %s\n", p);
			clean_up_exit(1);
		}
		p = strtok(NULL, ",");
	}
	free(str);
}

static void ptr_move(int dx, int dy) {
#ifdef UINPUT_OK
	struct input_event ev;
	int d = direct_rel_fd < 0 ? fd : direct_rel_fd;

	if (injectable && strchr(injectable, 'M') == NULL) {
		return;
	}

	memset(&ev, 0, sizeof(ev));

	if (db) fprintf(stderr, "ptr_move(%d, %d) fd=%d\n", dx, dy, d);

	gettimeofday(&ev.time, NULL);
	ev.type = EV_REL;
	ev.code = REL_Y;
	ev.value = dy;
	write(d, &ev, sizeof(ev));

	ev.type = EV_REL;
	ev.code = REL_X;
	ev.value = dx;
	write(d, &ev, sizeof(ev));

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	write(d, &ev, sizeof(ev));
#else
	if (!dx || !dy) {}
#endif
}

static void apply_tslib(int *x, int *y) {
	double x1 = *x, y1 = *y, x2, y2;

	/* this is the inverse of the tslib linear transform: */
	x2 = (a[4] * (a[6] * x1 - a[2]) - a[1] * (a[6] * y1 - a[5]))/(a[4]*a[0] - a[1]*a[3]);
	y2 = (a[0] * (a[6] * y1 - a[5]) - a[3] * (a[6] * x1 - a[2]))/(a[4]*a[0] - a[1]*a[3]);

	*x = (int) x2;
	*y = (int) y2;
}
	

static void ptr_abs(int x, int y, int p) {
#ifdef UINPUT_OK
	struct input_event ev;
	int x0, y0;
	int d = direct_abs_fd < 0 ? fd : direct_abs_fd;

	if (injectable && strchr(injectable, 'M') == NULL) {
		return;
	}

	memset(&ev, 0, sizeof(ev));

	x0 = x;
	y0 = y;

	if (tslib_cal) {
		apply_tslib(&x, &y);
	}

	if (db) fprintf(stderr, "ptr_abs(%d, %d => %d %d, p=%d) fd=%d\n", x0, y0, x, y, p, d);

	gettimeofday(&ev.time, NULL);
	ev.type = EV_ABS;
	ev.code = ABS_Y;
	ev.value = y;
	write(d, &ev, sizeof(ev));

	ev.type = EV_ABS;
	ev.code = ABS_X;
	ev.value = x;
	write(d, &ev, sizeof(ev));

	if (p >= 0) {
		ev.type = EV_ABS;
		ev.code = ABS_PRESSURE;
		ev.value = p;
		write(d, &ev, sizeof(ev));
	}

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	write(d, &ev, sizeof(ev));
#else
	if (!x || !y) {}
#endif
}

static int inside_thresh(int dx, int dy, int thr) {
	if (thresh_or) {
		/* this is peeking at qt-embedded qmouse_qws.cpp */
		if (nabs(dx) <= thresh && nabs(dy) <= thr) {
			return 1;
		}
	} else {
		/* this is peeking at xfree/xorg xf86Xinput.c */
		if (nabs(dx) + nabs(dy) < thr) {
			return 1;
		}
	}
	return 0;
}

static void ptr_rel(int dx, int dy) {
	int dxf, dyf, nx, ny, k;
	int accel, thresh_high, thresh_mid;
	double fx, fy;
	static int try_threshes = -1;

	if (try_threshes < 0) {
		if (getenv("X11VNC_UINPUT_THRESHOLDS")) {
			try_threshes = 1;
		} else {
			try_threshes = 0;
		}
	}

	if (try_threshes) {
		thresh_high = (int) ( (double) thresh/fudge_x );
		thresh_mid =  (int) ( (double) (thresh + thresh_high) / 2.0 );

		if (thresh_mid <= thresh) {
			thresh_mid = thresh + 1;
		}
		if (thresh_high <= thresh_mid) {
			thresh_high = thresh_mid + 1;
		}

		if (inside_thresh(dx, dy, thresh)) {
			accel = 0;
		} else {
			accel = 1;
		}
		nx = nabs(dx);
		ny = nabs(dy);

	} else {
		accel = 1;
		thresh_high = 0;
		nx = ny = 1;
	}

	if (accel && nx + ny > 0 ) {
		if (thresh_high > 0 && inside_thresh(dx, dy, thresh_high)) {
			double alpha, t;
			/* XXX */
			if (1 || inside_thresh(dx, dy, thresh_mid)) {
				t = thresh; 
				accel = 2;
			} else {
				accel = 3;
				t = thresh_high;
			}
			if (thresh_or) {
				if (nx > ny) {
					fx = t;
					fy =  ((double) ny / (double) nx) * t;
				} else {
					fx =  ((double) nx / (double) ny) * t;
					fy = t;
				}
				dxf = (int) fx;
				dyf = (int) fy;
				fx = dx;
				fy = dy;
				
			} else {
				if (t > 1) {
					/* XXX */
					t = t - 1.0;
				}
				alpha = t/(nx + ny);
				fx = alpha * dx;
				fy = alpha * dy;
				dxf = (int) fx;
				dyf = (int) fy;
				fx = dx;
				fy = dy;
			}
		} else {
			fx = fudge_x * (double) dx;
			fy = fudge_y * (double) dy;
			dxf = (int) fx;
			dyf = (int) fy;
		}
	} else {
		fx = dx;
		fy = dy;
		dxf = dx;
		dyf = dy;
	}

	if (db > 1) fprintf(stderr, "old dx dy: %d %d\n", dx, dy);
	if (db > 1) fprintf(stderr, "new dx dy: %d %d  accel: %d\n", dxf, dyf, accel);

	ptr_move(dxf, dyf);

	resid_x += fx - dxf;
	resid_y += fy - dyf;

	for (k = 0; k < 4; k++) {
		if (resid_x <= -1.0 || resid_x >= 1.0 || resid_y <= -1.0 || resid_y >= 1.0) {
			dxf = 0;
			dyf = 0;
			if (resid_x >= 1.0) {
				dxf = (int) resid_x;
				dxf = 1;
			} else if (resid_x <= -1.0)  {
				dxf = -((int) (-resid_x));
				dxf = -1;
			}
			resid_x -= dxf;
			if (resid_y >= 1.0) {
				dyf = (int) resid_y;
				dyf = 1;
			} else if (resid_y <= -1.0)  {
				dyf = -((int) (-resid_y));
				dyf = -1;
			}
			resid_y -= dyf;

			if (db > 1) fprintf(stderr, "*%s resid: dx dy: %d %d  %f %f\n", accel > 1 ? "*" : " ", dxf, dyf, resid_x, resid_y);
if (0) {usleep(100*1000) ;}
			ptr_move(dxf, dyf);
		}
	}
}

static void button_click(int down, int btn) {
#ifdef UINPUT_OK
	struct input_event ev;
	int d = direct_btn_fd < 0 ? fd : direct_btn_fd;

	if (injectable && strchr(injectable, 'B') == NULL) {
		return;
	}

	if (db) fprintf(stderr, "button_click: btn %d %s fd=%d\n", btn, down ? "down" : "up", d);

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);
	ev.type = EV_KEY;
	ev.value = down;

	if (uinput_touchscreen) {
		ev.code = BTN_TOUCH;
		if (db) fprintf(stderr, "set code to BTN_TOUCH\n");
	} else if (btn == 1) {
		ev.code = BTN_LEFT;
	} else if (btn == 2) {
		ev.code = BTN_MIDDLE;
	} else if (btn == 3) {
		ev.code = BTN_RIGHT;
	} else if (btn == 4) {
		ev.code = BTN_FORWARD;
	} else if (btn == 5) {
		ev.code = BTN_BACK;
	} else {
		return;
	}

	write(d, &ev, sizeof(ev));

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	write(d, &ev, sizeof(ev));

	last_button_click = dnow();
#else
	if (!down || !btn) {}
#endif
}


void uinput_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	static int last_x = -1, last_y = -1, last_mask = -1;
	static double last_zero = 0.0;
	allowed_input_t input;
	int do_reset, reset_lower_right = 1;
	double now;
	static int first = 1;

	if (first) {
		if (getenv("RESET_ALWAYS")) {
			set_uinput_always(1);
		} else {
			set_uinput_always(0);
		}
	}
	first = 0;
	
	if (db) fprintf(stderr, "uinput_pointer_command: %d %d - %d\n", x, y, mask);

	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);

	now = dnow();

	do_reset = 1;
	if (mask || bmask) {
		do_reset = 0;	/* do not do reset if mouse button down */
	} else if (! input.motion) {
		do_reset = 0;
	} else if (now < last_zero + zero_delay) {
		do_reset = 0;
	}
	if (do_reset) {
		if (mod_is_down()) {
			do_reset = 0;
		} else if (now < last_button_click + 0.25) {
			do_reset = 0;
		}
	}

	if (uinput_always && !mask && !bmask && input.motion) {
		do_reset = 1;
	}
	if (uinput_abs) {
		do_reset = 0;
	}

	if (do_reset) {
		static int first = 1;

		if (zero_delay > 0.0 || first) {
			/* try to push it to 0,0 */
			int tx, ty, bigjump = 1;

			if (reset_lower_right) {
				tx = fudge_x * (dpy_x - last_x);
				ty = fudge_y * (dpy_y - last_y);
			} else {
				tx = fudge_x * last_x;
				ty = fudge_y * last_y;
			}

			tx += 50;
			ty += 50;

			if (bigjump) {
				if (reset_lower_right) {
					ptr_move(0, +ty);
					usleep(2*1000);
					ptr_move(+tx, +ty);
					ptr_move(+tx, +ty);
				} else {
					ptr_move(0, -ty);
					usleep(2*1000);
					ptr_move(-tx, -ty);
					ptr_move(-tx, -ty);
				}
			} else {
				int i, step, n = 20;
				step = dpy_x / n;

				if (step < 100) step = 100;

				for (i=0; i < n; i++)  {
					if (reset_lower_right) {
						ptr_move(+step, +step);
					} else {
						ptr_move(-step, -step);
					}
				}
				for (i=0; i < n; i++)  {
					if (reset_lower_right) {
						ptr_move(+1, +1);
					} else {
						ptr_move(-1, -1);
					}
				}
			}
			if (db) {
				if (reset_lower_right) {
					fprintf(stderr, "uinput_pointer_command: reset -> (W,H) (%d,%d)  [%d,%d]\n", x, y, tx, ty);
				} else {
					fprintf(stderr, "uinput_pointer_command: reset -> (0,0) (%d,%d)  [%d,%d]\n", x, y, tx, ty);
				}
			}

			/* rest a bit for system to absorb the change */
			if (uinput_always) {
				static double last_sleep = 0.0;
				double nw = dnow(), delay = zero_delay;
				if (delay <= 0.0) delay = 0.1;
				if (nw > last_sleep + delay) {
					usleep(10*1000);
					last_sleep = nw;
				} else {
					usleep(1*1000);
				}
				
			} else {
				usleep(30*1000);
			}

			/* now jump back out */
			if (reset_lower_right) {
				ptr_rel(x - dpy_x, y - dpy_y);
			} else {
				ptr_rel(x, y);
			}
			if (1) {usleep(10*1000) ;}

			last_x = x;
			last_y = y;
			resid_x = 0.0;
			resid_y = 0.0;

			first = 0;
		}
		last_zero = dnow();
	}

	if (input.motion) {
		if (x != last_x || y != last_y) {
			if (uinput_touchscreen) {
				;
			} else if (uinput_abs) {
				ptr_abs(x, y, -1);
			} else {
				ptr_rel(x - last_x, y - last_y);
			}
			last_x = x;
			last_y = y;
		}
	}

	if (! input.button) {
		return;
	}

	if (last_mask < 0) {
		last_mask = mask;
	}

	if (db > 2) {
		fprintf(stderr, "mask:        %s\n", bitprint(mask, 16));
		fprintf(stderr, "bmask:       %s\n", bitprint(bmask, 16));
		fprintf(stderr, "last_mask:   %s\n", bitprint(last_mask, 16));
		fprintf(stderr, "button_mask: %s\n", bitprint(button_mask, 16));
	}

	if (uinput_touchscreen) {
		if (!btn_touch) {
			static int down_count = 0;
			int p = touch_pressure >=0 ? touch_pressure : 0;
			if (!last_mask && !mask) {
				if (touch_always) {
					ptr_abs(last_x, last_y, 0);
				}
			} else if (!last_mask && mask) {
				ptr_abs(last_x, last_y, p);
				down_count = 0;
			} else if (last_mask && !mask) {
				ptr_abs(last_x, last_y, 0);
			} else if (last_mask && mask) {
				down_count++;
				if (dragskip > 0) {
					if (down_count % dragskip == 0) {
						ptr_abs(last_x, last_y, p);
					}
				} else {
					ptr_abs(last_x, last_y, p);
				}
			}
		} else {
			if (!last_mask && !mask) {
				if (touch_always) {
					ptr_abs(last_x, last_y, 0);
				}
			} else if (!last_mask && mask) {
				ptr_abs(last_x, last_y, 0);
				button_click(1, 0);
			} else if (last_mask && !mask) {
				ptr_abs(last_x, last_y, 0);
				button_click(0, 0);
			} else if (last_mask && mask) {
				;
			}
		}
		last_mask = mask;
	} else if (mask != last_mask) {
		int i;
		for (i=1; i <= MAX_BUTTONS; i++) {
			int down, b = 1 << (i-1);
			if ( (last_mask & b) == (mask & b)) {
				continue;
			}
			if (mask & b) {
				down = 1;
			} else {
				down = 0;
			}
			button_click(down, i);
		}
		if (mask && uinput_abs && touch_pressure >= 0) {
			ptr_abs(last_x, last_y, touch_pressure);
		}
		last_mask = mask;
	}
	bmask = mask;
}

void uinput_key_command(int down, int keysym, rfbClientPtr client) {
#ifdef UINPUT_OK
	struct input_event ev;
	int scancode;
	allowed_input_t input;
	int d = direct_key_fd < 0 ? fd : direct_key_fd;

	if (injectable && strchr(injectable, 'K') == NULL) {
		return;
	}
	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		return;
	}

	scancode = lookup_code(keysym);

	if (scancode < 0) {
		return;
	}
	if (db) fprintf(stderr, "uinput_key_command: %d -> %d %s fd=%d\n", keysym, scancode, down ? "down" : "up", d);

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);
	ev.type = EV_KEY;
	ev.code = (unsigned char) scancode;
	ev.value = down;

	write(d, &ev, sizeof(ev));

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	write(d, &ev, sizeof(ev));

	if (0 <= scancode && scancode < 256) {
		key_pressed[scancode] = down ? 1 : 0;
	}
#else
	if (!down || !keysym || !client) {}
#endif
}

#if 0
  grep 'case XK_' x0vnc.c | sed -e 's/case /$key_lookup{/' -e 's/:/}/' -e 's/return /= $/'
#endif

static int lookup_code(int keysym) {

	if (keysym == NoSymbol) {
		return -1;
	}

	switch(keysym) {
#ifdef UINPUT_OK
	case XK_Escape:	return KEY_ESC;
	case XK_1:		return KEY_1;
	case XK_2:		return KEY_2;
	case XK_3:		return KEY_3;
	case XK_4:		return KEY_4;
	case XK_5:		return KEY_5;
	case XK_6:		return KEY_6;
	case XK_7:		return KEY_7;
	case XK_8:		return KEY_8;
	case XK_9:		return KEY_9;
	case XK_0:		return KEY_0;
	case XK_exclam:	return KEY_1;
	case XK_at:		return KEY_2;
	case XK_numbersign:	return KEY_3;
	case XK_dollar:	return KEY_4;
	case XK_percent:	return KEY_5;
	case XK_asciicircum:	return KEY_6;
	case XK_ampersand:	return KEY_7;
	case XK_asterisk:	return KEY_8;
	case XK_parenleft:	return KEY_9;
	case XK_parenright:	return KEY_0;
	case XK_minus:	return KEY_MINUS;
	case XK_underscore:	return KEY_MINUS;
	case XK_equal:	return KEY_EQUAL;
	case XK_plus:	return KEY_EQUAL;
	case XK_BackSpace:	return KEY_BACKSPACE;
	case XK_Tab:		return KEY_TAB;
	case XK_q:		return KEY_Q;
	case XK_Q:		return KEY_Q;
	case XK_w:		return KEY_W;
	case XK_W:		return KEY_W;
	case XK_e:		return KEY_E;
	case XK_E:		return KEY_E;
	case XK_r:		return KEY_R;
	case XK_R:		return KEY_R;
	case XK_t:		return KEY_T;
	case XK_T:		return KEY_T;
	case XK_y:		return KEY_Y;
	case XK_Y:		return KEY_Y;
	case XK_u:		return KEY_U;
	case XK_U:		return KEY_U;
	case XK_i:		return KEY_I;
	case XK_I:		return KEY_I;
	case XK_o:		return KEY_O;
	case XK_O:		return KEY_O;
	case XK_p:		return KEY_P;
	case XK_P:		return KEY_P;
	case XK_braceleft:	return KEY_LEFTBRACE;
	case XK_braceright:	return KEY_RIGHTBRACE;
	case XK_bracketleft:	return KEY_LEFTBRACE;
	case XK_bracketright:	return KEY_RIGHTBRACE;
	case XK_Return:	return KEY_ENTER;
	case XK_Control_L:	return KEY_LEFTCTRL;
	case XK_a:		return KEY_A;
	case XK_A:		return KEY_A;
	case XK_s:		return KEY_S;
	case XK_S:		return KEY_S;
	case XK_d:		return KEY_D;
	case XK_D:		return KEY_D;
	case XK_f:		return KEY_F;
	case XK_F:		return KEY_F;
	case XK_g:		return KEY_G;
	case XK_G:		return KEY_G;
	case XK_h:		return KEY_H;
	case XK_H:		return KEY_H;
	case XK_j:		return KEY_J;
	case XK_J:		return KEY_J;
	case XK_k:		return KEY_K;
	case XK_K:		return KEY_K;
	case XK_l:		return KEY_L;
	case XK_L:		return KEY_L;
	case XK_semicolon:	return KEY_SEMICOLON;
	case XK_colon:	return KEY_SEMICOLON;
	case XK_apostrophe:	return KEY_APOSTROPHE;
	case XK_quotedbl:	return KEY_APOSTROPHE;
	case XK_grave:	return KEY_GRAVE;
	case XK_asciitilde:	return KEY_GRAVE;
	case XK_Shift_L:	return KEY_LEFTSHIFT;
	case XK_backslash:	return KEY_BACKSLASH;
	case XK_bar:		return KEY_BACKSLASH;
	case XK_z:		return KEY_Z;
	case XK_Z:		return KEY_Z;
	case XK_x:		return KEY_X;
	case XK_X:		return KEY_X;
	case XK_c:		return KEY_C;
	case XK_C:		return KEY_C;
	case XK_v:		return KEY_V;
	case XK_V:		return KEY_V;
	case XK_b:		return KEY_B;
	case XK_B:		return KEY_B;
	case XK_n:		return KEY_N;
	case XK_N:		return KEY_N;
	case XK_m:		return KEY_M;
	case XK_M:		return KEY_M;
	case XK_comma:	return KEY_COMMA;
	case XK_less:	return KEY_COMMA;
	case XK_period:	return KEY_DOT;
	case XK_greater:	return KEY_DOT;
	case XK_slash:	return KEY_SLASH;
	case XK_question:	return KEY_SLASH;
	case XK_Shift_R:	return KEY_RIGHTSHIFT;
	case XK_KP_Multiply:	return KEY_KPASTERISK;
	case XK_Alt_L:	return KEY_LEFTALT;
	case XK_space:	return KEY_SPACE;
	case XK_Caps_Lock:	return KEY_CAPSLOCK;
	case XK_F1:		return KEY_F1;
	case XK_F2:		return KEY_F2;
	case XK_F3:		return KEY_F3;
	case XK_F4:		return KEY_F4;
	case XK_F5:		return KEY_F5;
	case XK_F6:		return KEY_F6;
	case XK_F7:		return KEY_F7;
	case XK_F8:		return KEY_F8;
	case XK_F9:		return KEY_F9;
	case XK_F10:		return KEY_F10;
	case XK_Num_Lock:	return KEY_NUMLOCK;
	case XK_Scroll_Lock:	return KEY_SCROLLLOCK;
	case XK_KP_7:		return KEY_KP7;
	case XK_KP_8:		return KEY_KP8;
	case XK_KP_9:		return KEY_KP9;
	case XK_KP_Subtract:	return KEY_KPMINUS;
	case XK_KP_4:		return KEY_KP4;
	case XK_KP_5:		return KEY_KP5;
	case XK_KP_6:		return KEY_KP6;
	case XK_KP_Add:	return KEY_KPPLUS;
	case XK_KP_1:		return KEY_KP1;
	case XK_KP_2:		return KEY_KP2;
	case XK_KP_3:		return KEY_KP3;
	case XK_KP_0:		return KEY_KP0;
	case XK_KP_Decimal:	return KEY_KPDOT;
	case XK_F13:		return KEY_F13;
	case XK_F11:		return KEY_F11;
	case XK_F12:		return KEY_F12;
	case XK_F14:		return KEY_F14;
	case XK_F15:		return KEY_F15;
	case XK_F16:		return KEY_F16;
	case XK_F17:		return KEY_F17;
	case XK_F18:		return KEY_F18;
	case XK_F19:		return KEY_F19;
	case XK_F20:		return KEY_F20;
	case XK_KP_Enter:	return KEY_KPENTER;
	case XK_Control_R:	return KEY_RIGHTCTRL;
	case XK_KP_Divide:	return KEY_KPSLASH;
	case XK_Sys_Req:	return KEY_SYSRQ;
	case XK_Alt_R:	return KEY_RIGHTALT;
	case XK_Linefeed:	return KEY_LINEFEED;
	case XK_Home:		return KEY_HOME;
	case XK_Up:		return KEY_UP;
	case XK_Page_Up:	return KEY_PAGEUP;
	case XK_Left:		return KEY_LEFT;
	case XK_Right:	return KEY_RIGHT;
	case XK_End:		return KEY_END;
	case XK_Down:		return KEY_DOWN;
	case XK_Page_Down:	return KEY_PAGEDOWN;
	case XK_Insert:	return KEY_INSERT;
	case XK_Delete:	return KEY_DELETE;
	case XK_KP_Equal:	return KEY_KPEQUAL;
	case XK_Pause:	return KEY_PAUSE;
	case XK_F21:		return KEY_F21;
	case XK_F22:		return KEY_F22;
	case XK_F23:		return KEY_F23;
	case XK_F24:		return KEY_F24;
	case XK_KP_Separator:	return KEY_KPCOMMA;
	case XK_Meta_L:	return KEY_LEFTMETA;
	case XK_Meta_R:	return KEY_RIGHTMETA;
	case XK_Multi_key:	return KEY_COMPOSE;
#endif
	default:		return -1;
	}
}

#if 0

From /usr/include/linux/input.h

We maintain it here since it is such a painful mess.

Here is a little script to make it easier:

#!/usr/bin/perl
while (<>) {
	$_ =~ s/-XK_/XK_/;
	next unless /^XK_/;
	chomp;
	if (/^(\S+)(\s+)(\S+)/) {
		$a = $1;
		$t = $2;
		$b = $3;
		print "\tcase $a:${t}return $b;\n";
		if ($a =~ /XK_[a-z]$/) {
			$a = uc($a);
			print "\tcase $a:${t}return $b;\n";
		}
	}
}

This only handles US kbd, we would need a kbd database in general...
Ugh: parse dumpkeys(1) or -fookeys /usr/share/keymaps/i386/qwerty/dk.kmap.gz

XK_Escape	KEY_ESC
XK_1		KEY_1
XK_2		KEY_2
XK_3		KEY_3
XK_4		KEY_4
XK_5		KEY_5
XK_6		KEY_6
XK_7		KEY_7
XK_8		KEY_8
XK_9		KEY_9
XK_0		KEY_0
-XK_exclam	KEY_1
-XK_at		KEY_2
-XK_numbersign	KEY_3
-XK_dollar	KEY_4
-XK_percent	KEY_5
-XK_asciicircum	KEY_6
-XK_ampersand	KEY_7
-XK_asterisk	KEY_8
-XK_parenleft	KEY_9
-XK_parenright	KEY_0
XK_minus	KEY_MINUS
-XK_underscore	KEY_MINUS
XK_equal	KEY_EQUAL
-XK_plus	KEY_EQUAL
XK_BackSpace	KEY_BACKSPACE
XK_Tab		KEY_TAB
XK_q		KEY_Q
XK_w		KEY_W
XK_e		KEY_E
XK_r		KEY_R
XK_t		KEY_T
XK_y		KEY_Y
XK_u		KEY_U
XK_i		KEY_I
XK_o		KEY_O
XK_p		KEY_P
XK_braceleft	KEY_LEFTBRACE
XK_braceright	KEY_RIGHTBRACE
-XK_bracketleft	KEY_LEFTBRACE
-XK_bracketright	KEY_RIGHTBRACE
XK_Return	KEY_ENTER
XK_Control_L	KEY_LEFTCTRL
XK_a		KEY_A
XK_s		KEY_S
XK_d		KEY_D
XK_f		KEY_F
XK_g		KEY_G
XK_h		KEY_H
XK_j		KEY_J
XK_k		KEY_K
XK_l		KEY_L
XK_semicolon	KEY_SEMICOLON
-XK_colon	KEY_SEMICOLON
XK_apostrophe	KEY_APOSTROPHE
-XK_quotedbl	KEY_APOSTROPHE
XK_grave	KEY_GRAVE
-XK_asciitilde	KEY_GRAVE
XK_Shift_L	KEY_LEFTSHIFT
XK_backslash	KEY_BACKSLASH
-XK_bar		KEY_BACKSLASH
XK_z		KEY_Z
XK_x		KEY_X
XK_c		KEY_C
XK_v		KEY_V
XK_b		KEY_B
XK_n		KEY_N
XK_m		KEY_M
XK_comma	KEY_COMMA
-XK_less	KEY_COMMA
XK_period	KEY_DOT
-XK_greater	KEY_DOT
XK_slash	KEY_SLASH
-XK_question	KEY_SLASH
XK_Shift_R	KEY_RIGHTSHIFT
XK_KP_Multiply	KEY_KPASTERISK
XK_Alt_L	KEY_LEFTALT
XK_space	KEY_SPACE
XK_Caps_Lock	KEY_CAPSLOCK
XK_F1		KEY_F1
XK_F2		KEY_F2
XK_F3		KEY_F3
XK_F4		KEY_F4
XK_F5		KEY_F5
XK_F6		KEY_F6
XK_F7		KEY_F7
XK_F8		KEY_F8
XK_F9		KEY_F9
XK_F10		KEY_F10
XK_Num_Lock	KEY_NUMLOCK
XK_Scroll_Lock	KEY_SCROLLLOCK
XK_KP_7		KEY_KP7
XK_KP_8		KEY_KP8
XK_KP_9		KEY_KP9
XK_KP_Subtract	KEY_KPMINUS
XK_KP_4		KEY_KP4
XK_KP_5		KEY_KP5
XK_KP_6		KEY_KP6
XK_KP_Add	KEY_KPPLUS
XK_KP_1		KEY_KP1
XK_KP_2		KEY_KP2
XK_KP_3		KEY_KP3
XK_KP_0		KEY_KP0
XK_KP_Decimal	KEY_KPDOT
NoSymbol	KEY_103RD
XK_F13		KEY_F13
NoSymbol	KEY_102ND
XK_F11		KEY_F11
XK_F12		KEY_F12
XK_F14		KEY_F14
XK_F15		KEY_F15
XK_F16		KEY_F16
XK_F17		KEY_F17
XK_F18		KEY_F18
XK_F19		KEY_F19
XK_F20		KEY_F20
XK_KP_Enter	KEY_KPENTER
XK_Control_R	KEY_RIGHTCTRL
XK_KP_Divide	KEY_KPSLASH
XK_Sys_Req	KEY_SYSRQ
XK_Alt_R	KEY_RIGHTALT
XK_Linefeed	KEY_LINEFEED
XK_Home		KEY_HOME
XK_Up		KEY_UP
XK_Page_Up	KEY_PAGEUP
XK_Left		KEY_LEFT
XK_Right	KEY_RIGHT
XK_End		KEY_END
XK_Down		KEY_DOWN
XK_Page_Down	KEY_PAGEDOWN
XK_Insert	KEY_INSERT
XK_Delete	KEY_DELETE
NoSymbol	KEY_MACRO
NoSymbol	KEY_MUTE
NoSymbol	KEY_VOLUMEDOWN
NoSymbol	KEY_VOLUMEUP
NoSymbol	KEY_POWER
XK_KP_Equal	KEY_KPEQUAL
NoSymbol	KEY_KPPLUSMINUS
XK_Pause	KEY_PAUSE
XK_F21		KEY_F21
XK_F22		KEY_F22
XK_F23		KEY_F23
XK_F24		KEY_F24
XK_KP_Separator	KEY_KPCOMMA
XK_Meta_L	KEY_LEFTMETA
XK_Meta_R	KEY_RIGHTMETA
XK_Multi_key	KEY_COMPOSE

NoSymbol	KEY_STOP
NoSymbol	KEY_AGAIN
NoSymbol	KEY_PROPS
NoSymbol	KEY_UNDO
NoSymbol	KEY_FRONT
NoSymbol	KEY_COPY
NoSymbol	KEY_OPEN
NoSymbol	KEY_PASTE
NoSymbol	KEY_FIND
NoSymbol	KEY_CUT
NoSymbol	KEY_HELP
NoSymbol	KEY_MENU
NoSymbol	KEY_CALC
NoSymbol	KEY_SETUP
NoSymbol	KEY_SLEEP
NoSymbol	KEY_WAKEUP
NoSymbol	KEY_FILE
NoSymbol	KEY_SENDFILE
NoSymbol	KEY_DELETEFILE
NoSymbol	KEY_XFER
NoSymbol	KEY_PROG1
NoSymbol	KEY_PROG2
NoSymbol	KEY_WWW
NoSymbol	KEY_MSDOS
NoSymbol	KEY_COFFEE
NoSymbol	KEY_DIRECTION
NoSymbol	KEY_CYCLEWINDOWS
NoSymbol	KEY_MAIL
NoSymbol	KEY_BOOKMARKS
NoSymbol	KEY_COMPUTER
NoSymbol	KEY_BACK
NoSymbol	KEY_FORWARD
NoSymbol	KEY_CLOSECD
NoSymbol	KEY_EJECTCD
NoSymbol	KEY_EJECTCLOSECD
NoSymbol	KEY_NEXTSONG
NoSymbol	KEY_PLAYPAUSE
NoSymbol	KEY_PREVIOUSSONG
NoSymbol	KEY_STOPCD
NoSymbol	KEY_RECORD
NoSymbol	KEY_REWIND
NoSymbol	KEY_PHONE
NoSymbol	KEY_ISO
NoSymbol	KEY_CONFIG
NoSymbol	KEY_HOMEPAGE
NoSymbol	KEY_REFRESH
NoSymbol	KEY_EXIT
NoSymbol	KEY_MOVE
NoSymbol	KEY_EDIT
NoSymbol	KEY_SCROLLUP
NoSymbol	KEY_SCROLLDOWN
NoSymbol	KEY_KPLEFTPAREN
NoSymbol	KEY_KPRIGHTPAREN

NoSymbol	KEY_INTL1
NoSymbol	KEY_INTL2
NoSymbol	KEY_INTL3
NoSymbol	KEY_INTL4
NoSymbol	KEY_INTL5
NoSymbol	KEY_INTL6
NoSymbol	KEY_INTL7
NoSymbol	KEY_INTL8
NoSymbol	KEY_INTL9
NoSymbol	KEY_LANG1
NoSymbol	KEY_LANG2
NoSymbol	KEY_LANG3
NoSymbol	KEY_LANG4
NoSymbol	KEY_LANG5
NoSymbol	KEY_LANG6
NoSymbol	KEY_LANG7
NoSymbol	KEY_LANG8
NoSymbol	KEY_LANG9

NoSymbol	KEY_PLAYCD
NoSymbol	KEY_PAUSECD
NoSymbol	KEY_PROG3
NoSymbol	KEY_PROG4
NoSymbol	KEY_SUSPEND
NoSymbol	KEY_CLOSE
NoSymbol	KEY_PLAY
NoSymbol	KEY_FASTFORWARD
NoSymbol	KEY_BASSBOOST
NoSymbol	KEY_PRINT
NoSymbol	KEY_HP
NoSymbol	KEY_CAMERA
NoSymbol	KEY_SOUND
NoSymbol	KEY_QUESTION
NoSymbol	KEY_EMAIL
NoSymbol	KEY_CHAT
NoSymbol	KEY_SEARCH
NoSymbol	KEY_CONNECT
NoSymbol	KEY_FINANCE
NoSymbol	KEY_SPORT
NoSymbol	KEY_SHOP
NoSymbol	KEY_ALTERASE
NoSymbol	KEY_CANCEL
NoSymbol	KEY_BRIGHTNESSDOWN
NoSymbol	KEY_BRIGHTNESSUP
NoSymbol	KEY_MEDIA

NoSymbol	KEY_UNKNOWN
NoSymbol	
NoSymbol	BTN_MISC
NoSymbol	BTN_0
NoSymbol	BTN_1
NoSymbol	BTN_2
NoSymbol	BTN_3
NoSymbol	BTN_4
NoSymbol	BTN_5
NoSymbol	BTN_6
NoSymbol	BTN_7
NoSymbol	BTN_8
NoSymbol	BTN_9
NoSymbol	
NoSymbol	BTN_MOUSE
NoSymbol	BTN_LEFT
NoSymbol	BTN_RIGHT
NoSymbol	BTN_MIDDLE
NoSymbol	BTN_SIDE
NoSymbol	BTN_EXTRA
NoSymbol	BTN_FORWARD
NoSymbol	BTN_BACK
NoSymbol	BTN_TASK
NoSymbol	
NoSymbol	BTN_JOYSTICK
NoSymbol	BTN_TRIGGER
NoSymbol	BTN_THUMB
NoSymbol	BTN_THUMB2
NoSymbol	BTN_TOP
NoSymbol	BTN_TOP2
NoSymbol	BTN_PINKIE
NoSymbol	BTN_BASE
NoSymbol	BTN_BASE2
NoSymbol	BTN_BASE3
NoSymbol	BTN_BASE4
NoSymbol	BTN_BASE5
NoSymbol	BTN_BASE6
NoSymbol	BTN_DEAD

NoSymbol	BTN_GAMEPAD
NoSymbol	BTN_A
NoSymbol	BTN_B
NoSymbol	BTN_C
NoSymbol	BTN_X
NoSymbol	BTN_Y
NoSymbol	BTN_Z
NoSymbol	BTN_TL
NoSymbol	BTN_TR
NoSymbol	BTN_TL2
NoSymbol	BTN_TR2
NoSymbol	BTN_SELECT
NoSymbol	BTN_START
NoSymbol	BTN_MODE
NoSymbol	BTN_THUMBL
NoSymbol	BTN_THUMBR

NoSymbol	BTN_DIGI
NoSymbol	BTN_TOOL_PEN
NoSymbol	BTN_TOOL_RUBBER
NoSymbol	BTN_TOOL_BRUSH
NoSymbol	BTN_TOOL_PENCIL
NoSymbol	BTN_TOOL_AIRBRUSH
NoSymbol	BTN_TOOL_FINGER
NoSymbol	BTN_TOOL_MOUSE
NoSymbol	BTN_TOOL_LENS
NoSymbol	BTN_TOUCH
NoSymbol	BTN_STYLUS
NoSymbol	BTN_STYLUS2
NoSymbol	BTN_TOOL_DOUBLETAP
NoSymbol	BTN_TOOL_TRIPLETAP

NoSymbol	BTN_WHEEL
NoSymbol	BTN_GEAR_DOWN
NoSymbol	BTN_GEAR_UP

NoSymbol	KEY_OK
NoSymbol	KEY_SELECT
NoSymbol	KEY_GOTO
NoSymbol	KEY_CLEAR
NoSymbol	KEY_POWER2
NoSymbol	KEY_OPTION
NoSymbol	KEY_INFO
NoSymbol	KEY_TIME
NoSymbol	KEY_VENDOR
NoSymbol	KEY_ARCHIVE
NoSymbol	KEY_PROGRAM
NoSymbol	KEY_CHANNEL
NoSymbol	KEY_FAVORITES
NoSymbol	KEY_EPG
NoSymbol	KEY_PVR
NoSymbol	KEY_MHP
NoSymbol	KEY_LANGUAGE
NoSymbol	KEY_TITLE
NoSymbol	KEY_SUBTITLE
NoSymbol	KEY_ANGLE
NoSymbol	KEY_ZOOM
NoSymbol	KEY_MODE
NoSymbol	KEY_KEYBOARD
NoSymbol	KEY_SCREEN
NoSymbol	KEY_PC
NoSymbol	KEY_TV
NoSymbol	KEY_TV2
NoSymbol	KEY_VCR
NoSymbol	KEY_VCR2
NoSymbol	KEY_SAT
NoSymbol	KEY_SAT2
NoSymbol	KEY_CD
NoSymbol	KEY_TAPE
NoSymbol	KEY_RADIO
NoSymbol	KEY_TUNER
NoSymbol	KEY_PLAYER
NoSymbol	KEY_TEXT
NoSymbol	KEY_DVD
NoSymbol	KEY_AUX
NoSymbol	KEY_MP3
NoSymbol	KEY_AUDIO
NoSymbol	KEY_VIDEO
NoSymbol	KEY_DIRECTORY
NoSymbol	KEY_LIST
NoSymbol	KEY_MEMO
NoSymbol	KEY_CALENDAR
NoSymbol	KEY_RED
NoSymbol	KEY_GREEN
NoSymbol	KEY_YELLOW
NoSymbol	KEY_BLUE
NoSymbol	KEY_CHANNELUP
NoSymbol	KEY_CHANNELDOWN
NoSymbol	KEY_FIRST
NoSymbol	KEY_LAST
NoSymbol	KEY_AB
NoSymbol	KEY_NEXT
NoSymbol	KEY_RESTART
NoSymbol	KEY_SLOW
NoSymbol	KEY_SHUFFLE
NoSymbol	KEY_BREAK
NoSymbol	KEY_PREVIOUS
NoSymbol	KEY_DIGITS
NoSymbol	KEY_TEEN
NoSymbol	KEY_TWEN

NoSymbol	KEY_DEL_EOL
NoSymbol	KEY_DEL_EOS
NoSymbol	KEY_INS_LINE
NoSymbol	KEY_DEL_LINE
NoSymbol	KEY_MAX

#endif


