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
// http://www.uvnc.com
#include <wchar.h>
#include <tchar.h>
#include "vncImportACL.h"

vncImportACL::vncImportACL(){
	lastAllowACE = NULL;
	lastDenyACE = NULL;
}

vncImportACL::~vncImportACL(){
	_ftprintf(stderr, _T("deleting ACE_DATA linked lists\n"));
	ACE_DATA *tmp = NULL;
	for (ACE_DATA *i = lastDenyACE; i != NULL; i = tmp){
		if (i->pSID)
			delete i->pSID;
		tmp = i->next;
		delete i;
	}
	for (ACE_DATA *j = lastAllowACE; j != NULL; j = tmp){
		if (j->pSID)
			delete j->pSID;
		tmp = j->next;
		delete j;
	}

}

void vncImportACL::GetOldACL(){
	PACL pOldACL = NULL;
	ACL_SIZE_INFORMATION asi;
	HKEY hk = NULL; 
	DWORD dwValueLength = SECURITY_DESCRIPTOR_MIN_LENGTH;
	
	__try{
		
		if (ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			_T("Software\\ORL\\WinVNC3"),
			0, KEY_QUERY_VALUE, &hk )){
			__leave;
		}
		// Read the ACL value from the VNC registry key
		// First call to RegQueryValueEx just gets the buffer length.
		if (ERROR_SUCCESS != RegQueryValueEx(hk,	// subkey handle 
            _T("ACL"),					// value name 
            0,                      // must be zero 
            0,						// value type , not needed here
			NULL,					// 
            &dwValueLength)){       // length of value data 
			__leave;
		}
		pOldACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueLength);
		
		if (ERROR_SUCCESS != RegQueryValueEx(hk,	// subkey handle 
            _T("ACL"),					// value name 
            0,										// must be zero 
            0,										// value type 
			(LPBYTE) pOldACL,
            &dwValueLength)){						// length of value data 
			__leave;
		}

	} __finally {
		if (hk)
			RegCloseKey(hk); 
	}

	if ( pOldACL == NULL )
		return;

	if (!GetAclInformation( pOldACL, &asi, sizeof asi, AclSizeInformation))	{
		_ftprintf(stderr, _T("GAI(): gle == %lu\n"), GetLastError());
		return;
	}

	for (DWORD i = 0; i < asi.AceCount; ++ i)
		ReadAce(i, pOldACL);

	HeapFree(GetProcessHeap(), 0, pOldACL);
}

void vncImportACL::ReadAce(int index, PACL pACL){
	ACE_HEADER *ace;
	//char *type;
	//PSID psid;
	ACE_DATA * pACEdata;

	if (!GetAce( pACL, index, (void **) &ace)) {
		_ftprintf(stderr, _T("DACL, entry %d: GetAce() failed, gle == %lu\n"),
			index, GetLastError());
		return;
	}

	pACEdata = new ACE_DATA;
	switch ( ace->AceType ) {
		case ACCESS_ALLOWED_ACE_TYPE:
			pACEdata->type = ACCESS_ALLOWED;
			pACEdata->pSID = CopySID(&((ACCESS_ALLOWED_ACE *) ace)->SidStart);
			pACEdata->mask = ((ACCESS_ALLOWED_ACE *) ace)->Mask;
			pACEdata->next = lastAllowACE;
			lastAllowACE   = pACEdata;
			break;
		case ACCESS_DENIED_ACE_TYPE:
			pACEdata->type = ACCESS_DENIED;
			pACEdata->pSID = CopySID(&((ACCESS_DENIED_ACE *) ace)->SidStart);
			pACEdata->mask = ((ACCESS_DENIED_ACE *) ace)->Mask;
			pACEdata->next = lastDenyACE;
			lastDenyACE   = pACEdata;
			break;
		default:
			_ftprintf(stderr, _T("Error: Unknown ACE type\n"));
			delete pACEdata;
			break;
	}
}

PSID vncImportACL::CopySID(PSID pSID){
	PSID pNewSID = NULL;
	DWORD sidLength = 0;

	if (IsValidSid(pSID)){
		sidLength = GetLengthSid(pSID);
		pNewSID = (PSID) new char[sidLength];
		CopySid(sidLength, pNewSID, pSID);
	}
	return pNewSID;
}

