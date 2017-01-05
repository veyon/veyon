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

/* -- xwrappers.c -- */

#include "x11vnc.h"
#include "xrecord.h"
#include "keyboard.h"
#include "xevents.h"
#include "connections.h"
#include "cleanup.h"
#include "macosx.h"

int xshm_present = 0;
int xshm_opcode = 0;
int xtest_present = 0;
int xtrap_present = 0;
int xrecord_present = 0;
int xkb_present = 0;
int xinerama_present = 0;

int keycode_state[256];
int rootshift = 0;
int clipshift = 0;


int guess_bits_per_color(int bits_per_pixel);

int XFlush_wr(Display *disp);

Status XShmGetImage_wr(Display *disp, Drawable d, XImage *image, int x, int y,
    unsigned long mask);
XImage *XShmCreateImage_wr(Display* disp, Visual* vis, unsigned int depth,
    int format, char* data, XShmSegmentInfo* shminfo, unsigned int width,
    unsigned int height);
Status XShmAttach_wr(Display *disp, XShmSegmentInfo *shminfo);
Status XShmDetach_wr(Display *disp, XShmSegmentInfo *shminfo);
Bool XShmQueryExtension_wr(Display *disp);
int XShmGetEventBase_wr(Display *disp);

XImage *xreadscreen(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, Bool show_cursor);
XImage *XGetSubImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format, XImage *dest_image, int dest_x, int dest_y);
XImage *XGetImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format);
XImage *XCreateImage_wr(Display *disp, Visual *visual, unsigned int depth,
    int format, int offset, char *data, unsigned int width,
    unsigned int height, int bitmap_pad, int bytes_per_line);
void copy_image(XImage *dest, int x, int y, unsigned int w, unsigned int h);
void init_track_keycode_state(void);

void XTRAP_FakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay);
void XTestFakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay);
void XTRAP_FakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay);
void XTestFakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay);
void XTRAP_FakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay);
void XTestFakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay);

Bool XTestCompareCurrentCursorWithWindow_wr(Display* dpy, Window w);
Bool XTestCompareCursorWithWindow_wr(Display* dpy, Window w, Cursor cursor);
Bool XTestQueryExtension_wr(Display *dpy, int *ev, int *er, int *maj,
    int *min);
void XTestDiscard_wr(Display *dpy);
Bool XETrapQueryExtension_wr(Display *dpy, int *ev, int *er, int *op);
int XTestGrabControl_wr(Display *dpy, Bool impervious);
int XTRAP_GrabControl_wr(Display *dpy, Bool impervious);
void disable_grabserver(Display *in_dpy, int change);

Bool XRecordQueryVersion_wr(Display *dpy, int *maj, int *min);

int xauth_raw(int on);
Display *XOpenDisplay_wr(char *display_name);
int XCloseDisplay_wr(Display *display);

Bool XQueryPointer_wr(Display *display, Window w, Window *root_return,
    Window *child_return, int *root_x_return, int *root_y_return,
    int *win_x_return, int *win_y_return, unsigned int *mask_return);

Status XQueryTree_wr(Display *display, Window w, Window *root_return,
    Window *parent_return, Window **children_return,
    unsigned int *nchildren_return);

int XFree_wr(void *data);
int XSelectInput_wr(Display *display, Window w, long event_mask);

void copy_raw_fb(XImage *dest, int x, int y, unsigned int w, unsigned int h);
static void upup_downdown_warning(KeyCode key, Bool down);

/* 
 * used in rfbGetScreen and rfbNewFramebuffer: and estimate to the number
 * of bits per color, of course for some visuals, e.g. 565, the number
 * is not the same for each color.  This is just a sane default.
 */
int guess_bits_per_color(int bits_per_pixel) {
	int bits_per_color;
	
	/* first guess, spread them "evenly" over R, G, and B */
	bits_per_color = bits_per_pixel/3;
	if (bits_per_color < 1) {
		bits_per_color = 1;	/* 1bpp, 2bpp... */
	}

	/* choose safe values for usual cases: */
	if (bits_per_pixel == 8) {
		bits_per_color = 2;
	} else if (bits_per_pixel == 15 || bits_per_pixel == 16) {
		bits_per_color = 5;
	} else if (bits_per_pixel == 24 || bits_per_pixel == 32) {
		bits_per_color = 8;
	}
	return bits_per_color;
}

int XFlush_wr(Display *disp) {
#if NO_X11
	if (!disp) {}
	return 1;
#else
	if (disp) {
		return XFlush(disp);
	} else {
		return 1;
	}
#endif	/* NO_X11 */
}

/*
 * Kludge to interpose image gets and limit to a subset rectangle of
 * the rootwin.  This is the -sid option trying to work around invisible
 * saveUnders menu, etc, windows.  Also -clip option.
 */

#define ADJUST_ROOTSHIFT \
	if (rootshift && subwin) { \
		d = rootwin; \
		x += off_x; \
		y += off_y; \
	} \
	if (clipshift) { \
		x += coff_x; \
		y += coff_y; \
	}

/*
 * Wrappers for Image related X calls
 */
Status XShmGetImage_wr(Display *disp, Drawable d, XImage *image, int x, int y,
    unsigned long mask) {

	ADJUST_ROOTSHIFT

	/* Note: the Solaris overlay stuff is all non-shm (using_shm = 0) */

#if LIBVNCSERVER_HAVE_XSHM
	return XShmGetImage(disp, d, image, x, y, mask); 
#else
	if (!disp || !d || !image || !x || !y || !mask) {}
	return (Status) 0;
#endif
}

