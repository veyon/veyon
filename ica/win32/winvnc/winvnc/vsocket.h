//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//  Copyright (C) 2001 HorizonLive.com, Inc. All Rights Reserved.
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


// VSocket.h

// RFB V3.0

// The VSocket class provides simple socket functionality,
// independent of platform.  Hurrah.

//#define FLOWCONTROL

class VSocket;
extern BOOL G_ipv6_allowed;
#if (!defined(_ATT_VSOCKET_DEFINED))
#define _ATT_VSOCKET_DEFINED

#include "vtypes.h"
#include <DSMPlugin/DSMPlugin.h>
////////////////////////////
#include <iphlpapi.h>
extern "C" {
        typedef ULONG (WINAPI *t_GetPerTcpConnectionEStats)(
                PMIB_TCPROW Row, TCP_ESTATS_TYPE EstatsType,
                PUCHAR Rw, ULONG RwVersion, ULONG RwSize,
                PUCHAR Ros, ULONG RosVersion, ULONG RosSize,
                PUCHAR Rod, ULONG RodVersion, ULONG RodSize);
       typedef ULONG (WINAPI *t_SetPerTcpConnectionEStats)(
                PMIB_TCPROW Row, TCP_ESTATS_TYPE EstatsType,
                PUCHAR Rw, ULONG RwVersion, ULONG RwSize,
				ULONG Offset);
}

// Socket implementation

// Create one or more VSocketSystem objects per application
class VSocketSystem
{
public:
	VSocketSystem();
	~VSocketSystem();
	VBool Initialised() {return m_status;};
private:
	VBool m_status;
};

// The main socket class
class VSocket
{
public:
  // Constructor/Destructor
  VSocket();
  virtual ~VSocket();

  // Create
  //        Create a socket and attach it to this VSocket object
#ifdef IPV6V4
#else
  VBool Create();
#endif

#ifdef IPV6V4
  VBool CreateConnect(const VString address, const VCard port);
  VBool CreateBindConnect(const VString address, const VCard port);
  VBool	CreateBindListen(const VCard port, const VBool localOnly = VFalse);
#endif

  // Shutdown
  //        Shutdown the currently attached socket
#ifdef IPV6V4
  VBool Shutdown();
  VBool Shutdown4();
  VBool Shutdown6();
#else
  VBool Shutdown();
#endif

  // Close
  //        Close the currently attached socket
#ifdef IPV6V4
  VBool Close();
  VBool Close4();
  VBool Close6();
#else
  VBool Close();
#endif

  // Bind
  //        Bind the attached socket to the specified port
  //		If localOnly is VTrue then the socket is bound only
  //        to the loopback adapter.
#ifdef IPV6V4
  VBool Bind4(const VCard port, const VBool localOnly=VFalse);
  VBool Bind6(const VCard port, const VBool localOnly=VFalse);
#else
  VBool Bind(const VCard port, const VBool localOnly=VFalse);
#endif

  // Connect
  //        Make a stream socket connection to the specified port
  //        on the named machine.
#ifdef IPV6V4
#else
  VBool Connect(const VString address, const VCard port);
#endif

  // Listen
  //        Set the attached socket to listen for connections
#ifdef IPV6V4
  VBool Listen4();
  VBool Listen6();
#else
  VBool Listen();
#endif

  // Accept
  //        If the attached socket is set to listen then this
  //        call blocks waiting for an incoming connection, then
  //        returns a new socket object for the new connection
#ifdef IPV6V4
  VSocket *Accept();
  VSocket *Accept4();
  VSocket *Accept6();
#else
  VSocket *Accept();
#endif

  // GetPeerName
  //        If the socket is connected then this returns the name
  //        of the machine to which it is connected.
  //        This string MUST be copied before the next socket call...
#ifdef IPV6V4
  VString GetPeerName();
  VString GetPeerName4();
  VString GetPeerName6();
#else
  VString GetPeerName();
#endif

