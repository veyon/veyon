#ifndef _X11VNC_SCAN_H
#define _X11VNC_SCAN_H

/* -- scan.h -- */

extern int nap_ok;
extern int scanlines[];

extern void initialize_tiles(void);
extern void free_tiles(void);
extern void shm_delete(XShmSegmentInfo *shm);
extern void shm_clean(XShmSegmentInfo *shm, XImage *xim);
extern void initialize_polling_images(void);
extern void scale_rect(double factor, int blend, int interpolate, int Bpp,
    char *src_fb, int src_bytes_per_line, char *dst_fb, int dst_bytes_per_line,
    int Nx, int Ny, int nx, int ny, int X1, int Y1, int X2, int Y2, int mark);
extern void scale_and_mark_rect(int X1, int Y1, int X2, int Y2);
extern void mark_rect_as_modified(int x1, int y1, int x2, int y2, int force);
extern int copy_screen(void);
extern int copy_snap(void);
extern void nap_sleep(int ms, int split);
extern void set_offset(void);
extern int scan_for_updates(int count_only);

#endif /* _X11VNC_SCAN_H */
