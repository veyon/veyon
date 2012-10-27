/*
 * sockets.c - deal with TCP & UDP sockets.
 *
 * This code should be independent of any changes in the RFB protocol.  It just
 * deals with the X server scheduling stuff, calling rfbNewClientConnection and
 * rfbProcessClientMessage to actually deal with the protocol.  If a socket
 * needs to be closed for any reason then rfbCloseClient should be called, and
 * this in turn will call rfbClientConnectionGone.  To make an active
 * connection out, call rfbConnect - note that this does _not_ call
 * rfbNewClientConnection.
 *
 * This file is divided into two types of function.  Those beginning with
 * "rfb" are specific to sockets using the RFB protocol.  Those without the
 * "rfb" prefix are more general socket routines (which are used by the http
 * code).
 *
 * Thanks to Karl Hakimian for pointing out that some platforms return EAGAIN
 * not EWOULDBLOCK.
 */

/*
 *  Copyright (C) 2011-2012 Christian Beier <dontmind@freeshell.org>
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
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

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#ifdef __linux__
/* Setting this on other systems hides definitions such as INADDR_LOOPBACK.
 * The check should be for __GLIBC__ in fact. */
# define _POSIX_SOURCE
#endif
#endif

#include <rfb/rfb.h>

#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

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
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
#include "rfbssl.h"
#endif

#if defined(__linux__) && defined(NEED_TIMEVAL)
struct timeval 
{
   long int tv_sec,tv_usec;
}
;
#endif

#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <errno.h>

#ifdef USE_LIBWRAP
#include <syslog.h>
#include <tcpd.h>
int allow_severity=LOG_INFO;
int deny_severity=LOG_WARNING;
#endif

#if defined(WIN32)
#ifndef __MINGW32__
#pragma warning (disable: 4018 4761)
#endif
#define read(sock,buf,len) recv(sock,buf,len,0)
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ETIMEDOUT WSAETIMEDOUT
#define write(sock,buf,len) send(sock,buf,len,0)
#else
#define closesocket close
#endif

int rfbMaxClientWait = 20000;   /* time (ms) after which we decide client has
                                   gone away - needed to stop us hanging */

/*
 * rfbInitSockets sets up the TCP and UDP sockets to listen for RFB
 * connections.  It does nothing if called again.
 */

