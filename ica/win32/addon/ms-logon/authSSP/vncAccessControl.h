/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define ViewOnly 0x0001
#define Interact 0x0002

// 
class vncAccessControl
{
public:
	PSECURITY_DESCRIPTOR GetSD();
	BOOL SetSD(PSECURITY_DESCRIPTOR pSD);

protected:
	void FreeSD(PSECURITY_DESCRIPTOR pSD);
	PACL GetACL(void);
	BOOL StoreACL(PACL pACL);
	PSID GetOwnerSID(void);
};

