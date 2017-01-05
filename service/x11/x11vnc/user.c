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

/* -- user.c -- */

#include "x11vnc.h"
#include "solid.h"
#include "cleanup.h"
#include "scan.h"
#include "screen.h"
#include "unixpw.h"
#include "sslhelper.h"
#include "xwrappers.h"
#include "connections.h"
#include "inet.h"
#include "keyboard.h"
#include "cursor.h"
#include "remote.h"
#include "sslhelper.h"
#include "avahi.h"

void check_switched_user(void);
void lurk_loop(char *str);
int switch_user(char *user, int fb_mode);
int read_passwds(char *passfile);
void install_passwds(void);
void check_new_passwds(int force);
void progress_client(void);
int wait_for_client(int *argc, char** argv, int http);
rfbBool custom_passwd_check(rfbClientPtr cl, const char *response, int len);
char *xdmcp_insert = NULL;

static void switch_user_task_dummy(void);
static void switch_user_task_solid_bg(void);
static char *get_login_list(int with_display);
static char **user_list(char *user_str);
#ifdef WIN32
typedef int uid_t;
typedef int gid_t;
#endif
static void user2uid(char *user, uid_t *uid, gid_t *gid, char **name, char **home);
static int lurk(char **users);
static int guess_user_and_switch(char *str, int fb_mode);
static int try_user_and_display(uid_t uid, gid_t gid, char *dpystr);
static int switch_user_env(uid_t uid, gid_t gid, char *name, char *home, int fb_mode);
static void try_to_switch_users(void);


/* tasks for after we switch */
static void switch_user_task_dummy(void) {
	;	/* dummy does nothing */
}
static void switch_user_task_solid_bg(void) {
	/* we have switched users, some things to do. */
	if (use_solid_bg && client_count) {
		solid_bg(0);
	}
}

void check_switched_user(void) {
	static time_t sched_switched_user = 0;
	static int did_solid = 0;
	static int did_dummy = 0;
	int delay = 15;
	time_t now = time(NULL);

	if (unixpw_in_progress) return;

	if (started_as_root == 1 && users_list) {
		try_to_switch_users();
		if (started_as_root == 2) {
			/*
			 * schedule the switch_user_tasks() call
			 * 15 secs is for piggy desktops to start up.
			 * might not be enough for slow machines...
			 */
			sched_switched_user = now;
			did_dummy = 0;
			did_solid = 0;
			/* add other activities */
		}
	}
	if (! sched_switched_user) {
		return;
	}

	if (! did_dummy) {
		switch_user_task_dummy();
		did_dummy = 1;
	}
	if (! did_solid) {
		int doit = 0;
		char *ss = solid_str;
		if (now >= sched_switched_user + delay) {
			doit = 1;
		} else if (ss && strstr(ss, "root:") == ss) {
		    	if (now >= sched_switched_user + 3) {
				doit = 1;
			}
		} else if (strcmp("root", guess_desktop())) {
			usleep(1000 * 1000);
			doit = 1;
		}
		if (doit) {
			switch_user_task_solid_bg();
			did_solid = 1;
		}
	}

	if (did_dummy && did_solid) {
		sched_switched_user = 0;
	}
}

/* utilities for switching users */
static char *get_login_list(int with_display) {
	char *out;
#if LIBVNCSERVER_HAVE_UTMPX_H
	int i, cnt, max = 200, ut_namesize = 32;
	int dpymax = 1000, sawdpy[1000];
	struct utmpx *utx;

	/* size based on "username:999," * max */
	out = (char *) malloc(max * (ut_namesize+1+3+1) + 1);
	out[0] = '\0';

	for (i=0; i<dpymax; i++) {
		sawdpy[i] = 0;
	}

	setutxent();
	cnt = 0;
	while (1) {
		char *user, *line, *host, *id;
		char tmp[10];
		int d = -1;
		utx = getutxent();
		if (! utx) {
			break;
		}
		if (utx->ut_type != USER_PROCESS) {
			continue;
		}
		user = lblanks(utx->ut_user);
		if (*user == '\0') {
			continue;
		}
		if (strchr(user, ',')) {
			continue;	/* unlikely, but comma is our sep. */
		}

		line = lblanks(utx->ut_line);
		host = lblanks(utx->ut_host);
		id   = lblanks(utx->ut_id);

		if (with_display) {
			if (0 && line[0] != ':' && strcmp(line, "dtlocal")) {
				/* XXX useful? */
				continue;
			}

			if (line[0] == ':') {
				if (sscanf(line, ":%d", &d) != 1)  {
					d = -1;
				}
			}
			if (d < 0 && host[0] == ':') {
				if (sscanf(host, ":%d", &d) != 1)  {
					d = -1;
				}
			}
			if (d < 0 && id[0] == ':') {
				if (sscanf(id, ":%d", &d) != 1)  {
					d = -1;
				}
			}

			if (d < 0 || d >= dpymax || sawdpy[d]) {
				continue;
			}
			sawdpy[d] = 1;
			sprintf(tmp, ":%d", d);
		} else {
			/* try to eliminate repeats */
			int repeat = 0;
			char *q;

			q = out;
			while ((q = strstr(q, user)) != NULL) {
				char *p = q + strlen(user) + strlen(":DPY");
				if (q == out || *(q-1) == ',') {
					/* bounded on left. */
					if (*p == ',' || *p == '\0') {
						/* bounded on right. */
						repeat = 1;
						break;
					}
				}
				q = p;
			}
			if (repeat) {
				continue;
			}
			sprintf(tmp, ":DPY");
		}

		if (*out) {
			strcat(out, ",");
		}
		strcat(out, user);
		strcat(out, tmp);

		cnt++;
		if (cnt >= max) {
			break;
		}
	}
	endutxent();
#else
	out = strdup("");
#endif
	return out;
}

static char **user_list(char *user_str) {
	int n, i;
	char *p, **list;
	
	p = user_str;
	n = 1;
	while (*p++) {
		if (*p == ',') {
			n++;
		}
	}
	list = (char **) calloc((n+1)*sizeof(char *), 1);

	p = strtok(user_str, ",");
	i = 0;
	while (p) {
		list[i++] = strdup(p);
		p = strtok(NULL, ",");
	}
	list[i] = NULL;
	return list;
}

static void user2uid(char *user, uid_t *uid, gid_t *gid, char **name, char **home) {
	int numerical = 1, gotgroup = 0;
	char *q;

	*uid = (uid_t) -1;
	*name = NULL;
	*home = NULL;

	q = user;
	while (*q) {
		if (! isdigit((unsigned char) (*q++))) {
			numerical = 0;
			break;
		}
	}

	if (user2group != NULL) {
		static int *did = NULL;
		int i;

		if (did == NULL) {
			int n = 0;
			i = 0;
			while (user2group[i] != NULL) {
				n++;
				i++;
			}
			did = (int *) malloc((n+1) * sizeof(int)); 
			i = 0;
			for (i=0; i<n; i++) {
				did[i] = 0;
			}
		}
		i = 0;
		while (user2group[i] != NULL) {
			if (strstr(user2group[i], user) == user2group[i]) {
				char *w = user2group[i] + strlen(user);
				if (*w == '.') {
#if (SMALL_FOOTPRINT > 2)
					gotgroup = 0;
#else
					struct group* gr = getgrnam(++w);
					if (! gr) {
						rfbLog("Invalid group: %s\n", w);
						clean_up_exit(1);
					}
					*gid = gr->gr_gid;
					if (! did[i]) {
						rfbLog("user2uid: using group %s (%d) for %s\n",
						    w, (int) *gid, user);
						did[i] = 1;
					}
					gotgroup = 1;
#endif
				}
			}
			i++;
		}
	}

	if (numerical) {
		int u = atoi(user);

		if (u < 0) {
			return;
		}
		*uid = (uid_t) u;
	}

#if LIBVNCSERVER_HAVE_PWD_H
	if (1) {
		struct passwd *pw;
		if (numerical) {
			pw = getpwuid(*uid);
		} else {
			pw = getpwnam(user);
		}
		if (pw) {
			*uid  = pw->pw_uid;
			if (! gotgroup) {
				*gid  = pw->pw_gid;
			}
			*name = pw->pw_name;	/* n.b. use immediately */
			*home = pw->pw_dir;
		}
	}
#endif
}


static int lurk(char **users) {
	uid_t uid;
	gid_t gid;
	int success = 0, dmin = -1, dmax = -1;
	char *p, *logins, **u;
	char **list;
	int lind;

	if ((u = users) != NULL && *u != NULL && *(*u) == ':') {
		int len;
		char *tmp;

		/* extract min and max display numbers */
		tmp = *u;
		if (strchr(tmp, '-')) {
			if (sscanf(tmp, ":%d-%d", &dmin, &dmax) != 2) {
				dmin = -1;
				dmax = -1;
			}
		}
		if (dmin < 0) {
			if (sscanf(tmp, ":%d", &dmin) != 1) {
				dmin = -1;
				dmax = -1;
			} else {
				dmax = dmin;
			}
		}
		if ((dmin < 0 || dmax < 0) || dmin > dmax || dmax > 10000) {
			dmin = -1;
			dmax = -1;
		}

		/* get user logins regardless of having a display: */
		logins = get_login_list(0);

		/*
		 * now we append the list in users (might as well try
		 * them) this will probably allow weird ways of starting
		 * xservers to work.
		 */
		len = strlen(logins);
		u++;
		while (*u != NULL) {
			len += strlen(*u) + strlen(":DPY,");
			u++;
		}
		tmp = (char *) malloc(len+1);
		strcpy(tmp, logins);

		/* now concatenate them: */
		u = users+1;
		while (*u != NULL) {
			char *q, chk[100];
			snprintf(chk, 100, "%s:DPY", *u);
			q = strstr(tmp, chk);
			if (q) {
				char *p = q + strlen(chk);
				
				if (q == tmp || *(q-1) == ',') {
					/* bounded on left. */
					if (*p == ',' || *p == '\0') {
						/* bounded on right. */
						u++;
						continue;
					}
				}
			}
			
			if (*tmp) {
				strcat(tmp, ",");
			}
			strcat(tmp, *u);
			strcat(tmp, ":DPY");
			u++;
		}
		free(logins);
		logins = tmp;
		
	} else {
		logins = get_login_list(1);
	}

	list = (char **) calloc((strlen(logins)+2)*sizeof(char *), 1);
	lind = 0;
	p = strtok(logins, ",");
	while (p) {
		list[lind++] = strdup(p);
		p = strtok(NULL, ",");
	}
	free(logins);

	lind = 0;
	while (list[lind] != NULL) {
		char *user, *name, *home, dpystr[10];
		char *q, *t;
		int ok = 1, dn;

		p = list[lind++];
		
		t = strdup(p);	/* bob:0 */
		q = strchr(t, ':'); 
		if (! q) {
			free(t);
			break;
		}
		*q = '\0';
		user = t;
		snprintf(dpystr, 10, ":%s", q+1);

		if (users) {
			u = users;
			ok = 0;
			while (*u != NULL) {
				if (*(*u) == ':') {
					u++;
					continue;
				}
				if (!strcmp(user, *u++)) {
					ok = 1;
					break;
				}
			}
		}

		user2uid(user, &uid, &gid, &name, &home);
		free(t);

		if (! uid || ! gid) {
			ok = 0;
		}

		if (! ok) {
			continue;
		}
		
		for (dn = dmin; dn <= dmax; dn++) {
			if (dn >= 0) {
				sprintf(dpystr, ":%d", dn);
			}
			if (try_user_and_display(uid, gid, dpystr)) {
				if (switch_user_env(uid, gid, name, home, 0)) {
					rfbLog("lurk: now user: %s @ %s\n",
					    name, dpystr);
					started_as_root = 2;
					success = 1;
				}
				set_env("DISPLAY", dpystr);
				break;
			}
		}
		if (success) {
			 break;
		}
	}

	lind = 0;
	while (list[lind] != NULL) {
		free(list[lind]);
		lind++;
	}

	return success;
}

