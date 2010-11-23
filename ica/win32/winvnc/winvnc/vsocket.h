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

class VSocket;

#if (!defined(_ATT_VSOCKET_DEFINED))
#define _ATT_VSOCKET_DEFINED

#include "vtypes.h"
#include <DSMPlugin/DSMPlugin.h>

////////////////////////////
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

  ////////////////////////////
  // Socket implementation

  // Create
  //        Create a socket and attach it to this VSocket object
  VBool Create();

  // Shutdown
  //        Shutdown the currently attached socket
  VBool Shutdown();

  // Close
  //        Close the currently attached socket
  VBool Close();

  // Bind
  //        Bind the attached socket to the specified port
  //		If localOnly is VTrue then the socket is bound only
  //        to the loopback adapter.
  VBool Bind(const VCard port, const VBool localOnly=VFalse);

  // Connect
  //        Make a stream socket connection to the specified port
  //        on the named machine.
  VBool Connect(const VString address, const VCard port);

  // Listen
  //        Set the attached socket to listen for connections
  VBool Listen();

  // Accept
  //        If the attached socket is set to listen then this
  //        call blocks waiting for an incoming connection, then
  //        returns a new socket object for the new connection
  VSocket *Accept();

  // GetPeerName
  //        If the socket is connected then this returns the name
  //        of the machine to which it is connected.
  //        This string MUST be copied before the next socket call...
  VString GetPeerName();

  // GetSockName
  //		If the socket exists then the name of the local machine
  //		is returned.  This string MUST be copied before the next
  //		socket call!
  VString GetSockName();

  // Resolve
  //        Uses the Winsock API to resolve the supplied DNS name to
  //        an IP address and returns it as an Int32
  static VCard32 Resolve(const VString name);

  // SetTimeout
  //        Sets the socket timeout on reads and writes.
  VBool SetTimeout(VCard32 msecs);
  VBool SetSendTimeout(VCard32 msecs);
  VBool SetRecvTimeout(VCard32 msecs);
  
  // adzm 2010-08
  VBool SetDefaultSocketOptions();

  // adzm 2010-08
  static void SetSocketKeepAliveTimeoutDefault(int timeout) { m_defaultSocketKeepAliveTimeout = timeout; }

  bool GetPeerAddress(char *address, int size);

  VBool Http_CreateConnect(const VString address);
  SOCKET GetChannel() const { return (SOCKET) sock; }
  // I/O routines
#ifdef HTTP_SAMEPORT
  // Check to see if the socket becomes readable within <to> msec.
  // Added to support HTTP-via-RFB.
  VBool ReadSelect(VCard to);
#endif

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


  ////////////////////////////
  // Internal structures
protected:
  // The internal socket id
  int sock;	      

  //adzm 2010-08-01
  DWORD m_LastSentTick;

  CDSMPlugin* m_pDSMPlugin; // sf@2002 - DSMPlugin
  //adzm 2009-06-20
  IPlugin* m_pPluginInterface;
  //adzm 2010-05-10
  IIntegratedPlugin* m_pIntegratedPluginInterface;
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

};

#endif // _ATT_VSOCKET_DEFINED
