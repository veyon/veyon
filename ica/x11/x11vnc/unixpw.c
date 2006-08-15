/* -- unixpw.c -- */

#ifdef __linux__
/* some conflict with _XOPEN_SOURCE */
extern int grantpt(int);
extern int unlockpt(int);
extern char *ptsname(int);
/* XXX remove need for this */
extern char *crypt(const char*, const char *);
#endif

#include "x11vnc.h"
#include "scan.h"
#include "cleanup.h"
#include "xinerama.h"
#include "connections.h"
#include "user.h"
#include "connections.h"
#include "cursor.h"
#include <rfb/default8x16.h>

#if LIBVNCSERVER_HAVE_FORK
#if LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_WAITPID
#define UNIXPW_SU
#endif
#endif

#if LIBVNCSERVER_HAVE_PWD_H && LIBVNCSERVER_HAVE_GETPWNAM
#if LIBVNCSERVER_HAVE_CRYPT || LIBVNCSERVER_HAVE_LIBCRYPT
#define UNIXPW_CRYPT
#if LIBVNCSERVER_HAVE_GETSPNAM
#include <shadow.h>
#endif
#endif
#endif

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if LIBVNCSERVER_HAVE_TERMIOS_H
#include <termios.h>
#endif
#if LIBVNCSERVER_HAVE_SYS_STROPTS_H
#include <sys/stropts.h>
#endif

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#define IS_BSD
#endif
#if (defined(__MACH__) && defined(__APPLE__))
#define IS_BSD
#endif

#ifdef NO_SSL_OR_UNIXPW
#undef UNIXPW_SU
#undef UNIXPW_CRYPT
#endif

void unixpw_screen(int init);
void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init);
void unixpw_accept(char *user);
void unixpw_deny(void);
void unixpw_msg(char *msg, int delay);
int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size);
int crypt_verify(char *user, char *pass);

static int white(void);
static int text_x(void);
static int text_y(void);
static void set_db(void);
static void unixpw_verify(char *user, char *pass);

int unixpw_in_progress = 0;
int unixpw_denied = 0;
int unixpw_in_rfbPE = 0;
int unixpw_login_viewonly = 0;
time_t unixpw_last_try_time = 0;
rfbClientPtr unixpw_client = NULL;

int keep_unixpw = 0;
char *keep_unixpw_user = NULL;
char *keep_unixpw_pass = NULL;
char *keep_unixpw_opts = NULL;

static int in_login = 0, in_passwd = 0, tries = 0;
static int char_row = 0, char_col = 0;
static int char_x = 0, char_y = 0, char_w = 8, char_h = 16;

static int db = 0;

static int white(void) {
	static unsigned long black_pix = 0, white_pix = 1, set = 0;

	RAWFB_RET(0xffffff)

	if (depth <= 8 && ! set) {
		X_LOCK;
		black_pix = BlackPixel(dpy, scr);
		white_pix = WhitePixel(dpy, scr);
		X_UNLOCK;
		set = 1;
	}
	if (depth <= 8) {
		return (int) white_pix;
	} else if (depth < 24) {
		return 0xffff;
	} else {
		return 0xffffff;
	}
}

static int text_x(void) {
	return char_x + char_col * char_w;
}

static int text_y(void) {
	return char_y + char_row * char_h;
}

static rfbScreenInfo fscreen;
static rfbScreenInfoPtr pscreen;

void unixpw_screen(int init) {
	if (unixpw_nis) {
#ifndef UNIXPW_CRYPT
	rfbLog("-unixpw_nis is not supported on this OS/machine\n");
	clean_up_exit(1);
#endif
	} else {
#ifndef UNIXPW_SU
	rfbLog("-unixpw is not supported on this OS/machine\n");
	clean_up_exit(1);
#endif
	}
	if (init) {
		int x, y;
		char log[] = "login: ";

		zero_fb(0, 0, dpy_x, dpy_y);

		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);

		x = nfix(dpy_x / 2 -  strlen(log) * char_w, dpy_x);
		y = dpy_y / 4;

		if (scaling) {
			x = (int) (x * scale_fac);
			y = (int) (y * scale_fac);
			x = nfix(x, scaled_x);
			y = nfix(y, scaled_y);
		}

		if (rotating) {
			fscreen.serverFormat.bitsPerPixel = bpp;
			fscreen.paddedWidthInBytes = rfb_bytes_per_line;
			fscreen.frameBuffer = rfb_fb;
			pscreen = &fscreen;
		} else {
			pscreen = screen;
		}

		rfbDrawString(pscreen, &default8x16Font, x, y, log, white());

		char_x = x;
		char_y = y;
		char_col = strlen(log);
		char_row = 0;

		set_warrow_cursor();
	}

	if (scaling) {
		mark_rect_as_modified(0, 0, scaled_x, scaled_y, 1);
	} else {
		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	}
}

	
#ifdef MAXPATHLEN
static char slave_str[MAXPATHLEN];
#else
static char slave_str[4096];
#endif