void lurk_loop(char *str) {
	char *tstr = NULL, **users = NULL;

	if (strstr(str, "lurk=") != str) {
		exit(1);
	}
	rfbLog("lurking for logins using: '%s'\n", str);
	if (strlen(str) > strlen("lurk=")) {
		char *q = strchr(str, '=');
		tstr = strdup(q+1);
		users = user_list(tstr);
	}

	while (1) {
		if (lurk(users)) {
			break;
		}
		sleep(3);
	}
	if (tstr) {
		free(tstr);
	}
	if (users) {
		free(users);
	}
}

static int guess_user_and_switch(char *str, int fb_mode) {
	char *dstr, *d;
	char *p, *tstr = NULL, *allowed = NULL, *logins, **users = NULL;
	int dpy1, ret = 0;
	char **list;
	int lind;

	RAWFB_RET(0)

	d = DisplayString(dpy);
	/* pick out ":N" */
	dstr = strchr(d, ':');
	if (! dstr) {
		return 0;
	}
	if (sscanf(dstr, ":%d", &dpy1) != 1) {
		return 0;
	}
	if (dpy1 < 0) {
		return 0;
	}

	if (strstr(str, "guess=") == str && strlen(str) > strlen("guess=")) {
		allowed = strchr(str, '=');
		allowed++;

		tstr = strdup(allowed);
		users = user_list(tstr);
	}

	/* loop over the utmpx entries looking for this display */
	logins = get_login_list(1);

	list = (char **) calloc((strlen(logins)+2)*sizeof(char *), 1);
	lind = 0;
	p = strtok(logins, ",");
	while (p) {
		list[lind++] = strdup(p);
		p = strtok(NULL, ",");
	}

	lind = 0;
	while (list[lind] != NULL) {
		char *user, *q, *t;
		int dpy2, ok = 1;

		p = list[lind++];

		t = strdup(p);
		q = strchr(t, ':'); 
		if (! q) {
			free(t);
			break;
		}
		*q = '\0';
		user = t;
		dpy2 = atoi(q+1);

		if (users) {
			char **u = users;
			ok = 0;
			while (*u != NULL) {
				if (!strcmp(user, *u++)) {
					ok = 1;
					break;
				}
			}
		}
		if (dpy1 != dpy2) {
			ok = 0;
		}

		if (! ok) {
			free(t);
			continue;
		}

		if (switch_user(user, fb_mode)) {
			rfbLog("switched to guessed user: %s\n", user);
			free(t);
			ret = 1;
			break;
		}
	}
	if (tstr) {
		free(tstr);
	}
	if (users) {
		free(users);
	}
	if (logins) {
		free(logins);
	}
	return ret;
}

static int try_user_and_display(uid_t uid, gid_t gid, char *dpystr) {
	/* NO strtoks */
#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_PWD_H
	pid_t pid, pidw;
	char *home, *name;
	int st;
	struct passwd *pw;
	
	pw = getpwuid(uid);
	if (pw) {
		name = pw->pw_name;
		home = pw->pw_dir;
	} else {
		return 0;
	}

	/* 
	 * We fork here and try to open the display again as the
	 * new user.  Unreadable XAUTHORITY could be a problem...
	 * This is not really needed since we have DISPLAY open but:
	 * 1) is a good indicator this user owns the session and  2)
	 * some activities do spawn new X apps, e.g.  xmessage(1), etc.
	 */
	if ((pid = fork()) > 0) {
		;
	} else if (pid == -1) {
		fprintf(stderr, "could not fork\n");
		rfbLogPerror("fork");
		return 0;
	} else {
		/* child */
		Display *dpy2 = NULL;
		int rc;

		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		rc = switch_user_env(uid, gid, name, home, 0); 
		if (! rc) {
			exit(1);
		}

		fclose(stderr);
		dpy2 = XOpenDisplay_wr(dpystr);
		if (dpy2) {
			XCloseDisplay_wr(dpy2);
			exit(0);	/* success */
		} else {
			exit(2);	/* fail */
		}
	}

	/* see what the child says: */
	pidw = waitpid(pid, &st, 0);
	if (pidw == pid && WIFEXITED(st) && WEXITSTATUS(st) == 0) {
		return 1;
	}
#endif	/* LIBVNCSERVER_HAVE_FORK ... */
	return 0;
}

int switch_user(char *user, int fb_mode) {
	/* NO strtoks */
	int doit = 0;
	uid_t uid = 0;
	gid_t gid = 0;
	char *name, *home;

	if (*user == '+') {
		doit = 1;
		user++;
	}

	ssl_helper_pid(0, -2);	/* waitall */

	if (strstr(user, "guess=") == user) {
		return guess_user_and_switch(user, fb_mode);
	}

	user2uid(user, &uid, &gid, &name, &home);

	if (uid == (uid_t) -1 || uid == 0) {
		return 0;
	}
	if (gid == 0) {
		return 0;
	}

	if (! doit && dpy) {
		/* see if this display works: */
		char *dstr = DisplayString(dpy);
		doit = try_user_and_display(uid, gid, dstr);
	}

	if (doit) {
		int rc = switch_user_env(uid, gid, name, home, fb_mode);
		if (rc) {
			started_as_root = 2;
		}
		return rc;
	} else {
		return 0;
	}
}

static int switch_user_env(uid_t uid, gid_t gid, char *name, char *home, int fb_mode) {
	/* NO strtoks */
	char *xauth;
	int reset_fb = 0;
	int grp_ok = 0;

#if !LIBVNCSERVER_HAVE_SETUID
	return 0;
#else
	/*
	 * OK, tricky here, we need to free the shm... otherwise
	 * we won't be able to delete it as the other user...
	 */
	if (fb_mode == 1 && (using_shm && ! xform24to32)) {
		reset_fb = 1;
		clean_shm(0);
		free_tiles();
	}
#if LIBVNCSERVER_HAVE_INITGROUPS
#if LIBVNCSERVER_HAVE_PWD_H
	if (getpwuid(uid) != NULL && getenv("X11VNC_SINGLE_GROUP") == NULL) {
		struct passwd *p = getpwuid(uid);
		/* another possibility is p->pw_gid instead of gid */
		if (setgid(gid) == 0 && initgroups(p->pw_name, gid) == 0)  {
			grp_ok = 1;
		} else {
			rfbLogPerror("initgroups");
		}
		endgrent();
	}
#endif
#endif
	if (! grp_ok) {
		if (setgid(gid) == 0) {
			grp_ok = 1;
		}
	}
	if (! grp_ok) {
		if (reset_fb) {
			/* 2 means we did clean_shm and free_tiles */
			do_new_fb(2);
		}
		return 0;
	}

	if (setuid(uid) != 0) {
		if (reset_fb) {
			/* 2 means we did clean_shm and free_tiles */
			do_new_fb(2);
		}
		return 0;
	}
#endif
	if (reset_fb) {
		do_new_fb(2);
	}

	xauth = getenv("XAUTHORITY");
	if (xauth && access(xauth, R_OK) != 0) {
		*(xauth-2) = '_';	/* yow */
	}
	
	set_env("USER", name);
	set_env("LOGNAME", name);
	set_env("HOME", home);
	return 1;
}

static void try_to_switch_users(void) {
	static time_t last_try = 0;
	time_t now = time(NULL);
	char *users, *p;

	if (getuid() && geteuid()) {
		rfbLog("try_to_switch_users: not root\n");
		started_as_root = 2;
		return;
	}
	if (!last_try) {
		last_try = now;
	} else if (now <= last_try + 2) {
		/* try every 3 secs or so */
		return;
	}
	last_try = now;

	users = strdup(users_list);

	if (strstr(users, "guess=") == users) {
		if (switch_user(users, 1)) {
			started_as_root = 2;
		}
		free(users);
		return;
	}

	p = strtok(users, ",");
	while (p) {
		if (switch_user(p, 1)) {
			started_as_root = 2;
			rfbLog("try_to_switch_users: now %s\n", p);
			break;
		}
		p = strtok(NULL, ",");
	}
	free(users);
}

int read_passwds(char *passfile) {
	char line[1024];
	char *filename;
	char **old_passwd_list = passwd_list;
	int linecount = 0, i, remove = 0, read_mode = 0, begin_vo = -1;
	struct stat sbuf;
	static int max = -1;
	FILE *in = NULL;
	static time_t last_read = 0;
	static int read_cnt = 0;
	int db_passwd = 0;

	if (max < 0) {
		max = 1000;
		if (getenv("X11VNC_MAX_PASSWDS")) {
			max = atoi(getenv("X11VNC_MAX_PASSWDS"));
		}
	}

	filename = passfile;
	if (strstr(filename, "rm:") == filename) {
		filename += strlen("rm:");
		remove = 1;
	} else if (strstr(filename, "read:") == filename) {
		filename += strlen("read:");
		read_mode = 1;
		if (stat(filename, &sbuf) == 0) {
			if (sbuf.st_mtime <= last_read) {
				return 1;
			}
			last_read = sbuf.st_mtime;
		}
	} else if (strstr(filename, "cmd:") == filename) {
		int rc;

		filename += strlen("cmd:");
		read_mode = 1;
		in = tmpfile();
		if (in == NULL) {
			rfbLog("run_user_command tmpfile() failed: %s\n",
			    filename);
			clean_up_exit(1);
		}
		rc = run_user_command(filename, latest_client, "read_passwds",
		    NULL, 0, in);
		if (rc != 0) {
			rfbLog("run_user_command command failed: %s\n",
			    filename);
			clean_up_exit(1);
		}
		rewind(in);
	} else if (strstr(filename, "custom:") == filename) {
		return 1;
	}

	if (in == NULL && stat(filename, &sbuf) == 0) {
		/* (poor...) upper bound to number of lines */
		max = (int) sbuf.st_size;
		last_read = sbuf.st_mtime;
	}

	/* create 1 more than max to have it be the ending NULL */
	passwd_list = (char **) malloc( (max+1) * (sizeof(char *)) );
	for (i=0; i<max+1; i++) {
		passwd_list[i] = NULL;
	}
	
	if (in == NULL) {
		in = fopen(filename, "r");
	}
	if (in == NULL) {
		rfbLog("cannot open passwdfile: %s\n", passfile);
		rfbLogPerror("fopen");
		if (remove) {
			unlink(filename);
		}
		clean_up_exit(1);
	}

	if (getenv("DEBUG_PASSWDFILE") != NULL) {
		db_passwd = 1;
	}

	while (fgets(line, 1024, in) != NULL) {
		char *p;
		int blank = 1;
		int len = strlen(line); 

		if (db_passwd) {
			fprintf(stderr, "read_passwds: raw line: %s\n", line);
		}

		if (len == 0) {
			continue;
		} else if (line[len-1] == '\n') {
			line[len-1] = '\0';
		}
		if (line[0] == '\0') {
			continue;
		}
		if (strstr(line, "__SKIP__") != NULL) {
			continue;
		}
		if (strstr(line, "__COMM__") == line) {
			continue;
		}
		if (!strcmp(line, "__BEGIN_VIEWONLY__")) {
			if (begin_vo < 0) {
				begin_vo = linecount;
			}
			continue;
		}
		if (line[0] == '#') {
			/* commented out, cannot have password beginning with # */
			continue;
		}
		p = line;
		while (*p != '\0') {
			if (! isspace((unsigned char) (*p))) {
				blank = 0;
				break;
			}
			p++;
		}
		if (blank) {
			continue;
		}

		passwd_list[linecount++] = strdup(line);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: keepline: %s\n", line);
			fprintf(stderr, "read_passwds: begin_vo: %d\n", begin_vo);
		}

		if (linecount >= max) {
			rfbLog("read_passwds: hit max passwd: %d\n", max);
			break;
		}
	}
	fclose(in);

	for (i=0; i<1024; i++) {
		line[i] = '\0';
	}

	if (remove) {
		unlink(filename);
	}

	if (! linecount) {
		rfbLog("cannot read a valid line from passwdfile: %s\n",
		    passfile);
		if (read_cnt == 0) {
			clean_up_exit(1);
		} else {
			return 0;
		}
	}
	read_cnt++;

	for (i=0; i<linecount; i++) {
		char *q, *p = passwd_list[i];
		if (!strcmp(p, "__EMPTY__")) {
			*p = '\0';
		} else if ((q = strstr(p, "__COMM__")) != NULL) {
			*q = '\0';
		}
		passwd_list[i] = strdup(p);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: trimline: %s\n", p);
		}
		strzero(p);
	}

	begin_viewonly = begin_vo;
	if (read_mode && read_cnt > 1) {
		if (viewonly_passwd) {
			free(viewonly_passwd);
			viewonly_passwd = NULL;
		}
	}

	if (begin_viewonly < 0 && linecount == 2) {
		/* for compatibility with previous 2-line usage: */
		viewonly_passwd = strdup(passwd_list[1]);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: linecount is 2.\n");
		}
		if (screen) {
			char **apd = (char **) screen->authPasswdData;
			if (apd) {
				if (apd[0] != NULL) {
					strzero(apd[0]);
				}
				apd[0] = strdup(passwd_list[0]);
			}
		}
		begin_viewonly = 1;
	}

	if (old_passwd_list != NULL) {
		char *p;
		i = 0;
		while (old_passwd_list[i] != NULL) {
			p = old_passwd_list[i];
			strzero(p);
			free(old_passwd_list[i]);
			i++;
		}
		free(old_passwd_list);
	}
	return 1;
}

