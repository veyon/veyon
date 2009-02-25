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


// vncMenu

// This class handles creation of a system-tray icon & menu

class vncMenu;

#if (!defined(_WINVNC_VNCMENU))
#define _WINVNC_VNCMENU

#include "stdhdrs.h"
#include <lmcons.h>
#include "vncServer.h"
#include "vncProperties.h"
#include "vncPropertiesPoll.h"
#include "vncAbout.h"
#include "vncListDlg.h"

// Constants
extern const UINT MENU_ADD_CLIENT_MSG;
extern const UINT MENU_AUTO_RECONNECT_MSG;
extern const UINT MENU_REPEATER_ID_MSG;
extern const char *MENU_CLASS_NAME;

// WTS Stuff for FastUserSwitching management
typedef BOOL (WINAPI *WTSREGISTERSESSIONNOTIFICATION)(HWND, DWORD);
typedef BOOL (WINAPI *WTSUNREGISTERSESSIONNOTIFICATION)(HWND);
#define WM_WTSSESSION_CHANGE            0x02B1
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8
#define WTS_SESSION_REMOTE_CONTROL         0x9
#define NOTIFY_FOR_THIS_SESSION     0

extern const UINT FileTransferSendPacketMessage;



// The tray menu class itself
class vncMenu
{
public:
	vncMenu(vncServer *server);
	~vncMenu();

	void Shutdown(); // sf@2007

protected:
	// Tray icon handling
	void AddTrayIcon();
	void DelTrayIcon();
	void FlashTrayIcon(BOOL flash);
	void SendTrayMsg(DWORD msg, BOOL flash);

	// Message handler for the tray window
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	// Fields
protected:
	// Check that the password has been set
	void CheckPassword();

	// The server that this tray icon handles
	vncServer		*m_server;

	// Properties object for this server
	vncProperties	m_properties;
	vncPropertiesPoll	m_propertiesPoll;

	// About dialog for this server
	vncAbout		m_about;

	// List of viewers
	vncListDlg		m_ListDlg;

	HWND			m_hwnd;
	HMENU			m_hmenu;

	NOTIFYICONDATA	m_nid;

	char			m_username[UNLEN+1];

	// The icon handles
	HICON			m_winvnc_icon;
	HICON			m_flash_icon;

	HINSTANCE   hWTSDll;
	BOOL bConnectSock;
	BOOL bAutoPort;
	UINT port_rfb;
	UINT port_http;
	BOOL ports_set;

	bool IsIconSet;
	int IconFaultCounter;
	HMODULE hUser32;
	//HANDLE hEvent;

};


#endif