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

/* -- v4l.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "scan.h"
#include "xinerama.h"
#include "screen.h"
#include "connections.h"
#include "keyboard.h"
#include "allowed_input_t.h"

#if LIBVNCSERVER_HAVE_LINUX_VIDEODEV_H
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#define CONFIG_VIDEO_V4L1_COMPAT
#include <linux/videodev.h>
#ifdef __LINUX_VIDEODEV2_H
# ifndef HAVE_V4L2
# define HAVE_V4L2 1
# endif
#endif
#define V4L_OK
#endif
#endif

char *v4l_guess(char *str, int *fd);
void v4l_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
void v4l_pointer_command(int mask, int x, int y, rfbClientPtr client);

static int v4l1_val(int pct);
static int v4l1_width(int w);
static int v4l1_height(int h);
static int v4l1_resize(int fd, int w, int h);
static void v4l1_setfreq(int fd, unsigned long freq, int verb);
static void v4l1_set_input(int fd, int which);
static int v4l1_setfmt(int fd, char *fmt);
static void apply_settings(char *dev, char *settings, int *fd);
static int v4l1_dpct(int old, int d);
static void v4l_requery(void);
static void v4l_br(int b);
static void v4l_hu(int b);
static void v4l_co(int b);
static void v4l_cn(int b);
static void v4l_sz(int b);
static void v4l_sta(int sta);
static void v4l_inp(int inp);
static void v4l_fmt(char *fmt);
static int colon_n(char *line);
static char *colon_str(char *line);
static char *colon_tag(char *line);
static void lookup_rgb(char *g_fmt, int *g_b, int *mask_rev);
static char *v4l1_lu_palette(unsigned short palette);
static unsigned short v4l1_lu_palette_str(char *name, int *bits, int *rev);
static char *v4l2_lu_palette(unsigned int palette);
static unsigned int v4l2_lu_palette_str(char *name, int *bits, int *rev);
static int v4l1_query(int fd, int verbose);
static int v4l2_query(int fd, int verbose);
static int open_dev(char *dev);
static char *guess_via_v4l(char *dev, int *fd);
static char *guess_via_v4l_info(char *dev, int *fd);
static void parse_str(char *str, char **dev, char **settings, char **atparms);
static unsigned long lookup_freqtab(int sta);
static unsigned long lookup_freq(int sta);
static int lookup_station(unsigned long freq);
static void init_freqtab(char *file);
static void init_freqs(void);
static void init_ntsc_cable(void);

#define C_VIDEO_CAPTURE 1
#define C_PICTURE       2
#define C_WINDOW        3 

#ifdef V4L_OK
static struct video_capability  v4l1_capability;
static struct video_channel     v4l1_channel;
static struct video_tuner       v4l1_tuner;
static struct video_picture     v4l1_picture;
static struct video_window      v4l1_window;

#if HAVE_V4L2
static struct v4l2_capability   v4l2_capability;
static struct v4l2_input        v4l2_input;
static struct v4l2_tuner        v4l2_tuner;
static struct v4l2_fmtdesc      v4l2_fmtdesc;
static struct v4l2_format       v4l2_format;
/*static struct v4l2_framebuffer  v4l2_fbuf; */
/*static struct v4l2_queryctrl    v4l2_qctrl; */
#endif
#endif

static int v4l1_cap = -1;
static int v4l2_cap = -1;
#define V4L1_MAX 65535

#define CHANNEL_MAX 500
static unsigned long ntsc_cable[CHANNEL_MAX];
static unsigned long custom_freq[CHANNEL_MAX];

static unsigned long last_freq = 0;
static int last_channel = 0;

static int v4l1_val(int pct) {
	/* pct is % */
	int val, max = V4L1_MAX; 
	if (pct < 0) {
		return 0;
	} else if (pct > 100) {
		return max;
	}
	val = (pct * max)/100;

	return val;
}
static int v4l1_width(int w) {
#ifdef V4L_OK
	int min = v4l1_capability.minwidth;
	int max = v4l1_capability.maxwidth;
	if (w < min) {
		w = min;
	}
	if (w > max) {
		w = max;
	}
#endif
	return w;
}
static int v4l1_height(int h) {
#ifdef V4L_OK
	int min = v4l1_capability.minheight;
	int max = v4l1_capability.maxheight;
	if (h < min) {
		h = min;
	}
	if (h > max) {
		h = max;
	}
#endif
	return h;
}

static int v4l1_resize(int fd, int w, int h) {
#ifdef V4L_OK
	int dowin = 0;

	memset(&v4l1_window, 0, sizeof(v4l1_window));
	if (ioctl(fd, VIDIOCGWIN, &v4l1_window) == -1) {
		return 0;
	}

	if (w > 0) w = v4l1_width(w);

	if (w > 0 && w != (int) v4l1_window.width) {
		dowin = 1;
	}

	if (h > 0) h = v4l1_height(h);

	if (h > 0 && h != (int) v4l1_window.height) {
		dowin = 1;
	}

	if (dowin) {
		v4l1_window.x = 0;
		v4l1_window.y = 0;
		ioctl(fd, VIDIOCSWIN, &v4l1_window);
		if (w > 0) v4l1_window.width = w;
		if (h > 0) v4l1_window.height = h;
		fprintf(stderr, "calling V4L_1: VIDIOCSWIN\n");
		fprintf(stderr, "trying new size %dx%d\n",
		    v4l1_window.width, v4l1_window.height);
		if (ioctl(fd, VIDIOCSWIN, &v4l1_window) == -1) {
			perror("ioctl VIDIOCSWIN");
			return 0;
		}
	}
#else
	if (!fd || !w || !h) {}
#endif
	return 1;
}

static void v4l1_setfreq(int fd, unsigned long freq, int verb) {
#ifdef V4L_OK
	unsigned long f0, f1;
	f1 = (freq * 16) / 1000;
	ioctl(fd, VIDIOCGFREQ, &f0);
	if (verb) fprintf(stderr, "read freq: %d\n", (int) f0);
	if (freq > 0) {
		if (ioctl(fd, VIDIOCSFREQ, &f1) == -1) {
			perror("ioctl VIDIOCSFREQ");
		} else {
			ioctl(fd, VIDIOCGFREQ, &f0);
			if (verb) fprintf(stderr, "read freq: %d\n", (int) f0);
			last_freq = freq;
		}
	}
#else
	if (!fd || !freq || !verb) {}
#endif
}

static void v4l1_set_input(int fd, int which) {
#ifdef V4L_OK
	if (which != -1) {
		memset(&v4l1_channel, 0, sizeof(v4l1_channel));
		v4l1_channel.channel = which;
		if (ioctl(fd, VIDIOCGCHAN, &v4l1_channel) != -1) {
			v4l1_channel.channel = which;
			fprintf(stderr, "setting input channel to %d: %s\n",
			    which, v4l1_channel.name);
			last_channel = which;
			ioctl(fd, VIDIOCSCHAN, &v4l1_channel);
		}
	}
#else
	if (!fd || !which) {}
#endif
}