void install_passwds(void) {
	if (viewonly_passwd) {
		/* append the view only passwd after the normal passwd */
		char **passwds_new = (char **) malloc(3*sizeof(char *));
		char **passwds_old = (char **) screen->authPasswdData;
		passwds_new[0] = passwds_old[0];
		passwds_new[1] = viewonly_passwd;
		passwds_new[2] = NULL;
		/* mutex */
		screen->authPasswdData = (void*) passwds_new;
	} else if (passwd_list) {
		int i = 0;
		while(passwd_list[i] != NULL) {
			i++;
		}
		if (begin_viewonly < 0) {
			begin_viewonly = i+1;
		}
		/* mutex */
		screen->authPasswdData = (void*) passwd_list;
		screen->authPasswdFirstViewOnly = begin_viewonly;
	}
}

void check_new_passwds(int force) {
	static time_t last_check = 0;
	time_t now;

	if (! passwdfile) {
		return;
	}
	if (strstr(passwdfile, "read:") != passwdfile) {
		return;
	}
	if (unixpw_in_progress) return;

	if (force) {
		last_check = 0;
	}

	now = time(NULL);
	if (now > last_check + 1) {
		if (read_passwds(passwdfile)) {
			install_passwds();
		}
		last_check = now;
	}
}

rfbBool custom_passwd_check(rfbClientPtr cl, const char *response, int len) {
	char *input, *cmd;
	char num[16];
	int j, i, n, rc;

	rfbLog("custom_passwd_check: len=%d\n", len);

	if (!passwdfile || strstr(passwdfile, "custom:") != passwdfile) {
		return FALSE;
	}
	cmd = passwdfile + strlen("custom:");

	sprintf(num, "%d\n", len);

	input = (char *) malloc(2 * len + 16 + 1);
	
	input[0] = '\0';
	strcat(input, num);
	n = strlen(num);

	j = n;
	for (i=0; i < len; i++) {
		input[j] = cl->authChallenge[i];
		j++;
	}
	for (i=0; i < len; i++) {
		input[j] = response[i];
		j++;
	}
	rc = run_user_command(cmd, cl, "custom_passwd", input, n+2*len, NULL);
	free(input);
	if (rc == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void handle_one_http_request(void) {
	rfbLog("handle_one_http_request: begin.\n");
	if (inetd || screen->httpPort == 0) {
		int port = find_free_port(5800, 5860);
		if (port) {
			/* mutex */
			screen->httpPort = port;
		} else {
			rfbLog("handle_one_http_request: no http port.\n");
			clean_up_exit(1);
		}
	}
	screen->autoPort = FALSE;
	screen->port = 0;

	http_connections(1);

	rfbInitServer(screen);

	if (!inetd) {
		/* XXX ipv6 */
		int conn = 0;
		while (1) {
			if (0) fprintf(stderr, "%d %d %d  %d\n", conn, screen->listenSock, screen->httpSock, screen->httpListenSock);
			usleep(10 * 1000);
			rfbHttpCheckFds(screen);
			if (conn) {
				if (screen->httpSock < 0) {
					break;
				}
			} else {
				if (screen->httpSock >= 0) {
					conn = 1;
				}
			}
			if (!screen->httpDir) {
				break;
			}
			if (screen->httpListenSock < 0) {
				break;
			}
		}
		rfbLog("handle_one_http_request: finished.\n");
		return;
	} else {
		/* inetd case: */
#if LIBVNCSERVER_HAVE_FORK
		pid_t pid;
		int s_in = screen->inetdSock;
		if (s_in < 0) {
			rfbLog("handle_one_http_request: inetdSock not set up.\n");
			clean_up_exit(1);
		}
		pid = fork();
		if (pid < 0) {
			rfbLog("handle_one_http_request: could not fork.\n");
			clean_up_exit(1);
		} else if (pid > 0) {
			int status;
			pid_t pidw;
			while (1) {
				rfbHttpCheckFds(screen);
				pidw = waitpid(pid, &status, WNOHANG); 
				if (pidw == pid && WIFEXITED(status)) {
					break;
				} else if (pidw < 0) {
					break;
				}
			}
			rfbLog("handle_one_http_request: finished.\n");
			return;
		} else {
			int sock = connect_tcp("127.0.0.1", screen->httpPort);
			if (sock < 0) {
				exit(1);
			}
			raw_xfer(sock, s_in, s_in);
			exit(0);
		}
#else
		rfbLog("handle_one_http_request: fork not supported.\n");
		clean_up_exit(1);
#endif
	}
}

void user_supplied_opts(char *opts) {
	char *p, *str;
	char *allow[] = {
		"skip-display", "skip-auth", "skip-shared",
		"scale", "scale_cursor", "sc", "solid", "so", "id",
		"clear_mods", "cm", "clear_keys", "ck", "repeat",
		"clear_all", "ca",
		"speeds", "sp", "readtimeout", "rd",
		"rotate", "ro",
		"geometry", "geom", "ge",
		"noncache", "nc",
		"nodisplay", "nd",
		"viewonly", "vo",
		"tag",
		NULL
	};

	if (getenv("X11VNC_NO_UNIXPW_OPTS")) {
		return;
	}

	str = strdup(opts);

	p = strtok(str, ",");
	while (p) {
		char *q;
		int i, n, m, ok = 0;

		i = 0;
		while (allow[i] != NULL) {
			if (strstr(allow[i], "skip-")) {
				i++;
				continue;
			}
			if (strstr(p, allow[i]) == p) 	{
				ok = 1;
				break;
			}
			i++;
		}

		if (! ok && strpbrk(p, "0123456789") == p &&
		    sscanf(p, "%d/%d", &n, &m) == 2) {
			if (scale_str) free(scale_str);
			scale_str = strdup(p);
		} else if (ok) {
			if (0 && strstr(p, "display=") == p) {
				if (use_dpy) free(use_dpy);
				use_dpy = strdup(p + strlen("display="));
			} else if (0 && strstr(p, "auth=") == p) {
				if (auth_file) free(auth_file);
				auth_file = strdup(p + strlen("auth="));
			} else if (0 && !strcmp(p, "shared")) {
				shared = 1;
			} else if (strstr(p, "scale=") == p) {
				if (scale_str) free(scale_str);
				scale_str = strdup(p + strlen("scale="));
			} else if (strstr(p, "scale_cursor=") == p ||
			    strstr(p, "sc=") == p) {
				if (scale_cursor_str) free(scale_cursor_str);
				q = strchr(p, '=') + 1;
				scale_cursor_str = strdup(q);
			} else if (strstr(p, "rotate=") == p ||
			    strstr(p, "ro=") == p) {
				if (rotating_str) free(rotating_str);
				q = strchr(p, '=') + 1;
				rotating_str = strdup(q);
			} else if (!strcmp(p, "solid") || !strcmp(p, "so")) {
				use_solid_bg = 1;
				if (!solid_str) {
					solid_str = strdup(solid_default);
				}
			} else if (!strcmp(p, "viewonly") || !strcmp(p, "vo")) {
				view_only = 1;
			} else if (strstr(p, "solid=") == p ||
			    strstr(p, "so=") == p) {
				use_solid_bg = 1;
				if (solid_str) free(solid_str);
				q = strchr(p, '=') + 1;
				if (!strcmp(q, "R")) {
					solid_str = strdup("root:");
				} else {
					solid_str = strdup(q);
				}
			} else if (strstr(p, "id=") == p) {
				unsigned long win;
				q = p + strlen("id=");
				if (strcmp(q, "pick")) {
					if (scan_hexdec(q, &win)) {
						subwin = win;
					}
				}
			} else if (!strcmp(p, "clear_mods") ||
			    !strcmp(p, "cm")) {
				clear_mods = 1;
			} else if (!strcmp(p, "clear_keys") ||
			    !strcmp(p, "ck")) {
				clear_mods = 2;
			} else if (!strcmp(p, "clear_all") ||
			    !strcmp(p, "ca")) {
				clear_mods = 3;
			} else if (!strcmp(p, "noncache") ||
			    !strcmp(p, "nc")) {
				ncache  = 0;
				ncache0 = 0;
			} else if (strstr(p, "nc=") == p) {
				int n2 = atoi(p + strlen("nc="));
				if (nabs(n2) < nabs(ncache)) {
					if (ncache < 0) {
						ncache = -nabs(n2);
					} else {
						ncache = nabs(n2);
					}
				}
			} else if (!strcmp(p, "repeat")) {
				no_autorepeat = 0;
			} else if (strstr(p, "speeds=") == p ||
			    strstr(p, "sp=") == p) {
				if (speeds_str) free(speeds_str);
				q = strchr(p, '=') + 1;
				speeds_str = strdup(q);
				q = speeds_str;
				while (*q != '\0') {
					if (*q == '-') {
						*q = ',';
					}
					q++;
				}
			} else if (strstr(p, "readtimeout=") == p ||
			    strstr(p, "rd=") == p) {
				q = strchr(p, '=') + 1;
				rfbMaxClientWait = atoi(q) * 1000;
			}
		} else {
			rfbLog("skipping option: %s\n", p);
		}
		p = strtok(NULL, ",");
	}
	free(str);
}

static void vnc_redirect_timeout (int sig) {
	write(2, "timeout: no clients connected.\n", 31);
	if (sig) {};
	exit(0);
}

static void do_chvt(int vt) {
	char chvt[100];
	sprintf(chvt, "chvt %d >/dev/null 2>/dev/null &", vt);
	rfbLog("running: %s\n", chvt);
	system(chvt);
	sleep(2);
}

static void setup_fake_fb(XImage* fb_image, int w, int h, int b) {
	if (fake_fb) {
		free(fake_fb);
	}
	fake_fb = (char *) calloc(w*h*b/8, 1);

	fb_image->data = fake_fb;
	fb_image->format = ZPixmap;
	fb_image->width  = w;
	fb_image->height = h;
	fb_image->bits_per_pixel = b;
	fb_image->bytes_per_line = w*b/8;
	fb_image->bitmap_unit = -1;
	if (b >= 24) {
		fb_image->depth = 24;
		fb_image->red_mask   = 0xff0000;
		fb_image->green_mask = 0x00ff00;
		fb_image->blue_mask  = 0x0000ff;
	} else if (b >= 16) {
		fb_image->depth = 16;
		fb_image->red_mask   = 0x003f;
		fb_image->green_mask = 0x07c0;
		fb_image->blue_mask  = 0xf800;
	} else if (b >= 2) {
		fb_image->depth = 8;
		fb_image->red_mask   = 0x07;
		fb_image->green_mask = 0x38;
		fb_image->blue_mask  = 0xc0;
	} else {
		fb_image->depth = 1;
		fb_image->red_mask   = 0x1;
		fb_image->green_mask = 0x1;
		fb_image->blue_mask  = 0x1;
	}

	depth = fb_image->depth;

	dpy_x = wdpy_x = w;
	dpy_y = wdpy_y = h;
	off_x = 0;
	off_y = 0;
}

void do_announce_http(void);
void do_mention_java_urls(void);

static void setup_service(void) {
	if (remote_direct) {
		return;
	}
	if (!inetd) {
		do_mention_java_urls();
		do_announce_http();
		if (!use_openssl) {
			announce(screen->port, use_openssl, NULL);
			fprintf(stdout, "PORT=%d\n", screen->port);
		} else {
			fprintf(stdout, "PORT=%d\n", screen->port);
			if (stunnel_port) {
				fprintf(stdout, "SSLPORT=%d\n", stunnel_port);
			} else if (use_openssl) {
				fprintf(stdout, "SSLPORT=%d\n", screen->port);
			}
		}
		fflush(stdout);
	} else if (!use_openssl && avahi) {
		char *name = rfb_desktop_name;
		if (!name) {
			name = use_dpy;
		}
		avahi_initialise();
		avahi_advertise(name, this_host(), screen->port);
	}
}

static void check_waitbg(void) {
	if (getenv("WAITBG")) {
#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SETSID
		int p, n;
		if ((p = fork()) > 0)  {
			exit(0);
		} else if (p == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "could not fork\n");
			perror("fork");
			clean_up_exit(1);
		}
		if (setsid() == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "setsid failed\n");
			perror("setsid");
			clean_up_exit(1);
		}
		/* adjust our stdio */
		n = open("/dev/null", O_RDONLY);
		dup2(n, 0);
		dup2(n, 1);
		if (! logfile) {
			dup2(n, 2);
		}
		if (n > 2) {
			close(n);
		}
#else
		clean_up_exit(1);
#endif
	}
}