XImage *XShmCreateImage_wr(Display* disp, Visual* vis, unsigned int depth,
    int format, char* data, XShmSegmentInfo* shminfo, unsigned int width,
    unsigned int height) {

#if LIBVNCSERVER_HAVE_XSHM
	return XShmCreateImage(disp, vis, depth, format, data, shminfo,
	    width, height); 
#else
	if (!disp || !vis || !depth || !format || !data || !shminfo || !width || !height) {}
	return (XImage *) 0;
#endif
}

Status XShmAttach_wr(Display *disp, XShmSegmentInfo *shminfo) {
#if LIBVNCSERVER_HAVE_XSHM
	return XShmAttach(disp, shminfo);
#else
	if (!disp || !shminfo) {}
	return (Status) 0;
#endif
}

Status XShmDetach_wr(Display *disp, XShmSegmentInfo *shminfo) {
#if LIBVNCSERVER_HAVE_XSHM
	if (getenv("X11VNC_SHM_DEBUG")) fprintf(stderr, "XShmDetach_wr: %p disp: %p\n", (void *)shminfo, (void *)disp);
	return XShmDetach(disp, shminfo);
#else
	if (!disp || !shminfo) {}
	return (Status) 0;
#endif
}

Bool XShmQueryExtension_wr(Display *disp) {
#if LIBVNCSERVER_HAVE_XSHM
	return XShmQueryExtension(disp);
#else
	if (!disp) {}
	return False;
#endif
}

int XShmGetEventBase_wr(Display *disp) {
#if LIBVNCSERVER_HAVE_XSHM
	return XShmGetEventBase(disp);
#else
	if (!disp) {}
	return 0;
#endif
}

/* wrapper for overlay screen reading: */

XImage *xreadscreen(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, Bool show_cursor) {
#if NO_X11
	if (!disp || !d || !x || !y || !width || !height || !show_cursor) {}
	return NULL;
#else

#ifdef SOLARIS_OVERLAY
	return XReadScreen(disp, d, x, y, width, height,
	    show_cursor);
#else
#  ifdef IRIX_OVERLAY
	{	unsigned long hints = 0, hints_ret;
		if (show_cursor) hints |= XRD_READ_POINTER;
		return XReadDisplay(disp, d, x, y, width, height,
		    hints, &hints_ret);
	}
#  else
	/* unused vars warning: */
	if (disp || d || x || y || width || height || show_cursor) {}

	return NULL;
#  endif
#endif

#endif	/* NO_X11 */
}

XImage *XGetSubImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format, XImage *dest_image, int dest_x, int dest_y) {
#if NO_X11
	nox11_exit(1);
	if (!disp || !d || !x || !y || !width || !height || !plane_mask || !format || !dest_image || !dest_x || !dest_y) {}
	return NULL;
#else
	ADJUST_ROOTSHIFT

	if (overlay && dest_x == 0 && dest_y == 0) {
		size_t size = dest_image->height * dest_image->bytes_per_line;
		XImage *xi;

		xi = xreadscreen(disp, d, x, y, width, height,
		    (Bool) overlay_cursor);

		if (! xi) return NULL;

		/*
		 * There is extra overhead from memcpy and free...
		 * this is not like the real XGetSubImage().  We hope
		 * this significant overhead is still small compared to
		 * the time to retrieve the fb data.
		 */
		memcpy(dest_image->data, xi->data, size);

		XDestroyImage(xi);
		return (dest_image);
	}
	return XGetSubImage(disp, d, x, y, width, height, plane_mask,
	    format, dest_image, dest_x, dest_y);
#endif	/* NO_X11 */
}

XImage *XGetImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format) {
#if NO_X11
	if (!disp || !d || !x || !y || !width || !height || !plane_mask || !format) {}
	nox11_exit(1);
	return NULL;
#else

	ADJUST_ROOTSHIFT

	if (overlay) {
		return xreadscreen(disp, d, x, y, width, height,
		    (Bool) overlay_cursor);
	}
	return XGetImage(disp, d, x, y, width, height, plane_mask, format);
#endif	/* NO_X11 */
}

XImage *XCreateImage_wr(Display *disp, Visual *visual, unsigned int depth,
    int format, int offset, char *data, unsigned int width,
    unsigned int height, int bitmap_pad, int bytes_per_line) {
	/*
	 * This is a kludge to get a created XImage to exactly match what
	 * XReadScreen returns: we noticed the rgb masks are different
	 * from XCreateImage with the high color visual (red mask <->
	 * blue mask).  Note we read from the root window(!) then free
	 * the data.
	 */

	if (raw_fb) {	/* raw_fb hack */
		XImage *xi;
		xi = (XImage *) malloc(sizeof(XImage));
		memset(xi, 0, sizeof(XImage));
		xi->depth = depth;
		if (depth >= 24) {
			xi->bits_per_pixel = 32;
		} else if (depth > 16) {
			xi->bits_per_pixel = 24;
		} else if (depth > 8) {
			xi->bits_per_pixel = 16;
		} else {
			xi->bits_per_pixel = 8;
		}
		xi->format = format;
		xi->xoffset = offset;
		xi->data = data;
		xi->width = width;
		xi->height = height;
		xi->bitmap_pad = bitmap_pad;
		xi->bytes_per_line = bytes_per_line ? bytes_per_line : 
		    xi->width * xi->bits_per_pixel / 8;
		xi->bitmap_unit = -1;	/* hint to not call XDestroyImage */
		return xi;
	}

#if NO_X11
	nox11_exit(1);
	if (!disp || !visual || !depth || !format || !offset || !data || !width
	    || !height || !width || !bitmap_pad || !bytes_per_line) {}
	return NULL;
#else
	if (overlay) {
		XImage *xi;
		xi = xreadscreen(disp, window, 0, 0, width, height, False);
		if (xi == NULL) {
			return xi;
		}
		if (xi->data != NULL) {
			free(xi->data);
		}
		xi->data = data;
		return xi;
	}

	return XCreateImage(disp, visual, depth, format, offset, data,
	    width, height, bitmap_pad, bytes_per_line);
#endif	/* NO_X11 */
}