void
rfbInitSockets(rfbScreenInfoPtr rfbScreen)
{
    in_addr_t iface = rfbScreen->listenInterface;

    if (rfbScreen->socketState!=RFB_SOCKET_INIT)
	return;

    rfbScreen->socketState = RFB_SOCKET_READY;

    if (rfbScreen->inetdSock != -1) {
	const int one = 1;

        if(!rfbSetNonBlocking(rfbScreen->inetdSock))
	    return;

	if (setsockopt(rfbScreen->inetdSock, IPPROTO_TCP, TCP_NODELAY,
		       (char *)&one, sizeof(one)) < 0) {
	    rfbLogPerror("setsockopt");
	    return;
	}

    	FD_ZERO(&(rfbScreen->allFds));
    	FD_SET(rfbScreen->inetdSock, &(rfbScreen->allFds));
    	rfbScreen->maxFd = rfbScreen->inetdSock;
	return;
    }

    if(rfbScreen->autoPort) {
        int i;
        FD_ZERO(&(rfbScreen->allFds));

        rfbLog("Autoprobing TCP port \n");
        for (i = 5900; i < 6000; i++) {
            if ((rfbScreen->listenSock = rfbListenOnTCPPort(i, iface)) >= 0) {
		rfbScreen->port = i;
		break;
	    }
        }

        if (i >= 6000) {
	    rfbLogPerror("Failure autoprobing");
	    return;
        }

        rfbLog("Autoprobing selected TCP port %d\n", rfbScreen->port);
        FD_SET(rfbScreen->listenSock, &(rfbScreen->allFds));
        rfbScreen->maxFd = rfbScreen->listenSock;

#ifdef LIBVNCSERVER_IPv6
        rfbLog("Autoprobing TCP6 port \n");
	for (i = 5900; i < 6000; i++) {
            if ((rfbScreen->listen6Sock = rfbListenOnTCP6Port(i, rfbScreen->listen6Interface)) >= 0) {
		rfbScreen->ipv6port = i;
		break;
	    }
        }

        if (i >= 6000) {
	    rfbLogPerror("Failure autoprobing");
	    return;
        }

        rfbLog("Autoprobing selected TCP6 port %d\n", rfbScreen->ipv6port);
	FD_SET(rfbScreen->listen6Sock, &(rfbScreen->allFds));
	rfbScreen->maxFd = max((int)rfbScreen->listen6Sock,rfbScreen->maxFd);
#endif
    }
    else
    {
	    if(rfbScreen->port>0) {
      FD_ZERO(&(rfbScreen->allFds));

      if ((rfbScreen->listenSock = rfbListenOnTCPPort(rfbScreen->port, iface)) < 0) {
	rfbLogPerror("ListenOnTCPPort");
	return;
      }
      rfbLog("Listening for VNC connections on TCP port %d\n", rfbScreen->port);  
  
      FD_SET(rfbScreen->listenSock, &(rfbScreen->allFds));
      rfbScreen->maxFd = rfbScreen->listenSock;
	    }

#ifdef LIBVNCSERVER_IPv6
	    if (rfbScreen->ipv6port>0) {
      if ((rfbScreen->listen6Sock = rfbListenOnTCP6Port(rfbScreen->ipv6port, rfbScreen->listen6Interface)) < 0) {
	/* ListenOnTCP6Port has its own detailed error printout */
	return;
      }
      rfbLog("Listening for VNC connections on TCP6 port %d\n", rfbScreen->ipv6port);  
	
      FD_SET(rfbScreen->listen6Sock, &(rfbScreen->allFds));
      rfbScreen->maxFd = max((int)rfbScreen->listen6Sock,rfbScreen->maxFd);
	    }
#endif

    }

    if (rfbScreen->udpPort != 0) {
	rfbLog("rfbInitSockets: listening for input on UDP port %d\n",rfbScreen->udpPort);

	if ((rfbScreen->udpSock = rfbListenOnUDPPort(rfbScreen->udpPort, iface)) < 0) {
	    rfbLogPerror("ListenOnUDPPort");
	    return;
	}
	rfbLog("Listening for VNC connections on TCP port %d\n", rfbScreen->port);  

	FD_SET(rfbScreen->udpSock, &(rfbScreen->allFds));
	rfbScreen->maxFd = max((int)rfbScreen->udpSock,rfbScreen->maxFd);
    }
}

void rfbShutdownSockets(rfbScreenInfoPtr rfbScreen)
{
    if (rfbScreen->socketState!=RFB_SOCKET_READY)
	return;

    rfbScreen->socketState = RFB_SOCKET_SHUTDOWN;

    if(rfbScreen->inetdSock>-1) {
	closesocket(rfbScreen->inetdSock);
	FD_CLR(rfbScreen->inetdSock,&rfbScreen->allFds);
	rfbScreen->inetdSock=-1;
    }

    if(rfbScreen->listenSock>-1) {
	closesocket(rfbScreen->listenSock);
	FD_CLR(rfbScreen->listenSock,&rfbScreen->allFds);
	rfbScreen->listenSock=-1;
    }

    if(rfbScreen->listen6Sock>-1) {
	closesocket(rfbScreen->listen6Sock);
	FD_CLR(rfbScreen->listen6Sock,&rfbScreen->allFds);
	rfbScreen->listen6Sock=-1;
    }

    if(rfbScreen->udpSock>-1) {
	closesocket(rfbScreen->udpSock);
	FD_CLR(rfbScreen->udpSock,&rfbScreen->allFds);
	rfbScreen->udpSock=-1;
    }
}

/*
 * rfbCheckFds is called from ProcessInputEvents to check for input on the RFB
 * socket(s).  If there is input to process, the appropriate function in the
 * RFB server code will be called (rfbNewClientConnection,
 * rfbProcessClientMessage, etc).
 */

