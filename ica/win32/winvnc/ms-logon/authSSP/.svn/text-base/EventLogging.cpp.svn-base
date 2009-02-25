/////////////////////////////////////////////////////////////////////////////
//	Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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
//
#include "EventLogging.h"


EventLogging::EventLogging()
{
	// returns a handle that links the source to the registry 
	m_hEventLinker = RegisterEventSource(NULL,_T("UltraVnc"));

}

EventLogging::~EventLogging()
{
	// Releases the handle to the registry
	DeregisterEventSource(m_hEventLinker);
}



void EventLogging::LogIt(WORD CategoryID, DWORD EventID, LPCTSTR *ArrayOfStrings,
						 UINT NumOfArrayStr,LPVOID RawData,DWORD RawDataSize)
{

	// Writes data to the event log
	ReportEvent(m_hEventLinker,EVENTLOG_INFORMATION_TYPE,CategoryID,
		EventID,NULL,1,RawDataSize,ArrayOfStrings,RawData);	

}


void EventLogging::AddEventSourceToRegistry(LPCTSTR lpszSourceName)
{
    HKEY  hk;
    DWORD dwData;
    TCHAR szBuf[MAX_PATH];
    TCHAR szKey[255] =_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    TCHAR szServicePath[MAX_PATH];

    lstrcat(szKey, _T("UltraVnc"));

    if(RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hk) != ERROR_SUCCESS)
    {
        return;
    }

    if (GetModuleFileName(NULL, szServicePath, MAX_PATH))
		{
			TCHAR* p = _tcsrchr(szServicePath, '\\');
			if (p == NULL) return;
			*p = '\0';
			_tcscat (szServicePath,_T("\\logmessages.dll"));
		}
    lstrcpy(szBuf, szServicePath);

    // Add the name to the EventMessageFile subkey.
    if(RegSetValueEx(hk,
                     _T("EventMessageFile"),
                     0,
                     REG_EXPAND_SZ,
                     (LPBYTE) szBuf,
                     (lstrlen(szBuf) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hk);
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |EVENTLOG_INFORMATION_TYPE;
    if(RegSetValueEx(hk,
                     _T("TypesSupported"),
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwData,
                     sizeof(DWORD)) != ERROR_SUCCESS)
    {
        
    } RegCloseKey(hk);
}

void LOG(long EventID, const TCHAR *format, ...) {
    FILE *file;
	TCHAR szMslogonLog[MAX_PATH];
	LPCTSTR ps[3];
	TCHAR textbuf[2 * MAXLEN] = _T("");
	char texttowrite[2 * MAXLEN] = "";
	TCHAR szTimestamp[MAXLEN] = _T("");
	TCHAR szText[MAXLEN] = _T("");
	SYSTEMTIME time;

	va_list ap;
	va_start(ap, format);
	_vstprintf(szText, format, ap);
	va_end(ap);
	ps[0] = szText;
    EventLogging log;
	log.AddEventSourceToRegistry(NULL);
	log.LogIt(1,EventID, ps,1,NULL,0);
	if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
	{
		TCHAR *p = _tcsrchr(szMslogonLog, '\\');
		if (p != NULL)
		{
			*p = '\0';
			_tcscat (szMslogonLog,_T("\\mslogon.log"));
		}
	}
	file = _tfopen(szMslogonLog, _T("a"));
	if(file!=NULL) 
	{
		// Prepend timestamp to message
		GetLocalTime(& time);
		_stprintf(szTimestamp,_T("%.2d/%.2d/%d %.2d:%.2d:%.2d  "), 
			time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
		_tcscpy(textbuf,szTimestamp);
		_tcscat(textbuf,szText);

		// Write ANSI
#if defined UNICODE || defined _UNICODE
		wcstombs(texttowrite, textbuf, 2 * MAXLEN);
#else
		strcpy(texttowrite, texttowrite);
#endif
		fwrite(texttowrite, sizeof(char), strlen(texttowrite),file);
		fclose(file);
	}
}
  