static void copy_raw_fb_low_bpp(XImage *dest, int x, int y, unsigned int w,
    unsigned int h) {
	char *src, *dst;
	unsigned int line;
	static char *buf = NULL;
	static int buflen = -1;
	int bpl = wdpy_x * raw_fb_native_bpp / 8;
	int n, ix, len, del, sz = wdpy_x * raw_fb_expand_bytes;

	unsigned int rm_n = raw_fb_native_red_mask;
	unsigned int gm_n = raw_fb_native_green_mask;
	unsigned int bm_n = raw_fb_native_blue_mask;
	unsigned int rm_f = main_red_mask;
	unsigned int gm_f = main_green_mask;
	unsigned int bm_f = main_blue_mask;

	unsigned int rs_n = raw_fb_native_red_shift;
	unsigned int gs_n = raw_fb_native_green_shift;
	unsigned int bs_n = raw_fb_native_blue_shift;
	unsigned int rs_f = main_red_shift;
	unsigned int gs_f = main_green_shift;
	unsigned int bs_f = main_blue_shift;

	unsigned int rx_n = raw_fb_native_red_max;
	unsigned int gx_n = raw_fb_native_green_max;
	unsigned int bx_n = raw_fb_native_blue_max;
	unsigned int rx_f = main_red_max;
	unsigned int gx_f = main_green_max;
	unsigned int bx_f = main_blue_max;

	static unsigned int msk[8];
	static int last_bpp = -1;
	static int cga = -1;

	if (rm_f | gm_f | bm_f) {}

	if (cga < 0) {
		if (getenv("RAWFB_CGA")) {
			cga = 1;
		} else {
			cga = 0;
		}
	}

	if (sz > buflen || buf == NULL) {
		if (buf) {
			free(buf);
		}
		buflen = sz + 1000;
		buf = (char *) malloc(buflen);
	}

	if (clipshift && ! use_snapfb) {
		x += coff_x;
		y += coff_y;
	}

	if (last_bpp != raw_fb_native_bpp) {
		int br;
		for (br = 0; br < 8; br++) {
			unsigned int pbit, k, m = 0;
			
			for (k=0; k < (unsigned int) raw_fb_native_bpp; k++) {
				pbit = 1 << (br+k);
				m |= pbit;
			}
			msk[br] = m;
		}
		last_bpp = raw_fb_native_bpp;
	}

	dst = dest->data;
if (0) fprintf(stderr, "x=%d y=%d w=%d h=%d bpl=%d d_bpl=%d-%dx%dx%d/%d %p\n",
    x, y, w, h, bpl, dest->bytes_per_line, dest->width, dest->height, dest->bits_per_pixel, dest->depth, dst);

	for (line = 0; line < h; line++) {

		if (! raw_fb_seek) {
			/* mmap */
			src = raw_fb_addr + raw_fb_offset + bpl*(y+line);

			memcpy(buf, src, bpl);
		} else {
			/* lseek */
			off_t off;
			off = (off_t) (raw_fb_offset + bpl*(y+line));

			lseek(raw_fb_fd, off, SEEK_SET);

			len = bpl;
			del = 0;
			while (len > 0) {
				n = read(raw_fb_fd, buf + del, len);

				if (n > 0) {
					del += n;
					len -= n;
				} else if (n == 0) {
					break;
				} else if (errno != EINTR && errno != EAGAIN) {
					break;
				}
			}
		}
		for (ix = 0; ix < (int) w; ix++) {
			int bx = (x + ix) * raw_fb_native_bpp;
			int ib = bx / 8;
			int br = bx - ib * 8;
			unsigned char val;

			val = *((unsigned char*) (buf + ib));

			val = msk[br] & val;
			val = val >> br;

			if (cga) {
				/* this is expt for CGA */
				double r, g, b;
				int ir, ig, ib;
				r = (2./3)*(val & 4) + (1./3)*(val & 8);
				g = (2./3)*(val & 2) + (1./3)*(val & 8);
				b = (2./3)*(val & 1) + (1./3)*(val & 8);
				if (val == 6) {
					g = g/2.;
				}
				ir = rx_f * r;
				ig = gx_f * g;
				ib = bx_f * b;
				val = (ib << bs_f) | (ig << gs_f) | (ir << rs_f);
			} else {
				unsigned char rval, gval, bval;
				
				rval = (val & rm_n) >> rs_n;
				gval = (val & gm_n) >> gs_n;
				bval = (val & bm_n) >> bs_n;

				rval = (rx_f * rval) / rx_n;
				gval = (gx_f * gval) / gx_n;
				bval = (bx_f * bval) / bx_n;

				rval = rval << rs_f;
				gval = gval << gs_f;
				bval = bval << bs_f;

				val = rval | gval | bval;
			}

			*(dst+ix) = (char) val;
		}

		dst += dest->bytes_per_line;
	}
}

