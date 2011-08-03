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


// vncService

// SERVICE-MODE CODE

// This class provides access to service-oriented routines, under both
// Windows NT and Windows 95.  Some routines only operate under one
// OS, others operate under any OS.

class vncService;

#if (!defined(_WINVNC_VNCSERVICE))
#define _WINVNC_VNCSERVICE

#include "stdhdrs.h"

BOOL PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam);

//adzm 2010-02-10 - Only posts to the same process
BOOL PostToThisWinVNC(UINT message, WPARAM wParam, LPARAM lParam);

//adzm 2010-02-10 - Finds the appropriate VNC window
HWND FindWinVNCWindow(bool bThisProcess);


// The NT-specific code wrapper class
class vncService
{

public:
	vncService();
	// Routine to establish and return the currently logged in user name
	static BOOL CurrentUser(char *buffer, UINT size);
	static BOOL IsWSLocked(); // sf@2005

	// Routines to establish which OS we're running on
	static BOOL IsWin95();
	static BOOL IsWinNT();
	static DWORD VersionMajor();
	static DWORD VersionMinor();

	// Routine to establish whether the current instance is running
	// as a service or not
	static BOOL RunningAsService();

	// Routine to kill any other running copy of WinVNC
	static BOOL KillRunningCopy();

	// Routine to set the current thread into the given desktop
	static BOOL SelectHDESK(HDESK newdesktop);

	// Routine to set the current thread into the named desktop,
	// or the input desktop if no name is given
	static BOOL SelectDesktop(char *name, HDESK *desktop);

	// Routine to switch the service process across to the currently
	// visible Window Station and back to its home window station again

	// Routine to establish whether the current thread desktop is the
	// current user input one
	static int InputDesktopSelected();

	// Routine to fake a CtrlAltDel to winlogon when required.
	// *** This is a nasty little hack...
	static BOOL SimulateCtrlAltDel();

	// Routine to lock the workstation.  Returns TRUE if successful.
	// Main cause of failure will be when locking is not supported
	static BOOL LockWorkstation();

	// Routine to make the an already running copy of WinVNC bring up its
	// About box so you can check the version!
	static BOOL ShowAboutBox();

	// Routine to make an already running copy of WinVNC form an outgoing
	// connection to a new VNC client
	static BOOL PostAddNewClient(unsigned long ipaddress, unsigned short port);
	static BOOL PostAddNewClientInit(unsigned long ipaddress, unsigned short port);

	//adzm 2009-06-20
	// Static routine to tell a locally-running instance of the server
	// to prompt for a new ID to connect out to the repeater
	static BOOL PostAddNewRepeaterClient();
	
	// Routine to make an already running copy of WinVNC deal with Auto Reconnect
	// along with an ID
	static BOOL PostAddAutoConnectClient( const char* pszId );
	static BOOL PostAddConnectClient( const char* pszId );
	static BOOL PostAddStopConnectClient();
	static BOOL PostAddStopConnectClientAll();

	static BOOL RunningFromExternalService();
	static void RunningFromExternalService(BOOL fEnabled);
	static bool IsInstalled();

};

#endif