  // GetSockName
  //		If the socket exists then the name of the local machine
  //		is returned.  This string MUST be copied before the next
  //		socket call!
#ifdef IPV6V4
  VString GetSockName();
  VString GetSockName4();
  VString GetSockName6();
#else
  VString GetSockName();
#endif

  // Resolve
  //        Uses the Winsock API to resolve the supplied DNS name to
  //        an IP address and returns it as an Int32
#ifdef IPV6V4
  static VCard32 Resolve4(const VString name);
  static bool Resolve6(const VString name, in6_addr * addr);
#else
  static VCard32 Resolve(const VString name);
#endif

  // SetTimeout
  //        Sets the socket timeout on reads and writes.
#ifdef IPV6V4
  VBool SetSendTimeout(VCard32 msecs);
  VBool SetRecvTimeout(VCard32 msecs);

  VBool SetTimeout(VCard32 msecs);
  VBool SetTimeout4(VCard32 msecs);
  VBool SetSendTimeout4(VCard32 msecs);
  VBool SetRecvTimeout4(VCard32 msecs);
  VBool SetTimeout6(VCard32 msecs);
  VBool SetSendTimeout6(VCard32 msecs);
  VBool SetRecvTimeout6(VCard32 msecs);
#else
  VBool SetTimeout(VCard32 msecs);
  VBool SetSendTimeout(VCard32 msecs);
  VBool SetRecvTimeout(VCard32 msecs);
#endif
  
  // adzm 2010-08
#ifdef IPV6V4
  VBool SetDefaultSocketOptions4();
  VBool SetDefaultSocketOptions6();
#else
  VBool SetDefaultSocketOptions();
#endif

  // adzm 2010-08
  static void SetSocketKeepAliveTimeoutDefault(int timeout) { m_defaultSocketKeepAliveTimeout = timeout; }
#ifdef IPV6V4
  bool GetPeerAddress4(char *address, int size);
  bool GetPeerAddress6(char *address, int size);
#else
  bool GetPeerAddress(char *address, int size);
#endif

#ifdef IPV6V4
 