static void copy_raw_fb_24_to_32(XImage *dest, int x, int y, unsigned int w,
    unsigned int h) {
	/*
	 * kludge to read 1 byte at a time and dynamically transform
	 * 24bpp -> 32bpp by inserting a extra 0 byte into dst.
	 */
	char *src, *dst;
	unsigned int line;
	static char *buf = NULL;
	static int buflen = -1;
	int bpl = wdpy_x * 3;	/* pixelsize == 3 */
	int LE, n, stp, len, del, sz = w * 3;
	int insert_zeroes = 1;

#define INSERT_ZEROES  \
	len = sz; \
	del = 0; \
	stp = 0; \
	while (len > 0) { \
		if (insert_zeroes && (del - LE) % 4 == 0) { \
			*(dst + del) = 0; \
			del++; \
		} \
		*(dst + del) = *(buf + stp); \
		del++; \
		len--; \
		stp++; \
	}

	if (rfbEndianTest) {
		LE = 3;	/* little endian */
	} else {
		LE = 0; /* big endian */
	}

	if (sz > buflen || buf == NULL) {
		if (buf) {
			free(buf);
		}
		buf = (char *) malloc(4*(sz + 1000));
	}

	if (clipshift && ! use_snapfb) {
		x += coff_x;
		y += coff_y;
	}

	if (use_snapfb && dest != snap) {
		/* snapfb src */
		src = snap->data + snap->bytes_per_line*y + 3*x;
		dst = dest->data;
		for (line = 0; line < h; line++) {
			memcpy(buf, src, sz);

			INSERT_ZEROES

			src += snap->bytes_per_line;
			dst += dest->bytes_per_line;
		}

	} else if (! raw_fb_seek) {
		/* mmap */
		bpl = raw_fb_bytes_per_line;
		if (clipshift && wdpy_x != cdpy_x) {
			bpl = wdpy_x * 3;
		}
		src = raw_fb_addr + raw_fb_offset + bpl*y + 3*x;
		dst = dest->data;

		if (use_snapfb && dest == snap) {
			/*
			 * writing *to* snap_fb: need the x,y offset,
			 * and also do not do inserts.
			 */
			dst += bpl*y + 3*x;
			insert_zeroes = 0;
		}

		for (line = 0; line < h; line++) {
			memcpy(buf, src, sz);

			INSERT_ZEROES

			src += bpl;
			dst += dest->bytes_per_line;
		}

	} else {
		/* lseek */
		off_t off;
		bpl = raw_fb_bytes_per_line;
		if (clipshift && wdpy_x != cdpy_x) {
			bpl = wdpy_x * 3;
		}
		off = (off_t) (raw_fb_offset + bpl*y + 3*x);

		lseek(raw_fb_fd, off, SEEK_SET);
		dst = dest->data;

		if (use_snapfb && dest == snap) {
			/*
			 * writing *to* snap_fb: need the x,y offset,
			 * and also do not do inserts.
			 */
			dst += bpl*y + 3*x;
			insert_zeroes = 0;
		}

		for (line = 0; line < h; line++) {
			len = sz;
			del = 0;
			while (len > 0) {
				n = read(raw_fb_fd, buf + del, len);

				if (n > 0) {
					del += n;
					len -= n;
				} else if (n == 0) {
					break;
				} else if (errno != EINTR && errno != EAGAIN) {
					break;
				}
			}

			INSERT_ZEROES

			if (bpl > sz) {
				off = (off_t) (bpl - sz);
				lseek(raw_fb_fd, off, SEEK_CUR);
			}
			dst += dest->bytes_per_line;
		}
	}
}

#ifdef MACOSX
void macosx_copy_opengl(char *, int, int, unsigned int, unsigned int);
#endif

void copy_raw_fb(XImage *dest, int x, int y, unsigned int w, unsigned int h) {
	char *src, *dst;
	unsigned int line;
	int pixelsize = bpp/8;
	static int db = -1;

#ifdef MACOSX
	if (macosx_console && macosx_read_opengl) {
		macosx_copy_opengl(dest->data, x, y, w, h);
		return;
	}
#endif

	if (xform24to32) {
		copy_raw_fb_24_to_32(dest, x, y, w, h);
		return;
	}
	if (raw_fb_native_bpp < 8) {
		copy_raw_fb_low_bpp(dest, x, y, w, h);
		return;
	}
	if (db < 0) {
		if (getenv("DEBUG_COPY_RAW_FB")) {
			db = atoi(getenv("DEBUG_COPY_RAW_FB"));
		} else {
			db = 0;
		}
	}

	if (clipshift && ! use_snapfb) {
		x += coff_x;
		y += coff_y;
	}


	if (use_snapfb && dest != snap) {
		/* snapfb src */
		src = snap->data + snap->bytes_per_line*y + pixelsize*x;
		dst = dest->data;

if (db) fprintf(stderr, "snap->bytes_per_line: %d, dest->bytes_per_line: %d, w: %d h: %d dpy_x: %d wdpy_x: %d cdpy_x: %d bpp: %d pixelsize: %d\n", snap->bytes_per_line, dest->bytes_per_line, w, h, dpy_x, wdpy_x, cdpy_x, bpp, pixelsize);

		for (line = 0; line < h; line++) {
			memcpy(dst, src, w * pixelsize);
			src += snap->bytes_per_line;
			dst += dest->bytes_per_line;
		}

	} else if (! raw_fb_seek) {
		/* mmap */
		int bpl = raw_fb_bytes_per_line;

		if (clipshift && wdpy_x != cdpy_x) {
			bpl = wdpy_x * pixelsize;
		}

		src = raw_fb_addr + raw_fb_offset + bpl*y + pixelsize*x;
		dst = dest->data;

if (db) fprintf(stderr, "bpl: %d, dest->bytes_per_line: %d, w: %d h: %d dpy_x: %d wdpy_x: %d cdpy_x: %d bpp: %d pixelsize: %d\n", bpl, dest->bytes_per_line, w, h, dpy_x, wdpy_x, cdpy_x, bpp, pixelsize);

		for (line = 0; line < h; line++) {
			memcpy(dst, src, w * pixelsize);
			src += bpl;
			dst += dest->bytes_per_line;
		}

	} else {
		/* lseek */
		int n, len, del, sz = w * pixelsize;
		off_t off;
		int bpl = raw_fb_bytes_per_line;

		if (clipshift && wdpy_x != cdpy_x) {
			bpl = wdpy_x * pixelsize;
		}

		off = (off_t) (raw_fb_offset + bpl*y + pixelsize*x);

		lseek(raw_fb_fd, off, SEEK_SET);
		dst = dest->data;

if (db) fprintf(stderr, "lseek 0 ps: %d  sz: %d off: %d bpl: %d\n", pixelsize, sz, (int) off, bpl);

		for (line = 0; line < h; line++) {
			len = sz;
			del = 0;
			while (len > 0) {
				n = read(raw_fb_fd, dst + del, len);

				if (n > 0) {
					del += n;
					len -= n;
				} else if (n == 0) {
					break;
				} else if (errno != EINTR && errno != EAGAIN) {
					break;
				}
			}
			if (bpl > sz) {
				off = (off_t) (bpl - sz);
				lseek(raw_fb_fd, off, SEEK_CUR);
			}
			dst += dest->bytes_per_line;
		}
	}
}

