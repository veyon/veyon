#include "stdhdrs.h"
#if !defined(_WINVNC_LOGON)
#define _WINVNC_LOGON
#pragma once
class vncLogonThread : public omni_thread
{
public:
	vncLogonThread() {m_returnsig = NULL;};
	~vncLogonThread() {if (m_returnsig != NULL) delete m_returnsig;};
public:
	virtual BOOL Init(vncDesktop *desktop);
	virtual void *run_undetached(void *arg);
	HWND logonhwnd;
	RECT    rect;

protected:
	omni_mutex m_returnLock;
	omni_condition *m_returnsig;
	vncDesktop *m_desktop;
	int x1,x2,y1,y2;
	VOID CenterWindow(HWND hwnd);
	static BOOL CALLBACK LogonDlgProc(HWND hDlg,UINT Message,WPARAM wParam,LPARAM lParam);
	int CreateLogonWindow(HINSTANCE hInstance);
};
#endif