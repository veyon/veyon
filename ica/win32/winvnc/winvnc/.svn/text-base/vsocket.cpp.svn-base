//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 2001 HorizonLive.com, Inc. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// VSocket.cpp

// The VSocket class provides a platform-independent socket abstraction
// with the simple functionality required for an RFB server.

class VSocket;

////////////////////////////////////////////////////////
// System includes

#include "stdhdrs.h"
#ifdef _VC80
#include <iostream>
#else
#include <iostream.h>
#endif

#include <stdio.h>
#ifdef __WIN32__
#include <io.h>
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#endif
#include <sys/types.h>

////////////////////////////////////////////////////////
// Custom includes

#include "VTypes.h"

////////////////////////////////////////////////////////
// *** Lovely hacks to make Win32 work.  Hurrah!

#ifdef __WIN32__
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

////////////////////////////////////////////////////////
// Socket implementation

#include "VSocket.h"

// The socket timeout value (currently 5 seconds, for no reason...)
// *** THIS IS NOT CURRENTLY USED ANYWHERE
const VInt rfbMaxClientWait = 5000;

////////////////////////////
// Socket implementation initialisation
static WORD winsockVersion = 0;

VSocketSystem::VSocketSystem()
{
  // Initialise the socket subsystem
  // This is only provided for compatibility with Windows.

#ifdef __WIN32__
  // Initialise WinPoxySockets on Win32
  WORD wVersionRequested;
  WSADATA wsaData;
	
  wVersionRequested = MAKEWORD(2, 0);
  if (WSAStartup(wVersionRequested, &wsaData) != 0)
  {
    m_status = VFalse;
	return;
  }

  winsockVersion = wsaData.wVersion;
 
#else
  // Disable the nasty read/write failure signals on UNIX
  signal(SIGPIPE, SIG_IGN);
#endif

  // If successful, or if not required, then continue!
  m_status = VTrue;
}

VSocketSystem::~VSocketSystem()
{
	if (m_status)
	{
		WSACleanup();
	}
}

////////////////////////////

VSocket::VSocket()
{
	// Clear out the internal socket fields
	sock = -1;
	vnclog.Print(LL_SOCKINFO, VNCLOG("VSocket() m_pDSMPlugin = NULL \n"));
	m_pDSMPlugin = NULL;
	m_fUsePlugin = false;
	
	m_pNetRectBuf = NULL;
	m_nNetRectBufSize = 0;
	m_fWriteToNetRectBuf = false;
	m_nNetRectBufOffset = 0;
	queuebuffersize=0;
}

////////////////////////////

VSocket::~VSocket()
{
  // Close the socket
  Close();

  if (m_pNetRectBuf != NULL)
 	delete [] m_pNetRectBuf;

}

////////////////////////////

VBool
VSocket::Create()
{
  const int one = 1;

  // Check that the old socket was closed
  if (sock >= 0)
    Close();

  // Create the socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      return VFalse;
    }

  // Set the socket options:
//#ifndef WIN32
  // sf@2006 - Trying to fix the neverending authentication bug
  /*
  // Usefull for NT4 ?
  if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&one, sizeof(one)))
    {
      return VFalse;
    }
  */
  // sf@2006 - Trying to fix the neverending authentication bug
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)))
  {
      return VFalse;
  }
  //#endif
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)))
  {
	  return VFalse;
  }

  return VTrue;
}

////////////////////////////

VBool
VSocket::Close()
{
  if (sock >= 0)
    {
	  vnclog.Print(LL_SOCKINFO, VNCLOG("closing socket\n"));

	  shutdown(sock, SD_BOTH);
#ifdef __WIN32__
	  closesocket(sock);
#else
	  close(sock);
#endif
      sock = -1;
    }
  return VTrue;
}

////////////////////////////