void copy_image(XImage *dest, int x, int y, unsigned int w, unsigned int h) {
	/* default (w=0, h=0) is the fill the entire XImage */
	if (dest == NULL) {
		return;
	}
	if (w < 1)  {
		w = dest->width;
	}
	if (h < 1)  {
		h = dest->height;
	}

	if (raw_fb) {
		copy_raw_fb(dest, x, y, w, h);

	} else if (use_snapfb && snap_fb && dest != snaprect) {
		char *src, *dst;
		unsigned int line;
		int pixelsize = bpp/8;

		src = snap->data + snap->bytes_per_line*y + pixelsize*x;
		dst = dest->data;
		for (line = 0; line < h; line++) {
			memcpy(dst, src, w * pixelsize);
			src += snap->bytes_per_line;
			dst += dest->bytes_per_line;
		}

	} else if ((using_shm && ! xform24to32) && (int) w == dest->width &&
	    (int) h == dest->height) {
		XShmGetImage_wr(dpy, window, dest, x, y, AllPlanes);

	} else {
		XGetSubImage_wr(dpy, window, x, y, w, h, AllPlanes,
		    ZPixmap, dest, 0, 0);
	}
}

#define DEBUG_SKIPPED_INPUT(dbg, str) \
	if (dbg) { \
		rfbLog("skipped input: %s\n", str); \
	}

void init_track_keycode_state(void) {
	int i;
	for (i=0; i<256; i++) {
		keycode_state[i] = 0;
	}
	get_keystate(keycode_state);
}

static void upup_downdown_warning(KeyCode key, Bool down) {
	RAWFB_RET_VOID
#if NO_X11
	if (!key || !down) {}
	return;
#else
	if ((down ? 1:0) == keycode_state[(int) key]) {
		char *str = XKeysymToString(XKeycodeToKeysym(dpy, key, 0));
		rfbLog("XTestFakeKeyEvent: keycode=0x%x \"%s\" is *already* "
		    "%s\n", key, str ? str : "null", down ? "down":"up");
	}
#endif	/* NO_X11 */
}

/*
 * wrappers for XTestFakeKeyEvent, etc..
 * also for XTrap equivalents XESimulateXEventRequest
 */

void XTRAP_FakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay) {

	RAWFB_RET_VOID
#if NO_X11
	nox11_exit(1);
	if (!dpy || !key || !down || !delay) {}
	return;
#else

	if (! xtrap_present) {
		DEBUG_SKIPPED_INPUT(debug_keyboard, "keyboard: no-XTRAP");
		return;
	}
	/* unused vars warning: */
	if (key || down || delay) {}

# if LIBVNCSERVER_HAVE_LIBXTRAP
	XESimulateXEventRequest(trap_ctx, down ? KeyPress : KeyRelease,
	    key, 0, 0, 0);
	if (debug_keyboard) {
		upup_downdown_warning(key, down);
	}
	keycode_state[(int) key] = down ? 1 : 0;
# else
	DEBUG_SKIPPED_INPUT(debug_keyboard, "keyboard: no-XTRAP-build");
# endif

#endif	/* NO_X11 */
}

void XTestFakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay) {
	static int first = 1;
	int regrab = 0;

	RAWFB_RET_VOID

#if NO_X11
	nox11_exit(1);
	if (!dpy || !key || !down || !delay || !first) {}
	return;
#else
	if (debug_keyboard) {
		char *str = XKeysymToString(XKeycodeToKeysym(dpy, key, 0));
		rfbLog("XTestFakeKeyEvent(dpy, keycode=0x%x \"%s\", %s)\n",
		    key, str ? str : "null", down ? "down":"up");
	}
	if (first) { 
		init_track_keycode_state();
		first = 0;
	}
	if (down) {
		last_keyboard_keycode = -key;
	} else {
		last_keyboard_keycode = key;
	}

	if (grab_kbd) {
		XUngrabKeyboard(dpy, CurrentTime);
		regrab = 1;
	}
	if (grab_ptr && ungrab_both) {
		XUngrabPointer(dpy, CurrentTime);
		regrab = 1;
	}

	if (xtrap_input) {
		XTRAP_FakeKeyEvent_wr(dpy, key, down, delay);
		if (regrab) {
			adjust_grabs(1, 1);
		}
		return;
	}

	if (! xtest_present) {
		DEBUG_SKIPPED_INPUT(debug_keyboard, "keyboard: no-XTEST");
		return;
	}
	if (debug_keyboard) {
		rfbLog("calling XTestFakeKeyEvent(%d, %d)  %.4f\n",
		    key, down, dnowx());	
	}
#if LIBVNCSERVER_HAVE_XTEST
	XTestFakeKeyEvent(dpy, key, down, delay);
	if (regrab) {
		adjust_grabs(1, 1);
	}
	if (debug_keyboard) {
		upup_downdown_warning(key, down);
	}
	keycode_state[(int) key] = down ? 1 : 0;
#endif

#endif	/* NO_X11 */
}

