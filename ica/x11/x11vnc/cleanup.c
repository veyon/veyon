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

/* -- cleanup.c -- */

#include "x11vnc.h"
#include "xwrappers.h"
#include "xdamage.h"
#include "remote.h"
#include "keyboard.h"
#include "scan.h"
#include "gui.h"
#include "solid.h"
#include "unixpw.h"
#include "sslcmds.h"
#include "sslhelper.h"
#include "connections.h"
#include "macosx.h"
#include "macosxCG.h"
#include "avahi.h"
#include "screen.h"
#include "xrecord.h"
#include "xevents.h"
#include "uinput.h"

/*
 * Exiting and error handling routines
 */

int trapped_xerror = 0;
int trapped_xioerror = 0;
int trapped_getimage_xerror = 0;
int trapped_record_xerror = 0;
XErrorEvent *trapped_xerror_event;

/* XXX CHECK BEFORE RELEASE */
int crash_debug = 0;

void clean_shm(int quick);
void clean_up_exit(int ret);
int trap_xerror(Display *d, XErrorEvent *error);
int trap_xioerror(Display *d);
int trap_getimage_xerror(Display *d, XErrorEvent *error);
char *xerror_string(XErrorEvent *error);
void initialize_crash_handler(void);
void initialize_signals(void);
void unset_signals(void);
void close_exec_fds(void);
int known_sigpipe_mode(char *s);


static int exit_flag = 0;
static int exit_sig = 0;

static void clean_icon_mode(void);
static int Xerror(Display *d, XErrorEvent *error);
static int XIOerr(Display *d);
static void crash_shell_help(void);
static void crash_shell(void);
static void interrupted (int sig);


void clean_shm(int quick) {
	int i, cnt = 0;

	/*
	 * to avoid deadlock, etc, under quick=1 we just delete the shm
	 * areas and leave the X stuff hanging.
	 */
	if (quick) {
		shm_delete(&scanline_shm);
		shm_delete(&fullscreen_shm);
		shm_delete(&snaprect_shm);
	} else {
		shm_clean(&scanline_shm, scanline);
		shm_clean(&fullscreen_shm, fullscreen);
		shm_clean(&snaprect_shm, snaprect);
	}

	/* 
	 * Here we have to clean up quite a few shm areas for all
	 * the possible tile row runs (40 for 1280), not as robust
	 * as one might like... sometimes need to run ipcrm(1). 
	 */
	for(i=1; i<=ntiles_x; i++) {
		if (i > tile_shm_count) {
			break;
		}
		if (quick) {
			shm_delete(&tile_row_shm[i]);
		} else {
			shm_clean(&tile_row_shm[i], tile_row[i]);
		}
		cnt++;
		if (single_copytile_count && i >= single_copytile_count) {
			break;
		}
	}
	if (!quiet && cnt > 0) {
		rfbLog("deleted %d tile_row polling images.\n", cnt);
	}
}

static void clean_icon_mode(void) {
	if (icon_mode && icon_mode_fh) {
		fprintf(icon_mode_fh, "quit\n");
		fflush(icon_mode_fh);
		fclose(icon_mode_fh);
		icon_mode_fh = NULL;
		if (icon_mode_file) {
			unlink(icon_mode_file);
			icon_mode_file = NULL;
		}
	}
}

/*
 * Normal exiting
 */