static int v4l1_setfmt(int fd, char *fmt) {
#ifdef V4L_OK
	unsigned short fnew;
	int bnew, rnew;

	fnew = v4l1_lu_palette_str(fmt, &bnew, &rnew);
	if (fnew) {
		v4l1_picture.depth = bnew; 
		v4l1_picture.palette = fnew;
	}
	fprintf(stderr, "calling V4L_1: VIDIOCSPICT\n");
	if (ioctl(fd, VIDIOCSPICT, &v4l1_picture) == -1) {
		perror("ioctl VIDIOCSPICT");
		return 0;
	}
	if (raw_fb_pixfmt) {
		free(raw_fb_pixfmt);
	}
	raw_fb_pixfmt = strdup(fmt);
#else
	if (!fd || !fmt) {}
#endif
	return 1;
}

static int ignore_all = 0;

static void apply_settings(char *dev, char *settings, int *fd) {
#ifdef V4L_OK
	char *str, *p, *fmt = NULL, *tun = NULL, *inp = NULL;
	int br = -1, co = -1, cn = -1, hu = -1;
	int w = -1, h = -1, b = -1;
	int sta = -1;
	int setcnt = 0;
	if (! settings || settings[0] == '\0') {
		return;
	}
	str = strdup(settings);
	p = strtok(str, ",");
	while (p) {
		if (strstr(p, "br=") == p) {
			br = atoi(p+3);
			if (br >= 0) setcnt++;
		} else if (strstr(p, "co=") == p) {
			co = atoi(p+3);
			if (co >= 0) setcnt++;
		} else if (strstr(p, "cn=") == p) {
			cn = atoi(p+3);
			if (cn >= 0) setcnt++;
		} else if (strstr(p, "hu=") == p) {
			hu = atoi(p+3);
			if (hu >= 0) setcnt++;
		} else if (strstr(p, "w=") == p) {
			w = atoi(p+2);
			if (w > 0) setcnt++;
		} else if (strstr(p, "h=") == p) {
			h = atoi(p+2);
			if (h > 0) setcnt++;
		} else if (strstr(p, "bpp=") == p) {
			b = atoi(p+4);
			if (b > 0) setcnt++;
		} else if (strstr(p, "fmt=") == p) {
			fmt = strdup(p+4);
			setcnt++;
		} else if (strstr(p, "tun=") == p) {
			tun = strdup(p+4);
			setcnt++;
		} else if (strstr(p, "inp=") == p) {
			inp = strdup(p+4);
			setcnt++;
		} else if (strstr(p, "sta=") == p) {
			sta = atoi(p+4);
			setcnt++;
		}
		p = strtok(NULL, ",");
	}
	free(str);
	if (! setcnt) {
		return;
	}
	if (*fd < 0) {
		*fd = open_dev(dev);
	}
	if (*fd < 0) {
		return;
	}
	v4l1_cap = v4l1_query(*fd, 1);
	v4l2_cap = v4l2_query(*fd, 1);

	if (v4l1_cap && ! ignore_all) {
		if (br >= 0) v4l1_picture.brightness = v4l1_val(br);
		if (hu >= 0) v4l1_picture.hue        = v4l1_val(hu);
		if (co >= 0) v4l1_picture.colour     = v4l1_val(co);
		if (cn >= 0) v4l1_picture.contrast   = v4l1_val(cn);

		fprintf(stderr, "calling V4L_1: VIDIOCSPICT\n");
		if (ioctl(*fd, VIDIOCSPICT, &v4l1_picture) == -1) {
			perror("ioctl VIDIOCSPICT");
		}

		if (fmt) {
			v4l1_setfmt(*fd, fmt);
		} else if (b > 0 && b != v4l1_picture.depth) {
			if (b == 8) {
				v4l1_setfmt(*fd, "HI240");
			} else if (b == 16) {
				v4l1_setfmt(*fd, "RGB565");
			} else if (b == 24) {
				v4l1_setfmt(*fd, "RGB24");
			} else if (b == 32) {
				v4l1_setfmt(*fd, "RGB32");
			}
		}

		v4l1_resize(*fd, w, h);

		if (tun) {
			int mode = -1;
			if (!strcasecmp(tun, "PAL")) {
				mode = VIDEO_MODE_PAL;
			} else if (!strcasecmp(tun, "NTSC")) {
				mode = VIDEO_MODE_NTSC;
			} else if (!strcasecmp(tun, "SECAM")) {
				mode = VIDEO_MODE_SECAM;
			} else if (!strcasecmp(tun, "AUTO")) {
				mode = VIDEO_MODE_AUTO;
			}
			if (mode != -1) {
				int i;
				for (i=0; i< v4l1_capability.channels; i++) {
					memset(&v4l1_channel, 0, sizeof(v4l1_channel));
					v4l1_channel.channel = i;
					if (ioctl(*fd, VIDIOCGCHAN, &v4l1_channel) == -1) {
						continue;
					}
					if (! v4l1_channel.tuners) {
						continue;
					}
					if (v4l1_channel.norm == mode) {
						continue;
					}
					v4l1_channel.norm = mode;
					ioctl(*fd, VIDIOCSCHAN, &v4l1_channel);
				}
			}
		}
		if (inp) {
			char s[2];
			int i, chan = -1;

			s[0] = inp[0];
			s[1] = '\0';
			if (strstr("0123456789", s)) {
				chan = atoi(inp);
			} else {
				for (i=0; i< v4l1_capability.channels; i++) {
					memset(&v4l1_channel, 0, sizeof(v4l1_channel));
					v4l1_channel.channel = i;
					if (ioctl(*fd, VIDIOCGCHAN, &v4l1_channel) == -1) {
						continue;
					}
					if (!strcmp(v4l1_channel.name, inp)) {
						chan = i;
						break;
					}
				}
			}
			v4l1_set_input(*fd, chan);
		}
		if (sta >= 0) {
			unsigned long freq = lookup_freq(sta);
			v4l1_setfreq(*fd, freq, 1);
		}
	}
	v4l1_cap = v4l1_query(*fd, 1);
	v4l2_cap = v4l2_query(*fd, 1);
#else
	if (!dev || !settings || !fd) {}
	return;
#endif
}

static double dval = 0.05;

static int v4l1_dpct(int old, int d) {
	int newval, max = V4L1_MAX; 
	
	/* -1 and 1 are special cases for "small increments" */
	if (d == -1) {
		newval = old - (int) (dval * max);
	} else if (d == 1) {
		newval = old + (int) (dval * max);
	} else {
		newval = (d * max)/100;
	}
	if (newval < 0) {
		newval = 0;
	}
	if (newval > max) {
		newval = max;
	}
	return newval;
}

static void v4l_requery(void) {
	if (raw_fb_fd < 0) {
		return;
	}
	v4l1_cap = v4l1_query(raw_fb_fd, 1);
	v4l2_cap = v4l2_query(raw_fb_fd, 1);
}
	
static void v4l_br(int b) {
#ifdef V4L_OK
	int old = v4l1_picture.brightness;

	v4l1_picture.brightness = v4l1_dpct(old, b);
	ioctl(raw_fb_fd, VIDIOCSPICT, &v4l1_picture);
	v4l_requery();
#else
	if (!b) {}
#endif
}

static void v4l_hu(int b) {
#ifdef V4L_OK
	int old = v4l1_picture.hue;

	v4l1_picture.hue = v4l1_dpct(old, b);
	ioctl(raw_fb_fd, VIDIOCSPICT, &v4l1_picture);
	v4l_requery();
#else
	if (!b) {}
#endif
}

