/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
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
/* 
 *  vncExportACL.h
 */

#define _WIN32_WINNT	0x0500
#define WINVER			0x0500

#include <windows.h>
#include <aclapi.h>
#include <stdio.h>
#include <string.h>
#include <lm.h>

#define MAXLEN 256
#define lenof(a) (sizeof(a) / sizeof((a)[0]) )

class vncExportACL{
public:
	// returns the address of a !!static!!, non-thread-local, buffer with
	// the text representation of the SID that was passed in
	const TCHAR *SidToText( PSID psid );
	
	// Translates a SID and terminates it with a linefeed. No provision is
	// made to dump the SID in textual form if LookupAccountSid() fails.
	void PrintSid( PSID psid );
	
	// Displays the index-th (0-based) ACE from ACL
	void PrintAce(int index, PACL acl);
	
	// Dumps an entire ACL
	void PrintAcl(PACL acl);
	
	bool GetACL(PACL *pACL);
	
};