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

#ifndef _X11VNC_X11VNC_H
#define _X11VNC_X11VNC_H

/* -- x11vnc.h -- */
/* 
 * These ' -- filename.[ch] -- ' comments represent a partial cleanup:
 * they are an odd way to indicate how this huge file would be split up
 * someday into multiple files.
 *
 * The primary reason we have not broken up this file is for user
 * convenience: those wanting to use the latest version download a single
 * file, x11vnc.c, and off they go...
 */

/****************************************************************************/

/* Standard includes and libvncserver */

#include <unistd.h>
#ifndef WIN32
#include <sys/utsname.h>
#else
#include <ws2tcpip.h>
#define _POSIX
#define __USE_MINGW_ALARM
#endif
#include <signal.h>
#ifdef __hpux
/* to avoid select() compiler warning */
#include <sys/time.h>
#endif
#include <time.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <rfb/rfb.h>
#include <rfb/rfbregion.h>


/* we can now build under --without-x: */
#if LIBVNCSERVER_HAVE_X11

#define NO_X11 0
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/keysym.h>
#include <X11/Xatom.h>

#else

#define NO_X11 1
#ifndef SKIP_NO_X11
#include "nox11.h"
#endif
#include <rfb/keysym.h>

#endif

/****************************************************************************/


/*
 * Build-time customization via CPPFLAGS.
 *
 * Summary of options to include in CPPFLAGS for custom builds:
 *
 * -DVNCSHARED  to have the vnc display shared by default.
 * -DFOREVER  to have -forever on by default.
 * -DNOREPEAT=0  to have -repeat on by default.
 * -DXINERAMA=0  to have -noxinerama on by default.
 * -DADDKEYSYMS=0  to have -noadd_keysyms the default.
 *
 * -DREMOTE_DEFAULT=0  to disable remote-control on by default (-yesremote).
 * -DREMOTE_CONTROL=0  to disable remote-control mechanism completely.
 * -DEXTERNAL_COMMANDS=0  to disable the running of all external commands.
 * -DTIGHTFILEXFER=0  disable tightfilexfer.
 *
 * -DHARDWIRE_PASSWD=...      hardwired passwords, quoting necessary.
 * -DHARDWIRE_VIEWPASSWD=...
 * -DNOPW=1                   make -nopw the default (skip warning)
 * -DUSEPW=1                  make -usepw the default
 * -DPASSWD_REQUIRED=1        exit unless a password is supplied.
 * -DPASSWD_UNLESS_NOPW=1     exit unless a password is supplied and no -nopw.
 *
 * -DCURSOR_DRAG=1  to have -cursor_drag as the default.
 * -DWIREFRAME=0  to have -nowireframe as the default.
 * -DWIREFRAME_COPYRECT=0  to have -nowirecopyrect as the default.
 * -DWIREFRAME_PARMS=...   set default -wirecopyrect parameters.
 * -DSCROLL_COPYRECT=0     to have -noscrollcopyrect as the default.
 * -DSCROLL_COPYRECT_PARMS=...  set default -scrollcopyrect parameters.
 * -DSCALING_COPYRECT=0
 * -DXDAMAGE=0    to have -noxdamage as the default.
 * -DSKIPDUPS=0   to have -noskip_dups as the default or vice versa.
 *
 * -DPOINTER_MODE_DEFAULT={0,1,2,3,4}  set default -pointer_mode.
 * -DBOLDLY_CLOSE_DISPLAY=0  to not close X DISPLAY under -rawfb.
 * -DSMALL_FOOTPRINT=1  for smaller binary size (no help, no gui, etc) 
 *                      use 2 or 3 for even smaller footprint.
 * -DNOGUI  do not include the gui tkx11vnc.
 * -DSKIP_HELP=1   smaller.
 * -DSKIP_XKB=1    a little smaller.
 * -DSKIP_8to24=1  a little smaller.
 * -DPOLL_8TO24_DELAY=N  
 * -DDEBUG_XEVENTS=1  enable printout for X events.
 *
 * -DX11VNC_MACOSX_USE_GETMAINDEVICE use deprecated GetMainDevice on macosx 
 *
 * -DX11VNC_MACOSX_NO_DEPRECATED_LOCALEVENTS={0,1}
 * -DX11VNC_MACOSX_NO_DEPRECATED_POSTEVENTS={0,1}
 * -DX11VNC_MACOSX_NO_DEPRECATED_FRAMEBUFFER={0,1}
 *
 * or for all:
 *
 * -DX11VNC_MACOSX_NO_DEPRECATED=1
 *
 * env. var. of the same names as above can be set to imply true.
 *
 * Set these in CPPFLAGS before running configure. E.g.:
 *
 *   % env CPPFLAGS="-DFOREVER -DREMOTE_CONTROL=0" ./configure
 *   % make
 */