static void v4l_co(int b) {
#ifdef V4L_OK
	int old = v4l1_picture.colour;

	v4l1_picture.colour = v4l1_dpct(old, b);
	ioctl(raw_fb_fd, VIDIOCSPICT, &v4l1_picture);
	v4l_requery();
#else
	if (!b) {}
#endif
}

static void v4l_cn(int b) {
#ifdef V4L_OK
	int old = v4l1_picture.contrast;

	v4l1_picture.contrast = v4l1_dpct(old, b);
	ioctl(raw_fb_fd, VIDIOCSPICT, &v4l1_picture);
	v4l_requery();
#else
	if (!b) {}
#endif
}

static void v4l_sz(int b) {
#ifdef V4L_OK
	int w_old = v4l1_window.width;
	int h_old = v4l1_window.height;
	int w, h;

	if (w_old == 0) {
		w_old = 160;
	}
	if (h_old == 0) {
		h_old = 120;
	}

	if (b == 1) {
		w = w_old + (int) (0.15 * w_old); 
		h = h_old + (int) (0.15 * h_old); 
	} else if (b == -1) {
		w = w_old - (int) (0.15 * w_old); 
		h = h_old - (int) (0.15 * h_old); 
	} else {
		return;
	}

	if (! v4l1_resize(raw_fb_fd, w, h)) {
		return;
	}

	v4l_requery();

	push_black_screen(4);

	ignore_all = 1;
	do_new_fb(1);
	ignore_all = 0;
#else
	if (!b) {}
#endif
}

static void v4l_sta(int sta) {
#ifdef V4L_OK
	unsigned long freq = 0;
	int cur = lookup_station(last_freq);

	if (! last_freq) {
		if (sta == 0 || sta == -1) {
			sta = 11;
		}
	}

	if (sta == -1) {
		while (cur > 0) {
			freq = lookup_freq(--cur);
			if (freq) {
				break;
			}
		}
	} else if (sta == 0) {
		while (cur < CHANNEL_MAX - 1) {
			freq = lookup_freq(++cur);
			if (freq) {
				break;
			}
		}
	} else {
		freq = lookup_freq(sta);
		cur = sta;
	}
	fprintf(stderr, "to station %d / %d\n", cur, (int) freq);
	v4l1_setfreq(raw_fb_fd, freq, 0);
#else
	if (!sta) {}
#endif
}

static void v4l_inp(int inp) {
#ifdef V4L_OK
	int next = -1;
	if (inp == -1) {
		inp = last_channel + 1;
		if (inp >= v4l1_capability.channels) {
			inp = 0;
		}
		next = inp;
	} else if (inp == -2) {
		inp = last_channel - 1;
		if (inp < 0) {
			inp = v4l1_capability.channels - 1;
		}
		next = inp;
	} else {
		next = inp;
	}
	v4l1_set_input(raw_fb_fd, next);
#else
	if (!inp) {}
#endif
}

static void v4l_fmt(char *fmt) {
	if (v4l1_setfmt(raw_fb_fd, fmt)) {
		v4l_requery();

		ignore_all = 1;
		do_new_fb(1);
		ignore_all = 0;
	}
}

void v4l_key_command(rfbBool down, rfbKeySym keysym, rfbClientPtr client) {
	allowed_input_t input;

	if (raw_fb_fd < 0) {
		return;		
	}
	if (! down) {
		return;
	}
	if (view_only) {
		return;
	}
	get_allowed_input(client, &input);
	if (! input.keystroke) {
		return;
	}

	if (keysym == XK_b) {
		v4l_br(-1);
	} else if (keysym == XK_B) {
		v4l_br(+1);
	} else if (keysym == XK_h) {
		v4l_hu(-1);
	} else if (keysym == XK_H) {
		v4l_hu(+1);
	} else if (keysym == XK_c) {
		v4l_co(-1);
	} else if (keysym == XK_C) {
		v4l_co(+1);
	} else if (keysym == XK_n) {
		v4l_cn(-1);
	} else if (keysym == XK_N) {
		v4l_cn(+1);
	} else if (keysym == XK_s) {
		v4l_sz(-1);
	} else if (keysym == XK_S) {
		v4l_sz(+1);
	} else if (keysym == XK_i) {
		v4l_inp(-1);
	} else if (keysym == XK_I) {
		v4l_inp(-2);
	} else if (keysym == XK_Up) {
		v4l_sta(+0);
	} else if (keysym == XK_Down) {
		v4l_sta(-1);
	} else if (keysym == XK_F1) {
		v4l_fmt("HI240");
	} else if (keysym == XK_F2) {
		v4l_fmt("RGB565");
	} else if (keysym == XK_F3) {
		v4l_fmt("RGB24");
	} else if (keysym == XK_F4) {
		v4l_fmt("RGB32");
	} else if (keysym == XK_F5) {
		v4l_fmt("RGB555");
	} else if (keysym == XK_F6) {
		v4l_fmt("GREY");
	}
	if (client) {}
}


void v4l_pointer_command(int mask, int x, int y, rfbClientPtr client) {
	/* do not forget viewonly perms */
	if (mask || x || y || client) {}
}

static int colon_n(char *line) {
	char *q;
	int n;
	q = strrchr(line, ':');
	if (! q) {
		return 0;
	}
	q = lblanks(q+1);
	if (sscanf(q, "%d", &n) == 1) {
		return n;
	}
	return 0;
}

static char *colon_str(char *line) {
	char *q, *p, *t;
	q = strrchr(line, ':');
	if (! q) {
		return strdup("");
	}
	q = lblanks(q+1);
	p = strpbrk(q, " \t\n");
	if (p) {
		*p = '\0';
	}
	t = strdup(q);
	*p = '\n';
	return t;
}

static char *colon_tag(char *line) {
	char *q, *p, *t;
	q = strrchr(line, '[');
	if (! q) {
		return strdup("");
	}
	q++;
	p = strrchr(q, ']');
	if (! p) {
		return strdup("");
	}
	*p = '\0';
	t = strdup(q);
	*p = ']';
	return t;
}

static void lookup_rgb(char *fmt, int *bits, int *rev) {
	int tb, tr;

	if (v4l2_lu_palette_str(fmt, &tb, &tr)) {
		*bits = tb;
		*rev  = tr;
		return;
	}
	if (v4l1_lu_palette_str(fmt, &tb, &tr)) {
		*bits = tb;
		*rev  = tr;
		return;
	}
}

static char *v4l1_lu_palette(unsigned short palette) {
	switch(palette) {
#ifdef V4L_OK
		case VIDEO_PALETTE_GREY:	return "GREY";
		case VIDEO_PALETTE_HI240:	return "HI240";
		case VIDEO_PALETTE_RGB565:	return "RGB565";
		case VIDEO_PALETTE_RGB24:	return "RGB24";
		case VIDEO_PALETTE_RGB32:	return "RGB32";
		case VIDEO_PALETTE_RGB555:	return "RGB555";
		case VIDEO_PALETTE_YUV422:	return "YUV422";
		case VIDEO_PALETTE_YUYV:	return "YUYV";
		case VIDEO_PALETTE_UYVY:	return "UYVY";
		case VIDEO_PALETTE_YUV420:	return "YUV420";
		case VIDEO_PALETTE_YUV411:	return "YUV411";
		case VIDEO_PALETTE_RAW:		return "RAW";
		case VIDEO_PALETTE_YUV422P:	return "YUV422P";
		case VIDEO_PALETTE_YUV411P:	return "YUV411P";
		case VIDEO_PALETTE_YUV420P:	return "YUV420P";
		case VIDEO_PALETTE_YUV410P:	return "YUV410P";
#endif
		default:			return "unknown";
	}
}