static int used_get_pty_ptmx = 0;

char *get_pty_ptmx(int *fd_p) {
	char *slave;
	int fd = -1, i, ndevs = 4, tmp;
	char *devs[] = { 
		"/dev/ptmx",
		"/dev/ptm/clone",
		"/dev/ptc",
		"/dev/ptmx_bsd"
	};

	*fd_p = -1;

#if LIBVNCSERVER_HAVE_GRANTPT

	for (i=0; i < ndevs; i++) {
#ifdef O_NOCTTY
		fd = open(devs[i], O_RDWR|O_NOCTTY);
#else
		fd = open(devs[i], O_RDWR);
#endif
		if (fd >= 0) {
			break;
		}
	}

	if (fd < 0) {
		rfbLogPerror("open /dev/ptmx");
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCPKT)
	tmp = 0;
	ioctl(fd, TIOCPKT, (char *) &tmp);
#endif

	if (grantpt(fd) != 0) {
		rfbLogPerror("grantpt");
		close(fd);
		return NULL;
	}
	if (unlockpt(fd) != 0) {
		rfbLogPerror("unlockpt");
		close(fd);
		return NULL;
	}

	slave = ptsname(fd);
	if (! slave)  {
		rfbLogPerror("ptsname");
		close(fd);
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCFLUSH)
	ioctl(fd, TIOCFLUSH, (char *) 0);
#endif

	strcpy(slave_str, slave);
	*fd_p = fd;
	return slave_str;

#else
	return NULL;

#endif /* GRANTPT */
}


char *get_pty_loop(int *fd_p) {
	char master_str[16];
	int fd = -1, i;
	char c;

	*fd_p = -1;

	/* for *BSD loop over /dev/ptyXY */

	for (c = 'p'; c <= 'z'; c++) {
		for (i=0; i < 16; i++) {
			sprintf(master_str, "/dev/pty%c%x", c, i);
#ifdef O_NOCTTY
			fd = open(master_str, O_RDWR|O_NOCTTY);
#else
			fd = open(master_str, O_RDWR);
#endif
			if (fd >= 0) {
				break;
			}
		}
		if (fd >= 0) {
			break;
		}
	}
	if (fd < 0) {
		return NULL;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCFLUSH)
	ioctl(fd, TIOCFLUSH, (char *) 0);
#endif

	sprintf(slave_str, "/dev/tty%c%x", c, i);
	*fd_p = fd;
	return slave_str;
}

char *get_pty(int *fd_p) {
	used_get_pty_ptmx = 0;
	if (getenv("BSD_PTY")) {
		return get_pty_loop(fd_p);
	}
#ifdef IS_BSD
	return get_pty_loop(fd_p);
#else
#if LIBVNCSERVER_HAVE_GRANTPT
	used_get_pty_ptmx = 1;
	return get_pty_ptmx(fd_p);
#else
	return get_pty_loop(fd_p);
#endif
#endif
}

void try_to_be_nobody(void) {

#if LIBVNCSERVER_HAVE_PWD_H
	struct passwd *pw;
	pw = getpwnam("nobody");

	if (pw) {
#if LIBVNCSERVER_HAVE_SETUID
		setuid(pw->pw_uid);
#endif
#if LIBVNCSERVER_HAVE_SETEUID
		seteuid(pw->pw_uid);
#endif
#if LIBVNCSERVER_HAVE_SETGID
		setgid(pw->pw_gid);
#endif
#if LIBVNCSERVER_HAVE_SETEGID
		setegid(pw->pw_gid);
#endif
	}
#endif	/* PWD_H */
}


static int slave_fd = -1, alarm_fired = 0;

static void close_alarm (int sig) {
	if (slave_fd >= 0) {
		close(slave_fd);
	}
	alarm_fired = 1;
	if (0) sig = 0;	/* compiler warning */
}

static void kill_child (pid_t pid, int fd) {
	int status;

	slave_fd = -1;
	alarm_fired = 0;
	if (fd >= 0) {
		close(fd);
	}
	kill(pid, SIGTERM);
	waitpid(pid, &status, WNOHANG); 
}