void clean_up_exit(int ret) {
	static int depth = 0;
	exit_flag = 1;

	if (depth++ > 2) {
		return;
		//exit(ret);
	}

	if (icon_mode) {
		clean_icon_mode();
	}

	/* remove the shm areas: */
	clean_shm(0);

	stop_stunnel();
	if (use_openssl) {
		ssl_helper_pid(0, 0);	/* killall */
	}

	if (ssh_pid > 0) {
		kill(ssh_pid, SIGTERM);
		ssh_pid = 0;
	}

#ifdef MACOSX
	if (client_connect_file) {
		if (strstr(client_connect_file, "/tmp/x11vnc-macosx-remote")
		    == client_connect_file) {
			unlink(client_connect_file);
		}
	}
	if (macosx_console) {
		macosxCG_fini();
	}
#endif

	if (pipeinput_fh != NULL) {
		pclose(pipeinput_fh);
		pipeinput_fh = NULL;
	}

	shutdown_uinput();

	if (unix_sock) {
		if (unix_sock_fd >= 0) {
			rfbLog("deleting unix sock: %s\n", unix_sock);
			close(unix_sock_fd);
			unix_sock_fd = -1;
			unlink(unix_sock);
		}
	}

	if (! dpy) {	/* raw_rb hack */
		if (rm_flagfile) {
			unlink(rm_flagfile);
			rm_flagfile = NULL;
		}
		exit(ret);
	}

	/* X keyboard cleanups */
	delete_added_keycodes(0);

	if (clear_mods == 1) {
		clear_modifiers(0);
	} else if (clear_mods == 2) {
		clear_keys();
	} else if (clear_mods == 3) {
		clear_keys();
		clear_locks();
	}

	if (no_autorepeat) {
		autorepeat(1, 0);
	}
	if (use_solid_bg) {
		solid_bg(1);
	}
	if (ncache || ncache0) {
		kde_no_animate(1);
	}
	X_LOCK;
	XTestDiscard_wr(dpy);
#if LIBVNCSERVER_HAVE_LIBXDAMAGE
	if (xdamage) {
		XDamageDestroy(dpy, xdamage);
	}
#endif
#if LIBVNCSERVER_HAVE_LIBXTRAP
	if (trap_ctx) {
		XEFreeTC(trap_ctx);
	}
#endif
	/* XXX rdpy_ctrl, etc. cannot close w/o blocking */
	XCloseDisplay_wr(dpy);
	X_UNLOCK;

	fflush(stderr);

	if (rm_flagfile) {
		unlink(rm_flagfile);
		rm_flagfile = NULL;
	}

	if (avahi) {
		avahi_cleanup();
		fflush(stderr);
	}

	//exit(ret);
}

/* X11 error handlers */

static XErrorHandler   Xerror_def;
static XIOErrorHandler XIOerr_def;

int trap_xerror(Display *d, XErrorEvent *error) {
	trapped_xerror = 1;
	trapped_xerror_event = error;

	if (d) {} /* unused vars warning: */

	return 0;
}

int trap_xioerror(Display *d) {
	trapped_xioerror = 1;

	if (d) {} /* unused vars warning: */

	return 0;
}

int trap_getimage_xerror(Display *d, XErrorEvent *error) {
	trapped_getimage_xerror = 1;
	trapped_xerror_event = error;

	if (d) {} /* unused vars warning: */

	return 0;
}

/* Are silly Xorg people removing X_ShmAttach from XShm.h? */
/* INDEED!  What stupid, myopic morons...  */
/* Maintenance Monkeys busy typing at their keyboards... */
#ifndef X_ShmAttach
#define X_ShmAttach 1
#endif

static int Xerror(Display *d, XErrorEvent *error) {
	X_UNLOCK;

	if (getenv("X11VNC_PRINT_XERROR")) {
		fprintf(stderr, "Xerror: major_opcode: %d minor_opcode: %d error_code: %d\n",
		    error->request_code, error->minor_code, error->error_code);
	}

	if (xshm_opcode > 0 && error->request_code == xshm_opcode) {
		if (error->minor_code == X_ShmAttach) {
			char *dstr = DisplayString(dpy);
			fprintf(stderr, "\nX11 MIT Shared Memory Attach failed:\n");
			fprintf(stderr, "  Is your DISPLAY=%s on a remote machine?\n", dstr);
			if (strstr(dstr, "localhost:")) {
				fprintf(stderr, "  Note:   DISPLAY=localhost:N suggests a SSH X11 redir to a remote machine.\n");
			} else if (dstr[0] != ':') {
				fprintf(stderr, "  Note:   DISPLAY=hostname:N suggests a remote display.\n");
			}
			fprintf(stderr, "  Suggestion, use: x11vnc -display :0 ... for local display :0\n\n");
		}
	}

	interrupted(0);

	if (d) {} /* unused vars warning: */

	return (*Xerror_def)(d, error);
}

void watch_loop(void);