static unsigned short v4l1_lu_palette_str(char *name, int *bits, int *rev) {
#ifdef V4L_OK
	*rev = 0;
	if (!strcmp(name, "RGB555")) {
		*bits = 16;
		return VIDEO_PALETTE_RGB555;
	} else if (!strcmp(name, "RGB565")) {
		*bits = 16;
		return VIDEO_PALETTE_RGB565;
	} else if (!strcmp(name, "RGB24")) {
		*bits = 24;
		return VIDEO_PALETTE_RGB24;
	} else if (!strcmp(name, "RGB32")) {
		*bits = 32;
		return VIDEO_PALETTE_RGB32;
	} else if (!strcmp(name, "HI240")) {
		*bits = 8;
		return VIDEO_PALETTE_HI240;
	} else if (!strcmp(name, "GREY")) {
		*bits = 8;
		return VIDEO_PALETTE_GREY;
	}
#else
	if (!name || !bits || !rev) {}
#endif
	return 0;
}

static char *v4l2_lu_palette(unsigned int fmt) {
	switch(fmt) {
#if defined(V4L_OK) && HAVE_V4L2
		case V4L2_PIX_FMT_RGB332:	return "RGB332";
		case V4L2_PIX_FMT_RGB555:	return "RGB555";
		case V4L2_PIX_FMT_RGB565:	return "RGB565";
		case V4L2_PIX_FMT_RGB555X:	return "RGB555X";
		case V4L2_PIX_FMT_RGB565X:	return "RGB565X";
		case V4L2_PIX_FMT_BGR24:	return "BGR24";
		case V4L2_PIX_FMT_RGB24:	return "RGB24";
		case V4L2_PIX_FMT_BGR32:	return "BGR32";
		case V4L2_PIX_FMT_RGB32:	return "RGB32";
		case V4L2_PIX_FMT_GREY:		return "GREY";
		case V4L2_PIX_FMT_YVU410:	return "YVU410";
		case V4L2_PIX_FMT_YVU420:	return "YVU420";
		case V4L2_PIX_FMT_YUYV:		return "YUYV";
		case V4L2_PIX_FMT_UYVY:		return "UYVY";
		case V4L2_PIX_FMT_YUV422P:	return "YUV422P";
		case V4L2_PIX_FMT_YUV411P:	return "YUV411P";
		case V4L2_PIX_FMT_Y41P:		return "Y41P";
		case V4L2_PIX_FMT_NV12:		return "NV12";
		case V4L2_PIX_FMT_NV21:		return "NV21";
		case V4L2_PIX_FMT_YUV410:	return "YUV410";
		case V4L2_PIX_FMT_YUV420:	return "YUV420";
		case V4L2_PIX_FMT_YYUV:		return "YYUV";
		case V4L2_PIX_FMT_HI240:	return "HI240";
		case V4L2_PIX_FMT_MJPEG:	return "MJPEG";
		case V4L2_PIX_FMT_JPEG:		return "JPEG";
		case V4L2_PIX_FMT_DV:		return "DV";
		case V4L2_PIX_FMT_MPEG:		return "MPEG";
#endif
		default:			return "unknown";
	}
}

static unsigned int v4l2_lu_palette_str(char *name, int *bits, int *rev) {
#if defined(V4L_OK) && HAVE_V4L2
	if (!strcmp(name, "RGB1") || !strcmp(name, "RGB332")) {
		*bits = 8;
		*rev = 0;
		return V4L2_PIX_FMT_RGB332;
	} else if (!strcmp(name, "RGBO") || !strcmp(name, "RGB555")) {
		*bits = 16;
		*rev = 0;
		return V4L2_PIX_FMT_RGB555;
	} else if (!strcmp(name, "RGBP") || !strcmp(name, "RGB565")) {
		*bits = 16;
		*rev = 0;
		return V4L2_PIX_FMT_RGB565;
	} else if (!strcmp(name, "RGBQ") || !strcmp(name, "RGB555X")) {
		*bits = 16;
		*rev = 1;
		return V4L2_PIX_FMT_RGB555X;
	} else if (!strcmp(name, "RGBR") || !strcmp(name, "RGB565X")) {
		*bits = 16;
		*rev = 1;
		return V4L2_PIX_FMT_RGB565X;
	} else if (!strcmp(name, "BGR3") || !strcmp(name, "BGR24")) {
		*bits = 24;
		*rev = 1;
		return V4L2_PIX_FMT_BGR24;
	} else if (!strcmp(name, "RGB3") || !strcmp(name, "RGB24")) {
		*bits = 24;
		*rev = 0;
		return V4L2_PIX_FMT_RGB24;
	} else if (!strcmp(name, "BGR4") || !strcmp(name, "BGR32")) {
		*bits = 32;
		*rev = 1;
		return V4L2_PIX_FMT_BGR32;
	} else if (!strcmp(name, "RGB4") || !strcmp(name, "RGB32")) {
		*bits = 32;
		*rev = 0;
		return V4L2_PIX_FMT_RGB32;
	} else if (!strcmp(name, "GREY")) {
		*bits = 8;
		*rev = 0;
		return V4L2_PIX_FMT_GREY;
	}
#else
	if (!name || !bits || !rev) {}
#endif
	return 0;
}

