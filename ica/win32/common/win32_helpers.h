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

#if !defined(UVNC_COMMON_H)
#define UVNC_COMMON_H

namespace helper {

template<typename T> inline T *SafeGetWindowUserData(HWND hwnd)
{
    T *pUserData;
#ifndef _X64
	pUserData = (T *) GetWindowLong(hwnd, GWL_USERDATA);
#else
	pUserData = (T *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif

    return pUserData;
}

void SafeSetWindowUserData(HWND hwnd, LPARAM lParam);

// DWL_MSGRESULT
void SafeSetMsgResult(HWND hwnd, LPARAM result);
// GWL_HINSTANCE
HINSTANCE SafeGetWindowInstance(HWND hWnd);
// GWL_WNDPROC
LONG SafeGetWindowProc(HWND hWnd);
void SafeSetWindowProc(HWND hWnd, LONG pWndProc);

void close_handle(HANDLE& h);

    class DynamicFnBase {
    public:
      DynamicFnBase(const TCHAR* dllName, const char* fnName);
      ~DynamicFnBase();
      bool isValid() const {return fnPtr != 0;}
    protected:
      void* fnPtr;
      HMODULE dllHandle;
    };

    template<typename T> class DynamicFn : public DynamicFnBase {
    public:
      DynamicFn(const TCHAR* dllName, const char* fnName) : DynamicFnBase(dllName, fnName) {}
      T operator *() const {return (T)fnPtr;};
    };

}

#endif
