/* -- util.c -- */

#include "x11vnc.h"
#include "cleanup.h"
#include "win_utils.h"
#include "unixpw.h"

struct timeval _mysleep;

/* this is only for debugging mutexes. see util.h */
int hxl = 0;

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
MUTEX(x11Mutex);
MUTEX(scrollMutex);
#endif

int nfix(int i, int n);
int nmin(int n, int m);
int nmax(int n, int m);
int nabs(int n);
double dabs(double x);
void lowercase(char *str);
void uppercase(char *str);
char *lblanks(char *str);
void strzero(char *str);
int scan_hexdec(char *str, unsigned long *num);
int parse_geom(char *str, int *wp, int *hp, int *xp, int *yp, int W, int H);
void set_env(char *name, char *value);
char *bitprint(unsigned int st, int nbits);
char *get_user_name(void);
char *get_home_dir(void);
char *get_shell(void);
char *this_host(void);

int match_str_list(char *str, char **list);
char **create_str_list(char *cslist);

double dtime(double *);
double dtime0(double *);
double dnow(void);
double dnowx(void);
double rnow(void);
double rfac(void);

void rfbPE(long usec);
void rfbCFD(long usec);

double rect_overlap(int x1, int y1, int x2, int y2, int X1, int Y1,
    int X2, int Y2);

char *choose_title(char *display);


/*
 * routine to keep 0 <= i < n
 */
int nfix(int i, int n) {
	if (i < 0) {
		i = 0;
	} else if (i >= n) {
		i = n - 1;
	}
	return i;
}

int nmin(int n, int m) {
	if (n < m) {
		return n;
	} else {
		return m;
	}
}

int nmax(int n, int m) {
	if (n > m) {
		return n;
	} else {
		return m;
	}
}

int nabs(int n) {
	if (n < 0) {
		return -n;
	} else {
		return n;
	}
}

double dabs(double x) {
	if (x < 0.0) {
		return -x;
	} else {
		return x;
	}
}

void lowercase(char *str) {
	char *p;
	if (str == NULL) {
		return;
	}
	p = str;
	while (*p != '\0') {
		*p = tolower((unsigned char) (*p));
		p++;
	}
}

void uppercase(char *str) {
	char *p;
	if (str == NULL) {
		return;
	}
	p = str;
	while (*p != '\0') {
		*p = toupper((unsigned char) (*p));
		p++;
	}
}

char *lblanks(char *str) {
	char *p = str;
	while (*p != '\0') {
		if (! isspace((unsigned char) (*p))) {
			break;
		}
		p++;
	}
	return p;
}

void strzero(char *str) {
	char *p = str;
	if (p != NULL) {
		while (*p != '\0') {
			*p = '\0';
			p++;
		}
	}
}

int scan_hexdec(char *str, unsigned long *num) {
	if (sscanf(str, "0x%lx", num) != 1) {
		if (sscanf(str, "%lu", num) != 1) {
			return 0;
		}
	}
	return 1;
}

int parse_geom(char *str, int *wp, int *hp, int *xp, int *yp, int W, int H) {
	int w, h, x, y;
	if (! str) {
		return 0;
	}
	/* handle +/-x and +/-y */
	if (sscanf(str, "%dx%d+%d+%d", &w, &h, &x, &y) == 4) {
		;
	} else if (sscanf(str, "%dx%d-%d+%d", &w, &h, &x, &y) == 4) {
		w = nabs(w);
		x = W - x - w;
	} else if (sscanf(str, "%dx%d+%d-%d", &w, &h, &x, &y) == 4) {
		h = nabs(h);
		y = H - y - h;
	} else if (sscanf(str, "%dx%d-%d-%d", &w, &h, &x, &y) == 4) {
		w = nabs(w);
		h = nabs(h);
		x = W - x - w;
		y = H - y - h;
	} else {
		return 0;
	}
	*wp = w;
	*hp = h;
	*xp = x;
	*yp = y;
	return 1;
}

void set_env(char *name, char *value) {
	char *str;
	if (! name) {
		return;
	}
	if (! value) {
		value = "";
	}
	str = (char *) malloc(strlen(name) + 1 + strlen(value) + 1);
	sprintf(str, "%s=%s", name, value);
	putenv(str);
}

char *bitprint(unsigned int st, int nbits) {
	static char str[33];
	int i, mask;
	if (nbits > 32) {
		nbits = 32;
	}
	for (i=0; i<nbits; i++) {
		str[i] = '0';
	}
	str[nbits] = '\0';
	mask = 1;
	for (i=nbits-1; i>=0; i--) {
		if (st & mask) {
			str[i] = '1';
		}
		mask = mask << 1;
	}
	return str;	/* take care to use or copy immediately */
}

char *get_user_name(void) {
	char *user = NULL;

	user = getenv("USER");
	if (user == NULL) {
		user = getenv("LOGNAME");
	}

#if LIBVNCSERVER_HAVE_PWD_H
	if (user == NULL) {
		struct passwd *pw = getpwuid(getuid());
		if (pw) {
			user = pw->pw_name;
		}
	}
#endif

	if (user) {
		return(strdup(user));
	} else {
		return(strdup("unknown-user"));
	}
}

