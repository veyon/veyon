//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
//
// Functions to hide the Windows Desktop
//
// This hides three variants:
//	- Desktop Patterns  (WIN.INI [Desktop] Pattern=)
//	- Desktop Wallpaper (.bmp [and JPEG on Windows XP])
//	- Active Desktop
//
// Written by Ed Hardebeck - Glance Networks, Inc.
// With some code from Paul DiLascia, MSDN Magazine - May 2001
//

#ifndef _G_HIDEDESKTOP_H_
#define _G_HIDEDESKTOP_H_

// Everything

void HideDesktop();				// hide it
void RestoreDesktop();			// restore it  (warning: uses some global state set by HideDesktop)

// Just the Active Desktop

bool HideActiveDesktop();		// returns true if the Active Desktop was enabled and has been hidden
void ShowActiveDesktop();		// show it always (if Shell version >= 4.71)

// adzm - 2010-07 - Disable more effects or font smoothing
// UI Effects

void DisableEffects();
void EnableEffects();

void DisableFontSmoothing();
void EnableFontSmoothing();

#endif
