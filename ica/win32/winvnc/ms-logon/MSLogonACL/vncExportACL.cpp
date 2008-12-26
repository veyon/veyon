/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//  Based on Felix Kasza's DumpACL (http://win32.mvps.org/security/dumpacl.html).
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
#include <wchar.h>
#include <tchar.h>
#include "vncExportACL.h"

// Get ACL from regkey HKLM\Software\ORL\WinVNC3\ACL
bool vncExportACL::GetACL(PACL *pACL){
	HKEY hk = NULL; 
	DWORD dwValueLength = SECURITY_DESCRIPTOR_MIN_LENGTH;
	
	_ftprintf(stderr, _T("== Entering GetACL\n"));
	__try{
		
		if (ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			_T("Software\\ORL\\WinVNC3"),
			0, KEY_QUERY_VALUE, &hk )){
			_ftprintf(stderr, _T("== Error %d: RegOpenKeyEx\n"), GetLastError());
			__leave;
		}
		// Read the ACL value from the VNC registry key
		// First call to RegQueryValueEx just gets the buffer length.
		if (ERROR_SUCCESS != RegQueryValueEx(hk,	// subkey handle 
            _T("ACL"),				// value name 
            0,                      // must be zero 
            0,						// value type , not needed here
			NULL,					// 
            &dwValueLength)){       // length of value data 
			_ftprintf(stderr, _T("== Error %d: RegQueryValueEx 1\tValueLength = %d\n"), 
				GetLastError(), dwValueLength);
			__leave;
		}
		*pACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueLength);
		
		if (ERROR_SUCCESS != RegQueryValueEx(hk,	// subkey handle 
            _T("ACL"),								// value name 
            0,										// must be zero 
            0,										// value type 
			(LPBYTE) *pACL,
            &dwValueLength)){						// length of value data 
			_ftprintf(stderr, _T("== Error %d: RegQueryValueEx 2\n"), GetLastError());
			__leave;
		}
		_ftprintf(stderr, _T("== RegQueryValueEx passed\tdwValueLength = %d\n"), dwValueLength);

	} __finally {
		if (hk)
			RegCloseKey(hk); 
	}
	return true;
}



