#ifndef _X11VNC_UINPUT_H
#define _X11VNC_UINPUT_H

/* -- uinput.h -- */

extern int check_uinput(void);
extern int initialize_uinput(void);
extern int set_uinput_accel(char *str);
extern int set_uinput_thresh(char *str);
extern void set_uinput_reset(int ms);
extern void set_uinput_always(int);
extern void set_uinput_touchscreen(int);
extern void set_uinput_abs(int);
extern char *get_uinput_accel();
extern char *get_uinput_thresh();
extern int get_uinput_reset();
extern int get_uinput_always();
extern int get_uinput_touchscreen();
extern int get_uinput_abs();
extern void parse_uinput_str(char *str);
extern void uinput_pointer_command(int mask, int x, int y, rfbClientPtr client);
extern void uinput_key_command(int down, int keysym, rfbClientPtr client);



#endif /* _X11VNC_UINPUT_H */