static int v4l1_query(int fd, int v) {
#ifdef V4L_OK
	unsigned int i;

	memset(&v4l1_capability, 0, sizeof(v4l1_capability));
	memset(&v4l1_channel,    0, sizeof(v4l1_channel));
	memset(&v4l1_tuner,      0, sizeof(v4l1_tuner));
	memset(&v4l1_picture,    0, sizeof(v4l1_picture));
	memset(&v4l1_window,     0, sizeof(v4l1_window));

	if (v) fprintf(stderr, "\nV4L_1 query:\n");
#ifdef VIDIOCGCAP
	if (ioctl(fd, VIDIOCGCAP, &v4l1_capability) == -1) {
		perror("ioctl VIDIOCGCAP");
		fprintf(stderr, "\n");
		return 0;
	}
#else
	return 0;
#endif
	if (v) fprintf(stderr, "v4l-1 capability:\n");
	if (v) fprintf(stderr, "     name:      %s\n", v4l1_capability.name);
	if (v) fprintf(stderr, "     channels:  %d\n", v4l1_capability.channels);
	if (v) fprintf(stderr, "     audios:    %d\n", v4l1_capability.audios);
	if (v) fprintf(stderr, "     maxwidth:  %d\n", v4l1_capability.maxwidth);
	if (v) fprintf(stderr, "     maxheight: %d\n", v4l1_capability.maxheight);
	if (v) fprintf(stderr, "     minwidth:  %d\n", v4l1_capability.minwidth);
	if (v) fprintf(stderr, "     minheight: %d\n", v4l1_capability.minheight);

	for (i=0; (int) i < v4l1_capability.channels; i++) {
		char *type = "unknown";
		memset(&v4l1_channel, 0, sizeof(v4l1_channel));
		v4l1_channel.channel = i;
		if (ioctl(fd, VIDIOCGCHAN, &v4l1_channel) == -1) {
			perror("ioctl VIDIOCGCHAN");
			continue;
		}
		if (v4l1_channel.type == VIDEO_TYPE_TV) {
			type = "TV";
		} else if (v4l1_channel.type == VIDEO_TYPE_CAMERA) {
			type = "CAMERA";
		}
		if (v) fprintf(stderr, "     channel[%d]: %s\ttuners: %d norm: %d type: %d  %s\n",
		    i, v4l1_channel.name, v4l1_channel.tuners, v4l1_channel.norm,
		    v4l1_channel.type, type);
	}

	memset(&v4l1_tuner, 0, sizeof(v4l1_tuner));
	if (ioctl(fd, VIDIOCGTUNER, &v4l1_tuner) != -1) {
		char *mode = "unknown";
		if (v4l1_tuner.mode == VIDEO_MODE_PAL) {
			mode = "PAL";
		} else if (v4l1_tuner.mode == VIDEO_MODE_NTSC) {
			mode = "NTSC";
		} else if (v4l1_tuner.mode == VIDEO_MODE_SECAM) {
			mode = "SECAM";
		} else if (v4l1_tuner.mode == VIDEO_MODE_AUTO) {
			mode = "AUTO";
		}

		if (v) fprintf(stderr, "     tuner[%d]:   %s\tflags: 0x%x mode: %s\n",
		    v4l1_tuner.tuner, v4l1_tuner.name, v4l1_tuner.flags, mode);
		
	}

	if (ioctl(fd, VIDIOCGPICT, &v4l1_picture) == -1) {
		perror("ioctl VIDIOCGCHAN");
		return 0;
	}
	if (v) fprintf(stderr, "v4l-1 picture:\n");
	if (v) fprintf(stderr, "     brightness:  %d\n", v4l1_picture.brightness);
	if (v) fprintf(stderr, "     hue:         %d\n", v4l1_picture.hue);
	if (v) fprintf(stderr, "     colour:      %d\n", v4l1_picture.colour);
	if (v) fprintf(stderr, "     contrast:    %d\n", v4l1_picture.contrast);
	if (v) fprintf(stderr, "     whiteness:   %d\n", v4l1_picture.whiteness);
	if (v) fprintf(stderr, "     depth:       %d\n", v4l1_picture.depth);
	if (v) fprintf(stderr, "     palette:     %d  %s\n", v4l1_picture.palette,
	    v4l1_lu_palette(v4l1_picture.palette));
	
	if (ioctl(fd, VIDIOCGWIN, &v4l1_window) == -1) {
		perror("ioctl VIDIOCGWIN");
		if (v) fprintf(stderr, "\n");
		return 0;
	}
	if (v) fprintf(stderr, "v4l-1 window:\n");
	if (v) fprintf(stderr, "     x:           %d\n", v4l1_window.x);
	if (v) fprintf(stderr, "     y:           %d\n", v4l1_window.y);
	if (v) fprintf(stderr, "     width:       %d\n", v4l1_window.width);
	if (v) fprintf(stderr, "     height:      %d\n", v4l1_window.height);
	if (v) fprintf(stderr, "     chromakey:   %d\n", v4l1_window.chromakey);
	if (v) fprintf(stderr, "\n");

	return 1;
#else
	if (!fd || !v) {}
	return 0;
#endif	/* V4L_OK */

}
static int v4l2_query(int fd, int v) {
#if defined(V4L_OK) && HAVE_V4L2
	unsigned int i;

	memset(&v4l2_capability, 0, sizeof(v4l2_capability));
	memset(&v4l2_input,      0, sizeof(v4l2_input));
	memset(&v4l2_tuner,      0, sizeof(v4l2_tuner));
	memset(&v4l2_fmtdesc,    0, sizeof(v4l2_fmtdesc));
	memset(&v4l2_format,     0, sizeof(v4l2_format));

	if (v) fprintf(stderr, "\nV4L_2 query:\n");
#ifdef VIDIOC_QUERYCAP
	if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_capability) == -1) {
		perror("ioctl VIDIOC_QUERYCAP");
		if (v) fprintf(stderr, "\n");
		return 0;
	}
#else
	return 0;
#endif

	if (v) fprintf(stderr, "v4l-2 capability:\n");
	if (v) fprintf(stderr, "    driver:       %s\n", v4l2_capability.driver);
	if (v) fprintf(stderr, "    card:         %s\n", v4l2_capability.card);
	if (v) fprintf(stderr, "    bus_info:     %s\n", v4l2_capability.bus_info);
	if (v) fprintf(stderr, "    version:      %d\n", v4l2_capability.version);
	if (v) fprintf(stderr, "    capabilities: %u\n", v4l2_capability.capabilities);

	for (i=0; ; i++) {
		memset(&v4l2_input, 0, sizeof(v4l2_input));
		v4l2_input.index = i;
		if (ioctl(fd, VIDIOC_ENUMINPUT, &v4l2_input) == -1) {
			break;
		}
		if (v) fprintf(stderr, "    input[%d]: %s\ttype: %d tuner: %d\n",
		    i, v4l2_input.name, v4l2_input.type, v4l2_input.tuner);
	}
	if (v4l2_capability.capabilities & V4L2_CAP_TUNER) {
		for (i=0; ; i++) {
			memset(&v4l2_tuner, 0, sizeof(v4l2_tuner));
			v4l2_tuner.index = i;
			if (ioctl(fd, VIDIOC_G_TUNER, &v4l2_tuner) == -1) {
				break;
			}
			if (v) fprintf(stderr, "    tuner[%d]: %s\ttype: %d\n",
			    i, v4l2_tuner.name, v4l2_tuner.type);
		}
	}
	if (v4l2_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
		for (i=0; ; i++) {
			memset(&v4l2_fmtdesc, 0, sizeof(v4l2_fmtdesc));
			v4l2_fmtdesc.index = i;
			v4l2_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			
			if (ioctl(fd, VIDIOC_ENUM_FMT, &v4l2_fmtdesc) == -1) {
				break;
			}
			if (v) fprintf(stderr, "    fmtdesc[%d]: %s\ttype: %d"
			    " pixelformat: %d\n",
			    i, v4l2_fmtdesc.description, v4l2_fmtdesc.type,
			    v4l2_fmtdesc.pixelformat);
		}
		v4l2_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(fd, VIDIOC_G_FMT, &v4l2_format) == -1) {
			perror("ioctl VIDIOC_G_FMT");
		} else {
			if (v) fprintf(stderr, "    width:  %d\n", v4l2_format.fmt.pix.width);
			if (v) fprintf(stderr, "    height: %d\n", v4l2_format.fmt.pix.height);
			if (v) fprintf(stderr, "    format: %u %s\n",
			    v4l2_format.fmt.pix.pixelformat,
			    v4l2_lu_palette(v4l2_format.fmt.pix.pixelformat));
		}
	}

	return 1;
#else
	if (!fd || !v) {}
	return 0;
#endif	/* V4L_OK && HAVE_V4L2 */

}

