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

/* -- inet.c -- */

#include "x11vnc.h"
#include "unixpw.h"
#include "sslhelper.h"

/*
 * Simple utility to map host name to dotted IP address.  Ignores aliases.
 * Up to caller to free returned string.
 */
char *host2ip(char *host);
char *raw2host(char *raw, int len);
char *raw2ip(char *raw);
char *ip2host(char *ip);
int dotted_ip(char *host);
int get_remote_port(int sock);
int get_local_port(int sock);
char *get_remote_host(int sock);
char *get_local_host(int sock);
char *ident_username(rfbClientPtr client);
int find_free_port(int start, int end);
int have_ssh_env(void);

static int get_port(int sock, int remote);
static char *get_host(int sock, int remote);

char *host2ip(char *host) {
	struct hostent *hp;
	struct sockaddr_in addr;
	char *str;

	if (! host_lookup) {
		return NULL;
	}

	hp = gethostbyname(host);
	if (!hp) {
		return NULL;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr =  *(unsigned long *)hp->h_addr;
	str = strdup(inet_ntoa(addr.sin_addr));
	return str;
}

char *raw2host(char *raw, int len) {
	char *str;
#if LIBVNCSERVER_HAVE_NETDB_H && LIBVNCSERVER_HAVE_NETINET_IN_H
	struct hostent *hp;

	if (! host_lookup) {
		return strdup("unknown");
	}

	hp = gethostbyaddr(raw, len, AF_INET);
	if (!hp) {
		return strdup(inet_ntoa(*((struct in_addr *)raw)));
	}
	str = strdup(hp->h_name);
#else
	str = strdup("unknown");
#endif
	return str;
}

char *raw2ip(char *raw) {
	return strdup(inet_ntoa(*((struct in_addr *)raw)));
}

char *ip2host(char *ip) {
	char *str;
#if LIBVNCSERVER_HAVE_NETDB_H && LIBVNCSERVER_HAVE_NETINET_IN_H
	struct hostent *hp;
	in_addr_t iaddr;

	if (! host_lookup) {
		return strdup("unknown");
	}

	iaddr = inet_addr(ip);
	if (iaddr == htonl(INADDR_NONE)) {
		return strdup("unknown");
	}

	hp = gethostbyaddr((char *)&iaddr, sizeof(in_addr_t), AF_INET);
	if (!hp) {
		return strdup("unknown");
	}
	str = strdup(hp->h_name);
#else
	str = strdup("unknown");
#endif
	return str;
}

int dotted_ip(char *host) {
	char *p = host;
	while (*p != '\0') {
		if (*p == '.' || isdigit((unsigned char) (*p))) {
			p++;
			continue;
		}
		return 0;
	}
	return 1;
}

static int get_port(int sock, int remote) {
	struct sockaddr_in saddr;
	unsigned int saddr_len;
	int saddr_port;
	
	saddr_len = sizeof(saddr);
	memset(&saddr, 0, sizeof(saddr));
	saddr_port = -1;
	if (remote) {
		if (!getpeername(sock, (struct sockaddr *)&saddr, &saddr_len)) {
			saddr_port = ntohs(saddr.sin_port);
		}
	} else {
		if (!getsockname(sock, (struct sockaddr *)&saddr, &saddr_len)) {
			saddr_port = ntohs(saddr.sin_port);
		}
	}
	return saddr_port;
}

int get_remote_port(int sock) {
	return get_port(sock, 1);
}

int get_local_port(int sock) {
	return get_port(sock, 0);
}

static char *get_host(int sock, int remote) {
	struct sockaddr_in saddr;
	unsigned int saddr_len;
	int saddr_port;
	char *saddr_ip_str = NULL;
	
	saddr_len = sizeof(saddr);
	memset(&saddr, 0, sizeof(saddr));
	saddr_port = -1;
#if LIBVNCSERVER_HAVE_NETINET_IN_H
	if (remote) {
		if (!getpeername(sock, (struct sockaddr *)&saddr, &saddr_len)) {
			saddr_ip_str = inet_ntoa(saddr.sin_addr);
		}
	} else {
		if (!getsockname(sock, (struct sockaddr *)&saddr, &saddr_len)) {
			saddr_ip_str = inet_ntoa(saddr.sin_addr);
		}
	}
#endif
	if (! saddr_ip_str) {
		saddr_ip_str = "unknown";
	}
	return strdup(saddr_ip_str);
}

char *get_remote_host(int sock) {
	return get_host(sock, 1);
}

char *get_local_host(int sock) {
	return get_host(sock, 0);
}

char *ident_username(rfbClientPtr client) {
	ClientData *cd = (ClientData *) client->clientData;
	char *str, *newhost, *user = NULL, *newuser = NULL;
	int len;

	if (cd) {
		user = cd->username;
	}
	if (!user || *user == '\0') {
		char msg[128];
		int n, sock, ok = 0;
		int block = 0;
		int refused = 0;

		/*
		 * need to check to see if the operation will block for
		 * a long time: a firewall may just ignore our packets.
		 */
#if LIBVNCSERVER_HAVE_FORK
	    {	pid_t pid, pidw;
		int rc;
		if ((pid = fork()) > 0) {
			usleep(100 * 1000);	/* 0.1 sec for quick success or refusal */
			pidw = waitpid(pid, &rc, WNOHANG);
			if (pidw <= 0) {
				usleep(1500 * 1000);	/* 1.5 sec */
				pidw = waitpid(pid, &rc, WNOHANG);
				if (pidw <= 0) {
					int rc2;
					rfbLog("ident_username: set block=1 (hung)\n");
					block = 1;
					kill(pid, SIGTERM);
					usleep(100 * 1000);
					waitpid(pid, &rc2, WNOHANG);
				}
			}
			if (pidw > 0 && !block) {
				if (WIFEXITED(rc) && WEXITSTATUS(rc) == 1) {
					rfbLog("ident_username: set refused=1 (exit)\n");
					refused = 1;
				}
			}
		} else if (pid == -1) {
			;
		} else {
			/* child */
			signal(SIGHUP,  SIG_DFL);
			signal(SIGINT,  SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);

			if ((sock = rfbConnectToTcpAddr(client->host, 113)) < 0) {
				exit(1);
			} else {
				close(sock);
				exit(0);
			}
		}
	    }
#endif
		if (block || refused) {
			;
		} else if ((sock = rfbConnectToTcpAddr(client->host, 113)) < 0) {
			rfbLog("ident_username: could not connect to ident: %s:%d\n",
			    client->host, 113);
		} else {
			int ret;
			fd_set rfds;
			struct timeval tv;
			int rport = get_remote_port(client->sock);
			int lport = get_local_port(client->sock);

			sprintf(msg, "%d, %d\r\n", rport, lport);
			n = write(sock, msg, strlen(msg));

			FD_ZERO(&rfds);
			FD_SET(sock, &rfds);
			tv.tv_sec  = 4;
			tv.tv_usec = 0;
			ret = select(sock+1, &rfds, NULL, NULL, &tv); 

			if (ret > 0) {
				int i;
				char *q, *p;
				for (i=0; i<128; i++) {
					msg[i] = '\0';
				}
				usleep(250*1000);
				n = read(sock, msg, 127);
				close(sock);
				if (n <= 0) goto badreply;

				/* 32782 , 6000 : USERID : UNIX :runge */
				q = strstr(msg, "USERID");
				if (!q) goto badreply;
				q = strstr(q, ":");
				if (!q) goto badreply;
				q++;
				q = strstr(q, ":");
				if (!q) goto badreply;
				q++;
				q = lblanks(q);
				p = q;
				while (*p) {
					if (*p == '\r' || *p == '\n') {
						*p = '\0';
					}
					p++;
				}
				ok = 1;
				if (strlen(q) > 24) {
					*(q+24) = '\0';
				}
				newuser = strdup(q);

				badreply:
				n = 0;	/* avoid syntax error */
			} else {
				close(sock);
			}
		}
		if (! ok || !newuser) {
			newuser = strdup("unknown-user");
		}
		if (cd) {
			if (cd->username) {
				free(cd->username);
			}
			cd->username = newuser;
		}
		user = newuser;
	}
	if (!strcmp(user, "unknown-user") && cd && cd->unixname[0] != '\0') {
		user = cd->unixname;
	}
	if (unixpw && openssl_last_ip && strstr("UNIX:", user) != user) {
		newhost = ip2host(openssl_last_ip);
	} else {
		newhost = ip2host(client->host);
	}
	len = strlen(user) + 1 + strlen(newhost) + 1;
	str = (char *) malloc(len);
	sprintf(str, "%s@%s", user, newhost);
	free(newhost);
	return str;
}

int find_free_port(int start, int end) {
	int port;
	if (start <= 0) {
		start = 1024;
	}
	if (end <= 0) {
		end = 65530;
	}
	for (port = start; port <= end; port++)  {
		int sock = rfbListenOnTCPPort(port, htonl(INADDR_ANY));
		if (sock >= 0) {
			close(sock);
			return port;
		}
	}
	return 0;
}

int have_ssh_env(void) {
	char *str, *p = getenv("SSH_CONNECTION");
	char *rhost, *rport, *lhost, *lport;
	
	if (! p) {
		char *q = getenv("SSH_CLIENT");
		if (! q) {
			return 0;
		}
		if (strstr(q, "127.0.0.1") != NULL) {
			return 0;
		}
		return 1;
	}

	if (strstr(p, "127.0.0.1") != NULL) {
		return 0;
	}

	str = strdup(p);
	
	p = strtok(str, " ");
	rhost = p;

	p = strtok(NULL, " ");
	if (! p) goto fail;

	rport = p;

	p = strtok(NULL, " ");
	if (! p) goto fail;

	lhost = p;
	
	p = strtok(NULL, " ");
	if (! p) goto fail;

	lport = p;

if (0) fprintf(stderr, "%d/%d - '%s' '%s'\n", atoi(rport), atoi(lport), rhost, lhost);

	if (atoi(rport) <= 16 || atoi(rport) > 65535) {
		goto fail;
	}
	if (atoi(lport) <= 16 || atoi(lport) > 65535) {
		goto fail;
	}

	if (!strcmp(rhost, lhost)) {
		goto fail;
	}

	free(str);

	return 1;
	
	fail:

	free(str);

	return 0;
}

