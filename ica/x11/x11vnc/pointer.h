#ifndef _X11VNC_POINTER_H
#define _X11VNC_POINTER_H

/* -- pointer.h -- */

extern int pointer_queued_sent;

extern void initialize_pointer_map(char *pointer_remap);
extern void do_button_mask_change(int mask, int button);
extern void pointer(int mask, int x, int y, rfbClientPtr client);
extern int check_pipeinput(void);
extern void initialize_pipeinput(void);
extern void update_x11_pointer_position(int x, int y);

#endif /* _X11VNC_POINTER_H */