int crypt_verify(char *user, char *pass) {
#ifndef UNIXPW_CRYPT
	return 0;
#else
	struct passwd *pwd;
	char *realpw, *cr;
	int n;
	pwd = getpwnam(user);
	if (! pwd) {
		return 0;
	}

	realpw = pwd->pw_passwd;
	if (realpw == NULL || realpw[0] == '\0') {
		return 0;
	}

	if (db > 1) fprintf(stderr, "realpw='%s'\n", realpw);

	if (strlen(realpw) < 12) {
		/* e.g. "x", try getspnam(), sometimes root for inetd, etc */
#if LIBVNCSERVER_HAVE_GETSPNAM
		struct spwd *sp = getspnam(user);
		if (sp != NULL && sp->sp_pwdp != NULL) {
			if (db) fprintf(stderr, "using getspnam()\n");
			realpw = sp->sp_pwdp;
		} else {
			if (db) fprintf(stderr, "skipping getspnam()\n");
		}
#endif
	}

	n = strlen(pass);
	if (pass[n-1] == '\n') {
		pass[n-1] = '\0';
	}

	/* XXX remove need for cast */
	cr = (char *) crypt(pass, realpw);
	if (db > 1) {
		fprintf(stderr, "user='%s' pass='%s' realpw='%s' cr='%s'\n",
		    user, pass, realpw, cr ? cr : "(null)");
	}
	if (cr == NULL) {
		return 0;
	}
	if (!strcmp(cr, realpw)) {
		return 1;
	} else {
		return 0;
	}
#endif	/* UNIXPW_CRYPT */
}

