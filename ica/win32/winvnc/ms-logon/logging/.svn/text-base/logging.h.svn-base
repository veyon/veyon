
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTH_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTH_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef LOGGING_EXPORTS
#define LOGGING_API __declspec(dllexport)
#else
#define LOGGING_API __declspec(dllimport)
#endif
#if defined( UNICODE ) || defined( _UNICODE )
#error Sorry -- please compile as an ANSI program.
#endif
#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <lm.h>
#include <stdio.h>
#pragma hdrstop
LOGGING_API void LOGEXIT(char *machine);

class EventLogging
{
public:
	EventLogging();
	virtual ~EventLogging();
	virtual void LogIt(WORD CategoryID, DWORD EventID, LPCTSTR *ArrayOfStrings,
		UINT NumOfArrayStr = 0,LPVOID RawData = NULL,DWORD RawDataSize = 0);
	HANDLE m_hEventLinker;
	void AddEventSourceToRegistry(LPCTSTR lpszSourceName);

};