int
rfbCheckFds(rfbScreenInfoPtr rfbScreen,long usec)
{
    int nfds;
    fd_set fds;
    struct timeval tv;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buf[6];
    rfbClientIteratorPtr i;
    rfbClientPtr cl;
    int result = 0;

    if (!rfbScreen->inetdInitDone && rfbScreen->inetdSock != -1) {
	rfbNewClientConnection(rfbScreen,rfbScreen->inetdSock); 
	rfbScreen->inetdInitDone = TRUE;
    }

    do {
	memcpy((char *)&fds, (char *)&(rfbScreen->allFds), sizeof(fd_set));
	tv.tv_sec = 0;
	tv.tv_usec = usec;
	nfds = select(rfbScreen->maxFd + 1, &fds, NULL, NULL /* &fds */, &tv);
	if (nfds == 0) {
	    /* timed out, check for async events */
            i = rfbGetClientIterator(rfbScreen);
            while((cl = rfbClientIteratorNext(i))) {
                if (cl->onHold)
                    continue;
                if (FD_ISSET(cl->sock, &(rfbScreen->allFds)))
                    rfbSendFileTransferChunk(cl);
            }
            rfbReleaseClientIterator(i);
	    return result;
	}

	if (nfds < 0) {
#ifdef WIN32
	    errno = WSAGetLastError();
#endif
	    if (errno != EINTR)
		rfbLogPerror("rfbCheckFds: select");
	    return -1;
	}

	result += nfds;

	if (rfbScreen->listenSock != -1 && FD_ISSET(rfbScreen->listenSock, &fds)) {

	    if (!rfbProcessNewConnection(rfbScreen))
                return -1;

	    FD_CLR(rfbScreen->listenSock, &fds);
	    if (--nfds == 0)
		return result;
	}

	if (rfbScreen->listen6Sock != -1 && FD_ISSET(rfbScreen->listen6Sock, &fds)) {

	    if (!rfbProcessNewConnection(rfbScreen))
                return -1;

	    FD_CLR(rfbScreen->listen6Sock, &fds);
	    if (--nfds == 0)
		return result;
	}

	if ((rfbScreen->udpSock != -1) && FD_ISSET(rfbScreen->udpSock, &fds)) {
	    if(!rfbScreen->udpClient)
		rfbNewUDPClient(rfbScreen);
	    if (recvfrom(rfbScreen->udpSock, buf, 1, MSG_PEEK,
			(struct sockaddr *)&addr, &addrlen) < 0) {
		rfbLogPerror("rfbCheckFds: UDP: recvfrom");
		rfbDisconnectUDPSock(rfbScreen);
		rfbScreen->udpSockConnected = FALSE;
	    } else {
		if (!rfbScreen->udpSockConnected ||
			(memcmp(&addr, &rfbScreen->udpRemoteAddr, addrlen) != 0))
		{
		    /* new remote end */
		    rfbLog("rfbCheckFds: UDP: got connection\n");

		    memcpy(&rfbScreen->udpRemoteAddr, &addr, addrlen);
		    rfbScreen->udpSockConnected = TRUE;

		    if (connect(rfbScreen->udpSock,
				(struct sockaddr *)&addr, addrlen) < 0) {
			rfbLogPerror("rfbCheckFds: UDP: connect");
			rfbDisconnectUDPSock(rfbScreen);
			return -1;
		    }

		    rfbNewUDPConnection(rfbScreen,rfbScreen->udpSock);
		}

		rfbProcessUDPInput(rfbScreen);
	    }

	    FD_CLR(rfbScreen->udpSock, &fds);
	    if (--nfds == 0)
		return result;
	}

	i = rfbGetClientIterator(rfbScreen);
	while((cl = rfbClientIteratorNext(i))) {

	    if (cl->onHold)
		continue;

            if (FD_ISSET(cl->sock, &(rfbScreen->allFds)))
            {
                if (FD_ISSET(cl->sock, &fds))
                    rfbProcessClientMessage(cl);
                else
                    rfbSendFileTransferChunk(cl);
            }
	}
	rfbReleaseClientIterator(i);
    } while(rfbScreen->handleEventsEagerly);
    return result;
}

