//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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
//need to be added for VS 2005
//#define _WIN32_IE 0x0500
#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif

//compile special case, rfb port is used for java and rfb
//#define HTTP_SAMEPORT

#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
//#include <winsock2.h>

#include <malloc.h>
#include <stdio.h>
#include <process.h>
#include <crtdbg.h>

#include <QtCore/QString>

//#include "dpi.h"

// LOGGING SUPPORT
void *memcpy_amd(void *dest, const void *src, size_t n);
bool CheckVideoDriver(bool);
#define MAXPATH 256

#include "vnclog.h"
extern VNCLog vnclog;

// No logging at all
#define LL_NONE		0
// Log server startup/shutdown
#define LL_STATE	1
// Log connect/disconnect
#define LL_CLIENTS	2
// Log connection errors (wrong pixfmt, etc)
#define LL_CONNERR	3
// Log socket errors
#define LL_SOCKERR	4
// Log internal errors
#define LL_INTERR	5

#define LL_ERROR	6

// Log internal warnings
#define LL_INTWARN	8
// Log internal info
#define LL_INTINFO	9
// Log socket errors
#define LL_SOCKINFO	10
// Log everything, including internal table setup, etc.
#define LL_ALL		10

#define strcat_s(dst,num,src) strncat(dst,src,num)
#define strncat_s(a,b,c,d) strncat(a,c,b)
#define strcpy_s(dst,num,src)	strncpy(dst,src,num)
#define strncpy_s(dst,num,src,x)	strncpy(dst,src,num)

#define KEEPALIVE_HEADROOM 1
#define KEEPALIVE_INTERVAL 5
#define FT_RECV_TIMEOUT    30

// adzm 2010-08
#define SOCKET_KEEPALIVE_TIMEOUT 10000
#define SOCKET_KEEPALIVE_INTERVAL 1000

// Macros for sticking in the current file name
#define VNCLOG(s)	(QString("%1 : %2").arg(__PRETTY_FUNCTION__).arg(s).toUtf8().constData())
#if MSC_VER > 12
#ifndef _X64
#pragma comment(linker,"/manifestdependency:\"type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
//#define memcpy memcpy_amd
//remove comment to compiler for >=athlon  or >=PIII
DWORD MessageBoxSecure(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType);
