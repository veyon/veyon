#ifndef _X11VNC_GUI_H
#define _X11VNC_GUI_H

/* -- gui.h -- */

extern int icon_mode;
extern char *icon_mode_file;
extern FILE *icon_mode_fh;
extern int icon_mode_socks[];
extern int tray_manager_ok;
extern Window tray_request;
extern Window tray_window;
extern int tray_unembed;
extern pid_t run_gui_pid;
extern pid_t gui_pid;

extern char *get_gui_code(void);
extern int tray_embed(Window iconwin, int remove);
extern void do_gui(char *opts, int sleep);

#endif /* _X11VNC_GUI_H */