/*
 * This can be used to disable the remote control mechanism.
 */
#ifndef REMOTE_CONTROL
#define REMOTE_CONTROL 1
#endif

#ifndef XINERAMA
#define XINERAMA 1
#endif

#ifndef NOPW
#define NOPW 0
#endif

#ifndef USEPW
#define USEPW 0
#endif

#ifndef PASSWD_REQUIRED
#define PASSWD_REQUIRED 0
#endif

#ifndef PASSWD_UNLESS_NOPW
#define PASSWD_UNLESS_NOPW 0
#endif

/* some -D macros for building with old LibVNCServer */
#ifndef LIBVNCSERVER_HAS_STATS
#define LIBVNCSERVER_HAS_STATS 1
#endif

#ifndef LIBVNCSERVER_HAS_SHUTDOWNSOCKETS
#define LIBVNCSERVER_HAS_SHUTDOWNSOCKETS 1
#endif

#ifndef LIBVNCSERVER_HAS_TEXTCHAT
#define LIBVNCSERVER_HAS_TEXTCHAT 1
#endif

#ifdef PRE_0_8_LIBVNCSERVER
#undef  LIBVNCSERVER_WITH_TIGHTVNC_FILETRANSFER
#undef  LIBVNCSERVER_HAS_STATS
#undef  LIBVNCSERVER_HAS_SHUTDOWNSOCKETS
#undef  LIBVNCSERVER_HAS_TEXTCHAT 
#define LIBVNCSERVER_HAS_STATS 0
#define LIBVNCSERVER_HAS_SHUTDOWNSOCKETS 0
#define LIBVNCSERVER_HAS_TEXTCHAT 0
#endif

/* these are for delaying features: */
#define xxNO_NCACHE

/*
 * Beginning of support for small binary footprint build for embedded
 * systems, PDA's etc.  It currently just cuts out the low-hanging
 * fruit (large text passages).  Set to 2, 3 to cut out some of the
 * more esoteric extensions.  More tedious is to modify LDFLAGS in the
 * Makefile to not link against the extension libraries... but that
 * should be done too (manually for now).
 *
 * If there is interest more of the bloat can be removed...  Currently
 * these shrink the binary from 1100K to about 600K.
 */
#ifndef SMALL_FOOTPRINT
#define SMALL_FOOTPRINT 0
#endif

#ifndef SKIP_XKB
#define SKIP_XKB 0
#endif
#ifndef SKIP_8TO24
#define SKIP_8TO24 0
#endif
#ifndef SKIP_HELP
#define SKIP_HELP 0
#endif

#if SMALL_FOOTPRINT
#undef  NOGUI
#define NOGUI
#undef  SKIP_HELP
#define SKIP_HELP 1
#endif

#if (SMALL_FOOTPRINT > 1)
#undef SKIP_XKB
#undef SKIP_8TO24
#undef LIBVNCSERVER_HAVE_LIBXINERAMA
#undef LIBVNCSERVER_HAVE_LIBXFIXES
#undef LIBVNCSERVER_HAVE_LIBXDAMAGE
#define SKIP_XKB 1
#define SKIP_8TO24 1
#define LIBVNCSERVER_HAVE_LIBXINERAMA 0
#define LIBVNCSERVER_HAVE_LIBXFIXES 0
#define LIBVNCSERVER_HAVE_LIBXDAMAGE 0
#endif

#if (SMALL_FOOTPRINT > 2)
#undef LIBVNCSERVER_HAVE_UTMPX_H
#undef LIBVNCSERVER_HAVE_PWD_H
#undef REMOTE_CONTROL
#define LIBVNCSERVER_HAVE_UTMPX_H 0
#define LIBVNCSERVER_HAVE_PWD_H 0
#define REMOTE_CONTROL 0
#endif

#ifndef X11VNC_MACOSX_NO_DEPRECATED_LOCALEVENTS
#if     X11VNC_MACOSX_NO_DEPRECATED
#define X11VNC_MACOSX_NO_DEPRECATED_LOCALEVENTS 1
#else
#define X11VNC_MACOSX_NO_DEPRECATED_LOCALEVENTS 0
#endif
#endif

