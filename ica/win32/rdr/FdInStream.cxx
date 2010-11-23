// Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
// Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
// USA.

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef _WIN32
// adzm 2010-08 - use winsock2
#include <winsock2.h>
#include <sys/timeb.h>
#define read(s,b,l) recv(s,(char*)b,l,0)
#undef errno
#define errno WSAGetLastError()
#else
#include <unistd.h>
#include <sys/time.h>
#endif

// XXX should use autoconf HAVE_SYS_SELECT_H
#ifdef _AIX
#include <sys/select.h>
#endif

#include "FdInStream.h"
#include "Exception.h"

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 8192,
       MIN_BULK_SIZE = 1024 };

FdInStream::FdInStream(int fd_, int timeout_, int bufSize_)
  : fd(fd_), timeout(timeout_), blockCallback(0), blockCallbackArg(0),
    timing(false), timeWaitedIn100us(5), timedKbits(0),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
	ptr = end = start = new U8[bufSize];

	// sf@2002
	m_fDSMMode = false;
	m_fReadFromNetRectBuf = false;
	m_nNetRectBufOffset = 0;
	m_nReadSize = 0;

	m_nBytesRead = 0; // For stats
}

FdInStream::FdInStream(int fd_, void (*blockCallback_)(void*),
                       void* blockCallbackArg_, int bufSize_)
  : fd(fd_), timeout(0), blockCallback(blockCallback_),
    blockCallbackArg(blockCallbackArg_),
    timing(false), timeWaitedIn100us(5), timedKbits(0),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
	ptr = end = start = new U8[bufSize];
	
	// sf@2002
	m_fDSMMode = false;
	m_fReadFromNetRectBuf = false;
	m_nNetRectBufOffset = 0;
	m_nReadSize = 0;
	
}

FdInStream::~FdInStream()
{
  delete [] start;
}

void
FdInStream::Update_socket()
{
  // test, not used
  //fd=INVALID_SOCKET;
}


int FdInStream::pos()
{
  return offset + ptr - start;
}

void FdInStream::readBytes(void* data, int length)
{
	// sf@2003 - Seems to fix the ZRLE+DSM bug... 
	if (!m_fDSMMode)
	{
		if (length < MIN_BULK_SIZE)
		{
			InStream::readBytes(data, length);
			return;
		}
	}
	
	U8* dataPtr = (U8*)data;
	
	int	n = end - ptr;
	if (n > length) n = length;
	
	memcpy(dataPtr, ptr, n);
	dataPtr += n;
	length -= n;
	ptr += n;

	while (length > 0) {
		n = readWithTimeoutOrCallback(dataPtr, length);
		dataPtr += n;
		length -= n;
		offset += n;
	}
}