rfbBool
rfbProcessNewConnection(rfbScreenInfoPtr rfbScreen)
{
    const int one = 1;
    int sock = -1;
#ifdef LIBVNCSERVER_IPv6
    struct sockaddr_storage addr;
#else
    struct sockaddr_in addr;
#endif
    socklen_t addrlen = sizeof(addr);
    fd_set listen_fds; 
    int chosen_listen_sock = -1;

    /* Do another select() call to find out which listen socket
       has an incoming connection pending. We know that at least 
       one of them has, so this should not block for too long! */
    FD_ZERO(&listen_fds);  
    if(rfbScreen->listenSock >= 0) 
      FD_SET(rfbScreen->listenSock, &listen_fds);
    if(rfbScreen->listen6Sock >= 0) 
      FD_SET(rfbScreen->listen6Sock, &listen_fds);
    if (select(rfbScreen->maxFd+1, &listen_fds, NULL, NULL, NULL) == -1) {
      rfbLogPerror("rfbProcessNewConnection: error in select");
      return FALSE;
    }
    if (rfbScreen->listenSock >= 0 && FD_ISSET(rfbScreen->listenSock, &listen_fds))
      chosen_listen_sock = rfbScreen->listenSock;
    if (rfbScreen->listen6Sock >= 0 && FD_ISSET(rfbScreen->listen6Sock, &listen_fds))
      chosen_listen_sock = rfbScreen->listen6Sock;

    if ((sock = accept(chosen_listen_sock,
		       (struct sockaddr *)&addr, &addrlen)) < 0) {
      rfbLogPerror("rfbCheckFds: accept");
      return FALSE;
    }

    if(!rfbSetNonBlocking(sock)) {
      closesocket(sock);
      return FALSE;
    }

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *)&one, sizeof(one)) < 0) {
      rfbLogPerror("rfbCheckFds: setsockopt");
      closesocket(sock);
      return FALSE;
    }

#ifdef USE_LIBWRAP
    if(!hosts_ctl("vnc",STRING_UNKNOWN,inet_ntoa(addr.sin_addr),
		  STRING_UNKNOWN)) {
      rfbLog("Rejected connection from client %s\n",
	     inet_ntoa(addr.sin_addr));
      closesocket(sock);
      return FALSE;
    }
#endif

#ifdef LIBVNCSERVER_IPv6
    char host[1024];
    if(getnameinfo((struct sockaddr*)&addr, addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST) != 0) {
      rfbLogPerror("rfbProcessNewConnection: error in getnameinfo");
    }
    rfbLog("Got connection from client %s\n", host);
#else
    rfbLog("Got connection from client %s\n", inet_ntoa(addr.sin_addr));
#endif

    rfbNewClient(rfbScreen,sock);

    return TRUE;
}


void
rfbDisconnectUDPSock(rfbScreenInfoPtr rfbScreen)
{
  rfbScreen->udpSockConnected = FALSE;
}



void
rfbCloseClient(rfbClientPtr cl)
{
    rfbExtensionData* extension;

    for(extension=cl->extensions; extension; extension=extension->next)
	if(extension->extension->close)
	    extension->extension->close(cl, extension->data);

    LOCK(cl->updateMutex);
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
    if (cl->sock != -1)
#endif
      {
	FD_CLR(cl->sock,&(cl->screen->allFds));
	if(cl->sock==cl->screen->maxFd)
	  while(cl->screen->maxFd>0
		&& !FD_ISSET(cl->screen->maxFd,&(cl->screen->allFds)))
	    cl->screen->maxFd--;
#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
	if (cl->sslctx)
	    rfbssl_destroy(cl);
	free(cl->wspath);
#endif
#ifndef __MINGW32__
	shutdown(cl->sock,SHUT_RDWR);
#endif
	closesocket(cl->sock);
	cl->sock = -1;
      }
    TSIGNAL(cl->updateCond);
    UNLOCK(cl->updateMutex);
}


/*
 * rfbConnect is called to make a connection out to a given TCP address.
 */