#ifndef X11VNC_MACOSX_NO_DEPRECATED_POSTEVENTS
#if     X11VNC_MACOSX_NO_DEPRECATED
#define X11VNC_MACOSX_NO_DEPRECATED_POSTEVENTS 1
#else
#define X11VNC_MACOSX_NO_DEPRECATED_POSTEVENTS 0
#endif
#endif

#ifndef X11VNC_MACOSX_NO_DEPRECATED_FRAMEBUFFER
#if     X11VNC_MACOSX_NO_DEPRECATED
#define X11VNC_MACOSX_NO_DEPRECATED_FRAMEBUFFER 1
#else
#define X11VNC_MACOSX_NO_DEPRECATED_FRAMEBUFFER 0
#endif
#endif

/*
 * Not recommended unless you know what you are getting into, but if you
 * define the HARDWIRE_PASSWD or HARDWIRE_VIEWPASSWD variables here or in
 * CPPFLAGS you can set a default -passwd and -viewpasswd string values,
 * perhaps this would be better than nothing on an embedded system, etc.
 * These default values will be overridden by the command line.
 * We don't even give an example ;-)
 */

/****************************************************************************/

/* Extensions and related includes: */

#if LIBVNCSERVER_HAVE_XSHM
#  if defined(__hpux) && defined(__ia64)  /* something weird on hp/itanic */
#    undef _INCLUDE_HPUX_SOURCE
#  endif
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#if LIBVNCSERVER_HAVE_SHMAT
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <dirent.h>

#if LIBVNCSERVER_HAVE_XTEST
#include <X11/extensions/XTest.h>
#endif
extern int xtest_base_event_type;

#if LIBVNCSERVER_HAVE_LIBXTRAP
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/extensions/xtraplib.h>
#include <X11/extensions/xtraplibp.h>
extern XETC *trap_ctx;
#endif
extern int xtrap_base_event_type;

#if LIBVNCSERVER_HAVE_RECORD
#include <X11/Xproto.h>
#include <X11/extensions/record.h>
#endif

#if LIBVNCSERVER_HAVE_XKEYBOARD
#include <X11/XKBlib.h>
#endif

#if LIBVNCSERVER_HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#if LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef WIN32
#include <netdb.h>
#endif

#if !defined(_AIX) && !defined(WIN32)
extern int h_errno;
#endif

#if LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif

#ifndef SOL_IPV6
#ifdef  IPPROTO_IPV6
#define SOL_IPV6 IPPROTO_IPV6
#endif
#endif

#ifndef IPV6_V6ONLY
#ifdef  IPV6_BINDV6ONLY
#define IPV6_V6ONLY IPV6_BINDV6ONLY
#endif
#endif

#ifndef X11VNC_IPV6
#if defined(AF_INET6)
#define X11VNC_IPV6 1
#else
#define X11VNC_IPV6 0
#endif
#endif

#ifndef X11VNC_LISTEN6
#define X11VNC_LISTEN6 1
#endif

#if !X11VNC_IPV6
#undef  X11VNC_LISTEN6
#define X11VNC_LISTEN6 0
#endif

#if LIBVNCSERVER_HAVE_PWD_H
#include <pwd.h>
#include <grp.h>
#endif
#if LIBVNCSERVER_HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if LIBVNCSERVER_HAVE_UTMPX_H
#include <utmpx.h>
#endif

#if LIBVNCSERVER_HAVE_MMAP
#include <sys/mman.h>
#endif

/*
 * overlay/multi-depth screen reading support
 * undef SOLARIS_OVERLAY or IRIX_OVERLAY if there are problems building.
 */

/* solaris/sun */
#if defined (__SVR4) && defined (__sun)
# define SOLARIS
# ifdef LIBVNCSERVER_HAVE_SOLARIS_XREADSCREEN
#  define SOLARIS_OVERLAY
#  define OVERLAY_OS
# endif
#endif

#ifdef SOLARIS_OVERLAY
#include <X11/extensions/transovl.h>
#endif

/* irix/sgi */
#if defined(__sgi)
# define IRIX
# ifdef LIBVNCSERVER_HAVE_IRIX_XREADDISPLAY
#  define IRIX_OVERLAY
#  define OVERLAY_OS
# endif
#endif