static void setup_client_connect(int *did_client_connect) {
	if (client_connect != NULL) {
		char *remainder = NULL;
		if (inetd) {
			rfbLog("wait_for_client: -connect disallowed in inetd mode: %s\n",
			    client_connect);
		} else if (screen && screen->clientHead) {
			rfbLog("wait_for_client: -connect disallowed: client exists: %s\n",
			    client_connect);
		} else if (strchr(client_connect, '=')) {
			rfbLog("wait_for_client: invalid -connect string: %s\n",
			    client_connect);
		} else {
			char *q = strchr(client_connect, ',');
			if (q) {
				rfbLog("wait_for_client: only using first"
				    " connect host in: %s\n", client_connect);
				remainder = strdup(q+1);
				*q = '\0';
			}
			rfbLog("wait_for_client: reverse_connect(%s)\n",
			    client_connect);
			reverse_connect(client_connect);
			*did_client_connect = 1;
		}
		free(client_connect);
		if (remainder != NULL) {
			/* reset to host2,host3,... */
			client_connect = remainder;
		} else {
			client_connect = NULL;
		}
	}
}

static void loop_for_connect(int did_client_connect) {
	int loop = 0;
	time_t start = time(NULL);

	if (first_conn_timeout < 0) {
		first_conn_timeout = -first_conn_timeout;
	}

	while (1) {
		loop++;
		if (first_conn_timeout && time(NULL) > start + first_conn_timeout) {
			rfbLog("no client connect after %d seconds.\n", first_conn_timeout);
			shut_down = 1;
		}
		if (shut_down) {
			clean_up_exit(0);
		}
		if (loop < 2) {
			if (did_client_connect) {
				goto screen_check;
			}
			if (inetd) {
				goto screen_check;
			}
			if (screen && screen->clientHead) {
				goto screen_check;
			}
		}
		if ((use_openssl || use_stunnel) && !inetd) {
			int enc_none = (enc_str && !strcmp(enc_str, "none"));
			if (!use_stunnel || enc_none) {
				check_openssl();
				check_https();
			}
			/*
			 * This is to handle an initial verify cert from viewer,
			 * they disconnect right after fetching the cert.
			 */
			if (use_threads) {
				usleep(10 * 1000);
			} else {
				rfbPE(-1);
			}
			if (screen && screen->clientHead) {
				int i;
				if (unixpw) {
					if (! unixpw_in_progress && !vencrypt_enable_plain_login) {
						rfbLog("unixpw but no unixpw_in_progress\n");
						clean_up_exit(1);
					}
					if (unixpw_client && unixpw_client->onHold) {
						rfbLog("taking unixpw_client off hold\n");
						unixpw_client->onHold = FALSE;
					}
				}
				for (i=0; i<10; i++) {
					if (shut_down) {
						clean_up_exit(0);
					}
					usleep(20 * 1000);
					if (0) rfbLog("wait_for_client: %d\n", i);

					if (! use_threads) {
						if (unixpw) {
							unixpw_in_rfbPE = 1;
						}
						rfbPE(-1);
						if (unixpw) {
							unixpw_in_rfbPE = 0;
						}
					}

					if (unixpw && !unixpw_in_progress) {
						/* XXX too soon. */
						goto screen_check;
					}
					if (!screen->clientHead) {
						break;
					}
				}
			}
		} else if (use_openssl) {
			check_openssl();
		}

		if (use_threads) {
			usleep(10 * 1000);
		} else {
			rfbPE(-1);
		}

		screen_check:
		if (! screen || ! screen->clientHead) {
			usleep(100 * 1000);
			continue;
		}

		rfbLog("wait_for_client: got client\n");
		break;
	}
}

static void do_unixpw_loop(void) {
	if (unixpw) {
		if (! unixpw_in_progress && !vencrypt_enable_plain_login) {
			rfbLog("unixpw but no unixpw_in_progress\n");
			clean_up_exit(1);
		}
		if (unixpw_client && unixpw_client->onHold) {
			rfbLog("taking unixpw_client off hold.\n");
			unixpw_client->onHold = FALSE;
		}
		while (1) {
			if (shut_down) {
				clean_up_exit(0);
			}
			if (! use_threads) {
				unixpw_in_rfbPE = 1;
				rfbPE(-1);
				unixpw_in_rfbPE = 0;
			}
			if (unixpw_in_progress) {
				static double lping = 0.0;
				if (lping < dnow() + 5) {
					mark_rect_as_modified(0, 0, 1, 1, 1);
					lping = dnow();
				}
				if (time(NULL) > unixpw_last_try_time + 45) {
					rfbLog("unixpw_deny: timed out waiting for reply.\n");
					unixpw_deny();
				}
				usleep(20 * 1000);
				continue;
			}
			rfbLog("wait_for_client: unixpw finished.\n");
			break;
		}
	}
}