int vncImportACL::ScanInput(){
	int rc = 0;
	vncScanInput scanInput;

	do {
		TCHAR linebuf[MAXLEN];
		TCHAR *line = linebuf;
		TCHAR *token[3];
		TCHAR accesstype[MAXLEN];
		TCHAR accessmaskstring[MAXLEN];
		TCHAR domainaccount[MAXLEN];
		DWORD accessmask = 0;
		token[0] = accesstype;
		token[1] = accessmaskstring;
		token[2] = domainaccount;

		if (!scanInput.GetLine(line)) {
			rc |= 1;
			break;
		}
		scanInput.RemoveNewline(line);
		scanInput.RemoveComment(line);
		if (scanInput.isEmptyLine(line))
			continue;
		if (scanInput.Tokenize(line, token)
			&& _stscanf(accessmaskstring, _T("%x"), &accessmask) == 1) {
			FillAceData(accesstype, accessmask, domainaccount);
		} else {
			_tprintf(_T("Error tokenizing line\n"));
			rc |= 2;
		}
	} while(!feof(stdin));
	return rc;
}

PACL vncImportACL::BuildACL(){
	PACL pACL = NULL;
	// Need canonical order and normalization??
	// Solution canonical order: aceAllowStart and aceDenyStart?
	// Solution normalization: Check for multiple occurrance of SID
	// in allow list and deny list and merge them.
	long aclSize = 8; // For ACL header

	for (ACE_DATA *i = lastDenyACE; i != NULL; i = i->next){
		aclSize += GetLengthSid(i->pSID) + sizeof(ACCESS_DENIED_ACE) - sizeof(DWORD);
	}
	for (ACE_DATA *j = lastAllowACE; j != NULL; j = j->next){
		aclSize += GetLengthSid(j->pSID) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD);
	}

	pACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,	aclSize);
	
	// Initialize the new ACL.
	if (InitializeAcl(pACL, aclSize, ACL_REVISION)) {
		// Add the access-denied ACEs to the ACL.
		for (ACE_DATA *i = lastDenyACE; i != NULL; i = i->next){
			//Todo: add error handling.
			AddAccessDeniedAce(pACL, ACL_REVISION, i->mask, i->pSID);
		}
		for (ACE_DATA *j = lastAllowACE; j != NULL; j = j->next){
			//Todo: add error handling.
			AddAccessAllowedAce(pACL, ACL_REVISION, j->mask, j->pSID);
		}
	}
	return pACL;
}

bool vncImportACL::SetACL(PACL pACL){
	HKEY hk = NULL; 
	bool isSaveOK = false;
	DWORD  dwDisposition;

    ACL_SIZE_INFORMATION AclInformation = {0, 0, 0};
	DWORD nAclInformationLength = sizeof(AclInformation);

	// Todo: Better error handling
	if (pACL)
		GetAclInformation(pACL, &AclInformation, nAclInformationLength, AclSizeInformation);

	__try{
		//if (ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		//	_T("Software\\ORL\\WinVNC3"),
		//	0, KEY_SET_VALUE, &hk )){
		//	_ftprintf(stderr, _T("Error %d: RegOpenKeyEx\n"), GetLastError());
		//	__leave;
		//}
		
		if (ERROR_SUCCESS != RegCreateKeyEx( HKEY_LOCAL_MACHINE,
			_T("Software\\ORL\\WinVNC3"),
			0, NULL, 0, KEY_SET_VALUE, NULL, &hk, &dwDisposition )){
			_ftprintf(stderr, _T("Error %d: RegOpenKeyEx\n"), GetLastError());
			__leave;
		}

		// Add the ACL value to the VNC registry key
		if (ERROR_SUCCESS != RegSetValueEx(hk,
            _T("ACL"),
            0,
            REG_BINARY,
			(LPBYTE) pACL,
            AclInformation.AclBytesInUse)){
			_ftprintf(stderr, _T("Error %d: RegSetValueEx\n"), GetLastError());
			__leave;
		}
		isSaveOK = true;
		_ftprintf(stderr, _T("RegSetValueEx passed\n"));
	} __finally {
		if (hk)
			RegCloseKey(hk); 
	}
	return isSaveOK;
}

