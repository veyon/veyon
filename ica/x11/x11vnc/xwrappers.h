#ifndef _X11VNC_XWRAPPERS_H
#define _X11VNC_XWRAPPERS_H

/* -- xwrappers.h -- */

extern int xshm_present;
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