static void vnc_redirect_loop(char *vnc_redirect_test, int *vnc_redirect_cnt) {
	if (unixpw) {
		rfbLog("wait_for_client: -unixpw and Xvnc.redirect not allowed\n");
		clean_up_exit(1);
	}
	if (client_connect) {
		rfbLog("wait_for_client: -connect and Xvnc.redirect not allowed\n");
		clean_up_exit(1);
	}
	if (inetd) {
		if (use_openssl) {
			accept_openssl(OPENSSL_INETD, -1);
		}
	} else {
		pid_t pid = 0;
		/* XXX ipv6 */
		if (screen->httpListenSock >= 0) {
#if LIBVNCSERVER_HAVE_FORK
			if ((pid = fork()) > 0) {
				close(screen->httpListenSock);
				/* mutex */
				screen->httpListenSock = -2;
				usleep(500 * 1000);
			} else {
				close(screen->listenSock);
				screen->listenSock = -1;
				while (1) {
					usleep(10 * 1000);
					rfbHttpCheckFds(screen);
				}
				exit(1);
			}
#else
			clean_up_exit(1);
#endif
		}
		if (first_conn_timeout) {
			if (first_conn_timeout < 0) {
				first_conn_timeout = -first_conn_timeout;
			}
			signal(SIGALRM, vnc_redirect_timeout);
			alarm(first_conn_timeout);
		}
		if (use_openssl) {
			int i;
			if (pid == 0) {
				accept_openssl(OPENSSL_VNC, -1);
			} else {
				for (i=0; i < 16; i++) {
					accept_openssl(OPENSSL_VNC, -1);
					rfbLog("iter %d: vnc_redirect_sock: %d\n", i, vnc_redirect_sock);
					if (vnc_redirect_sock >= 0) {
						break;
					}
				}
			}
		} else {
			struct sockaddr_in addr;
#ifdef __hpux
			int addrlen = sizeof(addr);
#else
			socklen_t addrlen = sizeof(addr);
#endif
			if (screen->listenSock < 0) {
				rfbLog("wait_for_client: Xvnc.redirect not listening... sock=%d port=%d\n", screen->listenSock, screen->port);
				clean_up_exit(1);
			}
			vnc_redirect_sock = accept(screen->listenSock, (struct sockaddr *)&addr, &addrlen);
		}
		if (first_conn_timeout) {
			alarm(0);
		}
		if (pid > 0) {
#if LIBVNCSERVER_HAVE_FORK
			int rc;
			pid_t pidw;
			rfbLog("wait_for_client: kill TERM: %d\n", (int) pid);
			kill(pid, SIGTERM);
			usleep(1000 * 1000);	/* 1.0 sec */
			pidw = waitpid(pid, &rc, WNOHANG);
			if (pidw <= 0) {
				usleep(1000 * 1000);	/* 1.0 sec */
				pidw = waitpid(pid, &rc, WNOHANG);
			}
#else
			clean_up_exit(1);
#endif
		}
	}
	if (vnc_redirect_sock < 0) {
		rfbLog("wait_for_client: vnc_redirect failed.\n");
		clean_up_exit(1);
	}
	if (!inetd && use_openssl) {
		/* check for Fetch Cert closing */
		fd_set rfds;
		struct timeval tv;
		int nfds;

		usleep(300*1000);

		FD_ZERO(&rfds);
		FD_SET(vnc_redirect_sock, &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 200000;
		nfds = select(vnc_redirect_sock+1, &rfds, NULL, NULL, &tv);

		rfbLog("wait_for_client: vnc_redirect nfds: %d\n", nfds);
		if (nfds > 0) {
			int n;
			n = read(vnc_redirect_sock, vnc_redirect_test, 1);
			if (n <= 0) {
				close(vnc_redirect_sock);
				vnc_redirect_sock = -1;
				rfbLog("wait_for_client: waiting for 2nd connection (Fetch Cert?)\n");
				accept_openssl(OPENSSL_VNC, -1);
				if (vnc_redirect_sock < 0) {
					rfbLog("wait_for_client: vnc_redirect failed.\n");
					clean_up_exit(1);
				}
			} else {
				*vnc_redirect_cnt = n;
			}
		}
	}
}

static void do_vnc_redirect(int created_disp, char *vnc_redirect_host, int vnc_redirect_port,
    int vnc_redirect_cnt, char *vnc_redirect_test) {
	char *q = strrchr(use_dpy, ':');
	int vdpy = -1, sock = -1;
	int s_in, s_out, i;
	if (vnc_redirect == 2) {
		char num[32];	
		sprintf(num, ":%d", vnc_redirect_port);
		q = num;
	}
	if (!q) {
		rfbLog("wait_for_client: can't find number in X display: %s\n", use_dpy);
		clean_up_exit(1);
	}
	if (sscanf(q+1, "%d", &vdpy) != 1) {
		rfbLog("wait_for_client: can't find number in X display: %s\n", q);
		clean_up_exit(1);
	}
	if (vdpy == -1 && vnc_redirect != 2) {
		rfbLog("wait_for_client: can't find number in X display: %s\n", q);
		clean_up_exit(1);
	}
	if (vnc_redirect == 2) {
		if (vdpy < 0) {
			vdpy = -vdpy;
		} else if (vdpy < 200) {
			vdpy += 5900;
		}
	} else {
		vdpy += 5900;
	}
	if (created_disp) {
		usleep(1000*1000);
	}
	for (i=0; i < 20; i++) {
		sock = connect_tcp(vnc_redirect_host, vdpy);
		if (sock >= 0) {
			break;
		}
		rfbLog("wait_for_client: ...\n");
		usleep(500*1000);
	}
	if (sock < 0) {
		rfbLog("wait_for_client: could not connect to a VNC Server at %s:%d\n", vnc_redirect_host, vdpy);
		clean_up_exit(1);
	}
	if (inetd) {
		s_in  = fileno(stdin);
		s_out = fileno(stdout);
	} else {
		s_in = s_out = vnc_redirect_sock;
	}
	if (vnc_redirect_cnt > 0) {
		write(vnc_redirect_sock, vnc_redirect_test, vnc_redirect_cnt);
	}
	rfbLog("wait_for_client: switching control to VNC Server at %s:%d\n", vnc_redirect_host, vdpy);
	raw_xfer(sock, s_in, s_out);
}

extern char find_display[];
extern char create_display[];

char *setup_cmd(char *str, int *vnc_redirect, char **vnc_redirect_host, int *vnc_redirect_port, int db) {
	char *cmd = NULL;
	
#ifndef WIN32
	if (no_external_cmds || !cmd_ok("WAIT")) {
		rfbLog("wait_for_client external cmds not allowed:"
		    " %s\n", use_dpy);
		clean_up_exit(1);
	}

	cmd = str + strlen("cmd=");
	if (!strcmp(cmd, "FINDDISPLAY-print")) {
		fprintf(stdout, "%s", find_display);
		clean_up_exit(0);
	}
	if (!strcmp(cmd, "FINDDISPLAY-run")) {
		char tmp[] = "/tmp/fd.XXXXXX";
		char com[100];
		int fd = mkstemp(tmp);
		if (fd >= 0) {
			int ret;
			write(fd, find_display, strlen(find_display));
			close(fd);
			set_env("FINDDISPLAY_run", "1");
			sprintf(com, "/bin/sh %s -n", tmp);
			ret = system(com);
			if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
				if (got_findauth && !getenv("FD_XDM")) {
					if (getuid() == 0 || geteuid() == 0) {
						set_env("FD_XDM", "1");
						system(com);
					}
				}
			}
		}
		unlink(tmp);
		exit(0);
	}
	if (!strcmp(str, "FINDCREATEDISPLAY-print")) {
		fprintf(stdout, "%s", create_display);
		clean_up_exit(0);
	}
	if (db) fprintf(stderr, "cmd: %s\n", cmd);
	if (strstr(str, "FINDCREATEDISPLAY") || strstr(str, "FINDDISPLAY")) {
		if (strstr(str, "Xvnc.redirect") || strstr(str, "X.redirect")) {
			*vnc_redirect = 1;
		}
	}
	if (strstr(cmd, "FINDDISPLAY-vnc_redirect") == cmd) {
		int p;
		char h[256];
		if (strlen(cmd) >= 256) {
			rfbLog("wait_for_client string too long: %s\n", str);
			clean_up_exit(1);
		}
		h[0] = '\0';
		if (sscanf(cmd, "FINDDISPLAY-vnc_redirect=%d", &p) == 1) {
			;
		} else if (sscanf(cmd, "FINDDISPLAY-vnc_redirect=%s %d", h, &p) == 2) {
			;
		} else {
			rfbLog("wait_for_client bad string: %s\n", cmd);
			clean_up_exit(1);
		}
		*vnc_redirect_port = p;
		if (strcmp(h, "")) {
			*vnc_redirect_host = strdup(h);
		}
		*vnc_redirect = 2;
		rfbLog("wait_for_client: vnc_redirect: %s:%d\n", *vnc_redirect_host, *vnc_redirect_port);
	}
#endif
	return cmd;
}

