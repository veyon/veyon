#ifndef _X11VNC_CURSOR_H
#define _X11VNC_CURSOR_H

/* -- cursor.h -- */

extern int xfixes_present;
extern int use_xfixes;
extern int got_xfixes_cursor_notify;
extern int cursor_changes;
extern int alpha_threshold;
extern double alpha_frac;
extern int alpha_remove;
extern int alpha_blend;
extern int alt_arrow;
extern int alt_arrow_max;


extern void first_cursor(void);
extern void setup_cursors_and_push(void);
extern void initialize_xfixes(void);
extern int known_cursors_mode(char *s);
extern void initialize_cursors_mode(void);
extern int get_which_cursor(void);
extern void restore_cursor_shape_updates(rfbScreenInfoPtr s);
extern void disable_cursor_shape_updates(rfbScreenInfoPtr s);
extern int cursor_shape_updates_clients(rfbScreenInfoPtr s);
extern int cursor_noshape_updates_clients(rfbScreenInfoPtr s);
extern int cursor_pos_updates_clients(rfbScreenInfoPtr s);
extern void cursor_position(int x, int y);
extern void set_no_cursor(void);
extern void set_warrow_cursor(void);
extern int set_cursor(int x, int y, int which);
extern int check_x11_pointer(void);
extern int store_cursor(int serial, unsigned long *data, int w, int h, int cbpp, int xhot, int yhot);
extern unsigned long get_cursor_serial(int mode);

#endif /* _X11VNC_CURSOR_H */
