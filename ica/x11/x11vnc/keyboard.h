#ifndef _X11VNC_KEYBOARD_H
#define _X11VNC_KEYBOARD_H

/* -- keyboard.h -- */
#include "allowed_input_t.h"

extern void get_keystate(int *keystate);
extern void clear_modifiers(int init);
extern int track_mod_state(rfbKeySym keysym, rfbBool down, rfbBool set);
extern void clear_keys(void);
extern int get_autorepeat_state(void);
extern int get_initial_autorepeat_state(void);
extern void autorepeat(int restore, int bequiet);
extern void check_add_keysyms(void);
extern int add_keysym(KeySym keysym);
extern void delete_added_keycodes(int bequiet);
extern void initialize_remap(char *infile);
extern int sloppy_key_check(int key, rfbBool down, rfbKeySym keysym, int *new);
extern void switch_to_xkb_if_better(void);
extern char *short_kmbc(char *str);
extern void initialize_allowed_input(void);
extern void initialize_modtweak(void);
extern void initialize_keyboard_and_pointer(void);
extern void get_allowed_input(rfbClientPtr client, allowed_input_t *input);
extern double typing_rate(double time_window, int *repeating);
extern int skip_cr_when_scaling(char *mode);
extern void keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client);

#endif /* _X11VNC_KEYBOARD_H */
