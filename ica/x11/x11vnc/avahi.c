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

/* -- avahi.c -- */

#include "x11vnc.h"
#include "connections.h"
#include "cleanup.h"

void avahi_initialise(void);
void avahi_advertise(char *name, char *host, uint16_t port);
void avahi_reset(void);
void avahi_cleanup(void);

static pid_t avahi_pid = 0;

static void kill_avahi_pid(void) {
	if (avahi_pid != 0) {
		rfbLog("kill_avahi_pid: %d\n", (int) avahi_pid);
		kill(avahi_pid, SIGTERM);
		avahi_pid = 0;
	}
}

static int try_avahi_helper(char *name, char *host, uint16_t port) {
#if LIBVNCSERVER_HAVE_FORK
	char *cmd, *p, *path = getenv("PATH"), portstr[32];
	int i;

	if (!name || !host || !port) {}

	/* avahi-publish */
	if (no_external_cmds || !cmd_ok("zeroconf")) {
		return 0;
	}

	if (!path) {
		return 0;
	}

	path = strdup(path); 
	cmd = (char *) malloc(strlen(path) + 100);
	sprintf(portstr, "%d", (int) port);

	p = strtok(path, ":");
	while (p) {
		struct stat sbuf;

		sprintf(cmd, "%s/avahi-publish", p);
		if (stat(cmd, &sbuf) == 0) {
			break;
		}
		sprintf(cmd, "%s/dns-sd", p);
		if (stat(cmd, &sbuf) == 0) {
			break;
		}
		sprintf(cmd, "%s/mDNS", p);
		if (stat(cmd, &sbuf) == 0) {
			break;
		}
		cmd[0] = '\0';

		p = strtok(NULL, ":");
	}
	free(path);

	if (!strcmp(cmd, "")) {
		free(cmd);
		rfbLog("Could not find an external avahi/zeroconf helper program.\n");
		return 0;
	}

	avahi_pid = fork();

	if (avahi_pid < 0) {
		rfbLogPerror("fork");
		avahi_pid = 0;
		free(cmd);
		return 0;
	}

	if (avahi_pid != 0) {
		int status;

		usleep(500 * 1000);
		waitpid(avahi_pid, &status, WNOHANG); 
		if (kill(avahi_pid, 0) != 0) {
			waitpid(avahi_pid, &status, WNOHANG); 
			avahi_pid = 0;
			free(cmd);
			return 0;
		}
		if (! quiet) {
			rfbLog("%s helper pid is: %d\n", cmd, (int) avahi_pid);
		}
		free(cmd);
		return 1;
	}

	for (i=3; i<256; i++) {
		close(i);
	}

	if (strstr(cmd, "/avahi-publish")) {
		execlp(cmd, cmd, "-s", name, "_rfb._tcp", portstr, (char *) NULL);
	} else {
		execlp(cmd, cmd, "-R", name, "_rfb._tcp", ".", portstr, (char *) NULL);
	}
	exit(1);
#else
	if (!name || !host || !port) {}
	return 0;
#endif
}

#if !defined(LIBVNCSERVER_HAVE_AVAHI) || !defined(LIBVNCSERVER_HAVE_LIBPTHREAD)
void avahi_initialise(void) {
	rfbLog("avahi_initialise: no Avahi support at buildtime.\n");
}

void avahi_advertise(char *name, char *host, uint16_t port) {
	char *t;
	t = getenv("X11VNC_AVAHI_NAME"); if (t) name = t;
	t = getenv("X11VNC_AVAHI_HOST"); if (t) host = t;
	t = getenv("X11VNC_AVAHI_PORT"); if (t) port = atoi(t);

	if (!try_avahi_helper(name, host, port)) {
		rfbLog("avahi_advertise:  no Avahi support at buildtime.\n");
		avahi = 0;
	}
}

void avahi_reset(void) {
	kill_avahi_pid();
	rfbLog("avahi_reset: no Avahi support at buildtime.\n");
}

void avahi_cleanup(void) {
	kill_avahi_pid();
	rfbLog("avahi_cleanup: no Avahi support at buildtime.\n");
}
#else

