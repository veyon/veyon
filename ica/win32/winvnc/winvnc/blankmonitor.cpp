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

#include "stdhdrs.h"
#include "vncdesktop.h"
#include "vncdesktopthread.h"
#include "vncOSVersion.h"

DWORD WINAPI BlackWindow(LPVOID lpParam);

void vncDesktop::SetBlankMonitor(bool enabled)
{

	// Also Turn Off the Monitor if allowed ("Blank Screen", "Blank Monitor")
	if (m_server->BlankMonitorEnabled())
    {
	    if (enabled)
	    {
			if (VNCOS.OS_AERO_ON) VNCOS.DisableAero();
			Sleep(1000);
		    if (!VNCOS.CaptureAlphaBlending() || VideoBuffer())
		    {			   
			    SendMessage(m_hwnd,WM_SYSCOMMAND,SC_MONITORPOWER,(LPARAM)2);
				m_screen_in_powersave=true;
		    }
		    else
		    {
			    HANDLE ThreadHandle2=NULL;
			    DWORD dwTId;
			    ThreadHandle2 = CreateThread(NULL, 0, BlackWindow, NULL, 0, &dwTId);
			   if (ThreadHandle2)  CloseHandle(ThreadHandle2);
			    m_Black_window_active=true;
		    }
	    }
	    else // Monitor On
	    {			
		    if (!VNCOS.CaptureAlphaBlending() || VideoBuffer())
		    {			   
			    SendMessage(m_hwnd,WM_SYSCOMMAND,SC_MONITORPOWER,(LPARAM)-1);
				//win8 require mouse move
				mouse_event(MOUSEEVENTF_MOVE, 0, 1, 0, NULL);
				Sleep(40);
				mouse_event(MOUSEEVENTF_MOVE, 0, -1, 0, NULL);
				//JUst in case video driver state was changed
				HWND Blackhnd = FindWindow(("blackscreen"), 0);
			    if (Blackhnd) PostMessage(Blackhnd, WM_CLOSE, 0, 0);
				 m_screen_in_powersave=false;
		    }
		    else
		    {
			    HWND Blackhnd = FindWindow(("blackscreen"), 0);
			    if (Blackhnd) PostMessage(Blackhnd, WM_CLOSE, 0, 0);
			    m_Black_window_active=false;
		    }
			VNCOS.ResetAero();
	    }
    }
}