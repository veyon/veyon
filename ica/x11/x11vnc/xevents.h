#ifndef _X11VNC_XEVENTS_H
#define _X11VNC_XEVENTS_H

/* -- xevents.h -- */

extern int grab_buster;
extern int grab_kbd;
extern int grab_ptr;
extern int grab_always;
extern int grab_local;
extern int sync_tod_delay;

extern void initialize_vnc_connect_prop(void);
extern void initialize_x11vnc_remote_prop(void);
extern void initialize_clipboard_atom(void);
extern void spawn_grab_buster(void);
extern void sync_tod_with_servertime(void);
extern void check_keycode_state(void);
extern void check_autorepeat(void);
extern void set_prop_atom(Atom atom);
extern void check_xevents(int reset);
extern void xcut_receive(char *text, int len, rfbClientPtr cl);

extern void kbd_release_all_keys(rfbClientPtr cl);
extern void set_single_window(rfbClientPtr cl, int x, int y);
extern void set_server_input(rfbClientPtr cl, int s);
extern void set_text_chat(rfbClientPtr cl, int l, char *t);
extern int get_keyboard_led_state_hook(rfbScreenInfoPtr s);
extern int get_file_transfer_permitted(rfbClientPtr cl);
extern void get_prop(char *str, int len, Atom prop);


#endif /* _X11VNC_XEVENTS_H */