int
rfbConnect(rfbScreenInfoPtr rfbScreen,
           char *host,
           int port)
{
    int sock;
    int one = 1;

    rfbLog("Making connection to client on host %s port %d\n",
	   host,port);

    if ((sock = rfbConnectToTcpAddr(host, port)) < 0) {
	rfbLogPerror("connection failed");
	return -1;
    }

    if(!rfbSetNonBlocking(sock)) {
        closesocket(sock);
	return -1;
    }

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *)&one, sizeof(one)) < 0) {
	rfbLogPerror("setsockopt failed");
	closesocket(sock);
	return -1;
    }

    /* AddEnabledDevice(sock); */
    FD_SET(sock, &rfbScreen->allFds);
    rfbScreen->maxFd = max(sock,rfbScreen->maxFd);

    return sock;
}

/*
 * ReadExact reads an exact number of bytes from a client.  Returns 1 if
 * those bytes have been read, 0 if the other end has closed, or -1 if an error
 * occurred (errno is set to ETIMEDOUT if it timed out).
 */

int
rfbReadExactTimeout(rfbClientPtr cl, char* buf, int len, int timeout)
{
    int sock = cl->sock;
    int n;
    fd_set fds;
    struct timeval tv;

    while (len > 0) {
#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
        if (cl->wsctx) {
            n = webSocketsDecode(cl, buf, len);
        } else if (cl->sslctx) {
	    n = rfbssl_read(cl, buf, len);
	} else {
            n = read(sock, buf, len);
        }
#else
        n = read(sock, buf, len);
#endif

        if (n > 0) {

            buf += n;
            len -= n;

        } else if (n == 0) {

            return 0;

        } else {
#ifdef WIN32
	    errno = WSAGetLastError();
#endif
	    if (errno == EINTR)
		continue;

#ifdef LIBVNCSERVER_ENOENT_WORKAROUND
	    if (errno != ENOENT)
#endif
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                return n;
            }

#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
	    if (cl->sslctx) {
		if (rfbssl_pending(cl))
		    continue;
	    }
#endif
            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            n = select(sock+1, &fds, NULL, &fds, &tv);
            if (n < 0) {
                rfbLogPerror("ReadExact: select");
                return n;
            }
            if (n == 0) {
                rfbErr("ReadExact: select timeout\n");
                errno = ETIMEDOUT;
                return -1;
            }
        }
    }
#undef DEBUG_READ_EXACT
#ifdef DEBUG_READ_EXACT
    rfbLog("ReadExact %d bytes\n",len);
    for(n=0;n<len;n++)
	    fprintf(stderr,"%02x ",(unsigned char)buf[n]);
    fprintf(stderr,"\n");
#endif

    return 1;
}

int rfbReadExact(rfbClientPtr cl,char* buf,int len)
{
  /* favor the per-screen value if set */
  if(cl->screen && cl->screen->maxClientWait)
    return(rfbReadExactTimeout(cl,buf,len,cl->screen->maxClientWait));
  else
    return(rfbReadExactTimeout(cl,buf,len,rfbMaxClientWait));
}

/*
 * PeekExact peeks at an exact number of bytes from a client.  Returns 1 if
 * those bytes have been read, 0 if the other end has closed, or -1 if an
 * error occurred (errno is set to ETIMEDOUT if it timed out).
 */

int
rfbPeekExactTimeout(rfbClientPtr cl, char* buf, int len, int timeout)
{
    int sock = cl->sock;
    int n;
    fd_set fds;
    struct timeval tv;

    while (len > 0) {
#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
	if (cl->sslctx)
	    n = rfbssl_peek(cl, buf, len);
	else
#endif
	    n = recv(sock, buf, len, MSG_PEEK);

        if (n == len) {

            break;

        } else if (n == 0) {

            return 0;

        } else {
#ifdef WIN32
	    errno = WSAGetLastError();
#endif
	    if (errno == EINTR)
		continue;

#ifdef LIBVNCSERVER_ENOENT_WORKAROUND
	    if (errno != ENOENT)
#endif
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                return n;
            }

#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
	    if (cl->sslctx) {
		if (rfbssl_pending(cl))
		    continue;
	    }
#endif
            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            n = select(sock+1, &fds, NULL, &fds, &tv);
            if (n < 0) {
                rfbLogPerror("PeekExact: select");
                return n;
            }
            if (n == 0) {
                errno = ETIMEDOUT;
                return -1;
            }
        }
    }