VBool
VSocket::Shutdown()
{
  if (sock >= 0)
    {
	  vnclog.Print(LL_SOCKINFO, VNCLOG("shutdown socket\n"));

	  shutdown(sock, SD_BOTH);
//	  sock = -1;
    }
  return VTrue;
}

////////////////////////////

VBool
VSocket::Bind(const VCard port, const VBool localOnly)
{
  struct sockaddr_in addr;

  // Check that the socket is open!
  if (sock < 0)
    return VFalse;

  // Set up the address to bind the socket to
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (localOnly)
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  else
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // And do the binding
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      return VFalse;

  return VTrue;
}

////////////////////////////

VBool
VSocket::Connect(const VString address, const VCard port)
{
  // Check the socket
  if (sock < 0)
    return VFalse;

  // Create an address structure and clear it
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  // Fill in the address if possible
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address);

  // Was the string a valid IP address?
  if (addr.sin_addr.s_addr == -1)
    {
      // No, so get the actual IP address of the host name specified
      struct hostent *pHost;
      pHost = gethostbyname(address);
      if (pHost != NULL)
	  {
		  if (pHost->h_addr == NULL)
			  return VFalse;
		  addr.sin_addr.s_addr = ((struct in_addr *)pHost->h_addr)->s_addr;
	  }
	  else
	    return VFalse;
    }

  // Set the port number in the correct format
  addr.sin_port = htons(port);

  // Actually connect the socket
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    return VFalse;

  // sf@2006 - Trying to fix the neverending authentication bug
  // Put the socket into non-blocking mode
/*
#ifdef __WIN32__
  u_long arg = 1;
  if (ioctlsocket(sock, FIONBIO, &arg) != 0)
	return VFalse;
#else
  if (fcntl(sock, F_SETFL, O_NDELAY) != 0)
	return VFalse;
#endif
*/
  return VTrue;
}

////////////////////////////

VBool
VSocket::Listen()
{
  // Check socket
  if (sock < 0)
    return VFalse;

	// Set it to listen
  if (listen(sock, 5) < 0)
    return VFalse;

  return VTrue;
}

////////////////////////////

VSocket *
VSocket::Accept()
{
  const int one = 1;

  int new_socket_id;
  VSocket * new_socket;

  // Check this socket
  if (sock < 0)
    return NULL;

  // Accept an incoming connection
  if ((new_socket_id = accept(sock, NULL, 0)) < 0)
    return NULL;

  // Create a new VSocket and return it
  new_socket = new VSocket;
  if (new_socket != NULL)
  {
      new_socket->sock = new_socket_id;
  }
  else
  {
	  shutdown(new_socket_id, SD_BOTH);
	  closesocket(new_socket_id);
  }

  // Attempt to set the new socket's options
  setsockopt(new_socket->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

  // Put the socket into non-blocking mode
  return new_socket;
}

////////////////////////////
////////////////////////////

VString
VSocket::GetPeerName()
{
	struct sockaddr_in	sockinfo;
	struct in_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	VString				name;

	// Get the peer address for the client socket
	getpeername(sock, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	name = inet_ntoa(address);
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}

////////////////////////////

VString
VSocket::GetSockName()
{
	struct sockaddr_in	sockinfo;
	struct in_addr		address;
	int					sockinfosize = sizeof(sockinfo);
	VString				name;

	// Get the peer address for the client socket
	getsockname(sock, (struct sockaddr *)&sockinfo, &sockinfosize);
	memcpy(&address, &sockinfo.sin_addr, sizeof(address));

	name = inet_ntoa(address);
	if (name == NULL)
		return "<unavailable>";
	else
		return name;
}

// 25 January 2008 jdp
bool VSocket::GetPeerAddress(char *address, int size)
{
    struct sockaddr_in addr;
    int addrsize = sizeof(sockaddr_in);

    if (sock < 0)
        return false;

    if (getpeername(sock, (struct sockaddr *) &addr, &addrsize) != 0)
        return false;

    _snprintf(address, size, "%s:%d",
              inet_ntoa(addr.sin_addr),
              ntohs(addr.sin_port));

    return true;
}
////////////////////////////

VCard32
VSocket::Resolve(const VString address)
{
  VCard32 addr;

  // Try converting the address as IP
  addr = inet_addr(address);

  // Was it a valid IP address?
  if (addr == 0xffffffff)
    {
      // No, so get the actual IP address of the host name specified
      struct hostent *pHost;
      pHost = gethostbyname(address);
      if (pHost != NULL)
	  {
		  if (pHost->h_addr == NULL)
			  return 0;
		  addr = ((struct in_addr *)pHost->h_addr)->s_addr;
	  }
	  else
		  return 0;
    }

  // Return the resolved IP address as an integer
  return addr;
}

////////////////////////////

VBool
VSocket::SetTimeout(VCard32 msecs)
{
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	int timeout=msecs;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	return VTrue;
}

VBool VSocket::SetSendTimeout(VCard32 msecs)
{
#if 1
    return SetTimeout (msecs);
#else
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	int timeout=msecs;

	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}

	return VTrue;
#endif
}

VBool VSocket::SetRecvTimeout(VCard32 msecs)
{
#if 1
    return SetTimeout (msecs);
#else
	if (LOBYTE(winsockVersion) < 2)
		return VFalse;
	int timeout=msecs;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return VFalse;
	}
	return VTrue;
#endif
}