static char *build_create_cmd(char *cmd, int *saw_xdmcp, char *usslpeer, char *tmp) {
	char *create_cmd = NULL;
	char *opts = strchr(cmd, '-');
	char st[] = "";
	char fdgeom[128], fdsess[128], fdopts[128], fdextra[256], fdprog[128];
	char fdxsrv[128], fdxdum[128], fdcups[128], fdesd[128];
	char fdnas[128], fdsmb[128], fdtag[128], fdxdmcpif[128];
	char cdout[128];

	if (opts) {
		opts++;
		if (strstr(opts, "xdmcp")) {
			*saw_xdmcp = 1;
		}
	} else {
		opts = st;
	}
	sprintf(fdgeom, "NONE");
	fdsess[0] = '\0';
	fdgeom[0] = '\0';
	fdopts[0] = '\0';
	fdextra[0] = '\0';
	fdprog[0] = '\0';
	fdxsrv[0] = '\0';
	fdxdum[0] = '\0';
	fdcups[0] = '\0';
	fdesd[0]  = '\0';
	fdnas[0]  = '\0';
	fdsmb[0]  = '\0';
	fdtag[0]  = '\0';
	fdxdmcpif[0]  = '\0';
	cdout[0]  = '\0';

	if (unixpw && keep_unixpw_opts && !getenv("X11VNC_NO_UNIXPW_OPTS")) {
		char *q, *p, *t = strdup(keep_unixpw_opts);

		if (strstr(t, "gnome")) {
			sprintf(fdsess, "gnome");
		} else if (strstr(t, "kde")) {
			sprintf(fdsess, "kde");
		} else if (strstr(t, "lxde")) {
			sprintf(fdsess, "lxde");
		} else if (strstr(t, "twm")) {
			sprintf(fdsess, "twm");
		} else if (strstr(t, "fvwm")) {
			sprintf(fdsess, "fvwm");
		} else if (strstr(t, "mwm")) {
			sprintf(fdsess, "mwm");
		} else if (strstr(t, "cde")) {
			sprintf(fdsess, "cde");
		} else if (strstr(t, "dtwm")) {
			sprintf(fdsess, "dtwm");
		} else if (strstr(t, "xterm")) {
			sprintf(fdsess, "xterm");
		} else if (strstr(t, "wmaker")) {
			sprintf(fdsess, "wmaker");
		} else if (strstr(t, "xfce")) {
			sprintf(fdsess, "xfce");
		} else if (strstr(t, "enlightenment")) {
			sprintf(fdsess, "enlightenment");
		} else if (strstr(t, "Xsession")) {
			sprintf(fdsess, "Xsession");
		} else if (strstr(t, "failsafe")) {
			sprintf(fdsess, "failsafe");
		}

		q = strstr(t, "ge=");
		if (! q) q = strstr(t, "geom=");
		if (! q) q = strstr(t, "geometry=");
		if (q) {
			int ok = 1;
			q = strstr(q, "=");
			q++;
			p = strstr(q, ",");
			if (p) *p = '\0';
			p = q;
			while (*p) {
				if (*p == 'x') {
					;
				} else if (isdigit((int) *p)) {
					;
				} else {
					ok = 0;
					break;
				}
				p++;
			}
			if (ok && strlen(q) < 32) {
				sprintf(fdgeom, "%s", q);
				if (!quiet) {
					rfbLog("set create display geom: %s\n", fdgeom);
				}
			}
		}
		q = strstr(t, "cups=");
		if (q) {
			int p;
			if (sscanf(q, "cups=%d", &p) == 1) {
				sprintf(fdcups, "%d", p);
			}
		}
		q = strstr(t, "esd=");
		if (q) {
			int p;
			if (sscanf(q, "esd=%d", &p) == 1) {
				sprintf(fdesd, "%d", p);
			}
		}
		if (!getenv("FD_TAG")) {
			char *s = NULL;

			q = strstr(t, "tag=");
			if (q) s = strchr(q, ',');
			if (s) *s = '\0';

			if (q && strlen(q) < 120) {
				char *p;
				int ok = 1;
				q = strchr(q, '=') + 1;
				p = q;
				while (*p != '\0') {
					char c = *p;
					if (*p == '_' || *p == '-') {
						;
					} else if (!isalnum((int) c)) {
						ok = 0;
						rfbLog("bad tag char: '%c' in '%s'\n", c, q);
						break;
					}
					p++;
				}
				if (ok) {
					sprintf(fdtag, "%s", q);
				}
			}
			if (s) *s = ',';
		}
		free(t);
	}
	if (fdgeom[0] == '\0' && getenv("FD_GEOM")) {
		snprintf(fdgeom,  120, "%s", getenv("FD_GEOM"));
	}
	if (fdsess[0] == '\0' && getenv("FD_SESS")) {
		snprintf(fdsess, 120, "%s", getenv("FD_SESS"));
	}
	if (fdopts[0] == '\0' && getenv("FD_OPTS")) {
		snprintf(fdopts, 120, "%s", getenv("FD_OPTS"));
	}
	if (fdextra[0] == '\0' && getenv("FD_EXTRA")) {
		if (!strchr(getenv("FD_EXTRA"), '\'')) {
			snprintf(fdextra, 250, "%s", getenv("FD_EXTRA"));
		}
	}
	if (fdprog[0] == '\0' && getenv("FD_PROG")) {
		snprintf(fdprog, 120, "%s", getenv("FD_PROG"));
	}
	if (fdxsrv[0] == '\0' && getenv("FD_XSRV")) {
		snprintf(fdxsrv, 120, "%s", getenv("FD_XSRV"));
	}
	if (fdcups[0] == '\0' && getenv("FD_CUPS")) {
		snprintf(fdcups, 120, "%s", getenv("FD_CUPS"));
	}
	if (fdesd[0] == '\0' && getenv("FD_ESD")) {
		snprintf(fdesd, 120, "%s", getenv("FD_ESD"));
	}
	if (fdnas[0] == '\0' && getenv("FD_NAS")) {
		snprintf(fdnas, 120, "%s", getenv("FD_NAS"));
	}
	if (fdsmb[0] == '\0' && getenv("FD_SMB")) {
		snprintf(fdsmb, 120, "%s", getenv("FD_SMB"));
	}
	if (fdtag[0] == '\0' && getenv("FD_TAG")) {
		snprintf(fdtag, 120, "%s", getenv("FD_TAG"));
	}
	if (fdxdmcpif[0] == '\0' && getenv("FD_XDMCP_IF")) {
		snprintf(fdxdmcpif,  120, "%s", getenv("FD_XDMCP_IF"));
	}
	if (fdxdum[0] == '\0' && getenv("FD_XDUMMY_RUN_AS_ROOT")) {
		snprintf(fdxdum, 120, "%s", getenv("FD_XDUMMY_RUN_AS_ROOT"));
	}
	if (getenv("CREATE_DISPLAY_OUTPUT")) {
		snprintf(cdout, 120, "CREATE_DISPLAY_OUTPUT='%s'", getenv("CREATE_DISPLAY_OUTPUT"));
	}

	if (strchr(fdgeom, '\''))	fdgeom[0] = '\0';
	if (strchr(fdopts, '\''))	fdopts[0] = '\0';
	if (strchr(fdextra, '\''))	fdextra[0] = '\0';
	if (strchr(fdprog, '\''))	fdprog[0] = '\0';
	if (strchr(fdxsrv, '\''))	fdxsrv[0] = '\0';
	if (strchr(fdcups, '\''))	fdcups[0] = '\0';
	if (strchr(fdesd, '\''))	fdesd[0] = '\0';
	if (strchr(fdnas, '\''))	fdnas[0] = '\0';
	if (strchr(fdsmb, '\''))	fdsmb[0] = '\0';
	if (strchr(fdtag, '\''))	fdtag[0] = '\0';
	if (strchr(fdxdmcpif, '\''))	fdxdmcpif[0] = '\0';
	if (strchr(fdxdum, '\''))	fdxdum[0] = '\0';
	if (strchr(fdsess, '\''))	fdsess[0] = '\0';
	if (strchr(cdout, '\''))	cdout[0] = '\0';

	set_env("FD_GEOM", fdgeom);
	set_env("FD_OPTS", fdopts);
	set_env("FD_EXTRA", fdextra);
	set_env("FD_PROG", fdprog);
	set_env("FD_XSRV", fdxsrv);
	set_env("FD_CUPS", fdcups);
	set_env("FD_ESD",  fdesd);
	set_env("FD_NAS",  fdnas);
	set_env("FD_SMB",  fdsmb);
	set_env("FD_TAG",  fdtag);
	set_env("FD_XDMCP_IF",  fdxdmcpif);
	set_env("FD_XDUMMY_RUN_AS_ROOT", fdxdum);
	set_env("FD_SESS", fdsess);

	if (usslpeer || (unixpw && keep_unixpw_user)) {
		char *uu = usslpeer;
		if (!uu) {
			uu = keep_unixpw_user;
		}
		if (strchr(uu, '\''))  {
			uu = "";
		}
		create_cmd = (char *) malloc(strlen(tmp)+1
		    + strlen("env USER='' ")
		    + strlen("FD_GEOM='' ")
		    + strlen("FD_OPTS='' ")
		    + strlen("FD_EXTRA='' ")
		    + strlen("FD_PROG='' ")
		    + strlen("FD_XSRV='' ")
		    + strlen("FD_CUPS='' ")
		    + strlen("FD_ESD='' ")
		    + strlen("FD_NAS='' ")
		    + strlen("FD_SMB='' ")
		    + strlen("FD_TAG='' ")
		    + strlen("FD_XDMCP_IF='' ")
		    + strlen("FD_XDUMMY_RUN_AS_ROOT='' ")
		    + strlen("FD_SESS='' /bin/sh ")
		    + strlen(uu) + 1
		    + strlen(fdgeom) + 1
		    + strlen(fdopts) + 1
		    + strlen(fdextra) + 1
		    + strlen(fdprog) + 1
		    + strlen(fdxsrv) + 1
		    + strlen(fdcups) + 1
		    + strlen(fdesd) + 1
		    + strlen(fdnas) + 1
		    + strlen(fdsmb) + 1
		    + strlen(fdtag) + 1
		    + strlen(fdxdmcpif) + 1
		    + strlen(fdxdum) + 1
		    + strlen(fdsess) + 1
		    + strlen(cdout) + 1
		    + strlen(opts) + 1);
		sprintf(create_cmd, "env USER='%s' FD_GEOM='%s' FD_SESS='%s' "
		    "FD_OPTS='%s' FD_EXTRA='%s' FD_PROG='%s' FD_XSRV='%s' FD_CUPS='%s' "
		    "FD_ESD='%s' FD_NAS='%s' FD_SMB='%s' FD_TAG='%s' FD_XDMCP_IF='%s' "
		    "FD_XDUMMY_RUN_AS_ROOT='%s' %s /bin/sh %s %s",
		    uu, fdgeom, fdsess, fdopts, fdextra, fdprog, fdxsrv,
		    fdcups, fdesd, fdnas, fdsmb, fdtag, fdxdmcpif, fdxdum, cdout, tmp, opts);
	} else {
		create_cmd = (char *) malloc(strlen(tmp)
		    + strlen("/bin/sh ") + 1 + strlen(opts) + 1);
		sprintf(create_cmd, "/bin/sh %s %s", tmp, opts);
	}
	return create_cmd;
}

static char *certret_extract() {
	char *q, *p, *str = strdup(certret_str);
	char *upeer = NULL;
	int ok = 0;

	q = strstr(str, "Subject: ");
	if (! q) return NULL;

	p = strstr(q, "\n");
	if (p) *p = '\0';

	q = strstr(q, "CN=");
	if (! q) return NULL;

	if (! getenv("X11VNC_SSLPEER_CN")) {
		p = q;
		q = strstr(q, "/emailAddress=");
		if (! q) q = strstr(p, "/Email=");
		if (! q) return NULL;
	}

	q = strstr(q, "=");
	if (! q) return NULL;

	q++;
	p = strstr(q, " ");
	if (p) *p = '\0';
	p = strstr(q, "@");
	if (p) *p = '\0';
	p = strstr(q, "/");
	if (p) *p = '\0';

	upeer = strdup(q);

	if (strcmp(upeer, "")) {
		p = upeer;
		while (*p != '\0') {
			char c = *p;
			if (!isalnum((int) c)) {
				*p = '\0';
				break;
			}
			p++;
		}
		if (strcmp(upeer, "")) {
			ok = 1;
		}
	}
	if (! ok) {
		upeer = NULL;
	}
	return upeer;
}

static void check_nodisplay(char **nd, char **tag) {
	if (unixpw && !getenv("X11VNC_NO_UNIXPW_OPTS") && keep_unixpw_opts && keep_unixpw_opts[0] != '\0') {
		char *q, *t2, *t = keep_unixpw_opts;
		q = strstr(t, "nd=");
		if (! q) q = strstr(t, "nodisplay=");
		if (q) {
			q = strchr(q, '=') + 1;
			t = strdup(q);
			q = t;
			t2 = strchr(t, ',');
			if (t2) *t2 = '\0';

			while (*t != '\0') {
				if (*t == '+') {
					*t = ',';
				}
				t++;
			}
			if (!strchr(q, '\'') && !strpbrk(q, "[](){}`'\"$&*|<>")) {
				if (! quiet) rfbLog("set X11VNC_SKIP_DISPLAY: %s\n", q);
				*nd = q;
			}
		}

		q = strstr(keep_unixpw_opts, "tag=");
		if (getenv("FD_TAG")) {
			*tag = strdup(getenv("FD_TAG"));
		} else if (q) {
			q = strchr(q, '=') + 1;
			t = strdup(q);
			q = t;
			t2 = strchr(t, ',');
			if (t2) *t2 = '\0';

			if (strlen(q) < 120) {
				int ok = 1;
				while (*t != '\0') {
					char c = *t;
					if (*t == '_' || *t == '-') {
						;
					} else if (!isalnum((int) c)) {
						ok = 0;
						rfbLog("bad tag char: '%c' in '%s'\n", c, q);
						break;
					}
					t++;
				}
				if (ok) {
					if (! quiet) rfbLog("set FD_TAG: %s\n", q);
					*tag = q;
				}
			}
		}
	}
	if (unixpw_system_greeter_active == 2) {
		if (!keep_unixpw_user) {
			clean_up_exit(1);
		}
		*nd = strdup("all");
	}
}

static char *get_usslpeer() {
	char *u = NULL, *upeer = NULL;

	if (certret_str) {
		upeer = certret_extract();
	}
	if (!upeer) {
		return NULL;
	}
	rfbLog("sslpeer unix username extracted from x509 cert: %s\n", upeer);

	u = (char *) malloc(strlen(upeer+2));
	u[0] = '\0';
	if (!strcmp(users_list, "sslpeer=")) {
		sprintf(u, "+%s", upeer);
	} else {
		char *p, *str = strdup(users_list);
		p = strtok(str + strlen("sslpeer="), ",");
		while (p) {
			if (!strcmp(p, upeer)) {
				sprintf(u, "+%s", upeer);
				break;
			}
			p = strtok(NULL, ",");
		}
		free(str);
	}
	if (u[0] == '\0') {
		rfbLog("sslpeer cannot determine user: %s\n", upeer);
		free(u);
		return NULL;
	}
	free(u);
	return upeer;
}

static void do_try_switch(char *usslpeer, char *users_list_save) {
	if (unixpw_system_greeter_active == 2) {
		rfbLog("unixpw_system_greeter: not trying switch to user '%s'\n", usslpeer ? usslpeer : "");
		return;
	}
	if (usslpeer) {
		char *u = (char *) malloc(strlen(usslpeer+2));
		sprintf(u, "+%s", usslpeer);
		if (switch_user(u, 0)) {
			rfbLog("sslpeer switched to user: %s\n", usslpeer);
		} else {
			rfbLog("sslpeer failed to switch to user: %s\n", usslpeer);
		}
		free(u);
		
	} else if (users_list_save && keep_unixpw_user) {
		char *user = keep_unixpw_user;
		char *u = (char *)malloc(strlen(user)+1); 

		users_list = users_list_save;

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
			rfbLog("unixpw_accept skipping switch to user: %s (drc)\n", user);
		} else if (switch_user(u, 0)) {
			rfbLog("unixpw_accept switched to user: %s (drc)\n", user);
		} else {
			rfbLog("unixpw_accept failed to switch to user: %s (drc)\n", user);
		}
		free(u);
	}
}

