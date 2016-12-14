/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
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
// /macine-vnc Greg Wood (wood@agressiv.com)
#include "logging.h"

/////////////////////////

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////

EventLogging::EventLogging()
{
	// returns a handle that links the source to the registry 
	m_hEventLinker = RegisterEventSource(NULL,"UltraVNC");

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

    lstrcat(szKey, _T("UltraVNC"));

    if(RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hk) != ERROR_SUCCESS)
    {
        return;
    }

    if (GetModuleFileName(NULL, szServicePath, MAX_PATH))
		{
			char* p = strrchr(szServicePath, '\\');
			if (p == NULL) return;
			*p = '\0';
			strcat (szServicePath,"\\logmessages.dll");
		}
	//printf(szServicePath);
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



/////////////////////////
///////////////////////
LOGGING_API
void LOGEXIT(char *machine)
{
	    FILE *file;
		const char* ps[3];
		char texttowrite[512];
		SYSTEMTIME time;
		GetLocalTime(& time);
		char			szText[256];
		sprintf(szText,"%d/%d/%d %d:%.2d   ", time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute );
		strcpy(texttowrite,szText);
		strcat(texttowrite,"Client ");
		strcat(texttowrite,machine);
		strcat(texttowrite," disconnected");
		strcat(texttowrite,"\n");
		ps[0] = texttowrite;
	    EventLogging log;
		log.AddEventSourceToRegistry(NULL);
		log.LogIt(1,0x00640003L, ps,1,NULL,0);

		char szMslogonLog[MAX_PATH];
		if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
		{
			char* p = strrchr(szMslogonLog, '\\');
			if (p != NULL)
			{
				*p = '\0';
				strcat (szMslogonLog,"\\mslogon.log");
			}
		}
		file = fopen(szMslogonLog, "a");
		if(file!=NULL) 
			{
				fwrite( texttowrite, sizeof( char ), strlen(texttowrite),file);
				fclose(file);
			}
}

LOGGING_API
void LOGLOGON(char *machine)
{
		FILE *file;
		const char* ps[3];
		char texttowrite[512];
		SYSTEMTIME time;
		GetLocalTime(& time);
		char			szText[256];
		sprintf(szText,"%d/%d/%d %d:%.2d   ", time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute );
		strcpy(texttowrite,szText);
		strcat(texttowrite,"Connection received from ");
		strcat(texttowrite,machine);
		strcat(texttowrite,"\n");
		ps[0] = texttowrite;
	    EventLogging log;
		log.AddEventSourceToRegistry(NULL);
		log.LogIt(1,0x00640001L, ps,1,NULL,0);
		char szMslogonLog[MAX_PATH];
		if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
		{
			char* p = strrchr(szMslogonLog, '\\');
			if (p != NULL)
			{
				*p = '\0';
				strcat (szMslogonLog,"\\mslogon.log");
			}
		}
		file = fopen(szMslogonLog, "a");
		if(file!=NULL) 
			{
				fwrite( texttowrite, sizeof( char ), strlen(texttowrite),file);
				fclose(file);
			}
}

LOGGING_API
void LOGFAILED(char *machine)
{
		FILE *file;
		const char* ps[3];
		char texttowrite[512];
		SYSTEMTIME time;
		GetLocalTime(& time);
		char			szText[256];
		sprintf(szText,"%d/%d/%d %d:%.2d   ", time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute );
		strcpy(texttowrite,szText);
		strcat(texttowrite,"Invalid attempt from client ");
		strcat(texttowrite,machine);
		strcat(texttowrite,"\n");
		ps[0] = texttowrite;
	    EventLogging log;
		log.AddEventSourceToRegistry(NULL);
		log.LogIt(1,0x00640002L, ps,1,NULL,0);
		char szMslogonLog[MAX_PATH];
		if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
		{
			char* p = strrchr(szMslogonLog, '\\');
			if (p != NULL)
			{
				*p = '\0';
				strcat (szMslogonLog,"\\mslogon.log");
			}
		}
		file = fopen(szMslogonLog, "a");
		if(file!=NULL) 
			{
				fwrite( texttowrite, sizeof( char ), strlen(texttowrite),file);
				fclose(file);
			}
}

LOGGING_API
void LOGLOGONUSER(char *machine,char *user)
{
		FILE *file;
		const char* ps[3];
		char texttowrite[512];
		SYSTEMTIME time;
		GetLocalTime(& time);
		char			szText[256];
		sprintf(szText,"%d/%d/%d %d:%.2d   ", time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute );
		strcpy(texttowrite,szText);
		strcat(texttowrite,"Connection received from ");
		strcat(texttowrite,machine);
		strcat(texttowrite," using ");
		strcat(texttowrite,user);
		strcat(texttowrite," account ");
		strcat(texttowrite,"\n");
		ps[0] = texttowrite;
	    EventLogging log;
		log.AddEventSourceToRegistry(NULL);
		log.LogIt(1,0x00640001L, ps,1,NULL,0);
		char szMslogonLog[MAX_PATH];
		if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
		{
			char* p = strrchr(szMslogonLog, '\\');
			if (p != NULL)
			{
				*p = '\0';
				strcat (szMslogonLog,"\\mslogon.log");
			}
		}
		file = fopen(szMslogonLog, "a");
		if(file!=NULL) 
			{
				fwrite( texttowrite, sizeof( char ), strlen(texttowrite),file);
				fclose(file);
			}
}

LOGGING_API
void LOGFAILEDUSER(char *machine, char *user)
{
		FILE *file;
		const char* ps[3];
		char texttowrite[512];
		SYSTEMTIME time;
		GetLocalTime(& time);
		char			szText[256];
		sprintf(szText,"%d/%d/%d %d:%.2d   ", time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute );
		strcpy(texttowrite,szText);
		strcat(texttowrite,"Invalid attempt from client ");
		strcat(texttowrite,machine);
		strcat(texttowrite," using ");
		strcat(texttowrite,user);
		strcat(texttowrite," account ");
		strcat(texttowrite,"\n");
		ps[0] = texttowrite;
	    EventLogging log;
		log.AddEventSourceToRegistry(NULL);
		log.LogIt(1,0x00640002L, ps,1,NULL,0);
		char szMslogonLog[MAX_PATH];
		if (GetModuleFileName(NULL, szMslogonLog, MAX_PATH))
		{
			char* p = strrchr(szMslogonLog, '\\');
			if (p != NULL)
			{
				*p = '\0';
				strcat (szMslogonLog,"\\mslogon.log");
			}
		}
		file = fopen(szMslogonLog, "a");
		if(file!=NULL) 
			{
				fwrite( texttowrite, sizeof( char ), strlen(texttowrite),file);
				fclose(file);
			}
}