const TCHAR *vncExportACL::SidToText( PSID psid )
{
	// S-rev- + SIA + subauthlen*maxsubauth + terminator
	static TCHAR buf[15 + 12 + 12*SID_MAX_SUB_AUTHORITIES + 1];
	TCHAR *p = &buf[0];
	PSID_IDENTIFIER_AUTHORITY psia;
	DWORD numSubAuths, i;

	// Validate the binary SID.

	if (!IsValidSid(psid))
		return FALSE;

	psia = GetSidIdentifierAuthority(psid);

	p = buf;
	p += _sntprintf(p, &buf[sizeof buf] - p, _T("S-%lu-"), 0x0f & *((byte *)psid));

	if ((psia->Value[0] != 0) || (psia->Value[1] != 0))
		p += _sntprintf( p, &buf[sizeof buf] - p, _T("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
			(USHORT) psia->Value[0], (USHORT) psia->Value[1],
			(USHORT) psia->Value[2], (USHORT) psia->Value[3],
			(USHORT) psia->Value[4], (USHORT) psia->Value[5] );
	else
		p += _sntprintf(p, &buf[sizeof buf] - p, _T("%lu"), (ULONG) (psia->Value[5]) +
			(ULONG) (psia->Value[4] << 8) + (ULONG) (psia->Value[3] << 16) +
			(ULONG) (psia->Value[2] << 24));

	// Add SID subauthorities to the string.

	numSubAuths = *GetSidSubAuthorityCount(psid);
	for (i = 0; i < numSubAuths; ++ i)
		p += _sntprintf(p, &buf[sizeof buf] - p, _T("-%lu"), *GetSidSubAuthority(psid, i));

	return buf;
}

void vncExportACL::PrintSid(PSID psid)
{
	TCHAR name[256], domain[256];
	DWORD cbname = sizeof name, cbdomain = sizeof domain, rc;
	SID_NAME_USE sidUse;

	//!! next line has hardcoded server name !!
	// NULL server name is usually appropriate, though.
	if (LookupAccountSid(NULL, psid, name, &cbname, domain, &cbdomain, &sidUse))
	{
		//Todo: Check if WellKnownSID and reserve special names for them.

/*		switch ( sidUse )
		{
			case SidTypeWellKnownGroup:	type = "well-known group"; break;
			default:					type = "bad sidUse value"; break;
		}
*/
		LPWKSTA_INFO_100 wkstainfo = NULL;
		NET_API_STATUS nStatus;
		TCHAR langroup[MAXLEN];
		TCHAR computername[MAXLEN];

		nStatus = NetWkstaGetInfo( 0 , 100 , (LPBYTE *) &wkstainfo);
		if (nStatus == NERR_Success){
			_tcsncpy(langroup, wkstainfo->wki100_langroup, MAXLEN);
			_tcsncpy(computername, wkstainfo->wki100_computername, MAXLEN);
			langroup[MAXLEN - 1] = _T('\0');
			computername[MAXLEN - 1] = _T('\0');
			// replace computername with a dot
			if (_tcsicmp(computername, domain) == 0)
				_tcscpy(domain,_T("."));
			else if (_tcsicmp(langroup, domain) == 0)
				_tcscpy(domain, _T(".."));
		}
		else
			_tprintf(_T("NetWkstaGetInfo() returned %lu \n"), wkstainfo);

		NetApiBufferFree(wkstainfo);
		wkstainfo = NULL;

		// If domain or username contains whitespace, print enclosing quotes
		if (_tcschr(domain, _T(' ')) || _tcschr(name, _T(' ')))
			_tprintf(_T("\"%s%s%s\"\n"), domain, (domain == 0 || *domain == _T('\0'))? _T(""): _T("\\"), name);
		else
			_tprintf(_T("%s%s%s\n"), domain, (domain == 0 || *domain == _T('\0'))? _T(""): _T("\\"), name);
	}
	else
	{
		rc = GetLastError();
		_tprintf(_T("[%s] *** error %lu\n"), SidToText( psid ), rc);
	}
}

void vncExportACL::PrintAce(int index, PACL acl){

	ACE_HEADER *ace;
	TCHAR *type;
	PSID psid;

	if (!GetAce(acl, index, (void **) &ace)) {
		_tprintf(_T("DACL, entry %d: GetAce() failed, gle == %lu\n"),
			index, GetLastError());
		return;
	}

	switch ( ace->AceType ) {
		case ACCESS_ALLOWED_ACE_TYPE:
			type = _T("allow");
			psid = &((ACCESS_ALLOWED_ACE *) ace)->SidStart;
			break;
		case ACCESS_DENIED_ACE_TYPE:
			type = _T("deny");
			psid = &((ACCESS_DENIED_ACE *) ace)->SidStart;
			break;
		default:
			type = _T("invalid");
			psid = &((ACCESS_ALLOWED_ACE *) ace)->SidStart;
			break;
	}
	_tprintf(_T("%s\t"), type);

	_tprintf(_T("0x%08lX\t"), ((ACCESS_ALLOWED_ACE *) ace)->Mask);

	PrintSid( psid );
}



void vncExportACL::PrintAcl(PACL acl)
{
	DWORD i;
	ACL_SIZE_INFORMATION aci;

	if ( acl == 0 )
		return;

	if (!GetAclInformation( acl, &aci, sizeof aci, AclSizeInformation))	{
		_tprintf(_T("GAI(): gle == %lu\n"), GetLastError());
		return;
	}

	for ( i = 0; i < aci.AceCount; ++ i )
		PrintAce(i, acl);

}