#include <avahi-common/thread-watch.h>
#include <avahi-common/alternative.h>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/malloc.h>
#include <avahi-common/error.h>


static AvahiThreadedPoll *_poll = NULL;
static AvahiClient *_client = NULL;
static AvahiEntryGroup *_group = NULL;

static int db = 0;

typedef struct {
	const char *name;
	const char *host;
	uint16_t port;
} avahi_service_t;

typedef struct {
	char *name;
	char *host;
	uint16_t port;
} avahi_reg_t;

#define NREG 16
static avahi_reg_t registered[NREG];

void avahi_initialise(void) {
	int ret;
	static int first = 1;

	if (getenv("AVAHI_DEBUG")) {
		db = 1;
	}
	if (first) {
		int i;
		for (i=0; i<NREG; i++) {
			registered[i].name = NULL;
			registered[i].host = NULL;
		}
		first = 0;
	}

if (db) fprintf(stderr, "in  avahi_initialise\n");
	if (_poll) {
if (db) fprintf(stderr, "    avahi_initialise: poll not null\n");
		return;
	}

	if (! (_poll = avahi_threaded_poll_new()) ) {
		rfbLog("warning: unable to open Avahi poll.\n");
		return;
	}

	_client = avahi_client_new(avahi_threaded_poll_get(_poll),
	    0, NULL, NULL, &ret);
	if (! _client) {
		rfbLog("warning: unable to open Avahi client: %s\n",
		    avahi_strerror(ret));

		avahi_threaded_poll_free(_poll);
		_poll = NULL;
		return;
	}

	if (avahi_threaded_poll_start(_poll) < 0) {
		rfbLog("warning: unable to start Avahi poll.\n");
		avahi_client_free(_client);
		_client = NULL;
		avahi_threaded_poll_free(_poll);
		_poll = NULL;
		return;
	}
if (db) fprintf(stderr, "out avahi_initialise\n");
}

static void _avahi_create_services(char *name, char *host,
    uint16_t port);

static void _avahi_entry_group_callback(AvahiEntryGroup *g,
    AvahiEntryGroupState state, void *userdata) {
	char *new_name;
	avahi_service_t *svc = (avahi_service_t *)userdata;

if (db) fprintf(stderr, "in  _avahi_entry_group_callback %d 0x%p\n", state, svc);
	if (g != _group && _group != NULL) {
		rfbLog("avahi_entry_group_callback fatal error (group).\n");
		clean_up_exit(1);
	}
	if (userdata == NULL) {
		rfbLog("avahi_entry_group_callback fatal error (userdata).\n");
		clean_up_exit(1);
	}

	switch(state) {
	case AVAHI_ENTRY_GROUP_ESTABLISHED:
		rfbLog("Avahi group %s established.\n", svc->name);
#if 0		/* is this the segv problem? */
		free(svc);
#endif
		break;
	case AVAHI_ENTRY_GROUP_COLLISION:
		new_name = avahi_alternative_service_name(svc->name);
		_avahi_create_services(new_name, svc->host, svc->port);
		rfbLog("Avahi Entry group collision\n");
		avahi_free(new_name);
		break;
	case AVAHI_ENTRY_GROUP_FAILURE:
		rfbLog("Avahi Entry group failure: %s\n",
		    avahi_strerror(avahi_client_errno(
		    avahi_entry_group_get_client(g))));
		break;
	default:
		break;
	}
if (db) fprintf(stderr, "out _avahi_entry_group_callback\n");
}

