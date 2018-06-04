// XEventLog.cpp  Version 1.0
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//                                  NOTE
//              Change  C/C++ Precompiled Headers settings to
//              "Not using precompiled headers" for this file.
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// this file does not need any MFC
//#include "stdafx.h"

// if you are not using MFC, you must have the following three includes:
#include <windows.h>
#include <tchar.h>
#include <crtdbg.h>


#include "XEventLog.h"


///////////////////////////////////////////////////////////////////////////////
//
// ctor
//
// Purpose:     Construct CXEventLog object.
//
// Parameters:  lpszApp             - name of app (event log source).  This
//                                    parameter is optional. If it is not
//                                    specified, Init() must be called separately.
//              lpszEventMessageDll - fully-qualified name of event message
//                                    dll. Includes complete path and file
//                                    name. Example: "C:\\bin\\MyMessage.dll".
//                                    This parameter is optional.
//
// Returns:     None
//
CXEventLog::CXEventLog(LPCTSTR lpszApp /* = NULL*/,
					   LPCTSTR lpszEventMessageDll /* = NULL*/)
{
#ifdef _DEBUG
	if ((lpszApp == NULL) || (lpszApp[0] == _T('\0')))
	{
		TRACE(_T("=== No app specified in CXEventLog ctor. ")
			  _T("Be sure to call Init() before calling Write(). ===\n"));
	}
#endif

	m_hEventLog = NULL;
	m_pszAppName = NULL;

	// open event log
	if (lpszApp && (lpszApp[0] != _T('\0')))
		Init(lpszApp, lpszEventMessageDll);
}

