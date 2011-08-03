//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
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


// vncDesktop object

// The vncDesktop object handles retrieval of data from the
// display buffer.  It also uses the RFBLib DLL to supply
// information on mouse movements and screen updates to
// the server

class vncDesktop;

#if !defined(_WINVNC_VNCDESKTOP)
#define _WINVNC_VNCDESKTOP
#pragma once

// Include files
#include "stdhdrs.h"

class vncServer;
#include "rfbRegion.h"
#include "rfbUpdateTracker.h"
#include "vncbuffer.h"
#include "translate.h"
#include <omnithread.h>

// Modif rdv@2002 - v1.1.x - videodriver
#include "videodriver.h"

// Modif sf@2002 - v1.1.0
#include <list>
#include <set>
#include "TextChat.h"
#ifdef AVILOG
#include "avilog/avilog/AVIGenerator.h"
#endif
// adzm - 2010-07 - Extended clipboard
#include "common/Clipboard.h"
//#define COMPILE_MULTIMON_STUBS
//#include "Multimon.h"
#ifndef SM_CMONITORS

#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#define SM_SAMEDISPLAYFORMAT    81

// HMONITOR is already declared if WINVER >= 0x0500 in windef.h
// This is for components built with an older version number.
//
#if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002

#define MONITORINFOF_PRIMARY        0x00000001

