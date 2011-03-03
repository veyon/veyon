/*
 * httpd.c - a simple HTTP server
 */

/*
 *  Copyright (C) 2002 RealVNC Ltd.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

#include <ctype.h>
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>

#ifdef WIN32
#include <winsock.h>
#define close closesocket
#else
#ifdef LIBVNCSERVER_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <pwd.h>
#endif

#ifdef USE_LIBWRAP
#include <tcpd.h>
#endif

#define connection_close
#ifndef connection_close

#define NOT_FOUND_STR "HTTP/1.0 404 Not found\r\n\r\n" \
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n" \
    "<BODY><H1>File Not Found</H1></BODY>\n"

#define INVALID_REQUEST_STR "HTTP/1.0 400 Invalid Request\r\n\r\n" \
    "<HEAD><TITLE>Invalid Request</TITLE></HEAD>\n" \
    "<BODY><H1>Invalid request</H1></BODY>\n"

#define OK_STR "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"

#else

#define NOT_FOUND_STR "HTTP/1.0 404 Not found\r\nConnection: close\r\n\r\n" \
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n" \
    "<BODY><H1>File Not Found</H1></BODY>\n"

#define INVALID_REQUEST_STR "HTTP/1.0 400 Invalid Request\r\nConnection: close\r\n\r\n" \
    "<HEAD><TITLE>Invalid Request</TITLE></HEAD>\n" \
    "<BODY><H1>Invalid request</H1></BODY>\n"

#define OK_STR "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"

#endif

static void httpProcessInput(rfbScreenInfoPtr screen);
static rfbBool compareAndSkip(char **ptr, const char *str);
static rfbBool parseParams(const char *request, char *result, int max_bytes);
static rfbBool validateString(char *str);

#define BUF_SIZE 32768

static char buf[BUF_SIZE];
static size_t buf_filled=0;

/*
 * httpInitSockets sets up the TCP socket to listen for HTTP connections.
 */

void
rfbHttpInitSockets(rfbScreenInfoPtr rfbScreen)
{
    if (rfbScreen->httpInitDone)
	return;

    rfbScreen->httpInitDone = TRUE;

    if (!rfbScreen->httpDir)
	return;

    if (rfbScreen->httpPort == 0) {
	rfbScreen->httpPort = rfbScreen->port-100;
    }

    rfbLog("Listening for HTTP connections on TCP port %d\n", rfbScreen->httpPort);

    rfbLog("  URL http://%s:%d\n",rfbScreen->thisHost,rfbScreen->httpPort);

    if ((rfbScreen->httpListenSock =
      rfbListenOnTCPPort(rfbScreen->httpPort, rfbScreen->listenInterface)) < 0) {
	rfbLogPerror("ListenOnTCPPort");
	return;
    }

   /*AddEnabledDevice(httpListenSock);*/
}

void rfbHttpShutdownSockets(rfbScreenInfoPtr rfbScreen) {
    if(rfbScreen->httpSock>-1) {
	close(rfbScreen->httpSock);
	FD_CLR(rfbScreen->httpSock,&rfbScreen->allFds);
	rfbScreen->httpSock=-1;
    }
}

/*
 * httpCheckFds is called from ProcessInputEvents to check for input on the
 * HTTP socket(s).  If there is input to process, httpProcessInput is called.
 */

