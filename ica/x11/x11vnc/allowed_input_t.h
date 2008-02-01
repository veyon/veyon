#ifndef _X11VNC_ALLOWED_INPUT_T_H
#define _X11VNC_ALLOWED_INPUT_T_H

/* -- allowed_input_t.h -- */

typedef struct allowed_input {
	int keystroke;
	int motion;
	int button;
	int clipboard;
	int files;
} allowed_input_t;

#endif /* _X11VNC_ALLOWED_INPUT_T_H */