static int open_dev(char *dev) {
	int dfd = -1;
	if (! dev) {
		return dfd;
	}
	dfd = open(dev, O_RDWR);
	if (dfd < 0) {
		rfbLog("failed to rawfb file: %s O_RDWR\n", dev);
		rfbLogPerror("open");
		dfd = open(dev, O_RDONLY);
	}
	if (dfd < 0) {
		rfbLog("failed to rawfb file: %s\n", dev);
		rfbLog("failed to rawfb file: %s O_RDONLY\n", dev);
		rfbLogPerror("open");
	}
	return dfd;
}

static char *guess_via_v4l(char *dev, int *fd) {
#ifdef V4L_OK
	int dfd;
	
	if (*fd < 0) {
		dfd = open_dev(dev);
		*fd = dfd;
	}
	dfd = *fd;
	if (dfd < 0) {
		return NULL;
	}
	if (v4l1_cap < 0) {
		v4l1_cap = v4l1_query(dfd, 1);
	}
	if (v4l2_cap < 0) {
		v4l2_cap = v4l2_query(dfd, 1);
	}

	if (v4l2_cap) {
#if HAVE_V4L2
		int g_w = v4l2_format.fmt.pix.width;
		int g_h = v4l2_format.fmt.pix.height;
		int g_d = 0, g_rev;

		if (v4l2_format.fmt.pix.pixelformat) {
			char *str = v4l2_lu_palette(v4l2_format.fmt.pix.pixelformat);
			if (strcmp(str, "unknown")) {
				v4l2_lu_palette_str(str, &g_d, &g_rev);
			}
		}
		
		if (g_w > 0 && g_h > 0 && g_d > 0) {
			char *atparms = (char *) malloc(200);
			char *pal = v4l2_lu_palette(v4l2_format.fmt.pix.pixelformat);
			sprintf(atparms, "%dx%dx%d", g_w, g_h, g_d);
			if (strstr(pal, "RGB555")) {
				strcat(atparms, ":7c00/3e0/1f");
			}
			*fd = dfd;
			return atparms;
		}
#endif
	}
	if (v4l1_cap) {
		int g_w = v4l1_window.width;
		int g_h = v4l1_window.height;
		int g_d = v4l1_picture.depth;
		int g_rev;
		if (g_d == 0) {
			char *str = v4l1_lu_palette(v4l1_picture.palette);
			if (strcmp(str, "unknown")) {
				v4l1_lu_palette_str(str, &g_d, &g_rev);
			}
		}
if (0) fprintf(stderr, "v4l1: %d %d %d\n", g_w, g_h, g_d);
		if (g_w > 0 && g_h > 0 && g_d > 0) {
			char *atparms = (char *) malloc(200);
			char *pal = v4l1_lu_palette(v4l1_picture.palette);
			fprintf(stderr, "palette: %s\n", pal);
			sprintf(atparms, "%dx%dx%d", g_w, g_h, g_d);
			if (strstr(pal, "RGB555")) {
				strcat(atparms, ":7c00/3e0/1f");
			}
			*fd = dfd;
			return atparms;
		}
	}

	/* failure */
	close(dfd);
	return NULL;
#else
	if (!dev || !fd) {}
	return NULL;
#endif
}

static char *guess_via_v4l_info(char *dev, int *fd) {
	char *atparms, *cmd;
	char line[1024], tmp[] = "/tmp/x11vnc-tmp.XXXXXX";
	FILE *out;
	int tmp_fd, len, rc, curr = 0;
	int g_w = 0, g_h = 0, g_b = 0, mask_rev = 0;
	char *g_fmt = NULL;

	if (*fd) {}

	/* v4l-info */
	if (no_external_cmds || !cmd_ok("v4l-info")) {
		rfbLog("guess_via_v4l_info: cannot run external "
		    "command: v4l-info\n");
		return NULL;
	}

	if (strchr(dev, '\'')) {
		rfbLog("guess_via_v4l_info: bad dev string: %s\n", dev);
		return NULL;
	}

	tmp_fd = mkstemp(tmp);
	if (tmp_fd < 0) {
		return NULL;
	}

	len =  strlen("v4l-info")+1+1+strlen(dev)+1+1+1+1+strlen(tmp)+1; 
	cmd = (char *) malloc(len);
	rfbLog("guess_via_v4l_info running: v4l-info '%s'\n", dev);
	sprintf(cmd, "v4l-info '%s' > %s", dev, tmp);

	close(tmp_fd);
	close_exec_fds();
	rc = system(cmd);
	if (rc != 0) {
		unlink(tmp);
		return NULL;
	}

	out = fopen(tmp, "r");
	if (out == NULL) {
		unlink(tmp);
		return NULL;
	}
	
	curr = 0;
	while (fgets(line, 1024, out) != NULL) {
		char *lb = lblanks(line);
		if (strstr(line, "video capture") == line) {
			curr = C_VIDEO_CAPTURE;
		} else if (strstr(line, "picture") == line) {
			curr = C_PICTURE;
		} else if (strstr(line, "window") == line) {
			curr = C_WINDOW;
		}

if (0) fprintf(stderr, "lb: %s", lb);

		if (curr == C_VIDEO_CAPTURE) {
			if (strstr(lb, "pixelformat ") == lb) {
				fprintf(stderr, "%s", line);
			} else if (strstr(lb, "fmt.pix.width ") == lb) {
				if (! g_w) {
					g_w = colon_n(line);
				}
			} else if (strstr(lb, "fmt.pix.height ") == lb) {
				if (! g_h) {
					g_h = colon_n(line);
				}
			} else if (strstr(lb, "fmt.pix.pixelformat ") == lb) {
				if (! g_fmt) {
					g_fmt = colon_tag(line);
				}
			}
		} else if (curr == C_PICTURE) {
			if (strstr(lb, "depth ") == lb) {
				if (! g_b) {
					g_b = colon_n(line);
				}
			} else if (strstr(lb, "palette ") == lb) {
				if (! g_fmt) {
					g_fmt = colon_str(line);
				}
			}
		} else if (curr == C_WINDOW) {
			if (strstr(lb, "width ") == lb) {
				if (! g_w) {
					g_w = colon_n(line);
				}
			} else if (strstr(lb, "height ") == lb) {
				if (! g_h) {
					g_h = colon_n(line);
				}
			}
		}
	}
	fclose(out);
	unlink(tmp);

	if (! g_w) {
		rfbLog("could not guess device width.\n");
		return NULL;
	}
	rfbLog("guessed device width:  %d\n", g_w);

	if (! g_h) {
		rfbLog("could not guess device height.\n");
		return NULL;
	}
	rfbLog("guessed device height: %d\n", g_h);

	if (g_fmt) {
		rfbLog("guessed pixel fmt:     %s\n", g_fmt);
		lookup_rgb(g_fmt, &g_b, &mask_rev);
	}
	if (! g_b) {
		rfbLog("could not guess device bpp.\n");
		return NULL;
	}
	rfbLog("guessed device bpp:    %d\n", g_b);

	atparms = (char *) malloc(100);
	sprintf(atparms, "%dx%dx%d", g_w, g_h, g_b);
	return atparms;
}

