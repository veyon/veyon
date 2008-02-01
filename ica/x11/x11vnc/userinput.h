#ifndef _X11VNC_USERINPUT_H
#define _X11VNC_USERINPUT_H

/* -- userinput.h -- */

extern int defer_update_nofb;
extern int last_scroll_type;

extern int get_wm_frame_pos(int *px, int *py, int *x, int *y, int *w, int *h,
    Window *frame, Window *win);
extern void parse_scroll_copyrect(void);
extern void parse_fixscreen(void);
extern void parse_wireframe(void);

extern void set_wirecopyrect_mode(char *str);
extern void set_scrollcopyrect_mode(char *str);
extern void initialize_scroll_keys(void);
extern void initialize_scroll_matches(void);
extern void initialize_scroll_term(void);
extern void initialize_max_keyrepeat(void);

extern int direct_fb_copy(int x1, int y1, int x2, int y2, int mark);
extern void fb_push(void);
extern int fb_push_wait(double max_wait, int flags);
extern void eat_viewonly_input(int max_eat, int keep);

extern void mark_for_xdamage(int x, int y, int w, int h);
extern void mark_region_for_xdamage(sraRegionPtr region);
extern void set_xdamage_mark(int x, int y, int w, int h);
extern int near_wm_edge(int x, int y, int w, int h, int px, int py);
extern int near_scrollbar_edge(int x, int y, int w, int h, int px, int py);

extern void check_fixscreen(void);
extern int check_xrecord(void);
extern int check_wireframe(void);
extern int fb_update_sent(int *count);
extern int check_user_input(double dt, double dtr, int tile_diffs, int *cnt);
extern void do_copyregion(sraRegionPtr region, int dx, int dy, int mode);

extern int check_ncache(int reset, int mode);
extern int find_rect(int idx, int x, int y, int w, int h);
extern int lookup_win_index(Window);
extern void set_ncache_xrootpmap(void);

#endif /* _X11VNC_USERINPUT_H */