static int XIOerr(Display *d) {
	static int reopen = 0, rmax = 1;
	X_UNLOCK;

	if (getenv("X11VNC_REOPEN_DISPLAY")) {
		rmax = atoi(getenv("X11VNC_REOPEN_DISPLAY"));
	}

#if !NO_X11
	if (reopen < rmax && getenv("X11VNC_REOPEN_DISPLAY")) {
		int db = getenv("X11VNC_REOPEN_DEBUG") ? 1 : 0;
		int sleepmax = 10, i;
		Display *save_dpy = dpy;
		char *dstr = strdup(DisplayString(save_dpy));
		reopen++;	
		if (getenv("X11VNC_REOPEN_SLEEP_MAX")) {
			sleepmax = atoi(getenv("X11VNC_REOPEN_SLEEP_MAX"));
		}
		rfbLog("*** XIO error: Trying to reopen[%d/%d] display '%s'\n", reopen, rmax, dstr);
		rfbLog("*** XIO error: Note the reopened state may be unstable.\n");
		for (i=0; i < sleepmax; i++) {
			usleep (1000 * 1000);
			dpy = XOpenDisplay_wr(dstr);
			rfbLog("dpy[%d/%d]: %p\n", i+1, sleepmax, dpy);
			if (dpy) {
				break;
			}
		}
		last_open_xdisplay = time(NULL);
		if (dpy) {
			rfbLog("*** XIO error: Reopened display '%s' successfully.\n", dstr);
			if (db) rfbLog("*** XIO error: '%s' 0x%x\n", dstr, dpy);
			scr = DefaultScreen(dpy);
			rootwin = RootWindow(dpy, scr);
			if (db) rfbLog("*** XIO error: disable_grabserver\n");
			disable_grabserver(dpy, 0);
			if (db) rfbLog("*** XIO error: xrecord\n");
			zerodisp_xrecord();
			initialize_xrecord();
			if (db) rfbLog("*** XIO error: xdamage\n");
			create_xdamage_if_needed(1);
			if (db) rfbLog("*** XIO error: do_new_fb\n");
			if (using_shm) {
				if (db) rfbLog("*** XIO error: clean_shm\n");
				clean_shm(1);
			}
			do_new_fb(1);
			if (db) rfbLog("*** XIO error: check_xevents\n");
			check_xevents(1);

			/* sadly, we can never return... */
			if (db) rfbLog("*** XIO error: watch_loop\n");
			watch_loop();
			clean_up_exit(1);	
		}
	}
#endif

	interrupted(-1);

	if (d) {} /* unused vars warning: */

	return (*XIOerr_def)(d);
}

static char *xerrors[] = {
	"Success",
	"BadRequest",
	"BadValue",
	"BadWindow",
	"BadPixmap",
	"BadAtom",
	"BadCursor",
	"BadFont",
	"BadMatch",
	"BadDrawable",
	"BadAccess",
	"BadAlloc",
	"BadColor",
	"BadGC",
	"BadIDChoice",
	"BadName",
	"BadLength",
	"BadImplementation",
	"unknown"
};
static int xerrors_max = BadImplementation;

char *xerror_string(XErrorEvent *error) {
	int index = -1;
	if (error) {
		index = (int) error->error_code;
	}
	if (0 <= index && index <= xerrors_max) {
		return xerrors[index];
	} else {
		return xerrors[xerrors_max+1];
	}
}

static char *crash_stack_command1 = NULL;
static char *crash_stack_command2 = NULL;
static char *crash_debug_command = NULL;

void initialize_crash_handler(void) {
	int pid = program_pid;
	crash_stack_command1 = (char *) malloc(1000);
	crash_stack_command2 = (char *) malloc(1000);
	crash_debug_command =  (char *) malloc(1000);

	snprintf(crash_stack_command1, 500, "echo where > /tmp/gdb.%d;"
	    " env PATH=$PATH:/usr/local/bin:/usr/sfw/bin:/usr/bin"
	    " gdb -x /tmp/gdb.%d -batch -n %s %d;"
	    " rm -f /tmp/gdb.%d", pid, pid, program_name, pid, pid);
	snprintf(crash_stack_command2, 500, "pstack %d", program_pid);
	
	snprintf(crash_debug_command, 500, "gdb %s %d", program_name, pid);
}