////////////////////////////
VInt
VSocket::Send(const char *buff, const VCard bufflen)
{
	int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;

	if (newsize >8192)
	{
		    memcpy(queuebuffer+queuebuffersize,buff2,8192-queuebuffersize);
			send(sock,queuebuffer,8192,0);
//			vnclog.Print(LL_SOCKERR, VNCLOG("SEND  %i\n") ,8192);
			buff2+=(8192-queuebuffersize);
			bufflen2-=(8192-queuebuffersize);
			queuebuffersize=0;
			while (bufflen2 > 8192)
			{
				if (!send(sock,buff2,8192,0)) return false;
//				vnclog.Print(LL_SOCKERR, VNCLOG("SEND 1 %i\n") ,8192);
				buff2+=8192;
				bufflen2-=8192;
			}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	if (!send(sock,queuebuffer,queuebuffersize,0)) 
        return false;
//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND 2 %i\n") ,queuebuffersize);
	queuebuffersize=0;
	return bufflen;
}
////////////////////////////
VInt
VSocket::SendQueued(const char *buff, const VCard bufflen)
{
	int newsize=queuebuffersize+bufflen;
	char *buff2;
	buff2=(char*)buff;
	unsigned int bufflen2=bufflen;

	if (newsize >8192)
	{
		    memcpy(queuebuffer+queuebuffersize,buff2,8192-queuebuffersize);
			send(sock,queuebuffer,8192,0);
		//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,8192);
			buff2+=(8192-queuebuffersize);
			bufflen2-=(8192-queuebuffersize);
			queuebuffersize=0;
			while (bufflen2 > 8192)
			{
				if (!send(sock,buff2,8192,0)) return false;
			//	vnclog.Print(LL_SOCKERR, VNCLOG("SEND Q  %i\n") ,8192);
				buff2+=8192;
				bufflen2-=8192;
			}
	}
	memcpy(queuebuffer+queuebuffersize,buff2,bufflen2);
	queuebuffersize+=bufflen2;
	return bufflen;
}
/////////////////////////////

// sf@2002 - DSMPlugin
VBool
VSocket::SendExact(const char *buff, const VCard bufflen, unsigned char msgType)
{
	//vnclog.Print(LL_SOCKERR, VNCLOG("SendExactMsg %i\n") ,bufflen);
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// Send the transformed message type first
		char t = (char)msgType;
		SendExact(&t, 1);
		// Then we send the transformed rfb message content
		SendExact(buff, bufflen);
	}
	else
		SendExact(buff, bufflen);

	return VTrue;
}

