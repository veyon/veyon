#ifndef _X11VNC_ENUMS_H
#define _X11VNC_ENUMS_H

/* -- enums.h -- */

enum {
	LR_UNSET = 0,
	LR_UNKNOWN,
	LR_DIALUP,
	LR_BROADBAND,
	LR_LAN
};

enum scroll_types {
	SCR_NONE = 0,
	SCR_MOUSE,
	SCR_KEY,
	SCR_FAIL,
	SCR_SUCCESS
};

#endif /* _X11VNC_ENUMS_H */
