/* -- macosxCGP.c -- */

void macosxCGP_unused(void) {}

#if (defined(__MACH__) && defined(__APPLE__))

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

int macosxCGP_save_dim(void);
int macosxCGP_restore_dim(void);
int macosxCGP_save_sleep(void);
int macosxCGP_restore_sleep(void);
int macosxCGP_init_dimming(void);
int macosxCGP_undim(void);
int macosxCGP_dim_shutdown(void);
void macosxCGP_screensaver_timer_off(void);
void macosxCGP_screensaver_timer_on(void);

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/pwr_mgt/IOPM.h>

extern CGDirectDisplayID displayID;

static unsigned long dim_time;
static unsigned long sleep_time;
static int dim_time_saved = 0;
static int sleep_time_saved = 0;
static int initialized = 0;
static mach_port_t master_dev_port;
static io_connect_t power_mgt;

extern int client_count;
extern int macosx_nodimming;
extern int macosx_nosleep;
extern int macosx_noscreensaver;

static EventLoopTimerUPP  sstimerUPP;
static EventLoopTimerRef  sstimer;

void macosxCG_screensaver_timer(EventLoopTimerRef timer, void *data) {
	if (0) fprintf(stderr, "macosxCG_screensaver_timer: %d\n", (int) time(0));
	if (!timer || !data) {}
	if (macosx_nosleep && client_count) {
		if (0) fprintf(stderr, "UpdateSystemActivity: %d\n", (int) time(0));
		UpdateSystemActivity(IdleActivity);
	}
}

void macosxCGP_screensaver_timer_off(void) {
	if (0) fprintf(stderr, "macosxCGP_screensaver_timer_off: %d\n", (int) time(0));
	RemoveEventLoopTimer(sstimer);
	DisposeEventLoopTimerUPP(sstimerUPP);
}

void macosxCGP_screensaver_timer_on(void) {
	if (0) fprintf(stderr, "macosxCGP_screensaver_timer_on: %d\n", (int) time(0));
	sstimerUPP = NewEventLoopTimerUPP(macosxCG_screensaver_timer);
	InstallEventLoopTimer(GetMainEventLoop(), kEventDurationSecond * 30,
	    kEventDurationSecond * 30, sstimerUPP, NULL, &sstimer);
}

int macosxCGP_save_dim(void) {
	if (IOPMGetAggressiveness(power_mgt, kPMMinutesToDim,
	    &dim_time) != kIOReturnSuccess) {
		return 0;
	}
	dim_time_saved = 1;
	return 1;
}

int macosxCGP_restore_dim(void) {
	if (! dim_time_saved) {
		return 0;
	}
	if (IOPMSetAggressiveness(power_mgt, kPMMinutesToDim,
	    dim_time) != kIOReturnSuccess) {
		return 0;
	}
	dim_time_saved = 0;
	dim_time = 0;
	return 1;
}

int macosxCGP_save_sleep(void) {
	if (IOPMGetAggressiveness(power_mgt, kPMMinutesToSleep,
	    &sleep_time) != kIOReturnSuccess) {
		return 0;
	}
	sleep_time_saved = 1;
	return 1;
}

int macosxCGP_restore_sleep(void) {
	if (! sleep_time_saved) {
		return 0;
	}
	if (IOPMSetAggressiveness(power_mgt, kPMMinutesToSleep,
	    dim_time) != kIOReturnSuccess) {
		return 0;
	}
	sleep_time_saved = 0;
	sleep_time = 0;
	return 1;
}

int macosxCGP_init_dimming(void) {
	if (IOMasterPort(bootstrap_port, &master_dev_port) != 
	    kIOReturnSuccess) {
		return 0;
	}
	if (!(power_mgt = IOPMFindPowerManagement(master_dev_port))) {
		return 0;
	}
	if (macosx_nodimming) {
		if (! macosxCGP_save_dim()) {
			return 0;
		}
		if (IOPMSetAggressiveness(power_mgt, kPMMinutesToDim, 0)
		    != kIOReturnSuccess) {
			return 0;
		}
	}
	if (macosx_nosleep) {
		if (! macosxCGP_save_sleep()) {
			return 0;
		}
		if (IOPMSetAggressiveness(power_mgt, kPMMinutesToSleep, 0)
		    != kIOReturnSuccess) {
			return 0;
		}
	}

	initialized = 1;
	return 1;
}

int macosxCGP_undim(void) {
	if (! initialized) {
		return 0;
	}
	if (! macosx_nodimming) {
		if (! macosxCGP_save_dim()) {
			return 0;
		}
		if (IOPMSetAggressiveness(power_mgt, kPMMinutesToDim, 0)
		    != kIOReturnSuccess) {
			return 0;
		}
		if (! macosxCGP_restore_dim()) {
			return 0;
		}
	}
	if (! macosx_nosleep) {
		if (! macosxCGP_save_sleep()) {
			return 0;
		}
		if (IOPMSetAggressiveness(power_mgt, kPMMinutesToSleep, 0)
		    != kIOReturnSuccess) {
			return 0;
		}
		if (! macosxCGP_restore_sleep()) {
			return 0;
		}
	}
	return 1;
}

int macosxCGP_dim_shutdown(void) {
	if (! initialized) {
		return 0;
	}
	if (dim_time_saved) {
		if (! macosxCGP_restore_dim()) {
			return 0;
		}
	}
	if (sleep_time_saved) {
		if (! macosxCGP_restore_sleep()) {
			return 0;
		}
	}
	return 1;
}

#endif	/* __APPLE__ */


