#ifndef _X11VNC_WIN_UTILS_H
#define _X11VNC_WIN_UTILS_H

/* -- win_utils.h -- */
#include "xinerama.h"
#include "winattr_t.h"

extern winattr_t *stack_list;
extern int stack_list_len;
extern int stack_list_num;

extern Window parent_window(Window win, char **name);
extern int valid_window(Window win, XWindowAttributes *attr_ret, int bequiet);
extern Bool xtranslate(Window src, Window dst, int src_x, int src_y, int *dst_x,
    int *dst_y, Window *child, int bequiet);
extern int get_window_size(Window win, int *x, int *y);
extern void snapshot_stack_list(int free_only, double allowed_age);
extern void update_stack_list(void);
extern Window query_pointer(Window start);
extern unsigned int mask_state(void);
extern int pick_windowid(unsigned long *num);
extern Window descend_pointer(int depth, Window start, char *name_info, int len);

#endif /* _X11VNC_WIN_UTILS_H */