void XTRAP_FakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay) {

	RAWFB_RET_VOID
#if NO_X11
	nox11_exit(1);
	if (!dpy || !button || !is_press || !delay) {}
	return;
#else

	if (! xtrap_present) {
		DEBUG_SKIPPED_INPUT(debug_keyboard, "button: no-XTRAP");
		return;
	}
	/* unused vars warning: */
	if (button || is_press || delay) {}

#if LIBVNCSERVER_HAVE_LIBXTRAP
	XESimulateXEventRequest(trap_ctx,
	    is_press ? ButtonPress : ButtonRelease, button, 0, 0, 0);
#else
	DEBUG_SKIPPED_INPUT(debug_keyboard, "button: no-XTRAP-build");
#endif

#endif	/* NO_X11 */
}

void XTestFakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay) {
	int regrab = 0;

	RAWFB_RET_VOID
#if NO_X11
	nox11_exit(1);
	if (!dpy || !button || !is_press || !delay) {}
	return;
#else

	if (grab_ptr) {
		XUngrabPointer(dpy, CurrentTime);
		regrab = 1;
	}
	if (grab_kbd && ungrab_both) {
		XUngrabKeyboard(dpy, CurrentTime);
		regrab = 1;
	}

	if (xtrap_input) {
		XTRAP_FakeButtonEvent_wr(dpy, button, is_press, delay);
		if (regrab) {
			adjust_grabs(1, 1);
		}
		return;
	}

	if (! xtest_present) {
		DEBUG_SKIPPED_INPUT(debug_keyboard, "button: no-XTEST");
		return;
	}
	if (debug_pointer) {
		rfbLog("calling XTestFakeButtonEvent(%d, %d)  %.4f\n",
		    button, is_press, dnowx());	
	}
#if LIBVNCSERVER_HAVE_XTEST
    	XTestFakeButtonEvent(dpy, button, is_press, delay);
#endif
	if (regrab) {
		adjust_grabs(1, 1);
	}
#endif	/* NO_X11 */
}

void XTRAP_FakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay) {

	RAWFB_RET_VOID

#if NO_X11
	nox11_exit(1);
	if (!dpy || !screen || !x || !y || !delay) {}
	return;
#else
	if (! xtrap_present) {
		DEBUG_SKIPPED_INPUT(debug_keyboard, "motion: no-XTRAP");
		return;
	}
	/* unused vars warning: */
	if (dpy || screen || x || y || delay) {}

#if LIBVNCSERVER_HAVE_LIBXTRAP
	XESimulateXEventRequest(trap_ctx, MotionNotify, 0, x, y, 0);
#else
	DEBUG_SKIPPED_INPUT(debug_keyboard, "motion: no-XTRAP-build");
#endif

#endif	/* NO_X11 */
}

void XTestFakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay) {
	int regrab = 0;

	RAWFB_RET_VOID
#if NO_X11
	nox11_exit(1);
	if (!dpy || !screen || !x || !y || !delay) {}
	return;
#else

	if (grab_ptr) {
		XUngrabPointer(dpy, CurrentTime);
		regrab = 1;
	}
	if (grab_kbd && ungrab_both) {
		XUngrabKeyboard(dpy, CurrentTime);
		regrab = 1;
	}

	if (xtrap_input) {
		XTRAP_FakeMotionEvent_wr(dpy, screen, x, y, delay);
		if (regrab) {
			adjust_grabs(1, 1);
		}
		return;
	}

	if (debug_pointer) {
		rfbLog("calling XTestFakeMotionEvent(%d, %d)  %.4f\n",
		    x, y, dnowx());	
	}
#if LIBVNCSERVER_HAVE_XTEST
	XTestFakeMotionEvent(dpy, screen, x, y, delay);
#endif
	if (regrab) {
		adjust_grabs(1, 1);
	}
#endif	/* NO_X11 */
}

Bool XTestCompareCurrentCursorWithWindow_wr(Display* dpy, Window w) {
	if (! xtest_present) {
		return False;
	}
	RAWFB_RET(False)

#if LIBVNCSERVER_HAVE_XTEST
	return XTestCompareCurrentCursorWithWindow(dpy, w);
#else
	if (!w) {}
	return False;
#endif
}

Bool XTestCompareCursorWithWindow_wr(Display* dpy, Window w, Cursor cursor) {
	if (! xtest_present) {
		return False;
	}
	RAWFB_RET(False)
#if LIBVNCSERVER_HAVE_XTEST
	return XTestCompareCursorWithWindow(dpy, w, cursor);
#else
	if (!dpy || !w || !cursor) {}
	return False;
#endif
}

Bool XTestQueryExtension_wr(Display *dpy, int *ev, int *er, int *maj,
    int *min) {
	RAWFB_RET(False)
#if LIBVNCSERVER_HAVE_XTEST
	return XTestQueryExtension(dpy, ev, er, maj, min);
#else
	if (!dpy || !ev || !er || !maj || !min) {}
	return False;
#endif
}