//////////////////////////////////////////
VBool
VSocket::SendExact(const char *buff, const VCard bufflen)
{
//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExact %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			pBuffer = (char*)(m_pDSMPlugin->TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}
		
	}
	else
		pBuffer = (char*) buff;
	
	VInt result=Send(pBuffer, nBufflen);
  return result == (VInt)nBufflen;
}
///////////////////////////////////////
VBool
VSocket::SendExactQueue(const char *buff, const VCard bufflen)
{
//	vnclog.Print(LL_SOCKERR, VNCLOG("SendExactQueue %i %i\n") ,bufflen,queuebuffersize);
//	vnclog.Print(LL_SOCKERR, VNCLOG("socket size %i\n") ,bufflen);
	// sf@2002 - DSMPlugin
	VCard nBufflen = bufflen;
	char* pBuffer = NULL;
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		// omni_mutex_lock l(m_TransMutex);

		// If required to store data into memory
		if (m_fWriteToNetRectBuf)
		{
			memcpy((char*)(m_pNetRectBuf + m_nNetRectBufOffset), buff, bufflen);
			m_nNetRectBufOffset += bufflen;
			return VTrue;
		}
		else // Tell the plugin to transform data
		{
			int nTransDataLen = 0;
			pBuffer = (char*)(m_pDSMPlugin->TransformBuffer((BYTE*)buff, bufflen, &nTransDataLen));
			if (pBuffer == NULL || (bufflen > 0 && nTransDataLen == 0))
			{
				// throw WarningException("SendExact: DSMPlugin-TransformBuffer Error.");
			}
			nBufflen = nTransDataLen;
		}
		
	}
	else
		pBuffer = (char*) buff;
	
	VInt result=SendQueued(pBuffer, nBufflen);
  return result == (VInt)nBufflen;
}
///////////////////////////////
void
VSocket::ClearQueue()
{
	if (queuebuffersize!=0)
  {
	send(sock,queuebuffer,queuebuffersize,0);
	queuebuffersize=0;
  }
}
////////////////////////////

VInt
VSocket::Read(char *buff, const VCard bufflen)
{
    int s = recv(sock, buff, bufflen, 0);
#if defined(_DEBUG)
    if (s == SOCKET_ERROR)
    {
        OutputDebugString("recv: SOCKET_ERROR");
    }

    if (s == 0)
    {
        OutputDebugString("recv: connection closed");
    }
#endif
  return s;
}

////////////////////////////

