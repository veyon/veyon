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
 * vncImportACL.h: 
 */
#define _WIN32_WINNT	0x0500	//??
#define WINVER		0x0500	//??

#include <windows.h>
#include <aclapi.h>
#include <stdio.h>
#include <string.h>
#include <lm.h>

#define MAXLEN 256
#define lenof(a) (sizeof(a) / sizeof((a)[0]) )


#define ViewOnly	1
#define Interact	2
#define ALL_RIGHTS (ViewOnly | Interact)

class vncImportACL {
private:
	enum ACE_TYPE {
		ACCESS_ALLOWED,
			ACCESS_DENIED
	};
	
	typedef struct _ACE_DATA {
		ACE_TYPE type;
		DWORD mask;
		PSID pSID;
		struct _ACE_DATA *next;
	} ACE_DATA;

	ACE_DATA *lastAllowACE;
	ACE_DATA *lastDenyACE;

public:	
	vncImportACL();
	~vncImportACL();

	void GetOldACL();
	void ReadAce(int index, PACL pACL);
	PSID CopySID(PSID pSID);
	int ScanInput();
	bool FillAceData(const TCHAR *accesstype, 
		DWORD accessmask, 
		const TCHAR *domainaccount);
	PSID GetSID(const TCHAR *domainaccount);
	PACL BuildACL(void);
	bool SetACL(PACL pACL);
	const TCHAR * SplitString(const TCHAR *input, TCHAR separator, TCHAR *head);
	TCHAR *AddComputername(const TCHAR *user);
	TCHAR *AddDomainname(const TCHAR *user);
};

struct vncScanInput{
	bool isEmptyLine(TCHAR *line);
	void RemoveComment(TCHAR *line);
	void RemoveNewline(TCHAR *line);
	int GetQuoteLength(const TCHAR *line);
	int GetWordLength(const TCHAR *line);
	bool GetLine(TCHAR *line);
	bool Tokenize(const TCHAR *line, TCHAR **tokens);
	int AddToken(TCHAR **token, int tokenCount, const TCHAR **line, int len);
};