void XTestDiscard_wr(Display *dpy) {
	if (! xtest_present) {
		return;
	}
	RAWFB_RET_VOID
#if LIBVNCSERVER_HAVE_XTEST
	XTestDiscard(dpy);
#else
	if (!dpy) {}
#endif
}

Bool XETrapQueryExtension_wr(Display *dpy, int *ev, int *er, int *op) {
	RAWFB_RET(False)
#if LIBVNCSERVER_HAVE_LIBXTRAP
	return XETrapQueryExtension(dpy, (INT32 *)ev, (INT32 *)er,
	    (INT32 *)op);
#else
	/* unused vars warning: */
	if (ev || er || op) {}
	return False;
#endif
}

int XTestGrabControl_wr(Display *dpy, Bool impervious) {
	if (! xtest_present) {
		return 0;
	}
	RAWFB_RET(0)
#if LIBVNCSERVER_HAVE_XTEST && LIBVNCSERVER_HAVE_XTESTGRABCONTROL
	XTestGrabControl(dpy, impervious);
	return 1;
#else
	if (!dpy || !impervious) {}
	return 0;
#endif
}

int XTRAP_GrabControl_wr(Display *dpy, Bool impervious) {
	if (! xtrap_present) {
		/* unused vars warning: */
		if (dpy || impervious) {}
		return 0;
	}
	RAWFB_RET(0)
#if LIBVNCSERVER_HAVE_LIBXTRAP
	  else {
		ReqFlags requests;

		if (! impervious) {
			if (trap_ctx) {
				XEFreeTC(trap_ctx);
			}
			trap_ctx = NULL;
			return 1;
		}

		if (! trap_ctx) {
			trap_ctx = XECreateTC(dpy, 0, NULL);
			if (! trap_ctx) {
				rfbLog("DEC-XTRAP XECreateTC failed.  Watch "
				    "out for XGrabServer from wm's\n");
				return 0;
			}
			XEStartTrapRequest(trap_ctx);
			memset(requests, 0, sizeof(requests));
			BitTrue(requests, X_GrabServer);
			BitTrue(requests, X_UngrabServer);
			XETrapSetRequests(trap_ctx, True, requests);
			XETrapSetGrabServer(trap_ctx, True);
		}
		return 1;
	}
#endif
	return 0;
}

void disable_grabserver(Display *in_dpy, int change) {
	int ok = 0;
	static int didmsg = 0;

	if (debug_grabs) {
		fprintf(stderr, "disable_grabserver/%d %.5f\n",
			xserver_grabbed, dnowx());
		didmsg = 0;
	}

	if (! xtrap_input) {
		if (XTestGrabControl_wr(in_dpy, True)) {
			if (change) {
				XTRAP_GrabControl_wr(in_dpy, False);
			}
			if (! didmsg && ! raw_fb_str) {
				rfbLog("GrabServer control via XTEST.\n"); 
				didmsg = 1;
			}
			ok = 1;
		} else {
			if (XTRAP_GrabControl_wr(in_dpy, True)) {
				ok = 1;
				if (! didmsg && ! raw_fb_str) {
					rfbLog("Using DEC-XTRAP for protection"
					    " from XGrabServer.\n");
					didmsg = 1;
				}
			}
		}
	} else {
		if (XTRAP_GrabControl_wr(in_dpy, True)) {
			if (change) {
				XTestGrabControl_wr(in_dpy, False);
			}
			if (! didmsg && ! raw_fb_str) {
				rfbLog("GrabServer control via DEC-XTRAP.\n"); 
				didmsg = 1;
			}
			ok = 1;
		} else {
			if (XTestGrabControl_wr(in_dpy, True)) {
				ok = 1;
				if (! didmsg && ! raw_fb_str) {
					rfbLog("DEC-XTRAP XGrabServer "
					    "protection not available, "
					    "using XTEST.\n");
					didmsg = 1;
				}
			}
		}
	}
	if (! ok && ! didmsg) {
		rfbLog("*********************************************************\n");
		rfbLog("* No XTEST or DEC-XTRAP protection from XGrabServer !!! *\n");
		rfbLog("* DEADLOCK if your window manager calls XGrabServer !!! *\n");
		rfbLog("*********************************************************\n");
	}
	XFlush_wr(in_dpy);
}

Bool XRecordQueryVersion_wr(Display *dpy, int *maj, int *min) {
	RAWFB_RET(False)
#if LIBVNCSERVER_HAVE_RECORD
	return XRecordQueryVersion(dpy, maj, min);
#else
	if (!dpy || !maj || !min) {}
	return False;
#endif
}

int xauth_raw(int on) {
	char tmp[] = "/tmp/x11vnc-xauth.XXXXXX";
	int tmp_fd = -1;
	static char *old_xauthority = NULL;
	static char *old_tmp = NULL;
	int db = 0;

	if (on) {
		if (old_xauthority) {
			free(old_xauthority);
			old_xauthority = NULL;
		}
		if (old_tmp) {
			free(old_tmp);
			old_tmp = NULL;
		}
		if (xauth_raw_data) {
			tmp_fd = mkstemp(tmp);
			if (tmp_fd < 0) {
				rfbLog("could not create tmp xauth file: %s\n", tmp);	
				return 0;
			}
			if (db) fprintf(stderr, "XAUTHORITY tmp: %s\n", tmp);
			write(tmp_fd, xauth_raw_data, xauth_raw_len);
			close(tmp_fd);
			if (getenv("XAUTHORITY")) {
				old_xauthority = strdup(getenv("XAUTHORITY"));
			} else {
				old_xauthority = strdup("");
			}
			set_env("XAUTHORITY", tmp);
			old_tmp = strdup(tmp);
		}
		return 1;
	} else {
		if (old_xauthority) {
			if (!strcmp(old_xauthority, "")) {
				char *xauth = getenv("XAUTHORITY");
				if (xauth) {
					*(xauth-2) = '_';	/* yow */
				}
			} else {
				set_env("XAUTHORITY", old_xauthority);
			}
			free(old_xauthority);
			old_xauthority = NULL;
		}
		if (old_tmp) {
			unlink(old_tmp);
			free(old_tmp);
			old_tmp = NULL;
		}
		return 1;
	}
}

