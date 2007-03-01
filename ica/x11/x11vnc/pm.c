/* -- pm.c -- */
#include "x11vnc.h"
#include "cleanup.h"

void check_pm(void);
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

