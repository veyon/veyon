// XEventLog.h  Version 1.0
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

#pragma once

class CXEventLog
{
// Construction
public:
	CXEventLog(LPCTSTR lpszApp = nullptr, LPCTSTR lpszEventMessageDll = nullptr);
	~CXEventLog();

// Attributes
public:
	LPTSTR	GetAppName();

// Operations
public:
	void	Close();
	BOOL	Init(LPCTSTR lpszApp, LPCTSTR lpszEventMessageDll = nullptr);
	BOOL	Write(WORD wType, LPCTSTR lpszMessage);

// Implementation
private:
	HANDLE	m_hEventLog;
	LPTSTR	m_pszAppName;
	PSID	GetUserSid();
	BOOL	RegisterSource(LPCTSTR lpszApp, LPCTSTR lpszEventMessageDll);
	void	SetAppName(LPCTSTR lpszApp);
};
