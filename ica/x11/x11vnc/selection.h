#ifndef _X11VNC_SELECTION_H
#define _X11VNC_SELECTION_H

/* -- selection.h -- */

extern char *xcut_str_primary;
extern char *xcut_str_clipboard;
extern int own_primary;
extern int set_primary;
extern int own_clipboard;
extern int set_clipboard;
extern int set_cutbuffer;
extern int sel_waittime;
extern Window selwin;
extern Atom clipboard_atom;

extern void selection_request(XEvent *ev, char *type);
extern int check_sel_direction(char *dir, char *label, char *sel, int len);
extern void cutbuffer_send(void);
extern void selection_send(XEvent *ev);

#endif /* _X11VNC_SELECTION_H */
