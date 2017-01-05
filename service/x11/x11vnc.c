#include <rfb/rfbconfig.h>
#include <rfb/keysym.h>

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

#ifdef LIBVNCSERVER_HAVE_RECORD
#include <X11/Xproto.h>
#endif

#include "x11vnc/avahi.c"
#include "x11vnc/pm.c"
#include "x11vnc/rates.c"
#include "x11vnc/cleanup.c"
#include "x11vnc/remote.c"
#include "x11vnc/pointer.c"
#include "x11vnc/userinput.c"
#include "x11vnc/unixpw.c"
#include "x11vnc/gui.c"
#include "x11vnc/xkb_bell.c"
#include "x11vnc/xinerama.c"
#include "x11vnc/solid.c"
#include "x11vnc/selection.c"
#include "x11vnc/xrandr.c"
#include "x11vnc/win_utils.c"
#include "x11vnc/cursor.c"
#include "x11vnc/screen.c"
#include "x11vnc/xevents.c"
#include "x11vnc/help.c"
#include "x11vnc/inet.c"
#include "x11vnc/sslcmds.c"
#include "x11vnc/xwrappers.c"
#include "x11vnc/scan.c"
#include "x11vnc/options.c"
#include "x11vnc/user.c"
#include "x11vnc/util.c"
#include "x11vnc/x11vnc_defs.c"
#include "x11vnc/xrecord.c"
#include "x11vnc/8to24.c"
#include "x11vnc/xdamage.c"
#include "x11vnc/keyboard.c"
#include "x11vnc/connections.c"
#include "x11vnc/sslhelper.c"
#include "x11vnc/linuxfb.c"
#include "x11vnc/v4l.c"
#include "x11vnc/macosx.c"
#include "x11vnc/macosxCG.c"
#include "x11vnc/macosxCGP.c"
#include "x11vnc/macosxCGS.c"

#define db db2
#include "x11vnc/uinput.c"
#undef db

#define SHOW_NO_PASSWORD_WARNING 0
#define main x11vnc_main

#include "x11vnc/x11vnc.c"