char *get_home_dir(void) {
	char *home = NULL;

	home = getenv("HOME");

#if LIBVNCSERVER_HAVE_PWD_H
	if (home == NULL) {
		struct passwd *pw = getpwuid(getuid());
		if (pw) {
			home = pw->pw_dir;
		}
	}
#endif

	if (home) {
		return(strdup(home));
	} else {
		return(strdup("/"));
	}
}

char *get_shell(void) {
	char *shell = NULL;

	shell = getenv("SHELL");

#if LIBVNCSERVER_HAVE_PWD_H
	if (shell == NULL) {
		struct passwd *pw = getpwuid(getuid());
		if (pw) {
			shell = pw->pw_shell;
		}
	}
#endif

	if (shell) {
		return(strdup(shell));
	} else {
		return(strdup("/bin/sh"));
	}
}

/*
 * utility to get the current host name
 */
char *this_host(void) {
	char host[MAXN];
#if LIBVNCSERVER_HAVE_GETHOSTNAME
	if (gethostname(host, MAXN) == 0) {
		host[MAXN-1] = '\0';
		return strdup(host);
	} else if (UT.nodename) {
		return strdup(UT.nodename);
	}
#endif
	return NULL;
}

int match_str_list(char *str, char **list) {
	int i = 0, matched = 0;

	if (! str || ! list) {
		return 0;
	}
	while (list[i] != NULL) {
		if (!strcmp(list[i], "*")) {
			matched = 1;
			break;
		} else if (strstr(str, list[i])) {
			matched = 1;
			break;
		}
		i++;
	}
	return matched;
}

char **create_str_list(char *cslist) {
	int i, n;
	char *p, *str;
	char **list = NULL;

	if (! cslist) {
		return NULL;
	}
	
	str = strdup(cslist);
	n = 1;
	p = str;
	while (*p != '\0') {
		if (*p == ',') {
			n++;
		}
		p++;
	}

	/* the extra last one holds NULL */
	list = (char **) malloc( (n+1)*sizeof(char *) );
	for(i=0; i < n+1; i++) {
		list[i] = NULL;
	}

	p = strtok(str, ",");
	i = 0;
	while (p && i < n) {
		list[i++] = strdup(p);
		p = strtok(NULL, ",");
	}
	free(str);

	return list;
}

/*
 * simple function for measuring sub-second time differences, using
 * a double to hold the value.
 */
double dtime(double *t_old) {
	/* 
	 * usage: call with 0.0 to initialize, subsequent calls give
	 * the time difference since last call.
	 */
	double t_now, dt;
	struct timeval now;

	gettimeofday(&now, NULL);
	t_now = now.tv_sec + ( (double) now.tv_usec/1000000. );
	if (*t_old == 0.0) {
		*t_old = t_now;
		return t_now;
	}
	dt = t_now - *t_old;
	*t_old = t_now;
	return(dt);
}

/* common dtime() activities: */
double dtime0(double *t_old) {
	*t_old = 0.0;
	return dtime(t_old);
}

double dnow(void) {
	double t;
	return dtime0(&t);
}

double dnowx(void) {
	return dnow() - x11vnc_start;
}

double rnow(void) {
	double t = dnowx();
	t = t - ((int) t); 
	if (t > 1.0) {
		t = 1.0;
	} else if (t < 0.0) {
		t = 0.0;
	}
	return t;
}

double rfac(void) {
	double f = (double) rand();
	f = f / ((double) RAND_MAX);
	return f;
}

void check_allinput_rate(void) {
	static double last_all_input_check = 0.0;
	static int set = 0;
	if (! set) {
		set = 1;
		last_all_input_check = dnow();
	} else {
		int dt = 4;
		if (x11vnc_current > last_all_input_check + dt) {
			int n, nq = 0;
			while ((n = rfbCheckFds(screen, 0))) {
				nq += n;
			}
			fprintf(stderr, "nqueued: %d\n", nq);
			if (0 && nq > 25 * dt) {
				double rate = nq / dt;
				rfbLog("Client is sending %.1f extra requests per second for the\n", rate);
				rfbLog("past %d seconds! Switching to -allpinput mode. (queued: %d)\n", dt, nq);
				all_input = 1;
			}
			set = 0;
		}
	}
}

static void do_allinput(long usec) {
	static double last = 0.0;
	static int meas = 0;
	int n, f = 1, cnt = 0;
	long usec0;
	double now;
	if (!screen || !screen->clientHead) {
		return;
	}
	if (usec < 0) {
		usec = 0;
	}
	usec0 = usec;
	if (last == 0.0) {
		last = dnow();
	}
	while ((n = rfbCheckFds(screen, usec)) > 0) {
		if (f) {
			fprintf(stderr, " *");
			f = 0;
		}
		if (cnt++ > 30) {
			break;
		}
		meas += n;
	}
	fprintf(stderr, "-%d", cnt);
	now = dnow();
	if (now > last + 2.0) {
		double rate = meas / (now - last);
		fprintf(stderr, "\n%.2f ", rate);
		meas = 0;
		last = dnow();
	}
}

