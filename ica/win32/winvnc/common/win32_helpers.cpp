#include <windows.h>
#include "win32_helpers.h"

namespace helper {

void SafeSetMsgResult(HWND hwnd, LPARAM result)
{
#ifndef _X64
	SetWindowLong(hwnd, DWL_MSGRESULT, result);
#else
	SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result);
#endif
}

void SafeSetWindowUserData(HWND hwnd, LPARAM lParam)
{
#ifndef _X64
	SetWindowLong(hwnd, GWL_USERDATA, lParam);
#else
	SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
#endif
}

HINSTANCE SafeGetWindowInstance(HWND hWnd)
{
#ifndef _X64
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd,GWL_HINSTANCE);
#else
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE);
#endif
    return hInstance;
}

LONG SafeGetWindowProc(HWND hWnd)
{
#ifndef _X64
    LONG pWndProc = GetWindowLong(hWnd, GWL_WNDPROC);
#else
    LONG pWndProc = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
#endif
    return pWndProc;
}

void SafeSetWindowProc(HWND hWnd, LONG pWndProc)
{
#ifndef _X64
    SetWindowLong(hWnd, GWL_WNDPROC, pWndProc);
#else
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, pWndProc);
#endif
}

void close_handle(HANDLE& h)
{
    if (h != INVALID_HANDLE_VALUE) 
    {
        ::CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

DynamicFnBase::DynamicFnBase(const TCHAR* dllName, const char* fnName) : dllHandle(0), fnPtr(0) {
  dllHandle = LoadLibrary(dllName);
  if (!dllHandle) {
    return;
  }
  fnPtr = (void *) GetProcAddress(dllHandle, fnName);
}

DynamicFnBase::~DynamicFnBase() {
  if (dllHandle)
    FreeLibrary(dllHandle);
}

} // namespace helper
