#ifndef _X11VNC_SCREEN_H
#define _X11VNC_SCREEN_H

/* -- screen.h -- */

extern void set_greyscale_colormap(void);
extern void set_hi240_colormap(void);
extern void unset_colormap(void);
extern void set_colormap(int reset);
extern void set_nofb_params(int restore);
extern void set_raw_fb_params(int restore);
extern void do_new_fb(int reset_mem);
extern void free_old_fb(void);
extern void check_padded_fb(void);
extern void install_padded_fb(char *geom);
extern XImage *initialize_xdisplay_fb(void);
extern XImage *initialize_raw_fb(int);
extern void parse_scale_string(char *str, double *factor, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom);
extern int parse_rotate_string(char *str, int *mode);
extern int scale_round(int len, double fac);
extern void initialize_screen(int *argc, char **argv, XImage *fb);
extern void set_vnc_desktop_name(void);
extern void announce(int lport, int ssl, char *iface);

extern char *vnc_reflect_guess(char *str, char **raw_fb_addr);
extern void vnc_reflect_process_client(void);
extern rfbBool vnc_reflect_send_pointer(int x, int y, int mask);
extern rfbBool vnc_reflect_send_key(uint32_t key, rfbBool down);
extern rfbBool vnc_reflect_send_cuttext(char *str, int len);

extern int rawfb_reset;
extern int rawfb_dev_video;
extern int rawfb_vnc_reflect;

#endif /* _X11VNC_SCREEN_H */
