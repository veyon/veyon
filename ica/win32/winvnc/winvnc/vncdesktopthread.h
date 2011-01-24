/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#if !defined(_WINVNC_VNCDESKTOPTHREAD)
#define _WINVNC_VNCDESKTOPTHREAD
#include "stdhdrs.h"
#include "vncserver.h"
#include "vnckeymap.h"
#include "vncdesktop.h"
#include "vncservice.h"
#include "mmsystem.h"
#include "IPC.h"
#include "CpuUsage.h"

typedef struct _CURSORINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HCURSOR hCursor;
    POINT   ptScreenPos;
} MyCURSORINFO, *PMyCURSORINFO, *LPMyCURSORINFO;
// The desktop handler thread
// This handles the messages posted by RFBLib to the vncDesktop window
typedef BOOL (WINAPI *_GetCursorInfo)(PMyCURSORINFO pci);
extern bool g_DesktopThread_running;
#define MSGFLT_ADD		1
typedef BOOL (WINAPI *CHANGEWINDOWMESSAGEFILTER)(UINT message, DWORD dwFlag);

extern const UINT RFB_SCREEN_UPDATE;
extern const UINT RFB_COPYRECT_UPDATE;
extern const UINT RFB_MOUSE_UPDATE;

class vncDesktopThread : public omni_thread
{
public:
	vncDesktopThread() {
		m_returnsig = NULL;
		user32 = LoadLibrary("user32.dll");
		MyGetCursorInfo=NULL;
		if (user32) MyGetCursorInfo=(_GetCursorInfo )GetProcAddress(user32, "GetCursorInfo");
		g_DesktopThread_running=true;

		m_lLastMouseMoveTime = 0L;
		
		hUser32 = LoadLibrary("user32.dll");
		CHANGEWINDOWMESSAGEFILTER pfnFilter = NULL;
		if (hUser32)
		{
		pfnFilter =(CHANGEWINDOWMESSAGEFILTER)GetProcAddress(hUser32,"ChangeWindowMessageFilter");
		if (pfnFilter) pfnFilter(RFB_SCREEN_UPDATE, MSGFLT_ADD);
		if (pfnFilter) pfnFilter(RFB_COPYRECT_UPDATE, MSGFLT_ADD);
		if (pfnFilter) pfnFilter(RFB_MOUSE_UPDATE, MSGFLT_ADD);
		}
		cpuUsage=0;
		MIN_UPDATE_INTERVAL=33;
		MIN_UPDATE_INTERVAL_MAX=500;
		MIN_UPDATE_INTERVAL_MIN=10;
		// replaced by macpu ini setting
		MAX_CPU_USAGE=20;
	};
protected:
	~vncDesktopThread() {
		if (m_returnsig != NULL) delete m_returnsig;
		if (user32) FreeLibrary(user32);
		g_DesktopThread_running=false;
		if (hUser32) FreeLibrary(hUser32);
	};
private:
	bool handle_display_change(HANDLE& threadhandle, rfb::Region2D& rgncache, rfb::SimpleUpdateTracker& clipped_updates, rfb::ClippedUpdateTracker& updates);
	void do_polling(HANDLE& threadHandle, rfb::Region2D& rgncache, int& fullpollcounter, bool cursormoved);

public:
	virtual BOOL Init(vncDesktop *desktop, vncServer *server);
	virtual void *run_undetached(void *arg);
	virtual void ReturnVal(DWORD result);
	void PollWindow(rfb::Region2D &rgn, HWND hwnd);
	// Modif rdv@2002 - v1.1.x - videodriver
	virtual BOOL handle_driver_changes(rfb::Region2D &rgncache,rfb::UpdateTracker &tracker);
	virtual void copy_bitmaps_to_buffer(ULONG i,rfb::Region2D &rgncache,rfb::UpdateTracker &tracker);
	bool Handle_Ringbuffer(mystruct *ringbuffer,rfb::Region2D &rgncache);
	CIPC g_obIPC;
	vncDesktop *m_desktop;

protected:
	vncServer *m_server;

	omni_mutex m_returnLock;
	omni_condition *m_returnsig;
	DWORD m_return;
	BOOL m_returnset;
	bool m_screen_moved;
	bool lastsend;
	HMODULE user32;
	_GetCursorInfo MyGetCursorInfo;
	bool XRichCursorEnabled;
	DWORD newtick,oldtick,oldtick2;

	DWORD m_lLastMouseMoveTime;
	HMODULE  hUser32;
	CProcessorUsage usage;
	short cpuUsage;
	DWORD MIN_UPDATE_INTERVAL;
	DWORD MIN_UPDATE_INTERVAL_MAX;
	DWORD MIN_UPDATE_INTERVAL_MIN;
	DWORD MAX_CPU_USAGE;
	bool capture;
	bool first_run;

};
#endif