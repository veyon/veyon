#if !defined(_WINVNC_VNCDESKTOPTHREAD)
#define _WINVNC_VNCDESKTOPTHREAD
#include "stdhdrs.h"
#include "vncServer.h"
#include "vncKeymap.h"
#include "vncDesktop.h"
#include "vncService.h"
#include "mmsystem.h"
#include "ipc.h"

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
		m_lLastUpdateTime = 0L;
		
		hUser32 = LoadLibrary("user32.dll");
		CHANGEWINDOWMESSAGEFILTER pfnFilter = NULL;
		pfnFilter =(CHANGEWINDOWMESSAGEFILTER)GetProcAddress(hUser32,"ChangeWindowMessageFilter");
		if (pfnFilter) pfnFilter(RFB_SCREEN_UPDATE, MSGFLT_ADD);
		if (pfnFilter) pfnFilter(RFB_COPYRECT_UPDATE, MSGFLT_ADD);
		if (pfnFilter) pfnFilter(RFB_MOUSE_UPDATE, MSGFLT_ADD);
	};
protected:
	~vncDesktopThread() {
		if (m_returnsig != NULL) delete m_returnsig;
		if (user32) FreeLibrary(user32);
		g_DesktopThread_running=false;
		if (hUser32) FreeLibrary(hUser32);
	};
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
	DWORD newtick,oldtick;

	DWORD m_lLastUpdateTime;
	DWORD m_lLastMouseMoveTime;
	HMODULE  hUser32;

};
#endif