static void parse_str(char *str, char **dev, char **settings, char **atparms) {
	char *p, *q, *s = NULL;

	q = strchr(str, '@');
	if (q && strlen(q+1) > 0) {
		/* ends @WxHXB... */
		*atparms = strdup(q+1);
		*q = '\0';
	}

	q = strchr(str, ':');
	if (q && strlen(q+1) > 0) {
		/* ends :br=N,w=N... */
		s = strdup(q+1);
		*settings = s;
		*q = '\0';
	}

	if (s != NULL) {
		/* see if fn=filename */
		q = strstr(s, "fn=");
		if (q) {
			q += strlen("fn=");
			p = strchr(q, ',');
			if (p) {
				*p = '\0';
				*dev = strdup(q); 
				*p = ',';
			} else {
				*dev = strdup(q); 
			}
			rfbLog("set video device to: '%s'\n", *dev);
		}
	}

	if (*dev == NULL) {
		struct stat sbuf;
		s = (char *) malloc(strlen("/dev/") + strlen(str) + 2);
		if (strstr(str, "/dev/") == str) {
			sprintf(s, "%s", str);
		} else {
			sprintf(s, "/dev/%s", str);
		} 
		rfbLog("Checking existence of '%s'\n", s);
                if (stat(s, &sbuf) != 0) {
			rfbLogPerror("stat");
			strcat(s, "0");
			rfbLog("switching to '%s'\n", s);
		}
                if (stat(s, &sbuf) != 0) {
			rfbLogPerror("stat");
			rfbLog("You will need to specify the video device more explicity.\n");
		}

		*dev = s;
		rfbLog("set video device to: '%s'\n", *dev);
	}
}

char *v4l_guess(char *str, int *fd) {
	char *dev = NULL, *settings = NULL, *atparms = NULL;

	parse_str(str, &dev, &settings, &atparms);

	init_freqs();

	v4l1_cap = -1;
	v4l2_cap = -1;
	*fd = -1;

	if (dev == NULL) {
		rfbLog("v4l_guess: could not find device in: %s\n", str);
		return NULL;
	}

	if (settings) {
		apply_settings(dev, settings, fd);
	}

	if (atparms) {
		/* use user's parameters. */
		char *t = (char *) malloc(5+strlen(dev)+1+strlen(atparms)+1);
		sprintf(t, "snap:%s@%s", dev, atparms);
		return t;
	}

	/* try to query the device for parameters. */
	atparms = guess_via_v4l(dev, fd);
	if (atparms == NULL) {
		/* try again with v4l-info(1) */
		atparms = guess_via_v4l_info(dev, fd);
	}

	if (atparms == NULL) {
		/* bad news */
		if (*fd >= 0) {
			close(*fd);
		}
		*fd = -1;
		return NULL;
	} else {
		char *t = (char *) malloc(5+strlen(dev)+1+strlen(atparms)+1);
		sprintf(t, "snap:%s@%s", dev, atparms);
		return t;
	}
}

static unsigned long lookup_freqtab(int sta) {

	if (sta >= CHANNEL_MAX) {
		return (unsigned long) sta;
	}
	if (sta < 0 || sta >= CHANNEL_MAX) {
		return 0;
	}
	return custom_freq[sta];
}

static unsigned long lookup_freq(int sta) {
	if (freqtab) {
		return lookup_freqtab(sta);
	}
	if (sta >= CHANNEL_MAX) {
		return (unsigned long) sta;
	}
	if (sta < 1 || sta > 125) {
		return 0;
	}
	return ntsc_cable[sta];
}

static int lookup_station(unsigned long freq) {
	int i;
	if (freqtab) {
		for (i = 0; i < CHANNEL_MAX; i++) {
if (0) fprintf(stderr, "%lu %lu\n", freq, custom_freq[i]);
			if (freq == custom_freq[i]) {
				return i;
			}
		}
	} else {
		for (i = 1; i <= 125; i++) {
			if (freq == ntsc_cable[i]) {
				return i;
			}
		}
	}
	return 0;
}

static void init_freqtab(char *file) {
	char *p, *q, *dir, *file2;
	char line[1024], inc[1024];
	char *text, *str;
	int size = 0, maxn, extra, currn;
	FILE *in1, *in2;
	static int v = 1;
	if (quiet) {
		v = 0;
	}

	/* YUCK */

	dir = strdup(file);
	q = strrchr(dir, '/');
	if (q) {
		*(q+1) = '\0';
	} else {
		free(dir);
		dir = strdup("./");
	}
	file2 = (char *) malloc(strlen(dir) + 1024 + 1);
	in1 = fopen(file, "r");
	if (in1 == NULL) {
		rfbLog("error opening freqtab: %s\n", file);
		clean_up_exit(1);
	}
	if (v) fprintf(stderr, "loading frequencies from: %s\n", file);
	while (fgets(line, 1024, in1) != NULL) {
		char *lb;
		char line2[1024];
		size += strlen(line);
		lb = lblanks(line);
		if (strstr(lb, "#include") == lb && 
		    sscanf(lb, "#include %s", inc) == 1) {
			char *q, *s = inc;
			if (s[0] == '"') {
				s++;
			}
			q = strrchr(s, '"');
			if (q) {
				*q = '\0';
			}
			sprintf(file2, "%s%s", dir, s);
			in2 = fopen(file2, "r");
			if (in2 == NULL) {
				rfbLog("error opening freqtab include: %s %s\n", line, file2);
				clean_up_exit(1);
			}
			if (v) fprintf(stderr, "loading frequencies from: %s\n", file2);
			while (fgets(line2, 1024, in2) != NULL) {
				size += strlen(line2);
			}
			fclose(in2);
		}
	}
	fclose(in1);

	size = 4*(size + 10000);

	text = (char *) malloc(size);

	text[0] = '\0';

	in1 = fopen(file, "r");
	if (in1 == NULL) {
		rfbLog("error opening freqtab: %s\n", file);
		clean_up_exit(1);
	}
	while (fgets(line, 1024, in1) != NULL) {
		char *lb;
		char line2[1024];
		lb = lblanks(line);
		if (lb[0] == '[') {
			strcat(text, lb);
		} else if (strstr(lb, "freq")) {
			strcat(text, lb);
		} else if (strstr(lb, "#include") == lb && 
		    sscanf(lb, "#include %s", inc) == 1) {
			char *lb2;
			char *q, *s = inc;
			if (s[0] == '"') {
				s++;
			}
			q = strrchr(s, '"');
			if (q) {
				*q = '\0';
			}
			sprintf(file2, "%s%s", dir, s);
			in2 = fopen(file2, "r");
			if (in2 == NULL) {
				rfbLog("error opening freqtab include: %s %s\n", line, file2);
				clean_up_exit(1);
			}
			while (fgets(line2, 1024, in2) != NULL) {
				lb2 = lblanks(line2);
				if (lb2[0] == '[') {
					strcat(text, lb2);
				} else if (strstr(lb2, "freq")) {
					strcat(text, lb2);
				}
				if ((int) strlen(text) > size/2) {
					break;
				}
			}
			fclose(in2);
		}
		if ((int) strlen(text) > size/2) {
			break;
		}
	}
	fclose(in1);

	if (0) fprintf(stderr, "%s", text);

	str = strdup(text);
	p = strtok(str, "\n");
	maxn = -1;
	extra = 0;
	while (p) {
		if (p[0] == '[') {
			int ok = 1;
			q = p+1;	
			while (*q) {
				if (*q == ']') {
					break;
				}
				if (! isdigit((unsigned char) (*q))) {
					if (0) fprintf(stderr, "extra: %s\n", p);
					extra++;
					ok = 0;
					break;
				}
				q++;
			}
			if (ok) {
				int n;
				if (sscanf(p, "[%d]", &n) == 1)  {
					if (n > maxn) {
						maxn = n;
					}
					if (0) fprintf(stderr, "maxn:  %d %d\n", maxn, n);
				}
			}
			
		}
		p = strtok(NULL, "\n");
	}
	free(str);

	str = strdup(text);
	p = strtok(str, "\n");
	extra = 0;
	currn = 0;
	if (v) fprintf(stderr, "\nname\tstation\tfreq (KHz)\n");
	while (p) {
		if (p[0] == '[') {
			int ok = 1;
			strncpy(line, p, 100);
			q = p+1;	
			while (*q) {
				if (*q == ']') {
					break;
				}
				if (! isdigit((unsigned char) (*q))) {
					extra++;
					currn = maxn + extra;
					ok = 0;
					break;
				}
				q++;
			}
			if (ok) {
				int n;
				if (sscanf(p, "[%d]", &n) == 1)  {
					currn = n;
				}
			}
		}
		if (strstr(p, "freq") && (q = strchr(p, '=')) != NULL) {
			int n;
			q = lblanks(q+1);
			if (sscanf(q, "%d", &n) == 1) {
				if (currn >= 0 && currn < CHANNEL_MAX) {
					if (v) fprintf(stderr, "%s\t%d\t%d\n", line, currn, n);
					custom_freq[currn] = (unsigned long) n;
					if (last_freq == 0) {
						last_freq = custom_freq[currn];
					}
				}
			}
		}
		p = strtok(NULL, "\n");
	}
	if (v) fprintf(stderr, "\n");
	v = 0;
	free(str);
	free(text);
	free(dir);
	free(file2);
}