int su_verify(char *user, char *pass, char *cmd, char *rbuf, int *rbuf_size) {
#ifndef UNIXPW_SU
	return 0;
#else
	int i, j, status, fd = -1, sfd, tfd, drain_size = 4096, rsize = 0;
	int slow_pw = 1;
	char *slave, *bin_true = NULL, *bin_su = NULL;
	pid_t pid, pidw;
	struct stat sbuf;
	static int first = 1;
	char instr[32], buf[10];

	if (first) {
		set_db();
		first = 0;
	}

	if (unixpw_list) {
		char *p, *q, *str = strdup(unixpw_list);
		int ok = 0;

		p = strtok(str, ",");
		while (p) {
			if ( (q = strchr(p, ':')) != NULL ) {
				*q = '\0';	/* get rid of options. */
			}
			if (!strcmp(user, p) || !strcmp("*", p)) {
				ok = 1;
				break;
			}
			p = strtok(NULL, ",");
		}
		free(str);
		if (! ok) {
			return 0;
		}
	}
	/* unixpw */
	if (no_external_cmds || !cmd_ok("unixpw")) {
		rfbLog("su_verify: cannot run external commands.\n");	
		clean_up_exit(1);
	}

#define SU_DEBUG 0
#if SU_DEBUG
	if (stat("/su", &sbuf) == 0) {
		bin_su = "/su";	/* Freesbie read-only-fs /bin/su not suid! */
#else
	if (0) {
		;
#endif
	} else if (stat("/bin/su", &sbuf) == 0) {
		bin_su = "/bin/su";
	} else if (stat("/usr/bin/su", &sbuf) == 0) {
		bin_su = "/usr/bin/su";
	}
	if (bin_su == NULL) {
		rfbLogPerror("existence /bin/su");
		return 0;
	}

	if (stat("/bin/true", &sbuf) == 0) {
		bin_true = "/bin/true";
	} if (stat("/usr/bin/true", &sbuf) == 0) {
		bin_true = "/usr/bin/true";
	}
	if (cmd != NULL && cmd[0] != '\0') {
		/* this is for ext. cmd su -c "my cmd" */
		bin_true = cmd;
	}
	if (bin_true == NULL) {
		rfbLogPerror("existence /bin/true");
		return 0;
	}

	slave = get_pty(&fd);

	if (slave == NULL) {
		rfbLogPerror("get_pty failed.");
		return 0;
	}

if (db) fprintf(stderr, "slave is: %s fd=%d\n", slave, fd);

	if (fd < 0) {
		rfbLogPerror("get_pty fd < 0");
		return 0;
	}

	fcntl(fd, F_SETFD, 1);

	pid = fork();
	if (pid < 0) {
		rfbLogPerror("fork");
		close(fd);
		return 0;
	}

	if (pid == 0) {
		/* child */

		int ttyfd;
		ttyfd = -1;	/* compiler warning */

#if LIBVNCSERVER_HAVE_SETSID
		if (setsid() == -1) {
			perror("setsid");
			exit(1);
		}
#else
		if (setpgrp() == -1) {
			perror("setpgrp");
			exit(1);
		}
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCNOTTY)
		ttyfd = open("/dev/tty", O_RDWR);
		if (ttyfd >= 0) {
			(void) ioctl(ttyfd, TIOCNOTTY, (char *) 0);
			close(ttyfd);
		}
#endif	

#endif	/* SETSID */

		close(0);
		close(1);
		close(2);

		sfd = open(slave, O_RDWR);
		if (sfd < 0) {
			exit(1);
		}

/* streams options fixups, handle cases as they are found: */
#if defined(__hpux)
#if LIBVNCSERVER_HAVE_SYS_STROPTS_H
#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(I_PUSH)
		if (used_get_pty_ptmx) {
			ioctl(sfd, I_PUSH, "ptem");
			ioctl(sfd, I_PUSH, "ldterm");
			ioctl(sfd, I_PUSH, "ttcompat");
		}
#endif
#endif
#endif

		/* n.b. sfd will be 0 since we closed 0. so dup it to 1 and 2 */
		if (fcntl(sfd, F_DUPFD, 1) == -1) {
			exit(1);
		}
		if (fcntl(sfd, F_DUPFD, 2) == -1) {
			exit(1);
		}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCSCTTY)
		ioctl(sfd, TIOCSCTTY, (char *) 0);
#endif

		if (db > 2) {
			char nam[256];
			unlink("/tmp/isatty");
			tfd = open("/tmp/isatty", O_CREAT|O_WRONLY, 0600);
			if (isatty(sfd)) {
				close(tfd);
				sprintf(nam, "stty -a < %s > /tmp/isatty 2>&1", slave);
				system(nam);
			} else {
				write(tfd, "NOTTTY\n", 7);
				close(tfd);
			}
		}

		chdir("/");

		try_to_be_nobody();
#if LIBVNCSERVER_HAVE_GETUID
		if (getuid() == 0 || geteuid() == 0) {
			exit(1);
		}
#else
		exit(1);
#endif

		set_env("LC_ALL", "C");
		set_env("LANG", "C");
		set_env("SHELL", "/bin/sh");

		/* synchronize with parent: */
		write(2, "C", 1);

		if (cmd) {
			execlp(bin_su, bin_su, "-", user, "-c",
			    bin_true, (char *) NULL);
		} else {
			execlp(bin_su, bin_su, user, "-c",
			    bin_true, (char *) NULL);
		}
		exit(1);
	}
	/* parent */

	if (db)	fprintf(stderr, "pid: %d\n", pid);

	/*
	 * set an alarm for blocking read() to close the master
	 * (presumably terminating the child. SIGTERM too...)
	 */
	slave_fd = fd;
	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(10);

	/* synchronize with child: */
	for (i=0; i<10; i++) {
		int n;
		buf[0] = '\0';
		buf[1] = '\0';
		n = read(fd, buf, 1);
		if (n < 0 && errno == EINTR) {
			continue;
		} else {
			break;
		}
	}

	if (db) {
		fprintf(stderr, "read from child: '%s'\n", buf);
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

#if LIBVNCSERVER_HAVE_SYS_IOCTL_H && defined(TIOCTRAP)
	{
		int control = 1;
		ioctl(fd, TIOCTRAP, &control);
	}
#endif
	
	/*
	 * In addition to checking exit code below, we watch for the
	 * appearance of the string "Password:".  BSD does not seem to
	 * ask for a password trying to su to yourself.  This is the
	 * setting in /etc/pam.d/su:
	 * 	auth      sufficient  pam_self.so
	 * it may be commented out without problem.
	 */
	for (i=0; i<32; i++) {
		instr[i] = '\0';
	}

	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(10);

	j = 0;
	for (i=0; i < (int) strlen("Password:"); i++) {
		char pstr[] = "password:";
		int n;	

		buf[0] = '\0';
		buf[1] = '\0';

		n = read(fd, buf, 1);
		if (n < 0 && errno == EINTR) {
			i--;
			continue;
		}

if (db) fprintf(stderr, "%s", buf);

		if (db > 3 && n == 1 && buf[0] == ':') {
			char cmd0[32];
			usleep( 100 * 1000 );
			fprintf(stderr, "\n\n");
			sprintf(cmd0, "ps wu %d", pid);
			system(cmd0);
			sprintf(cmd0, "stty -a < %s", slave);
			system(cmd0);
			fprintf(stderr, "\n\n");
		}

		if (n == 1) {
			if (isspace((unsigned char) buf[0])) {
				i--;
				continue;
			}
			instr[j++] = tolower((unsigned char)buf[0]);
		}
		if (n <= 0 || strstr(pstr, instr) != pstr) {
if (db) {
	fprintf(stderr, "\"Password:\" did not appear: '%s'" " n=%d\n", instr, n);
	if (db > 3 && n == 1 && j < 32) {
		continue;
	}
}
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			kill_child(pid, fd);
			return 0;
		}
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

	usleep(100 * 1000);
	if (slow_pw) {
		unsigned int k;
		for (k = 0; k < strlen(pass); k++) {
			write(fd, pass+k, 1); 
			usleep(100 * 1000);
		}
	} else {
		write(fd, pass, strlen(pass)); 
	}

	alarm_fired = 0;
	signal(SIGALRM, close_alarm);
	alarm(15);

	/*
	 * try to drain the output, hopefully never as much as 4096 (motd?)
	 * if we don't drain we may block at waitpid.  If we close(fd), the
	 * make cause child to die by signal.
	 */
	if (rbuf && *rbuf_size > 0) {
		/* asked to return output of command */
		drain_size = *rbuf_size;
		rsize = 0;
	}
	for (i = 0; i< drain_size; i++) {
		int n;	
		
		buf[0] = '\0';
		buf[1] = '\0';

		n = read(fd, buf, 1);
		if (n < 0 && errno == EINTR) {
			continue;
		}

if (db) fprintf(stderr, "%s", buf);

		if (n <= 0) {
			break;
		}

		if (rbuf) {
			rbuf[i] = buf[0];
			rsize++;
		}
	}
	if (rbuf) {
		char *s = rbuf;
		char *p = strdup(pass);
		int n, o = 0;
		
		n = strlen(p);
		if (p[n-1] == '\n') {
			p[n-1] = '\0';
		}
		/*
		 * usually is: Password: mypassword\r\n\r\n<output-of-command>
		 * and output will have \n -> \r\n
		 */
		if (rbuf[0] == ' ') {
			s++;
			o++;
		}
		if (strstr(s, p) == s) {
			s += strlen(p);
			o += strlen(p);
			for (n = 0; n < 4; n++) {
				if (s[0] == '\r' || s[0] == '\n') {
					s++;
					o++;
				}
			}
		}
		if (o > 0) {
			int i = 0;
			rsize -= o;
			while (o < drain_size) {
				rbuf[i++] = rbuf[o++];
			}
		}
		*rbuf_size = rsize;
		strzero(p);
	}

if (db) fprintf(stderr, "\n");

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (alarm_fired) {
		kill_child(pid, fd);
		return 0;
	}

	slave_fd = -1;
	
	pidw = waitpid(pid, &status, 0); 
	close(fd);

	if (pid != pidw) {
		return 0;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		return 1; /* this is the only return of success. */
	} else {
		return 0;
	}
#endif	/* UNIXPW_SU */
}

static void unixpw_verify(char *user, char *pass) {
	int x, y;
	char li[] = "Login incorrect";
	char log[] = "login: ";
	char *colon = NULL;
	ClientData *cd = NULL;

if (db) fprintf(stderr, "unixpw_verify: '%s' '%s'\n", user, db > 1 ? pass : "********");
	rfbLog("unixpw_verify: %s\n", user);

	colon = strchr(user, ':');
	if (colon) {
		*colon = '\0';
		rfbLog("unixpw_verify: colon: %s\n", user);
	}
	if (unixpw_client) {
		cd = (ClientData *) unixpw_client->clientData;
		if (cd) {
			char *str = (char *)malloc(strlen("UNIX:") +
			    strlen(user) + 1);
			sprintf(str, "UNIX:%s", user);	
			if (cd->username) {
				free(cd->username);
			}
			cd->username = str;
		}
	}

	if (unixpw_nis) {
		if (crypt_verify(user, pass)) {
			rfbLog("unixpw_verify: crypt_verify login for '%s'"
			    " succeeded.\n", user);
			unixpw_accept(user);
			if (keep_unixpw) {
				keep_unixpw_user = strdup(user);
				keep_unixpw_pass = strdup(pass);
				if (colon) {
					keep_unixpw_opts = strdup(colon+1);
				} else {
					keep_unixpw_opts = strdup("");
				}
			}
			if (colon) *colon = ':';
			return;
		}
		rfbLog("unixpw_verify: crypt_verify login for '%s' failed.\n",
		    user);
		usleep(3000*1000);
	} else {
		if (su_verify(user, pass, NULL, NULL, NULL)) {
			rfbLog("unixpw_verify: su_verify login for '%s'"
			    " succeeded.\n", user);
			unixpw_accept(user);
			if (keep_unixpw) {
				keep_unixpw_user = strdup(user);
				keep_unixpw_pass = strdup(pass);
				if (colon) {
					keep_unixpw_opts = strdup(colon+1);
				} else {
					keep_unixpw_opts = strdup("");
				}
			}
			if (colon) *colon = ':';
			return;
		}
		rfbLog("unixpw_verify: su_verify login for '%s' failed.\n",
		    user);
	}
	if (colon) *colon = ':';

	if (tries < 2) {
		char_row++;
		char_col = 0;

		x = text_x();
		y = text_y();
		rfbDrawString(pscreen, &default8x16Font, x, y, li, white());

		char_row += 2;

		x = text_x();
		y = text_y();
		rfbDrawString(pscreen, &default8x16Font, x, y, log, white());

		char_col = strlen(log);

		if (scaling) {
			mark_rect_as_modified(0, 0, scaled_x, scaled_y, 1);
		} else {
			mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
		}

		unixpw_last_try_time = time(NULL);
		unixpw_keystroke(0, 0, 2);
		tries++;
	} else {
		unixpw_deny();
	}
}

static void set_db(void) {
	if (getenv("DEBUG_UNIXPW")) {
		db = atoi(getenv("DEBUG_UNIXPW"));
	}
}

void unixpw_keystroke(rfbBool down, rfbKeySym keysym, int init) {
	int x, y, i, nmax = 100;
	static char user_r[100], user[100], pass[100];
	static int  u_cnt = 0, p_cnt = 0, first = 1;
	char keystr[100];
	char *str;

	if (first) {
		set_db();
		first = 0;
		for (i=0; i < nmax; i++) {
			user_r[i] = '\0';
		}
	}

	if (init) {
		in_login = 1;
		in_passwd = 0;
		unixpw_denied = 0;
		if (init == 1) {
			tries = 0;
		}

		u_cnt = 0;
		p_cnt = 0;
		for (i=0; i<nmax; i++) {
			user[i] = '\0';
			pass[i] = '\0';
		}
		if (keep_unixpw_user) {
			free(keep_unixpw_user);
			keep_unixpw_user = NULL;
		}
		if (keep_unixpw_pass) {
			free(keep_unixpw_pass);
			keep_unixpw_pass = NULL;
		}
		if (keep_unixpw_opts) {
			free(keep_unixpw_opts);
			keep_unixpw_opts = NULL;
		}
		return;
	}

	if (unixpw_denied) {
		rfbLog("unixpw_keystroke: unixpw_denied state: 0x%x\n", (int) keysym);
		return;
	}
	if (keysym <= 0) {
		rfbLog("unixpw_keystroke: bad keysym1: 0x%x\n", (int) keysym);
		return;
	}
	X_LOCK;
	str = XKeysymToString(keysym);
	X_UNLOCK;
	if (! str) {
		rfbLog("unixpw_keystroke: bad keysym2: 0x%x\n", (int) keysym);
		return;
	}
	snprintf(keystr, 100, "%s", str);

	if (db > 2) {
		fprintf(stderr, "%s / %s  0x%x %s\n", in_login ? "login":"pass ",
		    down ? "down":"up  ", keysym, keystr);
	}

	if (keysym == XK_Return || keysym == XK_Linefeed) {
		;	/* let "up" pass down below for Return case */
	} else if (! down) {
		return;
	}

	if (in_login) {
		if (keysym == XK_BackSpace || keysym == XK_Delete) {
			if (u_cnt > 0) {
				user[u_cnt-1] = '\0';
				x = text_x();
				y = text_y();
				if (scaling) {
					int x2 = x / scale_fac;
					int y2 = y / scale_fac;
					int w2 = char_w / scale_fac;
					int h2 = char_h / scale_fac;

					x2 = nfix(x2, dpy_x);
					y2 = nfix(y2, dpy_y);
					
					zero_fb(x2 - w2, y2 - h2, x2, y2);
					mark_rect_as_modified(x2 - w2,
					    y2 - h2, x2, y2, 0);
				} else {
					zero_fb(x - char_w, y - char_h, x, y);
					mark_rect_as_modified(x - char_w,
					    y - char_h, x, y, 0);
				}
				char_col--;
				u_cnt--;
			}
			return;
		}
		if (keysym == XK_Return || keysym == XK_Linefeed) {
			char pw[] = "Password: ";

			if (down) {
				/*
				 * require Up so the Return Up is not processed
				 * by the normal session after login.
				 */
				return;
			}

			in_login = 0;
			in_passwd = 1;

			char_row++;
			char_col = 0;

			x = text_x();
			y = text_y();
			rfbDrawString(pscreen, &default8x16Font, x, y, pw,
			    white());

			char_col = strlen(pw);
			if (scaling) {
				mark_rect_as_modified(0, 0, scaled_x,
				    scaled_y, 1);
			} else {
				mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
			}
			return;
		}
		if (u_cnt == 0 && keysym == XK_Up) {
			/*
			 * Allow user to hit Up arrow at beginning to
			 * regain their username plus any options.
			 */
			int i;
			for (i=0; i < nmax; i++) {
				user[u_cnt++] = user_r[i];
				if (user_r[i] == '\0') {
					break;
				}
				keystr[0] = (char) user_r[i];
				keystr[1] = '\0';
				x = text_x();
				y = text_y();
				rfbDrawString(pscreen, &default8x16Font, x, y,
				    keystr, white());
				mark_rect_as_modified(x, y-char_h, x+char_w,
				    y, scaling);
				char_col++;
				usleep(10*1000);
			}
			return;
		}
		if (keysym <= ' ' || keysym >= 0x7f) {
			return;
		}
		if (u_cnt >= nmax - 1) {
			rfbLog("unixpw_deny: username too long\n");
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			unixpw_deny();
			return;
		}

#if 0
		user[u_cnt++] = keystr[0];
#else
		user[u_cnt++] = (char) keysym;
		for (i=0; i < nmax; i++) {
			user_r[i] = user[i];
		}
		keystr[0] = (char) keysym;
#endif

		x = text_x();
		y = text_y();

if (db && db <= 2) fprintf(stderr, "u_cnt: %d %d/%d ks: 0x%x  %s\n", u_cnt, x, y, keysym, keystr);

		keystr[1] = '\0';
		rfbDrawString(pscreen, &default8x16Font, x, y, keystr, white());

		mark_rect_as_modified(x, y-char_h, x+char_w, y, scaling);
		char_col++;

	} else if (in_passwd) {
		if (keysym == XK_BackSpace || keysym == XK_Delete) {
			if (p_cnt > 0) {
				pass[p_cnt-1] = '\0';
				p_cnt--;
			}
			return;
		}
		if (keysym == XK_Return || keysym == XK_Linefeed) {
			if (down) {
				/*
				 * require Up so the Return Up is not processed
				 * by the normal session after login.
				 */
				return;
			}
			in_login = 0;
			in_passwd = 0;
			pass[p_cnt++] = '\n';
			unixpw_verify(user, pass);
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			return;
		}
		if (keysym <= ' ' || keysym >= 0x7f) {
			return;
		}
		if (p_cnt >= nmax - 2) {
			rfbLog("unixpw_deny: password too long\n");
			for (i=0; i<nmax; i++) {
				user[i] = '\0';
				pass[i] = '\0';
			}
			unixpw_deny();
			return;
		}
		pass[p_cnt++] = (char) keysym;
	} else {
		/* should not happen... clean up a bit. */
		u_cnt = 0;
		p_cnt = 0;
		for (i=0; i<nmax; i++) {
			user[i] = '\0';
			pass[i] = '\0';
		}
	}
}

static void apply_opts (char *user) {
	char *p, *q, *str, *opts = NULL, *opts_star = NULL;
	ClientData *cd = (ClientData *) unixpw_client->clientData;
	rfbClientPtr cl = unixpw_client;
	int i;

	if (! cd) {
		return;
	}
	
	if (user) {
		if (cd->unixname) {
			free(cd->unixname);
		}
		cd->unixname = strdup(user);
	}

	if (! unixpw_list) {
		return;
	}
	str = strdup(unixpw_list);

	/* apply any per-user options. */
	p = strtok(str, ",");
	while (p) {
		if ( (q = strchr(p, ':')) != NULL ) {
			*q = '\0';	/* get rid of options. */
		} else {
			p = strtok(NULL, ",");
			continue;
		}
		if (user && !strcmp(user, p)) {
			opts = strdup(q+1);
		}
		if (!strcmp("*", p)) {
			opts_star = strdup(q+1);
		}
		p = strtok(NULL, ",");
	}
	free(str);

	for (i=0; i < 2; i++) {
		char *s = (i == 0) ? opts_star : opts;
		if (s == NULL) {
			continue;
		}
		p = strtok(s, "+");
		while (p) {
			if (!strcmp(p, "viewonly")) {
				cl->viewOnly = TRUE;
				strncpy(cd->input, "-", CILEN);
			} else if (!strcmp(p, "fullaccess")) {
				cl->viewOnly = FALSE;
				strncpy(cd->input, "-", CILEN);
			} else if ((q = strstr(p, "input=")) == p) {
				q += strlen("input=");
				strncpy(cd->input, q, CILEN);
			} else if (!strcmp(p, "deny")) {
				cl->viewOnly = TRUE;
				unixpw_deny();
				break;
			}
			p = strtok(NULL, "+");
		}
		free(s);
	}
}

void unixpw_accept(char *user) {
	apply_opts(user);

	if (accept_cmd && strstr(accept_cmd, "popup") == accept_cmd) {
		if (use_dpy && strstr(use_dpy, "WAIT:") == use_dpy &&
		    dpy == NULL) {
			/* handled in main() */
			unixpw_client->onHold = TRUE;
		} else if (! accept_client(unixpw_client)) {
			unixpw_deny();
			return;
		}
	}

	if (started_as_root == 1 && users_list
	    && strstr(users_list, "unixpw=") == users_list) {
		if (getuid() && geteuid()) {
			rfbLog("unixpw_accept: unixpw= but not root\n");
			started_as_root = 2;
		} else {
			char *u = (char *)malloc(strlen(user)+1); 

			u[0] = '\0';
			if (!strcmp(users_list, "unixpw=")) {
				sprintf(u, "+%s", user);
			} else {
				char *p, *str = strdup(users_list);
				p = strtok(str + strlen("unixpw="), ",");
				while (p) {
					if (!strcmp(p, user)) {
						sprintf(u, "+%s", user);
						break;
					}
					p = strtok(NULL, ",");
				}
				free(str);
			}
			
			if (u[0] == '\0') {
				rfbLog("unixpw_accept skipping switch to user: %s\n", user);
			} else if (switch_user(u, 0)) {
				rfbLog("unixpw_accept switched to user: %s\n", user);
			} else {
				rfbLog("unixpw_accept failed to switched to user: %s\n", user);
			}
			free(u);
		}
	}

	if (unixpw_login_viewonly) {
		unixpw_client->viewOnly = TRUE;
	}
	unixpw_in_progress = 0;
	unixpw_client = NULL;
	mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
}

void unixpw_deny(void) {
	int x, y, i;
	char pd[] = "Permission denied.";

	rfbLog("unixpw_deny: %d, %d\n", unixpw_denied, unixpw_in_progress);
	if (! unixpw_denied) {
		unixpw_denied = 1;

		char_row += 2;
		char_col = 0;
		x = char_x + char_col * char_w;
		y = char_y + char_row * char_h;

		rfbDrawString(pscreen, &default8x16Font, x, y, pd, white());
		if (scaling) {
			mark_rect_as_modified(0, 0, scaled_x, scaled_y, 1);
		} else {
			mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
		}

		for (i=0; i<5; i++) {
			rfbPE(-1);
			usleep(500 * 1000);
		}
	}

	if (unixpw_client) {
		rfbCloseClient(unixpw_client);
		rfbClientConnectionGone(unixpw_client);
		rfbPE(-1);
	}

	unixpw_in_progress = 0;
	unixpw_client = NULL;
	copy_screen();
}

void unixpw_msg(char *msg, int delay) {
	int x, y, i;

	char_row += 2;
	char_col = 0;
	x = char_x + char_col * char_w;
	y = char_y + char_row * char_h;

	rfbDrawString(pscreen, &default8x16Font, x, y, msg, white());
	if (scaling) {
		mark_rect_as_modified(0, 0, scaled_x, scaled_y, 1);
	} else {
		mark_rect_as_modified(0, 0, dpy_x, dpy_y, 0);
	}

	for (i=0; i<5; i++) {
		rfbPE(-1);
		usleep(500 * 1000);
		if (i >= delay) {
			break;
		}
	}
}