void
rfbHttpCheckFds(rfbScreenInfoPtr rfbScreen)
{
    int nfds;
    fd_set fds;
    struct timeval tv;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (!rfbScreen->httpDir)
	return;

    if (rfbScreen->httpListenSock < 0)
	return;

    FD_ZERO(&fds);
    FD_SET(rfbScreen->httpListenSock, &fds);
    if (rfbScreen->httpSock >= 0) {
	FD_SET(rfbScreen->httpSock, &fds);
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    nfds = select(max(rfbScreen->httpSock,rfbScreen->httpListenSock) + 1, &fds, NULL, NULL, &tv);
    if (nfds == 0) {
	return;
    }
    if (nfds < 0) {
#ifdef WIN32
		errno = WSAGetLastError();
#endif
	if (errno != EINTR)
		rfbLogPerror("httpCheckFds: select");
	return;
    }

    if ((rfbScreen->httpSock >= 0) && FD_ISSET(rfbScreen->httpSock, &fds)) {
	httpProcessInput(rfbScreen);
    }

    if (FD_ISSET(rfbScreen->httpListenSock, &fds)) {
	if (rfbScreen->httpSock >= 0) close(rfbScreen->httpSock);

	if ((rfbScreen->httpSock = accept(rfbScreen->httpListenSock,
			       (struct sockaddr *)&addr, &addrlen)) < 0) {
	    rfbLogPerror("httpCheckFds: accept");
	    return;
	}
#ifdef USE_LIBWRAP
	if(!hosts_ctl("vnc",STRING_UNKNOWN,inet_ntoa(addr.sin_addr),
		      STRING_UNKNOWN)) {
	  rfbLog("Rejected HTTP connection from client %s\n",
		 inet_ntoa(addr.sin_addr));
	  close(rfbScreen->httpSock);
	  rfbScreen->httpSock=-1;
	  return;
	}
#endif
        if(!rfbSetNonBlocking(rfbScreen->httpSock)) {
	    close(rfbScreen->httpSock);
	    rfbScreen->httpSock=-1;
	    return;
	}
	/*AddEnabledDevice(httpSock);*/
    }
}


static void
httpCloseSock(rfbScreenInfoPtr rfbScreen)
{
    close(rfbScreen->httpSock);
    rfbScreen->httpSock = -1;
    buf_filled = 0;
}

static rfbClientRec cl;

/*
 * httpProcessInput is called when input is received on the HTTP socket.
 */

static void
httpProcessInput(rfbScreenInfoPtr rfbScreen)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char fullFname[512];
    char params[1024];
    char *ptr;
    char *fname;
    unsigned int maxFnameLen;
    FILE* fd;
    rfbBool performSubstitutions = FALSE;
    char str[256+32];
#ifndef WIN32
    char* user=getenv("USER");
#endif
   
    cl.sock=rfbScreen->httpSock;

    if (strlen(rfbScreen->httpDir) > 255) {
	rfbErr("-httpd directory too long\n");
	httpCloseSock(rfbScreen);
	return;
    }
    strcpy(fullFname, rfbScreen->httpDir);
    fname = &fullFname[strlen(fullFname)];
    maxFnameLen = 511 - strlen(fullFname);

    buf_filled=0;

    /* Read data from the HTTP client until we get a complete request. */
    while (1) {
	ssize_t got;

        if (buf_filled > sizeof (buf)) {
	    rfbErr("httpProcessInput: HTTP request is too long\n");
	    httpCloseSock(rfbScreen);
	    return;
	}

	got = read (rfbScreen->httpSock, buf + buf_filled,
			    sizeof (buf) - buf_filled - 1);

	if (got <= 0) {
	    if (got == 0) {
		rfbErr("httpd: premature connection close\n");
	    } else {
#ifdef WIN32
	        errno=WSAGetLastError();
#endif
		if (errno == EAGAIN) {
		    return;
		}
		rfbLogPerror("httpProcessInput: read");
	    }
	    httpCloseSock(rfbScreen);
	    return;
	}

	buf_filled += got;
	buf[buf_filled] = '\0';

	/* Is it complete yet (is there a blank line)? */
	if (strstr (buf, "\r\r") || strstr (buf, "\n\n") ||
	    strstr (buf, "\r\n\r\n") || strstr (buf, "\n\r\n\r"))
	    break;
    }


    /* Process the request. */
    if(rfbScreen->httpEnableProxyConnect) {
	const static char* PROXY_OK_STR = "HTTP/1.0 200 OK\r\nContent-Type: octet-stream\r\nPragma: no-cache\r\n\r\n";
	if(!strncmp(buf, "CONNECT ", 8)) {
	    if(atoi(strchr(buf, ':')+1)!=rfbScreen->port) {
		rfbErr("httpd: CONNECT format invalid.\n");
		rfbWriteExact(&cl,INVALID_REQUEST_STR, strlen(INVALID_REQUEST_STR));
		httpCloseSock(rfbScreen);
		return;
	    }
	    /* proxy connection */
	    rfbLog("httpd: client asked for CONNECT\n");
	    rfbWriteExact(&cl,PROXY_OK_STR,strlen(PROXY_OK_STR));
	    rfbNewClientConnection(rfbScreen,rfbScreen->httpSock);
	    rfbScreen->httpSock = -1;
	    return;
	}
	if (!strncmp(buf, "GET ",4) && !strncmp(strchr(buf,'/'),"/proxied.connection HTTP/1.", 27)) {
	    /* proxy connection */
	    rfbLog("httpd: client asked for /proxied.connection\n");
	    rfbWriteExact(&cl,PROXY_OK_STR,strlen(PROXY_OK_STR));
	    rfbNewClientConnection(rfbScreen,rfbScreen->httpSock);
	    rfbScreen->httpSock = -1;
	    return;
	}	   
    }

    if (strncmp(buf, "GET ", 4)) {
	rfbErr("httpd: no GET line\n");
	httpCloseSock(rfbScreen);
	return;
    } else {
	/* Only use the first line. */
	buf[strcspn(buf, "\n\r")] = '\0';
    }

    if (strlen(buf) > maxFnameLen) {
	rfbErr("httpd: GET line too long\n");
	httpCloseSock(rfbScreen);
	return;
    }

    if (sscanf(buf, "GET %s HTTP/1.", fname) != 1) {
	rfbErr("httpd: couldn't parse GET line\n");
	httpCloseSock(rfbScreen);
	return;
    }

    if (fname[0] != '/') {
	rfbErr("httpd: filename didn't begin with '/'\n");
	rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock(rfbScreen);
	return;
    }

    if (strchr(fname+1, '/') != NULL) {
	rfbErr("httpd: asking for file in other directory\n");
	rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock(rfbScreen);
	return;
    }

    getpeername(rfbScreen->httpSock, (struct sockaddr *)&addr, &addrlen);
    rfbLog("httpd: get '%s' for %s\n", fname+1,
	   inet_ntoa(addr.sin_addr));

    /* Extract parameters from the URL string if necessary */

    params[0] = '\0';
    ptr = strchr(fname, '?');
    if (ptr != NULL) {
       *ptr = '\0';
       if (!parseParams(&ptr[1], params, 1024)) {
           params[0] = '\0';
           rfbErr("httpd: bad parameters in the URL\n");
       }
    }


    /* If we were asked for '/', actually read the file index.vnc */

    if (strcmp(fname, "/") == 0) {
	strcpy(fname, "/index.vnc");
	rfbLog("httpd: defaulting to '%s'\n", fname+1);
    }

    /* Substitutions are performed on files ending .vnc */

    if (strlen(fname) >= 4 && strcmp(&fname[strlen(fname)-4], ".vnc") == 0) {
	performSubstitutions = TRUE;
    }

    /* Open the file */

    if ((fd = fopen(fullFname, "r")) == 0) {
        rfbLogPerror("httpProcessInput: open");
        rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
        httpCloseSock(rfbScreen);
        return;
    }

    rfbWriteExact(&cl, OK_STR, strlen(OK_STR));

    while (1) {
	int n = fread(buf, 1, BUF_SIZE-1, fd);
	if (n < 0) {
	    rfbLogPerror("httpProcessInput: read");
	    fclose(fd);
	    httpCloseSock(rfbScreen);
	    return;
	}

	if (n == 0)
	    break;

	if (performSubstitutions) {

	    /* Substitute $WIDTH, $HEIGHT, etc with the appropriate values.
	       This won't quite work properly if the .vnc file is longer than
	       BUF_SIZE, but it's reasonable to assume that .vnc files will
	       always be short. */

	    char *ptr = buf;
	    char *dollar;
	    buf[n] = 0; /* make sure it's null-terminated */

	    while ((dollar = strchr(ptr, '$'))!=NULL) {
		rfbWriteExact(&cl, ptr, (dollar - ptr));

		ptr = dollar;

		if (compareAndSkip(&ptr, "$WIDTH")) {

		    sprintf(str, "%d", rfbScreen->width);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$HEIGHT")) {

		    sprintf(str, "%d", rfbScreen->height);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETWIDTH")) {

		    sprintf(str, "%d", rfbScreen->width);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETHEIGHT")) {

		    sprintf(str, "%d", rfbScreen->height + 32);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$PORT")) {

		    sprintf(str, "%d", rfbScreen->port);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$DESKTOP")) {

		    rfbWriteExact(&cl, rfbScreen->desktopName, strlen(rfbScreen->desktopName));

		} else if (compareAndSkip(&ptr, "$DISPLAY")) {

		    sprintf(str, "%s:%d", rfbScreen->thisHost, rfbScreen->port-5900);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$USER")) {
#ifndef WIN32
		    if (user) {
			rfbWriteExact(&cl, user,
				   strlen(user));
		    } else
#endif
			rfbWriteExact(&cl, "?", 1);
		} else if (compareAndSkip(&ptr, "$PARAMS")) {
		    if (params[0] != '\0')
			rfbWriteExact(&cl, params, strlen(params));
		} else {
		    if (!compareAndSkip(&ptr, "$$"))
			ptr++;

		    if (rfbWriteExact(&cl, "$", 1) < 0) {
			fclose(fd);
			httpCloseSock(rfbScreen);
			return;
		    }
		}
	    }
	    if (rfbWriteExact(&cl, ptr, (&buf[n] - ptr)) < 0)
		break;

	} else {

	    /* For files not ending .vnc, just write out the buffer */

	    if (rfbWriteExact(&cl, buf, n) < 0)
		break;
	}
    }

    fclose(fd);
    httpCloseSock(rfbScreen);
}


