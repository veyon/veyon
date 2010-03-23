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

#ifndef _X11VNC_XWRAPPERS_H
#define _X11VNC_XWRAPPERS_H

/* -- xwrappers.h -- */

extern int xshm_present;
extern int xshm_opcode;
extern int xtest_present;
extern int xtrap_present;
extern int xrecord_present;
extern int xkb_present;
extern int xinerama_present;

extern int keycode_state[];
extern int rootshift;
extern int clipshift;


extern int guess_bits_per_color(int bits_per_pixel);

extern int XFlush_wr(Display *disp);

extern Status XShmGetImage_wr(Display *disp, Drawable d, XImage *image, int x, int y,
    unsigned long mask);
extern XImage *XShmCreateImage_wr(Display* disp, Visual* vis, unsigned int depth,
    int format, char* data, XShmSegmentInfo* shminfo, unsigned int width,
    unsigned int height);
extern Status XShmAttach_wr(Display *disp, XShmSegmentInfo *shminfo);
extern Status XShmDetach_wr(Display *disp, XShmSegmentInfo *shminfo);
extern Bool XShmQueryExtension_wr(Display *disp);
extern int XShmGetEventBase_wr(Display *disp);

extern XImage *xreadscreen(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, Bool show_cursor);
extern XImage *XGetSubImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format, XImage *dest_image, int dest_x, int dest_y);
extern XImage *XGetImage_wr(Display *disp, Drawable d, int x, int y,
    unsigned int width, unsigned int height, unsigned long plane_mask,
    int format);
extern XImage *XCreateImage_wr(Display *disp, Visual *visual, unsigned int depth,
    int format, int offset, char *data, unsigned int width,
    unsigned int height, int bitmap_pad, int bytes_per_line);
extern void copy_image(XImage *dest, int x, int y, unsigned int w, unsigned int h);
extern void copy_raw_fb(XImage *dest, int x, int y, unsigned int w, unsigned int h);
extern void init_track_keycode_state(void);

extern void XTRAP_FakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay);
extern void XTestFakeKeyEvent_wr(Display* dpy, KeyCode key, Bool down,
    unsigned long delay);
extern void XTRAP_FakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay);
extern void XTestFakeButtonEvent_wr(Display* dpy, unsigned int button, Bool is_press,
    unsigned long delay);
extern void XTRAP_FakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay);
extern void XTestFakeMotionEvent_wr(Display* dpy, int screen, int x, int y,
    unsigned long delay);

extern Bool XTestCompareCurrentCursorWithWindow_wr(Display* dpy, Window w);
extern Bool XTestCompareCursorWithWindow_wr(Display* dpy, Window w, Cursor cursor);
extern Bool XTestQueryExtension_wr(Display *dpy, int *ev, int *er, int *maj,
    int *min);
extern void XTestDiscard_wr(Display *dpy);
extern Bool XETrapQueryExtension_wr(Display *dpy, int *ev, int *er, int *op);
extern int XTestGrabControl_wr(Display *dpy, Bool impervious);
extern int XTRAP_GrabControl_wr(Display *dpy, Bool impervious);
extern void disable_grabserver(Display *in_dpy, int change);

extern Bool XRecordQueryVersion_wr(Display *dpy, int *maj, int *min);

extern int xauth_raw(int on);
extern Display *XOpenDisplay_wr(char *display_name);
extern int XCloseDisplay_wr(Display *display);

extern Bool XQueryPointer_wr(Display *display, Window w, Window *root_return,
    Window *child_return, int *root_x_return, int *root_y_return,
    int *win_x_return, int *win_y_return, unsigned int *mask_return);

extern Status XQueryTree_wr(Display *display, Window w, Window *root_return,
    Window *parent_return, Window **children_return,
    unsigned int *nchildren_return);

extern int XFree_wr(void *data);
extern int XSelectInput_wr(Display *display, Window w, long event_mask);


#endif /* _X11VNC_XWRAPPERS_H */