static void path_lookup(char *prog) {
	/* see create_display script */
	char *create_display_extra = "/usr/X11R6/bin:/usr/bin/X11:/usr/openwin/bin:/usr/dt/bin:/opt/kde4/bin:/opt/kde3/bin:/opt/gnome/bin:/usr/bin:/bin:/usr/sfw/bin:/usr/local/bin";
	char *path, *try, *p;
	int found = 0, len = strlen(create_display_extra);

	if (getenv("PATH")) {
		len += strlen(getenv("PATH")) + 1;
		path = (char *) malloc((len+1) * sizeof(char));
		sprintf(path, "%s:%s", getenv("PATH"), create_display_extra);
	} else {
		path = (char *) malloc((len+1) * sizeof(char));
		sprintf(path, "%s", create_display_extra);
	}
	try = (char *) malloc((len+2+strlen(prog)) * sizeof(char));

	p = strtok(path, ":");
	while (p) {
		struct stat sbuf;

		sprintf(try, "%s/%s", p, prog);
		if (stat(try, &sbuf) == 0) {
			found = 1;
			break;
		}
		p = strtok(NULL, ":");
	}

	free(path);
	free(try);

	if (!found) {
		fprintf(stderr, "\n");
		fprintf(stderr, "The program \"%s\" could not be found in PATH and standard locations.\n", prog);
		fprintf(stderr, "You probably need to install a package that provides the \"%s\" program.\n", prog);
		fprintf(stderr, "Without it FINDCREATEDISPLAY mode may not be able to create an X display.\n");
		fprintf(stderr, "\n");
	}
}

static int do_run_cmd(char *cmd, char *create_cmd, char *users_list_save, int created_disp, int db) {
#ifndef WIN32
	char tmp[] = "/tmp/x11vnc-find_display.XXXXXX";
	char line1[1024], line2[16384];
	char *q, *usslpeer = NULL;
	int n, nodisp = 0, saw_xdmcp = 0;
	int tmp_fd = -1;
	int internal_cmd = 0;
	int tried_switch = 0;

	memset(line1, 0, sizeof(line1));
	memset(line2, 0, sizeof(line2));

	if (users_list && strstr(users_list, "sslpeer=") == users_list) {
		usslpeer = get_usslpeer();
		if (! usslpeer) {
			return 0;
		}
	}
	if (getenv("DEBUG_RUN_CMD")) db = 1;

	/* only sets environment variables: */
	run_user_command("", latest_client, "env", NULL, 0, NULL);

	if (program_name) {
		set_env("X11VNC_PROG", program_name);
	} else {
		set_env("X11VNC_PROG", "x11vnc");
	}

	if (!strcmp(cmd, "FINDDISPLAY") ||
	    strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
		char *nd = "";
		char *tag = "";
		char fdout[128];

		internal_cmd = 1;

		tmp_fd = mkstemp(tmp);

		if (tmp_fd < 0) {
			rfbLog("wait_for_client: open failed: %s\n", tmp);
			rfbLogPerror("mkstemp");
			clean_up_exit(1);
		}
		chmod(tmp, 0644);
		if (getenv("X11VNC_FINDDISPLAY_ALWAYS_FAILS")) {
			char *s = "#!/bin/sh\necho _FAIL_\nexit 1\n";
			write(tmp_fd, s, strlen(s));
		} else {
			write(tmp_fd, find_display, strlen(find_display));
		}
		close(tmp_fd);
		nodisp = 1;

		if (strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
			create_cmd = build_create_cmd(cmd, &saw_xdmcp, usslpeer, tmp);
			if (db) fprintf(stderr, "create_cmd: %s\n", create_cmd);
		}
		if (getenv("X11VNC_SKIP_DISPLAY")) {
			nd = strdup(getenv("X11VNC_SKIP_DISPLAY"));
		}
		check_nodisplay(&nd, &tag);

		fdout[0] = '\0';
		if (getenv("FIND_DISPLAY_OUTPUT")) {
			snprintf(fdout, 120, " FIND_DISPLAY_OUTPUT='%s' ", getenv("FIND_DISPLAY_OUTPUT"));
		}

		cmd = (char *) malloc(strlen("env X11VNC_SKIP_DISPLAY='' ")
		    + strlen(nd) + strlen(" FD_TAG='' ") + strlen(tag) + strlen(tmp) + strlen("/bin/sh ") + strlen(fdout) + 1);

		if (strcmp(tag, "")) {
			sprintf(cmd, "env X11VNC_SKIP_DISPLAY='%s' FD_TAG='%s' %s /bin/sh %s", nd, tag, fdout, tmp);
		} else {
			sprintf(cmd, "env X11VNC_SKIP_DISPLAY='%s' %s /bin/sh %s", nd, fdout, tmp);
		}
	}

	rfbLog("wait_for_client: running: %s\n", cmd);

	if (create_cmd != NULL) {
		if (strstr(create_cmd, "Xvfb")) {
			path_lookup("Xvfb");
		}
		if (strstr(create_cmd, "Xvnc")) {
			path_lookup("Xvnc");
		}
		if (strstr(create_cmd, "Xdummy")) {
			path_lookup("Xdummy");
		}
	}

	if (unixpw && !unixpw_nis) {
		int res = 0, k, j, i;
		char line[18000];

		memset(line, 0, sizeof(line));

		if (unixpw_system_greeter_active == 2) {
			rfbLog("unixpw_system_greeter: forcing find display failure.\n");
			res = 0;
		} else if (keep_unixpw_user && keep_unixpw_pass) {
			n = sizeof(line);
			if (unixpw_cmd != NULL) {
				res = unixpw_cmd_run(keep_unixpw_user,
				    keep_unixpw_pass, cmd, line, &n);
			} else {
				res = su_verify(keep_unixpw_user,
				    keep_unixpw_pass, cmd, line, &n, nodisp);
			}
		}

if (db) {fprintf(stderr, "line: "); write(2, line, n); write(2, "\n", 1); fprintf(stderr, "res=%d n=%d\n", res, n);}
		if (! res) {
			rfbLog("wait_for_client: find display cmd failed.\n");
		}

		if (! res && create_cmd) {
			FILE *mt = fopen(tmp, "w");
			if (! mt) {
				rfbLog("wait_for_client: open failed: %s\n", tmp);
				rfbLogPerror("fopen");
				clean_up_exit(1);
			}
			fprintf(mt, "%s", create_display);
			fclose(mt);

			findcreatedisplay = 1;

			if (unixpw_cmd != NULL) {
				/* let the external unixpw command do it: */
				n = sizeof(line);
				close_exec_fds();
				res = unixpw_cmd_run(keep_unixpw_user,
				    keep_unixpw_pass, create_cmd, line, &n);
			} else if (getuid() != 0 && unixpw_system_greeter_active != 2) {
				/* if not root, run as the other user... */
				n = sizeof(line);
				close_exec_fds();
				res = su_verify(keep_unixpw_user,
				    keep_unixpw_pass, create_cmd, line, &n, nodisp);
if (db) fprintf(stderr, "c-res=%d n=%d line: '%s'\n", res, n, line);

			} else {
				FILE *p;
				close_exec_fds();
				if (unixpw_system_greeter_active == 2) {
					rfbLog("unixpw_system_greeter: not trying su_verify() to run\n");
					rfbLog("unixpw_system_greeter: create display command.\n");
				}
				rfbLog("wait_for_client: running: %s\n", create_cmd);
				p = popen(create_cmd, "r");
				if (! p) {
					rfbLog("wait_for_client: popen failed: %s\n", create_cmd);
					res = 0;
				} else if (fgets(line1, 1024, p) == NULL) {
					rfbLog("wait_for_client: read failed: %s\n", create_cmd);
					res = 0;
				} else {
					n = fread(line2, 1, 16384, p);
					if (pclose(p) != 0) {
						res = 0;
					} else {
						strncpy(line, line1, 100);
						memcpy(line + strlen(line1), line2, n);
if (db) fprintf(stderr, "line1: '%s'\n", line1);
						n += strlen(line1);
						created_disp = 1;
						res = 1;
					}
				}
			}
			if (res && saw_xdmcp && unixpw_system_greeter_active != 2) {
				xdmcp_insert = strdup(keep_unixpw_user);
			}
		}

		if (tmp_fd >= 0) {
			unlink(tmp);
		}

		if (! res) {
			rfbLog("wait_for_client: cmd failed: %s\n", cmd);
			unixpw_msg("No DISPLAY found.", 3);
			clean_up_exit(1);
		}

		/*
		 * we need to hunt for DISPLAY= since there may be
		 * a login banner or something at the beginning.
		 */
		q = strstr(line, "DISPLAY=");
		if (! q) {
			q = line;
		}
		n -= (q - line);

		for (k = 0; k < 1024; k++) {
			line1[k] = q[k];
			if (q[k] == '\n') {
				k++;
				break;
			}
		}
		n -= k;
		i = 0;
		for (j = 0; j < 16384; j++) {
			if (j < 16384 - 1) {
				/* xauth data, assume pty added CR */
				if (q[k+j] == '\r' && q[k+j+1] == '\n') {
					continue;
				}
			}
			
			line2[i] = q[k+j];
			i++;
		}
if (db) write(2, line, 100);
if (db) fprintf(stderr, "\n");

	} else {
		FILE *p;
		int rc;
		close_exec_fds();

		if (usslpeer) {
			char *c;
			if (getuid() == 0) {
				c = (char *) malloc(strlen("su - '' -c \"")
				    + strlen(usslpeer) + strlen(cmd) + 1 + 1);
				sprintf(c, "su - '%s' -c \"%s\"", usslpeer, cmd);
			} else {
				c = strdup(cmd);
			}
			p = popen(c, "r");
			free(c);
			
		} else if (unixpw_nis && keep_unixpw_user) {
			char *c;
			if (getuid() == 0) {
				c = (char *) malloc(strlen("su - '' -c \"")
				    + strlen(keep_unixpw_user) + strlen(cmd) + 1 + 1);
				sprintf(c, "su - '%s' -c \"%s\"", keep_unixpw_user, cmd);
			} else {
				c = strdup(cmd);
			}
			p = popen(c, "r");
			free(c);
			
		} else {
			p = popen(cmd, "r");
		}

		if (! p) {
			rfbLog("wait_for_client: cmd failed: %s\n", cmd);
			rfbLogPerror("popen");
			if (tmp_fd >= 0) {
				unlink(tmp);
			}
			clean_up_exit(1);
		}
		if (fgets(line1, 1024, p) == NULL) {
			rfbLog("wait_for_client: read failed: %s\n", cmd);
			rfbLogPerror("fgets");
			if (tmp_fd >= 0) {
				unlink(tmp);
			}
			clean_up_exit(1);
		}
		n = fread(line2, 1, 16384, p);
		rc = pclose(p);

		if (rc != 0) {
			rfbLog("wait_for_client: find display cmd failed.\n");
		}

		if (create_cmd && rc != 0) {
			FILE *mt = fopen(tmp, "w");
			if (! mt) {
				rfbLog("wait_for_client: open failed: %s\n", tmp);
				rfbLogPerror("fopen");
				if (tmp_fd >= 0) {
					unlink(tmp);
				}
				clean_up_exit(1);
			}
			fprintf(mt, "%s", create_display);
			fclose(mt);

			findcreatedisplay = 1;

			rfbLog("wait_for_client: FINDCREATEDISPLAY cmd: %s\n", create_cmd);

			p = popen(create_cmd, "r");
			if (! p) {
				rfbLog("wait_for_client: cmd failed: %s\n", create_cmd);
				rfbLogPerror("popen");
				if (tmp_fd >= 0) {
					unlink(tmp);
				}
				clean_up_exit(1);
			}
			if (fgets(line1, 1024, p) == NULL) {
				rfbLog("wait_for_client: read failed: %s\n", create_cmd);
				rfbLogPerror("fgets");
				if (tmp_fd >= 0) {
					unlink(tmp);
				}
				clean_up_exit(1);
			}
			n = fread(line2, 1, 16384, p);
			pclose(p);
		}
		if (tmp_fd >= 0) {
			unlink(tmp);
		}
	}

if (db) fprintf(stderr, "line1=%s\n", line1);

	if (strstr(line1, "DISPLAY=") != line1) {
		rfbLog("wait_for_client: bad reply '%s'\n", line1);
		if (unixpw) {
			unixpw_msg("No DISPLAY found.", 3);
		}
		clean_up_exit(1);
	}


	if (strstr(line1, ",VT=")) {
		int vt;
		char *t = strstr(line1, ",VT=");
		vt = atoi(t + strlen(",VT="));
		*t = '\0';
		if (7 <= vt && vt <= 15) {
			do_chvt(vt);
		}
	} else if (strstr(line1, ",XPID=")) {
		int i, pvt, vt = -1;
		char *t = strstr(line1, ",XPID=");
		pvt = atoi(t + strlen(",XPID="));
		*t = '\0';
		if (pvt > 0) {
			for (i=3; i <= 10; i++) {
				int k;
				char proc[100];
				char buf[100];
				sprintf(proc, "/proc/%d/fd/%d", pvt, i);
if (db) fprintf(stderr, "%d -- %s\n", i, proc);
				for (k=0; k < 100; k++) {
					buf[k] = '\0';
				}
	
				if (readlink(proc, buf, 100) != -1) {
					buf[100-1] = '\0';
if (db) fprintf(stderr, "%d -- %s -- %s\n", i, proc, buf);
					if (strstr(buf, "/dev/tty") == buf) {
						vt = atoi(buf + strlen("/dev/tty"));
						if (vt > 0) {
							break;
						}
					}
				}
			}
		}
		if (7 <= vt && vt <= 12) {
			do_chvt(vt);
		}
	}

	use_dpy = strdup(line1 + strlen("DISPLAY="));
	q = use_dpy;
	while (*q != '\0') {
		if (*q == '\n' || *q == '\r') *q = '\0';
		q++;
	}
	if (line2[0] != '\0') {
		if (strstr(line2, "XAUTHORITY=") == line2) {
			q = line2;
			while (*q != '\0') {
				if (*q == '\n' || *q == '\r') *q = '\0';
				q++;
			}
			if (auth_file) {
				free(auth_file);
			}
			auth_file = strdup(line2 + strlen("XAUTHORITY="));

		} else {
			xauth_raw_data = (char *)malloc(n);
			xauth_raw_len = n;
			memcpy(xauth_raw_data, line2, n);
if (db) {fprintf(stderr, "xauth_raw_len: %d\n", n);
write(2, xauth_raw_data, n);
fprintf(stderr, "\n");}
		}
	}

	if (!tried_switch) {
		do_try_switch(usslpeer, users_list_save);
		tried_switch = 1;
	}

	if (unixpw) {
		/* Some cleanup and messaging for -unixpw case: */
		char str[32];

		if (keep_unixpw_user && keep_unixpw_pass) {
			strzero(keep_unixpw_user);
			strzero(keep_unixpw_pass);
			keep_unixpw = 0;
		}

		if (created_disp) {
			snprintf(str, 30, "Created DISPLAY %s", use_dpy);
		} else {
			snprintf(str, 30, "Using DISPLAY %s", use_dpy);
		}
		unixpw_msg(str, 2);
	}
#endif
	return 1;
}