#undef DEBUG_READ_EXACT
#ifdef DEBUG_READ_EXACT
    rfbLog("PeekExact %d bytes\n",len);
    for(n=0;n<len;n++)
	    fprintf(stderr,"%02x ",(unsigned char)buf[n]);
    fprintf(stderr,"\n");
#endif

    return 1;
}

/*
 * WriteExact writes an exact number of bytes to a client.  Returns 1 if
 * those bytes have been written, or -1 if an error occurred (errno is set to
 * ETIMEDOUT if it timed out).
 */

int
rfbWriteExact(rfbClientPtr cl,
              const char *buf,
              int len)
{
    int sock = cl->sock;
    int n;
    fd_set fds;
    struct timeval tv;
    int totalTimeWaited = 0;
    const int timeout = (cl->screen && cl->screen->maxClientWait) ? cl->screen->maxClientWait : rfbMaxClientWait;

#undef DEBUG_WRITE_EXACT
#ifdef DEBUG_WRITE_EXACT
    rfbLog("WriteExact %d bytes\n",len);
    for(n=0;n<len;n++)
	    fprintf(stderr,"%02x ",(unsigned char)buf[n]);
    fprintf(stderr,"\n");
#endif

#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
    if (cl->wsctx) {
        char *tmp = NULL;
        if ((len = webSocketsEncode(cl, buf, len, &tmp)) < 0) {
            rfbErr("WriteExact: WebSockets encode error\n");
            return -1;
        }
        buf = tmp;
    }
#endif

    LOCK(cl->outputMutex);
    while (len > 0) {
#ifdef LIBVNCSERVER_WITH_WEBSOCKETS
        if (cl->sslctx)
	    n = rfbssl_write(cl, buf, len);
	else
#endif
	    n = write(sock, buf, len);

        if (n > 0) {

            buf += n;
            len -= n;

        } else if (n == 0) {

            rfbErr("WriteExact: write returned 0?\n");
            return 0;

        } else {
#ifdef WIN32
			errno = WSAGetLastError();
#endif
	    if (errno == EINTR)
		continue;

            if (errno != EWOULDBLOCK && errno != EAGAIN) {
	        UNLOCK(cl->outputMutex);
                return n;
            }

            /* Retry every 5 seconds until we exceed timeout.  We
               need to do this because select doesn't necessarily return
               immediately when the other end has gone away */

            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            n = select(sock+1, NULL, &fds, NULL /* &fds */, &tv);
	    if (n < 0) {
#ifdef WIN32
                errno=WSAGetLastError();
#endif
       	        if(errno==EINTR)
		    continue;
                rfbLogPerror("WriteExact: select");
                UNLOCK(cl->outputMutex);
                return n;
            }
            if (n == 0) {
                totalTimeWaited += 5000;
                if (totalTimeWaited >= timeout) {
                    errno = ETIMEDOUT;
                    UNLOCK(cl->outputMutex);
                    return -1;
                }
            } else {
                totalTimeWaited = 0;
            }
        }
    }
    UNLOCK(cl->outputMutex);
    return 1;
}

/* currently private, called by rfbProcessArguments() */
int
rfbStringToAddr(char *str, in_addr_t *addr)  {
    if (str == NULL || *str == '\0' || strcmp(str, "any") == 0) {
        *addr = htonl(INADDR_ANY);
    } else if (strcmp(str, "localhost") == 0) {
        *addr = htonl(INADDR_LOOPBACK);
    } else {
        struct hostent *hp;
        if ((*addr = inet_addr(str)) == htonl(INADDR_NONE)) {
            if (!(hp = gethostbyname(str))) {
                return 0;
            }
            *addr = *(unsigned long *)hp->h_addr;
        }
    }
    return 1;
}