/*
 * utility wrapper to call rfbProcessEvents
 * checks that we are not in threaded mode.
 */
#define USEC_MAX 999999		/* libvncsever assumes < 1 second */
void rfbPE(long usec) {
	int uip0 = unixpw_in_progress;
	static int check_rate = -1;
	if (! screen) {
		return;
	}
 	if (unixpw && unixpw_in_progress && !unixpw_in_rfbPE) {
		rfbLog("unixpw_in_rfbPE: skipping rfbPE\n");
 		return;
 	}

	if (debug_tiles > 2) {
		double tm = dnow();
		fprintf(stderr, "rfbPE(%d)  t: %.4f\n",
		    (int) usec, tm - x11vnc_start);
	}

	if (usec > USEC_MAX) {
		usec = USEC_MAX;
	}
	if (! use_threads) {
		rfbProcessEvents(screen, usec);
	}

 	if (unixpw && unixpw_in_progress && !uip0) {
		if (!unixpw_in_rfbPE) {
			rfbLog("rfbPE: got new client in non-rfbPE\n");
			;	/* this is new unixpw client  */
		}
 	}

	if (check_rate != 0) {
		if (check_rate < 0) {
			if (getenv("CHECK_RATE")) {
				check_rate = 1;
			} else {
				check_rate = 0;
			}
		}
		if (check_rate && !all_input && x11vnc_current < last_client + 45)  {
			check_allinput_rate();
		}
	}
	if (all_input) {
		do_allinput(usec);
	}
}

void rfbCFD(long usec) {
	int uip0 = unixpw_in_progress;
	if (! screen) {
		return;
	}
 	if (unixpw && unixpw_in_progress && !unixpw_in_rfbPE) {
		rfbLog("unixpw_in_rfbPE: skipping rfbCFD\n");
 		return;
 	}
	if (usec > USEC_MAX) {
		usec = USEC_MAX;
	}

	if (debug_tiles > 2) {
		double tm = dnow();
		fprintf(stderr, "rfbCFD(%d) t: %.4f\n",
		    (int) usec, tm - x11vnc_start);
	}

#if 0
fprintf(stderr, "handleEventsEagerly: %d\n", screen->handleEventsEagerly);
#endif

	if (! use_threads) {
		if (all_input) {
			do_allinput(usec);
		} else {
			/* XXX how for cmdline? */
			if (all_input) {
				screen->handleEventsEagerly = TRUE;
			} else {
				screen->handleEventsEagerly = FALSE;
			}
			rfbCheckFds(screen, usec);
		}
	}

 	if (unixpw && unixpw_in_progress && !uip0) {
		if (!unixpw_in_rfbPE) {
			rfbLog("rfbCFD: got new client in non-rfbPE\n");
			;	/* this is new unixpw client  */
		}
 	}
}

double rect_overlap(int x1, int y1, int x2, int y2, int X1, int Y1,
    int X2, int Y2) {
	double a, A, o;
	sraRegionPtr r, R;
	sraRectangleIterator *iter;
	sraRect rt;

	a = nabs((x2 - x1) * (y2 - y1));
	A = nabs((X2 - X1) * (Y2 - Y1));

	if (a == 0 || A == 0) {
		return 0.0;
	}

	r = sraRgnCreateRect(x1, y1, x2, y2);
	R = sraRgnCreateRect(X1, Y1, X2, Y2);

	sraRgnAnd(r, R);
	
	o = 0.0;
	iter = sraRgnGetIterator(r);
	while (sraRgnIteratorNext(iter, &rt)) {
		o += nabs( (rt.x2 - rt.x1) * (rt.y2 - rt.y1) );
	}
	sraRgnReleaseIterator(iter);

	sraRgnDestroy(r);
	sraRgnDestroy(R);

	if (a < A) {
		o = o/a;
	} else {
		o = o/A;
	}
	return o;
}

/*
 * choose a desktop name
 */
char *choose_title(char *display) {
	static char title[(MAXN+10)];	

	memset(title, 0, MAXN+10);
	strcpy(title, "x11vnc");

	if (display == NULL) {
		display = getenv("DISPLAY");
	}
	if (display == NULL) {
		return title;
	}
	title[0] = '\0';
	if (display[0] == ':') {
		char *th = this_host();
		if (th != NULL) {
			strncpy(title, th, MAXN - strlen(title));
		}
	}
	strncat(title, display, MAXN - strlen(title));
	if (subwin && dpy && valid_window(subwin, NULL, 0)) {
#if !NO_X11
		char *name = NULL;
		if (XFetchName(dpy, subwin, &name)) {
			if (name) {
				strncat(title, " ",  MAXN - strlen(title));
				strncat(title, name, MAXN - strlen(title));
				free(name);
			}
		}
#endif	/* NO_X11 */
	}
	return title;
}