void ssh_remote_tunnel(char *, int);

static XImage ximage_struct;

void progress_client(void) {
	int i, j = 0, progressed = 0, db = 0;
	double start = dnow();
	if (getenv("PROGRESS_CLIENT_DBG")) {
		rfbLog("progress_client: begin\n");
		db = 1;
	}
	for (i = 0; i < 15; i++) {
		if (latest_client) {
			for (j = 0; j < 10; j++) {
				if (latest_client->state != RFB_PROTOCOL_VERSION) {
					progressed = 1;
					break;
				}
				if (db) rfbLog("progress_client: calling-1 rfbCFD(1) %.6f\n", dnow()-start);
				rfbCFD(1);
			}
		}
		if (progressed) {
			break;
		}
		if (db) rfbLog("progress_client: calling-2 rfbCFD(1) %.6f\n", dnow()-start);
		rfbCFD(1);
	}
	if (!quiet) {
		rfbLog("client progressed=%d in %d/%d %.6f s\n",
		    progressed, i, j, dnow() - start);
	}
}

int wait_for_client(int *argc, char** argv, int http) {
	/* ugh, here we go... */
	XImage* fb_image;
	int w = 640, h = 480, b = 32;
	int w0 = -1, h0 = -1, i, chg_raw_fb = 0;
	char *str, *q, *cmd = NULL;
	int db = 0, dt = 0;
	char *create_cmd = NULL;
	char *users_list_save = NULL;
	int created_disp = 0, ncache_save;
	int did_client_connect = 0;
	char *vnc_redirect_host = "localhost";
	int vnc_redirect_port = -1, vnc_redirect_cnt = 0;
	char vnc_redirect_test[10];

	if (getenv("WAIT_FOR_CLIENT_DB")) {
		db = 1;
	}

	vnc_redirect = 0;

	if (! use_dpy || strstr(use_dpy, "WAIT:") != use_dpy) {
		return 0;
	}

	for (i=0; i < *argc; i++) {
		if (!strcmp(argv[i], "-desktop")) {
			dt = 1;
		}
		if (db) fprintf(stderr, "args %d %s\n", i, argv[i]);
	}
	if (!quiet && !strstr(use_dpy, "FINDDISPLAY-run")) {
		rfbLog("\n");
		rfbLog("wait_for_client: %s\n", use_dpy);
		rfbLog("\n");
	}

	str = strdup(use_dpy);
	str += strlen("WAIT");

	xdmcp_insert = NULL;

	/* get any leading geometry: */
	q = strchr(str+1, ':');
	if (q) {
		*q = '\0';
		if (sscanf(str+1, "%dx%d", &w0, &h0) == 2)  {
			w = w0;
			h = h0;
			rfbLog("wait_for_client set: w=%d h=%d\n", w, h);
		} else {
			w0 = -1;
			h0 = -1;
		}
		*q = ':';
		str = q;
	}
	if ((w0 == -1 || h0 == -1) && pad_geometry != NULL) {
		int b0, del = 0;
		char *s = pad_geometry;
		if (strstr(s, "once:") == s) {
			del = 1;
			s += strlen("once:");
		}
		if (sscanf(s, "%dx%dx%d", &w0, &h0, &b0) == 3)  {
			w = nabs(w0);
			h = nabs(h0);
			b = nabs(b0);
		} else if (sscanf(s, "%dx%d", &w0, &h0) == 2)  {
			w = nabs(w0);
			h = nabs(h0);
		}
		if (del) {
			pad_geometry = NULL;
		}
	}

	/* str currently begins with a ':' */
	if (strstr(str, ":cmd=") == str) {
		/* cmd=/path/to/mycommand */
		str++;
	} else if (strpbrk(str, "0123456789") == str+1) {
		/* :0.0 */
		;
	} else {
		/* hostname:0.0 */
		str++;
	}

	if (db) fprintf(stderr, "str: %s\n", str);

	if (strstr(str, "cmd=") == str) {
		cmd = setup_cmd(str, &vnc_redirect, &vnc_redirect_host, &vnc_redirect_port, db);
	}
	
	fb_image = &ximage_struct;
	setup_fake_fb(fb_image, w, h, b);

	if (! dt) {
		char *s;
		argv[*argc] = strdup("-desktop");
		*argc = (*argc) + 1;

		if (cmd) {
			char *q;
			s = choose_title(":0");
			q = strstr(s, ":0");
			if (q) {
				*q = '\0';
			}
		} else {
			s = choose_title(str);
		}
		rfb_desktop_name = strdup(s);
		argv[*argc] = s;
		*argc = (*argc) + 1;
	}

	ncache_save = ncache;
	ncache = 0;

	initialize_allowed_input();

	if (! multiple_cursors_mode) {
		multiple_cursors_mode = strdup("default");
	}
	initialize_cursors_mode();
	
	initialize_screen(argc, argv, fb_image);

	if (! inetd && ! use_openssl) {
		if (! screen->port || screen->listenSock < 0) {
			if (got_rfbport && got_rfbport_val == 0) {
				;
			} else if (ipv6_listen && ipv6_listen_fd >= 0) {
				rfbLog("Info: listening on IPv6 interface only.  (wait for client)\n");
			} else {
				rfbLogEnable(1);
				rfbLog("Error: could not obtain listening port.  (wait for client)\n");
				if (!got_rfbport && !got_ipv6_listen) {
					rfbLog("If this system is IPv6-only, use the -6 option.\n");
				}
				clean_up_exit(1);
			}
		}
	}

	initialize_signals();

	if (ssh_str != NULL) {
		ssh_remote_tunnel(ssh_str, screen->port);
	}

	if (! raw_fb) {
		chg_raw_fb = 1;
		/* kludge to get RAWFB_RET with dpy == NULL guards */
		raw_fb = (char *) 0x1;
	}

	if (cmd && !strcmp(cmd, "HTTPONCE")) {
		handle_one_http_request();	
		clean_up_exit(0);
	}

	if (http && check_httpdir()) {
		http_connections(1);
	}

	if (cmd && unixpw) {
		keep_unixpw = 1;
	}

	setup_service();

	check_waitbg();

	if (vnc_redirect) {
		vnc_redirect_loop(vnc_redirect_test, &vnc_redirect_cnt);
	} else {

		if (use_threads && !started_rfbRunEventLoop) {
			started_rfbRunEventLoop = 1;
			rfbRunEventLoop(screen, -1, TRUE);
		}

		if (inetd && use_openssl) {
			accept_openssl(OPENSSL_INETD, -1);
		}

		setup_client_connect(&did_client_connect);

		loop_for_connect(did_client_connect);

		if (unixpw) {
			if (cmd && strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
				if (users_list && strstr(users_list, "unixpw=") == users_list) {
					users_list_save = users_list;
					users_list = NULL;
				}
			}
			do_unixpw_loop();
		} else if (cmd && !use_threads) {
			/* try to get RFB proto done now. */
			progress_client();
		}
	}

	if (vnc_redirect == 2) {
		;
	} else if (cmd) {
		if (!do_run_cmd(cmd, create_cmd, users_list_save, created_disp, db)) {
			return 0;
		}
	} else {
		use_dpy = strdup(str);
	}
	if (chg_raw_fb) {
		raw_fb = NULL;
	}

	ncache = ncache_save;

	if (unixpw && keep_unixpw_opts && keep_unixpw_opts[0] != '\0') {
		user_supplied_opts(keep_unixpw_opts);
	}
	if (create_cmd) {
		free(create_cmd);
	}

	if (vnc_redirect) {
		do_vnc_redirect(created_disp, vnc_redirect_host, vnc_redirect_port,
		    vnc_redirect_cnt, vnc_redirect_test);
		clean_up_exit(0);
	}

	return 1;
}

