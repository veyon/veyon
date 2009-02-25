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
extern void free_old_fb(char *old_main, char *old_rfb, char *old_8to24, char *old_snap_fb);
extern void check_padded_fb(void);
extern void install_padded_fb(char *geom);
extern XImage *initialize_xdisplay_fb(void);
extern XImage *initialize_raw_fb(int);
extern void parse_scale_string(char *str, double *factor, int *scaling, int *blend,
    int *nomult4, int *pad, int *interpolate, int *numer, int *denom);
extern int scale_round(int len, double fac);
extern void initialize_screen(int *argc, char **argv, XImage *fb);
extern void set_vnc_desktop_name(void);

extern int rawfb_reset;
extern int rawfb_dev_video;

#endif /* _X11VNC_SCREEN_H */
