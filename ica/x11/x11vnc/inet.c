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
int ipv6_ip(char *host);
int dotted_ip(char *host, int partial);
int get_remote_port(int sock);
int get_local_port(int sock);
char *get_remote_host(int sock);
char *get_local_host(int sock);
char *ident_username(rfbClientPtr client);
int find_free_port(int start, int end);
int find_free_port6(int start, int end);
int have_ssh_env(void);
char *ipv6_getnameinfo(struct sockaddr *paddr, int addrlen);
char *ipv6_getipaddr(struct sockaddr *paddr, int addrlen);
int listen6(int port);
int listen_unix(char *file);
int accept_unix(int s);
int connect_tcp(char *host, int port);
int listen_tcp(int port, in_addr_t iface, int try6);

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

int ipv6_ip(char *host_in) {
	char *p, *host, a[2];
	int ncol = 0, nhex = 0;

	if (host_in[0] == '[')  {
		host = host_in + 1;
	} else {
		host = host_in;
	}

	if (strstr(host, "::ffff:") == host || strstr(host, "::FFFF:") == host) {
		return dotted_ip(host + strlen("::ffff:"), 0);
	}

	a[1] = '\0';

	p = host;
	while (*p != '\0' && *p != '%' && *p != ']') {
		if (*p == ':') {
			ncol++;
		} else {
			nhex++;
		}
		a[0] = *p;
		if (strpbrk(a, ":abcdef0123456789") == a) {
			p++;
			continue;
		}
		return 0;
	}
	if (ncol < 2 || ncol > 8 || nhex == 0) {
		return 0;
	} else {
		return 1;
	}
}