Display *XOpenDisplay_wr(char *display_name) {
	Display *d = NULL;
	int db = 0;

	if (! xauth_raw(1)) {
		return NULL;
	}
#if NO_X11
	rfbLog("This x11vnc was built without X11 support (-rawfb only).\n");
	if (!display_name || !d || !db) {}
	return NULL;
#else

	d = XOpenDisplay(display_name);
	if (db) fprintf(stderr, "XOpenDisplay_wr: %s  %p\n", display_name, (void *)d);

	if (d == NULL && !getenv("NO_XAUTHLOCALHOSTNAME")) {
		char *xalhn = getenv("XAUTHLOCALHOSTNAME");
		if (1 || !xalhn) {
			rfbLog("XOpenDisplay(\"%s\") failed.\n",
			    display_name ? display_name : "");
			rfbLog("Trying again with XAUTHLOCALHOSTNAME=localhost ...\n");
			set_env("XAUTHLOCALHOSTNAME", "localhost");
			d = XOpenDisplay(display_name);
			if (d == NULL && xalhn) {
				char *ptr = getenv("XAUTHLOCALHOSTNAME");
				if (ptr) {
					*(ptr-2) = '_';	/* yow */
					rfbLog("XOpenDisplay(\"%s\") failed.\n",
					    display_name ? display_name : "");
					rfbLog("Trying again with unset XAUTHLOCALHOSTNAME ...\n");
					d = XOpenDisplay(display_name);
				}
			}
		}
	}

	xauth_raw(0);

	return d;
#endif	/* NO_X11 */
}

int XCloseDisplay_wr(Display *display) {
	int db = 0;
	if (db) fprintf(stderr, "XCloseDisplay_wr: %p\n", (void *)display);
#if NO_X11
	return 0;
#else
	return XCloseDisplay(display);
#endif	/* NO_X11 */
}

static unsigned int Bmask = (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask);
static unsigned int Mmask = (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask);

static unsigned int last_local_button_mask = 0;
static unsigned int last_local_mod_mask = 0;
static int last_local_x = 0;
static int last_local_y = 0;

Bool XQueryPointer_wr(Display *display, Window w, Window *root_return,
    Window *child_return, int *root_x_return, int *root_y_return,
    int *win_x_return, int *win_y_return, unsigned int *mask_return) {
#if NO_X11
	if (!display || !w || !root_return || !child_return || !root_x_return
	    || !root_y_return || !win_x_return || !win_y_return || !mask_return) {}
	return False;
#else
	Bool rc;
	XErrorHandler old_handler;


	if (! display) {
		return False;
	}
	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;

	rc = XQueryPointer(display, w, root_return, child_return,
	    root_x_return, root_y_return, win_x_return, win_y_return,
	    mask_return);

	XSetErrorHandler(old_handler);
	if (trapped_xerror) {
		rc = 0;
	}
	if (rc) {
		display_button_mask = (*mask_return) & Bmask;
		display_mod_mask    = (*mask_return) & Mmask;
		if (last_local_button_mask != display_button_mask) {
			got_local_pointer_input++;
		} else if (*root_x_return != last_local_x ||
		    *root_y_return != last_local_y) {
			got_local_pointer_input++;
		}
		last_local_button_mask = display_button_mask;
		last_local_mod_mask = display_mod_mask;
		last_local_x = *root_x_return;
		last_local_y = *root_y_return;
	}
	return rc;
#endif	/* NO_X11 */
}
 

Status XQueryTree_wr(Display *display, Window w, Window *root_return,
    Window *parent_return, Window **children_return,
    unsigned int *nchildren_return) {

#ifdef MACOSX
	if (macosx_console) {
		return macosx_xquerytree(w, root_return, parent_return,
		    children_return, nchildren_return);
	}
#endif
#if NO_X11
	if (!display || !w || !root_return || !parent_return
	    || !children_return || !nchildren_return) {}
	return (Status) 0;
#else
	if (! display) {
		return (Status) 0;
	}
	return XQueryTree(display, w, root_return, parent_return,
	    children_return, nchildren_return);
#endif	/* NO_X11 */
    	
}

int XFree_wr(void *data) {
	if (data == NULL) {
		return 1;
	}
	if (! dpy) {
		return 1;
	}
#if NO_X11
	return 1;
#else
	return XFree(data);
#endif
}

int XSelectInput_wr(Display *display, Window w, long event_mask) {
#if NO_X11
	if (!display || !w || !event_mask) {}
	return 0;
#else
	int rc;
	XErrorHandler old_handler;
	if (display == NULL || w == None) {
		return 0;
	}
	old_handler = XSetErrorHandler(trap_xerror);
	trapped_xerror = 0;
	rc = XSelectInput(display, w, event_mask);
	XSetErrorHandler(old_handler);
	if (trapped_xerror) {
		rc = 0;
	}
	return rc;
#endif
}

void nox11_exit(int rc) {
#if NO_X11
	rfbLog("This x11vnc was not built with X11 support.\n");
	clean_up_exit(rc);
#else
	if (0) {rc = 0;}
#endif
}


#if NO_X11
#include "nox11_funcs.h"
#endif

