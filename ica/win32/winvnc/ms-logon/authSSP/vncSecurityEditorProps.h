/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/

// This table maps permissions onto strings.
// Notice the first entry, which includes all permissions; it maps to a string "Full Control".
// I love this feature of being able to provide multiple permissions in one entry,
// if you use it carefully, it's pretty powerful - the access control editor will
// automatically keep the permission checkboxes synchronized, which makes it clear
// to the user what is going on (if they pay close enough attention).
//
// Notice that I included standard permissions - don't forget them...
// Notice that I left some of the more esoteric permissions for the Advanced dialog,
// and that I only show "Full Control" in the basic permissions editor.
// This is consistent with the way the file acl editor works.
SI_ACCESS g_vncAccess[] = {	
	// these are a easy-to-swallow listing of basic rights for VNC
//	{ &GUID_NULL, 0x00000007, L"Full control", SI_ACCESS_GENERAL  },	// Full Control
	{ &GUID_NULL, 0x00000003, L"Full control", SI_ACCESS_GENERAL  },	// Full Control
	{ &GUID_NULL, 0x00000003, L"Interact",     SI_ACCESS_GENERAL  },	// Write
	{ &GUID_NULL, 0x00000001, L"View",     SI_ACCESS_GENERAL  },	// Read
//	{ &GUID_NULL, 0x00000004, L"Filetransfer",     SI_ACCESS_GENERAL  },	// ??
};

// Here's my crufted-up mapping for VNC generic rights
GENERIC_MAPPING g_vncGenericMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	STANDARD_RIGHTS_REQUIRED
};

const DWORD ViewOnly	   = 0x0001;
const DWORD Interact	   = 0x0002;

const DWORD GenericRead    = STANDARD_RIGHTS_READ |
							 ViewOnly;

const DWORD GenericWrite   = STANDARD_RIGHTS_WRITE |
							 Interact;

const DWORD GenericExecute = STANDARD_RIGHTS_EXECUTE;

const DWORD GenericAll     = STANDARD_RIGHTS_REQUIRED |
							  GenericRead |
							  GenericWrite |
							  GenericExecute;

