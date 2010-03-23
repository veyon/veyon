/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

/* -- pm.c -- */
#include "x11vnc.h"
#include "cleanup.h"

void check_pm(void);
void set_dpms_mode(char *mode);
static void check_fbpm(void);
static void check_dpms(void);

#if LIBVNCSERVER_HAVE_FBPM
#include <X11/Xmd.h>
#include <X11/extensions/fbpm.h>
#endif

#if LIBVNCSERVER_HAVE_DPMS
#include <X11/extensions/dpms.h>
#endif

void check_pm(void) {
	static int skip = -1;
	if (skip < 0) {
		skip = 0;
		if (getenv("X11VNC_NO_CHECK_PM")) {
			skip = 1;
		}
	}
	if (skip) {
		return;
	}
	check_fbpm();
	check_dpms();
	/* someday dpms activities? */
}

static void check_fbpm(void) {
	static int init_fbpm = 0;
#if LIBVNCSERVER_HAVE_FBPM
	static int fbpm_capable = 0;
	static time_t last_fbpm = 0;
	int db = 0;

	CARD16 level;
	BOOL enabled;

	RAWFB_RET_VOID

	if (! init_fbpm) {
		if (getenv("FBPM_DEBUG")) {
			db = atoi(getenv("FBPM_DEBUG"));
		}
		if (FBPMCapable(dpy)) {
			fbpm_capable = 1;
			rfbLog("X display is capable of FBPM.\n");
			if (watch_fbpm) {
				rfbLog("Preventing low-power FBPM modes when"
				    " clients are connected.\n");
			}
		} else {
			if (! raw_fb_str) {
				rfbLog("X display is not capable of FBPM.\n");
			}
			fbpm_capable = 0;
		}
		init_fbpm = 1;
	}

	if (! watch_fbpm) {
		return;
	}
	if (! fbpm_capable) {
		return;
	}
	if (! client_count) {
		return;
	}
	if (time(NULL) < last_fbpm + 5) {
		return;
	}
	last_fbpm = time(NULL);

	if (FBPMInfo(dpy, &level, &enabled)) {
		if (db) fprintf(stderr, "FBPMInfo level: %d enabled: %d\n", level, enabled);

		if (enabled && level != FBPMModeOn) {
			char *from = "unknown-fbpm-state";
			XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
			trapped_xerror = 0;

			if (level == FBPMModeStandby) {
				from = "FBPMModeStandby";
			} else if (level == FBPMModeSuspend) {
				from = "FBPMModeSuspend";
			} else if (level == FBPMModeOff) {
				from = "FBPMModeOff";
			}

			rfbLog("switching FBPM state from %s to FBPMModeOn\n", from);
			
			FBPMForceLevel(dpy, FBPMModeOn);
		
			XSetErrorHandler(old_handler);
			trapped_xerror = 0;
		}
	} else {
		if (db) fprintf(stderr, "FBPMInfo failed.\n");
	}
#else
	RAWFB_RET_VOID
	if (! init_fbpm) {
		if (! raw_fb_str) {
			rfbLog("X FBPM extension not supported.\n");
		}
		init_fbpm = 1;
	}
#endif
}

void set_dpms_mode(char *mode) {
#if NO_X11
	return;
#else
	RAWFB_RET_VOID
#if LIBVNCSERVER_HAVE_DPMS
	if (dpy && DPMSCapable(dpy)) {
		CARD16 level;
		CARD16 want;
		BOOL enabled;
		if (!strcmp(mode, "off")) {
			want = DPMSModeOff;
		} else if (!strcmp(mode, "on")) {
			want = DPMSModeOn;
		} else if (!strcmp(mode, "standby")) {
			want = DPMSModeStandby;
		} else if (!strcmp(mode, "suspend")) {
			want = DPMSModeSuspend;
		} else if (!strcmp(mode, "enable")) {
			DPMSEnable(dpy);
			return;
		} else if (!strcmp(mode, "disable")) {
			DPMSDisable(dpy);
			return;
		} else {
			return;
		}
		if (DPMSInfo(dpy, &level, &enabled)) {
			char *from = "unk";
			if (enabled && level != want) {
				XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
				trapped_xerror = 0;

				rfbLog("DPMSInfo level: %d enabled: %d\n", level, enabled);
				if (level == DPMSModeStandby) {
					from = "DPMSModeStandby";
				} else if (level == DPMSModeSuspend) {
					from = "DPMSModeSuspend";
				} else if (level == DPMSModeOff) {
					from = "DPMSModeOff";
				} else if (level == DPMSModeOn) {
					from = "DPMSModeOn";
				}

				rfbLog("switching DPMS state from %s to %s\n", from, mode);
				
				DPMSForceLevel(dpy, want);
			
				XSetErrorHandler(old_handler);
				trapped_xerror = 0;
			}
		}
	}
#endif
#endif
}

static void check_dpms(void) {
	static int init_dpms = 0;
#if LIBVNCSERVER_HAVE_DPMS
	static int dpms_capable = 0;
	static time_t last_dpms = 0;
	int db = 0;

	CARD16 level;
	BOOL enabled;

	RAWFB_RET_VOID

	if (! init_dpms) {
		if (getenv("DPMS_DEBUG")) {
			db = atoi(getenv("DPMS_DEBUG"));
		}
		if (DPMSCapable(dpy)) {
			dpms_capable = 1;
			rfbLog("X display is capable of DPMS.\n");
			if (watch_dpms) {
				rfbLog("Preventing low-power DPMS modes when"
				    " clients are connected.\n");
			}
		} else {
			if (! raw_fb_str) {
				rfbLog("X display is not capable of DPMS.\n");
			}
			dpms_capable = 0;
		}
		init_dpms = 1;
	}

	if (force_dpms || (client_dpms && client_count)) {
		static int last_enable = 0;
		if (time(NULL) > last_enable) {
			set_dpms_mode("enable");
			last_enable = time(NULL);
		}
		set_dpms_mode("off");
	}
	if (! watch_dpms) {
		return;
	}
	if (! dpms_capable) {
		return;
	}
	if (! client_count) {
		return;
	}
	if (time(NULL) < last_dpms + 5) {
		return;
	}
	last_dpms = time(NULL);

	if (DPMSInfo(dpy, &level, &enabled)) {
		if (db) fprintf(stderr, "DPMSInfo level: %d enabled: %d\n", level, enabled);

		if (enabled && level != DPMSModeOn) {
			char *from = "unknown-dpms-state";
			XErrorHandler old_handler = XSetErrorHandler(trap_xerror);
			trapped_xerror = 0;

			if (level == DPMSModeStandby) {
				from = "DPMSModeStandby";
			} else if (level == DPMSModeSuspend) {
				from = "DPMSModeSuspend";
			} else if (level == DPMSModeOff) {
				from = "DPMSModeOff";
			}

			rfbLog("switching DPMS state from %s to DPMSModeOn\n", from);
			
			DPMSForceLevel(dpy, DPMSModeOn);
		
			XSetErrorHandler(old_handler);
			trapped_xerror = 0;
		}
	} else {
		if (db) fprintf(stderr, "DPMSInfo failed.\n");
	}
#else
	RAWFB_RET_VOID
	if (! init_dpms) {
		if (! raw_fb_str) {
			rfbLog("X DPMS extension not supported.\n");
		}
		init_dpms = 1;
	}
#endif
}

