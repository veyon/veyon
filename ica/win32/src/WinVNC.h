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
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// WinVNC header file

#ifdef HORIZONLIVE
#include "horizon/horizonMain.h"
#else

#ifndef __WINVNC_H
#define __WINVNC_H

#include "stdhdrs.h"
#include "resource.h"
#include "VNCHelp.h"

// Application specific messages

// Message used for system tray notifications
#define WM_TRAYNOTIFY				WM_USER+1

// Messages used for the server object to notify windows of things
#define WM_SRV_CLIENT_CONNECT		WM_USER+2
#define WM_SRV_CLIENT_AUTHENTICATED	WM_USER+3
#define WM_SRV_CLIENT_DISCONNECT	WM_USER+4
#define WM_SRV_CLIENT_HIDEWALLPAPER	WM_USER+5

#define WINVNC_REGISTRY_KEY "Software\\ORL\\WinVNC3"

// Export the application details
extern HINSTANCE	hAppInstance;
extern const char	*szAppName;
extern DWORD		mainthreadId;
extern VNCHelp		help;
// Main VNC server routine
extern int WinVNCAppMain();

// Standard command-line flag definitions
const char winvncRunService[]		= "-service";
const char winvncRunServiceHelper[]	= "-servicehelper";
const char winvncRunAsUserApp[]		= "-run";

const char winvncInstallService[]	= "-install";
const char winvncRemoveService[]	= "-remove";
const char winvncReinstallService[]	= "-reinstall";

const char winvncReload[]			= "-reload";
const char winvncShowProperties[]	= "-settings";
const char winvncShowDefaultProperties[]	= "-defaultsettings";
const char winvncShowAbout[]		= "-about";
const char winvncKillRunningCopy[]	= "-kill";

const char winvncShareAll[]			= "-shareall";
const char winvncSharePrimary[]		= "-shareprimary";
const char winvncShareArea[]		= "-sharearea";
const char winvncShareWindow[]		= "-sharewindow";

const char winvncAddNewClient[]		= "-connect";
const char winvncKillAllClients[]	= "-killallclients";

const char winvncShowHelp[]			= "-help";

// Usage string
const char winvncUsageText[] =
	"winvnc [-run] [-kill] [-service] [-servicehelper]\n"
	" [-connect [host[:display]]] [-connect [host[::port]]]\n"
	" [-install] [-remove] [-reinstall] [-reload]\n"
	" [-settings] [-defaultsettings] [-killallclients]\n"
	" [-shareall] [-shareprimary] [-sharearea WxH+X+Y]\n"
	" [-sharewindow  \"title\"] [-about] [-help]\n";

#endif // __WINVNC_H

#endif // HORIZONLIVE