int dotted_ip(char *host, int partial) {
	int len, dots = 0;
	char *p = host;

	if (!host) {
		return 0;
	}

	if (!isdigit((unsigned char) host[0])) {
		return 0;
	}

	len = strlen(host);
	if (!partial && !isdigit((unsigned char) host[len-1])) {
		return 0;
	}

	while (*p != '\0') {
		if (*p == '.') dots++;
		if (*p == '.' || isdigit((unsigned char) (*p))) {
			p++;
			continue;
		}
		return 0;
	}
	if (!partial && dots != 3) {
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

			if ((sock = connect_tcp(client->host, 113)) < 0) {
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
		} else if ((sock = connect_tcp(client->host, 113)) < 0) {
			rfbLog("ident_username: could not connect to ident: %s:%d\n",
			    client->host, 113);
		} else {
			char msg[128];
			int ret;
			fd_set rfds;
			struct timeval tv;
			int rport = get_remote_port(client->sock);
			int lport = get_local_port(client->sock);

			sprintf(msg, "%d, %d\r\n", rport, lport);
			n = write(sock, msg, strlen(msg));

			FD_ZERO(&rfds);
			FD_SET(sock, &rfds);
			tv.tv_sec  = 3;
			tv.tv_usec = 0;
			ret = select(sock+1, &rfds, NULL, NULL, &tv); 

			if (ret > 0) {
				int i;
				char *q, *p;
				for (i=0; i < (int) sizeof(msg); i++) {
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
		int sock = listen_tcp(port, htonl(INADDR_ANY), 0);
		if (sock >= 0) {
			close(sock);
			return port;
		}
	}
	return 0;
}

int find_free_port6(int start, int end) {
	int port;
	if (start <= 0) {
		start = 1024;
	}
	if (end <= 0) {
		end = 65530;
	}
	for (port = start; port <= end; port++)  {
		int sock = listen6(port);
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

char *ipv6_getnameinfo(struct sockaddr *paddr, int addrlen) {
#if X11VNC_IPV6
	char name[200];
	if (noipv6) {
		return strdup("unknown");
	}
	if (getnameinfo(paddr, addrlen, name, sizeof(name), NULL, 0, 0) == 0) {
		return strdup(name);
	}
#endif
	return strdup("unknown");
}

char *ipv6_getipaddr(struct sockaddr *paddr, int addrlen) {
#if X11VNC_IPV6 && defined(NI_NUMERICHOST)
	char name[200];
	if (noipv6) {
		return strdup("unknown");
	}
	if (getnameinfo(paddr, addrlen, name, sizeof(name), NULL, 0, NI_NUMERICHOST) == 0) {
		return strdup(name);
	}
#endif
	return strdup("unknown");
}

int listen6(int port) {
#if X11VNC_IPV6
	struct sockaddr_in6 sin;
	int fd = -1, one = 1;

	if (noipv6) {
		return -1;
	}
	if (port <= 0 || 65535 < port) {
		/* for us, invalid port means do not listen. */
		return -1;
	}

	fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (fd < 0) {
		rfbLogPerror("listen6: socket");
		rfbLog("(Ignore the above error if this system is IPv4-only.)\n");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) < 0) {
		rfbLogPerror("listen6: setsockopt SO_REUSEADDR"); 
		close(fd);
		return -1;
	}

#if defined(SOL_IPV6) && defined(IPV6_V6ONLY)
	if (setsockopt(fd, SOL_IPV6, IPV6_V6ONLY, (char *)&one, sizeof(one)) < 0) {
		rfbLogPerror("listen6: setsockopt IPV6_V6ONLY"); 
		close(fd);
		return -1;
	}
#endif

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin6_family = AF_INET6;
	sin.sin6_port   = htons(port);
	sin.sin6_addr   = in6addr_any;

	if (listen_str6) {
		if (!strcmp(listen_str6, "localhost") || !strcmp(listen_str6, "::1")) {
			sin.sin6_addr = in6addr_loopback;
		} else {
			int err;
			struct addrinfo *ai;
			struct addrinfo hints;
			char service[32];

			memset(&hints, 0, sizeof(hints));
			sprintf(service, "%d", port);

			hints.ai_family = AF_INET6;
			hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
			hints.ai_flags |= AI_ADDRCONFIG;
#endif
#ifdef AI_NUMERICHOST
			if(ipv6_ip(listen_str6)) {
				hints.ai_flags |= AI_NUMERICHOST;
			}
#endif
#ifdef AI_NUMERICSERV
			hints.ai_flags |= AI_NUMERICSERV;
#endif
			err = getaddrinfo(listen_str6, service, &hints, &ai);
			if (err == 0) {
				struct addrinfo *ap = ai;
				err = 1;
				while (ap != NULL) {
					char *s = ipv6_getipaddr(ap->ai_addr, ap->ai_addrlen);
					if (!s) s = strdup("unknown");

					rfbLog("listen6: checking: %s family: %d\n", s, ap->ai_family); 
					if (ap->ai_family == AF_INET6) {
						memcpy((char *)&sin, ap->ai_addr, sizeof(sin));
						rfbLog("listen6: using:    %s scope_id: %d\n", s, sin.sin6_scope_id); 
						err = 0;
						free(s);
						break;
					}
					free(s);
					ap = ap->ai_next;
				}
				freeaddrinfo(ai);
			}

			if (err != 0) {
				rfbLog("Invalid or Unsupported -listen6 string: %s\n", listen_str6);
				close(fd);
				return -1;
			}
		}
	} else if (allow_list && !strcmp(allow_list, "127.0.0.1")) {
		sin.sin6_addr = in6addr_loopback;
	} else if (listen_str) {
		if (!strcmp(listen_str, "localhost")) {
			sin.sin6_addr = in6addr_loopback;
		}
	}

	if (bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		rfbLogPerror("listen6: bind"); 
		close(fd);
		return -1;
	}
	if (listen(fd, 32) < 0) {
		rfbLogPerror("listen6: listen");
		close(fd);
		return -1;
	}
	return fd;
#else
	if (port) {}
	return -1;
#endif
}

#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/un.h>
#endif

int listen_unix(char *file) {
#if !defined(AF_UNIX) || !defined(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
	if (sock) {}
	return -1;
#else
	int s, len;
	struct sockaddr_un saun;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		rfbLogPerror("listen_unix: socket");
		return -1;
	}
	saun.sun_family = AF_UNIX;
	strcpy(saun.sun_path, file);
	unlink(file);

	len = sizeof(saun.sun_family) + strlen(saun.sun_path);

	if (bind(s, (struct sockaddr *)&saun, len) < 0) {
		rfbLogPerror("listen_unix: bind");
		close(s);
		return -1;
	}

	if (listen(s, 32) < 0) {
		rfbLogPerror("listen_unix: listen");
		close(s);
		return -1;
	}
	rfbLog("listening on unix socket: %s fd=%d\n", file, s);
	return s;
#endif
}

int accept_unix(int s) {
#if !defined(AF_UNIX) || !defined(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
	if (s) {}
	return -1;
#else
	int fd, fromlen;
	struct sockaddr_un fsaun;

	fd = accept(s, (struct sockaddr *)&fsaun, &fromlen);
	if (fd < 0) {
		rfbLogPerror("accept_unix: accept");
		return -1;
	}
	return fd;
#endif
}

int connect_tcp(char *host, int port) {
	double t0 = dnow();
	int fd = -1;
	int fail4 = noipv4;
	if (getenv("IPV4_FAILS")) {
		fail4 = 2;
	}

	rfbLog("connect_tcp: trying:   %s %d\n", host, port);

	if (fail4) {
		if (fail4 > 1) {
			rfbLog("TESTING: IPV4_FAILS for connect_tcp.\n");
		}
	} else {
		fd = rfbConnectToTcpAddr(host, port);
	}

	if (fd >= 0) {
		return fd;
	}
	rfbLogPerror("connect_tcp: connection failed");

	if (dnow() - t0 < 4.0) {
		rfbLog("connect_tcp: re-trying %s %d\n", host, port);
		usleep (100 * 1000);
		if (!fail4) {
			fd = rfbConnectToTcpAddr(host, port);
		}
		if (fd < 0) {
			rfbLogPerror("connect_tcp: connection failed");
		}
	}

	if (fd < 0 && !noipv6) {
#if X11VNC_IPV6
		int err;
		struct addrinfo *ai;
		struct addrinfo hints;
		char service[32], *host2, *q;

		rfbLog("connect_tcp: trying IPv6 %s %d\n", host, port);

		memset(&hints, 0, sizeof(hints));
		sprintf(service, "%d", port);

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
		hints.ai_flags |= AI_ADDRCONFIG;
#endif
		if(ipv6_ip(host)) {
#ifdef AI_NUMERICHOST
			rfbLog("connect_tcp[ipv6]: setting AI_NUMERICHOST for %s\n", host);
			hints.ai_flags |= AI_NUMERICHOST;
#endif
		}
#ifdef AI_NUMERICSERV
		hints.ai_flags |= AI_NUMERICSERV;
#endif

		if (!strcmp(host, "127.0.0.1")) {
			host2 = strdup("::1");
		} else if (host[0] == '[') {
			host2 = strdup(host+1);
		} else {
			host2 = strdup(host);
		}
		q = strrchr(host2, ']');
		if (q) {
			*q = '\0';
		}

		err = getaddrinfo(host2, service, &hints, &ai);
		if (err != 0) {
			rfbLog("connect_tcp[ipv6]: getaddrinfo[%d]: %s\n", err, gai_strerror(err));
			usleep(100 * 1000);
			err = getaddrinfo(host2, service, &hints, &ai);
		}
		free(host2);

		if (err != 0) {
			rfbLog("connect_tcp[ipv6]: getaddrinfo[%d]: %s\n", err, gai_strerror(err));
		} else {
			struct addrinfo *ap = ai;
			while (ap != NULL) {
				int sock;

				if (fail4) {
					struct sockaddr_in6 *s6ptr;
					if (ap->ai_family != AF_INET6) {
						rfbLog("connect_tcp[ipv6]: skipping AF_INET address under -noipv4\n");
						ap = ap->ai_next;
						continue;
					}
#ifdef IN6_IS_ADDR_V4MAPPED
					s6ptr = (struct sockaddr_in6 *) ap->ai_addr;
					if (IN6_IS_ADDR_V4MAPPED(&(s6ptr->sin6_addr))) {
						rfbLog("connect_tcp[ipv6]: skipping V4MAPPED address under -noipv4\n");
						ap = ap->ai_next;
						continue;
					}
#endif
				}

				sock = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);

				if (sock == -1) {
					rfbLogPerror("connect_tcp[ipv6]: socket");
					if (0) rfbLog("(Ignore the above error if this system is IPv4-only.)\n");
				} else {
					int res = -1, dmsg = 0;
					char *s = ipv6_getipaddr(ap->ai_addr, ap->ai_addrlen);
					if (!s) s = strdup("unknown");

					rfbLog("connect_tcp[ipv6]: trying sock=%d fam=%d proto=%d using %s\n",
					    sock, ap->ai_family, ap->ai_protocol, s);
					res = connect(sock, ap->ai_addr, ap->ai_addrlen);
#if defined(SOL_IPV6) && defined(IPV6_V6ONLY)
					if (res != 0) {
						int zero = 0;
						rfbLogPerror("connect_tcp[ipv6]: connect");
						dmsg = 1;
						if (setsockopt(sock, SOL_IPV6, IPV6_V6ONLY, (char *)&zero, sizeof(zero)) == 0) {
							rfbLog("connect_tcp[ipv6]: trying again with IPV6_V6ONLY=0\n");
							res = connect(sock, ap->ai_addr, ap->ai_addrlen);
							dmsg = 0;
						} else {
							rfbLogPerror("connect_tcp[ipv6]: setsockopt IPV6_V6ONLY");
						}
					}
#endif
					if (res == 0) {
						rfbLog("connect_tcp[ipv6]: connect OK\n");
						fd = sock;
						if (!ipv6_client_ip_str) {
							ipv6_client_ip_str = strdup(s);
						}
						free(s);
						break;
					} else {
						if (!dmsg) rfbLogPerror("connect_tcp[ipv6]: connect");
						close(sock);
					}
					free(s);
				}
				ap = ap->ai_next;
			}
			freeaddrinfo(ai);
		}
#endif
	}
	if (fd < 0 && !fail4) {
		/* this is a kludge for IPv4-only machines getting v4mapped string. */
		char *q, *host2;
		if (host[0] == '[') {
			host2 = strdup(host+1);
		} else {
			host2 = strdup(host);
		}
		q = strrchr(host2, ']');
		if (q) {
			*q = '\0';
		}
		if (strstr(host2, "::ffff:") == host2 || strstr(host2, "::FFFF:") == host2) {
			char *host3 = host2 + strlen("::ffff:");
			if (dotted_ip(host3, 0)) {
				rfbLog("connect_tcp[ipv4]: trying fallback to IPv4 for %s\n", host2);
				fd = rfbConnectToTcpAddr(host3, port);
				if (fd < 0) {
					rfbLogPerror("connect_tcp[ipv4]: connection failed");
				}
			}
		}
		free(host2);
	}
	return fd;
}

int listen_tcp(int port, in_addr_t iface, int try6) {
	int fd = -1;
	int fail4 = noipv4;
	if (getenv("IPV4_FAILS")) {
		fail4 = 2;
	}

	if (port <= 0 || 65535 < port) {
		/* for us, invalid port means do not listen. */
		return -1;
	}

	if (fail4) {
		if (fail4 > 1) {
			rfbLog("TESTING: IPV4_FAILS for listen_tcp: port=%d try6=%d\n", port, try6);
		}
	} else {
		fd = rfbListenOnTCPPort(port, iface);
	}

	if (fd >= 0) {
		return fd;
	}
	if (fail4 > 1) {
		rfbLogPerror("listen_tcp: listen failed");
	}

	if (fd < 0 && try6 && ipv6_listen && !noipv6) {
#if X11VNC_IPV6
		char *save = listen_str6;
		if (iface == htonl(INADDR_LOOPBACK)) {
			listen_str6 = "localhost";
			rfbLog("listen_tcp: retrying on IPv6 in6addr_loopback ...\n");
			fd = listen6(port);
		} else if (iface == htonl(INADDR_ANY)) {
			listen_str6 = NULL;
			rfbLog("listen_tcp: retrying on IPv6 in6addr_any ...\n");
			fd = listen6(port);
		}
		listen_str6 = save;
#endif
	}
	return fd;
}

