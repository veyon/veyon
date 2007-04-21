//  Copyright (C) 2001 Constantin Kaplinsky. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
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
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/

// SharedData.cpp : Storage for the global data in the DLL
// This file is used only when your compiler is Borland C++.

// Change default data segment and data class names
#define SHARED __attribute__((section(".shr"), shared))

#include "windows.h"

SHARED HWND hVeneto = NULL;
SHARED UINT UpdateRectMessage = 0;
SHARED UINT CopyRectMessage = 0;
SHARED UINT MouseMoveMessage = 0;
SHARED UINT LocalMouseMessage = 0;
SHARED UINT LocalKeyboardMessage = 0;
SHARED HHOOK hCallWndHook = NULL;							// Handle to the CallWnd hook
SHARED HHOOK hGetMsgHook = NULL;							// Handle to the GetMsg hook
SHARED HHOOK hDialogMsgHook = NULL;						// Handle to the DialogMsg hook
SHARED HHOOK hLLKeyboardHook = NULL;						// Handle to LowLevel kbd hook
SHARED HHOOK hLLMouseHook = NULL;							// Handle to LowLevel mouse hook
SHARED HHOOK hLLKeyboardPrHook = NULL;						// Handle to LowLevel kbd hook for local event priority
SHARED HHOOK hLLMousePrHook = NULL;						// Handle to LowLevel mouse hook for local event priority
SHARED HHOOK hKeyboardHook = NULL;							// Handle to kbd hook
SHARED HHOOK hMouseHook = NULL;							// Handle to mouse hook
SHARED HWND hKeyboardPriorityWindow = NULL;
SHARED HWND hMousePriorityWindow = NULL;