static void crash_shell_help(void) {
	int pid = program_pid;
	fprintf(stderr, "\n");
	fprintf(stderr, "   *** Welcome to the x11vnc crash shell! ***\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "PROGRAM: %s  PID: %d\n", program_name, pid);
	fprintf(stderr, "\n");
	fprintf(stderr, "POSSIBLE DEBUGGER COMMAND:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  %s\n", crash_debug_command);
	fprintf(stderr, "\n");
	fprintf(stderr, "Press \"q\" to quit.\n");
	fprintf(stderr, "Press \"h\" or \"?\" for this help.\n");
	fprintf(stderr, "Press \"s\" to try to run some commands to"
	    " show a stack trace (gdb/pstack).\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Anything else is passed to -Q query function.\n");
	fprintf(stderr, "\n");
}

static void crash_shell(void) {
	char qry[1000], cmd[1000], line[1000];
	char *str, *p;

	crash_shell_help();
	fprintf(stderr, "\ncrash> ");
	while (fgets(line, 1000, stdin) != NULL) {
		str = lblanks(line);

		p = str;
		while(*p) {
			if (*p == '\n') {
				*p = '\0';
			}
			p++;
		}

		if (*str == 'q' && *(str+1) == '\0') {
			fprintf(stderr, "quiting.\n");
			return;
		} else if (*str == 'h' && *(str+1) == '\0') {
			crash_shell_help();
		} else if (*str == '?' && *(str+1) == '\0') {
			crash_shell_help();
		} else if (*str == 's' && *(str+1) == '\0') {
			sprintf(cmd, "sh -c '(%s) &'", crash_stack_command1);
			/* crash */
			if (no_external_cmds || !cmd_ok("crash")) {
				fprintf(stderr, "\nno_external_cmds=%d\n",
				    no_external_cmds);
				goto crash_prompt;
			}
			fprintf(stderr, "\nrunning:\n\t%s\n\n",
			    crash_stack_command1);
			system(cmd);
			usleep(1000*1000);

			sprintf(cmd, "sh -c '(%s) &'", crash_stack_command2);
			fprintf(stderr, "\nrunning:\n\t%s\n\n",
			    crash_stack_command2);
			system(cmd);
			usleep(1000*1000);
		} else {
			snprintf(qry, 1000, "qry=%s", str);
			p = process_remote_cmd(qry, 1);
			fprintf(stderr, "\n\nresult:\n%s\n", p);
			free(p);
		}
		
crash_prompt:
		fprintf(stderr, "crash> ");
	}
}

/*
 * General problem handler
 */
static void interrupted (int sig) {
	exit_sig = sig;
	if (exit_flag) {
		fprintf(stderr, "extra[%d] signal: %d\n", exit_flag, sig);
		exit_flag++;
		if (use_threads) {
			usleep2(250 * 1000);
		} else if (exit_flag <= 2) {
			return;
		}
		if (rm_flagfile) {
			unlink(rm_flagfile);
			rm_flagfile = NULL;
		}
		exit(4);
	}
	exit_flag++;
	if (sig == 0) {
		fprintf(stderr, "caught X11 error:\n");
		if (crash_debug) { crash_shell(); }
	} else if (sig == -1) {
		fprintf(stderr, "caught XIO error:\n");
	} else {
		fprintf(stderr, "caught signal: %d\n", sig);
	}
	if (sig == SIGINT) {
		shut_down = 1;
		return;
	}

	if (crash_debug) {
		crash_shell();
	}

	X_UNLOCK;

	if (icon_mode) {
		clean_icon_mode();
	}
	/* remove the shm areas with quick=1: */
	clean_shm(1);

	if (sig == -1) {
		/* not worth trying any more cleanup, X server probably gone */
		if (rm_flagfile) {
			unlink(rm_flagfile);
			rm_flagfile = NULL;
		}
		exit(3);
	}

	/* X keyboard cleanups */
	delete_added_keycodes(0);

	if (clear_mods == 1) {
		clear_modifiers(0);
	} else if (clear_mods == 2) {
		clear_keys();
	} else if (clear_mods == 3) {
		clear_keys();
		clear_locks();
	}
	if (no_autorepeat) {
		autorepeat(1, 0);
	}
	if (use_solid_bg) {
		solid_bg(1);
	}
	if (ncache || ncache0) {
		kde_no_animate(1);
	}
	stop_stunnel();

	if (crash_debug) {
		crash_shell();
	}

	if (sig) {
		if (rm_flagfile) {
			unlink(rm_flagfile);
			rm_flagfile = NULL;
		}
		exit(2);
	}
}

static void ignore_sigs(char *list) {
	char *str, *p;
	int ignore = 1;
	if (list == NULL || *list == '\0') {
		return;
	}
	str = strdup(list);
	p = strtok(str, ":,");

#define SETSIG(x, y) \
	if (strstr(p, x)) { \
		if (ignore) { \
			signal(y, SIG_IGN); \
		} else { \
			signal(y, interrupted); \
		} \
	}

#ifdef SIG_IGN
	while (p) {
		if (!strcmp(p, "ignore")) {
			ignore = 1;
		} else if (!strcmp(p, "exit")) {
			ignore = 0;
		}
		/* Take off every 'sig' ;-) */
#ifdef SIGHUP
		SETSIG("HUP", SIGHUP);
#endif
#ifdef SIGINT
		SETSIG("INT", SIGINT);
#endif
#ifdef SIGQUIT
		SETSIG("QUIT", SIGQUIT);
#endif
#ifdef SIGTRAP
		SETSIG("TRAP", SIGTRAP);
#endif
#ifdef SIGABRT
		SETSIG("ABRT", SIGABRT);
#endif
#ifdef SIGBUS
		SETSIG("BUS", SIGBUS);
#endif
#ifdef SIGFPE
		SETSIG("FPE", SIGFPE);
#endif
#ifdef SIGSEGV
		SETSIG("SEGV", SIGSEGV);
#endif
#ifdef SIGPIPE
		SETSIG("PIPE", SIGPIPE);
#endif
#ifdef SIGTERM
		SETSIG("TERM", SIGTERM);
#endif
#ifdef SIGUSR1
		SETSIG("USR1", SIGUSR1);
#endif
#ifdef SIGUSR2
		SETSIG("USR2", SIGUSR2);
#endif
#ifdef SIGCONT
		SETSIG("CONT", SIGCONT);
#endif
#ifdef SIGSTOP
		SETSIG("STOP", SIGSTOP);
#endif
#ifdef SIGTSTP
		SETSIG("TSTP", SIGTSTP);
#endif
		p = strtok(NULL, ":,");
	}
#endif	/* SIG_IGN */
	free(str);
}

/* signal handlers */
void initialize_signals(void) {
	signal(SIGHUP,  interrupted);
	signal(SIGINT,  interrupted);
	signal(SIGQUIT, interrupted);
	signal(SIGABRT, interrupted);
	signal(SIGTERM, interrupted);
	signal(SIGBUS,  interrupted);
	signal(SIGSEGV, interrupted);
	signal(SIGFPE,  interrupted);

	if (!sigpipe || *sigpipe == '\0' || !strcmp(sigpipe, "skip")) {
		;
	} else if (strstr(sigpipe, "ignore:") == sigpipe) {
		ignore_sigs(sigpipe);
	} else if (strstr(sigpipe, "exit:") == sigpipe) {
		ignore_sigs(sigpipe);
	} else if (!strcmp(sigpipe, "ignore")) {
#ifdef SIG_IGN
		signal(SIGPIPE, SIG_IGN);
#endif
	} else if (!strcmp(sigpipe, "exit")) {
		rfbLog("initialize_signals: will exit on SIGPIPE\n");
		signal(SIGPIPE, interrupted);
	}

#if NO_X11
	return;
#else
	X_LOCK;
	Xerror_def = XSetErrorHandler(Xerror);
	XIOerr_def = XSetIOErrorHandler(XIOerr);
	X_UNLOCK;
#endif	/* NO_X11 */
}

void unset_signals(void) {
	signal(SIGHUP,  SIG_DFL);
	signal(SIGINT,  SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGBUS,  SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
}

void close_exec_fds(void) {
	int fd;
#ifdef FD_CLOEXEC
	for (fd = 3; fd < 64; fd++) {
		int flags = fcntl(fd, F_GETFD);
		if (flags != -1) {
			flags |= FD_CLOEXEC;
			fcntl(fd, F_SETFD, flags);
		}
	}
#endif
}

int known_sigpipe_mode(char *s) {
/*
 * skip, ignore, exit
 */
	if (strstr(s, "ignore:") == s) {
		return 1;
	}
	if (strstr(s, "exit:") == s) {
		return 1;
	}
	if (strcmp(s, "skip") && strcmp(s, "ignore") && 
	    strcmp(s, "exit")) {
		return 0;
	} else {
		return 1;
	}
}