int FdInStream::overrun(int itemSize, int nItems)
{
  if (itemSize > bufSize)
    throw Exception("FdInStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  offset += ptr - start;
  end -= ptr - start;
  ptr = start;

  while (end < start + itemSize) {
    int n = readWithTimeoutOrCallback((U8*)end, start + bufSize - end);
    end += n;
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}


int FdInStream::Check_if_buffer_has_data()
{
 InStream::setptr(InStream::getend());
 return checkReadable(fd, 500);
}

int FdInStream::checkReadable(int fd, int timeout)
{
  while (true) {
    fd_set rfds;
    struct timeval tv;
    
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    int n = select(fd+1, &rfds, 0, 0, &tv);
    if (n != -1 || errno != EINTR)
      return n;
    fprintf(stderr,"select returned EINTR\n");
  }
}

/*#ifdef _WIN32
static void gettimeofday_(struct timeval* tv, void*)
{
  LARGE_INTEGER counts, countsPerSec;
  static double usecPerCount = 0.0;
  counts.QuadPart=0;
  if (QueryPerformanceCounter(&counts)) {
    if (usecPerCount == 0.0) {
      QueryPerformanceFrequency(&countsPerSec);
	  if (countsPerSec.QuadPart!=0)
      usecPerCount = 1000000.0 / countsPerSec.QuadPart;
	  else usecPerCount = 200;
    }

    LONGLONG usecs = (LONGLONG)(counts.QuadPart * usecPerCount);
	
    tv->tv_usec = (long)((LONGLONG)usecs % (LONGLONG)1000000);
    tv->tv_sec = (long)(usecs / 1000000);

  } else {
    struct timeb tb;
    ftime(&tb);
    tv->tv_sec = (long)tb.time;
    tv->tv_usec = tb.millitm * 1000;
  }
}
#endif*/

#ifdef _WIN32
LONGLONG 
Passedusecs()
{
  LARGE_INTEGER counts, countsPerSec;
  static LONGLONG usecPerCount = 0;
  LONGLONG usecs=0;

  if (QueryPerformanceCounter(&counts)) {
    if (usecPerCount == 0) {
      QueryPerformanceFrequency(&countsPerSec);
      usecPerCount = 1000000000000000000 / countsPerSec.QuadPart;
    }
    usecs = (LONGLONG)(counts.QuadPart * usecPerCount /1000000000000);

  } else {
    struct timeb tb;
    ftime(&tb);
	usecs=tb.time*1000000+tb.millitm * 1000;
  }
  return usecs;
}
#endif

int FdInStream::readWithTimeoutOrCallback(void* buf, int len)
{
  /*struct timeval before = {0, 0}, after; // before will not get initialized if the condition is false
  if (timing)
    gettimeofday_(&before, NULL);*/
  LONGLONG before =0, after; // before will not get initialized if the condition is false
  if (timing)
    before=Passedusecs();

  if (fd==INVALID_SOCKET) 
	  throw SystemException("read",errno);

  int n=0;
  if (!m_fReadFromNetRectBuf)
  {
	  n = checkReadable(fd, timeout);
	  
	  if (n < 0) throw SystemException("select",errno);
	  
	  if (n == 0)
	  {
		  if (timeout) throw TimedOut();
		  if (blockCallback) (*blockCallback)(blockCallbackArg);
	  }
  }
	if (fd==INVALID_SOCKET) 
		throw SystemException("read",errno);
  bool fAlreadyCounted = false; // sf@2004 - Avoid to count the plugin processed bytes twice...

  while (true)
  {
	  if (fd==INVALID_SOCKET) 
		throw SystemException("read",errno);
	// sf@2002 - DSM Plugin hack - Only necessary for ZRLE encoding
	// If we must read already restored data from DSMPLugin memory  
	if (m_fReadFromNetRectBuf)
	{
		int nRem = m_nReadSize - m_nNetRectBufOffset;
		int nRead = (len <= nRem) ? len : nRem;
		memcpy(buf, m_pNetRectBuf + m_nNetRectBufOffset, nRead);
		m_nNetRectBufOffset += nRead;
		if (m_nNetRectBufOffset == m_nReadSize)
		{
			// Next read calls should read the socket
			m_fReadFromNetRectBuf = false;
			m_nNetRectBufOffset = 0;
			m_nReadSize = 0;
		}
		n = nRead;
		fAlreadyCounted = true;
	}
	else
	{
		n = ::read(fd, buf, len);
	}

    if (n != -1 || errno != EINTR)
      break;
    fprintf(stderr,"read returned EINTR\n");
  }

  if (n < 0) throw SystemException("read",errno);
  if (n == 0) throw EndOfStream("read");

  if (fAlreadyCounted)
	  return n; 

  // sf@2002 - stats
  m_nBytesRead += n;

  if (timing)
  {
    after=Passedusecs();

    LONGLONG newTimeWaited = (after- before)/100;
    int newKbits = n * 8 / 1000;
    if (newTimeWaited > newKbits*1000) newTimeWaited = newKbits*1000;
    if (newTimeWaited < newKbits/4)    newTimeWaited = newKbits/4;

    timeWaitedIn100us += (unsigned int)newTimeWaited;
    timedKbits += newKbits;
  }


  /*if (timing)
  {
    gettimeofday_(&after, 0);
//      fprintf(stderr,"%d.%06d\n",(after.tv_sec - before.tv_sec),
//              (after.tv_usec - before.tv_usec));
    int newTimeWaited = ((after.tv_sec - before.tv_sec) * 10000 +
                         (after.tv_usec - before.tv_usec) / 100);
    int newKbits = n * 8 / 1000;

//      if (newTimeWaited == 0) {
//        fprintf(stderr,"new kbps infinite t %d k %d\n",
//                newTimeWaited, newKbits);
//      } else {
//        fprintf(stderr,"new kbps %d t %d k %d\n",
//                newKbits * 10000 / newTimeWaited, newTimeWaited, newKbits);
//      }

    // limit rate to between 10kbit/s and 40Mbit/s

    if (newTimeWaited > newKbits*1000) newTimeWaited = newKbits*1000;
    if (newTimeWaited < newKbits/4)    newTimeWaited = newKbits/4;

    timeWaitedIn100us += newTimeWaited;
    timedKbits += newKbits;
  }*/

  return n;
}

void FdInStream::startTiming()
{
  timing = true;

  // Carry over up to 1s worth of previous rate for smoothing.

  if (timeWaitedIn100us > 10000) {
    timedKbits = timedKbits * 10000 / timeWaitedIn100us;
    timeWaitedIn100us = 10000;
  }
}

void FdInStream::stopTiming()
{
  timing = false; 
  if (timeWaitedIn100us < timedKbits/2)
    timeWaitedIn100us = timedKbits/2; // upper limit 20Mbit/s
}

unsigned int FdInStream::kbitsPerSecond()
{
  // The following calculation will overflow 32-bit arithmetic if we have
  // received more than about 50Mbytes (400Mbits) since we started timing, so
  // it should be OK for a single RFB update.

  return timedKbits * 10000 / timeWaitedIn100us;
}


//
// sf@2002 - DSMPlugin - This hack is necessary because
// the ZRLE encoder/decoder does not use the SendExact/ReadExact functions
// like ALL the others encoders...
// 
void FdInStream::SetReadFromMemoryBuffer(int nReadSize, char* pMemBuffer)
{
	m_nReadSize = nReadSize; // Nb of bytes to read from buffer instead of socket
	m_pNetRectBuf = pMemBuffer; // The memory buffer containing restored data
	m_nNetRectBufOffset = 0;   // Initial offset

	m_fReadFromNetRectBuf = true; // Order to read from the buffer
}