VBool
VSocket::ReadExact(char *buff, const VCard bufflen)
{
	int n;
	VCard currlen = bufflen;
    
	// sf@2002 - DSM Plugin
	if (m_fUsePlugin && m_pDSMPlugin->IsEnabled())
	{
		omni_mutex_lock l(m_pDSMPlugin->m_RestMutex); 

		// Get the DSMPlugin destination buffer where to put transformed incoming data
		// The number of bytes to read calculated from bufflen is given back in nTransDataLen
		int nTransDataLen = 0;
		BYTE* pTransBuffer = m_pDSMPlugin->RestoreBufferStep1(NULL, bufflen, &nTransDataLen);
		if (pTransBuffer == NULL)
		{
			// m_pDSMPlugin->RestoreBufferUnlock();
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Alloc Error.");
			vnclog.Print(LL_SOCKERR, VNCLOG("WriteExact: DSMPlugin-RestoreBuffer Alloc Error\n"));
			return VFalse;
		}
		
		// Read bytes directly into Plugin Dest Rest. buffer
		int nTransDataLenSave = nTransDataLen;
		while (nTransDataLen > 0)
		{
			// Try to read some data in
			n = Read((char*)pTransBuffer, nTransDataLen);
			if (n > 0)
			{
				// Adjust the buffer position and size
				pTransBuffer += n;
				nTransDataLen -= n;
			} else if (n == 0) {
				//m_pDSMPlugin->RestoreBufferUnlock();
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read1\n"));
				return VFalse;
			} else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//m_pDSMPlugin->RestoreBufferUnlock();
					vnclog.Print(LL_SOCKERR, VNCLOG("socket error 1: %d\n"), WSAGetLastError());
					return VFalse;
				}
			}
		}
		
		// Ask plugin to restore data from rest. buffer into inbuf
		int nRestDataLen = 0;
		nTransDataLen = nTransDataLenSave;
		BYTE* pPipo = m_pDSMPlugin->RestoreBufferStep2((BYTE*)buff, nTransDataLen, &nRestDataLen);
		
		// Check if we actually get the real original data length
		if ((VCard)nRestDataLen != bufflen)
		{
			// throw WarningException("WriteExact: DSMPlugin-RestoreBuffer Error.");
		}
	}
	else // Non-Transformed
	{
		while (currlen > 0)
		{
			// Try to read some data in
			n = Read(buff, currlen);
			
			if (n > 0)
			{
				// Adjust the buffer position and size
				buff += n;
				currlen -= n;
			} else if (n == 0) {
				vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read2\n"));
				
				return VFalse;
			} else {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//int aa=WSAGetLastError();
					//vnclog.Print(LL_SOCKERR, VNCLOG("socket error 2: %d\n"), aa);
					return VFalse;
				}
			}
		}
	}

return VTrue;
}


//
// sf@2002 - DSMPlugin
// 

//
// Ensures that the temporary "alignement" buffer in large enough 
//
void VSocket::CheckNetRectBufferSize(int nBufSize)
{
    omni_mutex_lock l(m_CheckMutex); 

	if (m_nNetRectBufSize > nBufSize) return;

	BYTE *newbuf = new BYTE[nBufSize + 256];
	if (newbuf == NULL) 
	{
		// throw ErrorException("Insufficient memory to allocate network crypt buffer.");
	}

	if (m_pNetRectBuf != NULL)
	{
		// memcpy(newbuf, m_pNetRectBuf, m_nNetRectBufSize);
		delete [] m_pNetRectBuf;
	}

	m_pNetRectBuf = newbuf;
	m_nNetRectBufSize = nBufSize + 256;
	// vnclog.Print(4, _T("crypt bufsize expanded to %d\n"), m_netbufsize);
}


// sf@2002 - DSMPlugin
// Necessary for HTTP server (we'll see later if we can do something more intelligent...

VBool
VSocket::SendExactHTTP(const char *buff, const VCard bufflen)
{
  return Send(buff, bufflen) == (VInt)bufflen;
}


////////////////////////////

VBool
VSocket::ReadExactHTTP(char *buff, const VCard bufflen)
{
	int n;
	VCard currlen = bufflen;
    
	while (currlen > 0)
	{
		// Try to read some data in
		n = Read(buff, currlen);

		if (n > 0)
		{
			// Adjust the buffer position and size
			buff += n;
			currlen -= n;
		} else if (n == 0) {
			vnclog.Print(LL_SOCKERR, VNCLOG("zero bytes read3\n"));

			return VFalse;
		} else {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				vnclog.Print(LL_SOCKERR, VNCLOG("HTTP socket error: %d\n"), WSAGetLastError());
				return VFalse;
			}
		}
    }

	return VTrue;
}
#ifdef HTTP_SAMEPORT
VBool
VSocket::ReadSelect(VCard to)
 {
 	fd_set fds;
 	FD_ZERO(&fds);
 	FD_SET((unsigned long)sock,&fds);
	struct timeval tv;
 	tv.tv_sec = to/1000;
 	tv.tv_usec = (to % 1000)*1000;
 	int rc = select(sock+1,&fds,0,0,&tv);
 	if (rc>0) return true;
 	return false;
 }
#endif