/*
 * For reference, the OS header defines:
 __SVR4 && __sun   is solaris
 __sgi
 __hpux
 __osf__
 __OpenBSD__
 __FreeBSD__
 __NetBSD__
 __linux__
 (defined(__MACH__) && defined(__APPLE__))
 _AIX
 */
#if (defined(__MACH__) && defined(__APPLE__))
#define MACOSX
#endif


#ifdef IRIX_OVERLAY
#include <X11/extensions/readdisplay.h>
#endif

extern int overlay_present;

#if LIBVNCSERVER_HAVE_LIBXRANDR
#include <X11/extensions/Xrandr.h>
#endif
extern int xrandr_base_event_type;

#if LIBVNCSERVER_HAVE_LIBXFIXES
#include <X11/extensions/Xfixes.h>
#endif
extern int xfixes_base_event_type;

#if LIBVNCSERVER_HAVE_LIBXDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
extern int xdamage_base_event_type;

#define RAWFB_RET(y)   if (raw_fb && ! dpy) return y;
#define RAWFB_RET_VOID if (raw_fb && ! dpy) return;

extern char lastmod[];

/* X display info */

extern Display *dpy;		/* the single display screen we connect to */
extern int scr;
extern char *xauth_raw_data;
extern int xauth_raw_len;
extern Window window, rootwin;		/* polled window, root window (usu. same) */
extern Visual *default_visual;		/* the default visual (unless -visual) */
extern int bpp, depth;
extern int indexed_color;
extern int dpy_x, dpy_y;		/* size of display */
extern int fb_x, fb_y, fb_b;		/* fb size and bpp guesses at display */
extern int off_x, off_y;		/* offsets for -sid */
extern int wdpy_x, wdpy_y;		/* for actual sizes in case of -clip */
extern int cdpy_x, cdpy_y, coff_x, coff_y;	/* the -clip params */
extern int button_mask;		/* button state and info */
extern int button_mask_prev;
extern int num_buttons;

extern long xselectinput_rootwin;

extern unsigned int display_button_mask;
extern unsigned int display_mod_mask;

/* image structures */
extern XImage *scanline;
extern XImage *fullscreen;
extern XImage **tile_row;	/* for all possible row runs */
extern XImage *snaprect;	/* for XShmGetImage (fs_factor) */
extern XImage *snap;		/* the full snap fb */
extern XImage *raw_fb_image;	/* the raw fb */

#if !LIBVNCSERVER_HAVE_XSHM
/*
 * for simplicity, define this struct since we'll never use them
 * under using_shm = 0.
 */
typedef struct {
	int shmid; char *shmaddr; Bool readOnly;
} XShmSegmentInfo;
#endif

/* corresponding shm structures */
extern XShmSegmentInfo scanline_shm;
extern XShmSegmentInfo fullscreen_shm;
extern XShmSegmentInfo *tile_row_shm;	/* for all possible row runs */
extern XShmSegmentInfo snaprect_shm;

/* rfb screen info */
extern rfbScreenInfoPtr screen;
extern char *rfb_desktop_name;
extern char *http_dir;
extern char vnc_desktop_name[];
extern char *main_fb;			/* our copy of the X11 fb */
extern char *rfb_fb;			/* same as main_fb unless transformation */
extern char *fake_fb;			/* used under -padgeom */
extern char *snap_fb;			/* used under -snapfb */
extern char *cmap8to24_fb;		/* used under -8to24 */
extern char *rot_fb;			/* used under -rotate */
extern char *raw_fb;
extern char *raw_fb_addr;
extern int raw_fb_offset;
extern int raw_fb_shm;
extern int raw_fb_mmap;
extern int raw_fb_seek;
extern int raw_fb_fd;
extern int raw_fb_back_to_X;

extern int raw_fb_native_bpp;
extern int raw_fb_expand_bytes;
extern unsigned long  raw_fb_native_red_mask,  raw_fb_native_green_mask,  raw_fb_native_blue_mask;
extern unsigned short raw_fb_native_red_max,   raw_fb_native_green_max,   raw_fb_native_blue_max;
extern unsigned short raw_fb_native_red_shift, raw_fb_native_green_shift, raw_fb_native_blue_shift;

extern int rfb_bytes_per_line;
extern int main_bytes_per_line;
extern int rot_bytes_per_line;
extern unsigned long  main_red_mask,  main_green_mask,  main_blue_mask;
extern unsigned short main_red_max,   main_green_max,   main_blue_max;
extern unsigned short main_red_shift, main_green_shift, main_blue_shift;