int
rfbListenOnTCPPort(int port,
                   in_addr_t iface)
{
    struct sockaddr_in addr;
    int sock;
    int one = 1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = iface;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof(one)) < 0) {
	closesocket(sock);
	return -1;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	closesocket(sock);
	return -1;
    }
    if (listen(sock, 32) < 0) {
	closesocket(sock);
	return -1;
    }

    return sock;
}


int
rfbListenOnTCP6Port(int port,
                    const char* iface)
{
#ifndef LIBVNCSERVER_IPv6
    rfbLogPerror("This LibVNCServer does not have IPv6 support");
    return -1;
#else
    int sock;
    int one = 1;
    int rv;
    struct addrinfo hints, *servinfo, *p;
    char port_str[8];

    snprintf(port_str, 8, "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; /* fill in wildcard address if iface == NULL */

    if ((rv = getaddrinfo(iface, port_str, &hints, &servinfo)) != 0) {
        rfbErr("rfbListenOnTCP6Port error in getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    
    /* loop through all the results and bind to the first we can */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

#ifdef IPV6_V6ONLY
	/* we have seperate IPv4 and IPv6 sockets since some OS's do not support dual binding */
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&one, sizeof(one)) < 0) {
	  rfbLogPerror("rfbListenOnTCP6Port error in setsockopt IPV6_V6ONLY");
	  closesocket(sock);
	  freeaddrinfo(servinfo);
	  return -1;
	}
#endif

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) < 0) {
	  rfbLogPerror("rfbListenOnTCP6Port: error in setsockopt SO_REUSEADDR");
	  closesocket(sock);
	  freeaddrinfo(servinfo);
	  return -1;
	}

	if (bind(sock, p->ai_addr, p->ai_addrlen) < 0) {
	  closesocket(sock);
	  continue;
	}

        break;
    }

    if (p == NULL)  {
        rfbLogPerror("rfbListenOnTCP6Port: error in bind IPv6 socket");
        freeaddrinfo(servinfo);
        return -1;
    }

    /* all done with this structure now */
    freeaddrinfo(servinfo);

    if (listen(sock, 32) < 0) {
        rfbLogPerror("rfbListenOnTCP6Port: error in listen on IPv6 socket");
	closesocket(sock);
	return -1;
    }

    return sock;
#endif
}


int
rfbConnectToTcpAddr(char *host,
                    int port)
{
    int sock;
#ifdef LIBVNCSERVER_IPv6
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char port_str[8];

    snprintf(port_str, 8, "%d", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port_str, &hints, &servinfo)) != 0) {
        rfbErr("rfbConnectToTcpAddr: error in getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    /* loop through all the results and connect to the first we can */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        if (connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
            closesocket(sock);
            continue;
        }

        break;
    }

    /* all failed */
    if (p == NULL) {
        rfbLogPerror("rfbConnectToTcoAddr: failed to connect\n");
        sock = -1; /* set return value */
    }

    /* all done with this structure now */
    freeaddrinfo(servinfo);
#else
    struct hostent *hp;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if ((addr.sin_addr.s_addr = inet_addr(host)) == htonl(INADDR_NONE))
    {
	if (!(hp = gethostbyname(host))) {
	    errno = EINVAL;
	    return -1;
	}
	addr.sin_addr.s_addr = *(unsigned long *)hp->h_addr;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, (sizeof(addr))) < 0) {
	closesocket(sock);
	return -1;
    }
#endif
    return sock;
}

int
rfbListenOnUDPPort(int port,
                   in_addr_t iface)
{
    struct sockaddr_in addr;
    int sock;
    int one = 1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = iface;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof(one)) < 0) {
	return -1;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	return -1;
    }

    return sock;
}

/*
 * rfbSetNonBlocking sets a socket into non-blocking mode.
 */
rfbBool
rfbSetNonBlocking(int sock)
{
#ifdef WIN32
  unsigned long block=1;
  if(ioctlsocket(sock, FIONBIO, &block) == SOCKET_ERROR) {
    errno=WSAGetLastError();
#else
  int flags = fcntl(sock, F_GETFL);
  if(flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
#endif
    rfbLogPerror("Setting socket to non-blocking failed");
    return FALSE;
  }
  return TRUE;
}