  VBool ReadSelect(VCard to);
  VInt Send(const char *buff, const VCard bufflen);
  VInt SendQueued(const char *buff, const VCard bufflen);
  VInt Read(char *buff, const VCard bufflen);
  VBool SendExact(const char *buff, const VCard bufflen);
  VBool SendExact(const char *buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactQueue(const char *buff, const VCard bufflen);
  VBool SendExactQueue(const char *buff, const VCard bufflen, unsigned char msgType);
  VBool ReadExact(char *buff, const VCard bufflen);
  VBool ClearQueue();

  VBool ReadSelect(VCard to , int allsock);
  VInt Send(const char *buff, const VCard bufflen, int allsock);
  VInt SendQueued(const char *buff, const VCard bufflen, int allsock);
  VInt Read(char *buff, const VCard bufflen, int allsock);
  VBool SendExact(const char *buff, const VCard bufflen, int allsock);
  VBool SendExact(const char *buff, const VCard bufflen, unsigned char msgType, int allsock);
  VBool SendExactQueue(const char *buff, const VCard bufflen, int allsock);
  VBool SendExactQueue(const char *buff, const VCard bufflen, unsigned char msgType, int allsock);
  VBool ReadExact(char *buff, const VCard bufflen, int allsock);
  VBool ClearQueue(int allsock);
#else
  //VBool Http_CreateConnect(const VString address);
  // I/O routines
  // Check to see if the socket becomes readable within <to> msec.
  // Added to support HTTP-via-RFB.
  VBool ReadSelect(VCard to);

  // Send and Read return the number of bytes sent or recieved.
  VInt Send(const char *buff, const VCard bufflen);
  VInt SendQueued(const char *buff, const VCard bufflen);
  VInt Read(char *buff, const VCard bufflen);

  // SendExact and ReadExact attempt to send and recieve exactly
  // the specified number of bytes.
  VBool SendExact(const char *buff, const VCard bufflen);
  VBool SendExact(const char *buff, const VCard bufflen, unsigned char msgType); // sf@2002 - DSMPlugin
  VBool SendExactQueue(const char *buff, const VCard bufflen);
  //adzm 2010-09 - minimize packets. SendExact flushes the queue.
  VBool SendExactQueue(const char *buff, const VCard bufflen, unsigned char msgType);
  VBool ReadExact(char *buff, const VCard bufflen);
  VBool ClearQueue();
#endif
  // sf@2002 - DSMPlugin
  //adzm 2009-06-20
  void SetDSMPluginPointer(CDSMPlugin* pDSMPlugin);
  void EnableUsePlugin(bool fEnable) { m_fUsePlugin = fEnable;};
  bool IsUsePluginEnabled(void) { return m_fUsePlugin;};
  //adzm 2010-05-12 - dsmplugin config
  void SetDSMPluginConfig(char* szDSMPluginConfig);
  // adzm 2010-09
  void SetPluginStreamingIn() { m_fPluginStreamingIn = true; }
  void SetPluginStreamingOut() { m_fPluginStreamingOut = true; }
  bool IsPluginStreamingIn(void) { return m_fPluginStreamingIn; }
  bool IsPluginStreamingOut(void) { return m_fPluginStreamingOut; }

  void SetWriteToNetRectBuffer(bool fEnable) {m_fWriteToNetRectBuf = fEnable;}; 
  bool GetWriteToNetRectBuffer(void) {return m_fWriteToNetRectBuf;};
  int  GetNetRectBufOffset(void) {return m_nNetRectBufOffset;};
  void SetNetRectBufOffset(int nValue) {m_nNetRectBufOffset = nValue;};
  BYTE* GetNetRectBuf(void) {return m_pNetRectBuf;};
  void CheckNetRectBufferSize(int nBufSize);
  VBool SendExactHTTP(const char *buff, const VCard bufflen);
  VBool ReadExactHTTP(char *buff, const VCard bufflen);

  //adzm 2010-05-10
  IIntegratedPlugin* GetIntegratedPlugin() { return m_pIntegratedPluginInterface; };

  //adzm 2010-08-01
  DWORD GetLastSentTick() { return m_LastSentTick; };
  IIntegratedPlugin* m_pIntegratedPluginInterface;
  ////////////////////////////
  // Internal structures
protected:
  // The internal socket id
#ifdef IPV6V4
	int sock4;
	int sock6;
#else
  int sock;	      
#endif

  //adzm 2010-08-01
  DWORD m_LastSentTick;

  CDSMPlugin* m_pDSMPlugin; // sf@2002 - DSMPlugin
  //adzm 2009-06-20
  IPlugin* m_pPluginInterface;
  //adzm 2010-05-10
  bool m_fUsePlugin;
  bool m_fPluginStreamingIn; //adzm 2010-09
  bool m_fPluginStreamingOut; //adzm 2010-09
  omni_mutex m_TransMutex;
  omni_mutex m_RestMutex;
  omni_mutex m_CheckMutex;

  //adzm 2009-06-20
  BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* nTransformedDataLen);
  BYTE* RestoreBufferStep1(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);
  BYTE* RestoreBufferStep2(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);

  // All this should be private with accessors -> later
  BYTE* m_pNetRectBuf;
  bool m_fWriteToNetRectBuf;
  int m_nNetRectBufOffset;
  int m_nNetRectBufSize;

  char queuebuffer[9000];
  DWORD queuebuffersize;

  // adzm 2010-08
  static int m_defaultSocketKeepAliveTimeout;

  bool							GetOptimalSndBuf();
  unsigned int							G_SENDBUFFER;
};

#endif // _ATT_VSOCKET_DEFINED