static void init_freqs(void) {
	int i;
	for (i=0; i<CHANNEL_MAX; i++) {
		ntsc_cable[i] = 0;
		custom_freq[i] = 0;
	}

	init_ntsc_cable();
	last_freq = ntsc_cable[1];

	if (freqtab) {
		init_freqtab(freqtab);
	}
}

static void init_ntsc_cable(void) {
	ntsc_cable[1] = 73250;
	ntsc_cable[2] = 55250;
	ntsc_cable[3] = 61250;
	ntsc_cable[4] = 67250;
	ntsc_cable[5] = 77250;
	ntsc_cable[6] = 83250;
	ntsc_cable[7] = 175250;
	ntsc_cable[8] = 181250;
	ntsc_cable[9] = 187250;
	ntsc_cable[10] = 193250;
	ntsc_cable[11] = 199250;
	ntsc_cable[12] = 205250;
	ntsc_cable[13] = 211250;
	ntsc_cable[14] = 121250;
	ntsc_cable[15] = 127250;
	ntsc_cable[16] = 133250;
	ntsc_cable[17] = 139250;
	ntsc_cable[18] = 145250;
	ntsc_cable[19] = 151250;
	ntsc_cable[20] = 157250;
	ntsc_cable[21] = 163250;
	ntsc_cable[22] = 169250;
	ntsc_cable[23] = 217250;
	ntsc_cable[24] = 223250;
	ntsc_cable[25] = 229250;
	ntsc_cable[26] = 235250;
	ntsc_cable[27] = 241250;
	ntsc_cable[28] = 247250;
	ntsc_cable[29] = 253250;
	ntsc_cable[30] = 259250;
	ntsc_cable[31] = 265250;
	ntsc_cable[32] = 271250;
	ntsc_cable[33] = 277250;
	ntsc_cable[34] = 283250;
	ntsc_cable[35] = 289250;
	ntsc_cable[36] = 295250;
	ntsc_cable[37] = 301250;
	ntsc_cable[38] = 307250;
	ntsc_cable[39] = 313250;
	ntsc_cable[40] = 319250;
	ntsc_cable[41] = 325250;
	ntsc_cable[42] = 331250;
	ntsc_cable[43] = 337250;
	ntsc_cable[44] = 343250;
	ntsc_cable[45] = 349250;
	ntsc_cable[46] = 355250;
	ntsc_cable[47] = 361250;
	ntsc_cable[48] = 367250;
	ntsc_cable[49] = 373250;
	ntsc_cable[50] = 379250;
	ntsc_cable[51] = 385250;
	ntsc_cable[52] = 391250;
	ntsc_cable[53] = 397250;
	ntsc_cable[54] = 403250;
	ntsc_cable[55] = 409250;
	ntsc_cable[56] = 415250;
	ntsc_cable[57] = 421250;
	ntsc_cable[58] = 427250;
	ntsc_cable[59] = 433250;
	ntsc_cable[60] = 439250;
	ntsc_cable[61] = 445250;
	ntsc_cable[62] = 451250;
	ntsc_cable[63] = 457250;
	ntsc_cable[64] = 463250;
	ntsc_cable[65] = 469250;
	ntsc_cable[66] = 475250;
	ntsc_cable[67] = 481250;
	ntsc_cable[68] = 487250;
	ntsc_cable[69] = 493250;
	ntsc_cable[70] = 499250;
	ntsc_cable[71] = 505250;
	ntsc_cable[72] = 511250;
	ntsc_cable[73] = 517250;
	ntsc_cable[74] = 523250;
	ntsc_cable[75] = 529250;
	ntsc_cable[76] = 535250;
	ntsc_cable[77] = 541250;
	ntsc_cable[78] = 547250;
	ntsc_cable[79] = 553250;
	ntsc_cable[80] = 559250;
	ntsc_cable[81] = 565250;
	ntsc_cable[82] = 571250;
	ntsc_cable[83] = 577250;
	ntsc_cable[84] = 583250;
	ntsc_cable[85] = 589250;
	ntsc_cable[86] = 595250;
	ntsc_cable[87] = 601250;
	ntsc_cable[88] = 607250;
	ntsc_cable[89] = 613250;
	ntsc_cable[90] = 619250;
	ntsc_cable[91] = 625250;
	ntsc_cable[92] = 631250;
	ntsc_cable[93] = 637250;
	ntsc_cable[94] = 643250;
	ntsc_cable[95] = 91250;
	ntsc_cable[96] = 97250;
	ntsc_cable[97] = 103250;
	ntsc_cable[98] = 109250;
	ntsc_cable[99] = 115250;
	ntsc_cable[100] = 649250;
	ntsc_cable[101] = 655250;
	ntsc_cable[102] = 661250;
	ntsc_cable[103] = 667250;
	ntsc_cable[104] = 673250;
	ntsc_cable[105] = 679250;
	ntsc_cable[106] = 685250;
	ntsc_cable[107] = 691250;
	ntsc_cable[108] = 697250;
	ntsc_cable[109] = 703250;
	ntsc_cable[110] = 709250;
	ntsc_cable[111] = 715250;
	ntsc_cable[112] = 721250;
	ntsc_cable[113] = 727250;
	ntsc_cable[114] = 733250;
	ntsc_cable[115] = 739250;
	ntsc_cable[116] = 745250;
	ntsc_cable[117] = 751250;
	ntsc_cable[118] = 757250;
	ntsc_cable[119] = 763250;
	ntsc_cable[120] = 769250;
	ntsc_cable[121] = 775250;
	ntsc_cable[122] = 781250;
	ntsc_cable[123] = 787250;
	ntsc_cable[124] = 793250;
	ntsc_cable[125] = 799250;
}

