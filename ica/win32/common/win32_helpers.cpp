/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
#include <winsock2.h>
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