extern int raw_fb_bytes_per_line;	/* of actual raw region we poll, not our raw_fb */

/* scaling parameters */
extern char *scale_str;
extern double scale_fac_x;
extern double scale_fac_y;
extern int scaling;
extern int scaling_blend;		/* for no blending option (very course) */
extern int scaling_nomult4;		/* do not require width = n * 4 */
extern int scaling_pad;		/* pad out scaled sizes to fit denominator */
extern int scaling_interpolate;	/* use interpolation scheme when shrinking */
extern int scaled_x, scaled_y;		/* dimensions of scaled display */
extern int scale_numer, scale_denom;	/* n/m */
extern char *rotating_str;
extern int rotating;
extern int rotating_same;
extern int rotating_cursors;

/* scale cursor */
extern char *scale_cursor_str;
extern double scale_cursor_fac_x;
extern double scale_cursor_fac_y;
extern int scaling_cursor;
extern int scaling_cursor_blend;
extern int scaling_cursor_interpolate;
extern int scale_cursor_numer, scale_cursor_denom;

/* size of the basic tile unit that is polled for changes: */
extern int tile_x;
extern int tile_y;
extern int ntiles, ntiles_x, ntiles_y;

/* arrays that indicate changed or checked tiles. */
extern unsigned char *tile_has_diff, *tile_tried, *tile_copied;
extern unsigned char *tile_has_xdamage_diff, *tile_row_has_xdamage_diff;

/* times of recent events */
extern time_t last_event, last_input, last_client, last_open_xdisplay;
extern time_t last_keyboard_input, last_pointer_input; 
extern time_t last_local_input;	/* macosx */
extern time_t last_fb_bytes_sent;
extern double last_keyboard_time;
extern double last_pointer_time;
extern double last_pointer_click_time;
extern double last_pointer_motion_time;
extern double last_key_to_button_remap_time;
extern double last_copyrect;
extern double last_copyrect_fix;
extern double last_wireframe;
extern double servertime_diff;
extern double x11vnc_start;
extern double x11vnc_current;
extern double g_now;

extern double last_get_wm_frame_time;
extern Window last_get_wm_frame;
extern double last_bs_restore;
extern double last_su_restore;
extern double last_bs_save;
extern double last_su_save;

extern int hack_val;

/* last client to move pointer */
extern rfbClientPtr last_pointer_client;
extern rfbClientPtr latest_client;
extern double last_client_gone;
extern double last_new_client;

extern int waited_for_client;
extern int findcreatedisplay;
extern char *terminal_services_daemon;

extern int client_count;
extern int clients_served;
extern int client_normal_count;

/* more transient kludge variables: */
extern int cursor_x, cursor_y;		/* x and y from the viewer(s) */
extern int button_change_x, button_change_y;
extern int got_user_input;
extern int got_pointer_input;
extern int got_local_pointer_input;
extern int got_pointer_calls;
extern int got_keyboard_input;
extern int got_keyboard_calls;
extern int urgent_update;
extern int last_keyboard_keycode;
extern rfbBool last_rfb_down;
extern rfbBool last_rfb_key_accepted;
extern rfbKeySym last_rfb_keysym;
extern double last_rfb_keytime;
extern double last_rfb_key_injected;
extern double last_rfb_ptr_injected;
extern int fb_copy_in_progress;	
extern int drag_in_progress;	
extern int shut_down;	
extern int do_copy_screen;	
extern time_t damage_time;
extern int damage_delay;

extern int program_pid;
extern char *program_name;
extern char *program_cmdline;

extern struct utsname UT;

typedef struct hint {
	/* location x, y, height, and width of a change-rectangle  */
	/* (grows as adjacent horizontal tiles are glued together) */
	int x, y, w, h;
} hint_t;

/* struct with client specific data: */
#define CILEN 10
typedef struct _ClientData {
	int uid;
	char *hostname;
	char *username;
	char *unixname;
	int client_port;
	int server_port;
	char *server_ip;
	char input[CILEN];
	int login_viewonly;
	time_t login_time;

	pid_t ssl_helper_pid;

	int had_cursor_shape_updates;
	int had_cursor_pos_updates;

	double timer;
	double send_cmp_rate;
	double send_raw_rate;
	double latency;
	int cmp_bytes_sent;
	int raw_bytes_sent;

} ClientData;

extern void nox11_exit(int rc);

#include "params.h"
#include "enums.h"
#include "options.h"
#include "util.h"

#endif /* _X11VNC_X11VNC_H */
