#include <rfb/rfbconfig.h>
#include <rfb/keysym.h>
#include <config.h>

#ifdef WIN32
#define WNOHANG 1
#define SIGUSR1 10
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>
static int geteuid()
{
	errno = ENOSYS;
	return -1;
}
static int getuid()
{
	errno = ENOSYS;
	return -1;
}
static int kill(int pid, int sig)
{
	errno = ENOSYS;
	return -1;
}
static int fork()
{
	errno = ENOSYS;
	return -1;
}
static int waitpid(int pid, int * loc, int options)
{
	errno = ENOSYS;
	return -1;
}
static int mkstemp(char * template)
{
	mktemp(template);
	return open(template, O_RDWR);
}
#endif

#include "x11vnc/src/avahi.c"
#include "x11vnc/src/pm.c"
#include "x11vnc/src/rates.c"
#include "x11vnc/src/cleanup.c"
#include "x11vnc/src/remote.c"
#include "x11vnc/src/pointer.c"
#include "x11vnc/src/userinput.c"
#include "x11vnc/src/unixpw.c"
#include "x11vnc/src/gui.c"
#include "x11vnc/src/xkb_bell.c"
#include "x11vnc/src/xinerama.c"
#include "x11vnc/src/solid.c"
#include "x11vnc/src/selection.c"
#include "x11vnc/src/xrandr.c"
#include "x11vnc/src/win_utils.c"
#include "x11vnc/src/cursor.c"
#include "x11vnc/src/screen.c"
#include "x11vnc/src/xevents.c"
#include "x11vnc/src/help.c"
#include "x11vnc/src/inet.c"
#include "x11vnc/src/sslcmds.c"
#include "x11vnc/src/xwrappers.c"
#include "x11vnc/src/scan.c"
#include "x11vnc/src/options.c"
#include "x11vnc/src/user.c"
#include "x11vnc/src/util.c"
#include "x11vnc/src/x11vnc_defs.c"
#include "x11vnc/src/xrecord.c"
#include "x11vnc/src/8to24.c"
#include "x11vnc/src/xdamage.c"
#include "x11vnc/src/keyboard.c"
#include "x11vnc/src/connections.c"
#include "x11vnc/src/sslhelper.c"
#include "x11vnc/src/linuxfb.c"
#include "x11vnc/src/v4l.c"
#include "x11vnc/src/macosx.c"
#include "x11vnc/src/macosxCG.c"
#include "x11vnc/src/macosxCGP.c"
#include "x11vnc/src/macosxCGS.c"
#include "x11vnc/src/xi2_devices.c"

#define db db2
#include "x11vnc/src/uinput.c"
#undef db

#undef main
#undef SHOW_NO_PASSWORD_WARNING

#define SHOW_NO_PASSWORD_WARNING 0
#define main x11vnc_main

#include "x11vnc/src/x11vnc.c"