static void _avahi_create_services(char *name, char *host, uint16_t port) {
	avahi_service_t *svc = (avahi_service_t *)malloc(sizeof(avahi_service_t));
	int ret = 0;

if (db) fprintf(stderr, "in  _avahi_create_services  '%s' '%s' %d\n", name, host, port);
	svc->name = name;
	svc->host = host;
	svc->port = port;

	if (!_group) {
if (db) fprintf(stderr, "    _avahi_create_services create group\n");
		_group = avahi_entry_group_new(_client,
		    _avahi_entry_group_callback, svc);
	}
	if (!_group) {
		rfbLog("avahi_entry_group_new() failed: %s\n",
		    avahi_strerror(avahi_client_errno(_client)));
		return;
	}

	ret = avahi_entry_group_add_service(_group, AVAHI_IF_UNSPEC,
	    AVAHI_PROTO_UNSPEC, 0, name, "_rfb._tcp", NULL, NULL, port, NULL);
	if (ret < 0) {
		rfbLog("Failed to add _rfb._tcp service: %s\n",
		    avahi_strerror(ret));
		return;
	}

	ret = avahi_entry_group_commit(_group);
	if (ret < 0) {
		rfbLog("Failed to commit entry_group:: %s\n",
		    avahi_strerror(ret));
		return;
	}
if (db) fprintf(stderr, "out _avahi_create_services\n");
}

void avahi_advertise(char *name, char *host, uint16_t port) {
	int i;
	char *t;
	t = getenv("X11VNC_AVAHI_NAME"); if (t) name = t;
	t = getenv("X11VNC_AVAHI_HOST"); if (t) host = t;
	t = getenv("X11VNC_AVAHI_PORT"); if (t) port = atoi(t);

if (db) fprintf(stderr, "in  avahi_advertise: '%s' '%s' %d\n", name, host, port);
	if (!_client) {
if (db) fprintf(stderr, "    avahi_advertise client null\n");
		return;
	}
	if (_poll == NULL) {
		rfbLog("Avahi poll not initialized.\n");
		return;
	}
	/* well, we just track it ourselves... */
	for (i=0; i<NREG; i++) {
		if (!registered[i].name) {
			continue;
		}
		if (strcmp(registered[i].name, name)) {
			continue;
		}
		if (strcmp(registered[i].host, host)) {
			continue;
		}
		if (registered[i].port != port) {
			continue;
		}
if (db) fprintf(stderr, "    avahi_advertise already did this one\n");
		return;
	}
	for (i=0; i<NREG; i++) {
		if (!registered[i].name) {
			registered[i].name = strdup(name);
			registered[i].host = strdup(host);
			registered[i].port = port;
			break;
		}
	}

	avahi_threaded_poll_lock(_poll);
	_avahi_create_services(name, host, port >= 5900 ? port : 5900+port);
	avahi_threaded_poll_unlock(_poll);
if (db) fprintf(stderr, "out avahi_advertise\n");
}

void avahi_reset(void) {
	int i;
if (db) fprintf(stderr, "in  avahi_reset\n");
	for (i=0; i<NREG; i++) {
		if (registered[i].name) {
			free(registered[i].name);
			registered[i].name = NULL;
		}
		if (registered[i].host) {
			free(registered[i].host);
			registered[i].host = NULL;
		}
	}
	if (!_client || !_group) {
if (db) fprintf(stderr, "    avahi_reset client/group null\n");
		return;
	}
	avahi_entry_group_reset(_group);
	rfbLog("Avahi resetting group.\n");
if (db) fprintf(stderr, "out avahi_reset\n");
}

static void avahi_timeout (int sig) {
	rfbLog("sig: %d, avahi_cleanup timed out.\n", sig);
	exit(1);
}


void avahi_cleanup(void) {
if (db) fprintf(stderr, "in  avahi_cleanup\n");
	if (!_client) {
if (db) fprintf(stderr, "    avahi_cleanup client null\n");
		return;
	}
if (db) fprintf(stderr, "    avahi_cleanup poll_lock\n");
	avahi_threaded_poll_lock(_poll);
if (db) fprintf(stderr, "    avahi_cleanup poll_stop\n");

	signal(SIGALRM, avahi_timeout);
	alarm(3);
	avahi_threaded_poll_stop(_poll);
	alarm(0);
	signal(SIGALRM, SIG_DFL);

if (db) fprintf(stderr, "    avahi_cleanup client_free\n");
	avahi_client_free(_client);
	_client = NULL;

if (db) fprintf(stderr, "    avahi_cleanup poll_free\n");
	avahi_threaded_poll_free(_poll);
	_poll = NULL;
if (db) fprintf(stderr, "out avahi_cleanup\n");
}

#endif