static rfbBool
compareAndSkip(char **ptr, const char *str)
{
    if (strncmp(*ptr, str, strlen(str)) == 0) {
	*ptr += strlen(str);
	return TRUE;
    }

    return FALSE;
}

/*
 * Parse the request tail after the '?' character, and format a sequence
 * of <param> tags for inclusion into an HTML page with embedded applet.
 */

static rfbBool
parseParams(const char *request, char *result, int max_bytes)
{
    char param_request[128];
    char param_formatted[196];
    const char *tail;
    char *delim_ptr;
    char *value_str;
    int cur_bytes, len;

    result[0] = '\0';
    cur_bytes = 0;

    tail = request;
    for (;;) {
	/* Copy individual "name=value" string into a buffer */
	delim_ptr = strchr((char *)tail, '&');
	if (delim_ptr == NULL) {
	    if (strlen(tail) >= sizeof(param_request)) {
		return FALSE;
	    }
	    strcpy(param_request, tail);
	} else {
	    len = delim_ptr - tail;
	    if (len >= sizeof(param_request)) {
		return FALSE;
	    }
	    memcpy(param_request, tail, len);
	    param_request[len] = '\0';
	}

	/* Split the request into parameter name and value */
	value_str = strchr(&param_request[1], '=');
	if (value_str == NULL) {
	    return FALSE;
	}
	*value_str++ = '\0';
	if (strlen(value_str) == 0) {
	    return FALSE;
	}

	/* Validate both parameter name and value */
	if (!validateString(param_request) || !validateString(value_str)) {
	    return FALSE;
	}

	/* Prepare HTML-formatted representation of the name=value pair */
	len = sprintf(param_formatted,
		      "<PARAM NAME=\"%s\" VALUE=\"%s\">\n",
		      param_request, value_str);
	if (cur_bytes + len + 1 > max_bytes) {
	    return FALSE;
	}
	strcat(result, param_formatted);
	cur_bytes += len;

	/* Go to the next parameter */
	if (delim_ptr == NULL) {
	    break;
	}
	tail = delim_ptr + 1;
    }
    return TRUE;
}

/*
 * Check if the string consists only of alphanumeric characters, '+'
 * signs, underscores, and dots. Replace all '+' signs with spaces.
 */

static rfbBool
validateString(char *str)
{
    char *ptr;

    for (ptr = str; *ptr != '\0'; ptr++) {
	if (!isalnum(*ptr) && *ptr != '_' && *ptr != '.') {
	    if (*ptr == '+') {
		*ptr = ' ';
	    } else {
		return FALSE;
	    }
	}
    }
    return TRUE;
}