///////////////////////////////////////////////////////////////////////////////
//
// dtor
//
// Purpose:     Destroy CXEventLog object.
//
// Parameters:  none
//
// Returns:     none
//
CXEventLog::~CXEventLog()
{
	Close();
	if (m_pszAppName)
		delete [] m_pszAppName;
	m_pszAppName = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// Close()
//
// Purpose:     Close event log handle.  Called automatically by dtor.
//
// Parameters:  none
//
// Returns:     none
//
void CXEventLog::Close()
{
	if (m_hEventLog)
		::DeregisterEventSource(m_hEventLog);
	m_hEventLog = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetAppName()
//
// Purpose:     Get name of currently registered app.
//
// Parameters:  none
//
// Returns:     LPTSTR - app name
//
LPTSTR CXEventLog::GetAppName()
{
	return m_pszAppName;
}

///////////////////////////////////////////////////////////////////////////////
//
// Init()
//
// Purpose:     Initialize the registry for event logging from this app.
//              Normally Init() is called from ctor.  If default ctor (no args)
//              is used, then Init() must be called before Write() is called.
//              Sets class variable m_hEventLog.
//
// Parameters:  lpszApp             - name of app (event log source).
//                                    MUST BE SPECIFIED.
//              lpszEventMessageDll - fully-qualified name of event message
//                                    dll. Includes complete path and file
//                                    name. Example: "C:\\bin\\MyMessage.dll".
//                                    This parameter is optional.
//
// Returns:     BOOL - TRUE = success
//
BOOL CXEventLog::Init(LPCTSTR lpszApp, LPCTSTR lpszEventMessageDll /* = NULL*/)
{
	_ASSERTE((lpszApp != NULL) && (lpszApp[0] != _T('\0')));
	if (!lpszApp || lpszApp[0] == _T('\0'))
		return FALSE;

	Close();		// close event log if already open

	SetAppName(lpszApp);

	BOOL bRet = RegisterSource(lpszApp, lpszEventMessageDll);
	_ASSERTE(bRet);

	if (bRet)
	{
		m_hEventLog = ::RegisterEventSource(NULL, lpszApp);
	}

	_ASSERTE(m_hEventLog != NULL);

	return (m_hEventLog != NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
// Write()
//
// Purpose:     Write string to event log
//
// Parameters:  wType - event type.  See ReportEvent() in MSDN for complete list.
//              lpszMessage - string to log
//
// Returns:     BOOL - TRUE = success
//
BOOL CXEventLog::Write(WORD wType, LPCTSTR lpszMessage)
{
	BOOL bRet = TRUE;

	_ASSERTE(m_hEventLog != NULL);
	if (!m_hEventLog)
	{
		// Init() not called
		return FALSE;
	}

	_ASSERTE(lpszMessage != NULL);
	if (!lpszMessage)
		return FALSE;

	_ASSERTE((wType == EVENTLOG_ERROR_TYPE)       ||
			 (wType == EVENTLOG_WARNING_TYPE)     ||
			 (wType == EVENTLOG_INFORMATION_TYPE) ||
			 (wType == EVENTLOG_AUDIT_SUCCESS)    ||
			 (wType == EVENTLOG_AUDIT_FAILURE));

	// get our user name information
	PSID pSid = GetUserSid();

	LPCTSTR* lpStrings = &lpszMessage;

	bRet = ::ReportEvent(m_hEventLog,		// event log source handle
						 wType,				// event type to log
						 0,					// event category
						 0x20000001L,		// event identifier (GENERIC_MESSAGE)
						 pSid,				// user security identifier (optional)
						 1,					// number of strings to merge with message
						 0,					// size of binary data, in bytes
						 lpStrings,			// array of strings to merge with message
						 NULL);				// address of binary data

	if (pSid)
		HeapFree(GetProcessHeap(), 0, pSid);

	return bRet;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//                          INTERNAL METHODS
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// RegisterSource() - INTERNAL METHOD
//
// Purpose:     Create entry in registry for message DLL.  This method will
//              create the following registry keys:
//                 HKLM
//                    SYSTEM
//                       CurrentControlSet
//                          Services
//                             Eventlog
//                                Application
//                                   <app name>
//                                       EventMessageFile = <complete path to message DLL>
//                                       TypesSupported   = 0x0000001f (31)
//
//              Note that <app name> is the lpszApp parameter, and
//              <complete path to message DLL> is the lpszEventMessageDll
//              parameter.
//
// Parameters:  lpszApp             - name of app (event log source).
//              lpszEventMessageDll - fully-qualified name of event message
//                                    DLL. Includes complete path and file
//                                    name. Example: "C:\\bin\\MyMessage.dll".
//                                    If this parameter is NULL, default is to
//                                    use exe's path + "XEventMessage.dll".
//
// Returns:     BOOL - TRUE = success
//
BOOL CXEventLog::RegisterSource(LPCTSTR lpszApp,
								LPCTSTR lpszEventMessageDll)
{
	_ASSERTE((lpszApp != NULL) && (lpszApp[0] != _T('\0')));
	if (!lpszApp || lpszApp[0] == _T('\0'))
		return FALSE;

	TCHAR szRegPath[] =
		_T("SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\");

	TCHAR szKey[_MAX_PATH*2]; // Flawfinder: ignore
	memset(szKey, 0, _MAX_PATH*2*sizeof(TCHAR));
	wcsncpy(szKey, szRegPath, MAX_PATH*2-2); // Flawfinder: ignore
	wcsncat(szKey, lpszApp, MAX_PATH*2-2); // Flawfinder: ignore

	// open the registry event source key
	DWORD dwResult = 0;
	HKEY hKey = NULL;
	LONG lRet = ::RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwResult);

	if (lRet == ERROR_SUCCESS)
	{
		// registry key was opened or created -

		// === write EventMessageFile key ===

		TCHAR szPathName[_MAX_PATH*2]; // Flawfinder: ignore
		memset(szPathName, 0, _MAX_PATH*2*sizeof(TCHAR));

		if (lpszEventMessageDll)
		{
			// if dll path was specified use that - note that this
			// must be complete path + dll filename
			wcsncpy(szPathName, lpszEventMessageDll, _MAX_PATH*2-2);  // Flawfinder: ignore
		}
		else
		{
			// use app's directory + "XEventMessage.dll"
			::GetModuleFileName(nullptr, szPathName, _MAX_PATH*2-2);
		}

		::RegSetValueEx(hKey,  _T("EventMessageFile"), 0, REG_SZ,
			reinterpret_cast<const BYTE *>( szPathName ), static_cast<DWORD>( (wcslen(szPathName) + 1)*sizeof(TCHAR) ) ); // Flawfinder: ignore

		// === write TypesSupported key ===

		// message DLL supports all types
		DWORD dwSupportedTypes = EVENTLOG_ERROR_TYPE		|
								 EVENTLOG_WARNING_TYPE		|
								 EVENTLOG_INFORMATION_TYPE	|
								 EVENTLOG_AUDIT_SUCCESS		|
								 EVENTLOG_AUDIT_FAILURE;

		::RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD,
			(const BYTE *) &dwSupportedTypes, sizeof(DWORD));

		::RegCloseKey(hKey);

		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetUserSid() - INTERNAL METHOD
//
// Purpose:     Get SID of current user
//
// Parameters:  none
//
// Returns:     PSID - pointer to alloc'd SID; must be freed by caller.
//              Example:  HeapFree(GetProcessHeap(), 0, pSid);
//
PSID CXEventLog::GetUserSid()
{
	HANDLE       hToken   = NULL;
	PTOKEN_USER  ptiUser  = NULL;
	DWORD        cbti     = 0;

	// get calling thread's access token
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,	&hToken))
	{
		if (GetLastError() != ERROR_NO_TOKEN)
			return NULL;

		// retry for process token if no thread token exists
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			return NULL;
	}

	// get size of the user information in the token
	if (GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti))
	{
		// call should have failed due to zero-length buffer.
		return NULL;
	}
	else
	{
		// call should have failed due to zero-length buffer
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return NULL;
	}

	// allocate buffer for user information in the token
	ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);
	if (!ptiUser)
		return NULL;

	// retrieve the user information from the token
	if (!GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti))
		return NULL;

	DWORD dwLen = ::GetLengthSid(ptiUser->User.Sid);

	// allocate buffer for SID
	PSID psid = (PSID) HeapAlloc(GetProcessHeap(), 0, dwLen);
	if (!psid)
		return NULL;

	BOOL bRet = ::CopySid(dwLen, psid, ptiUser->User.Sid);
	if (!bRet)
		return NULL;

	// close access token
	if (hToken)
		CloseHandle(hToken);

	if (ptiUser)
		HeapFree(GetProcessHeap(), 0, ptiUser);

	return psid;
}

///////////////////////////////////////////////////////////////////////////////
//
// SetAppName() - INTERNAL METHOD
//
// Purpose:     Stores name of current app
//
// Parameters:  lpszApp - app name
//
// Returns:     none
//
void CXEventLog::SetAppName(LPCTSTR lpszApp)
{
	if (!lpszApp)
		return;
	if (!m_pszAppName)
		m_pszAppName = new TCHAR [_MAX_PATH*2];
	if (m_pszAppName)
	{
		memset(m_pszAppName, 0, _MAX_PATH*2*sizeof(TCHAR));
		wcsncpy(m_pszAppName, lpszApp, _MAX_PATH*2-2); // Flawfinder: ignore
	}
}