typedef struct tagMONITORINFO
{
    DWORD   cbSize;
    RECT    rcMonitor;
    RECT    rcWork;
    DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;

#ifndef CCHDEVICENAME
#define CCHDEVICENAME 32
#endif

#ifdef __cplusplus
typedef struct tagMONITORINFOEXA : public tagMONITORINFO
{
    CHAR        szDevice[CCHDEVICENAME];
} MONITORINFOEXA, *LPMONITORINFOEXA;
typedef struct tagMONITORINFOEXW : public tagMONITORINFO
{
    WCHAR       szDevice[CCHDEVICENAME];
} MONITORINFOEXW, *LPMONITORINFOEXW;
#ifdef UNICODE
typedef MONITORINFOEXW MONITORINFOEX;
typedef LPMONITORINFOEXW LPMONITORINFOEX;
#else
typedef MONITORINFOEXA MONITORINFOEX;
typedef LPMONITORINFOEXA LPMONITORINFOEX;
#endif // UNICODE
#else // ndef __cplusplus
typedef struct tagMONITORINFOEXA
{
    MONITORINFO;
    CHAR        szDevice[CCHDEVICENAME];
} MONITORINFOEXA, *LPMONITORINFOEXA;
typedef struct tagMONITORINFOEXW
{
    MONITORINFO;
    WCHAR       szDevice[CCHDEVICENAME];
} MONITORINFOEXW, *LPMONITORINFOEXW;
#ifdef UNICODE
typedef MONITORINFOEXW MONITORINFOEX;
typedef LPMONITORINFOEXW LPMONITORINFOEX;
#else
typedef MONITORINFOEXA MONITORINFOEX;
typedef LPMONITORINFOEXA LPMONITORINFOEX;
#endif // UNICODE
#endif
#endif

typedef std::vector<COLORREF> RGBPixelList;   // List of RGB values (pixels)
// sf@2002 - Generates ClassName lenght warning in debug mode compile.
// typedef std::list<RGBPixelList*> GridsList; // List of Grids of pixels
typedef std::vector<RGBPixelList*> GridsList; // List of Grids of pixels
typedef std::set<HWND> WindowsList;       // List of windows handles


// Constants
extern const UINT RFB_SCREEN_UPDATE;
extern const UINT RFB_COPYRECT_UPDATE;
extern const UINT RFB_MOUSE_UPDATE;
extern const char szDesktopSink[];

#define NONE 0
#define MIRROR 1
#define PSEUDO 2

// initialization errors
#define ERROR_DESKTOP_NO_PALETTE 					    1
#define ERROR_DESKTOP_INIT_FAILED 					    2
#define ERROR_DESKTOP_UNSUPPORTED_PIXEL_ORGANIZATION    3
#define ERROR_DESKTOP_UNSUPPORTED_PIXEL_FORMAT 		    4
#define ERROR_DESKTOP_NO_HOOKWINDOW 				    5
#define ERROR_DESKTOP_NO_ROOTDC                         6
#define ERROR_DESKTOP_NO_BITBLT                         7
#define ERROR_DESKTOP_NO_GETDIBITS                      8
#define ERROR_DESKTOP_NO_COMPATBITMAP                   9
#define ERROR_DESKTOP_NO_DISPLAYFORMAT                  10
#define ERROR_DESKTOP_OUT_OF_MEMORY                     11
#define ERROR_DESKTOP_NO_DIBSECTION_OR_COMPATBITMAP     12
#define ERROR_DESKTOP_NO_DESKTOPTHREAD                  13
typedef struct DrvWatch
{
	HWND hwnd;
	bool *stop;
}DrvWatch;

typedef BOOL (*SetHooksFn)(DWORD thread_id,UINT UpdateMsg,UINT CopyMsg,UINT MouseMsg,BOOL ddihook);
typedef BOOL (*UnSetHooksFn)(DWORD thread_id);
typedef BOOL (*SetKeyboardFilterHookFn)(BOOL activate);	
typedef BOOL (*SetMouseFilterHookFn)(BOOL activate);
typedef BOOL (*SetHookFn)(HWND hwnd);
typedef BOOL (*UnSetHookFn)(HWND hwnd);

typedef BOOL (WINAPI*  pBlockInput) (BOOL);
typedef BOOL (WINAPI* LPGETMONITORINFO)(HMONITOR, LPMONITORINFO);
typedef HMONITOR (WINAPI* LPMONITOTFROMPOINT) (POINT,DWORD);


// Class definition
// multi monitor
struct monitor
{
	int Width;
	int Height;
	int Depth;
	char device[32];
	int offsetx;
	int offsety;
};


class PixelCaptureEngine
{
public:
	PixelCaptureEngine();
	~PixelCaptureEngine();
	void PixelCaptureEngineInit(HDC rootdc, HDC memdc, HBITMAP membitmap, bool bCaptureAlpha, void *dibbits, int bpp, int bpr);
	bool CaptureRect(const rfb::Rect& rect);
	COLORREF CapturePixel(int x, int y);
	void ReleaseCapture();
	bool		m_bIsVista;

private:
	HDC			m_hrootdc;
	HDC			m_hmemdc;
	HBITMAP		m_membitmap;
	HBITMAP		m_oldbitmap;
	void		*m_DIBbits;
	rfb::Rect	m_rect;
	bool		m_bCaptureAlpha;
	int			m_bytesPerPixel;
	int			m_bytesPerRow;
};

class vncDesktop
{

// Fields
public:

// Methods
public:
	// Make the desktop thread & window proc friends
	friend class vncDesktopThread;
	friend LRESULT CALLBACK DesktopWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	// Create/Destroy methods
	vncDesktop();
	~vncDesktop();

	DWORD Init(vncServer *pSrv);

	// Tell the desktop hooks to grab & update a particular rectangle
	void QueueRect(const rfb::Rect &rect);
	
	// Kick the desktop hooks to perform an update
	void TriggerUpdate();

	// Get a reference to the desktop update lock
	// The lock is held while data is being grabbed and copied
	// to the back buffer, and while changes are being passed to
	// clients
	omni_mutex &GetUpdateLock() {return m_update_lock;};

	// Screen translation, capture, info
	void FillDisplayInfo(rfbServerInitMsg *scrInfo);
	void CaptureScreen(const rfb::Rect &UpdateArea, BYTE *scrBuff, UINT scrBuffSize,BOOL capture);
	int ScreenBuffSize();
	HWND Window() {return m_hwnd;};

	// Mouse related
	void CaptureMouse(BYTE *scrBuff, UINT scrBuffSize);
	rfb::Rect MouseRect();
	void SetCursor(HCURSOR cursor);
	// CURSOR HANDLING
	BOOL GetRichCursorData(BYTE *databuf, HCURSOR hcursor, int width, int height);
	HCURSOR GetCursor() { return m_hcursor; }

	// Clipboard manipulation
	void SetClipText(LPSTR text);
	// adzm - 2010-07 - Extended clipboard
	void SetClipTextEx(ExtendedClipboardDataMessage& extendedClipboardDataMessage);

	// Method to obtain the DIBsection buffer if fast blits are enabled
	// If they're disabled, it'll return NULL
	inline VOID *OptimisedBlitBuffer() {return m_DIBbits;};

	// adzm - 2010-07 - Extended clipboard
	//BOOL	m_initialClipBoardSeen;

	// Handler for pixel data grabbing and region change checking
	vncBuffer		m_buffer;
		//SINGLE WINDOW
	vncServer		*GetServerPointer() {return m_server;};
	HWND			m_Single_hWnd;
	HWND			m_Single_hWnd_backup;
	BOOL			CalculateSWrect(RECT &rect);
	rfb::Rect		GetSize();
	rfb::Rect		GetQuarterSize();

	// Modif rdv@2002 - v1.1.x - videodriver
	//BOOL IsVideoDriverEnabled();
	BOOL VideoBuffer();
	int m_ScreenOffsetx;
	int m_ScreenOffsety;
	int DriverType;
	DWORD color[10];
	// added jeff
	void SetBlankMonitor(bool enabled);
	// Modif rdv@2002 Dis/enable input
	void SetDisableInput();
	void SetSW(int x,int y);
	//hook selection
	BOOL m_hookdriver;
	void SethookMechanism(BOOL hookall,BOOL hookdriver);
	bool m_UltraEncoder_used;
	rfb::Rect		m_Cliprect;//the region to check
	bool StopDriverWatches;

	PCHANGES_BUF pchanges_buf;
	CHANGES_BUF changes_buf;

	int GetNrMonitors();
	void GetPrimaryDevice();
	void GetSecondaryDevice();
	void Checkmonitors();
    // 28 Mar 2008 jdp
    void SetBlockInputState(bool newstate);
    bool GetBlockInputState() { return m_bIsInputDisabledByClient; }
    bool block_input();
	BOOL InitWindow();
	HANDLE trigger_events[6];
	HANDLE restart_event;
	DWORD pumpID;
	rfb::Region2D rgnpump;
	bool lock_region_add;

	// The current mouse position
	rfb::Rect		m_cursorpos;
	void WriteMessageOnScreen(char*,BYTE *scrBuff, UINT scrBuffSize);

	// Implementation
protected:

	// Routines to hook and unhook us
	DWORD Startup();
	BOOL Shutdown();
	
	// Init routines called by the child thread
	BOOL InitDesktop();
	void KillScreenSaver();
//	void KillWallpaper();
//	void RestoreWallpaper();
	DWORD InitBitmap();
	BOOL ThunkBitmapInfo();
	DWORD SetPixFormat();
	BOOL SetPixShifts();
	BOOL InitHooks();
	BOOL SetPalette();
	int m_timer;

	// Fetching pixel data to a buffer, and handling copyrects
	void CopyToBuffer(const rfb::Rect &rect, BYTE *scrBuff, UINT scrBuffSize);
	bool CalcCopyRects(rfb::UpdateTracker &tracker);

	// Routine to attempt enabling optimised DIBsection blits
	DWORD EnableOptimisedBlits();

	// Convert a bit mask eg. 00111000 to max=7, shift=3
	static void MaskToMaxAndShift(DWORD mask, CARD16 &max, CARD8 &shift);
	
	// Enabling & disabling clipboard handling
	void SetClipboardActive(BOOL active) {m_clipboard_active = active;};

	// Modif sf@2002 - v1.1.0 - FastDetectChanges stuff
	bool FastDetectChanges(rfb::Region2D &rgn, rfb::Rect &rect, int nZone, bool fTurbo);
	GridsList    m_lGridsList;   // List of changes detection grids
	WindowsList  m_lWList;		 // List of Windows handles  
	// HDC	         m_hDC;			 // Local Screen Device context to capture our Grid of pixels 
	int          m_nGridCycle;   // Cycle index for grid shifting

	// Modif sf@2002 - v1.1.0
	long         m_lLastMouseUpdateTime;
	long         m_lLastSlowClientTestTime;
	// long			m_lLastTempo;

	// sf@2002 - TextChat - No more used for now
	// bool m_fTextChatRunning;
	// TextChat* m_pCurrentTextChat;

	BOOL m_fCaptureAlphaBlending;
	// DATA

	// Generally useful stuff
	vncServer 		*m_server;
	omni_thread 	*m_thread;
	HWND			m_hwnd;
	//UINT			m_timerid;
	HWND			m_hnextviewer;
	// adzm - 2010-07 - Fix clipboard hangs
	bool			m_settingClipboardViewer;
	BOOL			m_clipboard_active;

	// device contexts for memory and the screen
	HDC				m_hmemdc;
	HDC				m_hrootdc;

	// New and old bitmaps
	HBITMAP			m_membitmap;
	omni_mutex		m_update_lock;

	rfb::Rect		m_bmrect;
	struct _BMInfo {
		BOOL			truecolour;
		BITMAPINFO		bmi;
		// Colormap info - comes straight after BITMAPINFO - **HACK**
		RGBQUAD			cmap[256];
	} m_bminfo;

	// Screen info
	rfbServerInitMsg	m_scrinfo;

	// These are the red, green & blue masks for a pixel
	DWORD			m_rMask, m_gMask, m_bMask;

	// This is always handy to have
	int				m_bytesPerRow;

	// Handle of the default cursor
	HCURSOR			m_hcursor;
	HCURSOR			m_hOldcursor; // sf@2002

	// Handle of the basic arrow cursor
	HCURSOR			m_hdefcursor;

	// Boolean flag to indicate when the display resolution has changed
	BOOL			m_displaychanged;

	// Boolean flag to indicate whether or not an update trigger message
	// is already in the desktop thread message queue
	BOOL			m_update_triggered;

	// Extra vars used for the DIBsection optimisation
	VOID			*m_DIBbits;
	BOOL			m_formatmunged;

	// Info used for polling modes
	UINT			m_pollingcycle;
	// rfb::Rect		m_fullscreen; // sf@2002 - v1.1.0

	// Handling of the foreground window, to produce CopyRects
	HWND			m_foreground_window;
	rfb::Rect		m_foreground_window_rect;

	//SINGLE WINDOW
	void SWinit();
	int m_SWHeight;
	int m_SWWidth;
	BOOL m_SWSizeChanged;
	BOOL m_SWmoved;
	BOOL m_SWtoDesktop;
	int m_SWOffsetx;
	int m_SWOffsety;

	//DDIHOOK

	// Modif rdv@2002 - v1.1.x - videodriver
	VIDEODRIVER *m_videodriver;
	BOOL InitVideoDriver();
 	void ShutdownVideoDriver();
	omni_mutex		m_videodriver_lock;

	// Modif input dis/enabke
	DWORD m_thread_hooks;
	BOOL ddihook;
	UINT OldPowerOffTimeout;
	bool m_Black_window_active;

	//	[v1.0.2-jp1 fix] Monitor Blanking
	//BOOL m_grayed;
	//WORD bk_gamma[3][256];
	
	//hook selection
	BOOL m_hookdll;
	BOOL On_Off_hookdll;
	BOOL m_hookswitch;
	BOOL Hookdll_Changed;
	BOOL m_hookinited;
	HANDLE m_hddihook;
	void StartStopddihook(BOOL enabled);
	void StartStophookdll(BOOL enabled);
	void InitHookSettings();
	HMODULE hModule;
	HMODULE hSCModule;
	SetHooksFn SetHooks;
	UnSetHooksFn  UnSetHooks;
	SetHookFn SetHook;
	UnSetHookFn  UnSetHook;
	SetKeyboardFilterHookFn SetKeyboardFilterHook;
	SetMouseFilterHookFn SetMouseFilterHook;
	//hooks in schook Hook(s)
	SetKeyboardFilterHookFn SetKeyboardFilterHooks;
	SetMouseFilterHookFn SetMouseFilterHooks;


	pBlockInput pbi;
	HMODULE hUser32;
	BOOL m_OrigpollingSet;
	BOOL m_Origpolling;
/*	BOOL Check24bit();*/
	COLORREF CapturePixel(int x,int y);
	HANDLE InitWindowThreadh;
	void StopInitWindowthread();
	void StartInitWindowthread();
	void ShutdownInitWindowthread();
	bool can_be_hooked;
	int old_Blockinput;
	int old_Blockinput1;
	int old_Blockinput2;
	int nr_rects;
	HWND hDeskWnd;
	HWND hFolderView;
	rfb::Region2D iconregion;
	bool blankmonitorstate;

	
BOOL DriverWanted;
BOOL HookWanted;
BOOL DriverWantedSet;

//Multi monitor
monitor mymonitor[3];
int nr_monitors;
bool multi_monitor;
bool requested_multi_monitor;

bool m_bIsInputDisabledByClient; // 28 March 2008 jdp
#ifdef AVILOG
CAVIGenerator *AviGen;
#endif

private:
	HDESK m_input_desktop;
	HDESK m_home_desktop;
	PixelCaptureEngine PixelEngine;
	int idle_counter;
	bool change_found;
};

#endif // _WINVNC_VNCDESKTOP