bool vncImportACL::FillAceData(const TCHAR *type, 
				 DWORD mask, 
				 const TCHAR *user){
	ACE_TYPE acetype;
	TCHAR *domainaccount = NULL;
	PSID pSID = NULL;

	//type
	if (_tcsicmp(type, _T("allow")) == 0)
		acetype = ACCESS_ALLOWED;
	else if (_tcsicmp(type, _T("deny")) == 0)
		acetype = ACCESS_DENIED;
	else {
		_ftprintf(stderr, _T("error with ACE type, neither 'allow' nor 'deny'\n"));
		return false;
	}

	//mask
	mask &= ALL_RIGHTS;

	// if name starts with ".\", replace the dot with the computername.
	// if name starts with "..\", replace the dots with the domainname.
	if (_tcsncmp(user, _T(".\\"), 2) == 0)
		domainaccount = AddComputername(user);
	else if  (_tcsncmp(user, _T("..\\"), 2) == 0)
		domainaccount = AddDomainname(user);
	else {
		domainaccount = new TCHAR[_tcslen(user)+1];
		_tcscpy(domainaccount,user);
	}
	pSID = GetSID(domainaccount);
	if (!pSID)
		return false;
	_tprintf(_T("account: %s, mask: %x, type: %s\n"), 
		domainaccount, mask, acetype == ACCESS_ALLOWED ? _T("allow") : _T("deny"));
	ACE_DATA *ace = new ACE_DATA;
	ace->type = acetype;
	ace->mask = mask;
	ace->pSID = pSID;
	switch (acetype){
	case ACCESS_ALLOWED:
		ace->next = lastAllowACE;
		lastAllowACE = ace;
		break;
	case ACCESS_DENIED:
		ace->next = lastDenyACE;
		lastDenyACE = ace;
		break;
	default:
		break;
	}

	delete [] domainaccount;
	domainaccount = NULL;

	return true; // Needs better error signalling.
}

PSID vncImportACL::GetSID(const TCHAR *domainaccount){
	PSID pSid = NULL;
	unsigned long ulDomLen = 0;
	unsigned long ulSidLen = 0;
	SID_NAME_USE peUse;
	TCHAR *domain = NULL;

	// Get accounts's SID
	// First call to LookupAccountName is expected to fail.
	// Sets pSid and domain size
	LookupAccountName(NULL, domainaccount, pSid, &ulSidLen,
		domain, &ulDomLen, &peUse);
	pSid = (PSID)new TCHAR[ulSidLen];
	domain = new TCHAR[ulDomLen];
	LookupAccountName(NULL, domainaccount,
		pSid, &ulSidLen, domain, &ulDomLen, &peUse);
	if (!IsValidSid(pSid)){
		_ftprintf(stderr, _T("%s: SID not valid.\n"), domainaccount);
		delete [] pSid;
				pSid = NULL;
	}
	delete [] domain;

	return pSid;
}

// SplitString splits a string 'input' on the first occurence of char 'separator'
// into string 'head' and 'tail' (removing the separator).
// If separator is not found, head = "" and tail = input.
const TCHAR * vncImportACL::SplitString(const TCHAR *input, TCHAR separator, TCHAR *head){
	const TCHAR * tail;
	int l;

	tail = _tcschr(input, separator);
	if (tail){
		l = tail - input;
		// get rid of separator
		tail = tail + 1; 
		_tcsncpy(head, input, l);
		head[l] = _T('\0');
	} else {
		tail   = input;
		head[0] = _T('\0');
	}
	return tail;
}

TCHAR *vncImportACL::AddComputername(const TCHAR *user){
	unsigned long buflen = MAXLEN;
	TCHAR computername[MAXLEN];
	if (!GetComputerName(computername, &buflen)){
		_ftprintf(stderr, _T("GetComputerName error %i\n"), GetLastError());
		return NULL;
	}
	_ftprintf(stderr, _T("Detected computername = %s\n"), computername);
	// Length of computername and user minus beginning dot plus terminating '\0'.
	TCHAR *domainaccount = new TCHAR[_tcslen(computername) + _tcslen(user)];
	_tcscpy(domainaccount, computername);
	_tcscat(domainaccount, user + 1);

	return domainaccount;
}

TCHAR *vncImportACL::AddDomainname(const TCHAR *user){
	unsigned long buflen = MAXLEN;
	TCHAR domain[MAXLEN * sizeof(wchar_t)];
	LPWKSTA_INFO_100 wkstainfo = NULL;
	NET_API_STATUS nStatus;

	nStatus = NetWkstaGetInfo( 0 , 100 , (LPBYTE *) &wkstainfo);
	if (nStatus == NERR_Success)
		_tcsncpy(domain, wkstainfo->wki100_langroup, MAXLEN);
	else
		_ftprintf(stderr, _T("NetWkstaGetInfo() returned %lu \n"), wkstainfo);
	domain[MAXLEN - 1] = _T('\0');
	_ftprintf(stderr, _T("Detected domain = %s\n"),domain);
	NetApiBufferFree(wkstainfo);
	wkstainfo = NULL;

	// Length of domainname and user minus beginning dots plus terminating '\0'.
	TCHAR *domainaccount = new TCHAR[_tcslen(domain) + _tcslen(user) - 1];
	_tcscpy(domainaccount, domain);
	_tcscat(domainaccount, user + 2);

	return domainaccount;
}

// vncScanInput struct

bool vncScanInput::GetLine(TCHAR *line){
	line[0] = _T('\0');
	_fgetts(line, MAXLEN, stdin);
	return ferror(stdin) ? false : true;
}

bool vncScanInput::isEmptyLine(TCHAR *line){
	for (unsigned int i = 0; i < _tcslen(line); i++)
		if (line[i] != _T(' ') && line[i] != _T('\t'))
			return false;
	return true;
}

void vncScanInput::RemoveComment(TCHAR *line){
	if (TCHAR *comment = _tcschr(line, _T(';')))
		*comment = _T('\0');
}

void vncScanInput::RemoveNewline(TCHAR *line){
	if (line[_tcslen(line)-1] == _T('\n'))
		line[_tcslen(line)-1] = _T('\0');
}

bool vncScanInput::Tokenize(const TCHAR *line, TCHAR **token){
	int tokenCount = 0;

	// Loop until the end of line or until 4th token found
	for (; _tcslen(line) > 0 && tokenCount < 4;) {
		int len = 0;
		switch (*line) {
		case _T(' ') :
		case _T('\t'): 
			line++; // Eat whitespace
			break;
		case _T('"') : 
			line++; // Eat opening quote
			len = GetQuoteLength(line);
			tokenCount = AddToken(token, tokenCount, &line, len);
			line++; // Eat closing quote
			break;
		default:
			len = GetWordLength(line);
			tokenCount = AddToken(token, tokenCount, &line, len);
			break;
		}
	}
	return (tokenCount == 3 && _tcslen(line) == 0) ? true : false;
}

int vncScanInput::GetWordLength(const TCHAR *line){
	int length = 0;
	int blength = 0;
	int tlength = 0;
	
	const TCHAR *nextblank = _tcschr(line, _T(' '));
	const TCHAR *nexttab   = _tcschr(line, _T('\t'));
	if (nextblank) 
		blength = nextblank - line;
	if (nexttab)
		tlength = nexttab - line;
	if (nextblank && nexttab) // Found blank and tab
		length = __min(blength, tlength);
	else if (nextblank) // Found only a blank
		length = blength;
	else if (nexttab) // Found only a tab
		length = tlength;
	if (!length) // Found neither blank nor tab
		length = _tcslen(line);
	return length;
}

int vncScanInput::GetQuoteLength(const TCHAR *line){
	const TCHAR *eoq = _tcschr(line, _T('"'));
	int len = 0;
	if (eoq)
		len = eoq - line;
	return len;
}

int vncScanInput::AddToken(TCHAR **token, int tokenCount, const TCHAR **line, int len){
	if (len) {
		if (tokenCount < 3) {
			_tcsncpy(token[tokenCount], *line, len);
			token[tokenCount][len] = _T('\0');
		}
		*line += len; // Eat token
		tokenCount++;
	}
	return tokenCount;
}